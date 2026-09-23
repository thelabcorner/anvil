# build_i9.ps1 -- decode-perf I9 lane build.
# Snapshots (never edits) src/anvil.cpp into this lane, records provenance
# (HEAD, worktree diff stat, sha256), then builds:
#   prof_i9.exe   -- decode time-share profiler (snapshot compiled in, ANVIL_NO_MAIN)
#   snap_anvil.exe-- snapshot CLI (same source, with main) used to build containers
# Flags: clang-cl, -O3 -DNDEBUG -std:c++20, /STACK:8388608 (matches CMake Release
# flags in CMakeLists.txt + build/build-info.txt). libsais is linked for the BWT
# ratio backend; Brotli is deliberately NOT defined in this harness build (not
# needed for mode-15/17-bwt paths).
param([switch]$Head)
Set-Location $PSScriptRoot
$ErrorActionPreference = "Continue"
. ..\..\env.ps1
$ErrorActionPreference = "Stop"

if ($Head) {
  cmd /c "git -C ..\.. show HEAD:src/anvil.cpp > anvil_snapshot.cpp"
  Write-Host "snapshot source: git HEAD (committed state)"
} else {
  Copy-Item ..\..\src\anvil.cpp .\anvil_snapshot.cpp -Force
  Write-Host "snapshot source: working tree (may be mid-edit by arch)"
}
$prov = @()
$prov += "HEAD: " + (git -C ..\.. rev-parse HEAD)
$prov += "branch: " + (git -C ..\.. rev-parse --abbrev-ref HEAD)
$prov += "snapshot source: " + $(if ($Head) { "git HEAD" } else { "working tree" })
$prov += "snapshot taken: " + (Get-Date -Format o)
$prov += "worktree diff-vs-HEAD (src): " + ((git -C ..\.. diff --stat -- src/anvil.cpp | Out-String).Trim() -replace "`r?`n", " | ")
$prov += "snapshot sha256: " + (Get-FileHash .\anvil_snapshot.cpp -Algorithm SHA256).Hash
$prov += "snapshot bytes: " + (Get-Item .\anvil_snapshot.cpp).Length
$prov | Set-Content .\snapshot_provenance.txt
$prov | ForEach-Object { Write-Host "  $_" }

$saisInc = "..\..\third_party\libsais\include"
$saisSrc = "..\..\third_party\libsais\src\libsais.c"

if (-not (Test-Path $saisSrc)) { throw "libsais source missing: $saisSrc (run tools/setup_third_party.ps1)" }

$defs = @()
if ($Head) { $defs += "/DPROF_HEAD_SNAPSHOT=1" }   # HEAD decompress() has no Options param

Write-Host "building prof_i9.exe ..."
& clang-cl /O2 /EHsc /DNDEBUG /std:c++20 /D_CRT_SECURE_NO_WARNINGS /DANVIL_HAVE_LIBSAIS=1 @defs /I $saisInc prof_i9.cpp $saisSrc /Fe:prof_i9.exe /link /STACK:8388608
if ($LASTEXITCODE -ne 0) { throw "prof_i9 build failed" }

Write-Host "building snap_anvil.exe ..."
& clang-cl /O2 /EHsc /DNDEBUG /std:c++20 /D_CRT_SECURE_NO_WARNINGS /DANVIL_HAVE_LIBSAIS=1 /I $saisInc anvil_snapshot.cpp $saisSrc /Fe:snap_anvil.exe /link /STACK:8388608
if ($LASTEXITCODE -ne 0) { throw "snap_anvil build failed" }

Write-Host "building bwt_prof.exe ..."
& clang-cl /O2 /EHsc /DNDEBUG /std:c++20 /D_CRT_SECURE_NO_WARNINGS /DANVIL_HAVE_LIBSAIS=1 @defs /I $saisInc bwt_prof.cpp $saisSrc /Fe:bwt_prof.exe /link /STACK:8388608
if ($LASTEXITCODE -ne 0) { throw "bwt_prof build failed" }

Write-Host "OK:"
Get-Item .\prof_i9.exe, .\snap_anvil.exe | Select-Object Name, Length, LastWriteTime
