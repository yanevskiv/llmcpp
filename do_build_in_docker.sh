#!/bin/sh
set -eu

project_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)

# Build in the mounted project using the image's dependencies.
exec docker run -it --rm \
    --platform linux/amd64 \
    --user "$(id -u):$(id -g)" \
    --mount "type=bind,source=$project_dir,target=/workspace" \
    --mount type=volume,target=/workspace/deps \
    --workdir /workspace \
    yanevskiv:llmcpp-dev \
    ./do_build.sh "$@"
