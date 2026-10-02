# Measuring performance

- Build and measure in Release. Debug runs the runtime checks, so its profile is dominated by check helpers.
- Measure branchy gameplay-style code, not only demo FPS: for example a stress test in `Main.fz` timed with `Time.Now.Seconds` and printed with `Console.WriteLine`. The demo's hot path, `MeshRenderer.Deform`, is math-heavy and unrepresentative.
- **Instruction profile:** set `FRAZE_CODE_PROFILING` to 1 in `compiler/source/fraze/common/Platform.h`. `main.cpp` already calls `ProgramDiagnostics::DumpCodeProfile` under that macro, so nothing else is needed.
- **Static opcode count:** after `Compile()` in `main.cpp`, print the size of `program->code` (or count the opcodes you care about), then remove the print.
- **FPS:** uncomment `#define PRINT_FPS` in `demo/source/ExternFunctions.cpp`, which prints a rolling average about once a second after the scene loads. Run `.\demo\RunScene.ps1 -Configuration Release -WaitSeconds 45` and average the last 10 `FPS:` lines.
  - Discard the first run after a build, which reads 10–20% low.
  - Runs drift by a few percent over a session, so comparing one block of runs against a later block isn't reliable. Copy one build's `Demo.exe` aside, swap the two binaries in between runs, and compare paired means.
