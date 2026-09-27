#!/bin/sh
set -eu

# Keep documentation packages isolated from the system Python installation.
DOCS_VENV="$PWD/build/out/docs_venv"
if [ ! -x "${DOCS_VENV}/bin/python" ]; then
    python3 -m venv "${DOCS_VENV}"
fi
. "${DOCS_VENV}/bin/activate"
python -m pip install -r docs/web/requirements.txt

cmake -S . -B build/docs \
    -DLLMCPP_DOCS_ONLY=ON \
    -DCMAKE_INSTALL_PREFIX="$PWD/build/install"
cmake --build build/docs "$@"
cmake --install build/docs --component Documentation
