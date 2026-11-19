# libsais upstream provenance — v2.12.2 serial decode candidate

ANVIL vendors the 8-bit `libsais` implementation as its suffix-array / BWT
construction and inverse-BWT primitive. ANVIL's own BWT post-transform
representation, coding, backend framing, safety checks, and candidate selection
remain independent. `libbsc` is not vendored.

- Upstream: https://github.com/IlyaGrebnov/libsais
- Official tag: `v2.12.2`
- Immutable upstream commit: `b1aa1fde090e00a4e2030b9fb54198698b84e463`
- Published: 2026-11-09
- License: `Apache-2.0 WITH LLVM-exception`; see the complete upstream `LICENSE`
  in this directory. The changed license applies to the imported version.
- Upstream source Git blob SHA-1 (LF):
  - `src/libsais.c`: `88a4c6028a145fba675ba6c912256d0d1fdb9364`
  - `include/libsais.h`: `476463fd13897f59e4f92990727f67384514318b`
  - `LICENSE`: `04ec610d9bfbaf64f75e52dae766654648480e0b`
- Imported files: `include/libsais.h`, `src/libsais.c`, `LICENSE`.

**Source transport:** the C implementation and public header were copied from
this upstream commit with only LF-to-CRLF newline conversion to match ANVIL's
original vendored-source line endings. The license file was copied unmodified
with its upstream LF representation. Git *blob* IDs of these vendored CRLF files
therefore differ from their upstream LF IDs; this is newline normalization,
not a semantic or algorithmic patch. The originally vendored 2.10.4 source was
verified line-for-line identical to its upstream tag after LF normalization.

**Unverified candidate:** this is a source-only, version-only experiment on
the DRAFT research branch. The new implementation has NOT yet been built,
executed, fuzzed, or benchmarked in ANVIL. The four `libsais_*` APIs ANVIL
calls are present in the new public header, but compatibility and performance
must be certified by manual-only GitHub Actions under the A70 preregistration:
byte-identical old-format decoding first, paired serial end-to-end time, peak
memory, decoder+dependency size, malformed input behavior, cross-corpus
generalization, and then separately any OpenMP factoring.

Do NOT merge or replace the frozen candidate just because an upstream release
note reports a speedup. No new wire format or C7 policy change is intended.
