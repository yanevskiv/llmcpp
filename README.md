# llmcpp

A prototype of `llmc++`: clang++ with an `__llm__` function specifier. The body
of an `__llm__` function is a plain-language prompt, and an LLM agent writes the
real body while the file compiles. The design is in
[PLAN.md](PLAN.md).

```cpp
#include <iostream>
#include <string>

__llm__ void greet(const std::string& name) {
    Print "Hello, <name>!" followed by a newline.
}
```

## How the prototype works

This is milestone M0.5 of the plan: no clang fork. `llmc++` links against the
stock clang 19 libraries and runs clang's own driver in-process, with one extra
step in front of each C++ compile:

1. Make a length-preserving parsing view with each prompt blanked, then parse it
   with `__llm__` defined away. A preprocessor callback records each keyword.
2. Check each marked function or lambda: `void` return, a nonempty prompt, a
   definition in the main file, and so on.
3. Take the body from `.llmcache/`, or ask the agent. The agent never sees the
   source. It calls tools backed by the AST (`get_task`, `lookup`,
   `list_members`, ...) and checks candidates with `try_compile`, which compiles
   a copy of the file with the candidate in place.
4. Compile the main file with the bodies filled in.

Differences from PLAN.md:
- Bodies are generated after the whole file is parsed, not at the parse point.
  Lookups use the finished AST, filtered to declarations that come before the
  body.
- `__llm__` is supported only in the main file; in a header it is an error.
- The cache key is the function name, signature and prompt, without a context
  fingerprint.
- Diagnostics in generated code point into the rewritten file.

## Build

Needs Debian or Ubuntu on x86-64 with CMake 3.20+, a C++17 compiler, a CMake
build backend such as Make or Ninja, `python3`, `apt-get`, and `dpkg-deb`.
Root isn't needed: configuration downloads the Clang/LLVM 19 headers and
libraries into the selected build directory. Catch2 3.8.1 is fetched when a
compatible system package is not available.

```sh
./build.sh --parallel
ctest --test-dir build/out --output-on-failure
```

`build/out/` holds CMake's cache, downloaded dependencies, objects, and other
build churn. `build/install/` is the runnable bundle: its `bin/` directory
contains `llmc++`, `llmcpp-agent`, and `llmcpp-tests`; `lib/` contains the
Clang runtime library and resource headers required by `llmc++`.

The tests are written with Catch2 and use a scripted mock agent, so they need
neither an API key nor network access after configuration.

## Use

```sh
build/install/bin/llmc++ example.cpp -o example   # generate, compile and link
./example

build/install/bin/llmc++ --llm example.cpp        # write example.llm.cpp and stop
g++ example.llm.cpp                       # plain C++; any compiler builds it
```

Every clang++ option works. llmc++ adds:

| Option | Meaning |
|---|---|
| `--llm` | Write `<name>.llm.cpp` next to each input instead of compiling. `-o` names the output for a single input; with `-E` the output is preprocessed `<name>.llm.ii`. |
| `-fllm-agent=<command>` | The agent to run. Default: `$LLMCPP_AGENT`, else `llmcpp-agent` next to `llmc++`. |
| `-fllm-offline` | Use only cached bodies; a missing one is an error. |
| `-fllm-regenerate` | Ignore cached bodies and generate new ones. |
| `-fno-llm-cache` | Don't read or write the cache. |
| `-fllm-cache-dir=<dir>` | Cache directory. Default: `.llmcache` next to the source file. |
| `-fllm-dump` | Print each generated body. |
| `-fllm-dump-context` | Print what `get_task` and `get_context` return for each function, then stop. No LLM calls. |
| `-fllm-verbose` | Log every tool call. |
| `-fllm-quiet` | Don't print progress. |
| `-fllm-max-attempts=<n>` | Rejected submits allowed per function (default 4). |
| `-fllm-max-tool-calls=<n>` | Tool calls allowed per function (default 60). |
| `-fllm-timeout=<seconds>` | Time limit per function (default 600). |

Generated bodies are cached in `.llmcache/<key>.cpp`. Commit that directory to
make builds reproducible, and build with `-fllm-offline` in CI.

## The agent

`llmcpp-agent` is a Python script with no dependencies. It talks JSON-RPC (MCP)
with llmc++ over stdin/stdout and has two backends:

- `LLMCPP_BACKEND=anthropic` calls the Claude API directly. It needs
  `ANTHROPIC_API_KEY`; the model is `LLMCPP_MODEL` (default `claude-opus-5`).
- `LLMCPP_BACKEND=claude-code` runs `claude -p` with llmc++'s tools exposed as
  an MCP server, using your Claude Code login. `LLMCPP_MODEL` and
  `LLMCPP_EFFORT` (default `medium`) are passed on.

By default it uses the API when `ANTHROPIC_API_KEY` is set, and Claude Code
otherwise.

## Layout

```
src/        llmc++: driver, __llm__ pass, tool server, agent protocol
agent/      the reference agent
test/       Catch2 integration tests, test cases, and the mock agent
scripts/    fetch-deps.sh
```
