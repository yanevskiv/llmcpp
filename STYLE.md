# llmcpp style contract

This contract applies to project-owned C++ under `include/`, `src/`, and the
Catch2 test implementation. Fixture sources under `test/cases/` intentionally
exercise unusual llmcpp syntax and are exempt where a test requires it.

## Layout and naming

- Declarations, including private implementation adapters, live under
  `include/llmcpp/`; implementations in the `llmcpp` namespace live under
  `src/llmcpp/`. The driver entry point remains `src/main.cpp`.
- Header-only value types and enums live under `include/llmcpp/data/` in the
  `llmcpp::data` namespace. Use their fully scoped `data::TypeName` spelling
  from within `llmcpp` implementation code.
- Project-defined classes, structs, and enums begin with their PascalCase
  module prefix. Module filenames begin with the same prefix in snake_case.
  For example, `llmcpp::CompilerSandbox` lives in
  `include/llmcpp/compiler_sandbox.h` and `src/llmcpp/compiler_sandbox.cpp`.
- C++ filenames use lowercase `snake_case`.
- Every project-defined class has a same-named header/source pair. Do not
  define a project class or struct in a `.cpp` file; declare it in its header
  and implement callable members in its matching `.cpp` file. Stateless record
  structs that need no implementation may have only a header.
- Every C++ source and header starts with a descriptive `/* ... */` block using
  `C++ file for ...` or `C++ header for ...` wording.
- Namespace braces and header guards close without trailing comments.

## Formatting

- `.clang-format` is authoritative.
- Indentation is four spaces. Tabs are forbidden.
- Namespaces are indented. Access labels align with their class declaration,
  and members are indented four additional spaces.
- Every control-flow body, including single statements, uses braces. This
  applies to `if`, `else`, `for`, `while`, and `do` statements.
- Function, method, constructor, namespace, class, struct, enum, and union
  opening braces go on the following line. Control-flow and lambda braces stay
  on the statement's line.
- Constructor initializer lists put the colon before the first initializer and
  put each following initializer on its own line with a leading comma.

## Naming

- Classes, structs, enums, type aliases, and other type names use `PascalCase`.
- Project-owned functions and methods use `snake_case()`.
- Data members use `m_snake_case`, including public aggregate fields.
- Parameters and local variables use lower `camelCase`: prefer a single
  lowercase word such as `opts`, `begin`, or `label`, and use forms such as
  `parseOpts` only when multiple words are needed.
- Expand a one-letter local or parameter only when it obscures its role.
  Conventional narrow-scope names such as `i` for an iterator or `c` for a
  character remain appropriate.
- Required overrides of external APIs retain the spelling required by that API.

## Documentation

- Classes, structs, enums, functions, and methods, including private members,
  are introduced by a Doxygen
  block of the form `/** ... */`; `///` Doxygen comments are forbidden.
- Use a one-line `/** ... */` block for a concise single-sentence description,
  including namespace and simple type descriptions. Use a multi-line block
  when parameter, return, template, or additional detail tags are needed.
- Document every namespace and subnamespace immediately above its declaration:
  headers use a `/** ... */` block and source files use a short descriptive
  `// Namespace for ...` comment.
- Every data member and aggregate field, including private implementation
  state, has a concise Doxygen block describing the value it stores.
- Do not put a blank line before a Doxygen documentation block. Indent a
  field's block and declaration one four-space level inside the containing
  class or struct.
- Function and method documentation includes one `@param` entry for every
  named parameter. Non-`void` functions include `@return`. Constructors,
  destructors, and `void` functions omit `@return`.
- Function templates document template parameters with `@tparam`.
- A declaration's Doxygen block is the canonical documentation. Definitions do
  not repeat the full contract.
- In `.cpp` files, every namespace, type, free function, and out-of-line method
  definition has one short `// ...` comment immediately above it stating its
  purpose or intent. These implementation annotations stay outside the entity's
  body.
- Include groups in `.cpp` files have a short `// ...` annotation describing
  why those headers are present.
- Documentation and intent comments over functions, methods, constructors, and
  destructors use imperative mood: for example, `Run the llmc++ driver.`, not
  `Runs the llmc++ driver.`
- Intent comments over non-callable entities are descriptive noun phrases that
  name the entity kind and purpose, such as `Namespace for ...`, `Class for ...`,
  `Structure for ...`, or `Constants for ...`.

## Function bodies

- Function, method, and lambda bodies contain no source comments, except that an
  otherwise empty constructor body contains exactly `// Empty.`. Explain other
  intent in the entity's Doxygen block or through clearer names and smaller helpers.
- String literals containing comment syntax are not comments and are allowed.
- Comments outside callable bodies are allowed, except for trailing namespace
  and header-guard comments.

## Enforcement

Install the hooks with `pre-commit install`. The hooks run clang-format in
check mode, clang-tidy naming checks, and `python/check_style.py` for
filename, file-header, indentation, Doxygen-form, and body-comment rules. Run
all checks manually with `pre-commit run --all-files`.

## Repository documentation

- Do not use emojis or em dashes in project Markdown documentation.
- Use GitHub alert callouts only when they make an important note, warning, or
  limitation easier to identify.
