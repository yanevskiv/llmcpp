#!/usr/bin/env bash
# Fetches the clang/LLVM 19 headers, libclang-cpp and clang's builtin headers
# into build/deps/root without root access (Debian/Ubuntu, via apt-get download).
#
# Uses the system copy of libLLVM.so.19.1 (package libllvm19) when it's present.
set -euo pipefail

LLVM_VER="${LLVM_VER:-19}"
ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
DEPS="${LLMCPP_DEPS_DIR:-$ROOT_DIR/build/deps}"
STAMP="$DEPS/root/.fetched-$LLVM_VER"

if [[ -f "$STAMP" ]]; then
  exit 0
fi

command -v apt-get >/dev/null || { echo "fetch-deps: apt-get not found (Debian/Ubuntu only)" >&2; exit 1; }
command -v dpkg-deb >/dev/null || { echo "fetch-deps: dpkg-deb not found" >&2; exit 1; }

mkdir -p "$DEPS/debs" "$DEPS/root"
cd "$DEPS/debs"

pkgs=(libclang-cpp$LLVM_VER libclang-$LLVM_VER-dev llvm-$LLVM_VER-dev libclang-common-$LLVM_VER-dev)
if [[ ! -e /usr/lib/x86_64-linux-gnu/libLLVM.so.$LLVM_VER.1 ]]; then
  pkgs+=(libllvm$LLVM_VER)
fi

echo "fetch-deps: downloading ${pkgs[*]}"
apt-get download "${pkgs[@]}" >/dev/null

# Extract only what the build needs; the dev packages are mostly static libraries.
extract() {
  local pkg="$1"; shift
  local deb
  deb="$(ls "$pkg"_*.deb | head -1)"
  dpkg-deb --fsys-tarfile "$deb" | tar -x -C "$DEPS/root" --wildcards "$@"
}

extract libclang-cpp$LLVM_VER './usr/lib/x86_64-linux-gnu/libclang-cpp.so.*'
extract libclang-$LLVM_VER-dev './usr/lib/llvm-'$LLVM_VER'/include/clang/*' './usr/lib/llvm-'$LLVM_VER'/include/clang-c/*'
extract llvm-$LLVM_VER-dev './usr/include/llvm-'$LLVM_VER'/*' './usr/include/llvm-c-'$LLVM_VER'/*'
extract libclang-common-$LLVM_VER-dev './usr/lib/llvm-'$LLVM_VER'/lib/clang/'$LLVM_VER'/include/*'
if [[ " ${pkgs[*]} " == *" libllvm$LLVM_VER "* ]]; then
  extract libllvm$LLVM_VER './usr/lib/x86_64-linux-gnu/libLLVM.so.*'
fi

touch "$STAMP"
echo "fetch-deps: done ($(du -sh "$DEPS/root" | cut -f1) in $DEPS/root)"
