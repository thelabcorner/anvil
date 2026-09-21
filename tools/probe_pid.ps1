#!/usr/bin/env pwsh
$p = Get-Process -Id 59288 -ErrorAction SilentlyContinue
if ($null -ne $p) {
    $p | Select-Object Id,Name,StartTime,Responding,Path | Format-List
    Write-Host "--- modules referencing anvil ---"
    $p.Modules | Where-Object { $_.ModuleName -like "*anvil*" } | Select-Object FileName
} else {
    Write-Host "59288 gone"
}
# also list any remaining anvil processes
$all = Get-Process -Name anvil -ErrorAction SilentlyContinue
Write-Host "Remaining anvil processes: $($all.Count)"
foreach ($a in $all) { Write-Host "  PID $($a.Id) start $($a.StartTime)" }
