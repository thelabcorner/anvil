# streaming host-load sampler for the pnra autoseg ranking slot (PR-4 v1.2)
Remove-Item cpu-samples-autoseg.csv -ErrorAction SilentlyContinue
'utc,cpu_total_pct' | Set-Content -Path cpu-samples-autoseg.csv -Encoding ascii
while (-not (Test-Path stop-sampler-autoseg)) {
    $v = $null
    try { $v = (Get-CimInstance Win32_PerfFormattedData_PerfOS_Processor -Filter "Name='_Total'").PercentProcessorTime } catch { $v = -1 }
    Add-Content -Path cpu-samples-autoseg.csv -Value ('{0},{1}' -f (Get-Date).ToUniversalTime().ToString('yyyy-MM-ddTHH:mm:ss.fffZ'), $v) -Encoding ascii
    Start-Sleep -Milliseconds 100
}
