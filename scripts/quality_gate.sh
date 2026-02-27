#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${1:-build-quality}"

cmake -S . -B "${BUILD_DIR}" -G Ninja -DCMAKE_BUILD_TYPE=Debug -DGRAPHION_ENABLE_SANITIZERS=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build "${BUILD_DIR}"
ctest --test-dir "${BUILD_DIR}" --output-on-failure -C Debug
python3 scripts/check_asm_safety.py
python3 scripts/run_clang_tidy.py --build-dir "${BUILD_DIR}"

if command -v cppcheck >/dev/null 2>&1; then
  cppcheck --enable=warning,style,performance,portability --error-exitcode=1 --inline-suppr src
fi

echo "Quality gate passed (${BUILD_DIR})"
