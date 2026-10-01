# Fraze

Fraze is a strongly typed, garbage-collected scripting language for games, built from scratch in C++ (MSVC v145, `stdcpplatest`, Windows only). It compiles to stack-based bytecode that runs on its own VM. Goals: memory safety, simplicity, predictable behavior, fast execution. The long-term plan is a JIT, so the interpreter is being simplified toward primitive, assembly-like instructions.

## Build, run, test

Commands are PowerShell, run from the repository root.

**Build** Debug, unless you're profiling (then Release). Never build both. On a fresh clone, run `third_party\build-all.cmd` once first.

```powershell
$msbuild = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest -requires Microsoft.Component.MSBuild -find "MSBuild\**\Bin\MSBuild.exe"
& $msbuild Fraze.sln /p:Configuration=Debug /p:Platform=x64 /m /v:minimal /nologo
```

**Test** by running the demo. Its `main()` in `demo/assets/scripts/Main.fz` calls `RunAllTests()` (`compiler/assets/fraze.tests.fz`) and `MathTests.Run()` (`demo/assets/scripts/Tests.fz`), then opens a 3D scene that never exits on its own. `.fz` files are compiled at startup, so script-only edits need no rebuild.

```powershell
New-Item -ItemType Directory -Force demo\output | Out-Null
$p = Start-Process demo\bin\x64\Debug\Demo.exe -WorkingDirectory demo -PassThru `
    -RedirectStandardOutput demo\output\stdout.txt -RedirectStandardError demo\output\stderr.txt
if(!$p.WaitForExit(15000)) { $p.CloseMainWindow() | Out-Null; if(!$p.WaitForExit(10000)) { Stop-Process -Id $p.Id -Force } }
Get-Content demo\output\stdout.txt, demo\output\stderr.txt
```

- `-WorkingDirectory demo` is required because assets resolve relative to it. Close with `CloseMainWindow()`; force-killing loses buffered stdout.
- **Pass:** stdout ends with `Finished loading scene.` **Fail:** a compile error or failed `assert` prints `file(line,col): message` and the process exits early. The exit code is 0 either way, so read the output.
- Only Debug runs checks: Release calls `DisableAssert()`, `DisableNullCheck()`, `DisableBoundsCheck()` and `DisableTypeCheck()` in `demo/source/main.cpp`.
- Add language tests to `fraze.tests.fz` as a `section XxxTests` with a `void Run()` that uses `assert(...)`, and call it from `RunAllTests()`.
- Performance: measure in Release, on branchy gameplay-style code (e.g. a stress test in `Main.fz` timed with `Time.Now.Seconds` and printed with `Console.WriteLine`), not only demo FPS. The demo's hot path, `MeshRenderer.Deform`, is math-heavy and unrepresentative.

## Working in this repository

- The user works in parallel and usually has uncommitted changes, often in the files you touch. To undo your own temporary edit, write back the exact content you saved before editing. When you `stash`, `checkout`, `restore` or `reset`, scope it so it cannot discard changes that aren't yours. For compile probes, prefer a new scratch `.fz` file in `demo/assets/scripts/` (every `.fz` there is compiled), and delete it afterward.
- Commit only when explicitly asked in that turn, and never push. Commit messages have one brief line per major change, each starting with `-` and no space (e.g. `-add switch statements`), in plain ASCII. Never add `Co-Authored-By` or any other trailer, even if your tooling adds one by default.
- Propose language and compiler design changes before implementing them. The user makes design calls, one step at a time.
- When reporting back, cover what you did and anything that differs from what the user would expect. Don't affirm negatives: if you followed an instruction by not doing something (e.g. you didn't commit because you weren't asked to), leave it out. What already matches the user's expectations goes without saying.

## Layout

- `compiler/`: static library containing the lexer, parser, analyzer, code generator, VM and GC. C++ is under `compiler/source/fraze/`; the standard library (`fraze.*.fz`) and test suite are in `compiler/assets/`.
- `demo/`: 3D app that embeds Fraze. The C++ host is in `demo/source/` (extern functions are registered in `ExternFunctions.cpp`), and scripts are in `demo/assets/scripts/`.
- `syntax-extensions/`: syntax highlighting for Visual Studio and VS Code. The grammar is `fraze-syntax.json`.
- `third_party/`: the demo's external dependencies.

## Compiler pipeline

`Compiler` (`compiler/Compiler.*`) runs `Lexer -> Parser -> SemanticAnalyzer -> CodeGenerator` and returns an `sptr<Program>`. Paths below are relative to `compiler/source/fraze/`.

- **Lexer** (`compiler/Lexer.*`): source text to tokens. Keywords are in the `Keywords`/`KeywordNames` tables.
- **Parser** (`compiler/Parser.h`, header-only): builds the AST rooted at `ASTRoot`, with nodes in `ast/def`, `ast/expr`, `ast/stmt` and `ast/type`. It adds the implicit `this` as parameter 0 of every function where `FunctionDefinition::HasImplicitThisParam()` is true. It also turns each `Task` function into a state class (`ParseTaskObject`), with its locals hoisted into fields.
- **SemanticAnalyzer** (`compiler/SemanticAnalyzer.*`, an `ASTVisitor`): resolves scopes, types and overloads, validates, and lowers sugar into plain AST:
  - runtime checks become calls to the helpers in `fraze.system.fz` (`Wrap*`);
  - `await` becomes a `$position` switch plus gotos (`LowerAwait`);
  - temporaries use `CreateTempFold`.

  A member access gets its receiver as `node->context` (`this`, or `$this` inside a coroutine), and `VisitCallTarget` passes it as the first argument. By code generation, `this` is an ordinary argument.
- **CodeGenerator** (`compiler/CodeGenerator.*`): emits stack-based bytecode, similar to C# IL. It assigns everything numeric: frame and field offsets, check-site ids and jump targets.
- **Program** (`program/Program.*`): the VM. It owns the stack, globals, `Heap`, `TypeInfo` and `checkSites`. `OpCode` (`program/OpCode.h`) is a `uint8_t` enum, and each opcode has an `Execute_*` handler. `Dispatcher` runs async work, and `program->Invoke("main", args)` runs inside a `ScopedAllocator`.
- **GC** (`memory/Heap.*`): mark-and-sweep. It scans the VM stack conservatively, and `PinMemory` keeps objects in place for native code.
- **Built-in externs** (`Debug.Fail`, `Type.NewClass`, ...) are in `compiler/NativeFunctions.h` and registered in `Compiler.cpp`. Allocation is an extern call, not an opcode.

## Design principles

These serve two goals: being able to move or delete AST nodes freely, and keeping the compiler simple enough to understand and refactor. They are strong defaults, not hard rules. When following one would make the code significantly more complicated, simplicity wins; say so explicitly.

- **SemanticAnalyzer outputs a resolved AST and nothing else.** Ids, indices, offsets and side-table entries are assigned in CodeGenerator. If a lowering needs a runtime table entry, add a self-contained node that carries its own data (for example, a node holding a runtime check's message), and let CodeGenerator assign the id.
- **The AST flows downward.** No parent pointers, and no new references from nodes into generated collections. The existing `Scope*`, `targetDef` and `Type*` links are exceptions, not precedent.
- **Analyzer visits are idempotent**, because the AST is mutated, cloned and re-visited during analysis.
- **Infer node state instead of storing it.** Don't add a node field for anything derivable from the AST. If something really has to be tracked, prefer restructuring (e.g. lowering earlier). `Clone` must copy every field.
- **Keep opcodes primitive**, for the future JIT. Never add a composite opcode (e.g. `CheckNull`) as a speed fix; express it with existing primitives.
- **Keep CodeGenerator changes small and in place:** a bit of member state checked at the exact site that changes (for example, a member recording which expression's result the current statement will discard). Avoid flag parameters, `Emit*(node, bool)` wrappers, and special-case lists of node kinds.

## C++ conventions

- Namespace `fraze`. Use `sptr`/`spnew` from `common/Pointers.h`.
- PascalCase for types and methods; camelCase for locals and fields. One primary type per file.
- Add every new `.h`/`.cpp` to its `.vcxproj` and `.vcxproj.filters`.
- Every source file (`.h`, `.cpp`, `.fz`) starts with this banner:
  ```
  /*--------------------------------------------------------------*
  *  Copyright (c) 2026 Nicolas Jinchereau. All rights reserved.  *
  *---------------------------------------------------------------*/
  ```
- **Names:**
  - Balance brevity against meaning, but never drop an operative word to save space. The point is to avoid losing meaning, not to spell out a sentence. The usual short forms (`Arg`, `Var`, `Def`, `Expr`, `Ident`) are fine.
  - Give different views of the same thing congruent names: one shared stem, different suffixes. `ident` is fine on its own, but if one scope holds both the expression and its value, name them `identExpr` and `identValue`, not `ident` and `resolvedValue`.
  - Name a value after what it is, with a noun phrase, not a verb phrase or a process it takes part in: `expectedValue`, not `jumpWhen`; `isTrustedInput`, not `skipsValidation`.
  - Make a name informative rather than redundant: surface what the reader can't see where it's used, but through what the thing is, not through internals they'd have to go and read, since a name is read before the feature is understood. A field named `finalizer` is a good model: a noun that says what it is and gives insight into its purpose without naming anything hidden. For a function, that includes side effects: one isn't named like a query (`TryTakeCachedResult`, not `HasCachedResult`), and an effect beyond what the caller can already read is named (`TryCancelScheduledSave`, not `TryTakePendingSave`).
  - Don't rename existing code unless asked.
- **Comments:** one per function, on its definition, and on a field whose name can't carry its meaning.
  - The first sentence says concretely what the thing does, returns or holds, not what it isn't; the why comes after, briefly.
  - Name the calls made and the state changed rather than a verb that stands in for them: "stores the value, marks it done and calls Finish", not "completes the operation".
  - Leave out what the reader can infer: which pass does a step when only one pass does that kind of work, or a property that follows from how the thing was built.
  - Describe a node by what the source wrote and what it lowers to, not by the other parts that lowering emits.
  - Usually 1–2 lines; a mechanism spanning several functions can take about 7.
  - At compile time, code "emits"; the emitted code "evaluates", "pushes" or "jumps".

## Extending the compiler

- **AST node:** grep an existing node (e.g. `SwitchStatement`) and mirror every site:
  - its header in `ast/*/`;
  - `ASTFwd.h`, a `To*()` in `ASTNode.h`, and `AST.h`;
  - `Visit` in `ASTVisitor`, `ASTPrinter`, `CodePrinter` and the passes that handle it;
  - the project files.
- **Opcode:** grep an existing one (e.g. `NotEqualN`) and mirror every site: the enum and `OpCodeNames` in `OpCode.h`, the declaration and `handlers` entry in `Program.h`, and `Execute_*`, `VerifyHandlers` and operand printing in `Program.cpp`.
- **Keyword:** `Keyword`, `Keywords` and `KeywordNames` in `Lexer.h`, plus the grammar in `syntax-extensions/fraze-syntax.json`.

## Writing Fraze

The syntax is C#-like; `compiler/assets/fraze.tests.fz` has examples of every feature. Use PascalCase for types and methods, camelCase for locals.

- **Types:**
  - primitives: `bool`, `int` (64-bit), `num` (double), `string`, `object`;
  - `class`, `struct` (value type), `interface`, `enum`, and `functor` (callable type);
  - generics (`List<T>`, `Table<K, V>`), arrays (`T[]`), properties and operator overloads.
- `section` is a namespace. `extern` declarations bind to C++ functions registered with `Compiler::AddFunction` or `AddIntrinsic`.
- `fold { stmts; expr; }` is an expression whose value is its final expression statement.
- `switch` doesn't fall through and accepts `case 1, 2:`. `goto name;` jumps to a `name:` label. There is no `break` or `continue` yet.
- Coroutines return `Task<T>`. `await` works anywhere in an expression except a loop condition, an assignment target, the right of a compound assignment, and inside a fold.
- Structs and enums can't be converted to `object`, because boxing isn't implemented.

## Debugging the compiler

Call these on the `Compiler` in `demo/source/main.cpp`, before `Compile()`. Paths are relative to `demo/`; use `output/...`, which is git-ignored.

- `ExportAST(path)`: the AST after parsing and before analysis, written to `AST.txt`.
- `ExportLoweredCode(path)`: the AST after analysis, printed as Fraze-like pseudo-code. It's usually the quickest way to see what a lowering did.
  - It shows the true lowered form: the receiver as first argument, fully qualified call targets, runtime checks as calls, and pseudo-forms like `cast<T>(x)` and `default(T)` for nodes without syntax.
  - It writes one `.fz` per source file, plus `__global.fz`.
  - The output isn't meant to compile.
- `ExportBytecode(path)`: one CSV per source file, plus `all-sections.csv`.
- Macros in `common/Platform.h`:
  - `FRAZE_PRINT_EXECUTED_CODE` traces every executed op (very slow);
  - `FRAZE_CODE_PROFILING` counts executed instructions (`Program::DumpCodeProfile`);
  - `FRAZE_HEAP_DEBUG` is for GC debugging.

## Writing rules in this file

- What a rule is for decides whether it names code. A rule describing a convention is read before the code it governs is understood, so it states the principle without pointing at specific variables, functions or files, and any examples in it are standalone illustrations. Instructions, descriptions of the layout and pipeline, and rules whose purpose is to single out something in the codebase (an exception, or how to handle one particular part) do name code and paths.
- Leave out history: who asked for a rule, when, or what it replaced.
- One idea per rule. When a general rule overlaps a more specific one, merge them rather than keeping both.
