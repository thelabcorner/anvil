# run_post.ps1 -- decode-perf I9 leg 3: BWT postcoder profile (IDs 1/2/3) on dickens,
# webster, enwik8. Containers built+roundtrip-verified by the CANONICAL build
# (..\..\build\anvil.exe) so the wire is citation-grade; the instrumented stage
# profiler is a lane build (post_prof_wt.exe, same src, own sha).
# Also times the canonical CLI decode (median-7, Stopwatch) for the ID2 containers.
param(
  [int]$Reps = 7,
  [string]$Label = "i9_post",
  [string[]]$Files = @("dickens", "webster", "enwik8"),
  [string]$Posts = "2,1,3",
  [switch]$SkipEncode,
  [switch]$SkipCli
)
$ErrorActionPreference = "Stop"
Set-Location $PSScriptRoot
New-Item -ItemType Directory -Force -Path containers, results, attest | Out-Null
$Files = @($Files | ForEach-Object { $_ -split ',' } | Where-Object { $_ })
$postList = @($Posts -split ',' | Where-Object { $_ } | ForEach-Object { [int]$_ })

$canonical = "..\..\build\anvil.exe"
if (-not (Test-Path $canonical)) { throw "canonical build\anvil.exe missing" }
$canonHash = (Get-FileHash $canonical -Algorithm SHA256).Hash
if (-not (Test-Path .\post_prof_wt.exe)) { throw "post_prof_wt.exe missing (build_wt.ps1)" }

function SrcPath($f) {
  if ($f -eq "enwik8") { return "..\..\scratch\ratio-first\corpora\enwik8dir\enwik8" }
  return "..\..\scratch\ratio-first\corpora\silesia\$f"
}

# 1. containers via canonical binary (forced postcoders)
$rows = @()
foreach ($f in $Files) {
  $src = SrcPath $f
  $srcHash = (Get-FileHash $src -Algorithm SHA256).Hash
  foreach ($id in $postList) {
    $out = "containers\post_${f}_id$id.anv"
    if (-not ($SkipEncode -and (Test-Path $out))) {
      & $canonical c $src $out --parse=ratio --ratio-backend=bwt --ratio-context=off --ratio-lines=off "--bwt-post=$id" --quiet
      if ($LASTEXITCODE -ne 0) { throw "encode failed: $f id$id" }
    }
    $dec = "containers\post_${f}_id$id.dec"
    & $canonical d $out $dec --quiet
    if ($LASTEXITCODE -ne 0) { throw "decode failed: $f id$id" }
    $ok = ((Get-FileHash $dec -Algorithm SHA256).Hash -eq $srcHash)
    $rows += ("{0} id{1} comp_bytes={2} roundtrip={3}" -f $f, $id, (Get-Item $out).Length, $(if ($ok) { "OK" } else { "FAIL" }))
    if (-not $ok) { throw "roundtrip FAILED: $f id$id" }
    Remove-Item $dec -ErrorAction SilentlyContinue
    Write-Host $rows[-1]
  }
}

# 2. attested instrumented run (sampler records total load + own-process CPU seconds)
$cnt = $Files | ForEach-Object { $f = $_; $postList | ForEach-Object { "containers\post_${f}_id$_.anv" } }
$startUtc = (Get-Date).ToUniversalTime().ToString("o")
$profOut = "results\${Label}_stdout.txt"
$profErr = "results\${Label}_stderr.txt"
$p = Start-Process -FilePath .\post_prof_wt.exe -ArgumentList (@("--reps=$Reps", "--label=$Label", "--out=results") + $cnt) `
     -PassThru -NoNewWindow -RedirectStandardOutput $profOut -RedirectStandardError $profErr
$sampler = Start-Job -ArgumentList $p.Id -ScriptBlock {
  param($profPid)
  $deadline = (Get-Date).AddSeconds(3600)
  while ((Get-Date) -lt $deadline) {
    try { $c = (Get-CimInstance Win32_Processor | Measure-Object -Property LoadPercentage -Average).Average } catch { $c = -1 }
    $own = -1
    try { $pr = Get-Process -Id $profPid -ErrorAction Stop; $own = $pr.CPU } catch { break }
    Write-Output ("{0:o},{1},{2}" -f (Get-Date).ToUniversalTime(), $c, $own)
    Start-Sleep -Milliseconds 700
  }
}
$p.WaitForExit()
$rc = $p.ExitCode
Start-Sleep -Milliseconds 800
Stop-Job $sampler -ErrorAction SilentlyContinue
$samples = Receive-Job $sampler -ErrorAction SilentlyContinue
Remove-Job $sampler -Force -ErrorAction SilentlyContinue
$endUtc = (Get-Date).ToUniversalTime().ToString("o")
$loads = @(); $owns = @()
foreach ($s in $samples) { $q = ($s -split ','); if ($q.Count -ge 2 -and $q[1] -ne "") { $loads += [double]$q[1] }; if ($q.Count -ge 3 -and $q[2] -ne "" -and [double]$q[2] -ge 0) { $owns += [double]$q[2] } }
$med = $null; $mx = $null; $mn = $null
if ($loads.Count -gt 0) { $sorted = $loads | Sort-Object; $med = $sorted[[int][math]::Floor($sorted.Count / 2)]; $mx = ($loads | Measure-Object -Maximum).Maximum; $mn = ($loads | Measure-Object -Minimum).Minimum }
$ownCpu = 0.0; $ownPct = 0.0
if ($owns.Count -ge 2) {
  $ownCpu = $owns[-1] - $owns[0]
  $elapsed = ([datetime]$endUtc - [datetime]$startUtc).TotalSeconds
  if ($elapsed -gt 0) { $ownPct = 100.0 * $ownCpu / $elapsed }
}
$samples | Set-Content "attest\${Label}_cpuload.csv"

# 3. canonical CLI decode timing for ID2 containers (median-7, in-window)
$cli = @()
if (-not $SkipCli) {
  foreach ($f in $Files) {
    $out = "containers\post_${f}_id2.anv"
    if (-not (Test-Path $out)) { continue }
    $times = @()
    $cpu = @()
    for ($r = 0; $r -lt $Reps; $r++) {
      $dec = "containers\cli_${f}.dec"
      $sw = [System.Diagnostics.Stopwatch]::StartNew()
      $cp = Start-Process -FilePath $canonical -ArgumentList @("d", $out, $dec, "--quiet") -PassThru -NoNewWindow -Wait
      $sw.Stop()
      if ($cp.ExitCode -ne 0) { throw "cli decode failed: $f" }
      $times += $sw.Elapsed.TotalSeconds
      $cpu += $cp.TotalProcessorTime.TotalSeconds
      Remove-Item $dec -ErrorAction SilentlyContinue
    }
    $st = $times | Sort-Object
    $medS = $st[[int][math]::Floor($st.Count / 2)]
    $srcMB = (Get-Item (SrcPath $f)).Length / 1e6
    $medCpu = ($cpu | Sort-Object)[[int][math]::Floor($cpu.Count / 2)]
    $cli += ("{0} id2 canonical_cli_median={1:N4}s decode={2:N2} MB/s reps={3} own_cpu_median={4:N3}s (threads=1)" -f $f, $medS, ($srcMB / $medS), $Reps, $medCpu)
    Write-Host $cli[-1]
  }
}
$endUtc2 = (Get-Date).ToUniversalTime().ToString("o")

$attest = @()
$attest += "PR-4 attestation -- decode-perf I9 leg 3 (postcoder)"
$attest += "label: $Label"
$attest += "window_start_utc: $startUtc"
$attest += "window_end_utc: $endUtc (instrumented) / $endUtc2 (incl. canonical CLI)"
$attest += "host: $env:COMPUTERNAME"
$attest += ("canonical binary: build/anvil.exe sha256=$canonHash")
$attest += ("instrumented profiler: post_prof_wt.exe sha256=" + (Get-FileHash .\post_prof_wt.exe -Algorithm SHA256).Hash + " (lane build, same src)")
$attest += (Get-Content .\snapshot_provenance_wt.txt)
$attest += ("command(prof): .\post_prof_wt.exe --reps={0} --label={1} --out=results <containers>" -f $Reps, $Label)
$attest += ("command(containers): build\anvil.exe c <src> <out> --parse=ratio --ratio-backend=bwt --ratio-context=off --ratio-lines=off --bwt-post=<id> --quiet")
$attest += ("containers: " + ($rows -join " | "))
$attest += ("cpu_sampler: Get-CimInstance Win32_Processor -> LoadPercentage avg + own-process CPU seconds, 0.7 Hz loop; samples=$($loads.Count)")
$attest += ("cpu_load_pct_median={0} max={1} min={2}" -f $med, $mx, $mn)
$attest += ("own_process: profiler cpu_seconds={0:N2} own_pct_of_one_core={1:N1}% window={2:N1}s threads=1" -f $ownCpu, $ownPct, (([datetime]$endUtc - [datetime]$startUtc).TotalSeconds))
$attest += "pinning/priority: SetProcessAffinityMask last logical processor, HIGH_PRIORITY_CLASS (instrumented); canonical CLI unpinned (subprocess, disclosed)"
$attest += "threads: 1 (decode_threads=1; libsais_unbwt single-thread)"
$attest += "canonical_cli_decode:"
$attest += $cli
$attest += "prof_exit_code: $rc"
$attest | Set-Content "attest\$Label.txt"
$attest | ForEach-Object { Write-Host $_ }
