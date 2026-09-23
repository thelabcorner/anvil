# arch I9 P0 gate: full-corpus roundtrip across parse/backend modes.
# Usage: pwsh -File prototypes\i9-arch\gate_roundtrip.ps1 [-Exe build-i9-arch\anvil.exe]
param(
  [string]$Exe = 'build-i9-arch\anvil.exe',
  [string]$Corpus = 'tests\corpus',
  [string]$Log = 'prototypes\i9-arch\logs\gate_roundtrip.log'
)
$ErrorActionPreference = 'Continue'
New-Item -ItemType Directory -Force -Path (Split-Path $Log) | Out-Null
$exeHash = (Get-FileHash -LiteralPath $Exe -Algorithm SHA256).Hash
$srcHash = (Get-FileHash -LiteralPath 'src\anvil.cpp' -Algorithm SHA256).Hash
"# gate_roundtrip $(Get-Date -Format o)" | Set-Content -LiteralPath $Log
"# exe=$Exe sha256=$exeHash" | Add-Content -LiteralPath $Log
"# src/anvil.cpp sha256=$srcHash" | Add-Content -LiteralPath $Log

$specs = @(
  @{n='auto';        e=@('--parse=auto')},
  @{n='greedy';      e=@('--parse=greedy')},
  @{n='dp';          e=@('--parse=dp')},
  @{n='sparse';      e=@('--parse=sparse')},
  @{n='mdl';         e=@('--parse=mdl')},
  @{n='shape';       e=@('--parse=shape')},
  @{n='topology';    e=@('--parse=topology')},
  @{n='tcopy';       e=@('--parse=tcopy')},
  @{n='tcopy-pnra';  e=@('--parse=tcopy','--pnra=on')},
  @{n='hotop';       e=@('--parse=hotop')},
  @{n='hotop-rlzp';  e=@('--parse=hotop','--hotop-rlzp=on')},
  @{n='hotop-budget';e=@('--parse=hotop','--hotop-budget=on')},
  @{n='ariref';      e=@('--parse=auto','--ariref=on')},
  @{n='ratio';       e=@('--parse=ratio')},
  @{n='ratio-bwt';   e=@('--parse=ratio','--ratio-backend=bwt')},
  @{n='ratio-auto';  e=@('--parse=ratio','--ratio-backend=auto')},
  # --bwt-lzp=on is DISABLED with a clean up-front rejection (transform 3 dropped,
  # do-not-reburn 06 G2); reject=1 means a nonzero encode rc is the PASS condition.
  @{n='ratio-lzp-reject'; e=@('--parse=ratio','--ratio-backend=bwt','--bwt-lzp=on'); reject=$true}
)
$files = Get-ChildItem -LiteralPath $Corpus -File | Sort-Object Name
$tmp = Join-Path $env:TEMP ('arch-gate-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Force -Path $tmp | Out-Null
$pass=0; $fail=0
foreach ($f in $files) {
  $src = $f.FullName
  $srcSha = (Get-FileHash -LiteralPath $src -Algorithm SHA256).Hash
  $fsize = $f.Length
  foreach ($s in $specs) {
    $anv = Join-Path $tmp 'x.anv'; $out = Join-Path $tmp 'x.out'; $out4 = Join-Path $tmp 'x4.out'
    $eargs = @('c', $src, $anv, '--quiet') + $s.e
    & $Exe @eargs 2> (Join-Path $tmp 'enc.err') | Out-Null
    if ($s.reject) {
      if ($LASTEXITCODE -ne 0) { $pass++; "PASS $($f.Name) [$($s.n)] clean-reject rc=$LASTEXITCODE err=$(Get-Content (Join-Path $tmp 'enc.err') -Raw)" | Add-Content -LiteralPath $Log }
      else { $fail++; "FAIL $($f.Name) [$($s.n)] expected clean rejection, got rc=0" | Add-Content -LiteralPath $Log }
      continue
    }
    if ($LASTEXITCODE -ne 0) { $fail++; "FAIL $($f.Name) [$($s.n)] encode rc=$LASTEXITCODE $(Get-Content (Join-Path $tmp 'enc.err') -Raw)" | Add-Content -LiteralPath $Log; continue }
    $wire = (Get-FileHash -LiteralPath $anv -Algorithm SHA256).Hash
    $wireBytes = (Get-Item -LiteralPath $anv).Length
    & $Exe d $anv $out --quiet 2> (Join-Path $tmp 'dec.err') | Out-Null
    $rc1 = $LASTEXITCODE
    $sha1 = if (Test-Path $out) { (Get-FileHash -LiteralPath $out -Algorithm SHA256).Hash } else { 'MISSING' }
    & $Exe d $anv $out4 --quiet --decode-threads=4 2> (Join-Path $tmp 'dec4.err') | Out-Null
    $rc4 = $LASTEXITCODE
    $sha4 = if (Test-Path $out4) { (Get-FileHash -LiteralPath $out4 -Algorithm SHA256).Hash } else { 'MISSING' }
    if ($rc1 -eq 0 -and $rc4 -eq 0 -and $sha1 -eq $srcSha -and $sha4 -eq $srcSha) {
      $pass++; "PASS $($f.Name) [$($s.n)] in=$fsize wire=$wireBytes wire_sha=$wire 1t+4t ok" | Add-Content -LiteralPath $Log
    } else {
      $fail++; "FAIL $($f.Name) [$($s.n)] rc1=$rc1 rc4=$rc4 sha1=$sha1 sha4=$sha4 want=$srcSha enc=$(Get-Content (Join-Path $tmp 'enc.err') -Raw) dec=$(Get-Content (Join-Path $tmp 'dec.err') -Raw) dec4=$(Get-Content (Join-Path $tmp 'dec4.err') -Raw)" | Add-Content -LiteralPath $Log
    }
    Remove-Item -LiteralPath $anv,$out,$out4 -ErrorAction SilentlyContinue
  }
  "INFO file-done $($f.Name)" | Add-Content -LiteralPath $Log
}
"# SUMMARY pass=$pass fail=$fail total=$($pass+$fail)" | Add-Content -LiteralPath $Log
Remove-Item -Recurse -Force $tmp -ErrorAction SilentlyContinue
Write-Output "SUMMARY pass=$pass fail=$fail total=$($pass+$fail) log=$Log"
if ($fail -gt 0) { exit 1 } else { exit 0 }
