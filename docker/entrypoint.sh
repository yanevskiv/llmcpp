#!/bin/sh
set -eu

# Give development tools a writable home when running as the host user.
if [ ! -w "${HOME:-/}" ]; then
    HOME=$(mktemp -d /tmp/llmcpp-home.XXXXXX)
    export HOME
fi

exec "$@"
