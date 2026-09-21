#!/usr/bin/env pwsh
$ErrorActionPreference = "Continue"
$b = "build\anvil.exe"
$f = "scratch\ratio-first\corpora\silesia\samba"
$out = "scratch\auto-confirm\samba.262144.bwt.anv"
Write-Host "=== samba 262144 BWT encode ==="
& $b c $f $out --parse=ratio --ratio-backend=bwt --ratio-context=off --ratio-lines=off --block=262144 --quiet
Write-Host "EXIT: $LASTEXITCODE"
Write-Host "=== samba 65536 BWT encode (probe smaller) ==="
& $b c $f "scratch\auto-confirm\samba.65536.bwt.anv" --parse=ratio --ratio-backend=bwt --ratio-context=off --ratio-lines=off --block=65536 --quiet
Write-Host "EXIT: $LASTEXITCODE"
Write-Host "=== samba 131072 BWT encode ==="
& $b c $f "scratch\auto-confirm\samba.131072.bwt.anv" --parse=ratio --ratio-backend=bwt --ratio-context=off --ratio-lines=off --block=131072 --quiet
Write-Host "EXIT: $LASTEXITCODE"
