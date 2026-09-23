# 04 — Architecture, Format, and Safety Audit

## 1. Architecture today

ANVIL is now best described as two research architectures sharing one executable.

### Revision 1 — token-codec laboratory

Revision 1 contains the historical block modes:

- raw;
- arithmetic token modes;
- rANS-separated exact LZ;
- SPARSE-REF;
- SHAPE;
- TOPOLOGY;
- TCOPY;
- HOTOP;
- ARI-REF.

This branch is where most mechanism-level parser/reference/decoder research occurred.

### Revision 2 — representation/backend framework

Revision 2 deliberately narrows the block namespace to:

- mode 0: raw;
- mode 17: ratio envelope.

Mode 17 is:

```text
transform_id
backend_id
transformed_size
backend_payload
```

The architectural decision is sound: **transforms and compressors are orthogonal wire choices**.

Current transform IDs:

| ID | transform |
|---:|---|
| 0 | direct |
| 1 | previous-byte routed `ctx1-256` partition |
| 2 | line-record column transpose |

Current source backend IDs:

| ID | backend | state |
|---:|---|---|
| 1 | Brotli | established rev-2 backend |
| 2 | BWT + ANVIL postcoder portfolio | validated (E0) |

The encoder can request `brotli`, `bwt`, or `auto`.

## 2. What is right about this separation

### 2.1 It makes attribution possible

A transform is evaluated under the same backend as direct bytes. A backend can be evaluated under the same transform. This prevents a common compression-research confound: changing representation and entropy model simultaneously, then attributing the result to whichever part is novel.

### 2.2 It makes heterogeneous routing natural

The direct-BWT Silesia results prove that backend identity is content-dependent. The explicit registry turns that empirical fact into a first-class architecture instead of an `if` hidden in the encoder.

### 2.3 It preserves decoder determinism

The decoder does not infer the backend. It executes an explicit ID and strict payload semantics. Encoder heuristics remain encoder policy.

## 3. Current format/documentation mismatch — CLOSED (E0)

`src/anvil.cpp` already implements backend ID 2. As of E0, `FORMAT.md` documents
backend ID 2 in full: BWT backend payload (`postcoder_id` byte, `primary_index`
uvarint, postcoder payload), all 4 postcoder IDs (0 static MTF+RLE
stream-suite, 1 adaptive order-0 arith, 2 adaptive order-1 arith, 3 raw BWT
stream-suite), uvarint bounds, `primary_index < transformed_size` bound,
arithmetic bit-count/padding rules, stream-consumption requirements, and the
decode memory bound (`transformed_size` output + `int32[transformed_size+1]`
work array). Golden backend-2 files and a deterministic BWT adversarial fuzz
matrix are added (see §7).

Before backend 2 is treated as landed:

1. ~~add backend ID 2 to `FORMAT.md`~~ done;
2. ~~specify the BWT backend payload and postcoder IDs~~ done;
3. ~~specify primary-index bounds~~ done (`< transformed_size`);
4. ~~specify MTF/RLE token/run semantics~~ done;
5. ~~specify arithmetic bit-count/padding rules~~ done;
6. ~~specify exact stream-consumption requirements~~ done;
7. ~~specify memory/size bounds~~ done;
8. ~~add golden backend-2 files~~ done (tests/fuzz.py golden BWT set + recorded sizes/hashes).

Measured validation (canonical, pinned clang-cl build; `bench-normalize`,
blackboard `deliverable/auto-routing` + `deliverable/enwik8-bwt`): Silesia
whole-file `--ratio-backend=auto` = **46,446,995 B** (beats xz -9e by
2,009,105 B; backend 2 chosen on 7/12 files) and enwik8 = **23,534,368 B**
(beats xz by 1,297,288 B). All round-trips sha256-verified. The byte win is
real but a **front-gap on decode cost** (BWT decode ~11.2x slower than
Brotli) — reported honestly, not as a Pareto crossing. The ~159 B gap to the
46,446,836 B oracle is rev-2 framing only. Backend 2 is therefore a valid
**validated** format as of E0. The degenerate (1-byte / all-equal /
`primary_index == 0`) round-trip was an open item pending the `libsais`
inverse-BWT fix in `arch-bwt`; that fix is now landed and verified (uni/one/
mix round-trips byte-exact). The deterministic BWT malformed-input matrix
(tests/fuzz.py, 24 adversarial cases) plus the degenerate round-trip checks all
pass with **zero skips** on the current `build/anvil.exe` (full
`python tests/fuzz.py --exe build/anvil.exe --cases 50` →
`deterministic_bwt=24, golden_bwt=4, 0 skips`). A stack-overflow regression in
the `--parse=ratio` encode entry (introduced with the same change set, now fixed
— root cause: Windows 1 MiB default thread stack too small for third-party
`libsais_bwt()` / `BrotliEncoderCompress()` buffers; fixed with `/STACK:8388608`
in CMakeLists.txt) had temporarily blocked the end-to-end re-run; it is resolved,
the canonical numbers reproduce on the current source (Silesia 46,446,995 B /
enwik8 23,534,368 B), and backend 2 is fuzz-complete — a valid validated format.

## 4. BWT backend design audit

Backend 2 uses pinned Apache-2.0 `libsais` only for forward/inverse BWT. ANVIL owns the post-transform and entropy representation.

Internal postcoders:

| ID | representation/coder | role |
|---:|---|---|
| 0 | MTF + rank-zero RLE + smallest stream-suite coding | separated-stream candidate |
| 1 | MTF/RLE + adaptive order-0 arithmetic | adaptive candidate |
| 2 | MTF/RLE + adaptive order-1 token arithmetic | adaptive context candidate |
| 3 | raw BWT + stream-suite coding | BWT-only ablation/control |

The encoder builds all candidates and chooses the exact smallest payload.

### Strengths

- strong direct Silesia evidence already exists;
- internal postcoder ablation is built in;
- primary index is explicit and bounded;
- inverse output length is externally fixed by `transformed_size`;
- arithmetic payload has explicit meaningful-bit count and zero-padding validation;
- no floating-point state is required for decode.

### Weaknesses

- all candidate buffers are built eagerly;
- forward BWT needs an `int32` work array;
- inverse BWT needs an `int32` work array plus output;
- BWT-specific malformed-input fuzz is missing;
- format spec is missing;
- no golden compatibility artifacts exist;
- no compiler-normalized throughput data exists;
- no enwik8 BWT result exists yet;
- current `auto` backend result has not been measured.

## 5. BWT memory economics — YELLOW/RED for 128 MiB blocks

The backend is bounded by rev-2 block limits, so there is no obvious attacker-selected unbounded allocation beyond the already-validated block/output declarations. But bounded does not mean operationally good.

At large blocks the encoder can hold simultaneously:

- original block copy;
- BWT output;
- ~4×N libsais work array;
- MTF token/run buffers;
- several encoded postcoder candidates;
- outer ratio candidates when multiple transforms/backends are tried.

The 51 MB `mozilla` BWT experiment peaked around hundreds of MiB. A naïve linear extrapolation to 128 MiB can enter multi-gigabyte territory.

### Required changes before production-scale BWT

1. cap BWT sub-block size independently from rev-2 outer block size;
2. serialize/evaluate postcoder candidates one at a time and retain only the current best;
3. release BWT/MTF intermediates as soon as possible;
4. avoid constructing ctx/line transform × BWT candidates if a cheap pre-gate predicts no chance of beating current best;
5. record encoder peak-memory budget as part of backend policy.

The decoder also deserves an explicit maximum resident-memory calculation rather than relying only on “bounded by 128 MiB.”

## 6. Decoder safety record — a project strength

ANVIL’s decoder-hardening work is one of the most successful project threads. Historical fuzz/audit work found and fixed classes including:

- attacker-declared huge substream allocation;
- declared-total output amplification;
- arithmetic payload trailing garbage;
- RLZ varint add/wrap leading to invalid distances;
- RePair expansion amplification;
- recursive grammar/reference nesting;
- EOF match-finder overread;
- topology modal-table bounds issues.

The project repeatedly used fuzz failures to change the format implementation before counting benchmark results. Keep this culture.

## 7. Current rev-2 fuzz status

After the backend-ID wire change:

- 1,040 roundtrip variants passed;
- 6,240 random mutations passed the reject-or-identical rule;
- 7 deterministic rev-2 malformed-header cases passed.

Deterministic cases cover:

- unknown transform;
- unknown backend;
- transformed-size too small;
- transformed-size too large;
- transformed-size over bound;
- truncated backend payload;
- illegal rev-2 legacy mode.

### Missing BWT adversarial matrix — CLOSED (E0)

The deterministic seed explicitly created a direct/Brotli archive and the generic ratio fuzz used the default Brotli backend, so backend 2 was historically under-fuzzed. As of E0 this is closed: `tests/fuzz.py` adds a dedicated `adversarial_bwt` matrix (build_rev2 / bwt_layout / bwt_payload helpers) covering all of the following deterministic backend-2 cases, all of which are rejected by the decoder (zero skips):

- unknown postcoder ID;
- primary index == output length and > output length;
- zero / malformed arithmetic bit count;
- bit count requiring more/fewer bytes than payload;
- nonzero final padding bits;
- truncated arithmetic payload;
- MTF token output overflow;
- zero-run output overflow;
- truncated token substream;
- truncated run substream;
- token/run trailing bytes;
- raw-BWT decoded-length mismatch;
- malformed nested stream codec inside postcoder 0;
- transformed-size mismatch around inverse BWT;
- tiny 1-byte and highly degenerate all-equal BWT cases (byte-exact round-trips, arch-bwt libsais fix landed).

Backend 2 is therefore fuzz-complete: the deterministic BWT malformed-input matrix passes with zero skips, and the degenerate round-trip cases are covered as true encode->decode round-trips.

## 8. Dependency audit

### Brotli

Backend 1 is an explicit external codec dependency. This is appropriate now that ANVIL is explicitly a codec/container framework.

### libsais

Pinned provenance:

- tag `v2.10.4`;
- commit `ce90878d784b5ff7d019300535675e4a2e22aae0`;
- Apache-2.0;
- vendored files limited to 8-bit `libsais.c`, `libsais.h`, license.

ANVIL does **not** vendor libbsc/QLFC code.

This is a good boundary: use a specialized, audited suffix/BWT primitive; keep ANVIL’s codec economics independently testable.

## 9. Build-system audit

`docs/CONTEXT.md` is stale:

- it says the codec has no external dependencies;
- it describes a C++-only project;
- it assumes the old clang-cl setup.

Current CMake enables C and C++ for libsais. The initial rebuild failed because the plain shell did not expose the Windows SDK resource compiler and did not register clang-cl consistently. A VS developer environment + MSVC recovered the build.

### Action

Create one supported build entry point, e.g. `tools/build_release.ps1`, that:

- enters the VS SDK environment;
- selects a pinned compiler explicitly;
- builds C and C++ consistently;
- records compiler + flags + git/worktree hash into a generated build-info file;
- can make both full codec and decoder-only targets.

Do not let benchmark sessions choose a compiler accidentally based on the launching shell.

## 10. Decoder-size / modularity question

A portfolio codec accumulates code. Current full executable is >1.3 MB and ZIPs to ~609 KB, but that includes encoder code.

Possible architectures:

### Monolith

All backends always linked. Simplest deployment; highest decoder-size tax.

### Core + optional backend modules

Core container recognizes IDs, modules provide backends. Better install-size economics but complicates standalone-file portability and benchmark accounting.

### Decoder-only monolith + encoder research binary

Likely best next step. Produce a compact `anvil_decode` containing only inverse transforms/backends. This gives honest LTCB-style accounting without changing the file format.

Recommendation: implement decoder-only first before considering dynamic modules.

