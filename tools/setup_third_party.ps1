# Build the Brotli + Zstd reference libraries used by the ANVIL native
# benchmark harness. Produces static .lib files under third_party/install.
# Usage:  pwsh -File tools/setup_third_party.ps1   (run from repo root; load env.ps1 first)
param([switch]$SkipFetch)

$ErrorActionPreference = "Stop"
$root = Split-Path $PSScriptRoot -Parent
$install = Join-Path $root "third_party\install"

if (-not $SkipFetch) {
    if (-not (Test-Path "$root\third_party\brotli\CMakeLists.txt")) {
        git clone --depth 1 https://github.com/google/brotli "$root\third_party\brotli"
    }
    if (-not (Test-Path "$root\third_party\zstd\lib\zstd.h")) {
        git clone --depth 1 https://github.com/facebook/zstd "$root\third_party\zstd"
        git -C "$root\third_party\zstd" fetch --depth 1 origin tag v1.5.7
        git -C "$root\third_party\zstd" checkout v1.5.7
    }
}

New-Item -ItemType Directory -Force -Path $install | Out-Null

# Brotli: static
cmake -S "$root\third_party\brotli" -B "$root\third_party\brotli\build" -G Ninja `
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang-cl -DBUILD_SHARED_LIBS=OFF `
  -DCMAKE_SUPPRESS_REGENERATION=ON -DBROTLI_DISABLE_TESTS=ON `
  -DCMAKE_INSTALL_PREFIX=$install | Out-Null
if ($LASTEXITCODE -ne 0) { throw "brotli configure failed" }
ninja -C "$root\third_party\brotli\build" install | Out-Null
if ($LASTEXITCODE -ne 0) { throw "brotli build failed" }

# Zstd 1.5.7: static. Note: zstd's build dir is named 'build', so output to 'cmbuild'.
cmake -S "$root\third_party\zstd\build\cmake" -B "$root\third_party\zstd\cmbuild" -G Ninja `
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl `
  -DCMAKE_SUPPRESS_REGENERATION=ON -DZSTD_BUILD_PROGRAMS=OFF -DZSTD_BUILD_TESTS=OFF `
  -DZSTD_BUILD_SHARED=OFF -DZSTD_BUILD_STATIC=ON `
  -DCMAKE_INSTALL_PREFIX=$install | Out-Null
if ($LASTEXITCODE -ne 0) { throw "zstd configure failed" }
ninja -C "$root\third_party\zstd\cmbuild" install | Out-Null
if ($LASTEXITCODE -ne 0) { throw "zstd build failed" }

# Drop stale shared DLLs (if a shared brotli build leaked into install/bin)
Remove-Item "$install\bin\brotli*.dll" -ErrorAction SilentlyContinue

Write-Host "third_party ready:"
Get-ChildItem "$install\lib" -Filter *.lib | ForEach-Object { Write-Host "  $($_.Name)" }
