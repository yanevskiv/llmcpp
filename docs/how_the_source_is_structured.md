# How the source is structured

```mermaid
flowchart LR
    Driver[driver\nCLI and Clang driver orchestration] --> Frontend[frontend\nPreprocessor and AST actions]
    Frontend --> Generation[generation\nPrompt replacement and LLM pass]
    Generation --> Agent[agent\nMCP session and tool server]
    Generation --> Compiler[compiler\nAST inspection and shadow compilation]
    Compiler --> Source[source\nSource-text helpers]
    Agent --> Data[data\nHeader-only value types]
```

| Path | Purpose |
| --- | --- |
| `src/main.cpp` | Executable entry point. |
| `src/llmcpp/driver/` | Command-line handling and Clang driver orchestration. |
| `src/llmcpp/frontend/` | Keyword detection, preprocessing callbacks, and AST actions. |
| `src/llmcpp/generation/` | Annotation validation, caching, prompt generation, and rewriting. |
| `src/llmcpp/agent/` | Native Anthropic client, external-agent session, MCP protocol, and compiler tools. |
| `src/llmcpp/compiler/` | AST extraction, type inspection, and candidate shadow compilation. |
| `src/llmcpp/source/` | Source-text utilities. |
| `include/llmcpp/` | Public declarations mirroring the implementation subnamespaces. |
| `include/llmcpp/data/` | Header-only data types in `llmcpp::data`. |
| `agent/` | The optional Python adapter for Claude Code and external Anthropic use. |
| `test/` | Catch2 integration tests, test cases, and a deterministic mock agent. |
