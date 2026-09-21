# arch I9 leg-4 A/B runner: interleaved paired decode_time on two arm binaries.
# Protocol v1.2: same pinned core, alternating arm order per rep, reps>=7,
# threads=1, per-rep ns_per_outB + median + CV, host-load field (decode_time prints
# a sampled total load per invocation), container unchanged (wire anchor).
param(
  [Parameter(Mandatory=$true)][string]$ArmA,
  [Parameter(Mandatory=$true)][string]$ArmB,
  [Parameter(Mandatory=$true)][string]$Containers,
  [int]$Reps = 7,
  [int]$Core = 18,
  [string]$Log = 'prototypes\i9-arch\logs\dt_ab.log'
)
$ErrorActionPreference='Stop'
$conts = @($Containers -split '[,;]' | Where-Object { $_ -ne '' })
New-Item -ItemType Directory -Force -Path (Split-Path $Log) | Out-Null
$arms = @{ A=$ArmA; B=$ArmB }
"# dt_ab $(Get-Date -Format o)" | Set-Content -LiteralPath $Log
"# armA=$ArmA sha=$((Get-FileHash -LiteralPath $ArmA -Algorithm SHA256).Hash)" | Add-Content -LiteralPath $Log
"# armB=$ArmB sha=$((Get-FileHash -LiteralPath $ArmB -Algorithm SHA256).Hash) core=$Core reps=$Reps" | Add-Content -LiteralPath $Log

function Run-One([string]$exe,[string]$cont,[int]$core) {
  $psi = New-Object System.Diagnostics.ProcessStartInfo
  $psi.FileName = (Resolve-Path -LiteralPath $exe).Path
  $psi.Arguments = '"' + (Resolve-Path -LiteralPath $cont).Path + '" 1'
  $psi.RedirectStandardOutput = $true
  $psi.UseShellExecute = $false
  $psi.CreateNoWindow = $true
  $p = [System.Diagnostics.Process]::Start($psi)
  try { $p.ProcessorAffinity = [IntPtr]([int64]1 -shl $core) } catch {}
  $stdout = $p.StandardOutput.ReadToEnd()
  $p.WaitForExit()
  return ,$stdout
}

$results = @()
foreach ($c in $conts) {
  $contSha = (Get-FileHash -LiteralPath $c -Algorithm SHA256).Hash
  "CONT sha=$contSha file=$c" | Add-Content -LiteralPath $Log
  for ($r=0; $r -lt $Reps; $r++) {
    $order = if ($r % 2 -eq 0) { @('A','B') } else { @('B','A') }
    foreach ($armTag in $order) {
      $out = Run-One $arms[$armTag] $c $Core
      $line = ($out -split "`r?`n" | Where-Object { $_ -match 'decode median' } | Select-Object -First 1)
      if (-not $line) { throw "no decode line from $($arms[$armTag]) on $c" }
      $loadLine = ($out -split "`r?`n" | Where-Object { $_ -match 'host_cpu_load' } | Select-Object -First 1)
      $ns = [double]([regex]::Match($line,'ns_per_outB=([0-9.]+)').Groups[1].Value)
      $mbps = [double]([regex]::Match($line,'([0-9.]+) MB/s').Groups[1].Value)
      $load = if ($loadLine) { [regex]::Match($loadLine,'=([0-9.]+)').Groups[1].Value } else { 'na' }
      $rec = [pscustomobject]@{cont=[IO.Path]::GetFileName($c);rep=$r;arm=$armTag;ns=$ns;mbps=$mbps;load=$load}
      $results += $rec
      "REP file=$($rec.cont) arm=$armTag rep=$r ns_per_outB=$ns MBps=$mbps load=$load" | Add-Content -LiteralPath $Log
    }
  }
}
"# SUMMARY" | Add-Content -LiteralPath $Log
foreach ($g in ($results | Group-Object cont,arm)) {
  $v = @($g.Group | ForEach-Object { $_.ns } | Sort-Object)
  $med = $v[[int][math]::Floor($v.Count/2)]
  $mean = ($v | Measure-Object -Average).Average
  $sd = [math]::Sqrt((($v | ForEach-Object { ($_-$mean)*($_-$mean) }) | Measure-Object -Average).Average)
  $cv = if ($mean -gt 0) { 100*$sd/$mean } else { 0 }
  $line = "ARM={0} file={1} n={2} median_ns_per_outB={3} CV={4}%" -f $g.Group[0].arm,$g.Group[0].cont,$g.Count,[math]::Round($med,4),[math]::Round($cv,2)
  $line | Add-Content -LiteralPath $Log
  Write-Output $line
}
"# done $(Get-Date -Format o)" | Add-Content -LiteralPath $Log
