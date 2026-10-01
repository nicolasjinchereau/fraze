# Fraze

Fraze is a strongly typed, garbage-collected scripting language for games, built from scratch in C++ (MSVC v145, `stdcpplatest`, Windows only). It compiles to stack-based bytecode that runs on its own VM. Goals: memory safety, simplicity, predictable behavior, fast execution.

## Task docs

These aren't loaded automatically. Before starting a task, read the ones that match it:

- Changing the compiler, VM or GC: `docs/agents/compiler-architecture.md`, covering what each pass does, where it lives and the traps in it.
- Adding an AST node, opcode or keyword: `docs/agents/extending-the-compiler.md`.
- Writing or editing `.fz` code, tests included: `docs/agents/writing-fraze.md`.
- Adding or changing an extern, intrinsic or extern class, or C++ that holds or allocates Fraze objects: `docs/agents/native-interop.md`.
- Investigating a miscompile, a lowering or a crash: `docs/agents/debugging.md`, covering the AST, lowered-code and bytecode dumps and the trace macros.
- Hitting behavior that looks like a compiler bug, or fixing one: `docs/agents/known-bugs.md`.
- Measuring or optimizing speed: `docs/agents/performance.md`.
- Editing this file or anything in `docs/agents/`: `docs/agents/README.md`.

## Working in this repository

- The user works in parallel and usually has uncommitted changes, often in the files you touch.
  - To undo your own temporary edit, write back the exact content you saved before editing.
  - When you `stash`, `checkout`, `restore` or `reset`, scope it so it cannot discard changes that aren't yours.
  - For compile probes, prefer a new scratch `.fz` file in `demo/assets/scripts/` (every `.fz` there is compiled), and delete it afterward.
- Commit only when explicitly asked in that turn, and never push. Commit messages have one brief line per major change, each starting with `-` and no space (e.g. `-add switch statements`), in plain ASCII. Never add `Co-Authored-By` or any other trailer, even if your tooling adds one by default.
- Propose language and compiler design changes before implementing them. The user makes design calls, one step at a time.
- When reporting back, cover what you did and anything that differs from what the user would expect. Don't affirm negatives: if you followed an instruction by not doing something (e.g. you didn't commit because you weren't asked to), leave it out.

## Build and test

Commands are PowerShell, run from the repository root.

**Build** Debug, unless you're profiling (then Release). Never build both. On a fresh clone, run `third_party\build-all.cmd` once first.

```powershell
$msbuild = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest -requires Microsoft.Component.MSBuild -find "MSBuild\**\Bin\MSBuild.exe"
& $msbuild Fraze.sln /p:Configuration=Debug /p:Platform=x64 /m /v:minimal /nologo
```

**Test** by running the demo with `--test`. Its `main()` in `demo/assets/scripts/Main.fz` calls `RunAllTests()` (`compiler/assets/fraze.tests.fz`) and `MathTests.Run()` (`demo/assets/scripts/Tests.fz`), and `--test` makes it exit there instead of opening the 3D scene. `.fz` files are compiled at startup, so script-only edits need no rebuild. The working directory must be `demo`, because assets resolve relative to it.

```powershell
Push-Location demo; .\bin\x64\Debug\Demo.exe --test; "exit code: $LASTEXITCODE"; Pop-Location
```

- **Pass:** exit code 0. **Fail:** exit code 1, after a compile error or failed `assert` prints `file(line,col): message`.
- Before finishing a change to the compiler, VM, GC or demo, also run the full scene, which runs far more code than the tests. `demo\RunScene.ps1` closes it after 30 s and exits with 0 when the scene finished loading:

  ```powershell
  .\demo\RunScene.ps1; "exit code: $LASTEXITCODE"
  ```
- Only Debug runs checks: Release calls `DisableAssert()`, `DisableNullCheck()`, `DisableBoundsCheck()` and `DisableTypeCheck()` in `demo/source/main.cpp`.

## Layout

- `compiler/`: static library with the whole language. C++ is under `compiler/source/fraze/`, where `Compiler` runs `Lexer -> Parser -> SemanticAnalyzer -> CodeGenerator` and returns a `Program`, the VM. The standard library (`fraze.*.fz`) and test suite are in `compiler/assets/`.
- `demo/`: 3D app that embeds Fraze. The C++ host is in `demo/source/` (extern functions are registered in `ExternFunctions.cpp`), and scripts are in `demo/assets/scripts/`.
- `syntax-extensions/`: syntax highlighting for Visual Studio and VS Code. The grammar is `fraze-syntax.json`.
- `third_party/`: the demo's external dependencies, which are most of the repository's files. Leave it out of searches.

## Design principles

These serve two goals: being able to move or delete AST nodes freely, and keeping the compiler simple enough to understand and refactor. They are strong defaults, not hard rules. When following one would make the code significantly more complicated, simplicity wins; say so explicitly.

- **SemanticAnalyzer outputs a resolved AST and nothing else.** Ids, indices, offsets and side-table entries are assigned in CodeGenerator. If a lowering needs a runtime table entry, add a self-contained node that carries its own data (for example, a node holding a runtime check's message), and let CodeGenerator assign the id.
- **The AST flows downward.** No parent pointers, and no new references from nodes into generated collections. The existing `Scope*`, `targetDef` and `Type*` links are exceptions, not precedent.
- **Analyzer visits are idempotent**, because the AST is mutated, cloned and re-visited during analysis.
- **Infer node state instead of storing it.** Don't add a node field for anything derivable from the AST. If something really has to be tracked, prefer restructuring (e.g. lowering earlier). `Clone` must copy every field that analysis doesn't recompute.
- **Keep opcodes primitive and assembly-like**, because the long-term plan is a JIT. Never add a composite opcode (e.g. `CheckNull`) as a speed fix; express it with existing primitives.
- **Keep CodeGenerator changes small and in place:** a bit of member state checked at the exact site that changes (for example, a member recording which expression's result the current statement will discard). Avoid flag parameters, `Emit*(node, bool)` wrappers, and special-case lists of node kinds.

## Code conventions

- Namespace `fraze`. Use `sptr`/`spnew` from `common/Pointers.h`.
- PascalCase for types and methods; camelCase for locals and fields. One primary type per file.
- Add every new `.h`/`.cpp` to its `.vcxproj` and `.vcxproj.filters`.
- Every source file (`.h`, `.cpp`, `.fz`) starts with the copyright banner the existing ones have; copy it exactly.
- **Names:**
  - Never drop an operative word to save space, but don't spell out a sentence either. The usual short forms (`Arg`, `Var`, `Def`, `Expr`, `Ident`) are fine.
  - Give different views of the same thing one shared stem with different suffixes: if a scope holds both an expression and its value, name them `identExpr` and `identValue`, not `ident` and `resolvedValue`.
  - Name a value after what it is, with a noun phrase, not a verb phrase or a process it takes part in: `expectedValue`, not `jumpWhen`; `isTrustedInput`, not `skipsValidation`.
  - Surface what the reader can't see where the name is used, through what the thing is rather than through internals they'd have to go and read. A field named `finalizer` is a good model: it says what the field is and hints at its purpose without naming anything hidden.
  - Name a function's side effects. Don't name one like a query (`TryTakeCachedResult`, not `HasCachedResult`), and name an effect beyond what the caller can already read (`TryCancelScheduledSave`, not `TryTakePendingSave`).
  - Don't rename existing code unless asked.
- **Comments:** one per function, on its definition, and on a field whose name can't carry its meaning.
  - The first sentence says concretely what the thing does, returns or holds, not what it isn't; the why comes after, briefly.
  - Name the calls made and the state changed rather than a verb that stands in for them: "stores the value, marks it done and calls Finish", not "completes the operation".
  - Leave out what the reader can infer: which pass does a step when only one pass does that kind of work, or a property that follows from how the thing was built.
  - Describe a node by what the source wrote and what it lowers to, not by the other parts that lowering emits.
  - Usually 1–2 lines; a mechanism spanning several functions can take about 7.
  - At compile time, code "emits"; the emitted code "evaluates", "pushes" or "jumps".
