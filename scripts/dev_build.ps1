param(
  [string]$BuildDir = "build-dev"
)

$ErrorActionPreference = "Stop"

cmake -S . -B $BuildDir -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build $BuildDir
ctest --test-dir $BuildDir --output-on-failure -C Debug
python scripts/check_asm_safety.py

Write-Host "Build completed in $BuildDir"
