# run_bwt.ps1 -- BWT-routed Silesia decode stage split (bwtinv request), attested.
# Containers: canonical anvil-bwt-direct flags (--ratio-context=off --ratio-lines=off),
# built by the HEAD snapshot CLI. Round-trip verified before timing.
param(
  [int]$Reps = 7,
  [string]$Label = "i9_bwt",
  [string[]]$Files = @("dickens", "webster"),
  [switch]$Wt
)
$ErrorActionPreference = "Stop"
Set-Location $PSScriptRoot
New-Item -ItemType Directory -Force -Path containers, results, attest | Out-Null
$sfx = if ($Wt) { "_wt" } else { "" }
$provFile = if ($Wt) { "snapshot_provenance_wt.txt" } else { "snapshot_provenance.txt" }
$Files = @($Files | ForEach-Object { $_ -split ',' } | Where-Object { $_ })
if (-not (Test-Path ".\bwt_prof$sfx.exe")) { throw "bwt_prof$sfx.exe missing (build script)" }

$srcDir = "..\..\scratch\ratio-first\corpora\silesia"
$rows = @()
foreach ($f in $Files) {
  $src = Join-Path $srcDir $f
  $out = "containers\bwt_$f.anv"
  if (-not (Test-Path $out)) {
    & ".\snap_anvil$sfx.exe" c $src $out --parse=ratio --ratio-backend=bwt --ratio-context=off --ratio-lines=off --quiet
    if ($LASTEXITCODE -ne 0) { throw "encode failed: $f" }
  }
  $dec = "containers\bwt_$f.dec"
  & ".\snap_anvil$sfx.exe" d $out $dec --quiet
  if ($LASTEXITCODE -ne 0) { throw "decode failed: $f" }
  $ok = ((Get-FileHash $src -Algorithm SHA256).Hash -eq (Get-FileHash $dec -Algorithm SHA256).Hash)
  $rows += ("{0} comp_bytes={1} roundtrip={2} src_sha256={3}" -f $f, (Get-Item $out).Length, $(if ($ok) { "OK" } else { "FAIL" }), (Get-FileHash $src -Algorithm SHA256).Hash)
  if (-not $ok) { throw "roundtrip FAILED: $f" }
  Write-Host $rows[-1]
}

$startUtc = (Get-Date).ToUniversalTime().ToString("o")
$sampler = Start-Job -ScriptBlock {
  $deadline = (Get-Date).AddSeconds(900)
  while ((Get-Date) -lt $deadline) {
    try { $c = (Get-CimInstance Win32_Processor | Measure-Object -Property LoadPercentage -Average).Average } catch { $c = -1 }
    Write-Output ("{0:o},{1}" -f (Get-Date).ToUniversalTime(), $c)
    Start-Sleep -Milliseconds 700
  }
}
$log = "results\${Label}_stdout.txt"
& ".\bwt_prof$sfx.exe" --reps=$Reps --label=$Label --out=results @($Files | ForEach-Object { "containers\bwt_$_.anv" }) 2>&1 | Tee-Object -FilePath $log
$rc = $LASTEXITCODE
Start-Sleep -Milliseconds 800
Stop-Job $sampler -ErrorAction SilentlyContinue
$samples = Receive-Job $sampler -ErrorAction SilentlyContinue
Remove-Job $sampler -Force -ErrorAction SilentlyContinue
$endUtc = (Get-Date).ToUniversalTime().ToString("o")

$loads = @(); foreach ($s in $samples) { $p = ($s -split ','); if ($p.Count -eq 2 -and $p[1] -ne "") { $loads += [double]$p[1] } }
$med = $null; $mx = $null; $mn = $null
if ($loads.Count -gt 0) { $sorted = $loads | Sort-Object; $med = $sorted[[int][math]::Floor($sorted.Count / 2)]; $mx = ($loads | Measure-Object -Maximum).Maximum; $mn = ($loads | Measure-Object -Minimum).Minimum }
$samples | Set-Content "attest\${Label}_cpuload.csv"
$attest = @()
$attest += "PR-4 v1 attestation -- decode-perf I9 / BWT stage split"
$attest += "label: $Label"
$attest += "window_start_utc: $startUtc"
$attest += "window_end_utc: $endUtc"
$attest += "host: $env:COMPUTERNAME"
$attest += ("profiler: prototypes/i9-decode-perf/bwt_prof$sfx.exe sha256=" + (Get-FileHash ".\bwt_prof$sfx.exe" -Algorithm SHA256).Hash)
$attest += ("snap_anvil$sfx.exe sha256=" + (Get-FileHash ".\snap_anvil$sfx.exe" -Algorithm SHA256).Hash)
$attest += (Get-Content $provFile)
$attest += ("command: .\bwt_prof$sfx.exe --reps={0} --label={1} --out=results <containers>" -f $Reps, $Label)
$attest += ("containers: " + ($rows -join " | "))
$attest += ("cpu_sampler: Get-CimInstance Win32_Processor -> LoadPercentage avg, 0.7 Hz loop; samples=$($loads.Count)")
$attest += ("cpu_load_pct_median={0} max={1} min={2}" -f $med, $mx, $mn)
$attest += "pinning/priority: SetProcessAffinityMask last logical processor, HIGH_PRIORITY_CLASS"
$attest += "threads: 1 (snapshot HEAD has no parallel decode); libsais_unbwt called single-thread (freq=nullptr)"
$attest += "bwt_prof_exit_code: $rc"
$attest | Set-Content "attest\$Label.txt"
$attest | ForEach-Object { Write-Host $_ }
