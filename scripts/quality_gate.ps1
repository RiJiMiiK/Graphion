param(
  [string]$BuildDir = "build-quality"
)

$ErrorActionPreference = "Stop"

cmake -S . -B $BuildDir -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build $BuildDir
ctest --test-dir $BuildDir --output-on-failure -C Debug
python scripts/check_asm_safety.py
python scripts/run_clang_tidy.py --build-dir $BuildDir

if (Get-Command cppcheck -ErrorAction SilentlyContinue) {
  cppcheck --enable=warning,style,performance,portability --error-exitcode=1 --inline-suppr src
}

Write-Host "Quality gate passed ($BuildDir)"
