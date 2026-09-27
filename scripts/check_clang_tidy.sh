#!/bin/sh
set -eu

if ! command -v clang-tidy >/dev/null 2>&1; then
    echo "clang-tidy is required to check the naming contract" >&2
    exit 1
fi

build_dir=build/out
if [ ! -f "$build_dir/compile_commands.json" ]; then
    echo "build the project with ./build.sh before running the naming check" >&2
    exit 1
fi

resource_dir=$(find "$build_dir/lib/clang" -mindepth 1 -maxdepth 1 -type d -print -quit)
if [ -z "$resource_dir" ]; then
    echo "Clang resource headers are missing from $build_dir" >&2
    exit 1
fi

find src tests -type f -name '*.cpp' ! -path 'tests/test_*' -print0 |
    xargs -0 -n 1 clang-tidy -p "$build_dir" \
        --extra-arg="-resource-dir=$resource_dir" \
        --header-filter='^.*/include/llmcpp/.*' \
        --warnings-as-errors='readability-identifier-naming'
