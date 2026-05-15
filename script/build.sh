#!/usr/bin/env bash
set -euo pipefail

cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER=clang \
    -DCMAKE_CXX_COMPILER=clang++ \
    -DCMAKE_CXX_FLAGS="-stdlib=libc++" \
    -DCMAKE_EXE_LINKER_FLAGS="-stdlib=libc++"

cmake --build build --target sandbox_app -j"$(nproc)"

find build -name 'sandbox_app' -type f -print