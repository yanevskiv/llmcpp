# Project instructions

- Follow `STYLE.md` as a strict contract for project C++ and Catch2 code.
- Apply the spirit of `STYLE.md` to Python and CMake without importing its
  C++-specific rules.
- Keep `llmc++` a Clang-based compiler driver. Use C++20 internally, but respect
  the language standard selected for user source files.
- Keep integration tests self-contained under `tests/`; do not depend on
  examples or live model APIs. Use deterministic local agents and servers.
- Update README, guides, and man pages when public behavior changes.
- Keep successful generation silent unless the user requests diagnostics.
- Build with `./do_build.sh` and test with `./do_tests.sh`.
- Build the separate documentation site with `./do_build_docs.sh` after changing
  `docs/web/`.

# Versions

- Version `llmc++` in `CMakeLists.txt` and `llmcpp-agent` in its `VERSION`
  constant independently.
- Assess compatibility for every change. Bump affected shipped programs for
  fixes (patch), features or pre-1.0 breaks (minor), and post-1.0 breaks (major).
- Document breaking changes. Do not bump for docs, tests, or build-only edits
  unless they change shipped behavior.
- Do not version `llmcpp-tests` separately. Bump protocol and cache-format
  versions only when their contracts change.
