#!/usr/bin/env pwsh
$ErrorActionPreference = "Stop"
$repo = "C:\Users\<user>\Documents\ANVIL"
Push-Location $repo
$anvil = "build\anvil.exe"

function Peak-RSS($label, $cmd) {
    $proc = Start-Process -FilePath $anvil -ArgumentList $cmd -PassThru -RedirectStandardOutput "scratch/auto-confirm/pk_out.txt" -RedirectStandardError "scratch/auto-confirm/pk_err.txt" -WindowStyle Hidden
    $peak = 0
    try {
        while (-not $proc.HasExited) {
            try {
                $ws = $proc.WorkingSet64
                if ($ws -gt $peak) { $peak = $ws }
            } catch {}
            Start-Sleep -Milliseconds 20
        }
        # final read
        try { $ws = $proc.WorkingSet64; if ($ws -gt $peak) { $peak = $ws } } catch {}
    } finally {
        if (-not $proc.HasExited) { $proc.Kill() }
    }
    $MiB = [math]::Round($peak / 1MB, 1)
    Write-Host "$label peak RSS = ${MiB} MiB (exit $($proc.ExitCode))"
}

# enwik8: auto (should route BWT) vs explicit bwt
Peak-RSS "enwik8 auto enc " @("c","scratch\ratio-first\corpora\enwik8","scratch\auto-confirm\enwik8_pk.anv","--parse=ratio","--ratio-backend=auto","--ratio-context=off","--ratio-lines=off","--quiet")
Peak-RSS "enwik8 bwt  enc " @("c","scratch\ratio-first\corpora\enwik8","scratch\auto-confirm\enwik8_pk2.anv","--parse=ratio","--ratio-backend=bwt","--ratio-context=off","--ratio-lines=off","--quiet")

# mozilla: auto (should route Brotli)
Peak-RSS "mozilla auto enc" @("c","scratch\ratio-first\corpora\silesia\mozilla","scratch\auto-confirm\mzilla_pk.anv","--parse=ratio","--ratio-backend=auto","--ratio-context=off","--ratio-lines=off","--quiet")

Pop-Location
