# llmcpp style contract

- Apply this contract to project C++ under `include/`, `src/`, and the Catch2 tests.
- Exempt `tests/` fixtures when a test requires unusual llmcpp syntax.

## Layout and naming

- Put declarations and private adapters under `include/llmcpp/`.
- Put `llmcpp` implementations under `src/llmcpp/`.
- Keep the driver entry point in `src/main.cpp`.
- Put header-only value types and enums under `include/llmcpp/data/` in `llmcpp::data`.
- Spell data types as `data::TypeName` in `llmcpp` implementations.
- Prefix project classes, structs, and enums with their PascalCase module name.
- Prefix module filenames with the same name in snake_case: `CompilerSandbox` uses `compiler_sandbox.h/.cpp`.
- Name C++ files in lowercase `snake_case`.
- Give every project class a same-named header/source pair.
- Declare project classes and structs in headers, not `.cpp` files.
- Implement callable members in the matching `.cpp` file.
- Keep stateless record structs header-only when they need no implementation.
- Start each C++ file with a `/* ... */` block using `C++ file for ...` or `C++ header for ...`.
- Close namespace braces and header guards without trailing comments.

## Formatting

- Follow `.clang-format` as the formatting authority.
- Keep C++ copyright headers within 72 columns and fully justify their license
  paragraphs, except for the final line of each paragraph.
- Indent with four spaces; do not use tabs.
- Indent namespaces.
- Align access labels with the class declaration.
- Indent members four more spaces than access labels.
- Brace every `if`, `else`, `for`, `while`, and `do` body, even one-liners.
- Put function, method, constructor, namespace, class, struct, enum, and union braces on the next line.
- Keep control-flow and lambda braces on the statement's line.
- Put the constructor initializer colon before the first initializer.
- Put each later initializer on its own line with a leading comma.

## Naming

- Name classes, structs, enums, type aliases, and other types in `PascalCase`.
- Name project-owned functions and methods in `snake_case()`.
- Name data members in `m_snake_case`, including public aggregate fields.
- Name parameters and locals in lower `camelCase`.
- Prefer one-word local names such as `opts`, `begin`, and `label`.
- Use multiword names such as `parseOpts` only when needed.
- Expand one-letter names only when unclear; keep conventional narrow-scope `i` or `c`.
- Preserve the spelling required by external APIs in overrides.

## Documentation

- Give every class, struct, enum, function, and method a `/** ... */` Doxygen block, including private ones.
- Do not use `///` Doxygen comments.
- Use one-line Doxygen blocks for concise, single-sentence descriptions.
- Use multi-line blocks when parameter, return, template, or other detail tags are needed.
- Document every namespace directly above its declaration with `/** ... */` in headers.
- Document every source namespace with a short `// Namespace for ...` comment.
- Document every data member and aggregate field, including private state, with a concise Doxygen block.
- Put no blank line before a Doxygen block.
- Indent a field's block and declaration one four-space level inside its class or struct.
- Give every named function or method parameter one `@param` entry.
- Give non-`void` functions an `@return` entry.
- Omit `@return` for constructors, destructors, and `void` functions.
- Document function template parameters with `@tparam`.
- Keep the declaration's Doxygen block canonical; do not repeat it at the definition.
- Put one short intent comment above every `.cpp` namespace, type, free function, and out-of-line method.
- Keep implementation intent comments outside entity bodies.
- Put a short purpose comment above each `.cpp` include group.
- Write callable intent comments in imperative mood: `Run the llmc++ driver.`, not `Runs the llmc++ driver.`
- Write non-callable intent comments as kind-and-purpose noun phrases: `Namespace for ...`, `Class for ...`.

## Function bodies

- Keep source comments out of function, method, and lambda bodies.
- Put exactly `// Empty.` in an otherwise empty constructor body.
- Express other intent in Doxygen blocks, clearer names, or smaller helpers.
- Allow string literals containing comment syntax.
- Allow comments outside callable bodies, except trailing namespace and header-guard comments.

## Enforcement

- Install the hooks with `pre-commit install`.
- Run `pre-commit run --all-files` to check formatting, naming, documentation, and structural rules.
- Use the configured clang-format, clang-tidy, and `scripts/check_style.py` hooks.

## Repository documentation

- Do not use emojis or em dashes in project Markdown documentation.
- Use GitHub alert callouts only for important notes, warnings, or limitations.
