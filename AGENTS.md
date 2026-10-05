# Project instructions

- Follow `STYLE.md` as a strict contract for project C++ and Catch2 code.
- Apply the spirit, not the C++-specific rules, of `STYLE.md` to Python and CMake.
- Keep `llmc++` a Clang-based compiler driver.
- Use C++20 internally; respect the language standard selected for user source files.
- Keep integration tests self-contained under `tests/`, independent of examples.
- Use deterministic local agents and servers in tests, not live model APIs.
- Update README, guides, and man pages when public behavior changes.
- Keep successful generation silent unless the user requests diagnostics.
- Build with `./do_build.sh` and test with `./do_tests.sh`.
- Build the documentation site with `./do_build_docs.sh` after changing `docs/web/`.

# Versions

- Version `llmc++` in `CMakeLists.txt` and `llmcpp-agent` in its `VERSION` constant independently.
- Assess compatibility for every change; bump only affected shipped programs.
- Bump patch for fixes, minor for features or pre-1.0 breaks, and major for post-1.0 breaks.
- Document breaking changes.
- Skip version bumps for docs, tests, and build-only edits unless shipped behavior changes.
- Do not version `llmcpp-tests` separately.
- Bump protocol and cache-format versions only when their contracts change.
