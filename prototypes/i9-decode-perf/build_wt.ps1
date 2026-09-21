# build_wt.ps1 -- decode-perf I9: build the CURRENT WORKTREE source as a lane-private
# snapshot. If the known decode_one_block payload-vs-mode pointer defect is still
# present (both serial and parallel paths), a minimal exactly-located 2-hunk repair
# is applied to the COPY and recorded; if arch's fix is present (decode_one_block
# takes the mode as an explicit parameter), the snapshot is used UNMODIFIED.
# This lane never edits src/anvil.cpp.
# Builds the current-path binaries:
#   prof_i9_wt.exe / snap_anvil_wt.exe / bwt_prof_wt.exe
# (worktree = slicing-by-8 CRC + parallel-decode plumbing, decode_threads=1 used).
$ErrorActionPreference = "Continue"
Set-Location $PSScriptRoot
. ..\..\env.ps1
$ErrorActionPreference = "Stop"

$raw = Get-Content ..\..\src\anvil.cpp -Raw
$rawSha = (Get-FileHash ..\..\src\anvil.cpp -Algorithm SHA256).Hash
Set-Content .\anvil_wt_raw.cpp -Value $raw -NoNewline
$src = $raw -replace "`r`n", "`n"
$nl = [char]10

function Apply-Patch([string]$s, [string[]]$oldLines, [string[]]$newLines, [string]$tag) {
  $old = ($oldLines -join $nl)
  $new = ($newLines -join $nl)
  $count = ([regex]::Matches($s, [regex]::Escape($old))).Count
  if ($count -ne 1) { throw "repair hunk '$tag' matched $count times (expected 1) - source changed under this lane; aborting" }
  return $s.Replace($old, $new)
}

$defectPresent = $src.Contains('auto b=decode_one_block(p,static_cast<size_t>(plen)')
$repairState = ""
if ($defectPresent) {
  $src = Apply-Patch $src @(
    '            if(p>=e) throw std::runtime_error("truncated block header");',
    '            uint8_t mode=*p++; uint64_t plen=get_uvar(p,e); uint32_t expected_crc=get_u32le(p,e);',
    '            if(plen>uint64_t(e-p)) throw std::runtime_error("truncated block payload");',
    '            if(blen>total-out.size()) throw std::runtime_error("block exceeds declared output");',
    '            auto b=decode_one_block(p,static_cast<size_t>(plen),static_cast<size_t>(blen),expected_crc,revision,block_size,e,opt);'
  ) @(
    '            if(p>=e) throw std::runtime_error("truncated block header");',
    '            const uint8_t* pmode=p; // DECODE-PERF REPAIR: decode_one_block expects the mode byte',
    '            uint8_t mode=*p++; uint64_t plen=get_uvar(p,e); uint32_t expected_crc=get_u32le(p,e);',
    '            if(plen>uint64_t(e-p)) throw std::runtime_error("truncated block payload");',
    '            if(blen>total-out.size()) throw std::runtime_error("block exceeds declared output");',
    '            auto b=decode_one_block(pmode,static_cast<size_t>(plen),static_cast<size_t>(blen),expected_crc,revision,block_size,e,opt);'
  ) "serial-mode-byte"
  $src = Apply-Patch $src @(
    '            const uint8_t* bp=q; // points at the mode byte (decode_one_block expects this)',
    '            uint64_t blen=get_uvar(q,e); if(blen==0 || blen>block_size) throw std::runtime_error("invalid block length");',
    '            if(q>=e) throw std::runtime_error("truncated block header");',
    '            uint8_t mode=*q++; uint64_t plen=get_uvar(q,e); uint32_t expected_crc=get_u32le(q,e);'
  ) @(
    '            uint64_t blen=get_uvar(q,e); if(blen==0 || blen>block_size) throw std::runtime_error("invalid block length");',
    '            if(q>=e) throw std::runtime_error("truncated block header");',
    '            const uint8_t* bp=q; // DECODE-PERF REPAIR: points at the mode byte (decode_one_block expects this)',
    '            uint8_t mode=*q++; uint64_t plen=get_uvar(q,e); uint32_t expected_crc=get_u32le(q,e);'
  ) "parallel-mode-byte"
  $repairState = "REPAIRED in lane copy (2 hunks): defect was present in the raw worktree"
} else {
  $repairState = "NONE - arch's decode_one_block(mode,p,...) fix is present in the raw worktree; snapshot used UNMODIFIED"
}

Set-Content .\anvil_snapshot_wt.cpp -Value $src -NoNewline
$patchedSha = (Get-FileHash .\anvil_snapshot_wt.cpp -Algorithm SHA256).Hash

# API-compat detection: some intermediate worktree revisions have a one-arg
# decompress() and no Options::decode_threads (parallel plumbing reverted).
# /DPROF_HEAD_SNAPSHOT=1 selects the one-arg call wrapper + skips decode_threads.
$defs = @("/DANVIL_SNAPSHOT_NAME=anvil_snapshot_wt.cpp")
$oneArgSig = [regex]::IsMatch($src, 'static std::vector<uint8_t> decompress\(const std::vector<uint8_t>& in\)')
$twoArgSig = [regex]::IsMatch($src, 'static std::vector<uint8_t> decompress\(const std::vector<uint8_t>& in\s*,\s*const Options& opt\)')
$oneArg = $oneArgSig -and (-not $twoArgSig)
if ($oneArg) { $defs += "/DPROF_HEAD_SNAPSHOT=1" }
Write-Host "  api-compat: one-arg decompress = $oneArg; defs = $($defs -join ' ')"

$prov = @()
$prov += "HEAD: " + (git -C ..\.. rev-parse HEAD)
$prov += "branch: " + (git -C ..\.. rev-parse --abbrev-ref HEAD)
$prov += "snapshot source: WORKING TREE (raw copy: anvil_wt_raw.cpp)"
$prov += "snapshot taken: " + (Get-Date -Format o)
$prov += "worktree diff-vs-HEAD (src): " + ((git -C ..\.. diff --stat -- src/anvil.cpp | Out-String).Trim() -replace "`r?`n", " | ")
$prov += "raw copy sha256: $rawSha"
$prov += "snapshot (possibly repaired) sha256: $patchedSha"
$prov += "repair state: $repairState"
$prov += "api-compat: one-arg decompress = $oneArg; extra defs = $($defs -join ' ')"
$prov | Set-Content .\snapshot_provenance_wt.txt
$prov | ForEach-Object { Write-Host "  $_" }

$saisInc = "..\..\third_party\libsais\include"
$saisSrc = "..\..\third_party\libsais\src\libsais.c"

Write-Host "building prof_i9_wt.exe (worktree snapshot) ..."
& clang-cl /O2 /EHsc /DNDEBUG /std:c++20 /D_CRT_SECURE_NO_WARNINGS /DANVIL_HAVE_LIBSAIS=1 @defs /I $saisInc prof_i9.cpp $saisSrc /Fe:prof_i9_wt.exe /link /STACK:8388608
if ($LASTEXITCODE -ne 0) { throw "prof_i9_wt build failed" }

Write-Host "building snap_anvil_wt.exe (worktree snapshot) ..."
& clang-cl /O2 /EHsc /DNDEBUG /std:c++20 /D_CRT_SECURE_NO_WARNINGS /DANVIL_HAVE_LIBSAIS=1 /I $saisInc anvil_snapshot_wt.cpp $saisSrc /Fe:snap_anvil_wt.exe /link /STACK:8388608
if ($LASTEXITCODE -ne 0) { throw "snap_anvil_wt build failed" }

Write-Host "building bwt_prof_wt.exe (worktree snapshot) ..."
& clang-cl /O2 /EHsc /DNDEBUG /std:c++20 /D_CRT_SECURE_NO_WARNINGS /DANVIL_HAVE_LIBSAIS=1 @defs /I $saisInc bwt_prof.cpp $saisSrc /Fe:bwt_prof_wt.exe /link /STACK:8388608
if ($LASTEXITCODE -ne 0) { throw "bwt_prof_wt build failed" }

Write-Host "building post_prof_wt.exe (worktree snapshot) ..."
& clang-cl /O2 /EHsc /DNDEBUG /std:c++20 /D_CRT_SECURE_NO_WARNINGS /DANVIL_HAVE_LIBSAIS=1 @defs /I $saisInc post_prof.cpp $saisSrc /Fe:post_prof_wt.exe /link /STACK:8388608
if ($LASTEXITCODE -ne 0) { throw "post_prof_wt build failed" }

Write-Host "OK:"
Get-Item .\prof_i9_wt.exe, .\snap_anvil_wt.exe, .\bwt_prof_wt.exe | Select-Object Name, Length
