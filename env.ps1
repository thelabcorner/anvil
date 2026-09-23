# Load the ANVIL build environment for this PowerShell session.
# Usage:  . .\env.ps1     (dot-source so PATH/vars persist in the session)
# This adds CMake, Ninja and the LLVM toolchain to PATH, then pulls the
# MSVC + Windows SDK environment out of vcvars64.bat so clang-cl/CMake can
# find the CRT and SDK libs. No admin needed.

$env:Path = "C:\Program Files\LLVM\bin;C:\Program Files\CMake\bin;$env:LOCALAPPDATA\Microsoft\WinGet\Links;" + $env:Path

$vcvars = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
if (Test-Path -LiteralPath $vcvars) {
    $lines = cmd /c "`"$vcvars`" >nul 2>&1 && set"
    foreach ($line in $lines) {
        if ($line -match "^([^=]+)=(.*)$") {
            try { [System.Environment]::SetEnvironmentVariable($matches[1], $matches[2], "Process") } catch {}
        }
    }
} else {
    Write-Warning "vcvars64.bat not found; MSVC/SDK environment NOT loaded"
}

Write-Host "cmake:  $(cmake --version 2>$null | Select-Object -First 1)"
Write-Host "ninja:  $(ninja --version 2>$null)"
Write-Host "clang:  $((clang++ --version 2>$null | Select-Object -First 1))"
Write-Host "cl:     $((cl 2>&1 | Select-Object -First 1))"
