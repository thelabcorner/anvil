#!/usr/bin/env pwsh
$procs = Get-Process -Name anvil -ErrorAction SilentlyContinue
foreach ($p in $procs) {
    try { Stop-Process -Id $p.Id -Force -ErrorAction SilentlyContinue } catch {}
}
Start-Sleep -Seconds 2
$remaining = Get-Process -Name anvil -ErrorAction SilentlyContinue
if ($remaining) {
    Write-Host "STILL RUNNING: $($remaining.Id -join ',')"
} else {
    Write-Host "all anvil processes killed"
}
