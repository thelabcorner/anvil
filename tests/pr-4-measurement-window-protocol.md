# PR-4 — Measurement-window attestation protocol (bench lane)

Status: BINDING for the whole `anvil-i9-pareto` swarm. Owner: `bench`.
Version: v1.2 (2026-09-07). Supersedes any earlier informal practice.
v1.2 amendment: quiet gate is now at CORE granularity (see section 3a); the total-CPU <10%
threshold proved unattainable on this host even with no known jobs (ambient agent load
keeps total at ~21-58%), so total CPU is context, not the gate.

This is the profile of record. Any throughput number that will be compared,
ranked, or cited (including any `EXTENDS_FRONT` / FRONT-CROSSING argument) MUST
carry the fields below or it is not arbiter-usable. Byte counts and ratios are
deterministic and are always safe (see §6); throughput is not.

Grounding: `tests/noise-floor.csv` measures CV 25–42% under load (AUDIT-6).
A throughput delta smaller than the same-cell CV is not measurable on this host.

## 1. Required fields (every published throughput number)

1. **Tool + binary.** Exact command line; SHA-256 of the executable that ran;
   build directory (`build\`, `build-i9-bench\`, `build-i9-<lane>\`, ...).
2. **Input.** Path; SHA-256; byte count.
3. **Window.** `window_start_utc` / `window_end_utc`, ISO-8601, second precision.
4. **Reps.** Raw per-rep values for each reported metric. `median` over reps;
   **reps >= 3** for any citation-grade number. Report `reps` explicitly.
5. **Dispersion.** `cv_pct = stddev/mean*100` per metric.
   Gate: if `cv_pct` exceeds the `tests/noise-floor.csv` CV for the same
   `(file, codec, metric)`, the number needs more reps or a quieter window
   before it may be cited. If the cell is absent from noise-floor.csv, use the
   42% worst-case ceiling as the provisional gate and say so.
6. **Host load.** 1 Hz non-idle CPU% sampler over the window; report **median
   and max**. State the sampler command. Name any concurrent measurement job
   and its core set (or state "none").
7. **Pinning / priority.** CPU affinity mask and process priority actually set
   (e.g. `Start-Process -Priority High`, `SetProcessAffinityMask`). State the
   logical cores used.
8. **Thread counts** (v1.1, coordinator ruling). For every row:
   `enc_threads`, `dec_threads` (ANVIL) and `ref_threads` per reference.
   Missing thread counts are treated as **1** only if the binary is known
   single-threaded; otherwise the row is unattested.
9. **Label.** `measured` | `derived` | `projection`. Derived/projection numbers
   never support a crossing claim.

## 2. Thread-count disclosure rule (binding, coordinator ruling)

> Every decode-throughput number must carry its thread count. Any
> multi-threaded ANVIL row compared against single-threaded brotli/zstd/xz must
> either be measured at the same core count, or disclose the thread imbalance
> **next to the row** (`THREAD-IMBALANCE: anvil <N>t vs ref 1t`). A row with an
> undisclosed imbalance is not arbiter-usable.

Operational form for `tests/benchmark-suite.csv` (until the harness emits a
threads column): each suite run is accompanied by an attestation row per
`(file, codec, window_id)` in `tests/thread-attestation.csv`
(`window_id,input_file,codec,plane,enc_threads,dec_threads,ref_threads,attested_by,note`).
All current `tests/benchmark-suite.csv` rows and all brotli/zstd rows in the
in-process harness (`tools/bench_native.cpp`) are single-threaded; the dirty
9/7 parallel-decode plumbing is **not** in any published row (see §5).

## 3. Serialization (measurement isolation)

- One timing job at a time on the host for citation-grade windows. The
  `bench` lane owns the window; announce start/stop to the swarm.
- Profiling passes of ~1–2 min pinned to disjoint logical cores MAY run in
  parallel **only** if §1 fields 1–9 are recorded and no two timing jobs share
  a physical core. Their numbers are `measured (parallel)` and may inform
  ranking, never a crossing claim.
- Final/citation runs (any `EXTENDS_FRONT` support, any headline number) are
  quiet-window only, with `none` for concurrent jobs.
- Byte-only work (ratio, compressed bytes, oracle arithmetic, arbiter over an
  existing CSV) is exempt — no timing, no window needed.

## 3a. Quiet gate, core granularity (v1.2 — measured necessity)

Measured 2026-09-12 06:38-06:44Z: total CPU stayed 21-58% median over 6+ minutes
even when no swarm measurement/build job was alive — ambient load (agents, python)
means the old total-CPU <10% gate can never pass. Substitute gate, binding:

1. **Known-job gate:** zero `anvil*` / `ninja` / `clang*` / `cmake` processes, and no
   other swarm lane's timing job announced. (Byte-identity checks do not count.)
2. **Pinned-core gate:** identify the affinity mask the benchmark will use; sample
   `Get-Counter '\Processor Information(*)\% Processor Time'` for 30 s and require
   each pinned logical core's mean < 5%. Total CPU is reported as context only.
3. **During-run sampler:** for runs >= 10 s, 1 Hz total CPU + own-process share;
   for micro-runs (<= 5 s), sampler at <= 250 ms, report ambient pre-window baseline
   (median over 30 s before start), during-window median/max, and own-process share;
   external = total - own - ambient.
4. **Announce:** window owner marks start/stop; if a known job starts mid-window,
   the run is `measured (contaminated; label)` and must be repeated.

## 4. Arbiter verification procedure (crossing claims)

`tools/pareto_front.py` emits `EXTENDS_FRONT`, the sole necessary condition.
Before any lane may cite a crossing:

1. Re-run the arbiter on the exact suite CSV twice, in **two separate process
   invocations**, and require identical output (`sha256sum` of
   `tests/pareto-baseline.csv` equal). If they differ, the input is not pinned.
2. Run the FRONT-GAP bracket test and the DEGENERATE guard
   (`docs/gate-ruling-i8-pareto-win.md` R-2; `tools/pareto_verify.py`).
3. Only a non-dominated row that is NOT FRONT-GAP, NOT DEGENERATE, and whose
   window passes §1 may be called a FRONT-CROSSING. `bench` signs off.
4. The mandated co-listed tuple is always quoted whole, with its grid:
   - committed HEAD grid: `5 non-dominated | 5 FRONT-GAP | 0 FRONT-CROSSING |
     411/416 dominated` (`443/448` including AGGREGATE);
   - uncommitted worktree grid C70179EA (used only if labelled): `5 | 5 | 0 |
     463/468` (`499/504`).
   Never quote one figure alone, never swap the grids without the label.

## 5. Binary provenance (2026-09-07 blocker)

`build\anvil.exe` / `build\anvil_bench.exe` dated 2026-09-07 19:35 (dirty
worktree, uncommitted `decode_one_block` refactor) **encode but do not decode**
on any file (`format`, confirmed by coordinator). Until `arch` lands the fix and
publishes its SHA-256:

- no roundtrip / sha256-dependent row from that binary may be published or
  re-verified;
- the canonical Silesia `46,446,995 B` / enwik8 `23,534,368 B` figures remain
  as-recorded on binary `de9b4caf` and must be quoted with that label;
- fresh reference-row reconnaissance and fresh arbiter benchmark runs are held.

The P0 arbiter recompute over the **committed CSV artifact** is unaffected
(pure CSV transform; no binary invoked). See `tests/pareto-recompute-i9.md`.
Fresh reference-row remediation waits for the fixed binary SHA-256 from `arch`.

## 6. Deterministic vs timed

| quantity | deterministic? | window required? |
|---|---|---|
| `compressed_bytes`, `ratio`, oracle sums | yes | no |
| `roundtrip` OK/FAIL | yes | no (but needs a green binary) |
| `encode_MBps`, `decode_MBps` | no | yes (full §1) |
| arbiter status over an existing CSV | yes | no |

## 3b. Acceptance ladder for decode claims (v1.2, binding)

- **RANKING-GRADE** (citable as directional/approximate only; label
  `measured (parallel; attestation attached)`): fields 1-9 present except an
  affinity mask; effect >= 5x the larger arm CV; external load
  (total - own - ambient) <= 20 pp over ambient. Cannot support a bar, limit,
  or crossing arithmetic.
- **CITATION-GRADE** (may support bars/limits/crossing arithmetic): all fields;
  same-window paired arms; affinity set AND pinned-core gate passed (pinned core
  mean < 5% pre-window); reps >= 3 (>= 7 for a headline); effect clears the
  requirement by >= max(2%, CV_target, CV_ref); external load <= 5 pp over
  ambient; arms interleaved or replicated so load drift cannot favour one arm.
- Anything below citation-grade may select the next experiment, never declare a
  result.

## 3c. Window exclusivity (v1.2)

Builds, gate runs, and fuzz sweeps are timing jobs. Inside an announced window
(owner posts start/stop), no peer may start a compile, gate test, or fuzz run.
At w-bwtinv-20260912T064334Z the host had arch's gate tests live (anvil_head_ref
/ arch-gate), i.e. the window was not swarm-quiet; such windows are
ranking-grade at best.

## 3d. Wire-charge co-listing — BWT-inverse aux index (v1.2, corrected)

Any BWT-aux ratio/net claim MUST co-list the index charge at the proposed
single-thread policy (r = smallest power of two with `r*1024 >= n`). Exact
per-file values (bwtinv recompute, coordinator-corrected 2026-09-12):

| file | charged B |
|---|---:|
| dickens | 2,492 |
| mr | 2,436 |
| nci | 4,096 |
| osdb | 2,464 |
| reymont | 3,236 |
| webster | 2,532 |
| x-ray | 2,072 |
| **Silesia 7-file total** | **19,328** (0.00912% of 211,938,580; ~0.96% of the 2,009,105-B xz margin) |
| enwik8 | 3,052 |

The earlier "~40KB / 2-5KB per block" figure was a worst-case 2048-entry bound;
measured files use 518-1024 entries. Do not cite 40KB. Spot-verification of the
policy computation requested from research-gate (msg pending).

## 4a. Reference-class v2 (I9-6) — tuple co-listing (2026-09-12)

The arbiter reference class is now v2 = brotli + zstd + `xz-9e` (GRID-THIN: zstd
4-22 / brotli lw30 unmeasured). Same-transform controls (`xz --delta`, `xz --x86`)
stay side-channel (`tests/xz-transform-controls-i9.csv`) and are required with any
crossing claim (gate A13). Co-listed tuples (all double-run hash-identical):

- HEAD fc23d9a, v1 brotli/zstd: `5 | 5 | 0 | 411/416` (443/448)
- HEAD fc23d9a, v2 +xz-9e:     `4 | 4 | 0 | 412/416` (444/448)
- worktree C70179EA, v1:       `5 | 5 | 0 | 463/468` (499/504)
- worktree C70179EA, v2 +xz:   `1 | 1 | 0 | 467/468` (503/504)

Cite v1 only with the "brotli+zstd reference class (grid v1)" label. Details:
`tests/pareto-reference-class-i9.md`.
