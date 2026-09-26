#!/bin/sh
set -eu

cmake -S . -B build/out -DCMAKE_INSTALL_PREFIX="$PWD/build/install"
cmake --build build/out "$@"
cmake --install build/out
