# ANVIL I10 — BWT Independent Decode Regions Preregistration

**Status:** FROZEN r1 BEFORE ANY INDEPENDENT-REGIONS DISCOVERY, VALIDATION, OR TIMING MEASUREMENT  
**Date:** 2026-09-24  
**Parent evidence:** I10-1A auxiliary-index integration; BWT subblock sweep; external reference-cost run  
**Parent runs:** `35920128901`, `35927623136`, `35930672607`  
**Frozen candidate:** `i10-aux-unbwt-final-v1`  
**Scope:** existing ratio blocks and existing inner BWT `0xFF` subblocks only  
**New representation mechanism:** none  
**New wire semantics:** none  
**Production source change:** none  
**Workflow change:** none  
**Heavy measurement:** GitHub Actions or another explicitly approved remote CI runner only

This document is a preregistration. It does not report a new BWT result and does not authorize a local benchmark, source edit, workflow edit, or CI dispatch.

---

## 0. Freeze declaration and non-goals

The experiment is frozen before any new independent-region measurement. The existing I10 artifacts may be used to select the frozen corpus, controls, and known framing, but no new timing, RSS, route count, or BWT byte result from this experiment may be used to alter the arms, gates, thread counts, caps, or selection rules.

The primary experiment answers only:

> Can already-existing outer ratio blocks or already-existing inner `0xFF` BWT subblocks expose independent decode work without changing the encoded payload, and can that work be scheduled under a bounded memory budget?

This is a systems experiment, not a compression-mechanism claim.

The following are explicitly excluded:

- a new BWT transform or inverse-BWT implementation;
- a new auxiliary-index format;
- a new postcoder, QLFC, LZP, or context-coding format;
- changing BWT sampling policy in the primary experiment;
- changing ratio transforms, parser state, routing policy, or postcoder selection;
- claiming random access or streaming from parallel decode;
- replacing Brotli, xz, or libsais;
- integrating a scheduler into production `src/anvil.cpp`;
- modifying a workflow in this preregistration turn.

The aux-rate sweep is specified only as a later, separately labeled arm. It cannot influence the primary ruling or rescue a failed independent-region result.

---

## 1. Existing evidence and exact starting point

### 1.1 Historical ratio/decode/RSS tradeoff

The historical BWT byte result is real: BWT wins 7 of 12 Silesia files and beats xz on all seven. The measured BWT/Brotli/xz disagreement is documented in `docs/audit-2026-09-07/10-specialist-disagreement.md`. The current integrated auxiliary result is a separate, later systems point:

| Target/corpus | Aux wire delta | Whole-decode effect | Current role |
|---|---:|---:|---|
| dickens | +2,494 B | 1.364x | integrated aux speed point |
| webster | +2,535 B | 1.795x | integrated aux speed point |
| enwik8 | +3,054 B | 2.339x | integrated aux speed point |
| Silesia portfolio | +19,344 B | no route changes | exact charged aux portfolio |
| enwik8 | +3,054 B | no route changes | exact charged aux portfolio |

The source-of-record results are `docs/I10-AUX-UNBWT-RESULTS.md`. These are not new measurements in this experiment.

The same-job external reference-cost result is `FRONT-GAP_COST`, not a crossing:

| Corpus | ANVIL aux | xz -9e | Brotli q11/lw30 | Binding deficits |
|---|---:|---:|---:|---|
| Silesia | 46,466,339 B; ~47.9 MB/s; 248.4 MiB | 48,456,004 B; ~82.5 MB/s; 54.3 MiB | 49,383,136 B; ~166.9 MB/s; 124.5 MiB | decode and memory |
| enwik8 | 23,537,422 B; ~26.5 MB/s; 598.9 MiB | 24,831,648 B; ~103.6 MB/s; 66.2 MiB | 24,810,180 B; ~150.2 MB/s; 251.9 MiB | decode and working set |

The existing aux mechanism improves inverse-BWT throughput but does not remove the large-input working-set problem.

### 1.2 Existing subblock evidence

The frozen I10 subblock sweep measured 8/16/32/64/128 MiB caps. Its full-wire results are source-of-record evidence, not inputs that may be retuned here:

- Silesia 16 MiB: +1.0169% bytes versus 128 MiB, RSS 125.7 MiB versus 248.4 MiB.
- Silesia 8 MiB: +1.9346% bytes, RSS 124.4 MiB, serial decode 3.7022 s versus 4.2015 s.
- enwik8 8/16/32 MiB: BWT routes away, so these are not bounded-BWT results.
- enwik8 64 MiB: BWT retained, +3.2259% bytes and RSS 411.4 MiB; the timing row is ambient-invalid.
- No smaller cap dominates the 128 MiB max-ratio point.

The subblock conclusion remains: subblocking is a rate/working-set profile lever, not a completed frontier solution.

---

## 2. Frozen representations and framing

### 2.1 Outer ratio blocks

The existing rev-2 ratio container already emits independent outer blocks. The existing encoder path partitions the input at `opt.block_size` and emits a mode/17 payload, payload length, CRC, and payload for each block. The existing decoder path already has a bounded worker-count option named `decode_threads`.

The primary outer arms use the existing ratio block sizes:

```
O128, O64, O32, O16
```

where the number is the outer ratio block size in MiB. For the direct-BWT causal arms, use:

```
--parse=ratio
--ratio-backend=bwt
--ratio-context=off
--ratio-lines=off
```

The auto portfolio arms use the same block sizes with `--ratio-backend=auto`, but are secondary to the direct-BWT comparison.

The outer container framing, including every block header and CRC, is part of the measured complete bytes.

### 2.2 Inner BWT subblocks

The existing ratio BWT backend already emits an outer `0xFF` frame when `bwt_subblock` is smaller than the transformed input:

```
0xFF
uvarint subblock_count
repeat subblock_count times:
    uvarint decoded_len
    uvarint payload_len
    inner_bwt_payload
```

Each inner payload is either legacy v1 or auxiliary v2. The auxiliary v2 payload retains its complete `0xFE` tag, sampling rate, count, index array, and postcoder payload. The full framing contract is in `FORMAT.md`; no new field is permitted.

The primary inner arms use:

```
I128, I64, I32, I16
```

where the number is the existing `bwt_subblock` cap and the outer ratio block remains 128 MiB. The inner arm must use the same direct-BWT options as its matched outer control.

### 2.3 Matched-boundary identity control

For every matched cap `C in {16,32,64}`:

1. encode the same source with outer block size `C`, inner cap 128 MiB;
2. encode the same source with outer block size 128 MiB, inner cap `C` MiB;
3. verify that every compressed region has the same input byte interval;
4. compare the underlying BWT backend payload for each matched region byte-for-byte;
5. charge the outer versus inner framing separately.

The underlying BWT payload identity is expected for the direct, transform-disabled, same-backend, same-postcoder, same-aux arm. A mismatch is an implementation failure, not evidence that one framing is intrinsically better. The complete outer and inner container bytes may differ because their framing is intentionally different.

This matched-boundary check is required before any outer-versus-inner rate comparison.

---

## 3. Frozen decode arms

### 3.1 Identical-payload rule

For every framing/cap/aux/backend arm, prepare the encoded payload once. Decode that exact payload at every requested thread count. Do not re-encode between thread-count arms.

For each arm:

```text
D1: requested decode threads = 1
D2: requested decode threads = 2
D4: requested decode threads = 4
D8: requested decode threads = 8
```

The requested value is recorded separately from the actual worker count. If fewer regions exist than requested workers, the implementation may use fewer workers, but it must report `requested_threads`, `actual_threads`, and `region_count` exactly.

The primary thread comparison is therefore:

```text
same payload SHA-256
same complete wire bytes
same source SHA-256
same decoded output SHA-256
D1 versus D2/D4/D8
```

A thread-count arm that changes the payload is invalid.

### 3.2 Outer-thread arm

Use the existing `--decode-threads` path for outer blocks. No production source change is required for this arm.

Measure `O128/O64/O32/O16` at D1/D2/D4/D8, with aux-off and aux-on, first for direct BWT and then for the auto portfolio.

### 3.3 Inner-thread arm

The current production inner decoder is serial. A future remote experiment may use a research-only scheduler that:

- parses and validates the existing `0xFF` frame before launching work;
- decodes each existing inner payload independently;
- never changes the payload or the BWT algorithm;
- uses a bounded number of live region workers;
- writes or joins outputs in original region order;
- propagates any worker failure to the whole decode.

This scheduler is not a production integration. If it is not available in a pinned remote build, the inner threaded rows are `INCONCLUSIVE-INFRA`; they may not be replaced with a different payload or a post-hoc implementation.

Measure `I128/I64/I32/I16` at D1/D2/D4/D8, with aux-off and aux-on, first for direct BWT and then for the auto portfolio.

### 3.4 No hidden parallelism

The following are not equivalent to a valid independent-region result:

- changing `decode_threads` without recording actual workers;
- comparing one-thread payload A against a separately encoded multi-thread payload B;
- allowing worker count to change model/cache state without an A/A or serialized control;
- dropping slow repetitions;
- using thread count only in the encoder;
- running a different reference binary in the comparison job.

The libsais auxiliary index is an intra-region inverse-BWT mechanism. Outer/inner region scheduling is a separate mechanism. Their effects must be reported separately.

---

## 4. Bounded memory contract

### 4.1 Reference memory budget

For each corpus, define the same-run reference peak RSS values:

```text
R_xz = peak RSS of xz -9e
R_br = peak RSS of raw Brotli q11/lw30
M_budget = 2 * min(R_xz, R_br)
```

`M_budget` is a hard decode-memory comparison budget, not a target that may be relaxed after seeing results. The artifact must report `R_xz`, `R_br`, `M_budget`, and the selected reference identity.

The experiment does not alter the established project rule that a bytes-only result with excessive RSS is `FRONT-GAP_COST`.

### 4.2 Worker and live-region bounds

A scheduler must declare before timing:

```text
max_workers
max_live_regions
memory_budget_bytes
```

For the requested D1/D2/D4/D8 arms, `max_workers` is the requested count and `max_live_regions` is at most that count. The scheduler may not allocate one worker, output vector, or BWT work array per subblock when the requested worker count is smaller.

A candidate thread arm passes the memory gate only if its observed peak decode RSS is at most `M_budget`. A D8 arm that is faster but exceeds the budget is not a passing balanced point. It remains a measured negative/cost result.

The report must distinguish:

- whole-output allocation;
- current outer-thread join/copy overhead;
- per-region postcoder buffers;
- per-region BWT bytes;
- libsais work arrays;
- auxiliary-index bytes;
- worker-local and shared allocations.

A scheduler must not hide an allocation by counting only the final output file size.

### 4.3 RSS measurement

Measure peak resident memory per process, not only a sampled working set:

- use the remote runner's maximum-RSS facility;
- record the exact command and `/usr/bin/time`-style output where available;
- record both per-file and aggregate-script measurements;
- report the maximum across files and the maximum across concurrent regions;
- retain the output file and process exit status for every run;
- never substitute compressed payload size for RSS.

The existing subblock sweep's maximum per-file RSS is historical evidence only. It must not be combined with the new multi-thread measurements as if it were a same-job comparison.

---

## 5. Thread-error, ordering, and safety behavior

The following are mandatory correctness cases, not optional diagnostics.

### 5.1 Worker failure propagation

A worker exception, decoder rejection, CRC mismatch, or allocation failure must:

1. stop admission of new work where possible;
2. join or cancel all already-running workers deterministically;
3. return a nonzero process status;
4. write no accepted output artifact;
5. record the region index, worker count, and error class;
6. never swallow the first error and continue as if the decode succeeded.

A retry is allowed only as an explicitly labeled infrastructure retry. It may not replace the original failure or change a gate.

### 5.2 Ordering

Workers may complete in any order. The final output must be assembled in encoded region order. A successful D2/D4/D8 run must have the exact same output SHA-256 as D1.

### 5.3 Malformed framing

At minimum, the existing strictness must remain covered for:

- zero outer subblock count;
- impossible subblock count;
- zero decoded region length;
- decoded length larger than remaining output;
- payload length beyond the enclosing frame;
- truncated inner payload;
- trailing bytes after the outer frame;
- malformed auxiliary `0xFE` rate/count/index;
- invalid inner BWT primary/index;
- postcoder truncation/trailing bytes;
- outer block header/CRC truncation;
- worker failure injected into a valid multi-region frame.

Any parser acceptance of a malformed frame is an unconditional correctness failure.

### 5.4 Resource exhaustion

The experiment must have a finite timeout. Timeout, OOM, runner termination, or infrastructure failure is `BLOCKED_INFRA` or `INCONCLUSIVE_INFRA`, not a codec failure and not permission to reduce the corpus, thread grid, or gate after observing partial data.

---

## 6. Random-access and streaming implications

Independent outer blocks and inner subblocks are not automatically random-access regions.

### 6.1 Current format

The current inner frame stores sequential `decoded_len` and `payload_len` fields but no absolute compressed offset table. A decoder can derive offsets by scanning from the frame start; it cannot jump directly to an arbitrary subblock without either scanning or adding an index.

The current outer ratio container likewise provides block boundaries and CRCs but no separately published seek index in this experiment.

Therefore the preregistered ruling is:

```text
RANDOM_ACCESS = NOT_CLAIMED
```

Parallel decode must not be described as random access, seek support, or streaming.

### 6.2 Future index experiment

A future random-access experiment may add an explicit offset/index representation, but it must:

- charge the complete index bytes;
- specify index encoding and bounds;
- define whether the index is per outer block, per inner region, or both;
- measure scan and seek costs;
- pass malformed/truncated index tests;
- remain a separate preregistration.

No index is authorized by this document.

---

## 7. Same-run controls

All primary evidence must be collected in one remote job per corpus, from one checkout and one pinned toolchain. Cross-job absolute timing is context only.

### 7.1 Input and identity controls

For every file:

- canonical source byte count;
- source SHA-256;
- source file name and corpus identity;
- frozen candidate source SHA;
- binary SHA-256;
- exact command line;
- outer block size and inner subblock cap;
- aux state;
- direct-BWT versus auto route;
- reference codec identity.

The canonical primary corpus is the existing Silesia 12-file set and enwik8 used by the I10 artifacts. No new corpus is introduced. Tiny synthetic fixtures are correctness-only and never performance evidence.

### 7.2 Codec arms

The same job must measure, from identical source bytes:

- direct BWT with `aux=off`;
- direct BWT with `aux=on`;
- raw Brotli q1;
- raw Brotli q4;
- raw Brotli q6;
- raw Brotli q9;
- raw Brotli q11/lw30;
- xz -9e;
- auto portfolio with aux-off;
- auto portfolio with aux-on.

The direct-BWT arms are the causal primary arms. The raw references and auto arms are required controls and frontier context; they cannot be replaced by a cross-job number.

### 7.3 Transform control

The primary region experiment uses transforms off:

```text
--ratio-context=off
--ratio-lines=off
```

This isolates framing, BWT, postcoder, and scheduling. A transform-enabled run, if later performed, must be a separately labeled arm and may not retroactively change the primary result.

### 7.4 Timing protocol

Use the repository's paired/interleaved protocol:

- symmetric warmups;
- randomized or alternating A/B order;
- at least seven paired repetitions for a scout, with a larger count for a frontier claim;
- all raw repetitions retained;
- median and robust CV;
- paired log ratio `log(T_candidate / T_control)`;
- seeded bootstrap 95% confidence interval;
- A/A null experiment;
- CPU affinity and ambient-load gate;
- no dropped outliers.

A blocked ambient run is neutral, not a failure or a pass.

---

## 8. Exact primary gates

The gate outcomes are mechanical and are evaluated in the order below. A later gate cannot rescue an earlier failure.

### IR-G0 — Identity and correctness

All must pass:

1. frozen source and binary identity recorded;
2. every source hash matches the canonical manifest;
3. every arm roundtrips byte-exactly;
4. every D1/D2/D4/D8 output SHA equals the serial output SHA;
5. all malformed-frame cases are rejected as specified;
6. injected worker errors terminate nonzero and do not publish output;
7. no output is accepted after a timeout, OOM, or partial worker join;
8. no dropped files, repetitions, or malformed cases.

Any failure gives `NO-GO-IR` for the affected experiment.

### IR-G1 — Wire neutrality

For every same-payload thread comparison:

```text
bytes(D1) = bytes(D2) = bytes(D4) = bytes(D8)
payload_sha256(D1) = payload_sha256(D2) = payload_sha256(D4) = payload_sha256(D8)
```

This applies separately to aux-off and aux-on. Any thread-related wire change is `NO-GO-IR`, not a new format.

For matched outer/inner boundaries, the underlying region BWT payloads must be byte-identical under the direct, transform-disabled control. The outer and inner complete containers are expected to differ only in their declared framing and related metadata.

### IR-G2 — Timing validity

A thread arm is timing-valid only if:

- A/A null CI spans 1.0;
- maximum arm robust CV is at most 0.15;
- ambient load is within the declared gate;
- every repetition is retained;
- output verification occurs outside the timed interval;
- no infrastructure failure occurred.

An invalid timing arm is `BLOCKED_AMBIENT` or `INCONCLUSIVE_INFRA`; it is not a speed pass or failure.

### IR-G3 — Practical decode speed

For each requested worker count `k` in `{2,4,8}`, relative to the same payload at D1:

```text
paired_ratio_k = T_k / T_1
CI95_k         = bootstrap 95% CI of paired_ratio_k
```

The arm clears the practical speed gate only if:

```text
CI95_k.upper <= 0.98
```

This is the existing 2% practical-speed threshold. Report the raw numerator, raw denominator, median, robust CV, paired log ratio, CI, and every repetition.

A speed pass at D2/D4/D8 does not imply a linear scaling claim. Report the observed scaling factors without upgrading them to a theoretical model.

### IR-G4 — Bounded RSS

For each timing-valid thread arm:

```text
peak_rss_k <= M_budget
```

where `M_budget = 2 * min(R_xz, R_br)` is calculated in the same job and same corpus.

The arm also must satisfy:

- configured worker and live-region bounds;
- no unbounded per-region allocation;
- no memory-budget violation hidden by measuring only the output file;
- complete RSS provenance.

If an arm exceeds the budget, it is `RSS-BUDGET-FAIL`, regardless of its speed.

A thread arm is an engineering pass only when IR-G0 through IR-G4 all pass for that arm. The selected arm is the non-dominated passing arm by the frozen report order: lowest complete bytes, then lowest decode time, then lowest RSS, then lowest requested worker count.

### IR-G5 — Rate/framing evidence

For every outer/inner cap and aux state, report:

```text
complete_wire_bytes
outer_header_bytes
outer_block_count
outer_mode17_payload_bytes
outer_crc_bytes
inner_0xff_bytes
inner_count_bytes
inner_region_length_bytes
inner_payload_length_bytes
aux_tag_rate_count_bytes
aux_index_bytes
postcoder_bytes
transform_metadata_bytes
route_count
```

No inner or outer cap is called Pareto solely because it is non-dominated inside the cap sweep. The complete wire result is mandatory.

The matched-boundary identity check must pass before outer and inner rate costs are interpreted as framing/context costs rather than implementation differences.

### IR-G6 — External frontier interpretation

This is a classification gate, not an automatic adoption gate. A candidate may be called an external frontier point only if, in the same job:

```text
complete_bytes < min(raw_brotli_q11_bytes, xz_bytes)
decode_time <= 2 * min(raw_brotli_q11_decode, xz_decode)
peak_rss <= 2 * min(raw_brotli_q11_rss, xz_rss)
decoder_binary_size <= reference_decoder_size
```

The candidate must be strictly better on at least one axis. If bytes win but decode or RSS fail, the result is `FRONT-GAP_COST`.

No independent-region result may be described as a Pareto crossing without all five comparisons.

### IR-G7 — Random-access honesty

The default result is `RANDOM_ACCESS_NOT_CLAIMED`.

A result may claim a random-access property only if a separate index/offset wire is present, fully charged, bounded, malformed-tested, and measured. This preregistration does not authorize that index.

### IR-G8 — Primary ruling

Apply mechanically:

1. any IR-G0 or IR-G1 failure: `NO-GO-IR`;
2. any unavailable/malformed thread-error behavior: `NO-GO-IR`;
3. all arms timing-invalid: `BLOCKED_IR`;
4. at least one arm clears IR-G3 and IR-G4: `GO-IR-BALANCED-REGION`;
5. all timing-valid arms fail IR-G3: `NO-SPEED-IR`;
6. a speed-valid arm fails IR-G4: `RSS-BUDGET-FAIL`;
7. `GO-IR-BALANCED-REGION` is not automatically `GO-FRONTIER`; apply IR-G6 separately.

---

## 9. Later aux-rate sweep arm

This arm is deliberately excluded from the primary decision.

Run it only after the independent-region result is recorded, using the same source, corpus, complete wire accounting, aux-off control, raw references, A/A protocol, and RSS budget. Keep outer block size and inner cap fixed at the selected 128 MiB control unless a separately frozen profile is being evaluated.

The later grid is target walk density:

```text
256, 512, 1024, 2048, 4096 walks
```

For each target, derive the existing power-of-two `r` policy and record:

- `r`;
- `aux_count`;
- exact index bytes;
- `0xFE` tag/rate/count bytes;
- complete inner payload bytes;
- outer/inner framing bytes;
- total complete container bytes;
- source and payload SHA-256;
- BWT and postcoder identity with the 1024-walk control where applicable.

The later aux arm has its own gates:

1. BWT bytes and postcoder payload identity must be unchanged except for the intended aux index/header;
2. the complete index cost must be charged;
3. whole-decode speed must improve by the existing 2% practical threshold against the matched 1024-walk payload;
4. RSS must remain within `M_budget`;
5. no outer/inner or reference control may be omitted.

A later aux failure cannot retroactively change the primary region ruling. A later aux pass cannot authorize a default policy without a separate profile decision.

---

## 10. Why backend replacement is deferred

No backend replacement is an arm of this preregistration.

Brotli and xz are required controls because they establish the current reference front, but replacing ANVIL's BWT with either one would trade away the measured byte advantage rather than answer the independent-region question. Replacing libsais would add a new decoder implementation and new correctness surface while the integrated aux mechanism has already demonstrated a substantial whole-decode improvement.

Backend replacement may be reconsidered only after:

- the outer/inner region experiment is complete;
- the failure mode is localized to inverse BWT or working-set behavior rather than scheduling;
- a candidate has a concrete same-run speed/RSS requirement;
- decoder binary size, dependency, malformed-input, and complete-wire costs are preregistered.

Until then, backend replacement is a negative/deferred path, not a default or a hidden fallback.

---

## 11. Required artifact contract

The remote experiment must upload one immutable artifact directory. A human-readable job summary is not a substitute for any file below.

```text
i10-bwt-independent-regions/
  prereg.md
  manifest.json
  provenance/
    runner.txt
    build.txt
    source-ids.txt
    binary-sha256.txt
    corpus-manifest.csv
    commands.txt
  payload-index.csv
  bytes.csv
  wire-breakdown.csv
  routes.csv
  timing/
    raw-reps.csv
    paired-null.json
    paired-outer-d1-d2.json
    paired-outer-d1-d4.json
    paired-outer-d1-d8.json
    paired-inner-d1-d2.json
    paired-inner-d1-d4.json
    paired-inner-d1-d8.json
    paired-summary.csv
  rss/
    per-process.csv
    peak-summary.json
    memory-budget.json
  correctness/
    roundtrip.csv
    matched-boundary.csv
    malformed.csv
    thread-errors.csv
    fuzz-summary.json
  references/
    reference-bytes.csv
    reference-timing.csv
    reference-rss.csv
    frontier.csv
  aux-rate-later/
    README.txt
    index-sweep.csv
    paired-summary.csv
  summary.md
  summary.json
  checksums.sha256
```

### 11.1 `manifest.json`

`manifest.json` is the provenance authority and must contain:

- experiment name and prereg revision;
- workflow run ID and attempt;
- frozen candidate tag and resolved source SHA;
- tooling source SHA;
- runner OS/image, kernel, `uname -a`, `lscpu`, memory size;
- compiler, CMake, Ninja, Python, Brotli, xz, and libsais identities;
- feature flags and build flags;
- requested and actual thread counts;
- affinity result;
- corpus manifests and source hashes;
- command lines;
- reference codec versions and parameters;
- `M_budget` inputs and calculation;
- artifact file list and checksums;
- explicit validity status: `VALID`, `BLOCKED_AMBIENT`, or `INCONCLUSIVE_INFRA`.

### 11.2 `payload-index.csv`

Required columns:

```text
corpus,file,source_bytes,source_sha256,
framing,cap_mib,aux,backend,mode,route,
region_index,region_input_start,region_input_bytes,
payload_bytes,payload_sha256,
container_bytes,container_sha256,
output_sha256,roundtrip
```

`framing` is exactly `outer` or `inner`. `aux` is exactly `off` or `on`. No row may use a transformed-size proxy in place of `payload_bytes` or `container_bytes`.

### 11.3 `bytes.csv` and `wire-breakdown.csv`

`bytes.csv` must contain one row per source/framing/cap/aux/backend/thread arm and include:

```text
complete_wire_bytes,source_bytes,ratio,
aux_off_bytes,aux_on_bytes,aux_delta_bytes,
brotli_bytes,xz_bytes,route_changed
```

`wire-breakdown.csv` must contain the complete accounting required by IR-G5, including all outer headers, `0xFF` framing, `0xFE` framing, index bytes, postcoder bytes, transform metadata, and CRCs.

### 11.4 `timing/raw-reps.csv`

Every repetition is retained with:

```text
run_id,corpus,file,framing,cap_mib,aux,backend,
requested_threads,actual_threads,repetition,order,
seconds,peak_rss_kib,exit_code,output_sha256,
ambient_sample,ambient_gate
```

There is no outlier deletion. A failed or blocked repetition remains in the artifact with its reason.

### 11.5 Paired JSON files

Every paired JSON must include:

```text
control_arm
candidate_arm
requested_threads
actual_threads
repetitions
warmups
measurement_order
control_median_s
candidate_median_s
control_robust_cv
candidate_robust_cv
paired_log_ratio
paired_ratio_point
paired_ratio_ci95_lower
paired_ratio_ci95_upper
bootstrap_seed
bootstrap_replicates
ambient_gate
timing_valid
correctness_valid
```

The A/A null file must show whether its CI spans 1.0. A candidate CI that is not valid is not evidence of speed.

### 11.6 RSS and memory-budget files

`rss/per-process.csv` must contain:

```text
run_id,corpus,file,framing,cap_mib,aux,requested_threads,
actual_threads,peak_rss_kib,elapsed_s,exit_code,
rss_source,worker_live_regions
```

`rss/memory-budget.json` must contain:

```text
rss_xz_kib
rss_brotli_kib
m_budget_kib
formula
per_arm_peak_rss_kib
per_arm_budget_result
max_worker_live_regions
```

The current-thread output-copy behavior must be reported separately from worker-local BWT/postcoder memory.

### 11.7 Correctness files

`correctness/roundtrip.csv` must include every arm and every file, with output size, source SHA, output SHA, and status.

`correctness/matched-boundary.csv` must show, for every matched outer/inner region, input interval equality, underlying BWT payload SHA equality, and complete outer/inner container byte differences.

`correctness/malformed.csv` must list every malformed case, expected rejection class, actual exit status, and whether any output was published.

`correctness/thread-errors.csv` must record injected failure region, worker count, join result, exit status, elapsed time, and output-publication decision.

`correctness/fuzz-summary.json` must record the remote fuzz command, seed, case count, mutation count, build identity, and pass/fail status.

### 11.8 Reference/frontier files

`references/reference-bytes.csv`, `references/reference-timing.csv`, and `references/reference-rss.csv` must contain the same-run Brotli and xz rows with complete wire bytes, encode/decode times, RSS, environment, and roundtrip status.

`references/frontier.csv` must contain the mechanically recomputed classification:

```text
candidate_bytes,best_reference_bytes,
candidate_decode_s,best_reference_decode_s,
candidate_peak_rss_kib,best_reference_peak_rss_kib,
decoder_binary_bytes,reference_decoder_bytes,
classification
```

Allowed classifications are:

```text
FRONT_CROSSING
FRONT_GAP_COST
DEGENERATE
INSUFFICIENT
```

### 11.9 `summary.json`

`summary.json` must be machine-readable and include:

- each gate ID;
- `PASS`, `FAIL`, `BLOCKED`, or `NOT_RUN`;
- the exact failing condition;
- selected outer/inner arm if any;
- requested and actual worker counts;
- complete bytes, decode, RSS, and binary-size deltas;
- the primary ruling;
- the later aux-rate status as `NOT_RUN` unless separately run;
- all comparability flags;
- checksums for referenced artifacts.

A result may not be summarized as successful unless `IR-G0`, `IR-G1`, `IR-G2`, `IR-G3`, and `IR-G4` are all passed for the named arm.

---

## 12. Execution discipline and disposition

1. Commit this preregistration before any new independent-region corpus measurement.
2. Do not edit production `src/anvil.cpp` in the preregistration turn.
3. Do not edit a workflow in the preregistration turn.
4. Do not run a local build, benchmark, sweep, or fuzz campaign.
5. Resolve and record the frozen candidate source and binary identity remotely.
6. Run correctness and malformed-frame checks remotely before timing.
7. Prepare each encoded payload once and reuse it across all thread arms.
8. Run the outer and inner direct-BWT arms before auto/reference frontier interpretation.
9. Preserve all blocked, invalid, failed, and infrastructure-inconclusive rows.
10. Apply the gates mechanically; do not retune caps, thread counts, memory budgets, or controls after seeing results.
11. Do not use the later aux-rate arm to rescue the primary independent-region result.
12. Do not replace a backend as a consequence of this experiment without a separate preregistration.

The experiment closes with one of:

```text
GO-IR-BALANCED-REGION
NO-SPEED-IR
RSS-BUDGET-FAIL
NO-GO-IR
BLOCKED-IR
```

`GO-IR-BALANCED-REGION` means only that one named outer or inner region arm passed the frozen correctness, wire-neutrality, timing, speed, and RSS gates. It does not mean that ANVIL crossed the external Brotli/xz frontier, that random access is supported, that the aux-rate sweep passed, or that a backend replacement is justified.
