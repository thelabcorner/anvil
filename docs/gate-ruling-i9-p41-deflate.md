# GATE RULING I9-5 — P4.1 DEFLATE reconstruction: verified GO direction, adopt-class, NO novelty

**Author:** `research-gate`. **Date:** 2026-09-12. **Swarm:** `anvil-i9-pareto`.
**Requester:** coordinator `msg_bc24a94818cd4fc9b5a3293a0c4d50fa`; submission
`deflate` (`msg_8ec7bbbb…` prior art, `msg_398a3ceb…` result,
`msg_55830e01…` reproduction package).
**Status: REPRODUCED (census/replay/scope/artifacts). GO recorded as DIRECTION;
prototype wire now roundtrips on the green binary (update V-3) — compression
claim still PENDING src integration + fuzz. NOVELTY: NO.**

Artifacts: `prototypes/i9-deflate/{RESULTS.md, PRIOR-ART-NOTE.md, README.md}`
+ `results/` (untracked worktree; source-corpus `mozilla` sha256
`657fc3764b0c75ac…`). Build: `build\anvil.exe` sha256 prefix **`DA24665C`**
(1,490,432 B) — the dirty-tree binary that **encodes but cannot decode**.

## V-1. Independent reproduction (this gate)

```powershell
python prototypes\i9-deflate\census.py scratch\ratio-first\corpora\silesia\mozilla --out-dir <tmp>
python prototypes\i9-deflate\replay.py <tmp>\census-mozilla.json --src scratch\ratio-first\corpora\silesia\mozilla --out-dir <tmp>
python prototypes\i9-deflate\census.py scratch\ratio-first\corpora\silesia\{sao,ooffice,samba} --out-dir <tmp>
```

| quantity | claimed | recomputed | status |
|---|---|---|---|
| mozilla detected / DEFLATE / stored | 2,354 DEFLATE | **2,564 detected = 2,354 DEFLATE + 210 stored** | VERIFY, reconciled (V-2a) |
| mozilla DEFLATE C / U | 3,177,007 / 9,991,436 | **3,177,007 / 9,991,436** | VERIFY |
| mozilla replay attempted | 2,354 | **2,354** | VERIFY |
| replay valid / diff | 2,331 / 23 | **2,331 / 23** | VERIFY |
| replay valid fraction of bytes | 91.2% | **0.91222084** | VERIFY |
| mozilla local ceiling (brotli q11 lw24) | 1,459,509 | **1,459,509** (`summary-mozilla.json`) | VERIFY |
| charged recovery (sizes on disk) | 13,806,173 -> 12,443,996 = 1,362,177 | **13,806,173 -> 12,443,996 = 1,362,177** (`mozilla-anvil-brotli.anv`, `transformed-mozilla-anvil-brotli.anv`) | VERIFY (artifact sizes) |
| sao / ooffice DEFLATE streams | zero | **0 / 0** | VERIFY |
| samba DEFLATE C / ceiling | 411 KB / 193 KB | **411,393 / 193,463** (`census-samba-ceiling.json`) | VERIFY |

## V-2. Corrections (binding)

- **a. Stream-count reconciliation — my earlier "conflation" correction is
  WITHDRAWN (it was wrong in substance; the rule binds its author).** deflate's
  convention A2: the census detects **2,564** streams = **2,354 DEFLATE + 210
  stored** (`stored_clen` 138,045); replay attempts exactly the 2,354 DEFLATE
  streams and validates 2,331. So "2,354 verified DEFLATE streams" was correct
  for DEFLATE streams, and "2,564 verified" was correct for all detected
  streams — different scopes, both right. Canonical quote: **"2,564 detected
  (2,354 DEFLATE + 210 stored); replay 2,354 attempted / 2,331 valid"**.
- **b. sao+ooffice untouchable = 212,047 B**, not 234,047 (the gap shares are
  sao 160,422 + ooffice 51,625 = 212,047 of the 664,307-B residual; mozilla
  429,893, samba 22,367). P4.1 addressable total = **1,652,972 B**
  (mozilla ceiling 1,459,509 + samba 193,463). The 234,047 figure in the first
  handoff is a slip and is struck.

## V-3. Compression claim status (PENDING)

**UPDATE (deflate, post-decode-fix re-verification).** On the green binary
`8EAE1FB3` (src `62BC6631`, HEAD fc23d9a; same source as the canonical
`0D1E130B`) both encodes reproduce byte-identically
(13,806,173 / 12,443,996; `.anv` sha256 `175C6861…` / `B3954DFC…`, identical to
the `DA24665C` run) and **both `.anv` decode rc=0 sha256-exact** (original ->
mozilla `657FC376…`; transformed -> `B3E16040…`), with the prototype inverse
chain sha256-identical. So the **prototype wire roundtrip is now verified**.
The figure is nevertheless **still not citable as an ANVIL result**: it needs
`arch`'s `src/anvil.cpp` integration behind a flag (byte-identity gate on
existing paths) plus fuzz. Status: **GO direction; prototype roundtrip verified;
compression claim PENDING integration + fuzz.**
**Canonical-binary addendum:** both encodes were re-run on canonical
`0D1E130B` and are **byte-identical** (`.anv` sha256 `175C6861…` /
`B3954DFC…`); since the decoder is deterministic, the sha256-exact decode
verification transfers to the canonical artifacts (`*-0D1E130B.anv` under
`prototypes/i9-deflate/results/`).
**E8AA2E48 addendum (deflate, `msg_cec65d31`):** both existing archives decode
**sha256-exact on canonical `E8AA2E48`** (dec-orig == mozilla `657FC376…`;
dec-transformed == `B3E16040…`); byte totals unchanged via the 494/494
owning-lane encoder-identity gate; citation adds *"decode-verified on canonical
E8AA2E48; byte totals unchanged (encoder identity 494/494)"*. The two-encode
re-run is redundant for the record.
**sha-chain extension (deflate, `msg_a43d1832`):** both archives also decode
sha256-exact on the newest canonical **`72D65150`** (frozen src `BDC90474`),
and the byte totals are unchanged across
`8EAE1FB3 / 0D1E130B / E8AA2E48 / 72D65150` via the 494/494
encoder-wire-identity gates.

## V-4. NOVELTY RULING — NO mechanism-level novelty (adopt-class infrastructure)

DEFLATE reconstruction is a mature published family. Lineage (from the
lane's note, consistent with `docs/gate-priorart-audit-i8.md` §3's settled-ANS
and copy-with-edits records): **precomp** (schnaader; container scan +
same-encoder recompress + bit-exact accept — the valid mode prototyped here),
**preflate** (D. Steinke; parameter prediction + compact corrections for
unknown encoders), **preflate-rs** (Microsoft; multi-encoder detection +
CABAC corrections; production use), **reflate**, **grittibanzli**, plus
container-specific recompression (zopflipng-class, ZIP recompressors).

- Core mechanism, valid/diff/brute split, parameter transmission, corrections:
  **not different**.
- The corpus observations (99.0% of mozilla streams at 2 parameter bytes;
  23/2,354 unknown-encoder streams) are **measurements**, not mechanism.
- The one candidate ruling item — *portfolio composition* (first ANVIL mechanism
  that wins bytes on BWT-catastrophic files while preserving fast decode) — is a
  **routing/portfolio property, not a compression mechanism**; precedent: the
  K≈8–12 context quantizer kept as enabling infrastructure with no novelty
  claim. **Not a novelty position.**

**Disposition:** record P4.1 as **prior-art-deployed infrastructure
(adopt-class), no novelty claim**. If integrated: clean-room implementation
against zlib as prototyped; **do not import preflate-rs code** (license and
clean-room rule). Cite the lineage once in any integration note.

## V-5. Dispositions (one line each)

| # | claim | disposition |
|---|---|---|
| P41-1 | mozilla census C/U 3,177,007 / 9,991,436 | **VERIFY** |
| P41-2 | "2,354 verified DEFLATE streams" | **VERIFY (my correction WITHDRAWN)** — 2,564 detected = 2,354 DEFLATE + 210 stored; replay 2,354 attempted / 2,331 valid (V-2a) |
| P41-3 | replay valid 2,331/2,354 = 91.2% bytes | **VERIFY** |
| P41-4 | local ceiling 1,459,509 | **VERIFY** |
| P41-5 | recovery 13,806,173 -> 12,443,996 = 1,362,177 | **VERIFY artifact sizes + prototype roundtrip** (8EAE1FB3, sha-exact); not citable until src integration + fuzz |
| P41-6 | sao+ooffice zero DEFLATE; untouchable 212,047 | **VERIFY** / 234,047 struck |
| P41-7 | samba 411,393 C / 193,463 ceiling; addressable total 1,652,972 | **VERIFY** |
| P41-8 | mechanism novelty (valid-mode reconstruction) | **REJECT** — prior art (precomp/preflate/preflate-rs/reflate); adopt-class infrastructure |
| P41-9 | portfolio-composition property as a claim | **REJECT as novelty** — routing property (K-quantizer precedent) |
| P41-10 | "GO verdict" | **VERIFY as DIRECTION** — compression claim PENDING roundtrip (V-3) |

**Binding citation string (v3):**
> "P4.1 prototype wire: 1,362,177 B recovery on mozilla; wire roundtrip verified
> sha256-exact on green binary 8EAE1FB3, canonical binary 0D1E130B verified
> byte-identical (integration + fuzz PENDING); local ceiling 1,459,509 B;
> adopt-class prior art (precomp/preflate), no mechanism novelty; addressable
> total 1,652,972 B (sao+ooffice zero DEFLATE)."

---

*Ruled by `research-gate`. Ledger: PART XIV addendum. The recovery number is not
a result until it roundtrips.*
