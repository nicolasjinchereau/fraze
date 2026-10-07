# JIT planning

What stands between Fraze and a JIT backend, with MIR as the backend. Paths are relative to `compiler/source/fraze/`.

The backend is an off-the-shelf JIT library, which brings its own optimizations and portability. Writing a code emitter for Fraze is ruled out.

## Current state

- **Pipeline:** `Compiler::SetCodeGenerator(CodeGenerator::JIT)` makes `Compile` run `JITCodeGenerator` (`compiler/JITCodeGenerator.*`) instead of `VMCodeGenerator`, and return a `JITProgram` (`program/JITProgram.*`), which owns the MIR context and the machine code in it. Only the `.cpp` files and `JITCodeGenerator.h` include MIR's headers; `JITProgram.h` forward-declares `MIR_context`.
- **Generated so far:** every function in the program, over `bool`, `int` and `num` only, which `MIRTypeOf` maps to `I64` and `D`; any other type, a `ref` parameter or a static variable is an error. In a body: blocks, `return`, literals, direct calls, locals, assignment and reads of a local or parameter. Arithmetic, conversion and every other statement and expression throw "the JIT can't generate X yet" at their location. Parameters and locals are MIR registers, named by index because Fraze allows shadowing and MIR rejects a repeated register name; `variableRegisters` maps each definition to its register.
- **Names:** a MIR item name is `qualifiedName:signature` (`FunctionDefinition::GetSignature`), since overloads share a qualified name and MIR item names must be unique. `functionItems` is keyed on the function's `Type*` for the same reason, and `protoItems` on the signature, so one proto serves every function that shares it. A call references a forward item, because MIR allows only one open function at a time, and MIR resolves it when the module is loaded.
- **Optimization:** `Compiler::SetOptimization` takes an `Optimization` level that maps to `MIR_gen_set_optimize_level`, and the VM code generator ignores it. All four levels generate the current test.
- **Running:** `JITProgram::InvokeImpl` calls a function by qualified name with no arguments and returns its `int`, running the global `$staticConstructor` before the first one, as `VMProgram` does. The qualified name can't name one overload, so the first generated one wins, which is also what `Program::GetTypeInfo` does. `GetUsedStackRange` is empty, since compiled code can't allocate yet.
- **Testing:** the demo's `--jit` flag compiles `demo/assets/jit/MainTest.fz` alone, without `AddFrazeRuntime`, and exits with what `main` returns. `MainTest.fz` grows as the generator does, and the runtime comes back once the generator can compile it. Keep the expected values under 256: PowerShell's `$LASTEXITCODE` carries the whole `int`, but a shell that masks the exit status to 8 bits turns 300 into 44.
- **Errors:** MIR calls `ThrowMIRError`, which throws a `fraze::Exception` located at the node `JITCodeGenerator` is visiting, so a repeated item name, a wrong operand mode or a proto that doesn't match its call reports like any other compile error. An error out of `MIR_finish_func`, which validates a whole body at once, can only point at the function. The error func is a field of the context, so `MIRContext` installs the handler in its constructor, where a new context would otherwise keep MIR's print-and-exit default.
- **Memory:** `MIRContext` owns the context and both allocators it is created with. Nothing in MIR allocates outside them, so the destructor releases everything MIR allocated and `MIR_finish` is never called. `JITProgram` owns the context from construction, and `Visit(ASTRoot)` keeps the program itself local until generating has fully succeeded, so a throw anywhere in generation releases both and needs no handler. `realloc` moves a block, hence `intrusive_list::relink_neighbors`.
- **Forwards:** a call references a forward item, so a call to a function the generator never defines leaves one unresolved, which spins `MIR_link` instead of reaching `ThrowMIRError`. `Visit(ASTRoot)` checks `ref_def` on every item in `functionItems` before loading the module, because `add_item` sets it on both items as soon as a forward and a definition share a name, whichever came first. This is the check to watch when externs start generating imports or forwards of their own.
- **Reference material absent from `third_party/mir`:** `MIR.md` and `CUSTOM-ALLOCATORS.md` ship only in a full MIR checkout. `MIR.md` documents the error func as `(MIR_context ctx, MIR_error_type_t, const char *message)`, which is stale - both 0.2 headers declare `(MIR_error_type_t, const char *format, ...)`. There is no `ctx` parameter, which is why `ThrowMIRError` reads the location from a file-static. No tool in the MIR tree calls `MIR_set_error_func`, and nothing in it handles a node whose storage moved.

## Already compatible

- **Globals** live in `Program::globals`, a fixed-address array, which a JIT can address directly or import as one symbol.
- **Static initialization** is ordinary functions: the global section's `$staticConstructor` is the entry point.
- **Coroutines** are lowered to task classes driven by a `$position` switch, so nothing needs stack switching.
- **Runtime checks and allocation** are calls (`Wrap*` helpers, `Type.NewClass`), not special opcodes.
- **Dynamic dispatch** exists only for interface calls.

## MIR on Windows

Windows x64 isn't an official MIR target: the Windows ABI is implemented but minimally tested ([discussion](https://github.com/vnmakarov/mir/discussions/345)). MIR builds as a static library with MSVC v143 (MIR's generated `projects/mir.vcxproj`, which defines `MIR_PARALLEL_GEN`). Checked in Debug x64:

- **Works:**
  - eager generation (`MIR_set_gen_interface`);
  - compiled code calling C functions, varargs included, and C++ functions taking and returning `double`;
  - the host calling compiled code, and host → compiled → host → compiled reentrancy;
  - a `longjmp` through compiled frames when the jump buffer's `Frame` field is cleared after `setjmp` (`reinterpret_cast<_JUMP_BUFFER*>(&buffer)->Frame = 0`), which makes it restore registers without unwinding. Compiled code still runs correctly afterwards;
  - the landing-pad scheme under **Failures** below, including two chained pads across two compiled segments, run twice in a row.
- **Fails:**
  - a C++ exception thrown by a host function called from compiled code ends the process with `0xC0000409` instead of reaching the host's `catch`;
  - a plain `longjmp` through compiled frames does the same, because MSVC's x64 `longjmp` unwinds like an exception.

The failures have one cause: Win64 unwinding requires registered unwind data (`RtlAddFunctionTable`) for every frame it walks, and MIR emits none. That also stops debuggers and profilers walking compiled frames.

The whole program is compiled up front. Lazy generation isn't used, and on Win64 it crashes on the first call.

## Blockers

- **Untyped bytecode.** Loads and stores (`PushLocal`, `PushArgument`, `PushWord`, `PopGlobal`, ...) move untyped words. MIR keeps integer and floating-point registers apart and has no bitcast, only value conversions, so translating bytecode would need a type on every load and store. Stack-only operations (`Dup`, `DupN`, `Pop` counts, the return storage a caller reserves) have no register equivalent.
- **Calling convention.** VM frames keep arguments below `rbp`, and the caller reserves return storage that `Return` copies into. A native convention needs:
  - typed parameters;
  - structs passed and returned by address (MIR returns only a few values in registers, and `Mat4` is 16 words);
  - memory (`ALLOCA`) for any local whose address is taken (`PushLocalAddr`, `ref` parameters, struct receivers passed by reference).
- **Call targets.** `Call` and `CallExternal` carry a `typeInfo` index that the VM resolves at run time; a JIT resolves direct calls when compiling. `CallVirtual` goes through `Class::GetFunctionID`, a linear scan of the class's interfaces that returns a function id, then through `typeInfo` to `codeStart`. Compiled code needs dispatch data that yields a code pointer in a couple of loads.
- **Externs.** `IExternalFunction::Invoke(Program*, Word* argsEnd)` reads its arguments and writes its result at fixed offsets below `argsEnd`, in the VM stack's layout: the first argument nearest `argsEnd`, and the return storage below the last argument. Two MIR constraints shape any direct call instead:
  - **Aggregates are argument-only.** `MIR_T_BLK` through `MIR_T_RBLK` "can be used only args" (`mir.h`), and the five `BLK` variants encode a C ABI's register classification, so passing a struct by value needs the variant the callee's ABI expects, and returning one needs `RBLK` (the address of return storage) or an explicit pointer parameter. Passing every struct by address avoids the classification entirely.
  - **`Boolean` is `bool`, one byte.** Mapping it to `I64` is right inside compiled code and right for an argument, since the callee reads AL, but a C++ function returning `bool` leaves the high bits of RAX undefined. A `bool` result wants `U8` plus an explicit extension. `ExternalFunction::Invoke` already special-cases Boolean for the same reason.
- **Host entry.** `VMProgram::InvokeImpl` builds a VM frame from a `Word` array.
- **GC roots.** `Heap::CollectInternal` conservatively scans the globals and the VM stack, but compiled code keeps references in registers and on the native stack. A collection can also start on the worker thread (asset loading allocates through `ScopedAllocator` on it) while the main thread is running.
- **Failures.** `Debug.Fail` and `ENFORCE` throw C++ exceptions that `demo/source/main.cpp` catches, and externs can throw too. Neither an exception nor a plain `longjmp` can cross compiled frames; see **Failures** under the recommended approach.
- **VM-only diagnostics.** Per-operation source locations drive the heap debug locations, `FRAZE_PRINT_EXECUTED_CODE` and `ExportBytecode`; compiled code has none.

## Recommended approach

- **Generate MIR from the lowered AST,** with a second `ASTVisitor` beside `VMCodeGenerator`, rather than translating bytecode. The AST is typed and fully lowered, so the generator is mechanical. Whether the bytecode VM stays, as a reference or debug backend, is undecided.
- **Externs:** call a C-ABI bridge that takes a block holding the return storage and arguments in that layout, and calls `Invoke` with its end. Later, call the C++ function directly: `ExternalFunction<Func>` already holds both its address and its signature. A third option sits between them and is cheaper than it looks, because `Func` is a template argument rather than a member: instantiate a `static` shim per extern whose MIR-visible signature is scalars and pointers only, so MIR never sees an aggregate. The spike at `CPPTest` has the C++-signature half of the type mapping (`MIRTypeOf<T>` traits over Boolean/Integer/Number/Reference/`Object*`/enums), which pairs with the generator's `MIRTypeOf(Type*)`.
- **A signature-driven call thunk already exists.** `_MIR_get_ff_call` (`mir.h`) generates a function that takes a callee address and an array of pointers to the result and argument slots, and performs the call; MIR's interpreter uses it, and `mir-x86_64.c` has a real `_WIN32` path. It would implement **host entry** generically, off the `Word` span, cached per signature. It is an underscore-prefixed internal API, and the gen interface doesn't exercise it, so it needs a probe first.
- **Host entry:** one uniform native entry signature, or a thunk per entry point.
- **GC:** spill registers with `setjmp` (which doesn't unwind), then scan the native stack from the current stack pointer up to the thread's stack base. Worker-thread allocations don't start collections; the main thread does.
- **Failures:** C++ exceptions travel only through native segments of the stack, and non-unwinding `longjmp`s cross the compiled segments. This needs nothing from MIR.
  - **Landing pads:** every native → compiled edge is one: host entry, an extern calling back into Fraze, interpreter → compiled. It does a `setjmp`, then clears `Frame`. On landing, it restores VM state (`rsp`, `rbp`, `rip` and any JIT state) and rethrows the exception saved on the `Program`.
  - **Stubs:** every compiled → native edge is one: extern, runtime helper, compiled → interpreter. It catches all exceptions, stores `std::current_exception()` on the `Program`, leaves the `catch`, then `longjmp`s to the innermost landing pad. `Debug_Fail` keeps throwing, and its stub converts the exception. Only a native function that compiled code calls directly, with no stub, must not throw.
  - **Pad chain:** pads form a chain, per `Program` or `thread_local`, and each one pops itself before rethrowing.
  - **Constraints:**
    - Never `longjmp` from inside a `catch` block: MSVC keeps the exception object and the CRT's exception state live there.
    - Keep the function that calls `setjmp` minimal: no live objects with destructors, and `volatile` for locals changed after `setjmp`.
    - The `longjmp` must not unwind. Clearing `Frame` after `setjmp` works. `_setjmp(buffer, nullptr)` doesn't compile against MSVC's `setjmp.h`, which declares `_setjmp` with one parameter and has the compiler supply the frame, and a custom MASM `setjmp`/`longjmp` is the alternative.
    - `Frame` is an MSVC implementation detail of `_JUMP_BUFFER`.
    - Nothing here is linked `/CETCOMPAT`. If it ever is, the non-unwinding `longjmp` must also unwind the shadow stack (`incsspq`); whether the CRT's does with `Frame` cleared is unverified.
  - **Cost:** a `try` in a stub costs nothing until something throws. Each landing pad is a `setjmp` with a 256-byte buffer, which only matters if native → compiled entries are hot.
  - **Later:** the scheme extends to a Fraze `try`/`catch`, where a `try` becomes a landing pad, like Lua's `pcall`. Landing pads also fix the known bug where a `Program` can't run again after an exception escapes it.
