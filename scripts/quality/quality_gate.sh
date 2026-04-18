#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="${1:-build-quality}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CONFIGURE_ARGS=(
  -S .
  -B "${BUILD_DIR}"
  -G Ninja
  -DCMAKE_BUILD_TYPE=Debug
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
)

case "$(uname -s)" in
  MINGW*|MSYS*|CYGWIN*)
    echo "Quality gate: sanitizers disabled on Windows toolchains"
    ;;
  *)
    CONFIGURE_ARGS+=(-DGRAPHION_ENABLE_SANITIZERS=ON)
    ;;
esac

cmake "${CONFIGURE_ARGS[@]}"
cmake --build "${BUILD_DIR}"
ctest --test-dir "${BUILD_DIR}" --output-on-failure -C Debug
python3 "${SCRIPT_DIR}/check_asm_safety.py"
python3 "${SCRIPT_DIR}/run_clang_tidy.py" --build-dir "${BUILD_DIR}"

if command -v cppcheck >/dev/null 2>&1; then
  cppcheck --enable=warning,style,performance,portability --error-exitcode=1 --inline-suppr src
fi

echo "Quality gate passed (${BUILD_DIR})"
