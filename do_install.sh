#!/bin/sh
set -eu

PROJECT_ROOT=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
INSTALL_SOURCE="${PROJECT_ROOT}/build/install"
LOCAL_PREFIX="${HOME:?HOME is required}/.local"

if [ ! -x "${INSTALL_SOURCE}/bin/llmc++" ] ||
   [ ! -x "${INSTALL_SOURCE}/bin/llmcpp-agent" ] ||
   [ ! -f "${INSTALL_SOURCE}/share/man/man1/llmc++.1" ] ||
   [ ! -f "${INSTALL_SOURCE}/share/man/man1/llmcpp-agent.1" ]; then
    printf '%s\n' 'Build llmcpp first with ./do_build.sh.' >&2
    exit 1
fi

mkdir -p "${LOCAL_PREFIX}"
cp -R "${INSTALL_SOURCE}/." "${LOCAL_PREFIX}/"

printf 'Installed llmcpp under %s\n' "${LOCAL_PREFIX}"
printf '%s\n' 'If needed, add these lines to your shell configuration:'
printf '%s\n' 'export PATH="$HOME/.local/bin:$PATH"'
printf '%s\n' 'export MANPATH="$HOME/.local/share/man:${MANPATH-}"'
