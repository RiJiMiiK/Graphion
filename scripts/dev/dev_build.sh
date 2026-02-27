#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${1:-build-dev}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

cmake -S . -B "${BUILD_DIR}" -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build "${BUILD_DIR}"
ctest --test-dir "${BUILD_DIR}" --output-on-failure -C Debug
python3 "${SCRIPT_DIR}/../quality/check_asm_safety.py"

echo "Build completed in ${BUILD_DIR}"
