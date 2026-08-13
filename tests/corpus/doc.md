# Project ANVIL — v0.1 executable research prototype

ANVIL is a from-scratch experimental general-purpose lossless compressor. This repository is the first implementation pass: it is **not** yet a Brotli replacement and the format is intentionally unstable.

## Implemented now

- independent blocks with raw fallback
- hash-chain LZ match finder
- greedy parser
- estimated-bit dynamic-programming parser
- literal-run + match instruction representation
- separate statistical domains for token type / lengths / distances / literals
- adaptive arithmetic backend
- static separated-stream rANS backend
- order-0, order-1, and confidence-gated literal models for experiments
- brute-force research router (`auto`) that can compare parsers/backends and keep the smallest block
- per-block CRC-32
- strict malformed/truncation checks
- native Brotli/Zstd benchmark harness
- randomized round-trip/truncation fuzz harness

## Build

### Linux / macOS-like C++20 compiler

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

Or directly:

```bash
g++ -std=c++20 -O3 -DNDEBUG src/anvil.cpp -o anvil
```

### Windows / MSVC

```powershell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

The codec itself has no external runtime dependency beyond the C++ standard library. The optional native benchmark target uses Brotli and Zstd development libraries.

## CLI

```bash
# Ratio-oriented current candidate
./anvil c input.bin output.anv --parse=dp --literal=o0 --entropy=arith

# Faster experimental backend
./anvil c input.bin output.anv --parse=greedy --literal=o0 --entropy=rans

# Expensive research search: try multiple parser/model/backend combinations
./anvil c input.bin output.anv --parse=auto --literal=auto --entropy=auto

./anvil d output.anv restored.bin
./anvil verify input.bin --parse=dp --literal=o0 --entropy=rans
```

Useful knobs: `--block=N`, `--chain=N`, and `--max-match=N`.

## Correctness testing

```bash
python3 tests/fuzz.py --exe ./anvil --cases 120
```

For sanitizer testing:

```bash
g++ -std=c++20 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer src/anvil.cpp -o anvil_san
python3 tests/fuzz.py --exe ./anvil_san --cases 50
```

## Native benchmarks

If Brotli and Zstd development libraries are available:

```bash
g++ -std=c++20 -O3 -DNDEBUG tools/bench_native.cpp -o bench_native \
  -lbrotlienc -lbrotlidec -lbrotlicommon -lzstd
./bench_native some-file.bin 5
```

For several files:

```bash
python3 tools/bench_suite.py file1 file2 file3 --bench ./bench_native --reps 3 --out results.csv
```

See `RESEARCH_LEDGER.md` for the first measured results and `FORMAT.md` for the current experimental bitstream.
