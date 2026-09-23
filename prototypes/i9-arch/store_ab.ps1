# arch I9 store-path 3-way microbench runner (bytewise vs slicing-by-8 vs PCLMUL).
# Interleaved, reps>=5, pinned core, per-rep enc+dec ns/B, median + CV, load field.
param(
  [string]$Log = 'prototypes\i9-arch\logs\store_ab_w-arch-storepath.log',
  [int]$Reps = 5,
  [int]$Core = 18
)
$ErrorActionPreference='Stop'
New-Item -ItemType Directory -Force -Path (Split-Path $Log) | Out-Null
$arms = [ordered]@{ bytewise='prototypes\i9-arch\st_bytewise.exe'; slice8='prototypes\i9-arch\st_slice8.exe'; pclmul='prototypes\i9-arch\st_pclmul.exe' }
$files = @('tests\corpus\random.bin','tests\corpus\synth-arith.bin')
"# store_ab $(Get-Date -Format o)" | Set-Content -LiteralPath $Log
foreach($k in $arms.Keys){ "# arm $k sha=$((Get-FileHash -LiteralPath $arms[$k] -Algorithm SHA256).Hash)" | Add-Content -LiteralPath $Log }
foreach($f in $files){ "# file $f sha=$((Get-FileHash -LiteralPath $f -Algorithm SHA256).Hash)" | Add-Content -LiteralPath $Log }

function Run-One([string]$exe,[string]$file,[int]$core){
  $psi=New-Object System.Diagnostics.ProcessStartInfo
  $psi.FileName=(Resolve-Path -LiteralPath $exe).Path
  $psi.Arguments='"'+ (Resolve-Path -LiteralPath $file).Path +'" 1 mdl'
  $psi.RedirectStandardOutput=$true; $psi.UseShellExecute=$false; $psi.CreateNoWindow=$true
  $p=[System.Diagnostics.Process]::Start($psi)
  try { $p.ProcessorAffinity=[IntPtr]([int64]1 -shl $core) } catch {}
  $o=$p.StandardOutput.ReadToEnd(); $p.WaitForExit(); return ,$o
}
$keys=@($arms.Keys)
$results=@()
for($r=0;$r -lt $Reps;$r++){
  $order = if($r % 2 -eq 0){ $keys } else { @($keys[2],$keys[1],$keys[0]) }
  foreach($f in $files){
    foreach($arm in $order){
      $load=$null; try { $load=(Get-CimInstance Win32_Processor | Measure-Object -Property LoadPercentage -Average).Average } catch {}
      $out=Run-One $arms[$arm] $f $Core
      if($out -notmatch 'encode_ns_per_inB=([0-9.]+).*decode_ns_per_outB=([0-9.]+)'){ throw "parse fail $arm $f" }
      $en=[double]$Matches[1]; $de=[double]$Matches[2]
      $fnv=[regex]::Match($out,'wire_fnv=([0-9A-F]+)').Groups[1].Value
      $results+=[pscustomobject]@{file=[IO.Path]::GetFileName($f);arm=$arm;rep=$r;enc=$en;dec=$de;fnv=$fnv;load=$load}
      "REP file=$([IO.Path]::GetFileName($f)) arm=$arm rep=$r enc_ns_per_inB=$en dec_ns_per_outB=$de wire_fnv=$fnv load=$load" | Add-Content -LiteralPath $Log
    }
  }
}
"# SUMMARY" | Add-Content -LiteralPath $Log
foreach($g in ($results | Group-Object file,arm)){
  $e=@($g.Group|ForEach-Object{$_.enc}|Sort-Object); $d=@($g.Group|ForEach-Object{$_.dec}|Sort-Object)
  $me=$e[[int][math]::Floor($e.Count/2)]; $md=$d[[int][math]::Floor($d.Count/2)]
  $eme=($e|Measure-Object -Average).Average; $dme=($d|Measure-Object -Average).Average
  $ecv=[math]::Sqrt((($e|ForEach-Object{($_-$eme)*($_-$eme)})|Measure-Object -Average).Average)/$eme*100
  $dcv=[math]::Sqrt((($d|ForEach-Object{($_-$dme)*($_-$dme)})|Measure-Object -Average).Average)/$dme*100
  $line="ARM={0} file={1} n={2} enc_median_ns_per_inB={3} enc_CV={4}% dec_median_ns_per_outB={5} dec_CV={6}%" -f $g.Group[0].arm,$g.Group[0].file,$g.Count,[math]::Round($me,4),[math]::Round($ecv,2),[math]::Round($md,4),[math]::Round($dcv,2)
  $line | Add-Content -LiteralPath $Log; Write-Output $line
}
"# done $(Get-Date -Format o)" | Add-Content -LiteralPath $Log
