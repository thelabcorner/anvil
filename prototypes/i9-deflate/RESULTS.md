# P4.1 — Bit-exact DEFLATE reconstruction: feasibility result (GO)

**Lane:** `deflate` (swarm `anvil-i9-pareto`, I9). **Date:** 2026-09-12.
**Status:** prototype wire — **not an ANVIL row**; no `src/` integration (arch owns that).
**Verdict: GO.** The mozilla go/stop bar (>~430 KB plausibly recoverable on mozilla
alone) is cleared by a **measured, overhead-charged 1,362,177 B** end-to-end
(3.17× the bar) and a **1,459,509 B** region-local ceiling.

> **Citation form (binding, per research-gate ruling + coordinator 2026-09-12):**
> "P4.1 prototype wire: 1,362,177 B recovery on mozilla, **encode-only, roundtrip
> PENDING (decode-fix gated)**; local ceiling 1,459,509 B; adopt-class prior art
> (precomp/preflate), no mechanism novelty; addressable total 1,652,972 B
> (sao+ooffice zero DEFLATE)." The recovery becomes a citable result only after
> the integrated path roundtrips byte-exact and fuzzes green.

---

## 0. Scope finding (checked first, because it bounds everything)

Census over the four files named by the brief (full decompression + checksum
verification of every reported stream):

| file | DEFLATE streams | deflate C (B) | plaintext U (B) | C/U | stored entries | where |
|---|---:|---:|---:|---:|---:|---|
| **mozilla** | 2,354 (2,310 ZIP method-8 + 44 PNG IDAT) | **3,177,007** | 9,991,436 | 0.318 | 210 zip stored, 138,045 B | 19 ZIP archives inside a TAR (843 members): `chrome/*.jar` + XPI |
| **samba** | 160 (raw zlib, FlateDecode in 2 PDFs) | 411,393 | 1,292,412 | 0.318 | 0 | `docs/Samba-HOWTO-Collection.pdf` (86), `docs/textdocs/kurs.pdf` (74) |
| **sao** | **0** | 0 | 0 | — | 0 | fixed-width star-catalog binary |
| **ooffice** | **0** | 0 | 0 | — | 0 | MZ/PE binary; no ZIP/gzip/zlib/CAB-MSCF found |

**Consequence (handoff to strategy + research-gate):** P4.1 **cannot touch
sao/ooffice (212,047 B of the 664,307-B landscape gap: sao 160,422 + ooffice
51,625)** — they contain no DEFLATE at all. It is a **mozilla-first** mechanism,
with samba (411,393 B deflate, local ceiling gain 193,463 B) as a small second
target. This is a scope fact, not a mechanism failure.

**Addressable headroom by P4.1** (local ceilings): mozilla 1,459,509 + samba
193,463 = **1,652,972 B ≈ 1.65 MB**; untouchable: sao+ooffice **212,047 B**.
(Note: 234,414 B is the sao+ooffice+samba group; samba *is* addressable.)

Coverage caveat: detection is signature/container based (ZIP local/central,
gzip members, zlib headers, PNG IDAT, CAB MSZIP). A raw DEFLATE stream with no
wrapper and outside a recognized container is not signature-detectable; none
was counted. All reported streams were decompressed in full and CRC-32
(ZIP) / Adler-32 (zlib/PNG) verified — 100 % verified.

**Count convention (research-gate correction A2):** quote both numbers. mozilla
has **2,564 detected streams = 2,354 DEFLATE** (2,310 ZIP method-8 + 44 PNG
IDAT) **+ 210 ZIP stored**; the replay pass attempts all **2,354 DEFLATE**
streams. The other three files: samba 160 DEFLATE / 160 detected; sao 0/0;
ooffice 0/0.

---

## 1. Exact replay ("valid" mode) — measured

`replay.py` searches zlib parameter space per stream
(wbits ∈ {-15, 15} × level 1–9 × memLevel 1–9 × strategy 0–4, 405 combos worst
case) and compares **bit-for-bit** against the original payload.

| mozilla | streams | share | deflate bytes | share | plaintext |
|---|---:|---:|---:|---:|---:|
| valid (bit-exact replay found) | **2,331** | 99.0 % | **2,898,132** | **91.2 %** | 8,689,786 |
| diff (unknown encoder, corrections needed) | 23 | 1.0 % | 278,875 | 8.8 % | 1,301,650 |
| brute (no replay possible) | 0 | — | 0 | — | — |

Parameter distribution: 2,282 × `level 6 / memLevel 8 / Z_DEFAULT / raw`,
7 × `level 6 / memLevel 9 / raw`, 42 × `level 6 / memLevel 8 / Z_FILTERED /
zlib-wrapped` (the PNGs). Parameters are near-zero-entropy; 2 bytes/stream.

**Bounded region** (required by the task): `mozilla/chrome/en-US.jar`
(`zip@156160`) — 464 streams, 575 KB deflate, 1.89 MB plaintext:
462 valid (99.6 % of streams / 92.2 % of bytes), 2 diff
(`locale/en-US/help/help-toc.rdf`, `mail_help.html` — no zlib-1.3.1 setting
reproduces these two; they are the unknown-encoder class).

The 23 diff streams are the honest unknown-encoder population: a preflate-class
predictor/correction coder is required for them; we did not implement one
(bounded prototype). They are charged as **brute** (payload kept verbatim) in
every number below.

---

## 2. Ceiling and measured end-to-end

### 2.1 Region-local ceiling (all 2,354 streams; labelled)

Concatenated original payloads vs concatenated plaintexts, compressed with the
reference codecs (Python bindings; lgwin 24 for Brotli here):

| codec | cost(deflate.bin) | cost(plain.bin) | gain | % of deflate bytes |
|---|---:|---:|---:|---:|
| brotli q11 | 3,041,864 | 1,582,355 | **1,459,509** | 48.0 % |
| xz -9e | 3,064,680 | 1,592,612 | 1,472,068 | 48.0 % |
| zstd 22 | 3,054,524 | 1,620,772 | 1,433,752 | 46.9 % |

(Valid-stream subset only: C 2,898,132 → gain 1,360,459 by the same method.)

### 2.2 Charged end-to-end transform (measured; the headline number)

`transform.py` replaces each valid stream's payload **in situ** with its
plaintext and appends a side table (offset/clen/ulen varints + 2 param bytes
per stream). The decoder replays each plaintext with the stored parameters.

* transformed file: 57,034,135 B (from 51,220,480 B)
* **inverse verified byte-identical: sha256 657fc376… == source sha256** (the
  prototype performs the full container-inverse + zlib replay in-process)
* side table charged: 21,985 B raw / 14,199 B brotli-q11 (conservative — in a
  container-aware decoder, offsets/clen/ulen are derivable from the ZIP central
  directory, so only ~2 param bytes/stream are genuinely informative)

ANVIL measurements, current binary `build\anvil.exe` sha256 `DA24665C…`,
`--parse=ratio --ratio-backend=brotli --ratio-context=off --ratio-lines=off`
(encode-only — see caveats):

| input | output bytes | elapsed |
|---|---:|---:|
| original mozilla | 13,806,173 | 129.6 s |
| transformed mozilla | **12,443,996** | 136.4 s |
| **recovered** | **1,362,177 B** | |

* Binding per-file reference (mozilla, xz -9e) = **13,376,248 B**
  (`tests/ratio-first-standard.csv`). The transformed representation beats it
  by **932,252 B** — i.e. this single mechanism closes mozilla's 429,893-B
  share of the landscape gap with 3.2× margin.
* Consistent with §2.1: end-to-end 1,362,177 vs valid-subset local 1,360,459
  (the whole-file codec recovers a little cross-entry redundancy; the side
  table and the 23 brute streams are charged).

### 2.3 Diff-mode upside ([PROJECTION] — no correction coder implemented)

For the 23 unknown-encoder streams (C 278,875, U 1,301,650): their plaintext
costs 206,302 B brotli-q11. With preflate-rs's **published** correction
overheads (percentage of uncompressed data) applied to this population:

| scenario | incremental gain vs brute |
|---|---:|
| zero corrections (unattainable bound) | +72,443 |
| preflate-rs zlib-class 0.01 % (U) | +72,443 − 130 = +72,313 |
| preflate-rs miniz-class upper 2.70 % (U) | +37,429 |

Realizing even the pessimistic case lifts mozilla recovery to ≈ **1.40 MB**.
Label: PROJECTION (prior-art correction model applied to our measured stream
population), not measured.

### 2.4 Samba (local ceiling only)

160 PDF FlateDecode streams: C 411,393, U 1,292,412; brotli gain 193,463 B
(47.0 % of deflate bytes). Same replay machinery applies (these matched the
same zlib family in spot checks); not end-to-end measured. Not part of the
mozilla bar.

---

## 3. Strict zero-bit derivation analysis (handoff §2.95 test)

| item | verdict |
|---|---|
| DEFLATE bitstream body | **exactly derived** by deterministic replay from plaintext + encoder parameters; 0 transmitted bits beyond params |
| replay parameters (level/memLevel/strategy/window) | **must be transmitted** (~2 B/stream). Not derivable: the decoder has no reference bitstream to check candidate params against (circular). Genuinely informative, ~0 entropy |
| zlib wrapper (2-B header + 4-B Adler-32) | **derived** (Adler is a function of plaintext; header follows from level) |
| stream offsets / clen / ulen | **derivable** from container structure (ZIP CD) in a container-aware decoder; prototype charges them (upper bound) |
| plaintext itself | transmitted (replaces the compressed payload); its cost is the strong-codec rate, not zero |
| corrections (23 unknown-encoder streams) | **not derivable**; either transmitted (preflate-class) or brute fallback (what we charged) |

---

## 4. Decode-side cost (measured, Python; C projection)

Replaying all 2,331 valid streams in-process: **8.69 MB plaintext in 0.194 s**
(44.7 MB/s including Python per-stream overhead). ANVIL's recorded mozilla
decode is 0.504 s (`tests/auto-routing.csv`, auto/Brotli). So replay is a real
but bounded decode cost (order +0.1 s in C, implementation-dependent);
it does **not** carry BWT's 11× decode penalty. This is the mechanism's
strategic point: the target files are Brotli/xz-routed with fast decode.

---

## 5. Falsification status / caveats (all explicit)
1. **Prototype wire, not an ANVIL row.** No `src/anvil.cpp` change (arch's lane).
2. **ANVIL-integrated roundtrip not yet run** — the team's current dirty-tree
   binary fails decode (format's `decode_one_block` blocker). All ANVIL byte
   figures above are **encode-only** on `DA24665C…`. The prototype's own
   container-inverse + replay is sha256-verified in-process, which is the
   roundtrip evidence for this deliverable.
3. Single encode run per input; byte counts are deterministic, timing is not
   the claim.
4. Side-table bytes are charged conservatively (see §3).
5. Raw-DEFLATE detection outside wrappers/containers is out of signature
   scope; no such bytes are claimed.
6. Diff-mode numbers are projections from published preflate-rs overheads.
7. `ooffice` MZ/CAB scanning found no MSCF cabinet; if it uses another
   compressed container, the scanner did not see it (census limitation noted).

## 5.5 Post-decode-fix verification (2026-09-12, format gate green)

format's decode-fix gate landed green on `build-i9-format\anvil.exe` sha256
`8EAE1FB327AD759B1CF80BE765B59FA7EFB8AB843F908341AC2283652BFDD105`
(src/anvil.cpp `62BC6631…`, HEAD fc23d9a). Re-ran both encodes and both decodes
on that fixed-source binary:

| step | result |
|---|---|
| encode original mozilla | 13,806,173 B (identical to DA24665C run) |
| encode transformed | 12,443,996 B (identical to DA24665C run) |
| decode `mozilla-anvil-brotli-8EAE1FB3.anv` | rc=0, sha256 == mozilla source (`657fc376…`) |
| decode `transformed-mozilla-anvil-brotli-8EAE1FB3.anv` | rc=0, sha256 == `transformed-mozilla.bin` (`B3E16040…`) |
| chain T → original (prototype inverse + zlib replay) | sha256 identical (transform.py `roundtrip identical=True`) |

So the codec leg and the reconstruction leg are each byte-verified; the
**fully integrated** path (ANVIL decoder invoking the replay) still needs arch's
integration + fuzz before the recovery figure is citable.

**Canonical-binary confirmation (coordinator-published `build\anvil.exe` sha256
`0D1E130BA2FBC43ACCE46E423BDD2AE732A2CA56A419B45929E26A6487C1DDAB`, same src
`62BC6631…`):** both encodes reproduce the same sizes (13,806,173 / 12,443,996)
and the resulting `.anv` sha256s are **identical** to the 8EAE1FB3 run
(`175C68610E213BB7…` / `B3954DFC4E57C5B8…`). Because the canonical archives are
byte-identical to the decode-verified green archives, the sha256-exact decode
result transfers to the canonical artifacts.

Status per research-gate V-2a/V-3/V-5: **wire roundtrip VERIFIED**; compression
claim **PENDING src integration + fuzz**. Binding citation: "P4.1 prototype wire:
1,362,177 B recovery on mozilla; wire roundtrip verified sha256-exact on green
binary 8EAE1FB3 (integration + fuzz PENDING; canonical binary verified
byte-identical); local ceiling 1,459,509 B; adopt-class prior art
(precomp/preflate), no mechanism novelty; addressable total 1,652,972 B
(sao+ooffice zero DEFLATE)."

**Canonical update to src `38409E26` / `build\anvil.exe` `E8AA2E48` (MAT
reverted, ALLOC-only decoder, 2026-09-12):** the decode-path source moved, so the
P4.1 artifacts were re-verified against it: both existing `.anv` archives (built
by `0D1E130B`) decode rc=0 **sha256-exact** on the new canonical decoder
(`dec-orig-E8AA2E48.bin` sha `657FC3764B0C75AC…` == mozilla;
`dec-transformed-E8AA2E48.bin` sha `B3E16040501E5DEA…` == `transformed-mozilla.bin`).
**Compressed-byte totals are unchanged** per the owning-lane encoder-identity gate
(arch: encode identity 494/494; coordinator: wire identical), so the two-encode
re-run is redundant for the record; it is deferred to the first quiet byte slot
because timing windows are currently continuous. The sha move is therefore
covered by encoder identity + fresh decode verification.

Current-binary mapping: canonical at time of writing = `E8AA2E48` (src `38409E26`);
the `0D1E130B`/`8EAE1FB3` figures above were produced and decode-verified on the
same source line (`62BC6631`) and are byte-valid for `E8AA2E48` per the encoder
identity gate.

**Canonical update to frozen src `BDC90474` / `build\anvil.exe` `72D65150`
(ALLOC + leg 4 retained, 2026-09-12):** both P4.1 archives again decode rc=0
**sha256-exact** (`dec-orig-72D65150.bin` == mozilla; `dec-transformed-72D65150.bin`
== `transformed-mozilla.bin`). Byte results from the earlier shas carry by the
494/494 encode-wire-identity gate (originating sha labelled).

## 6. GO/STOP verdict

**GO.** Bar: mozilla mechanism must plausibly recover >~430 KB on mozilla alone.
* ceiling (all streams) **1,459,509 B**
* charged end-to-end **1,362,177 B** (3.17× bar), and it beats xz on mozilla
  by 932,252 B.

Recommended next step (not this lane's call): arch stages the transform behind
a flag with a byte-identity gate; format documents the container-mode wire;
bench adds the row only after the decode fix lands and two hash-identical
arbiter runs pass.

**Prior-art note:** `PRIOR-ART-NOTE.md` (handed to research-gate). This is
precomp/preflate-class established prior art — no mechanism-level novelty
claimed; the deliverable is deployment value in ANVIL's portfolio (fast-decode
byte win) plus the measured mozilla economics.

## 7. Recompute commands (from repo root; `. .\env.ps1` first)

```powershell
# census (mozilla, samba, sao, ooffice)
python prototypes\i9-deflate\census.py scratch\ratio-first\corpora\silesia\mozilla --out-dir prototypes\i9-deflate\results
python prototypes\i9-deflate\census.py scratch\ratio-first\corpora\silesia\samba   --out-dir prototypes\i9-deflate\results
python prototypes\i9-deflate\census.py scratch\ratio-first\corpora\silesia\sao     --out-dir prototypes\i9-deflate\results
python prototypes\i9-deflate\census.py scratch\ratio-first\corpora\silesia\ooffice --out-dir prototypes\i9-deflate\results
# bounded-region replay (en-US.jar) and full replay
python prototypes\i9-deflate\replay.py prototypes\i9-deflate\results\census-mozilla.json --src scratch\ratio-first\corpora\silesia\mozilla --out-dir prototypes\i9-deflate\results --region "zip@156160"
python prototypes\i9-deflate\replay.py prototypes\i9-deflate\results\census-mozilla.json --src scratch\ratio-first\corpora\silesia\mozilla --out-dir prototypes\i9-deflate\results
# local ceiling
python prototypes\i9-deflate\ceiling.py prototypes\i9-deflate\results\census-mozilla.json --src scratch\ratio-first\corpora\silesia\mozilla --out-dir prototypes\i9-deflate\results
python prototypes\i9-deflate\ceiling.py prototypes\i9-deflate\results\census-samba.json   --src scratch\ratio-first\corpora\silesia\samba   --out-dir prototypes\i9-deflate\results
# charged transform + inverse verification (prints sha equality)
python prototypes\i9-deflate\transform.py prototypes\i9-deflate\results\census-mozilla.json prototypes\i9-deflate\results\replay-mozilla.csv --src scratch\ratio-first\corpora\silesia\mozilla --out-dir prototypes\i9-deflate\results
# end-to-end encode bytes (encode-only; decode blocked by the team decode bug)
.\build\anvil.exe c scratch\ratio-first\corpora\silesia\mozilla prototypes\i9-deflate\results\mozilla-anvil-brotli.anv --parse=ratio --ratio-backend=brotli --ratio-context=off --ratio-lines=off --quiet
.\build\anvil.exe c prototypes\i9-deflate\results\transformed-mozilla.bin prototypes\i9-deflate\results\transformed-mozilla-anvil-brotli.anv --parse=ratio --ratio-backend=brotli --ratio-context=off --ratio-lines=off --quiet
Get-Item prototypes\i9-deflate\results\mozilla-anvil-brotli.anv, prototypes\i9-deflate\results\transformed-mozilla-anvil-brotli.anv | Select-Object Name, Length
# consolidated summary
python prototypes\i9-deflate\analyze.py --results prototypes\i9-deflate\results --src scratch\ratio-first\corpora\silesia\mozilla --name mozilla
```

Artifact hashes (sha256 first 16) are in `README.md`.
