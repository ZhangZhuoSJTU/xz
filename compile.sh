#!/bin/bash
# ProgramBench-style build: must produce ./executable at the repo root.
set -e
cd "$(dirname "$0")"
sh autogen.sh --no-po4a --no-doxygen
./configure --disable-shared --disable-nls
make
cp src/xz/xz executable
