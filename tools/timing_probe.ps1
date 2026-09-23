#!/usr/bin/env pwsh
# Median-3 timing probe for benchmark throughput claims (audit 01 §4: median >=3).
$ErrorActionPreference = "Stop"
$repo = "C:\Users\<user>\Documents\ANVIL"
Push-Location $repo
$anvil = "build\anvil.exe"

function Measure-Median($label, $cmd) {
    $ts = @()
    for ($i=1; $i -le 3; $i++) {
        $t = Measure-Command { & $anvil @cmd | Out-Null }
        $ts += [math]::Round($t.TotalSeconds, 3)
        Write-Host "$label rep$i = $($t.TotalSeconds)s"
    }
    $sorted = $ts | Sort-Object
    $median = $sorted[1]
    Write-Host "$label MEDIAN = ${median}s  (reps: $($ts -join ', '))"
}

# xml: Brotli-routed (decode-cheap control)
Measure-Median "xml auto enc"  @("c","scratch\ratio-first\corpora\silesia\xml","scratch\auto-confirm\xml_t.anv","--parse=ratio","--ratio-backend=auto","--ratio-context=off","--ratio-lines=off","--quiet")
Measure-Median "xml dec"       @("d","scratch\auto-confirm\xml.anv","scratch\auto-confirm\xml_t.out","--quiet")

# dickens: BWT-routed (decode-heavy)
Measure-Median "dickens auto enc" @("c","scratch\ratio-first\corpora\silesia\dickens","scratch\auto-confirm\dickens_t.anv","--parse=ratio","--ratio-backend=auto","--ratio-context=off","--ratio-lines=off","--quiet")
Measure-Median "dickens dec"      @("d","scratch\auto-confirm\dickens.anv","scratch\auto-confirm\dickens_t.out","--quiet")

# enwik8 BWT decode (E7 decode characterization)
Measure-Median "enwik8 bwt dec" @("d","scratch\auto-confirm\enwik8.anv","scratch\auto-confirm\enwik8_t.out","--quiet")
Pop-Location
