param(
  [string]$BuildDir = "build-quality"
)

$ErrorActionPreference = "Stop"

$ConfigureArgs = @(
  "-S", ".",
  "-B", $BuildDir,
  "-G", "Ninja",
  "-DCMAKE_BUILD_TYPE=Debug",
  "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
)

if (-not $IsWindows) {
  $ConfigureArgs += "-DGRAPHION_ENABLE_SANITIZERS=ON"
} else {
  Write-Host "Quality gate: sanitizers disabled on Windows toolchains"
}

cmake @ConfigureArgs
cmake --build $BuildDir
ctest --test-dir $BuildDir --output-on-failure -C Debug
python "$PSScriptRoot\run_clang_tidy.py" --build-dir $BuildDir

if (Get-Command cppcheck -ErrorAction SilentlyContinue) {
  cppcheck --enable=warning,style,performance,portability --error-exitcode=1 --inline-suppr src
}

Write-Host "Quality gate passed ($BuildDir)"
