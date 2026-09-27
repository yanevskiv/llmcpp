# llmcpp style contract

- Apply this contract to project-owned C++ under `include/` and `src/`, and to
  the Catch2 test implementation.
- Exempt fixture sources under `tests/` where a test requires unusual llmcpp
  syntax.

## Layout and naming

- Put declarations, including private implementation adapters, under
  `include/llmcpp/`; put `llmcpp` implementations under `src/llmcpp/`. Keep the
  driver entry point in `src/main.cpp`.
- Put header-only value types and enums under `include/llmcpp/data/` in
  `llmcpp::data`. Spell them as `data::TypeName` in `llmcpp` implementation code.
- Prefix project-defined classes, structs, and enums with their PascalCase
  module name. Prefix their filenames with the same name in snake_case: put
  `llmcpp::CompilerSandbox` in `compiler_sandbox.h` and `compiler_sandbox.cpp`.
- Name C++ files in lowercase `snake_case`.
- Give every project-defined class a same-named header/source pair. Declare
  project classes and structs in headers, not `.cpp` files; implement callable
  members in the matching `.cpp` file. Keep a stateless record struct
  header-only when it needs no implementation.
- Start every C++ source and header with a descriptive `/* ... */` block using
  `C++ file for ...` or `C++ header for ...` wording.
- Close namespace braces and header guards without trailing comments.

## Formatting

- Follow `.clang-format` as the formatting authority.
- Indent with four spaces; do not use tabs.
- Indent namespaces. Align access labels with their class declaration and
  indent members four additional spaces.
- Brace every control-flow body, including single statements in `if`, `else`,
  `for`, `while`, and `do` statements.
- Put opening braces for functions, methods, constructors, namespaces, classes,
  structs, enums, and unions on the following line. Keep control-flow and
  lambda braces on the statement's line.
- Put the colon before the first constructor initializer. Put each following
  initializer on its own line with a leading comma.

## Naming

- Name classes, structs, enums, type aliases, and other types in `PascalCase`.
- Name project-owned functions and methods in `snake_case()`.
- Name data members in `m_snake_case`, including public aggregate fields.
- Name parameters and local variables in lower `camelCase`. Prefer a single
  lowercase word such as `opts`, `begin`, or `label`; use `parseOpts` only when
  multiple words are needed.
- Expand a one-letter local or parameter only when it obscures its role. Keep
  conventional narrow-scope names such as `i` for an iterator or `c` for a
  character.
- Preserve the spelling required by external APIs in overrides.

## Documentation

- Introduce every class, struct, enum, function, and method, including private
  members, with a `/** ... */` Doxygen block; do not use `///` Doxygen comments.
- Use a one-line `/** ... */` block for a concise single-sentence description,
  including namespace and simple type descriptions. Use a multi-line block for
  parameter, return, template, or additional detail tags.
- Document every namespace and subnamespace immediately above its declaration.
  Use `/** ... */` in headers and a short `// Namespace for ...` comment in
  source files.
- Document every data member and aggregate field, including private
  implementation state, with a concise Doxygen block describing its value.
- Put no blank line before a Doxygen block. Indent a field's block and
  declaration one four-space level inside its class or struct.
- Include one `@param` entry for every named function or method parameter.
  Include `@return` for non-`void` functions; omit it for constructors,
  destructors, and `void` functions.
- Document function template parameters with `@tparam`.
- Treat the declaration's Doxygen block as canonical; do not repeat the full
  contract at the definition.
- Put one short `// ...` intent comment immediately above every namespace,
  type, free function, and out-of-line method definition in `.cpp` files. Keep
  these comments outside entity bodies.
- Put a short `// ...` comment above each `.cpp` include group to explain its
  purpose.
- Write documentation and intent comments over functions, methods,
  constructors, and destructors in imperative mood: `Run the llmc++ driver.`,
  not `Runs the llmc++ driver.`
- Write intent comments over non-callable entities as descriptive noun phrases
  naming their kind and purpose: `Namespace for ...`, `Class for ...`,
  `Structure for ...`, or `Constants for ...`.

## Function bodies

- Keep source comments out of function, method, and lambda bodies. Put exactly
  `// Empty.` in an otherwise empty constructor body. Express other intent in
  Doxygen blocks, clearer names, or smaller helpers.
- Allow string literals containing comment syntax; do not treat them as comments.
- Allow comments outside callable bodies except for trailing namespace and
  header-guard comments.

## Enforcement

- Install the hooks with `pre-commit install`.
- Run `pre-commit run --all-files` to check formatting, naming, file headers,
  indentation, Doxygen form, and body comments. The hooks use clang-format,
  clang-tidy, and `scripts/check_style.py`.

## Repository documentation

- Do not use emojis or em dashes in project Markdown documentation.
- Use GitHub alert callouts only when they make an important note, warning, or
  limitation easier to identify.
