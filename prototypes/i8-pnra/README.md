# prototypes/i8-pnra — pnra lane, iteration 8

Windows-host prototypes for transformation-invariant reference discovery.
Everything here is a measurement harness; none of it is a codec. I never touched
`src/anvil.cpp` (arch owns it) and never ran `tools/pareto_front.py` (bench owns it).

## Build
```
. .\env.ps1
cd prototypes\i8-pnra
clang-cl /O2 /EHsc /std:c++20 /MD /nologo /DNDEBUG ^
  /I ..\..\third_party\install\include /FeX.exe X.cpp ^
  /link /LIBPATH:..\..\third_party\install\lib ^
  brotlienc.lib brotlidec.lib brotlicommon.lib
```
Files that do not touch brotli (`pe_text.cpp`, `tbl_width.cpp`) need neither the
include nor the link lines.

| tool | brotli | what it measures |
|---|---|---|
| `pe_text.cpp` | no | PE/COFF executable-section extractor (Windows analogue of ELF .text) |
| `brotli_ref.cpp` | yes | brotli size + enc/dec MB/s reference column |
| `arith_codec.cpp` | yes | transform ladder scored by brotli, round-trip verified |
| `inv_selector.cpp` | yes | record-period detector + theory's integrating/stationary discriminator |
| `tbl_width.cpp` | no | temporal-table physical layout (lane 3, cache economics) |
| `sigma_est.cpp` | yes | pre-hoc frozen sigma estimators vs reference distance |

## Headline results
- **synth-arith.bin is unmodelled, not incompressible**: 8 runs x 8000 u32,
  integer slopes 64/83/79/69/96/48/91/2; first differences take exactly 3
  values at p=1/3. Floor 12,716 B (theory; independently reproduced here).
  brotli-q11 at 87,013 B is 6.84x that floor; ANVIL today is 20.1x.
- **Record-stride invariant**: -69.98% (synth-columnar-align, P=184 B),
  -20.63% (synth-timeseries, P=28 B). brotli q9, round-trip verified.
  {synthetic} tag applies — the structure is generator-determined.
- **SoA-K4 temporal table**: identical bytes and identical footprint, 1.35x
  match-finder on anvil.exe .text but -8.4% on anvil_bench — NOT a clean win,
  needs corpus validation before adoption.
- **Position-derived sigma is dead on statistical transforms**: residual entropy
  grows 0.65-0.69 bits per doubling of reference distance for both single-pair
  and least-squares estimators. Multi-pair reduces the intercept, not the slope.

## Gate position (docs/gate-verdict-i8-ari-stride.md)
- Transmitted sigma: FAIL — prior art (parametric dictionary).
- Position-derived sigma: defensible ONLY where the transform is EXACT
  (G4 amended). Measured dead on statistical transforms. Recorded as
  **math-class negative DNB-M2**; the A1-A4 ablation is retired for this family
  (exactness now decides first and is cheaper to test).
- Record-period P: **infrastructure, not novelty** — xz `--delta` covers 1..256
  and both measured periods (28, 184) fall inside it. Handed to datastruct as a
  measured capability gap (~30% of bytes at constant record offsets) plus a
  placement constraint (inside-the-reference, per the lane-transpose negative).
- Nothing here is a novelty claim. No Pareto claim is made.

## Errors I made, kept on record so nobody repeats them
1. A piecewise-linear row reported **13 bytes**: the segmenter collapsed to
   length-1 segments, each value became its own base, the residual was
   identically zero, and parameter cost went uncharged. RETRACTED. It measured
   "store data in an uncounted side channel." (strategy logged the pattern as
   AUDIT-7, UNCHARGED-PARAMETER TRANSFORM.)
2. A 49,585 B "ceiling" coded the residual at marginal entropy on an
   integrating process. SUPERSEDED by 12,716 B.
3. `tbl_width.cpp` asserted A/B/C must be byte-identical. **False** — reducing
   bucket depth K removes candidate positions, so the parse legitimately
   changes. Fixed in-file; the valid controls are A vs D (same K, different
   layout) and C vs E vs F.
4. `sigma_est.cpp`'s S1 and S3 columns are identical and that is CORRECT, not a
   bug: mean-of-differences telescopes to `(v[b-1]-v[a])/(b-a-1)`, which is
   exactly the single-pair estimator. It is a passed consistency check.

## Build gotcha (cost three attempts)
brotli's static libs reference `__imp_log2`. With clang-cl you MUST pass `/MD`.
Without it lld-link reports "undefined symbol: __declspec(dllimport) log2" and
suggests an unavailable import library. Adding `ucrt.lib` gives duplicate
symbols; switching to MSVC `cl` gives LNK2001. `/MD` is the fix.

Per the coordinator's isolation rule: build ONLY here, never in `./build`
(arch and bench own that directory).

## Blackboard
`deliverable/pnra-i8-invariant-family`, `-windows-port`, `-temporal-width`,
`-event-exact`, `-status` (the last one is the index).
