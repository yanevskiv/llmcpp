# Versioning

Use Semantic Versioning for the two shipped programs independently. `llmc++`
gets its version from `project(llmcpp VERSION ...)` in `CMakeLists.txt`; the
compiler reports that value through `--version` and its MCP `serverInfo`.
`llmcpp-agent` gets its version from `VERSION` in `agents/llmcpp-agent`; it
reports that value through `--version` and its MCP `clientInfo`/`serverInfo`.
Both started at `0.0.1`.
`llmcpp-tests` is a build-tree test tool, not a separately versioned release.

For every edit, assess whether it changes either program's shipped behavior or
compatibility. Bump only the affected program's version when it does. Use a patch
bump for compatible fixes, a minor bump for compatible new functionality, and a
minor bump for breaking changes while the program remains at major version 0.
Call out breaking changes in the commit message or documentation. Once a program
reaches major version 1, use a major bump for breaking changes. Documentation,
tests, and build-only edits do not require a version bump unless they alter a
shipped program's behavior.

The generation protocol version, MCP protocol version, and cache format version
are separate compatibility identifiers. Change them only when their respective
formats or contracts change; do not tie them to either program's release version.
