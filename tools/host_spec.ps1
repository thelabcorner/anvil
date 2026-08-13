# Capture the ANVIL host specification for benchmark reproducibility.
# Usage:  pwsh -File tools\host_spec.ps1   (run from repo root; load env.ps1 first)
# Writes tests\host-spec.md. Re-run after any toolchain/host change so the
# benchmark CSVs stay interpretable.
param([string]$Out = "tests\host-spec.md")

$ErrorActionPreference = "Stop"
# Bootstrap toolchain PATH (mirrors env.ps1) so the script works standalone.
$env:Path = "C:\Program Files\LLVM\bin;C:\Program Files\CMake\bin;$env:LOCALAPPDATA\Microsoft\WinGet\Links;" + $env:Path
# Load the MSVC/SDK environment so `cl` version is capturable.
$vcvars = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
if (Test-Path -LiteralPath $vcvars) {
    cmd /c "`"$vcvars`" >nul 2>&1 && set" | ForEach-Object {
        if ($_ -match "^([^=]+)=(.*)$") { try { [System.Environment]::SetEnvironmentVariable($matches[1], $matches[2], "Process") } catch {} }
    }
}

$L = New-Object System.Collections.Generic.List[string]
$L.Add('# ANVIL host specification')
$L.Add('')
$L.Add("Captured $(Get-Date -Format yyyy-MM-dd'T'HH:mm:ss'Z') (UTC) by tools\host_spec.ps1. Every benchmark run in this repo should be read against this file.")

# --- OS ---
$os = Get-CimInstance Win32_OperatingSystem
$L.Add('')
$L.Add('## OS')
$L.Add('')
$L.Add("- Name: $($os.Caption)")
$L.Add("- Version: $($os.Version) (build $($os.BuildNumber))")
$L.Add("- Architecture: $($os.OSArchitecture)")

# --- CPU ---
$cpu = Get-CimInstance Win32_Processor | Select-Object -First 1
$L.Add('')
$L.Add('## CPU')
$L.Add('')
$L.Add("- Model: $($cpu.Name)")
$L.Add("- Sockets/Cores/Threads: $($cpu.SocketDesignation) / $($cpu.NumberOfCores) cores / $($cpu.NumberOfLogicalProcessors) logical")
$L.Add("- Base clock: $($cpu.MaxClockSpeed) MHz")

# --- Memory ---
$L.Add('')
$L.Add('## Memory')
$L.Add('')
$L.Add("- Total: $([math]::Round($os.TotalVisibleMemorySize/1MB,2)) GB")

# --- Toolchain ---
$L.Add('')
$L.Add('## Toolchain (measured on this host)')
$L.Add('')
$clVer = (cl 2>&1 | Where-Object { $_ -match 'Version' } | Select-Object -First 1)
if (-not $clVer) { $clVer = "n/a (vcvars64.bat not loaded / cl not found)" }
$L.Add("- clang-cl / clang++: $((clang++ --version 2>$null | Select-Object -First 1))")
$L.Add("- MSVC cl: $clVer")
$L.Add("- cmake: $((cmake --version 2>$null | Select-Object -First 1))")
$L.Add("- ninja: $(ninja --version 2>$null)")
$L.Add("- python: $(python --version 2>&1)")
$L.Add("- PowerShell: $($PSVersionTable.PSVersion) ($($PSVersionTable.PSEdition))")

# --- Third-party reference codecs ---
$brotliRev = (git -C third_party\brotli log --oneline -1 2>$null)
$zstdRev = (git -C third_party\zstd describe --tags 2>$null)
$L.Add('')
$L.Add('## Third-party reference codecs (third_party/)')
$L.Add('')
$L.Add("- Brotli: git snapshot, commit $brotliRev - NOT pinned to a release tag; re-cloning via tools\setup_third_party.ps1 may move it.")
$L.Add("- Zstd: $zstdRev (pinned; tools\setup_third_party.ps1 checks out tag v1.5.7).")
$L.Add('- Built: static libs under third_party\install via tools\setup_third_party.ps1 (clang-cl, Release).')
$L.Add('- NOTE: third_party/ is git-ignored; re-run tools\setup_third_party.ps1 on a fresh clone.')

# --- Build recipe ---
$L.Add('')
$L.Add('## Reproducible build (from scratch)')
$L.Add('')
$L.Add('```powershell')
$L.Add('. .\env.ps1')
$L.Add('Remove-Item -Recurse -Force build -ErrorAction SilentlyContinue')
$L.Add('cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=clang-cl -DCMAKE_SUPPRESS_REGENERATION=ON')
$L.Add('ninja -C build')
$L.Add('```')
$L.Add('')
$L.Add('Build log: tests\build-baseline.log (first from-scratch run of the v0.1 baseline).')
$L.Add('Codec flags (from CMakeLists.txt): MSVC /O2 /EHsc; else -O3 -DNDEBUG -Wall -Wextra -Wpedantic. Bench harness links static brotli/zstd.')

# --- Benchmark recipe ---
$L.Add('')
$L.Add('## Reproducible benchmark')
$L.Add('')
$L.Add('```powershell')
$L.Add('python tools\bench_suite.py tests\corpus\doc.md tests\corpus\src.cpp tests\corpus\random.bin tests\corpus\generated.json tests\corpus\generated.repeat.jsonl tests\corpus\generated.sqlite tests\corpus\generated.log tests\corpus\generated.jsonl --bench build\anvil_bench.exe --reps 3 --out tests\benchmark-suite.csv')
$L.Add('```')
$L.Add('')
$L.Add('- Codecs: anvil greedy/dp x arith/rans, brotli q1/4/6/9/11, zstd 1/3/9/19 (fixed set in tools\bench_native.cpp).')
$L.Add('- Speed = median of 3 reps (median_speed in tools\bench_native.cpp); ratio = compressed/input bytes; every row round-trip verified.')
$L.Add('- Outputs: tests\benchmark-suite.csv (per file) + tests\benchmark-summary.csv (aggregate).')

$L | Set-Content -Path $Out -Encoding utf8
Write-Host "wrote $Out"
