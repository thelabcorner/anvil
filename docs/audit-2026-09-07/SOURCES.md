# Sources and Provenance

## 1. Local source-of-truth artifacts

### Current external-corpus measurements

- `../../tests/ratio-first-standard.csv` — canonical Silesia + enwik8 reference/ANVIL table; 52 cells; current timing status fields.
- `../../tests/bwt-backend-standard.csv` — first direct-BWT Silesia backend-2 pass.
- `../../tools/bench_ratio.py` — checkpointed benchmark harness and manifest validation.

### Current codec / format

- `../../src/anvil.cpp` — current encoder/decoder, rev1 modes, rev2 transform/backend registry, BWT backend.
- `../../FORMAT.md` — current format spec; note audit finding that backend ID 2 is not yet documented there.
- `../../tests/fuzz.py` — current generic + deterministic rev2 fuzz harness; note backend-2-specific gap.
- `../../CMakeLists.txt` — current Brotli/libsais build wiring.
- `../../third_party/libsais/UPSTREAM.md` — pinned libsais provenance.

### Historical experiment record

- `../../RESEARCH_LEDGER.md` — primary long-form experiment ledger.
- `../CONTEXT.md` — historical shared context; useful but stale in dependency/build sections.
- `../decoder-audit.md` — malformed-input audit history.
- `../gate-ruling-i8-pareto-win.md` — FRONT-GAP / FRONT-CROSSING / DEGENERATE ruling.
- `../gate-verdict-i8-ari-stride.md` — exact-vs-statistical derived-parameter narrowing.
- `../i8-streak-correction.md` — historical tally error and recomputation rule.

## 2. Canonical/public benchmark references

### Matt Mahoney compression benchmarks

- Silesia corpus/results: <https://mattmahoney.net/dc/silesia.html>
- Large Text Compression Benchmark / enwik8: <https://mattmahoney.net/dc/text.html>
- Data Compression Explained: <https://mattmahoney.net/dc/dce.html>

Use published figures as landscape references. Direct ANVIL claims should use local current builds where practical.

## 3. BWT / block sorting

### libbsc

- repository: <https://github.com/IlyaGrebnov/libbsc>
- project: <https://libbsc.com/>
- current public API exposes BWT/ST3–ST8, QLFC static/adaptive/fast, and LZP parameters.

ANVIL does not vendor libbsc. It is a prior-art/reference implementation.

### libsais

- repository: <https://github.com/IlyaGrebnov/libsais>
- ANVIL pin: v2.10.4 / `ce90878d784b5ff7d019300535675e4a2e22aae0`
- license: Apache-2.0

### QLFC

- Florin Ghido, “QLFC - a Compression Algorithm Using the Burrows-Wheeler Transform,” DCC 2005, DOI `10.1109/DCC.2005.75`.
- public summary/reference: <https://citeseerx.ist.psu.edu/document?doi=d7e0d59ff0df71982be87aae129c2a3c4e329124&repid=rep1&type=pdf>

The central useful distinction for ANVIL: QLFC carries symbol-associated local-frequency information rather than treating the BWT post-stage as only anonymous MTF ranks.

### BWT postprocessing survey

- “Burrows–Wheeler compression: Principles and reflections,” Theoretical Computer Science 387(3), 2007, DOI `10.1016/j.tcs.2007.07.012`.

## 4. DEFLATE reconstruction

### Microsoft preflate-rs

- <https://github.com/microsoft/preflate-rs>

Architecture used as a reference:

- parse original DEFLATE decisions;
- estimate producer parameters;
- predict original choices;
- encode mispredictions/corrections;
- replay bit-exactly.

It supports several zlib-family implementations and container scanning, with unknown-compressor correction fallback.

### Original preflate

- <https://github.com/deus-libri/preflate>

Both references are kept/studied separately from ANVIL integration; any incorporation must be explicit and license-attributed.

## 5. Repetitive-corpus future work

- Pizza&Chili repetitive corpus: <http://pizzachili.dcc.uchile.cl/repcorpus.html>

Future fetch scripts should pin hashes and record the published diagnostic measures (δ, z, r, g, H0–H8) alongside each file.

## 6. Source-handling rules

1. A historical document is evidence of what was believed/measured at that time, not automatically current truth.
2. Current byte counts must be recomputed from CSV/executable output before inclusion in claims.
3. Published benchmark numbers are external orientation unless reproduced locally.
4. Scratch clones are reference material. Vendored dependencies require explicit provenance and license files.
5. Generated corpora are mechanism tests, not evidence of general-purpose transfer.

