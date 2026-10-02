# Debugging the compiler

Call these on the `Compiler` in `demo/source/main.cpp`, before `Compile()`. Paths are relative to `demo/`; use `output/...`, which is git-ignored. The commented-out export call in `main.cpp` sits inside `#if !FRAZE_ASSERTS`, which only Release compiles, so for a Debug run put the call outside that block.

- `ExportAST(path)`: the AST after parsing and before analysis, written to `AST.txt`.
- `ExportLoweredCode(path)`: the AST after analysis, printed as Fraze-like pseudo-code. It's usually the quickest way to see what a lowering did.
  - It shows the true lowered form: the receiver as first argument, fully qualified call targets, runtime checks as calls, and pseudo-forms like `cast<T>(x)` and `default(T)` for nodes without syntax.
  - It writes one `.fz` per source file, plus `__global.fz`.
  - The output isn't meant to compile.
- `ExportBytecode(path)`: one CSV per source file, plus `static-constructors.csv`.

Macros in `compiler/source/fraze/common/Platform.h`:

- `FRAZE_PRINT_EXECUTED_CODE` traces every executed op (very slow);
- `FRAZE_HEAP_DEBUG` is for GC debugging;
- `FRAZE_CODE_PROFILING` counts executed instructions; see `performance.md`.
