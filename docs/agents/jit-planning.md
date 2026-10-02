# JIT planning

What stands between Fraze and a JIT backend, with MIR as the backend. Paths are relative to `compiler/source/fraze/`.

The backend is an off-the-shelf JIT library, which brings its own optimizations and portability. Writing a code emitter for Fraze is ruled out.

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
- **Externs.** `IExternalFunction::Invoke(Program*, Word* argsEnd)` reads its arguments and writes its result at fixed offsets below `argsEnd`, in the VM stack's layout: the first argument nearest `argsEnd`, and the return storage below the last argument.
- **Host entry.** `Program::InvokeImpl` builds a VM frame from a `Word` array.
- **GC roots.** `Heap::CollectInternal` conservatively scans the globals and the VM stack, but compiled code keeps references in registers and on the native stack. A collection can also start on the worker thread (asset loading allocates through `ScopedAllocator` on it) while the main thread is running.
- **Failures.** `Debug.Fail` and `ENFORCE` throw C++ exceptions that `demo/source/main.cpp` catches, and externs can throw too. Neither an exception nor a plain `longjmp` can cross compiled frames; see **Failures** under the recommended approach.
- **VM-only diagnostics.** Per-operation source locations drive the heap debug locations, `FRAZE_PRINT_EXECUTED_CODE` and `ExportBytecode`; compiled code has none.

## Recommended approach

- **Generate MIR from the lowered AST,** with a second `ASTVisitor` beside `CodeGenerator`, rather than translating bytecode. The AST is typed and fully lowered, so the generator is mechanical. Whether the bytecode VM stays, as a reference or debug backend, is undecided.
- **Externs:** call a C-ABI bridge that takes a block holding the return storage and arguments in that layout, and calls `Invoke` with its end. Later, call the C++ function directly: `ExternalFunction<Func>` already holds both its address and its signature.
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
