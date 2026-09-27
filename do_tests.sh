#!/bin/sh
set -eu

ctest --test-dir build/out --output-on-failure "$@"
