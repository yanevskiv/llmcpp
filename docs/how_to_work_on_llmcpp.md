# How to work on llmcpp

Build and test the project once before making changes:

```sh
./do_fetch_deps.sh
./do_build.sh --parallel 8
./do_test.sh
```

The integration suite covers the driver, AST tools, source rewriting, cache,
native OpenAI and Anthropic clients, and external-agent protocol. Remote
backends are represented by deterministic local stand-ins, so tests should not
need credentials or network access.

## Build in Docker

Build the development image, then build the project inside it:

```sh
./do_docker_buildx.sh
./do_build_in_docker.sh --parallel 8
```

The image is named `yanevskiv:llmcpp-dev` and starts from `ubuntu:latest`.
It includes the build tools and fetched dependencies. Docker builds use
x86-64 because the current dependency layout requires it.

The wrapper runs as your user and writes build output to `build/docker/out`
and the installation to `build/docker/install`. Native build output and
dependencies are left untouched. Build arguments are forwarded to `do_build.sh`.
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
- Add end-to-end behavior to `tests/llmcpp_tests.cpp`. Fixture sources use
  the `test_` prefix under `tests/`, with scripts in `tests/json/`
  and fixture headers in `tests/include/`.
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
the component boundaries. [PLAN.md](../PLAN.md) records the prototype design
and the remaining milestones.
