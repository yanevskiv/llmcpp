# How the source is structured

The compiler pipeline is split by responsibility. Following an annotated body
from the driver into generation is usually the quickest way to understand the
project.

```{mermaid}
flowchart LR
    Driver[driver] --> Frontend[frontend]
    Frontend --> Generation[generation]
    Generation --> Agent[agent]
    Generation --> Compiler[compiler]
    Compiler --> Source[source]
    Agent --> Data[data]
```

| Path | What belongs there |
| --- | --- |
| `src/main.cpp` | The executable entry point. |
| `src/llmcpp/driver_*.cpp` | Command-line handling and Clang driver orchestration. |
| `src/llmcpp/frontend_*.cpp` | Keyword detection, preprocessing callbacks, and AST actions. |
| `src/llmcpp/generation_*.cpp` | Annotation validation, cache lookup, generation, and source rewriting. |
| `src/llmcpp/agent_*.cpp` | Native API clients, external-agent transport, MCP server, and tool dispatch. |
| `src/llmcpp/compiler_*.cpp` | AST queries, type inspection, and candidate shadow compilation. |
| `src/llmcpp/source_*.cpp` | Source locations and text manipulation. |
| `include/llmcpp/` | Declarations matching the implementation module names. |
| `include/llmcpp/data/` | Header-only records shared across components. |
| `include/llmcpp/test/`, `src/llmcpp/test/` | `llmcpp::test` workspace, text checks, and local model-server fixtures. |
| `agents/llmcpp-agent` | The Python adapter for Codex, Claude Code, and external Anthropic access. |
| `agents/llmcpp-mock-agent` | Scripted agent for deterministic integration tests. |
| `tests/` | Catch2 integration tests, source fixtures, and deterministic backend stand-ins. |

## Follow one generated body

1. `driver` separates llmc++ options from the arguments passed to Clang.
2. `frontend` finds `__llm__`, records the prompt range, and builds the AST.
3. `generation` checks the cache and owns the pass for each target.
4. `agent` selects a native or external backend and exposes the compiler tools.
5. `compiler` answers context queries and compiles candidate bodies.
6. `generation` stores the accepted body and rewrites the translation unit.
7. `driver` hands the rewritten source back to the normal Clang pipeline.

Project-owned C++ declarations live under `include/llmcpp/`; their definitions
live under the matching `src/llmcpp/` path. Filename prefixes group modules,
and their declarations live directly in `llmcpp`; only the shared records in
`data/` retain the nested `llmcpp::data` namespace. The detailed naming and
formatting rules are in {download}`STYLE.md <../../STYLE.md>`.
