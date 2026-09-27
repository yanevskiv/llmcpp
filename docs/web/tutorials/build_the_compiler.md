# Build the compiler

We will build a small program that sorts scores. Before writing it, get a
working compiler and choose the model that will generate its function body.
Run the commands in this page from the repository root.

## Prepare the host

On Debian or Ubuntu x86-64, install the compiler build tools:

```sh
sudo apt update
sudo apt install build-essential
sudo apt install cmake
sudo apt install python3
sudo apt install curl
sudo apt install ca-certificates
```

For now, we will use the native build.

## Build and install

```sh
./do_fetch_deps.sh
./do_build.sh --parallel 8
export PATH="$PWD/build/install/bin:$PATH"
```

The first command downloads local dependencies into `deps/`. The second builds
and installs the compiler and agent under `build/install/`.
The last command makes `llmc++` available in this shell.

## Choose a backend

If you already have an authenticated Codex CLI, select it:

```sh
export LLMCPP_BACKEND=codex
```

Alternatively, select the OpenAI API and provide your key:

```sh
export LLMCPP_BACKEND=openai
export OPENAI_API_KEY=your-api-key
```

Choose one of these setups, not both. Generation sends instructions and requested
compiler context to that backend and may incur usage charges.

The compiler is ready. Next, [generate your first function](generate_your_first_function.md).
