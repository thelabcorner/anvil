# ANVIL benchmark corpus

24 data files, ~21.1 MB total input (21,127,883 B). This is a deterministic **smoke/development
corpus**, not a standard corpus (no Silesia/Canterbury/enwik yet). Ratios on
this corpus are directional; throughput is host-specific (see
`tests/host-spec.md`).

| file | bytes | type | provenance |
|---|---:|---|---|
| `doc.md` | 2,724 | markdown | hand-written project doc (committed) |
| `src.cpp` | 32,512 | C++ source | the codec source, snapshot (committed) |
| `generated.json` | 827,664 | structured JSON | `make_smoke_corpus.py`, deterministic |
| `generated.log` | 1,942,280 | log lines | `make_smoke_corpus.py`, deterministic |
| `generated.sqlite` | 1,740,800 | SQLite page file | `make_smoke_corpus.py`, deterministic |
| `random.bin` | 262,144 | pseudo-random | `make_smoke_corpus.py`, deterministic (seed 12345) |
| `generated.jsonl` | 2,815,267 | JSON-lines, record-structured (SPARSE-REF stress) | `make_smoke_corpus.py`, deterministic |
| `generated.repeat.jsonl` | 940,000 | JSON-lines, identical records (no-regression control) | `make_smoke_corpus.py`, deterministic |
| `anvil.exe` | 268,800 | PE executable (x64) — TCOPY binary-lane target | snapshot of `build/anvil.exe`, 2026-08-13 |
| `anvil_bench.exe` | 1,929,216 | PE executable (x64) — TCOPY binary-lane target | pinned bytes on disk 2026-08-21 (see *Manifest drift note*) |
| `synth-timeseries.bin` | 280,000 | fixed-stride binary records (14 B: u64 ts + f32 value + u16 id) | `make_synth_corpus.py`, deterministic (seed 90001) |
| `synth-arith.bin` | 256,000 | 8 columnar u32 arithmetic progressions + small noise | `make_synth_corpus.py`, deterministic (seed 90002) |
| `synth-jitter.bin` | 974,920 | 64 B near-duplicate records at a jittered (0-2 B pad) stride | `make_synth_corpus.py`, deterministic (seed 90003) |
| `pe-winver.exe` | 28,672 | PE executable (x64) — S6-2 held-out verdict set | Windows system binary, see below |
| `pe-where.exe` | 61,440 | PE executable (x64) — S6-2 held-out verdict set | Windows system binary, see below |
| `pe-notepad.exe` | 360,448 | PE executable (x64) — S6-2 held-out verdict set | Windows system binary, see below |
| `pe-python.exe` | 103,704 | PE executable (x64) — S6-2 held-out verdict set | CPython 3.12.4 official build, see below |
| `pe-ninja.exe` | 603,648 | PE executable (x64) — S6-2 held-out verdict set | Ninja 1.13.2 release, see below |
| `pe-git.exe` | 4,383,048 | PE executable (x64) — S6-2 held-out verdict set | Git for Windows 2.55.0 (MinGW/GCC), see below |
| `synth-columnar-align.bin` | 276,000 | row-interleaved columnar AP table, all fields misaligned (23 B prime rows) | `make_synth_corpus.py`, deterministic (seed 90004) |
| `synth-drift-stride.bin` | 615,376 | 40 B structured records at a systematically drifting (40-48 B sawtooth) stride | `make_synth_corpus.py`, deterministic (seed 90005) |
| `synth-counters.log` | 1,107,326 | counter/timestamp-heavy text log (several simultaneous monotonic sequences) | `make_synth_corpus.py`, deterministic (seed 90006) |
| `synth-telemetry-f64.bin` | 320,000 | smooth f64 sensor telemetry, exact 1000 ms cadence (Gorilla sweet spot) | `make_synth_corpus.py`, deterministic (seed 90007) |
| `synth-ndjson-columnar.ndjson` | 995,894 | mostly-stable-column NDJSON, variable line length (vXOR trigger) | `make_synth_corpus.py`, deterministic (seed 90008) |

`generated.jsonl` is the record-structured stress file requested in
`docs/research-agenda.md` §5: ~235 B records, mostly-identical skeleton, a few
bytes changing per record at consistent positions (id/ts/kind/value/checksum/
session), changed content never repeated, and one structural-drift record
(extra field) every 200 records so correction masks shift. `generated.repeat.
jsonl` is the pure-repetition control from agenda §1.4: SPARSE-REF must equal
exact-LZ there (no regression).

`anvil.exe` / `anvil_bench.exe` are **snapshots** of the codec's own binaries
(TCOPY's target domain — real PE executables with relocation fields). They are
copied from `build/` and pinned here so suite rows stay stable; re-snapshot
deliberately (and update `CHECKSUMS.txt`) when the binaries change, since a
rebuild of `src/anvil.cpp` changes them. Do NOT re-derive them from `build/`
on every run.

`synth-timeseries.bin` / `synth-arith.bin` / `synth-jitter.bin` are the
Iteration-5-prerequisite synthetic structural corpus (RESEARCH_LEDGER.md
PART VII): purpose-built files with genuine tight periodic (timeseries),
arithmetic-progression (arith), and periodic-with-jitter (jitter) structure,
added because Experiments T and U both found near-zero periodic/arithmetic
signal in the `generated.*` files and needed a fair, controlled retest
before concluding the structural-regularity mechanisms (SRR probe,
finite-difference invariant) don't work at all.

## Held-out PE set (S6-2 anti-overfit verdict set, added 2026-08-21)

The six `pe-*.exe` files are REAL x64 PE executables, **distinct from the two
pinned PEs** (`anvil.exe` / `anvil_bench.exe`). Per the pre-registered S6-2
contract (`docs/swarm-i6-strategy.md` §5): calibration/tuning happens ONLY on
the pinned pair; acceptance-threshold verdicts are measured ONLY on this
held-out set. Diversity was chosen across producer, toolchain, and size
(28 KB – 4.3 MB; MSVC + MinGW/GCC; OS binaries + three third-party projects).
Each is pinned by exact bytes below; all are valid MZ/PE, machine=x64.

| file | source (exact origin) | version | SHA-256 |
|---|---|---|---|
| `pe-winver.exe` | `%SystemRoot%\System32\winver.exe` ("Version Reporter Applet", Windows 22H2 system binary, MSVC) | 10.0.22621.1 | `d1d050efbae74c970ba6e666de004405b21f60f34ff0886000026763fe117cf0` |
| `pe-where.exe` | `%SystemRoot%\System32\where.exe` ("Where - Lists location of files", Windows 22H2 system binary, MSVC) | 10.0.22621.1 | `ade557dd65848c5cf6565913cf6e01cf5c9a8033f0d784c4d6932394958d743e` |
| `pe-notepad.exe` | `%SystemRoot%\System32\notepad.exe` (Windows 22H2 system binary, MSVC) | 10.0.22621.1 | `49f096cbf9337b0a80bde835d29be41bc9371057c4ff6c72f8a36158c29cfa3a` |
| `pe-python.exe` | `C:\Program Files\Python312\python.exe` (official python.org CPython build, MSC v.1940 x64) | 3.12.4 (tags/v3.12.4) | `fd5c46d73d29ba21b04c844bbaf9096066136526911230645a2a040d23fb612b` |
| `pe-ninja.exe` | winget package `Ninja-build.Ninja` (ninja-build project release binary, MSVC) | 1.13.2 (`ninja --version`) | `e52a7ad9538d9618c67a0bd777964e2eec8a30f68b810a2f6adce1f2daf847b8` |
| `pe-git.exe` | `C:\Program Files\Git\mingw64\bin\git.exe` (Git for Windows, **MinGW-w64/GCC** toolchain — the only non-MSVC-built PE in the corpus) | 2.55.0.windows.3 | `1a0043555d254618f2d56c936c3d9a1fbfb878bc878416a133c346bc7835eda9` |

Provenance honesty: these are host-installed binaries pinned by exact bytes
(the task's "system tools with documented versions" rule). They are NOT
bit-for-bit reproducible from source — but the corpus pins the bytes via
SHA-256, so the verdict set is stable regardless of future host updates.
Total added: 5,540,960 B.

## Structure-carrying additions (Iteration-7, added 2026-08-21)

Experiments T/U/W found the whole prior corpus's periodic/arithmetic signal
at the hash-noise floor. The three new `synth-*` files extend
`make_synth_corpus.py` (fixed seeds 90004–90006, integer-only math, no float
formatting → byte-stable across Python versions/platforms) with structure
types the first three did not cover:

- `synth-columnar-align.bin` (seed 90004, 12,000 × 23 B): a columnar
  arithmetic table stored ROW-interleaved with every field misaligned (prime
  23 B row stride). Fields: u16 row_id (Δ=1), u32 seq (Δ=7), u64 ts_ns
  (Δ=1e6), u32 temp_x100 (Δ=3±1), u16 volt (Δ=2), u8 status (mod 8), u8
  flags (rare flip), u8 parity = XOR of preceding field bytes (cross-field
  linear structure). Unlike `synth-arith.bin` (block-wise columns — the easy
  case), a mechanism must find the TRUE field offsets to see the long
  near-constant-delta runs; wrong offsets yield nothing.
- `synth-drift-stride.bin` (seed 90005, 14,000 records, ~44 B mean stride):
  fixed-width 40 B structured records (u64 ts Δ=500, u32 counter, u16
  channel mod 64, 24 B slow-ticking skeleton, u16 CRC16) preceded by a pad
  whose length follows a SYSTEMATIC sawtooth ramp `(i//96) % 9`, so the
  record period drifts 40→48 B and snaps back every 864 records. This is
  the deterministic-drift counterpart to `synth-jitter.bin`'s random 0–2 B
  jitter: any fixed-period assumption breaks while content stays highly
  structured.
- `synth-counters.log` (seed 90006, 11,000 lines, ~110 B/line): the TEXT
  analogue — several simultaneous monotonic sequences per line: ISO-8601
  timestamp ticking +37 ms, `seq` +1, `tick` +1000, hex `addr` page-stepping
  +0x40 within each of 8 modules, bounded integer random-walk `lat`, cyclic
  `qd`, deterministic `crc`. Strong arithmetic/periodic signal in text form,
  which no prior corpus file had.

## Gorilla/vXOR sweet-spot substrate (post-I7 research briefing, added 2026-08-21)

Added per the coordinator's GROTLI × GORILLA × ANVIL briefing so the proposed
auto-detect mechanisms have a fair substrate: without sweet-spot files the
Gorilla route correctly stays NEUTRAL (G_s guard) and the win is unmeasurable.
These files were generated and their specs FROZEN before any Gorilla/vXOR
mechanism code exists in ANVIL — they are measurement substrate, not a
mechanism claim. Routing statistics below were measured with a faithful
Python port of grotli.ts `shredBuffer` (512 B windows, strides [8,4,2],
thresholds H<7.5 / S>0.60 / G_s>0.55 / R>0.15) + `detectRecordStride`
(delimiter CV<0.20):

| file | routed B (Gorilla) | H | S | G_s | R(stride) | delim CV → stride |
|---|---:|---:|---:|---:|---:|---|
| `synth-telemetry-f64.bin` | **100.0%** of windows | 5.79 | 0.909 | 0.783 | 0.213 | n/a (binary) |
| `synth-ndjson-columnar.ndjson` | 0% (ASCII guard holds ✓) | 4.27 | 0.582 | 0.358 | — | **0.0111 → vXOR** |
| `random.bin` (control) | 0% — correctly neutral | 7.59 | 0.457 | 0.016 | 0.017 | 0.93 no |

- `synth-telemetry-f64.bin` (seed 90007, 20,000 × 16 B): { u64 ts_ms, f64
  value } at EXACT 1000 ms cadence — timestamp delta-of-delta is `'0'` for
  **100.0000%** of records (Gorilla's 1-bit tier); values drift a smooth
  0.001°/sample with ±0.0001 seeded perturbation (Case A/B1 territory).
  Float math restricted to IEEE-754 basic ops for cross-platform byte
  stability. Note recorded honestly: grotli's stride candidates are [8,4,2],
  so routing locks onto stride 8 (the value sub-stride); the true record
  stride is 16 — mechanism lanes should treat that as a known test case.
- `synth-ndjson-columnar.ndjson` (seed 90008, 11,000 lines): natural NDJSON,
  stable columns (sensor/unit constant; status flips every ~1000 lines),
  exact-cadence ts, one smoothly-drifting numeric column. Column-equal
  density under modal-length-aligned record pairs: **96.3%** (above grotli's
  93.2% zero-density honest-win zone). Line lengths deliberately VARIABLE
  (86–91 B, modal 89 × 54.5%) so vXOR frame detection is tested honestly.

Orientation through current `build/anvil.exe` defaults (round-trip OK):
telemetry-f64 **0.888** — generic LZ barely touches smooth f64, quantifying
exactly the gap a Gorilla lane would attack; ndjson-columnar **0.0222** —
record-structured text is already ANVIL's home turf.

## Orientation ratios (2026-08-21)

Round-trip orientation only (NOT a benchmark row): each new file compressed
with `build/anvil.exe c` default settings (codec sha256 d7b02b2fa76c5451…,
built 2026-08-21 04:44 — newer than the pinned snapshot; arch lane rebuilds),
decompressed, byte-compared OK:

| file | raw B | anvil default B | ratio |
|---|---:|---:|---:|
| pe-winver.exe | 28,672 | 5,587 | 0.1949 |
| pe-where.exe | 61,440 | 22,362 | 0.3640 |
| pe-python.exe | 103,704 | 54,220 | 0.5228 |
| pe-ninja.exe | 603,648 | 293,296 | 0.4859 |
| pe-notepad.exe | 360,448 | 203,424 | 0.5644 |
| pe-git.exe | 4,383,048 | 2,131,578 | 0.4863 |
| synth-columnar-align.bin | 276,000 | 118,089 | 0.4279 |
| synth-drift-stride.bin | 615,376 | 143,950 | 0.2339 |
| synth-counters.log | 1,107,326 | 127,048 | 0.1147 |
| synth-telemetry-f64.bin | 320,000 | 284,152 | 0.8880 |
| synth-ndjson-columnar.ndjson | 995,894 | 22,158 | 0.0222 |

The PE ratios span 0.19–0.56 across toolchains/producers — a real spread for
the S6-2 verdict aggregate, unlike the two near-identical pinned anchors.

### Manifest drift note (anvil_bench.exe, discovered 2026-08-21)

Before this expansion began, the on-disk `anvil_bench.exe` (1,929,216 B,
sha256 `fcd30da5f2745df439aaedcb3d84ec71451e0974a5c0f84da814a73dd9ef4ce0`)
already mismatched the manifest entry carried since 2026-08-13 (1,866,752 B,
sha256 `e5a0f8a7c415bed17058320a8a54b7623d61aa03e6d8407a8af04fd81c9aa72c`):
the corpus copy had been re-snapshotted from a rebuilt `build/` at some point
without updating `CHECKSUMS.txt`. The file is git-ignored (`.gitignore`
matches `anvil_bench.exe`), so the old-hash bytes are unrecoverable from
history. This expansion did NOT modify the file; it re-pinned the manifest
line to the on-disk bytes so integrity checks pass truthfully. **RESOLVED
(pnra-cost, 2026-08-21, empirical):** S6-2 calibration AND Experiment X were
both measured on the CURRENT on-disk bytes (`fcd30da5…`) — an instrumented
EXP. X clone reproduces the ledger chain exactly on bench (gate 6919 / idxhit
2145 / verify 1958 / commit 1599, out=846,050), so the manifest drift
predates Experiment X and calibration/ledger numbers are on identical data.
Recorded in `docs/s62-threshold-formula.md` §1. No re-calibration needed;
the held-out verdict set above is unaffected.

Independent confirmation (verify, 2026-08-21): the same mismatch was found in
a separate checksum audit before this note existed. Additionally, recent
experiment claims (EXP. V token economics, EXP. X tcopy 845,675→846,050)
reproduce BYTE-EXACT against the current on-disk bytes from a fresh
build-verify build — so only measurements made against the old 1,866,752-byte
snapshot (pre-2026-08-13) are potentially non-comparable. Recorded as a
provenance caveat, not a defect.

## Regeneration (fully deterministic)

```powershell
python tests\make_smoke_corpus.py tests\corpus
python tests\make_synth_corpus.py tests\corpus
```

All six `generated.*` / `random.bin` files are byte-identical to a fresh run
(verified 2026-08-12; see `CHECKSUMS.txt`). `doc.md` and `src.cpp` are
hand-written and are *not* regenerated by the script. All EIGHT `synth-*`
files are byte-identical to a fresh run of `make_synth_corpus.py`
(fixed seeds; re-verified 2026-08-21 after both extensions — all prior files
regenerate unchanged). The two Gorilla/vXOR files additionally rely only on
IEEE-754 basic float ops + correctly-rounded formatting (no libm
transcendentals), double-run hash-verified. The `pe-*.exe` held-out set and
the two pinned PEs are **not** regenerable: they are pinned host/third-party
binaries; integrity is defined by `CHECKSUMS.txt`, not by a build.

## Integrity

`CHECKSUMS.txt` lists SHA-256 + size for every corpus file. Verify with:

```powershell
Get-ChildItem tests\corpus -File | Where-Object { $_.Name -notin 'CHECKSUMS.txt','README.md' } | ForEach-Object {
  $h = (Get-FileHash $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
  $line = Select-String -Path tests\corpus\CHECKSUMS.txt -Pattern ([regex]::Escape($_.Name)) | Select-Object -First 1
  if ($line -and $h -eq $line.Line.Split(' ')[0]) { "OK  $($_.Name)" } else { "BAD $($_.Name)" }
}
```

## History note

The earlier `tests/benchmark-suite.csv` referenced `tests/corpus/anvil-task.md`
and `tests/corpus/anvil.cpp`, which are **not** in this repo. Their numbers are
preserved in `tests/benchmark-taskdoc.csv` (task document, 26,904 B) and the
`src.cpp` content (renamed from `anvil.cpp`). Do not regenerate suite rows for
files that do not exist; the current suite covers exactly the 6 files above.
