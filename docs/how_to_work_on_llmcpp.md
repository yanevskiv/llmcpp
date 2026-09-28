# How to work on llmcpp

Build and test the project once before making changes:

```sh
./do_fetch_deps.sh
./do_build.sh --parallel 8
./do_tests.sh
```

The integration suite covers the driver, AST tools, source rewriting, cache,
native OpenAI and Anthropic clients, and external-agent protocol. Remote
backends are represented by deterministic local stand-ins, so tests should not
need credentials or network access.

## Build reference documentation

Documentation is built separately from the compiler. `do_build_docs.sh` generates
the HTML site in `build/docs/html` and installs it to
`build/install/share/doc/llmcpp/html`.
Open `index.html` in a browser. Sphinx builds the HTML
site with the Read the Docs theme; MyST reads the Markdown guides and Breathe
imports the Doxygen XML reference.

Doxygen and Python's `venv` support are required for documentation and included in the
Docker image. `do_build_docs.sh` creates a virtual environment under
`build/out/docs_venv` and installs Sphinx and the other packages from
`docs/web/requirements.txt`. Subsequent builds reuse the environment and check
that its packages satisfy the requirements.

Edit website pages as Markdown under `docs/web/` and independent how-to guides
under `docs/`. The homepage's toctrees define the sidebar sections; the tutorials
form a numbered sequence. Edit C++ reference documentation in the headers,
not the generated HTML or XML.

Build and install the documentation:

```sh
./do_build_docs.sh
```

The documentation build uses its own CMake tree under `build/docs` and does not
require fetched compiler dependencies. Normal compiler builds do not require
Doxygen or Sphinx.

The hand-written command references in `docs/man/` install to
`build/install/share/man/man1`, even when Doxygen is disabled. Preview them with
`man -l docs/man/llmc++.1` and `man -l docs/man/llmcpp-agent.1`, or use
`man -M build/install/share/man llmc++` after installation.

## Build in Docker

Build the development image, then build the project inside it:

```sh
./do_docker_buildx.sh
./do_build_in_docker.sh --parallel 8
```

The image is named `yanevskiv:llmcpp-dev` and starts from `ubuntu:latest`.
It includes the build tools and fetched dependencies. Docker builds use
x86-64 because the current dependency layout requires it.

The wrapper runs as your user and calls `do_build.sh`, writing build output to
`build/out` and the installation to `build/install`, just like a native build.
Container dependencies are kept in a separate volume; host dependencies are
left untouched. Build arguments are forwarded to `do_build.sh`.
Run `./do_clean.sh` before switching between native and Docker builds because
CMake caches source paths and compiler settings.
After building, run the integration suite with
`./do_build_in_docker.sh --target test`.

## Run the project checks

Install the repository hooks once:

```sh
pre-commit install
```

Run the same checks over the whole tree before committing:

```sh
pre-commit run --all-files
```

The hooks check C++ formatting, naming, documentation, and structural rules.
Read [STYLE.md](../STYLE.md) before adding a C++ type or moving declarations;
it defines where headers and implementations belong as well as the required
comments and naming conventions.

## Add or change behavior

- Put public and private C++ declarations under `include/llmcpp/` and matching
  definitions under `src/llmcpp/`.
- Add end-to-end behavior to `tests/llmcpp_tests.cpp`. Put numbered fixture
  sources under `tests/data/`, with scripts in `tests/data/json/`, headers in
  `tests/data/include/`, and expected output in `tests/data/expected/`.
- Put reusable C++ test helpers in `llmcpp::test`, with declarations under
  `include/llmcpp/test/` and implementations under `src/llmcpp/test/`.
  Prefix their types with `Test` and filenames with `test_`, such as
  `TestWorkspace` in `test_workspace.h` and `test_workspace.cpp`.
  Keep `tests/llmcpp_tests.cpp` focused on Catch2 test cases.
- Use `agents/llmcpp-mock-agent` for scripted compiler-tool
  conversations and CLI-adapter tests. Tests must not depend on an installed
  CLI, a login, or a live model.
- Keep user documentation aligned with behavior that the integration suite
  exercises.

[How the source is structured](how_the_source_is_structured.md) describes
the component boundaries.
