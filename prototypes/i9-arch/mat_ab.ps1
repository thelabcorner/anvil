# arch I9 MAT A/B runner -- interleaved paired timing of two anvil_bench binaries.
# Protocol (contract v1.2): same pinned core, alternating arm order per rep,
# threads=1 (anvil rows are single-thread), per-rep rows + median + CV, host-load
# sampler at rep granularity, wire bytes reported (byte-identity anchor).
#
# Usage:
#   pwsh -File prototypes\i9-arch\mat_ab.ps1 -ArmA <exeA> -ArmB <exeB> `
#        -Pairs 'file.anv:codec,file2:codec2' -Reps 7 -Core 18 -Log out.log
# NOTE: pair target = (input file, anvil row name); e.g.
#   "tests\corpus\generated.json:anvil-mdl-rans"
param(
  [Parameter(Mandatory=$true)][string]$ArmA,
  [Parameter(Mandatory=$true)][string]$ArmB,
  [Parameter(Mandatory=$true)][string[]]$Pairs,
  [int]$Reps = 7,
  [int]$Core = 18,
  [string]$Log = 'prototypes\i9-arch\logs\mat_ab.log'
)
$ErrorActionPreference='Stop'
# Normalize: accept a single comma-joined element or separate elements.
$Pairs = @($Pairs | ForEach-Object { $_ -split '[,;]' } | Where-Object { $_ -ne '' })
New-Item -ItemType Directory -Force -Path (Split-Path $Log) | Out-Null
$arms = @{ A=$ArmA; B=$ArmB }
$shaA=(Get-FileHash -LiteralPath $ArmA -Algorithm SHA256).Hash
$shaB=(Get-FileHash -LiteralPath $ArmB -Algorithm SHA256).Hash
"# mat_ab $(Get-Date -Format o)" | Set-Content -LiteralPath $Log
"# armA=$ArmA sha=$shaA" | Add-Content -LiteralPath $Log
"# armB=$ArmB sha=$shaB  core=$Core reps=$Reps" | Add-Content -LiteralPath $Log

function Run-One([string]$exe,[string]$file,[int]$core) {
  $psi = New-Object System.Diagnostics.ProcessStartInfo
  $psi.FileName = (Resolve-Path -LiteralPath $exe).Path
  $psi.Arguments = '"' + (Resolve-Path -LiteralPath $file).Path + '" 1'
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
foreach ($pair in $Pairs) {
  $parts = $pair -split ':',2
  $file = $parts[0]; $codec = $parts[1]
  for ($r=0; $r -lt $Reps; $r++) {
    $order = if ($r % 2 -eq 0) { @('A','B') } else { @('B','A') }
    foreach ($armTag in $order) {
      $exe = $arms[$armTag]
      $load = $null
      try { $load = (Get-CimInstance Win32_Processor | Measure-Object -Property LoadPercentage -Average).Average } catch {}
      $out = Run-One $exe $file $Core
      $line = $out -split "`r?`n" | Where-Object { $_ -match [regex]::Escape($codec) } | Select-Object -First 1
      if (-not $line) { throw "row '$codec' not found for $file (arm $armTag)" }
      $c = $line -split ','
      $bytes = [int64]$c[1]; $ratio=[double]$c[2]; $enc=[double]$c[3]; $dec=[double]$c[4]; $rt=$c[5]
      $inBytes = (Get-Item -LiteralPath $file).Length
      $ms = if ($dec -gt 0) { ($inBytes/1e6)/$dec*1e3 } else { -1 }
      $rec = [pscustomobject]@{file=[IO.Path]::GetFileName($file);codec=$codec;rep=$r;arm=$armTag;dec_MBps=$dec;enc_MBps=$enc;ms=$ms;bytes=$bytes;ratio=$ratio;rt=$rt;load=$load}
      $results += $rec
      "REP file=$($rec.file) codec=$codec arm=$armTag rep=$r dec_MBps=$dec ms=$([math]::Round($ms,3)) bytes=$bytes rt=$rt load=$load" | Add-Content -LiteralPath $Log
    }
  }
}
"# SUMMARY" | Add-Content -LiteralPath $Log
foreach ($g in ($results | Group-Object file,codec,arm)) {
  $v = $g.Group | ForEach-Object { $_.ms }
  $sorted = $v | Sort-Object
  $med = $sorted[[int][math]::Floor($sorted.Count/2)]
  $mean = ($v | Measure-Object -Average).Average
  $sd = [math]::Sqrt((($v | ForEach-Object { ($_-$mean)*($_-$mean) }) | Measure-Object -Average).Average)
  $cv = if ($mean -gt 0) { 100*$sd/$mean } else { 0 }
  $bytes = ($g.Group | Select-Object -First 1).bytes
  $decMed = ($g.Group | Measure-Object -Property dec_MBps -Average).Average
  $line = "ARM={0} file={1} codec={2} n={3} median_ms={4} CV={5}% bytes={6}" -f $g.Name,$g.Group[0].file,$g.Group[0].codec,$g.Count,[math]::Round($med,4),[math]::Round($cv,2),$bytes
  $line | Add-Content -LiteralPath $Log
  Write-Output $line
}
"# done $(Get-Date -Format o)" | Add-Content -LiteralPath $Log
