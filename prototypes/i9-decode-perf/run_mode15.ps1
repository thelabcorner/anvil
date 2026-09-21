# run_mode15.ps1 -- decode-perf I9 attested profiling run (PR-1).
# Builds containers with snap_anvil.exe (snapshot of src/anvil.cpp), verifies
# byte-identical round-trips, then runs prof_i9.exe (median-7 interleaved) under
# a 1 Hz host-load sampler. Attestation fields per bench's PR-4 v1:
# tool+binary, input sha256+bytes, window UTC, reps+raw, CV, host load, pinning,
# label. Raw per-rep values are in results/<label>_results_*.txt (median + CV);
# the raw rep vectors are re-computable by re-running the exact command below.
param(
  [int]$Reps = 7,
  [string]$Label = "i9_mode15",
  [switch]$SkipEncode,
  [switch]$Wt
)
$ErrorActionPreference = "Stop"
Set-Location $PSScriptRoot
New-Item -ItemType Directory -Force -Path containers, results, attest | Out-Null
$sfx = if ($Wt) { "_wt" } else { "" }
$provFile = if ($Wt) { "snapshot_provenance_wt.txt" } else { "snapshot_provenance.txt" }

if (-not (Test-Path ".\snap_anvil$sfx.exe") -or -not (Test-Path ".\prof_i9$sfx.exe")) { throw "run build_i9.ps1 / build_wt.ps1 first" }

# record the exact binary hashes used
$binaryHashes = @()
foreach ($b in @(".\prof_i9$sfx.exe", ".\snap_anvil$sfx.exe")) {
  $binaryHashes += ("{0} sha256={1} bytes={2}" -f $b, (Get-FileHash $b -Algorithm SHA256).Hash, (Get-Item $b).Length)
}

$jobs = @(
  @{ name = "synth_timeseries_hotop";        src = "..\..\tests\corpus\synth-timeseries.bin"; args = @("--parse=hotop", "--literal=o0", "--entropy=rans", "--shape-states=28") },
  @{ name = "synth_timeseries_hotop_budget"; src = "..\..\tests\corpus\synth-timeseries.bin"; args = @("--parse=hotop", "--literal=o0", "--entropy=rans", "--shape-states=28", "--hotop-budget=on") },
  @{ name = "synth_timeseries_hotop_rlzp";   src = "..\..\tests\corpus\synth-timeseries.bin"; args = @("--parse=hotop", "--literal=o0", "--entropy=rans", "--shape-states=28", "--hotop-rlzp=on") },
  @{ name = "generated_json_hotop";          src = "..\..\tests\corpus\generated.json";       args = @("--parse=hotop", "--literal=o0", "--entropy=rans", "--shape-states=28") },
  @{ name = "generated_json_hotop_budget";   src = "..\..\tests\corpus\generated.json";       args = @("--parse=hotop", "--literal=o0", "--entropy=rans", "--shape-states=28", "--hotop-budget=on") },
  @{ name = "generated_json_hotop_rlzp";     src = "..\..\tests\corpus\generated.json";       args = @("--parse=hotop", "--literal=o0", "--entropy=rans", "--shape-states=28", "--hotop-rlzp=on") },
  @{ name = "generated_json_mdl";            src = "..\..\tests\corpus\generated.json";       args = @("--parse=mdl", "--literal=o0", "--entropy=rans") }
)

$rows = @()
foreach ($j in $jobs) {
  $out = "containers\$($j.name).anv"
  $dec = "containers\$($j.name).dec"
  if (-not ($SkipEncode -and (Test-Path $out))) {
    & ".\snap_anvil$sfx.exe" c $j.src $out @($j.args) --quiet
    if ($LASTEXITCODE -ne 0) { throw "encode failed: $($j.name)" }
  }
  & ".\snap_anvil$sfx.exe" d $out $dec --quiet
  if ($LASTEXITCODE -ne 0) { throw "decode failed: $($j.name)" }
  $hIn = (Get-FileHash $j.src -Algorithm SHA256).Hash
  $hOut = (Get-FileHash $dec -Algorithm SHA256).Hash
  $ok = ($hIn -eq $hOut)
  $rows += ("{0} comp_bytes={1} roundtrip={2} src_sha256={3}" -f $j.name, (Get-Item $out).Length, $(if ($ok) { "OK" } else { "FAIL" }), $hIn)
  if (-not $ok) { throw "roundtrip FAILED: $($j.name)" }
  Write-Host $rows[-1]
}

# ---- attested run ----
$files = $jobs | ForEach-Object { "containers\$($_.name).anv" }
$expect = ""
# prof verifies one expect source for all files; run per-file to keep verification strong
$startUtc = (Get-Date).ToUniversalTime().ToString("o")
$procs = (Get-Process | Sort-Object CPU -Descending | Select-Object -First 10 Name, Id, CPU | Format-Table -AutoSize | Out-String)

$sampler = Start-Job -ScriptBlock {
  $deadline = (Get-Date).AddSeconds(600)
  while ((Get-Date) -lt $deadline) {
    try {
      $c = (Get-CimInstance Win32_Processor | Measure-Object -Property LoadPercentage -Average).Average
    } catch { $c = -1 }
    Write-Output ("{0:o},{1}" -f (Get-Date).ToUniversalTime(), $c)
    Start-Sleep -Milliseconds 700
  }
}

$log = "results\${Label}_stdout.txt"
& ".\prof_i9$sfx.exe" --reps=$Reps --label=$Label --out=results @files 2>&1 | Tee-Object -FilePath $log
$profRc = $LASTEXITCODE

Start-Sleep -Milliseconds 800
Stop-Job $sampler -ErrorAction SilentlyContinue
$samples = Receive-Job $sampler -ErrorAction SilentlyContinue
Remove-Job $sampler -Force -ErrorAction SilentlyContinue
$endUtc = (Get-Date).ToUniversalTime().ToString("o")

$loads = @()
foreach ($s in $samples) { $p = ($s -split ','); if ($p.Count -eq 2 -and $p[1] -ne "") { $loads += [double]$p[1] } }
$med = $null; $mx = $null; $mn = $null
if ($loads.Count -gt 0) {
  $sorted = $loads | Sort-Object
  $med = $sorted[[int][math]::Floor($sorted.Count / 2)]
  $mx = ($loads | Measure-Object -Maximum).Maximum
  $mn = ($loads | Measure-Object -Minimum).Minimum
}
$samples | Set-Content "attest\${Label}_cpuload.csv"

$attest = @()
$attest += "PR-4 v1 attestation -- decode-perf I9 / PR-1"
$attest += "label: $Label"
$attest += "window_start_utc: $startUtc"
$attest += "window_end_utc: $endUtc"
$attest += "host: $env:COMPUTERNAME"
$attest += "profilers: prototypes/i9-decode-perf/prof_i9$sfx.exe (snapshot-instrumented decoder, decode_threads=1)"
$attest += $binaryHashes
$attest += (Get-Content $provFile)
$attest += ("command: .\prof_i9$sfx.exe --reps={0} --label={1} --out=results <containers...>" -f $Reps, $Label)
$attest += ("containers: " + ($rows -join " | "))
$attest += ("cpu_sampler: Get-CimInstance Win32_Processor -> LoadPercentage avg, 1 Hz loop; samples=$($loads.Count)")
$attest += ("cpu_load_pct_median={0} max={1} min={2}" -f $med, $mx, $mn)
$attest += "pinning/priority: SetProcessAffinityMask last logical processor, HIGH_PRIORITY_CLASS (printed by prof_i9)"
$attest += "prof_exit_code: $profRc"
$attest += "top_processes_at_start:"
$attest += $procs
$attest | Set-Content "attest\$Label.txt"
Write-Host "attestation -> attest\$Label.txt"
$attest | ForEach-Object { Write-Host $_ }
