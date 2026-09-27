#!/bin/sh
set -eu

# Prefer the project's documentation environment when it is installed.
if [ -x "$PWD/build/docs-venv/bin/sphinx-build" ]; then
    PATH="$PWD/build/docs-venv/bin:$PATH"
    export PATH
fi

if ! command -v sphinx-build >/dev/null 2>&1; then
    echo "Sphinx is missing. Install the documentation tools:" >&2
    echo "python3 -m venv build/docs-venv" >&2
    echo "build/docs-venv/bin/pip install -r docs/web/requirements.txt" >&2
    exit 1
fi

cmake -S . -B build/docs \
    -DLLMCPP_DOCS_ONLY=ON \
    -DCMAKE_INSTALL_PREFIX="$PWD/build/install"
cmake --build build/docs "$@"
cmake --install build/docs --component Documentation
