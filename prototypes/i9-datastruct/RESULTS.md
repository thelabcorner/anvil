# datastruct (I9) — synth-timeseries byte axis via a columnar reference-copy mechanism

Lane: `datastruct`, task `datastruct-ts`, swarm `anvil-i9-pareto`.
Artifacts: `prototypes/i9-datastruct/{colref.py, analyze_ts.py, verify_artifacts.py, out/*}`
Status: **prototype wire only — NOT an ANVIL row.** No frontier/crossing claim.
research-gate VERIFIED the ladder (double-run, byte-identical) and ruled **NOVELTY: NO**
(`docs/gate-ruling-i9-datastruct-ts.md`, ledger PART XIV S5): adopt-class capability. The
dual-bar same-transform controls (§0/§4) put even a perfect integration at FRONT-GAP, not
a crossing. Every number labelled measured / derived / projection. Recompute commands below.

---

## 0. TL;DR

| object | bytes | ratio | basis |
|---|---:|---:|---|
| synth-timeseries.bin | 280,000 | 1.000 | measured, `tests/corpus`, sha256 `1b7d6182…` |
| ANVIL best, committed HEAD fc23d9a (`anvil-sparse-rans`) | 143,132 | 0.511 | measured, `git show HEAD:tests/benchmark-suite.csv` (20,000-B file, 25 rows) |
| ANVIL best, uncommitted worktree grid C70179EA (`hotop-rlzp-rans`) | 140,898 | 0.503 | measured; row exists ONLY in the uncommitted worktree `tests/benchmark-suite.csv` (sha256 C70179EA…) |
| brotli q6 | 120,669 | 0.431 | measured, same file in BOTH grids; reproduced exactly by python-brotli 1.1.0 |
| **colref prototype `ref_field`** | **89,877** | **0.321** | **measured prototype wire, roundtrip OK** |
| xz -9e `--delta=dist=14` (same-transform control) | 89,564 (gate) / 89,556 (reproduced, Δ8 B framing) | 0.320 | dual-bar reference (research-gate) + my reproduction |
| brotli q11 + delta14 (same-transform control) | 80,650 | 0.288 | dual-bar reference, reproduced exactly |

`ref_field` is 30,792 B (25.5%) below brotli q6 and 36.2%/37.2% below the ANVIL rows — but
**under the dual-bar rule (same-transform reference controls, DNR E1) it is NOT ahead:**
it **ties xz -9e --delta=14** (89,877 vs 89,564, i.e. 313 B / 0.35% larger) and **loses to
brotli q11+delta14 by 9,227 B (10.3%)**. Integrated it would be FRONT-GAP (dual-bar), not a
crossing. research-gate verified the ladder and ruled **NOVELTY: NO** (copy-with-edits +
columnar/delta + context modeling; P and P-detection are infrastructure; correction-topology
coding is a different, live family). Build as **adopt-class capability**, per
`docs/gate-ruling-i9-datastruct-ts.md` + ledger PART XIV S5. The mechanism is a record-period
reference copy with per-column, reference-conditioned residual models — placement inside the
reference, no global lane transpose.

**Structure erratum (source-grounded, accepted by coordinator):** the task brief and
`docs/swarm-i8-strategy.md` §2.7c said "10,000 x 28-byte records (u64/f64/u64/u32)".
The file is **20,000 x 14-byte records**: `<Q f H` = u64 ts_ms (+1000±50), f32 value
(random walk ±0.1), u16 id = i mod 1000. The 28-B reading is stride-28 aliasing
(28 = 2x14) and parses a garbage f64 (1.16e232). True record period **P = 14**.
`synth-columnar-align.bin` true row period is **23 B** (12,000 rows), not 184; 184 = 8x23.
Evidence: byte-exact reconstruction from `tests/make_synth_corpus.py` (seed 90001) and
0/20,000 id mismatches under the 14-B parse (see `analyze_ts.py`).

---

## 1. Mechanism and placement

`prototypes/i9-datastruct/colref.py`. The decoder reconstructs record `i` by **copying
record `i-P`** (distance = the record period, a framing constant charged in the wire
header) and coding only corrections. Corrections are modelled **per byte column**
(offset within the record) and **conditioned on the reference byte at that column**.
Output stays row-interleaved; there is no global field transposition. This is the
placement the retired lane-transpose negative demands: the reference explains only the
changing fields, in place.

Per-field classes the encoder selects (all verified exact before use, parameters charged):
`const`, `posmod` (exact `v_i = phase + i*step mod m`, or exact linear when `m=0`),
`prev` (per-byte, context = reference byte), `delta` (per-byte `mod 256` vs reference),
`worddelta`. `P` is treated as infrastructure (xz `--delta` documents 1..256); **P
detection is not claimed as a mechanism.**

Wire = raw self-describing header (magic, mode, P, nrecords, field table + class
parameters) + one LZMA-style binary arithmetic stream. All bytes counted.
Totals include header and record 0.

Chosen classes on synth-timeseries P=14 layout `0:8,8:4,12:2`:
`ts(0:8)=prev`, `f32(8:4)=prev`, `id(12:2)=posmod(m=1000,step=1,phase=0)`.
`id = i mod 1000` is exactly position-derivable, so it costs 0 body bits (coordinator
directive; charged as a 24-B header parameter).

---

## 2. Measured end-to-end bytes (prototype wire)

Recompute: `python prototypes/i9-datastruct/verify_artifacts.py`
(encodes, writes `out/`, decodes the written file and byte-compares; prints the table).

| variant | mechanism | coder | bytes | ratio | sha256[0:16] | roundtrip |
|---|---|---:|---:|---:|---|---|
| ts.raw_o0 | none | order-0 adaptive | 207,197 | 0.7400 | 5c18df58668be296 | OK |
| ts.raw_o1 | none | order-1 adaptive (ctx = prev byte) | 187,436 | 0.6694 | 4e363fd244bb55b1 | OK |
| ts.ref_flat | reference copy | one global model ctx = ref byte | 107,564 | 0.3842 | e6595818ea138e79 | OK |
| ts.ref_col | reference copy | per-column model ctx = ref byte | 97,057 | 0.3466 | 2b89ccbeb43e64bf | OK |
| **ts.ref_field** | **reference copy + field classes** | **same coder** | **89,877** | **0.3210** | **56b60452d61b8416** | **OK** |

Also: `python prototypes/i9-datastruct/analyze_ts.py` (structure + per-column entropy
table; measured empirics).

Input sha256 = `1B7D618234F13418959A7DCE166D65F1D08C7018FD1E335B7360F07869417425`
(matches `tests/corpus/CHECKSUMS.txt`).
Reference reproduction: `python -c "import brotli;d=open('tests/corpus/synth-timeseries.bin','rb').read();print(len(brotli.compress(d,quality=6)))"` -> 120669.
ANVIL rows: committed HEAD best = `anvil-sparse-rans` 143,132 (0.511) in
`git show HEAD:tests/benchmark-suite.csv`; worktree-grid best = `anvil-hotop-rlzp-rans`
140,898 (0.503), present only in the uncommitted `tests/benchmark-suite.csv`
(sha256 C70179EA…). Both labelled measured-at-their-provenance.

---

## 3. Ablation — mechanism vs entropy coder (same coder family)

| step | bytes | delta | meaning |
|---|---:|---:|---|
| raw, order-0 coder | 207,197 | — | coder alone |
| raw, order-1 coder | 187,436 | **-19,761** | coder strength only |
| + reference copy, global model | 107,564 | **-79,872** | **mechanism (reference)** |
| + column identity | 97,057 | **-10,507** | columnar de-interleaving inside the ref |
| + field classes (incl. posmod id) | 89,877 | **-7,180** | field-aware residuals |

Separation: **the order-1 coder alone (187,436) is worse than brotli q6 (120,669).**
The mechanism (same coder, mechanism toggled off/on: 187,436 -> 97,057) is what moves
the byte axis. Coder-only gain is 19,761 B; mechanism gain is 97,559 B.

*Complementarity (derived, not measured):* with the mechanism but **no** entropy coding,
the exposed residual alphabet is the non-constant column bytes — 4 (ts low) + 4 (f32) =
**8 B/record = 160,000 B** (plus a per-column change mask), i.e. still above q6. Neither
component alone reaches the result; the measured win is the composition.

*Floor context (derived):* the empirical per-column order-1 entropy sum is 22.2 bits/record
(~55,300 B with id zeroed). Counted wire 89,877 is **1.62x that floor** — the gap is
adaptive-model warmup/redundancy, chiefly the 256-context f32 byte models. The floor
itself is a downward-biased empirical estimate; treat as approximate, not achievable.

---

## 4. Capability gap — `synth-columnar-align.bin` (P=23, not P=184)

Measured with the same prototype (recompute: `verify_artifacts.py`):

| variant | bytes | ratio | notes |
|---|---:|---:|---|
| ca.ref_col (byte columns, no field identity) | 69,748 | 0.2527 | needs only P |
| ca.ref_field_naive (4-B word partition) | 44,837 | 0.1625 | wrong field boundaries |
| ca.ref_field_p184 (8-row = 184-B framing) | 35,808 | 0.1297 | true fields, aliased period |
| ca.ref_field_true (P=23, true fields) | 24,055 | 0.0872 | supplied period + layout |
| xz -9e `--delta=dist=23` (same-transform control) | 21,340 (gate) / 21,332 (reproduced, Δ8 B) | 0.0773 | dual-bar control; **24,055 > 21,340** |
| brotli q6 (reference) | 116,829 | 0.4233 | python-brotli 1.1.0 |
| brotli q11 / zstd-19 (reference) | 105,915 / 130,310 | 0.384 / 0.472 | python-brotli / python-zstandard |
| README historical "anvil default" | 118,089 | 0.4279 | old snapshot, not re-measured |

Chosen classes at P=23 true layout: `0:2 posmod, 2:4 posmod, 6:8 posmod, 14:4 prev,
18:2 posmod, 20:1 posmod, 21:1 prev, 22:1 prev`. **Dual-bar standing:** even the
supplied-perfect-layout result (24,055) is above xz -9e `--delta=23` (21,340) — the
same-transform reference wins here too, so the columnar-align cell is capability-only,
never a crossing.

**The gap is discovery, not coding:**
1. **Period aliasing.** 276,000 = 1,500 x 184 = 12,000 x 23. At stride 184 there are
   56 "constant" offsets (30.4%); at the true stride 23 there are **5** (21.7%). The 56
   are an artifact — e.g. ts_ns advances +8,000,000 per 8-row group, and 8,000,000 ≡ 0
   (mod 256), so the low ts byte looks constant. A detector that reports 184 as the
   fundamental period is reading aliasing, not field structure. The minimal period must
   be validated.
2. **Field boundaries are misaligned.** Field offsets 0,2,6,14,18,20,21,22 (widths
   2,4,8,4,2,1,1,1); only offset 0 is word-aligned. The measured cost of getting the
   partition wrong is +20,782 B (naive 4-B words vs true fields). The mechanism needs
   the partition supplied; **auto-discovering it from local copy residuals — inside the
   reference, without a global transpose — is unsolved.** This is the open capability gap.
3. **{synthetic} caution (binding).** Our generator stores *exact* integer arithmetic
   progressions (row_id, seq, ts_ns, volt, status are exact `posmod`; temp is ±1 noise).
   The 0.087 ratio is a property of the generator, not an expected value on real columnar
   data.

---

## 5. Honest limitations

- **Prototype wire, not an ANVIL row.** The coder here is a Python bit-level binary
  arithmetic coder, not ANVIL's rANS/token machinery. Framing, parser, and per-stream
  costs of a native integration are unmeasured. No crossing language; any corridor
  arithmetic is a projection for arch to cost, not a result.
- **Team blocker in force.** `format` reports the current dirty-tree `anvil.exe` cannot
  decode (arch's in-progress `decode_one_block` refactor); no roundtrip-dependent byte
  claim from the dirty build. The ANVIL 140,898 B cited here is the **uncommitted
  worktree-grid** row; the committed-HEAD best on this file is 143,132 B. Neither is a
  fresh run by this lane.
- **Throughput not measured** (bytes only). The Python prototype says nothing about
  encode/decode speed.
- `posmod`/`const` are exactness gates; `prev` dominates where residual noise exists.
  The class *selector* uses an entropy proxy, so class choice is not proven optimal.

---

## 6. Handoff to `arch` (representation feature: columnar residual contexts for copy tokens)

Measured on the target file: a copy token at distance = record period P plus residual
models conditioned on (offset within record, reference byte) turns 187,436 B (order-1
coder alone) into 97,057 B, and field classes add another 7,180 B -> **89,877 B**
(prototype wire). Proposed native shape (for arch to accept, cost, or reject):
- when a match distance is a multiple of a block-header record period P (P is
  infrastructure, transmitted/counted, not novelty), route the **correction values of
  that copy token** to per-column contexts instead of a flat correction stream;
- keep the correction model *inside the token* (the decoder already holds the reference
  bytes), so no pre-LZ transpose is introduced;
- field classes (`const`/`posmod` exact, `prev`/`delta` residual) are a candidate
  extension, but the field partition must be discovered locally or supplied; the
  `synth-columnar-align` measurements show partition sensitivity (+86% for a 4-B naive
  partition).

This is a **measured capability + placement constraint**, not a novelty claim.
**The mechanism is not "solved": it is measured with P and the partition supplied.
Until discovery of P and the field partition is automatic and measured, treat this as
an open capability gap, not a representation that can be integrated.**

### 6.1 Integration spec (for arch to accept, cost, or reject)

Callsites (read-only inspection of the current `src/anvil.cpp`; arch owns edits):
- mode 12 SHAPE correction emission — `resid.push_back(t.val[k])` (~L2156), streams
  `types/ll/ml/ds/lits/masks/resid` (~L2139);
- mode 14 TCOPY — `resid.push_back(t.val[k])` (~L2548), 8 streams incl. `resid`
  (~L2530); note TCOPY's 4-aligned transform windows cannot address misaligned fields
  (offsets 2,6,14,18 in columnar-align), whereas columnar residuals are byte-indexed;
- mode 15 HOTOP macro correction emission — `mresid.push_back(t.val[k])` (~L2784).
In each case the token's match distance and each correction's offset `t.off[k]` within
the token are in scope at emission, and the reference bytes are already in the output
window (`out[token_start - dist + t.off[k]]`).

Wire semantics (no new token type required; two separable options):
1. **Context change only** — keep the same residual stream bytes, key the residual
   model on `(col, ref_byte)` where `col = t.off[k] % P`, `ref_byte` = the byte at
   `token_start - dist + t.off[k]`. `dist % P == 0` is the token trigger; for
   `dist == P` this is exactly "column of the previous record".
2. **Stream split** — emit residuals into P sub-streams indexed by `col` (per-stream
   suite/rANS per column). More framing (P stream lengths), simpler models.
Option 1 is the minimal wire change; option 2 is likely the faster decode and the
better rANS fit. Field classes (`const`/`posmod` exact, `prev`/`delta` residual) are a
further extension that changes the block header.

Transmitted (charged): `P` per block (uvarint); a token/block flag enabling the column
context; if field classes are used, the partition `(off,width,class)` list and exact
class parameters (`const` value; `posmod` m/step/phase). Nothing else.
Derived (zero bits): token trigger from `dist mod P`; `col = t.off[k] % P`; the
reference byte from the already-decoded output; correction values from the existing
residual stream. For `posmod` fields, the value is a function of the record index
(`i = token_start / P`), exactly as `id = i mod 1000` here.
Placement invariant: the model lives at the copy token, using bytes already decoded;
no pre-LZ transpose, no global re-layout, so contiguous phrase structure is untouched.

Measured sensitivity that makes discovery the binding gap: true field partition
(P=23, offsets 0,2,6,14,18,20,21,22) = 24,055 B; naive 4-B word partition = 44,837 B
(+86%); byte-columns with no field identity = 69,748 B. A partition-discovery pass must
run inside the reference (from local copy residuals), and P must be validated at the
minimal period (184 = 8x23 aliasing, §4).

---

## 7. Recompute commands (exact)

```powershell
. .\env.ps1
python prototypes\i9-datastruct\analyze_ts.py
python prototypes\i9-datastruct\verify_artifacts.py
python prototypes\i9-datastruct\colref.py verify tests\corpus\synth-timeseries.bin prototypes\i9-datastruct\out\ts --all-modes --period 14 --layout "0:8,8:4,12:2"
```

Dual-bar same-transform controls (reproduced by datastruct; +8 B vs the gate's xz CLI
framing on the xz rows):

```powershell
python -c "
import lzma, brotli
def delta(d,p):
    b=bytearray(d)
    for i in range(len(d)-1,p-1,-1): b[i]=(d[i]-d[i-p])&0xFF
    return bytes(b)
for f,p in (('synth-timeseries.bin',14),('synth-columnar-align.bin',23)):
    d=open('tests/corpus/'+f,'rb').read(); dd=delta(d,p)
    print(f, 'xz9e+delta', len(lzma.compress(dd,format=lzma.FORMAT_XZ,preset=9|lzma.PRESET_EXTREME)),
          'br11+delta', len(brotli.compress(dd,quality=11)))
"
```

Expected: ts xz9e+delta14 = 89,556 (gate 89,564), br11+delta14 = 80,650 (matches gate
exactly); ca xz9e+delta23 = 21,332 (gate 21,340).

Roundtrip is asserted inside every command (fail -> nonzero exit). Wire files written to
`prototypes/i9-datastruct/out/`.
