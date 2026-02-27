#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${1:-build-dev}"

cmake -S . -B "${BUILD_DIR}" -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build "${BUILD_DIR}"
python3 scripts/check_asm_safety.py

echo "Build completed in ${BUILD_DIR}"
