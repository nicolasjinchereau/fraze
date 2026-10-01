# Writing Fraze

The syntax is C#-like, and `compiler/assets/fraze.tests.fz` has examples of every feature; check it before assuming a C# feature exists. Add language tests there as a `section XxxTests` with a `void Run()` that uses `assert(...)`, and call it from `RunAllTests()`.

- **Types:**
  - primitives: `bool`, `int` (64-bit), `num` (double), `string`, `object`;
  - `class`, `struct` (value type), `interface`, `enum`, and `functor` (callable type);
  - generics (`List<T>`, `Table<K, V>`), arrays (`T[]`), properties and operator overloads.
- `section` is a namespace. An unqualified call also finds overloads in the scope that defines each argument's type, so an overload can live beside the type it's for.
- `extern` declarations bind to C++ functions registered with `Compiler::AddFunction` or `AddIntrinsic`; the demo registers its own in `demo/source/ExternFunctions.cpp`.
- `fold { stmts; expr; }` is an expression whose value is its final expression statement. A `return` inside it returns from the enclosing function.
- `switch` doesn't fall through and accepts `case 1, 2:`. `goto name;` jumps to a `name:` label. There is no `break` or `continue` yet.
- Coroutines return `Task<T>`. `await` works anywhere in an expression except a loop condition, an assignment target, the right of a compound assignment, and inside a fold.
- Structs and enums can't be converted to `object`, because boxing isn't implemented.
