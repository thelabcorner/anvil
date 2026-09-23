# 100 ms-class host-load sampler for the pnra attested parallel window (PR-4 v1.1)
# Streams each sample to cpu-samples.csv; runs until stop-sampler exists.
Remove-Item cpu-samples.csv -ErrorAction SilentlyContinue
'utc,cpu_total_pct' | Set-Content -Path cpu-samples.csv -Encoding ascii
while (-not (Test-Path stop-sampler)) {
    $v = $null
    try { $v = (Get-CimInstance Win32_PerfFormattedData_PerfOS_Processor -Filter "Name='_Total'").PercentProcessorTime } catch { $v = -1 }
    Add-Content -Path cpu-samples.csv -Value ('{0},{1}' -f (Get-Date).ToUniversalTime().ToString('yyyy-MM-ddTHH:mm:ss.fffZ'), $v) -Encoding ascii
    Start-Sleep -Milliseconds 100
}
