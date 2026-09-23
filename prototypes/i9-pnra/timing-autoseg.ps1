# timing-autoseg.ps1 — pnra autoseg ranking-grade timing (PR-4 v1.2 fields).
# Run ONLY when the bench window queue grants slot 3; announces w-pnra-autoseg-<utc>.
# Ambient 30 s baseline, streaming during-window sampler, per-arm own-process CPU share,
# pinned to the least-loaded logical core, priority High.
$lane = 'C:\Users\<user>\Documents\ANVIL\prototypes\i9-pnra'
Set-Location $lane
$log = New-Object System.Collections.Generic.List[string]

# 0) pick least-loaded logical core
$loads = foreach ($i in 0..23) {
    $l = 100; try { $l = (Get-CimInstance Win32_PerfFormattedData_PerfOS_Processor -Filter "Name='$i'").PercentProcessorTime } catch {}
    [pscustomobject]@{ core = $i; load = $l }
}
$idle = ($loads | Sort-Object load | Select-Object -First 1).core
$log.Add("idle_core=$idle per_core_loads=$($loads.load -join ',')")

# 1) ambient baseline (30 s)
$bl = New-Object System.Collections.Generic.List[int]
$tb = (Get-Date)
while (((Get-Date) - $tb).TotalSeconds -lt 30) {
    try { $v = (Get-CimInstance Win32_PerfFormattedData_PerfOS_Processor -Filter "Name='_Total'").PercentProcessorTime } catch { $v = -1 }
    $bl.Add([int]$v); Start-Sleep -Milliseconds 150
}
$blSorted = $bl | Sort-Object
$log.Add("ambient_30s n=$($bl.Count) median=$($blSorted[[int]($bl.Count/2)]) max=$($blSorted[-1])")

# 2) start during-window sampler
Remove-Item stop-sampler-autoseg -ErrorAction SilentlyContinue
$sampler = Start-Process pwsh -ArgumentList '-NoProfile','-File','sampler-autoseg.ps1' -PassThru -WindowStyle Hidden
$deadline = (Get-Date).AddSeconds(15)
while ((Test-Path cpu-samples-autoseg.csv) -eq $false -or (Get-Content cpu-samples-autoseg.csv | Measure-Object -Line).Lines -lt 3) {
    if ((Get-Date) -gt $deadline) { break }
    Start-Sleep -Milliseconds 100
}

# 3) arms
$arms = @(
    @{ name = 'synth-arith-auto';   file = '..\..\tests\corpus\synth-arith.bin';       reps = 801 },
    @{ name = 'synth-drift-auto';   file = '..\..\tests\corpus\synth-drift-stride.bin'; reps = 501 },
    @{ name = 'generated-log-auto'; file = '..\..\tests\corpus\generated.log';          reps = 201 }
)
$exeHash = (Get-FileHash autoseg.exe -Algorithm SHA256).Hash.ToLower()
$log.Add("exe=autoseg.exe sha256=$exeHash")
$winStart = (Get-Date).ToUniversalTime().ToString('yyyy-MM-ddTHH:mm:ss.fffZ')
foreach ($a in $arms) {
    $t0 = (Get-Date).ToUniversalTime()
    $so = "asr-$($a.name).out"; $se = "asr-$($a.name).err"
    $p = Start-Process -FilePath '.\autoseg.exe' -ArgumentList @('bench', $a.file, '--backend=0', '--param=1', '--table=compact', "--reps=$($a.reps)") -RedirectStandardOutput $so -RedirectStandardError $se -PassThru -WindowStyle Hidden
    try { $p.ProcessorAffinity = [IntPtr](1 -shl $idle) } catch { $log.Add("affinity_set_failed arm=$($a.name)") }
    try { $p.PriorityClass = 'High' } catch { }
    $p.WaitForExit()
    $cpu = 0.0; try { $cpu = $p.TotalProcessorTime.TotalSeconds } catch {}
    $t1 = (Get-Date).ToUniversalTime()
    $wall = ($t1 - $t0).TotalSeconds
    $log.Add("arm=$($a.name) start=$($t0.ToString('yyyy-MM-ddTHH:mm:ss.fffZ')) end=$($t1.ToString('yyyy-MM-ddTHH:mm:ss.fffZ')) wall_s=$([math]::Round($wall,3)) own_cpu_s=$([math]::Round($cpu,3)) own_share_pct=$([math]::Round(100*$cpu/[math]::Max($wall,1e-9),1))")
}
$winEnd = (Get-Date).ToUniversalTime().ToString('yyyy-MM-ddTHH:mm:ss.fffZ')
$log.Add("window_start_utc=$winStart window_end_utc=$winEnd")

# 4) stop sampler and aggregate
New-Item -ItemType File stop-sampler-autoseg | Out-Null
Wait-Process -Id $sampler.Id -Timeout 60
$samples = Get-Content cpu-samples-autoseg.csv | Select-Object -Skip 1 | ForEach-Object { $p = $_ -split ','; if ($p.Count -ge 2 -and $p[1] -ne '-1') { [pscustomobject]@{ t = $p[0]; cpu = [int]$p[1] } } }
$sv = $samples.cpu | Sort-Object
$log.Add("during_window n=$($sv.Count) median=$($sv[[int]($sv.Count/2)]) max=$($sv[-1])")
$log | Set-Content autoseg-timing-summary.txt -Encoding ascii
$log | ForEach-Object { Write-Output $_ }
foreach ($a in $arms) { Write-Output "--- $($a.name) ---"; Get-Content "asr-$($a.name).out" | Select-String 'SRESULT|SDETAIL' | ForEach-Object { $_.Line } }
