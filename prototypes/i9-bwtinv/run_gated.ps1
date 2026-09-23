# i9-bwtinv gated measurement window (PR-4 v1.1 aware)
# usage: pwsh -File prototypes/i9-bwtinv/run_gated.ps1 [-Force] [-Reps 7]
# Gate: 30 s pre-sample total CPU median < 10% AND no anvil*/ninja/clang*/cmake* live.
param(
  [switch]$Force,
  [int]$Reps = 7,
  [string]$Tag = ""
)
$ErrorActionPreference = "Stop"
$repo = Resolve-Path "$PSScriptRoot\..\.."
Set-Location $repo
. "$repo\env.ps1" | Out-Null

$files = @(
  "scratch/ratio-first/corpora/silesia/dickens",
  "scratch/ratio-first/corpora/silesia/webster",
  "scratch/ratio-first/corpora/enwik8dir/enwik8"
)

function Sample-Cpu([int]$n, [int]$intervalMs) {
  $vals = @()
  for ($i = 0; $i -lt $n; $i++) {
    try {
      $c = Get-Counter '\Processor(_Total)\% Processor Time' -ErrorAction Stop
      $vals += [double]$c.CounterSamples[0].CookedValue
    } catch { $vals += -1 }
    Start-Sleep -Milliseconds $intervalMs
  }
  return $vals
}
function Median([double[]]$v) {
  if ($v.Count -eq 0) { return -1 }
  $s = $v | Sort-Object
  return $s[[int]($s.Count / 2)]
}

Write-Output "=== i9-bwtinv gated window ==="
$pre = Sample-Cpu 30 1000
$preMed = Median $pre
$preMax = ($pre | Measure-Object -Maximum).Maximum
$busy = @(Get-Process -Name anvil*,ninja,clang*,cmake* -ErrorAction SilentlyContinue)
Write-Output ("pre-window CPU median={0:N1}% max={1:N1}% busy-procs={2}" -f $preMed, $preMax, $busy.Count)
if (-not $Force) {
  if ($busy.Count -gt 0 -or $preMed -ge 10) {
    Write-Output "GATE FAIL (bench rule): need CPU median < 10% and no anvil*/ninja/clang*/cmake*; use -Force for a labelled degraded window."
    exit 3
  }
}

$utcStart = (Get-Date).ToUniversalTime().ToString("yyyy-MM-ddTHH:mm:ssZ")
$windowId = "w-bwtinv-" + (Get-Date).ToUniversalTime().ToString("yyyyMMddTHHmmssZ")
$stamp = $Tag; if ($stamp -eq "") { $stamp = if ($Force) { "degraded" } else { "gated" } }
$csv = "prototypes/i9-bwtinv/measure/$windowId-$stamp.csv"
New-Item -ItemType Directory -Force prototypes/i9-bwtinv/measure | Out-Null

# during-window 1 Hz CPU sampler (background job), plus per-process share of the bench
$sampleOut = "prototypes/i9-bwtinv/measure/$windowId-load.csv"
$job = Start-Job -ScriptBlock {
  param($out, $secs)
  "ts_utc,total_cpu_pct,own_anvil_pct" | Out-File -Encoding ascii $out
  for ($i = 0; $i -lt $secs; $i++) {
    $tot = -1.0; $own = 0.0
    try { $tot = [double](Get-Counter '\Processor(_Total)\% Processor Time' -ErrorAction Stop).CounterSamples[0].CookedValue } catch {}
    try {
      $p = Get-Process -Name unbwt_bench_ref,unbwt_bench_omp -ErrorAction SilentlyContinue
      if ($p) { $own = ($p | Measure-Object -Property CPU -Sum).Sum }
    } catch {}
    $ts = (Get-Date).ToUniversalTime().ToString("o")
    "$ts,$tot,$own" | Out-File -Encoding ascii -Append $out
    Start-Sleep -Seconds 1
  }
} -ArgumentList $sampleOut, 900

$exeRef = (Resolve-Path build-i9-bwtinv/unbwt_bench_ref.exe).Path
$exeOmp = (Resolve-Path build-i9-bwtinv/unbwt_bench_omp.exe).Path
$shaRef = (Get-FileHash $exeRef -Algorithm SHA256).Hash
$shaOmp = (Get-FileHash $exeOmp -Algorithm SHA256).Hash

Write-Output "window_id=$windowId start=$utcStart force=$Force reps=$Reps"
Write-Output "cmd: & '$exeRef' --reps=$Reps --warmup=1 --csv=$csv <files>"
Write-Output "cmd: & '$exeOmp' --reps=$Reps --warmup=1 --csv=$csv <files>"

& $exeRef --reps=$Reps --warmup=1 --csv=$csv @files 2>&1 | Tee-Object -FilePath "prototypes/i9-bwtinv/measure/$windowId-ref-stdout.txt"
& $exeOmp --reps=$Reps --warmup=1 --csv=$csv @files 2>&1 | Tee-Object -FilePath "prototypes/i9-bwtinv/measure/$windowId-omp-stdout.txt"

$utcEnd = (Get-Date).ToUniversalTime().ToString("yyyy-MM-ddTHH:mm:ssZ")
Stop-Job $job -ErrorAction SilentlyContinue | Out-Null
Receive-Job $job -ErrorAction SilentlyContinue | Out-Null
Remove-Job $job -Force -ErrorAction SilentlyContinue | Out-Null

$load = Import-Csv $sampleOut
$duringMed = Median @($load | Where-Object { [double]$_.total_cpu_pct -ge 0 } | ForEach-Object { [double]$_.total_cpu_pct })
$duringMax = (@($load | Where-Object { [double]$_.total_cpu_pct -ge 0 } | ForEach-Object { [double]$_.total_cpu_pct }) | Measure-Object -Maximum).Maximum

$att = "prototypes/i9-bwtinv/measure/$windowId-attestation.md"
@"
# Window $windowId attestation (PR-4 v1.1)

- label: $(if ($Force) { "measured (parallel; attestation attached)" } else { "measured" })
- window_start_utc: $utcStart
- window_end_utc: $utcEnd
- binaries: unbwt_bench_ref.exe sha256 $shaRef (build-i9-bwtinv); unbwt_bench_omp.exe sha256 $shaOmp (build-i9-bwtinv)
- commands: `& build-i9-bwtinv/unbwt_bench_ref.exe --reps=$Reps --warmup=1 --csv=$csv dickens webster enwik8` ; same for _omp
- inputs (sha256 computed below)
- reps: $Reps (raw per-rep values in the CSV reps_ms column and in $windowId-*-stdout.txt)
- host load (1 Hz Get-Counter `\Processor(_Total)\% Processor Time`): pre-window median $preMed %, max $preMax %; during-window median $duringMed %, max $duringMax %; own bench CPU share column in $sampleOut
- pinning/priority: no affinity mask set (harness sets HIGH_PRIORITY_CLASS); make targets use all logical processors
- threads: ref exe rows = 1; omp exe rows = per-variant t in the variant name (t1/t4/t8/t16); libsais internal = explicit num_threads
- concurrent measurement jobs: none started by this lane; see host-load column
"@ | Out-File -Encoding ascii $att

$files | ForEach-Object { Get-FileHash -LiteralPath Get-FileHash @files -Algorithm SHA256 | ForEach-Object { "$($_.Hash)  $($_.Path)" } | Out-File -Encoding ascii -Append $att -Algorithm SHA256 } | ForEach-Object { "$($_.Hash)  $($_.Path)" } | Out-File -Encoding ascii -Append $att
Write-Output "attestation: $att"
Write-Output "csv: $csv"
Write-Output "load: $sampleOut"
