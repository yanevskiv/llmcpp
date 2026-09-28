#!/bin/sh
set -eu

project_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)

# The dependency layout currently requires x86-64 Linux.
exec docker buildx build \
    --platform linux/amd64 \
    --load \
    --tag yanevskiv:llmcpp-dev \
    --file "$project_dir/docker/Dockerfile" \
    "$@" \
    "$project_dir"
