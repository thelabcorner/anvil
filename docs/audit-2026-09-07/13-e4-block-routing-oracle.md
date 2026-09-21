# E4 — Block-local backend routing oracle

**Verdict: DECISIVE NEGATIVE.** Block-local backend routing is strictly worse than
whole-file routing at every measured scale (256 KiB, 1 MiB, 4 MiB, 16 MiB) on
mozilla, samba, ooffice, sao, webster. The go-bar (>+250 KiB or >0.25% on Silesia)
is not met — regret is **+0.64 MB to +5.42 MB**, i.e. materially *worse*.

## Method (honest, anchored to canonical numbers)

- Brotli q11/lw30 per block: `build\brotli_lw.exe c` (round-trip validated — every
  output decoded and compared byte-for-byte to the input before it is accepted,
  because the subprocess occasionally returns exit 0 with no output under load).
- BWT per block: parsed out of `build\anvil.exe c --parse=ratio
  --ratio-backend=bwt --ratio-context=off --ratio-lines=off --block=N`. One
  subprocess yields every per-block mode-17 BWT payload (each block is an
  independent payload); the ANVIL container parser recovers per-block coded bytes.
- Oracle = Σ over blocks of min(framed Brotli, framed BWT), where framing is the
  **real** rev-2 cost (1 mode byte + uvar(payload_len) + 4 CRC + payload) derived
  from `src/anvil.cpp:4033-4042`. Framing is negligible (≥4 MiB, <200 B total).
- Resumable CSV: `scratch/block-oracle.csv` (+ `scratch/block-oracle-wholes.csv`
  for whole-file reference). Driver: `tools/block_oracle.py`; analyzer:
  `tools/block_oracle_analyze.py`.

## Results (aggregate over the 5 mixed files)

| block size | block oracle | whole-file framed | whole-file regret | warmup tax | vs xz -9e |
|---|---:|---:|---:|---:|---:|
| 256 KiB | 37,374,557 | 31,950,395 | **+5,424,162** | +5,431,727 | -11.08 MB |
| 1 MiB   | 35,194,516 | 31,950,395 | **+3,244,121** | +3,248,022 | -13.26 MB |
| 4 MiB   | 33,604,442 | 31,950,395 | **+1,654,047** | +1,654,047 | -14.85 MB |
| 16 MiB  | 32,593,848 | 31,950,395 | **+643,453**   | +643,453   | -15.86 MB |

Whole-file framed reference = 31,950,395 B (the 5-file min, winner per file).
Measured whole-file `--ratio-backend=auto` Silesia total (bench-normalize) =
46,446,995 B over all 12 files; the 5-file slice is the fair comparison base here.

## The mechanism (the valuable part)

- **Every file is won by a single backend across all blocks.** webster = 100% BWT at
  every scale; mozilla / samba / ooffice / sao = 100% Brotli at every scale. A
  perfect block router therefore gains *nothing* from switching.
- The entire regret is **model-warmup tax**: independent blocks restart MTF/context
  state. webster at 4 MiB: Σ BWT blocks = 8,211,644 B vs whole-file BWT 7,317,329 B
  = **+894,315 B (+12.2%)**; Brotli shows the same (+773,686 B, +9.3%). Framing is
  negligible; warmup is the whole story.
- Warmup tax grows **monotonically** as blocks shrink → finer granularity is
  monotonically worse. 16 MiB already ~0 tax on ooffice/sao (single-block files).

## Falsification

- mozilla / samba / ooffice / sao have **ZERO BWT-winning blocks at every size**
  (mozilla 0/196 @256 KiB, 0/13 @4 MiB; samba 0/6 @4 MiB except 1/21 @1 MiB; etc.).
- This **falsifies** the "mixed files contain BWT-friendly text regions" hypothesis
  (03 §6 / I5 / P1.2): those files are BWT-hostile *throughout*, not merely on
  average. You cannot recover them by carving out text regions.

## Consequences

1. **E5 (byte-regret router) is not runnable** — it was pre-gated on E4 passing. Do
   not build the router. `tools/router_fit.py` is therefore not produced.
2. Strengthens **P4.1 DEFLATE reconstruction**: mozilla/samba loss is embedded
   compressed regions (bwt-theory confirmed 1 MiB blocks span 1.58–7.31 b/B), not
   carveable text. Only reconstruction of embedded streams can help.
3. Skip finer scales (already measured 256 KiB / 1 MiB — they are worse, as expected).

## Honest standing vs the shared enemy (xz -9e = 48,456,100 B)

Even the *32.59 MB* 16 MiB block oracle is far above xz. The real Silesia byte win
vs xz comes from **whole-file** `--ratio-backend=auto` (46.45 MB, +159 B over the
46.45 MB oracle, per bench-normalize) — not from block segmentation. Block routing
is a negative result that stops the project wasting effort on a router.

Reopen condition: only if a backend with near-zero warmup cost appears, or with
cross-block model carryover (so independent blocks do not restart state).
Reopen condition: only if a backend with near-zero warmup cost appears, or with
cross-block model carryover (so independent blocks do not restart state).

## Measurement-integrity note (tooling)

- `tools/block_oracle.py` default `--workers` is **1**. The pinned clang-cl
  build's `anvil.exe` was observed to crash (exit 0xFFFFFFFF) when many processes
  open container files concurrently; `--workers` only parallelizes the Brotli
  subprocess calls (anvil.exe itself runs once per file x size), so raise it only
  with the MSVC recovery build. Every Brotli size is round-trip validated
  (decode + byte-compare) before acceptance, because the subprocess can
  occasionally return exit 0 with no output file under load.
- bench-normalize independently re-ran the oracle at `--workers 1` on the pinned
  clang-cl build and reproduced every number here (whole-file 31,950,395 B;
  block oracle +643 KB ... +5.4 MB). The negative is corroborated across both builds.

## Convergent story across lanes (this round)

- E1/E2/E7 (bench-normalize): Silesia auto 46,446,995 B, enwik8 auto 23,534,368 B
  (bwt-theory verified enwik8 to the byte), combined 69,981,363 B; both beat xz.
- E6 (bwt-theory): NO-GO on QLFC/LZP - no ratio headroom.
- E4 (this): block routing loses everywhere; **whole-file `--ratio-backend=auto`
  is the correct router.** block segmentation + QLFC/LZP add nothing.
- E5: PARKED (gate failed) - router_fit.py not produced.