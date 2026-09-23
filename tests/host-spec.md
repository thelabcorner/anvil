# ANVIL host specification

Captured 2026-08-12T23:06:56Z (UTC) by tools\host_spec.ps1. Every benchmark run in this repo should be read against this file.

## OS

- Name: Microsoft Windows 11 Pro
- Version: 10.0.22631 (build 22631)
- Architecture: 64-bit

## CPU

- Model: AMD Ryzen 9 5900X 12-Core Processor            
- Sockets/Cores/Threads: AM4 / 12 cores / 24 logical
- Base clock: 4200 MHz

## Memory

- Total: 127.9 GB

## Toolchain (measured on this host)

- clang-cl / clang++: clang version 22.1.8 (https://github.com/llvm/llvm-project ca7933e47d3a3451d81e72ac174dcb5aa28b59d1)
- MSVC cl: Microsoft (R) C/C++ Optimizing Compiler Version 19.44.35213 for x64
- cmake: cmake version 4.4.2
- ninja: 1.13.2
- python: Python 3.12.4
- PowerShell: 7.6.4 (Core)

## Third-party reference codecs (third_party/)

- Brotli: git snapshot, commit 8e10eeb Add params to brotli to manually enable or disable the SIMD hashers. - NOT pinned to a release tag; re-cloning via tools\setup_third_party.ps1 may move it.
- Zstd: v1.5.7 (pinned; tools\setup_third_party.ps1 checks out tag v1.5.7).
- Built: static libs under third_party\install via tools\setup_third_party.ps1 (clang-cl, Release).
- NOTE: third_party/ is git-ignored; re-run tools\setup_third_party.ps1 on a fresh clone.

## Reproducible build (from scratch)

```powershell
. .\env.ps1
Remove-Item -Recurse -Force build -ErrorAction SilentlyContinue
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=clang-cl -DCMAKE_SUPPRESS_REGENERATION=ON
ninja -C build
```

Build log: tests\build-baseline.log (first from-scratch run of the v0.1 baseline).
Codec flags (from CMakeLists.txt): MSVC /O2 /EHsc; else -O3 -DNDEBUG -Wall -Wextra -Wpedantic. Bench harness links static brotli/zstd.

## Reproducible benchmark

```powershell
python tools\bench_suite.py tests\corpus\doc.md tests\corpus\src.cpp tests\corpus\random.bin tests\corpus\generated.json tests\corpus\generated.repeat.jsonl tests\corpus\generated.sqlite tests\corpus\generated.log tests\corpus\generated.jsonl --bench build\anvil_bench.exe --reps 3 --out tests\benchmark-suite.csv
```

- Codecs: anvil greedy/dp x arith/rans, brotli q1/4/6/9/11, zstd 1/3/9/19 (fixed set in tools\bench_native.cpp).
- Speed = median of 3 reps (median_speed in tools\bench_native.cpp); ratio = compressed/input bytes; every row round-trip verified.
- Outputs: tests\benchmark-suite.csv (per file) + tests\benchmark-summary.csv (aggregate).
