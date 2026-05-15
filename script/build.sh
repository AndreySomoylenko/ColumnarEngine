#!/usr/bin/env bash
set -euo pipefail

cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

cmake --build build --target sandbox_app -j"$(nproc)"

echo "Built files:"
find build -name 'sandbox_app' -type f -print