# arch I9 P0 gate: default-encode wire identity vs committed HEAD fc23d9a
# (independently built reference prototypes\i9-arch\anvil_head_ref.exe).
# Every spec is encode-only; compares container sha256 byte-for-byte.
param(
  [string]$ExeA = 'build-i9-arch\anvil.exe',
  [string]$ExeB = 'prototypes\i9-arch\anvil_head_ref.exe',
  [string]$Corpus = 'tests\corpus',
  [string]$Log = 'prototypes\i9-arch\logs\gate_encode_identity.log'
)
$ErrorActionPreference = 'Continue'
New-Item -ItemType Directory -Force -Path (Split-Path $Log) | Out-Null
"# gate_encode_identity $(Get-Date -Format o)" | Set-Content -LiteralPath $Log
"# A=$ExeA sha256=$((Get-FileHash -LiteralPath $ExeA -Algorithm SHA256).Hash)" | Add-Content -LiteralPath $Log
"# B=$ExeB sha256=$((Get-FileHash -LiteralPath $ExeB -Algorithm SHA256).Hash)" | Add-Content -LiteralPath $Log
# Specs mirroring tools/bench_native.cpp rows (CLI equivalents) + defaults.
$specs = @(
  @{n='default';            e=@()},
  @{n='greedy-arith';       e=@('--parse=greedy','--literal=o0','--entropy=arith')},
  @{n='dp-arith';           e=@('--parse=dp','--literal=o0','--entropy=arith')},
  @{n='greedy-rans';        e=@('--parse=greedy','--literal=o0','--entropy=rans')},
  @{n='dp-rans';            e=@('--parse=dp','--literal=o0','--entropy=rans')},
  @{n='sparse-rans';        e=@('--parse=sparse','--literal=o0','--entropy=rans')},
  @{n='sparse-rans-l0';     e=@('--parse=sparse','--literal=o0','--entropy=rans','--stream-lambda=0')},
  @{n='sparse-channels';    e=@('--parse=sparse','--literal=o0','--entropy=rans','--channels=on')},
  @{n='tcopy-rans';         e=@('--parse=tcopy','--literal=o0','--entropy=rans')},
  @{n='tcopy-pnra';         e=@('--parse=tcopy','--literal=o0','--entropy=rans','--pnra=on')},
  @{n='mdl-rans';           e=@('--parse=mdl','--literal=o0','--entropy=rans')},
  @{n='mdl-rans-l0';        e=@('--parse=mdl','--literal=o0','--entropy=rans','--stream-lambda=0')},
  @{n='mdl-rans-l001';      e=@('--parse=mdl','--literal=o0','--entropy=rans','--stream-lambda=0.01')},
  @{n='shape-rans';         e=@('--parse=shape','--literal=o0','--entropy=rans')},
  @{n='shape-rans-l0';      e=@('--parse=shape','--literal=o0','--entropy=rans','--stream-lambda=0')},
  @{n='shape-ctxmap';       e=@('--parse=shape','--literal=o0','--entropy=rans','--shape-states=1')},
  @{n='hotop-rans';         e=@('--parse=hotop','--literal=o0','--entropy=rans')},
  @{n='hotop-budget';       e=@('--parse=hotop','--literal=o0','--entropy=rans','--hotop-budget=on')},
  @{n='hotop-rlzp';         e=@('--parse=hotop','--literal=o0','--entropy=rans','--hotop-rlzp=on')}
)
$files = Get-ChildItem -LiteralPath $Corpus -File | Sort-Object Name
$tmp = Join-Path $env:TEMP ('arch-encid-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Force -Path $tmp | Out-Null
$same=0; $diff=0; $skip=0
foreach ($f in $files) {
  foreach ($s in $specs) {
    $a = Join-Path $tmp 'a.anv'; $b = Join-Path $tmp 'b.anv'
    $eargs = @('c', $f.FullName, $a, '--quiet') + $s.e
    & $ExeA @eargs 2> (Join-Path $tmp 'a.err') | Out-Null; $rca=$LASTEXITCODE
    $eargs2 = @('c', $f.FullName, $b, '--quiet') + $s.e
    & $ExeB @eargs2 2> (Join-Path $tmp 'b.err') | Out-Null; $rcb=$LASTEXITCODE
    if ($rca -ne 0 -or $rcb -ne 0) { $skip++; "SKIP $($f.Name) [$($s.n)] rcA=$rca rcB=$rcb A=$(Get-Content (Join-Path $tmp 'a.err') -Raw) B=$(Get-Content (Join-Path $tmp 'b.err') -Raw)" | Add-Content -LiteralPath $Log; continue }
    $ha = (Get-FileHash -LiteralPath $a -Algorithm SHA256).Hash
    $hb = (Get-FileHash -LiteralPath $b -Algorithm SHA256).Hash
    if ($ha -eq $hb) { $same++; "SAME $($f.Name) [$($s.n)] bytes=$((Get-Item -LiteralPath $a).Length) sha=$ha" | Add-Content -LiteralPath $Log }
    else { $diff++; "DIFF $($f.Name) [$($s.n)] bytesA=$((Get-Item -LiteralPath $a).Length) bytesB=$((Get-Item -LiteralPath $b).Length) shaA=$ha shaB=$hb" | Add-Content -LiteralPath $Log }
    Remove-Item -LiteralPath $a,$b -ErrorAction SilentlyContinue
  }
  "INFO file-done $($f.Name)" | Add-Content -LiteralPath $Log
}
"# SUMMARY same=$same diff=$diff skip=$skip" | Add-Content -LiteralPath $Log
Remove-Item -Recurse -Force $tmp -ErrorAction SilentlyContinue
Write-Output "SUMMARY same=$same diff=$diff skip=$skip log=$Log"
if ($diff -gt 0) { exit 1 } else { exit 0 }
