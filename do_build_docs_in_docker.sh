#!/bin/sh
set -eu

project_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)

# Build and install documentation in the mounted project.
exec docker run -it --rm \
    --platform linux/amd64 \
    --user "$(id -u):$(id -g)" \
    --mount "type=bind,source=$project_dir,target=/workspace" \
    --workdir /workspace \
    yanevskiv:llmcpp-dev \
    ./do_build_docs.sh "$@"
