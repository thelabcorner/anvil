<#:_power_shell
<#
.SYNOPSIS
    ONE pinned build entry point for the ANVIL project.

.DESCRIPTION
    The build/measure work in Project ANVIL has historically suffered from a
    compiler confound: the refreshed reference (Brotli/xz/zstd) timings were
    taken with a clang-cl build, while the first BWT measurements used an MSVC
    build. Byte counts are deterministic and comparable; THROUGHPUT is not.

    This script is the only supported way to produce benchmark binaries. It:
      1. enters the VS SDK environment (vcvars64.bat) via the same mechanism as
         env.ps1, so the CRT/SDK are found;
      2. PINS the compiler explicitly to LLVM clang-cl (never the launching
         shell's default, which could be MSVC cl.exe);
      3. builds C (libsais) and C++ (anvil / brotli_lw / anvil_bench) with one
         consistent compiler and one consistent flag set;
      4. records compiler id + version + full flags + git HEAD + worktree
         status + every executable SHA-256 into build/build-info.txt.

    Usage:
        .\tools\build_release.ps1                 # configure + build + record
        .\tools\build_release.ps1 -ConfigureOnly  # only reconfigure CMake
        .\tools\build_release.ps1 -Targets anvil  # build only the named targets
        .\tools\build_release.ps1 -DecoderOnly    # (reserved) decoder-only target

    This script MUST be dot-sourced AFTER . .\env.ps1 OR standalone: it loads
    env.ps1 internally so the toolchain PATH is correct. Run it from the repo
    root (C:\Users\<user>\Documents\ANVIL).
#>
[CmdletBinding()]
param(
    [string[]] $Targets = @("anvil", "brotli_lw", "anvil_bench"),
    [switch]   $ConfigureOnly,
    [switch]   $NoBuild
)

$ErrorActionPreference = "Stop"
$repo = Get-Location
if (-not (Test-Path (Join-Path $repo "CMakeLists.txt"))) {
    throw "Run build_release.ps1 from the ANVIL repo root (CMakeLists.txt not found in $(Get-Location))"
}

# ---------------------------------------------------------------------------
# 1. Load the VS SDK + LLVM environment the same way env.ps1 does.
# ---------------------------------------------------------------------------
$envPs1 = Join-Path $repo "env.ps1"
if (Test-Path -LiteralPath $envPs1) {
    Write-Host "[build_release] sourcing env.ps1 (VC SDK + LLVM on PATH)" -ForegroundColor Cyan
    & $envPs1 | Out-Null
} else {
    Write-Warning "env.ps1 not found; relying on ambient PATH/SDK"
}

# ---------------------------------------------------------------------------
# 2. PIN the compiler. We refuse to build if clang-cl is not the resolved
#    compiler. This is the whole point: the launching shell must not decide.
# ---------------------------------------------------------------------------
$clangCl = Get-Command clang-cl -ErrorAction SilentlyContinue
if (-not $clangCl) {
    throw "clang-cl not found on PATH after env.ps1. The pinned toolchain is LLVM clang-cl. Refusing to build with an unknown compiler."
}
$clangClExe = $clangCl.Source
$clangVer = & $clangClExe --version 2>&1 | Select-Object -First 1
Write-Host "[build_release] PINNED C/C++ compiler: $clangClExe" -ForegroundColor Cyan
Write-Host "[build_release] $clangVer" -ForegroundColor Cyan

# ---------------------------------------------------------------------------
# 3. Consistent flags. Optimized Release, deterministic, C++20.
#    -DNDEBUG everywhere; -O3 for both C and C++ so libsais and the codec are
#    built with the same optimization level.
# ---------------------------------------------------------------------------
$cxxFlags = "-O3 -DNDEBUG -std:c++20 -Wno-everything"
$cFlags   = "-O3 -DNDEBUG"
$cmakeGenerator = "Ninja"
$buildDir = Join-Path $repo "build"

# ---------------------------------------------------------------------------
# 4. Configure (idempotent; CMAKE_SUPPRESS_REGENERATION avoids the CMake/Ninja
#    "manifest still dirty" loop). Force the compilers explicitly.
# ---------------------------------------------------------------------------
if (-not (Test-Path -LiteralPath (Join-Path $buildDir "build.ninja"))) {
    Write-Host "[build_release] configuring CMake ($cmakeGenerator, clang-cl)..." -ForegroundColor Cyan
    & cmake -S $repo -B $buildDir -G $cmakeGenerator `
        -DCMAKE_BUILD_TYPE=Release `
        -DCMAKE_C_COMPILER="$clangClExe" `
        -DCMAKE_CXX_COMPILER="$clangClExe" `
        -DCMAKE_C_FLAGS="$cFlags" `
        -DCMAKE_CXX_FLAGS="$cxxFlags" `
        -DCMAKE_SUPPRESS_REGENERATION=ON
    if ($LASTEXITCODE -ne 0) { throw "CMake configure failed (exit $LASTEXITCODE)" }
} else {
    Write-Host "[build_release] build/ already configured; verifying pinned compiler..." -ForegroundColor Cyan
    $cachedCxx = (Get-Content (Join-Path $buildDir "CMakeCache.txt") | Where-Object { $_ -match "CMAKE_CXX_COMPILER:STRING=(.*)" } | Select-Object -First 1).Split("=")[1]
    if ($cachedCxx -and ($cachedCxx.Trim() -ne $clangClExe)) {
        Write-Host "[build_release] cached compiler ($cachedCxx) differs from pinned clang-cl ($clangClExe); reconfiguring." -ForegroundColor Yellow
        & cmake -S $repo -B $buildDir -G $cmakeGenerator `
            -DCMAKE_BUILD_TYPE=Release `
            -DCMAKE_C_COMPILER="$clangClExe" `
            -DCMAKE_CXX_COMPILER="$clangClExe" `
            -DCMAKE_C_FLAGS="$cFlags" `
            -DCMAKE_CXX_FLAGS="$cxxFlags" `
            -DCMAKE_SUPPRESS_REGENERATION=ON
        if ($LASTEXITCODE -ne 0) { throw "CMake reconfigure failed (exit $LASTEXITCODE)" }
    }
}

if ($ConfigureOnly) {
    Write-Host "[build_release] -ConfigureOnly set; stopping before build." -ForegroundColor Green
    exit 0
}

# ---------------------------------------------------------------------------
# 5. Build the requested targets with one consistent toolchain.
# ---------------------------------------------------------------------------
if (-not $NoBuild) {
    foreach ($t in $Targets) {
        Write-Host "[build_release] building target: $t" -ForegroundColor Cyan
        & cmake --build $buildDir --target $t -- -v
        if ($LASTEXITCODE -ne 0) { throw "Build of '$t' failed (exit $LASTEXITCODE)" }
    }
}

# ---------------------------------------------------------------------------
# 6. Record provenance: compiler, flags, git HEAD, worktree status, hashes.
# ---------------------------------------------------------------------------
function Get-Sha256([string]$path) {
    if (-not (Test-Path -LiteralPath $path)) { return "<MISSING>" }
    $h = (Get-FileHash -Algorithm SHA256 -LiteralPath $path).Hash
    return $h
}

$gitHead = (& git rev-parse HEAD 2>$null)
$gitBranch = (& git rev-parse --abbrev-ref HEAD 2>$null)
$gitStatus = (& git status --porcelain=v1 2>$null)
$worktreeDirty = if ($gitStatus) { "DIRTY" } else { "clean" }

$exePaths = @(
    (Join-Path $buildDir "anvil.exe"),
    (Join-Path $buildDir "brotli_lw.exe"),
    (Join-Path $buildDir "anvil_bench.exe")
)

$info = @()
$info += "# ANVIL pinned build record"
$info += "# Generated by tools/build_release.ps1"
$info += "build_time_utc      = $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss UTC' -AsUTC)"
$info += "host                = $env:COMPUTERNAME"
$info += "repo                = $repo"
$info += "git_head            = $gitHead"
$info += "git_branch          = $gitBranch"
$info += "worktree_status     = $worktreeDirty"
if ($gitStatus) {
    $info += "worktree_modified   ="
    foreach ($line in $gitStatus) { $info += "    $line" }
}
$info += ""
$info += "# --- PINNED COMPILER (same for C and C++) ---"
$info += "compiler_path       = $clangClExe"
$info += "compiler_version    = $clangVer"
$info += "c_flags             = $cFlags"
$info += "cxx_flags           = $cxxFlags"
$info += "cmake_generator     = $cmakeGenerator"
$info += ""
$info += "# --- ARTIFACTS (SHA-256) ---"
foreach ($p in $exePaths) {
    $rel = Resolve-Path -LiteralPath $p -Relative -ErrorAction SilentlyContinue
    $info += ("{0,-20} = {1}  ({2})" -f (Split-Path $p -Leaf), (Get-Sha256 $p), $rel)
}

$infoPath = Join-Path $buildDir "build-info.txt"
$info | Set-Content -LiteralPath $infoPath -Encoding utf8
Write-Host "[build_release] wrote $infoPath" -ForegroundColor Green
Write-Host ($info -join "`n") -ForegroundColor Green

# Emit a machine-readable companion for benchmark scripts to consume.
$json = @{
    build_time_utc = (Get-Date -AsUTC -Format "o")
    git_head       = $gitHead
    git_branch     = $gitBranch
    worktree_status = $worktreeDirty
    compiler_path  = $clangClExe
    compiler_version = "$clangVer"
    c_flags        = $cFlags
    cxx_flags      = $cxxFlags
    artifacts = @{}
}
foreach ($p in $exePaths) {
    $json.artifacts[(Split-Path $p -Leaf)] = [PSCustomObject]@{
        path = $p
        sha256 = (Get-Sha256 $p)
    }
}
$jsonPath = Join-Path $buildDir "build-info.json"
$json | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $jsonPath -Encoding utf8
Write-Host "[build_release] wrote $jsonPath" -ForegroundColor Green
