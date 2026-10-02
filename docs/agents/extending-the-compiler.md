# Extending the compiler

To add any of these, grep an existing one and mirror every site it appears in. Paths are relative to `compiler/source/fraze/`.

- **AST node** (e.g. `SwitchStatement`):
  - its header in `ast/*/`;
  - `ASTFwd.h`, a `To*()` in `ASTNode.h`, and `AST.h`;
  - `Visit` in `ASTVisitor`, `ASTPrinter`, `CodePrinter` and the passes that handle it;
  - the project files.
- **Opcode** (e.g. `NotEqualN`): the enum and `OpCodeNames` in `program/OpCode.h`, the `Execute_*` declaration in `program/Program.h`, its definition and a `FRAZE_EXECUTE_CASE` line in `Program::Run` in `program/Program.cpp` (a missing case asserts in Debug when the opcode runs), and operand printing in `ProgramDiagnostics::PrintOperation` (`program/ProgramDiagnostics.cpp`).
- **Keyword:** `Keyword`, `Keywords` and `KeywordNames` in `compiler/Lexer.h`, plus the grammar in `syntax-extensions/fraze-syntax.json`.
