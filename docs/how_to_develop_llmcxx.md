# How to develop llmc++

Build and test the project once before making changes:

```sh
./fetch-deps.sh
./build.sh --parallel 8
./test.sh
```

The integration suite covers the driver, AST tools, source rewriting, cache,
native Anthropic client, and external-agent protocol. Codex and other remote
backends are represented by deterministic local stand-ins, so tests should not
need credentials or network access.

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
- Add end-to-end behavior to `test/integration_tests.cpp`. Fixture sources use
  the `case_` prefix under `test/cases/`, with scripts in `test/cases/json/`
  and fixture headers in `test/cases/include/`.
- Use `test/mock-agent/llmcpp-mock-agent` for scripted compiler-tool
  conversations and CLI-adapter tests. Tests must not depend on an installed
  CLI, a login, or a live model.
- Keep user documentation aligned with behavior that the integration suite
  exercises.

[Find your way around the source](how_the_source_is_structured.md) describes
the component boundaries. [PLAN.md](../PLAN.md) records the prototype design
and the remaining milestones.
