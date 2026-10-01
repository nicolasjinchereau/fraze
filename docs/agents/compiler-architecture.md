# Compiler architecture

`Compiler` (`compiler/Compiler.*`) runs `Lexer -> Parser -> SemanticAnalyzer -> CodeGenerator` and returns an `sptr<Program>`. Paths below are relative to `compiler/source/fraze/`.

Every `.fz` under the assets path given to the `Compiler` constructor (the standard library) and under each `AddDirectory` path is parsed into one `ASTRoot`. There are no imports: a definition in any file is visible from every other.

- **Lexer** (`compiler/Lexer.*`): source text to tokens. Keywords are in the `Keywords`/`KeywordNames` tables.
- **Parser** (`compiler/Parser.h`, header-only): builds the AST rooted at `ASTRoot`, with nodes in `ast/def`, `ast/expr`, `ast/stmt` and `ast/type`. It adds the implicit `this` as parameter 0 of every function where `FunctionDefinition::HasImplicitThisParam()` is true. It also turns each `Task` function into a state class (`ParseTaskObject`), with its locals hoisted into fields.
- **SemanticAnalyzer** (`compiler/SemanticAnalyzer.*`, an `ASTVisitor`): resolves scopes, types and overloads, validates, and lowers sugar into plain AST:
  - runtime checks become calls to the helpers in `fraze.system.fz` (`Wrap*`);
  - `await` becomes a `$position` switch plus gotos (`LowerAwait`), and the operands evaluated around it are spilled into fields of the task (`SpillAwaits`);
  - temporaries use `CreateTempFold`;
  - a lowering written out as statements builds its nodes with `LoweringBuilder` (`compiler/LoweringBuilder.h`).

  A member access gets its receiver as `node->context` (`this`, or `$this` inside a coroutine), and `VisitCallTarget` passes it as the first argument. By code generation, `this` is an ordinary argument.
- **CodeGenerator** (`compiler/CodeGenerator.*`): emits stack-based bytecode, similar to C# IL. It assigns everything numeric: frame and field offsets, check-site ids and jump targets.
- **Program** (`program/Program.*`): the VM. It owns the stack, globals, `Heap`, `TypeInfo` and `checkSites`. `OpCode` (`program/OpCode.h`) is a `uint8_t` enum, and each opcode has an `Execute_*` handler. `Dispatcher` runs async work, and `program->Invoke("main", args)` runs inside a `ScopedAllocator`.
- **GC** (`memory/Heap.*`): mark-and-sweep. It scans the VM stack conservatively, and `PinMemory` keeps objects in place for native code.
- **Built-in externs** (`Debug.Fail`, `Type.NewClass`, ...) are in `compiler/NativeFunctions.h` and registered in `Compiler.cpp`. Allocation is an extern call, not an opcode.

## Traps

Behavior that isn't visible from the code you'd be editing. Unfixed bugs are in `known-bugs.md`.

- **Replacing a node:** `ASTVisitor::VisitChild` casts a visit's `replacement` to the type of the pointer holding the child. Hold a node whose visit may replace it with another kind in a base-type pointer such as `sptr<Expression>`: a failed cast asserts in Debug and silently leaves null in Release.
- **Re-analyzing a clone:** `Clone` doesn't copy an identifier's `targetDef`, and analysis has already moved a call's receiver into `arguments[0]`. A clone of an analyzed call gets resolved again, which can fail to find the member or add a second receiver.
- **Fields the compiler adds:** for each field a `new` doesn't pass a value for, CodeGenerator emits the field's `initializer` without checking it for null. The parser gives every declared field a `DefaultValueExpression`, so a field added by a lowering needs one too.
- **Argument order:** CodeGenerator reserves the return storage, then evaluates and pushes call arguments last to first. Their side effects run right to left, and the receiver in `arguments[0]` is pushed last.
