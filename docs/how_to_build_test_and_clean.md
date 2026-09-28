# How to build, test, and clean

The build is self-contained under `deps/` and `build/`. You do not need root
access, and cleaning the project does not touch system packages.

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

## Test

Run the registered Catch2 integration tests with:

```sh
./do_test.sh
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
  install/
    bin/                 llmc++, llmcpp-agent, and llmcpp-tests
    lib/                 Clang/LLVM runtime libraries and resource headers
```

The installed `llmcpp-agent` sits beside `llmc++`, which lets the driver find
the Codex or Claude Code adapter without another path setting.
