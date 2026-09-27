#!/bin/sh
set -eu

project_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
mkdir -p "$project_dir/build/docker"

# Keep container dependencies and build artifacts separate from native builds.
exec docker run --rm \
    --platform linux/amd64 \
    --user "$(id -u):$(id -g)" \
    --mount "type=bind,source=$project_dir,target=/workspace" \
    --mount type=volume,target=/workspace/deps \
    --mount "type=bind,source=$project_dir/build/docker,target=/workspace/build" \
    --workdir /workspace \
    yanevskiv:llmcpp-dev \
    ./do_build.sh "$@"
