# Project priorities

- Follow `STYLE.md` strictly for project C++ and Catch2 code. For Python and
  CMake, follow its intent (clear grouping, naming, and short intent comments),
  not its C++-specific rules.
- Keep `llmc++` a Clang-based compiler driver. It uses C++20 internally, but
  must respect the language standard selected for user source files.
- Keep integration tests self-contained under `tests/`; never make them depend
  on examples. Use deterministic local agents and servers, not live model APIs.
- Update README, guides, and man pages when public behavior changes. Successful
  generation should remain silent unless the user requests diagnostic output.
- Build with `./do_build.sh` and test with `./do_tests.sh`. Build the separate
  documentation site with `./do_build_docs.sh` when changing `docs/web/`.

# Versions

Version `llmc++` in `CMakeLists.txt` and `llmcpp-agent` in its `VERSION` constant
independently. Bump affected programs for shipped fixes (patch), features or
pre-1.0 breaks (minor), and post-1.0 breaks (major); document breaks. Docs,
tests, and build-only edits need no bump unless shipped behavior changes.
`llmcpp-tests` has no separate version. Bump protocol and cache-format versions
only when their contracts change.
