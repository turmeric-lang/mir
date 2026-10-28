#!/bin/sh
# Build and run the wasm32 FFI test under node.
# Requires the Emscripten SDK (emcc, em-config) and node on PATH.
#
# libc headers come from Emscripten's own musl sysroot, read off the host
# disk (-sNODERAWFS): the same headers a consumer serves c2mir from MEMFS,
# so a predefined macro that disagrees with clang fails here first.
# -sSAFE_HEAP traps a load or store through address 0, which a plain wasm
# module silently reads as zeroes.
set -e
cd "$(dirname "$0")/.."
SYSINC="$(em-config CACHE)/sysroot/include"
emcc -O0 -I. -Ic2mir wasm-tests/ffi-test.c mir.c c2mir/c2mir.c \
     -o wasm-tests/ffi-test.js \
     -sNODERAWFS=1 -sSAFE_HEAP=1 -sALLOW_TABLE_GROWTH=1 \
     -sTOTAL_MEMORY=256mb -sSTACK_SIZE=8mb
node wasm-tests/ffi-test.js "$SYSINC"
