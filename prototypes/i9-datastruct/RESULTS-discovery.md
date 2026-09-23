# datastruct (I9 follow-up) — automatic period + field-partition discovery, inside the reference

Lane `datastruct`, task `task_c1dabc9354c44b5abc56dcc4da510fe8`.
Artifacts: `prototypes/i9-datastruct/{auto_colref.py, verify_discovery.py, out/*.auto}`.
Status: **prototype wire only — NOT an ANVIL row; {synthetic} wherever the file is
generator-made; no crossing language.** Novelty questions to research-gate (the
auto-search is infrastructure; the underlying mechanism is unchanged from the verified
`datastruct-ts` deliverable). All discovery state is charged in the wire.

---

## 0. Verdict

**Auto-discovery works on fixed-period structured files, with discovery charged.**
Given only the byte stream (no P, no partition, no offsets), the encoder recovers the
true structure and the decoder rebuilds from transmitted state:

| file | discovered P | AUTO (charged) | supplied-oracle ladder (same info, free) | discovery overhead |
|---|---:|---:|---:|---:|
| synth-timeseries.bin | **14** | **89,916 B** | 89,877 B | **+39 B (+0.04%)** |
| synth-columnar-align.bin | **23** (not the 184 alias) | **24,026 B** | 23,995 B | **+31 B (+0.13%)** |

- synth-timeseries partition recovered exactly: `0:8:prev,8:4:prev,12:2:posmod` (the
  hand-supplied oracle layout from the previous task).
- synth-columnar-align: auto rejects the 184 alias and finds
  `0:6:posmod,6:8:posmod,14:4:prev,18:3:posmod,21:2:prev`, which is **29 B smaller than
  the previous hand-supplied layout** (24,055 B) and **45,722 B below the supplied
  byte-column rung** (69,748 B).
- Positive control, `generated.repeat.jsonl` (pure repetition): P=235 discovered,
  partition = all `const` (exact reproduction) — discovery finds the exact period.

**Required NON-SYNTHETIC test: NOT a success — honest discoverability bound.**
`generated.jsonl`: auto discovers P=235 and produces 1,199,898 B (roundtrip OK), which is
**5.8x brotli-q6 (206,840 B)** and 6.5x ANVIL `mdl-rans` (183,506 B).
`generated.sqlite`: auto discovers P=140, proxy 1,359,687 B, **5.4x brotli-q6 (252,591 B)**.
The fixed-period mechanism is not competitive on these files because **the record phase is
not stable** (see §4). Auto beats a *naive* supplied period, but not the supplied-oracle
period, and neither is remotely competitive. The bound is the deliverable for that leg.

---

## 1. Algorithm (no oracle)

### 1.1 Period discovery (`auto_colref.discover_period`)
1. **Match-fraction spectrum** over a charged budget `pmax`:
   `m(p) = #{i : x[i] == x[i-p]} / (n-p)`, computed for all `p in [1, pmax]`
   (`pmax` is transmitted).
2. **Candidate set** (charged): top `K/2` periods by `m(p)` plus all divisors `>= 2` of
   the top 4 peaks (aliasing guard — a multiple such as 184 can score higher than the
   true period 23; the divisors put 23 back on the table).
3. **Coding-cost proxy** for each candidate, in bits:
   `cost(p) = sum_j H0(x[i,j] - x[i-1,j])  +  LAM*p  +  8*(n mod p)`
   (per-column byte-delta entropy + per-column model/adaptation penalty + raw tail).
   Choose the min (ties -> smaller p).
   Encoder constants: `LAM = 48` bits/column, `K = 24`.

### 1.2 Partition discovery (`auto_colref.discover_partition`)
Dynamic programming over the record template `[0, P)`:
- candidate segments `[s, s+w)`, `w in 1..8`;
- exact classes tested first: `const` (numpy all-equal), `posmod`
  (`v_i = phase + i*step mod m`, recovered exactly from a gcd identity; prefix-filtered
  on 128 records then **fully verified**, so the emitted class is exact);
- otherwise `prev` / `delta`, cost = sum of per-byte empirical
  conditional/delta entropies + 48-bit per-field header.
Backtrack gives the field table. Everything is verified before it is written.

### 1.3 Decoder
Reads `P`, the charged candidate list, and the field table from the wire and rebuilds
with the same `FieldCoder` classes as `colref.py`. **No search at decode time.** Roundtrip
is asserted in-process and the written wire is decoded again in `verify_discovery.py`.

---

## 2. Charged discovery state

Wire `AC2`: mode, P u32, nrec u32, total_n u32, **pmax u16, K u8, K x u16 candidate
periods, flags u8, maxw u8**, field table, arithmetic body. For K=24 the discovery block
is 57 B, plus the field table (the partition description). The candidate costs and the
proxy constants are encoder-side and are not needed by the decoder; the *periods actually
cost-evaluated* and the search budget are transmitted. Measured total overhead vs the
supplied-info ladder: **+39 B (ts), +31 B (ca)**.

*Honest accounting note:* the DP/proxy heuristic constants (`LAM`, `HDRF`, prefix
length) are not transmitted; a stricter protocol could charge 2-3 more bytes for them.
They are deterministic algorithm constants, not per-file information.

---

## 3. Measured results — synthetic fixed-period files

Recompute: `python prototypes/i9-datastruct/verify_discovery.py` (writes `out/ts.auto`,
`out/ca.auto`, roundtrip-verified).

| variant (synth-timeseries.bin, 280,000 B) | bytes | ratio |
|---|---:|---:|
| raw_o0 (no mechanism) | 207,197 | 0.7400 |
| raw_o1 (no mechanism) | 187,436 | 0.6694 |
| supplied P=14, byte-columns | 97,057 | 0.3466 |
| supplied P=14, oracle partition | 89,877 | 0.3210 |
| **AUTO (P+partition discovered, charged)** | **89,916** | **0.3211** |

| variant (synth-columnar-align.bin, 276,000 B) | bytes | ratio |
|---|---:|---:|
| raw_o0 | 211,216 | 0.7653 |
| raw_o1 | 184,568 | 0.6687 |
| supplied P=23, byte-columns | 69,748 | 0.2527 |
| previous hand-supplied layout (I9 `datastruct-ts`) | 24,055 | 0.0872 |
| supplied P=23, auto-discovered partition | 23,995 | 0.0869 |
| **AUTO (P+partition discovered, charged)** | **24,026** | **0.0871** |

`out/ts.auto` sha256 `90a87fe17f4111e6cdb88012eaabdbf4…`,
`out/ca.auto` sha256 `28858ad114fe28b1681514f5d4e1549b…`.
Dual-bar caveat carriers over from `datastruct-ts`: these results remain above the
same-transform references (xz `--delta`/brotli+delta) — capability, not crossing.

---

## 4. Non-synthetic result and the honest bound

`generated.jsonl` was run end-to-end: AUTO P=235, **1,199,898 B (ratio 0.4262), roundtrip
OK**, partition 30 `prev` fields. Proxy-optimal fixed period found within the charged
search. But the absolute number is far from the reference class:

| generated.jsonl (2,815,267 B) | bytes | ratio |
|---|---:|---:|
| **AUTO fixed-period (P=235, charged)** | **1,199,898** | **0.4262** |
| best fixed-period proxy, P=235 | 1,284,086 | 0.4561 |
| naive supplied period = modal line length 234 | 1,773,088 (proxy) | 0.6298 |
| ANVIL mdl-rans (CSV) | 183,506 | 0.065 |
| brotli q6 / q9 / q11 (CSV) | 206,840 / 206,162 / 150,619 | 0.073 / 0.073 / 0.054 |
| zstd-19 (CSV) | 166,693 | 0.059 |

**Why it fails — phase instability (measured):** line lengths take 9 values in
229..250 (`234x7209, 233x3904, ...`, plus 59 drift lines at 248-250), so the record
phase is not locked. For P=235 the line-start positions mod 235 occupy **235/235 distinct
phases, phase entropy 7.76 bits vs the 7.88-bit maximum** — effectively uniform. A
fixed-period column model is therefore reading arbitrary alignments; the best match
fraction any period achieves is 0.57, and the per-column residual entropy keeps the wire
above 1.1 MB. A phase-locked variant would need per-record alignment state (a shift per
record) — that is a record-delimited/LZ mechanism, outside the fixed-period model.

`generated.sqlite` (1,740,800 B, 425 x 4096-B pages): auto discovers **P=140** (proxy
1,359,687 B); the obvious supplied page period 4096 gives proxy 1,399,513 B. Pages are
not near-duplicates: at P=4096 the match fraction is 0.103 and **0 constant offsets**;
references are ANVIL mdl 323,014, brotli q6 252,591, zstd-19 215,468. Not competitive.

Additional (not required, same bound): `generated.log` (1,942,280 B) discovers P=108
(proxy 990,744 B) vs modal-line-length 107 (proxy 1,314,884 B); references ANVIL mdl
132,434, brotli q6 139,649.

**Success criterion read-out:** auto beats a naive supplied period on the non-synthetic
files in proxy terms (jsonl 1.28 MB < 1.77 MB at the modal 234; log 0.99 MB < 1.31 MB at
107) and finds the proxy-optimal period within the charged search, but it does **not**
beat the supplied-oracle ladder on any non-synthetic file, and no fixed period makes the
mechanism competitive there. Per the task's failure clause, the honest discoverability
bound is: **fixed-period discovery requires a phase-stable period; variable-length
record files (jsonl, log) and non-duplicate page files (sqlite) violate it, and the
required extension is per-record alignment, not period discovery.**

---

## 5. Limitations

- Prototype wire, Python coder; not an ANVIL row, no throughput measured, no integration.
- Discovery is heuristic: `K=24` candidates, `LAM`/`HDRF` constants, 128-record prefix
  filter for `posmod`. A missed candidate degrades to `prev` (no correctness risk).
- `pmax` bounds the search: a true period above `pmax` is not found (charged budget).
- The proxy is optimistic; the counted wire is the reported number.
- `{synthetic}`: the two synth files are generator-made with exact APs; the non-synthetic
  files are also generator-produced corpus files, labelled by provenance.
- No novelty claim: period+partition search + columnar/delta contexts are
  infrastructure/prior art; the gate has already ruled the mechanism NO-novelty.

---

## 6. Recompute commands (exact)

```powershell
. .\env.ps1
python prototypes\i9-datastruct\verify_discovery.py              # ts + ca, roundtrip-verified
python prototypes\i9-datastruct\auto_colref.py probe  tests\corpus\generated.jsonl --pmax 1024
python prototypes\i9-datastruct\auto_colref.py verify tests\corpus\generated.jsonl prototypes\i9-datastruct\out\jsonl.auto --pmax 1024
python prototypes\i9-datastruct\auto_colref.py probe  tests\corpus\generated.sqlite --pmax 512
```

`out/jsonl.auto` sha256 `3701c0e405673649e00c290f7921386b…` (1,199,898 B, roundtrip OK).
