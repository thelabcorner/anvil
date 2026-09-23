# P4.1 prior-art note — DEFLATE reconstruction (for research-gate)

**Author:** `deflate` (I9). **Audience:** `research-gate` (novelty gate), `strategy`.
**Bottom line up front: NO mechanism-level novelty is claimed.** DEFLATE
reconstruction is a mature published family; P4.1 is deployment/enabling
infrastructure for ANVIL's portfolio (a byte win on fast-decode files), not a new
mechanism. Do not spend gate time defending it as novel; spend it only if the
portfolio-interaction framing below is judged worth a ruling.

## 1. Lineage (what exists)

| art | mechanism | status |
|---|---|---|
| **precomp** (schnaader, precomp-cpp, ~2010→current) | scan file for zlib/DEFLATE (also bzip2/GIF/JPG/MP3); decompress streams; **recompress with the same encoder and accept only if bit-to-bit identical**; emit PCF + plaintext to be compressed by a stronger codec; recursive stream handling | exact match for the "valid mode" we prototyped; precomp is the original demonstration |
| **preflate** (Dirk Steinke) | when same-encoder replay fails, *predict* the original encoder's parse decisions from estimated parameters and transmit compact corrections | the "diff mode" our 23 unknown-encoder streams would need |
| **preflate-rs** (Microsoft, 2023–2025) | modern Rust port; detects zlib, zlib-ng, libdeflate, miniz/miniz_oxide, Windows zlib; estimates hash/chain/nice-length/block-split params; CABAC-coded corrections; production cloud-storage use | the strongest current reference; published overhead table quoted in RESULTS §2.3 |
| **reflate**, **grittibanzli** | same problem class (listed as siblings by preflate-rs) | established |
| PNG recompression (e.g. `zopflipng`-class tooling), ZIP recompression tools | container-specific instances | established |

Our prototype's components map 1:1 onto this lineage: container scan ≈ precomp;
zlib parameter search + bit-exact check ≈ precomp valid mode; per-stream
parameter record ≈ preflate parameters; corrections for unknown encoders ≈
preflate. The zero-bit analysis (RESULTS §3) is the same accounting precomp
makes (the DEFLATE stream is regenerated, not stored).

## 2. What is (and is not) different here

**Not different:** the core mechanism, the valid/diff/brute mode split, the
container scan, parameter transmission, corrections for unknown encoders.

**Different at most in engineering/portfolio terms — not gate-grade:**
1. **Valid-mode sufficiency on this corpus.** 99.0 % of mozilla streams
   (91.2 % of bytes) need only 2 parameter bytes — no correction coder at all.
   That is a *measurement*, not a mechanism: this corpus's jars were zlib-level-6
   produced, exactly the case precomp targets.
2. **Unknown-encoder population.** 23/2,354 streams are not reproducible by any
   zlib-1.3.1 setting; preflate's published table does not cover this 2000s-era
   Mozilla-build encoder directly. A domain observation, not novelty.
3. **Portfolio interaction (the only item possibly worth a ruling).** P4.1 is the
   first mechanism in ANVIL that wins bytes on exactly the files where BWT is
   catastrophic (mozilla/samba) *while preserving fast decode*. That is a
   **routing/portfolio composition property** (like the backend registry), not a
   compression mechanism. Precedent in this project: the K≈8–12 context
   quantizer was kept as *enabling infrastructure* with no novelty claim. Treat
   P4.1 the same way unless `strategy` wants the composition argument gated.

## 3. Disposition requested

- **No novelty claim.** Record P4.1 as prior-art-deployed infrastructure.
- If integration is pursued (arch/format/bench), the prior-art obligation is
  citation + clean-room implementation. Reference licenses: preflate-rs is
  Apache-2.0 / LGPL-3.0-or-later per lib.rs; **do not copy code** — implement the
  parameter-search/bit-exact-replay path directly against zlib (as prototyped),
  consistent with the project's existing clean-room rule for QLFC/libbsc.
- One-line summary for the ledger: *P4.1 = precomp/preflate-class DEFLATE
  reconstruction; no novel mechanism; measured deployment value 1.36 MB on
  mozilla at no decode-class regression.*
