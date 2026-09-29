# How to build, test, and clean

Use this guide when you need to rebuild the compiler, run its tests, or recover
space after experimenting. If this is your first build, follow the
[first tutorial](tutorials/build_the_compiler.md) for a complete walkthrough.
Downloaded dependencies and build artifacts stay under `deps/` and `build/`;
cleaning them does not touch system packages.

## Requirements

- Debian or Ubuntu on x86-64
- CMake 3.20 or newer and a C++17 compiler
- Make or Ninja
- `apt-get`, `dpkg-deb`, and either `curl` or `wget`
- Python 3 for the Codex and Claude Code adapters and the scripted test agent
- At least one generation backend: an OpenAI or Anthropic API key, Codex, or
  Claude Code

## Build

Fetch the pinned dependencies, then configure, build, and install the project:

```sh
./do_fetch_deps.sh
./do_build.sh --parallel 8
```

`do_fetch_deps.sh` extracts Clang/LLVM 19, OpenSSL development files, Catch2, and
the single-header `cpp-httplib` dependency into `deps/`. Re-running it reuses
what is already present.

`do_build.sh` creates the build tree under `build/out/` and installs a runnable
bundle under `build/install/`.

Documentation is built separately with `./do_build_docs.sh`.
Its build tree is `build/docs/`, and the HTML
and man pages install under the same `build/install/` prefix. See
[How to work on llmcpp](how_to_work_on_llmcpp.md#build-reference-documentation)
for the documentation tools.

## Test

Run the registered Catch2 integration tests with:

```sh
./do_tests.sh
```

The suite uses deterministic stand-ins for external agents and APIs. It does
not spend model tokens or require a live backend login.

## Clean

| Command | Removes |
| --- | --- |
| `./do_clean.sh` | `build/`, including compiled objects and the install bundle |
| `./do_clean_deps.sh` | Downloaded content under `deps/` |

Run `do_clean_deps.sh` only when you want the next build to download dependencies
again.

## Output layout

```text
deps/
  root/                 extracted compiler and system dependencies
  catch2/               Catch2 source
  cpp-httplib/          HTTP header
build/
  out/                  CMake cache, objects, and generated build files
  docs/                 documentation build tree and HTML
  install/
    bin/                 llmc++, llmcpp-agent, and llmcpp-tests
    lib/                 Clang/LLVM runtime libraries and resource headers
    share/doc/llmcpp/     HTML documentation
    share/man/man1/      compiler and agent command references
```

The installed `llmcpp-agent` sits beside `llmc++`, which lets the driver find
the Codex or Claude Code adapter without another path setting.
