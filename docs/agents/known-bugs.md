# Known bugs

Unfixed bugs, each confirmed against the code or by a probe. Remove an entry when it's fixed. Paths are relative to `compiler/source/fraze/`.

- **Duplicate functor captures:** a functor literal whose body names the same captured variable twice fails with "multiple definitions found". `Parser::GetCaptures` returns one entry per occurrence, and `ParseFunctorExpr` adds a field for each.
- **Functor literals collide across files:** each literal's class is named `$functorClass<n>` from a counter in `Parser`, and every file gets its own `Parser`. So the first functor literal in any file other than `fraze.tests.fz` collides with the one there, failing with "multiple definitions found".
- **Compound assignment evaluates its target twice:** `a[F()].x += 1` calls `F()` twice, because `CodeGenerator::Visit(AssignExpression)` evaluates the left side to read it, and the store evaluates its receiver again.
- **`.Count` and `.Size` on a null array aren't null-checked:** `SemanticAnalyzer::Visit(IdentifierExpression)` lowers them to an `ArrayCountExpression` and an `Array.GetSize` call without `WrapWithNullCheck`, so a null array is read through instead of failing a check.
- **Ternary arms aren't checked against each other:** `SemanticAnalyzer::Visit(TernaryExpression)` checks only the condition, and the expression takes the true arm's type, so `true ? 1 : "str"` compiles.
- **A `Program` can't run again after an exception escapes it:** `Program::InvokeImpl` restores `rip`, `rsp` and `rbp` only on a normal return. The demo exits after catching one, so this matters only to a host that catches and continues.
