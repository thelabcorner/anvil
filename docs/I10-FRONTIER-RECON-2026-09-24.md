# ANVIL I10 Current Frontier Reconciliation — 2026-09-24

**Status:** control-plane reconciliation; no new local compression benchmark

**Live source:** branch `i10-aux-unbwt`, commit `b8eae11fa353bb5e3c88e8e757fe3ad41e4bbd24`  
**Published main at reconciliation:** `b6a243c9657776058a37e5f0ee2eaae2ff925dff` (documentation-only closure navigation; no scientific or source change)

**Local state:** the working tree is intentionally dirty with generated research
artifacts and concurrent worktree state. No reset, cleanup, stash, or overwrite was
performed.

## 1. Evidence classes and comparability

### Class A — frozen I9 multi-quality suite

`tests/benchmark-suite.frozen-bdc90474.plus-xz.csv` is the current comparable
multi-quality grid available locally. It contains 13 files, the frozen
`BDC90474` ANVIL source, Brotli q1/q4/q6/q9/q11, and `xz-9e`.

- Windows 11 x64, Ryzen 9 5900X, clang-cl 22.1.8; see `tests/host-spec.md`.
- Per-rep process invocations; 3 repetitions for 11 files and 7 for
  `generated.json` and `synth-timeseries.bin`.
- Bytes are deterministic. Throughput is ranking-grade, not citation-grade:
  the frozen window had elevated ambient utilization.
- The CSV has no peak-RSS field. RSS is therefore `not recorded` for this grid.
- The table below aggregates complete bytes and harmonic input-byte throughput
  from the frozen CSV (`tools/pareto_front.py`: total input bytes divided by the
  sum of per-file `input_bytes / rate` times, not an arithmetic mean of rates);
  it does not combine results from other hosts. The 13 codec rows were
  independently recomputed and matched exactly on 2026-09-25.

| candidate | complete bytes | ratio | encode MB/s | decode MB/s | RSS | comparability |
|---|---:|---:|---:|---:|---|---|
| Brotli q1 | 2,727,579 | 0.222254 | 432.106 | 471.702 | not recorded | Class A |
| Brotli q4 | 2,492,866 | 0.203129 | 137.114 | 603.721 | not recorded | Class A |
| Brotli q6 | 2,163,836 | 0.176318 | 69.262 | 624.797 | not recorded | Class A |
| Brotli q9 | 2,123,887 | 0.173063 | 25.009 | 608.767 | not recorded | Class A |
| Brotli q11 | 1,788,233 | 0.145713 | 0.681 | 528.346 | not recorded | Class A |
| xz -9e | 1,720,520 | 0.140195 | 1.310 | 159.369 | not recorded | Class A |
| ANVIL dp-rANS | 2,539,808 | 0.206954 | 1.321 | 357.571 | not recorded | Class A |
| ANVIL sparse-rANS | 2,609,164 | 0.212605 | 10.567 | 222.692 | not recorded | Class A |
| ANVIL TCOPY-rANS | 2,607,687 | 0.212485 | 10.374 | 226.017 | not recorded | Class A |
| ANVIL MDL-rANS | 2,439,834 | 0.198808 | 0.874 | 278.323 | not recorded | Class A |
| ANVIL shape-rANS | 2,508,878 | 0.204434 | 0.786 | 225.989 | not recorded | Class A |
| ANVIL hot-op rANS | 2,642,320 | 0.215307 | 10.462 | 330.374 | not recorded | Class A |
| ANVIL hot-op RLZP | 2,561,700 | 0.208738 | 0.157 | 264.753 | not recorded | Class A |

The frozen reference-class verdict remains **zero FRONT-CROSSING**. The
canonical I9 tuple is `33 non-dominated | 5 FRONT-GAP | 0 FRONT-CROSSING |
28 DEGENERATE | 435/468 dominated` with xz included. `GRID-THIN` remains
binding because intermediate zstd tiers and Brotli window tiers were not all
measured.

### Class B — remote I10-1A auxiliary-index BWT

These are same-job GitHub Actions measurements and must not be spliced with
Class A absolute throughput. Runner identity and reference versions are
recorded in `docs/I10-AUX-UNBWT-RESULTS.md` and the remote artifacts.

| corpus / arm | complete bytes | ratio | encode MB/s | decode MB/s | peak RSS | timing evidence |
|---|---:|---:|---:|---:|---:|---|
| Silesia ANVIL legacy | 46,446,995 | 0.219153 | not reclassified | not reclassified | not recorded here | byte identity only |
| Silesia ANVIL aux | 46,466,339 | 0.219244 | neutral in paired runs | 47.9 | 248.4 MiB | run 35927623136 |
| Silesia xz -9e | 48,456,004 | 0.228632 | reference | 82.5 | 54.3 MiB | same run |
| Silesia Brotli q11/lw30 | 49,383,136 | 0.233007 | reference | 166.9 | 124.5 MiB | same run |
| enwik8 ANVIL legacy | 23,534,368 | 0.235344 | not reclassified | not reclassified | not recorded here | byte identity only |
| enwik8 ANVIL aux | 23,537,422 | 0.235374 | neutral in paired runs | 26.5 | 598.9 MiB | run 35927623136 |
| enwik8 xz -9e | 24,831,648 | 0.248316 | reference | 103.6 | 66.2 MiB | same run |
| enwik8 Brotli q11/lw30 | 24,810,180 | 0.248102 | reference | 150.2 | 251.9 MiB | same run |

The one-shot scout values above are context only. The paired same-job values
from run `35927623136` are: Silesia ANVIL aux 47.908 MB/s decode, xz
82.504 MB/s, Brotli 166.867 MB/s; enwik8 ANVIL aux 26.519 MB/s, xz
103.559 MB/s, Brotli 150.245 MB/s. The paired ratios are ANVIL/xz
1.722623 [1.664049, 1.839984] on Silesia and 3.906480 [3.900312,
4.008800] on enwik8; ANVIL/Brotli is 3.447447 [3.268007, 3.505939] and
5.664220 [5.652814, 6.759625]. A/A controls span 1.0. Peak-RSS ratios are
4.572x/1.995x versus xz/Brotli on Silesia and 9.043x/2.378x on enwik8.
The paired isolated encode ratios for aux are neutral within the recorded
CIs; they are not a full-portfolio encode Pareto result.

The same remote frontier also measured one-shot scout rows for the standard
corpora. These rows are not promotion-grade on their own, but they provide the
missing dense reference context:

| corpus / arm | bytes | ratio | 1-shot enc / dec MB/s | max RSS enc / dec MiB |
|---|---:|---:|---:|---:|
| Silesia legacy ANVIL | 46,446,995 | 0.219153 | 0.469 / 26.733 | 691.4 / 248.4 |
| Silesia ANVIL aux | 46,466,339 | 0.219244 | 0.467 / 43.099 | 689.6 / 248.4 |
| Silesia Brotli q11/lw30 | 49,383,136 | 0.233007 | 0.505 / 140.400 | 619.1 / 116.6 |
| Silesia xz -9e | 48,456,004 | 0.228632 | 2.122 / 60.261 | 510.1 / 50.2 |
| Silesia zstd ultra-22/long-27 | 52,364,240 | 0.247073 | 1.802 / 161.909 | 745.4 / 51.7 |
| enwik8 legacy ANVIL | 23,534,368 | 0.235344 | 0.432 / 12.313 | 1,229.1 / 598.9 |
| enwik8 ANVIL aux | 23,537,422 | 0.235374 | 0.461 / 25.578 | 1,227.3 / 599.1 |
| enwik8 Brotli q11/lw30 | 24,810,180 | 0.248102 | 0.464 / 249.021 | 1,099.2 / 209.8 |
| enwik8 xz -9e | 24,831,648 | 0.248316 | 1.475 / 76.671 | 675.3 / 66.2 |
| enwik8 zstd ultra-22/long-27 | 25,272,471 | 0.252725 | 1.462 / 496.243 | 833.0 / 58.2 |

The one-shot rows use process-level measurements and must not be compared
across runner jobs. The paired decode values above remain the timing authority
for aux-v2; the full encode axis is not yet a paired full-portfolio result.

### Fresh public-main scout confirmation (run 2026-09-24/25)

Two existing public scout runs on published main
`b6a243c9657776058a37e5f0ee2eaae2ff925dff` completed successfully. They are
5-repetition, same-job, per-file process measurements with
`timing_status=VALID_100MS_NONRECURSIVE`; bytes were deterministic across all
repetitions (Silesia 60/60 cells stable, enwik8 5/5 stable). They are scout-grade
and unpaired: they do not provide a paired candidate/reference ratio, A/A
controls, or bootstrap CIs, and they must not be spliced with the paired
reference-cost run or Class A.

- Silesia run: <https://github.com/thelabcorner/anvil/actions/runs/36061224833>,
  artifact `anvil-silesia-b6a243c9657776058a37e5f0ee2eaae2ff925dff-run1`,
  API ID `10841382467`, digest
  `sha256:5bf0b868c65a0bcf7eb09202ac0b60a8c58d42752b6f029260da32d0f37747d9`.
- enwik8 run: <https://github.com/thelabcorner/anvil/actions/runs/36061230082>,
  artifact `anvil-enwik8-b6a243c9657776058a37e5f0ee2eaae2ff925dff-run1`,
  API ID `10837967799`, digest
  `sha256:aff30ddbdaa90fba1117492fa8cac02326491bc207e30b5cc6295aef708adde6`.

| corpus / arm | bytes | ratio | encode MB/s (CV) | decode MB/s (CV) | enc RSS MiB | dec RSS MiB |
|---|---:|---:|---:|---:|---:|---:|
| Silesia legacy ANVIL | 46,446,995 | 0.219153 | 0.443 (0.86%) | 23.551 (4.13%) | 689.0 | 248.4 |
| Silesia Brotli q11/lw30 | 49,383,136 | 0.233007 | 0.476 (0.44%) | 140.267 (0.03%) | 620.6 | 114.4 |
| Silesia xz -9e | 48,456,004 | 0.228632 | 1.818 (2.60%) | 60.253 (0.00%) | 510.1 | 50.2 |
| Silesia zstd ultra-22/long-27 | 52,364,240 | 0.247073 | 1.831 (1.56%) | 161.781 (0.02%) | 745.4 | 49.9 |
| enwik8 legacy ANVIL | 23,534,368 | 0.235344 | 0.435 (0.37%) | 12.407 (1.28%) | 1,232.5 | 598.9 |
| enwik8 Brotli q11/lw30 | 24,810,180 | 0.248102 | 0.465 (0.67%) | 248.759 (0.08%) | 1,099.7 | 209.0 |
| enwik8 xz -9e | 24,831,648 | 0.248316 | 1.474 (2.48%) | 76.692 (0.01%) | 675.3 | 66.2 |
| enwik8 zstd ultra-22/long-27 | 25,272,471 | 0.252725 | 1.473 (2.56%) | 497.536 (0.10%) | 833.0 | 56.5 |

The scout confirms the existing disposition without a crossing: legacy ANVIL is
5.95%/5.14% smaller than Brotli on Silesia/enwik8 but decodes 5.96x/20.05x
slower with 2.17x/2.87x the decode RSS; xz is 1.88% smaller than Brotli on
Silesia but 0.09% larger on enwik8; zstd has the fastest decode here but larger
bytes. Brotli q11 remains the decode-throughput and decode-memory reference on
these corpora. `FRONT-GAP` and `GRID-THIN` remain binding, and the paired
reference-cost run remains the timing authority for the aux branch.

### Fresh reference-cost rerun

Run `36061511123` on published main
`b6a243c9657776058a37e5f0ee2eaae2ff925dff` independently reproduced the
aux-v2 byte totals but not the earlier absolute timing series:

- Silesia ruling: `FRONT-GAP_COST`; ANVIL/xz paired decode ratio **1.799536**
  (95% CI **[1.778155, 1.800136]**), ANVIL/Brotli **3.331196**
  (**[3.247262, 3.382778]**), A/A **[0.987508, 1.012244]**; ANVIL aux
  50.782 MB/s / 248.4 MiB, xz 91.388 MB/s / 54.3 MiB, Brotli
  167.104 MB/s / 122.5 MiB.
- enwik8 ruling: `TIMING_BLOCKED`; A/A passed, but the Brotli comparison
  exceeded the ambient/CV gate. Bytes and RSS remain 23,537,422 B /
  598.9 MiB for aux, 24,831,648 B / 66.1 MiB for xz, and 24,810,180 B /
  251.8 MiB for Brotli.
- Artifacts: `anvil-i10-aux-reference-cost-silesia-36061511123`, ID
  `10834858417`, digest
  `sha256:30574a25c718fe45828e1c6e43278f188302c66ee4975ea3e74a35cbb034ac6e`;
  `anvil-i10-aux-reference-cost-enwik8-36061511123`, ID `10835616518`,
  digest `sha256:837cf95daa310f1e11e58813f11ae9f41a2a560a1173f372c1fd1b3fd27c1c85`.

This rerun reinforces the rule that absolute timings cannot be spliced across
jobs. It does not weaken the byte identity or the bounded aux-unBWT result.

The auxiliary index is a real decoder-side Pareto trade: it costs 8,083 B
across the three paired targets and improves whole-codec decode by 1.364x,
1.795x, and 2.339x. It is not an external frontier crossing: the candidate
remains slower and much larger in working set than the same-run references.

### Class C — G3/G4/G5 structured-representation research

These rows are mechanism evidence, not full-codec production rows. Their
complete-byte accounting is valid for the frozen carrier and Brotli backend,
but their timing and corpus roles must not be compared directly with Class A
or Class B.

| experiment | frozen result | status | reason |
|---|---|---|---|
| G3 D1-D4 routed portfolio | 1,384,654 B vs 1,466,769 B raw Brotli, -5.5984% | PASS-G3 discovery | regionization is causal on discovery |
| G3 V1 | 2,179,615 B vs 2,112,235 B raw, +3.1900% | PASS-G3-NARROW | fully structured V1 still regresses; not a coverage failure |
| G4 O11 versus frozen G3 | 16/16 exact carrier SHA-256 equalities | valid identity control | G4 implementation is measuring the frozen representation |
| G4 S0/S1/S2 | worst-file regret 6.6056%, 7.0802%, 1.5388%; published speed ratios are withdrawn | NO-GO-G4 / INVALID_SPEED_ACCOUNTING | byte-fidelity failure is valid; the workflow compares cached O11 labels with proxy work rather than equivalent q11 work |
| G5A D1-D4 A0/A1/A2/A3 | 2,054,532 / 1,470,205 / 1,452,383 / 1,380,245 B | ORDER-MATERIAL, COLUMN-DOMINANT | same charged token multiset, column ordering saves 89,960 B; A3 beats A1 on 4/4 |
| G5A V1 | A1 2,182,265 B; A3 2,183,986 B | V1-COLUMN-ADVERSE | ordering is population-dependent; known stress only |
| G5B corrected B0/B1/B2 | 2,054,910 / 1,380,245 / 1,403,029 B | ORDINAL-ADVERSE | B2 is 1.6507% above B1 and smaller on only 1/4 non-degenerate files |

G5A's 89,960 B net ordering saving is concentrated: D3 contributes 63,483 B
and D4 contributes 22,823 B, while shape grouping alone is +18,351 B on D3
and -529 B on D4. A1 is source order of extracted scalar chunks, not raw source
bytes; A1 is already 3,436 B above aggregate raw Brotli. Therefore the valid
claim is narrow: column ordering improves the fixed G5A carrier, not all
structured representations. The one A0 permutation is a null draw, not a
significance distribution.

## 2. G4/G5 identity and invalid-result quarantine

### G4

- Run: <https://github.com/thelabcorner/anvil/actions/runs/35952830106>
- Run head: `a3c9c5a2e4270449387d86f3a91d9ee56ceeaa67`
- Frozen implementation: `6c556dc522112aff4b3566990965e000418c5a57`
- Frozen G3: `1a3d18fed76adb6fb33264e1994f9c357306b3fa`
- G4 source blob: `977213641a6de55d771f091874d2aa886d3d3be3b`; source SHA-256:
  `08c2a9861e25511b15e00ae12e2a4cee8288e0c5974bca855e493b38b5683c85`;
  binary SHA-256: `b2851bfcaa8cf41f74073f6d91b947da036fbcd66a61b05f8e9eacc892f1e577`
- Artifact: `grotli-g4-planner-fidelity-35952830106`, API artifact ID
  `10789896530`, digest `sha256:8a96fabc32cc0d6c5cef9a7ba35d07d881b8ca9b21e98f3062fbf82b79704f63`
- Verdict: `NO-GO-G4`; correctness and identity gates passed.
- Speed accounting is separately **INVALID_SPEED_ACCOUNTING**: the workflow's
  G8.6 denominator uses `ranking_score_ms_with_selection` for O11, while the
  actual q11 label build is timed as `o11_label_ms` (for D1, 667.948690 ms
  versus 0.004758 ms in the cached table). The S0/S1/S2 speedup figures are
  withdrawn; the byte-fidelity NO-GO remains valid because every proxy misses
  the frozen byte gates by large margins.

### G5A

- Run: <https://github.com/thelabcorner/anvil/actions/runs/35985412906>
- Run head: `996c2dbe67736c42288287abba7ed6f3307a0b34`
- Frozen implementation: `e6714e81aeff1579c4502f1fd9af4d7f205a8b4b`
- Frozen G3: `1a3d18fed76adb6fb33264e1994f9c357306b3fa`
- G5A source blob: `2772d7eaf0a64be1fdbbb377f5d668d21dab9cbe`; source SHA-256:
  `0fe9b8e599c922d3f489ffed54d595d867cfdf7f1ff0a6f6f785996cf88d382f`;
  prereg blob: `a84f42a3f414cc987d8c19cc70a578e2d4222467`; binary SHA-256:
  `119f1e661993976e14b98948538887cd3260ce22062f45f9c6b8e8f3778e4b00`
- Artifact: `grotli-g5a-ordering-attribution-35985412906`, API artifact ID
  `10801714249`, digest `sha256:4765b317e7c01170cffbd96b08c639e001ec1e2f317152d1f83a20a5331a3278`
- Verdict: `ORDER-MATERIAL`, attribution `COLUMN-DOMINANT`; V1 is known stress
  and adverse, not held-out evidence.

### G5B-ORDINAL

The first run is quarantined:

- Invalid run: <https://github.com/thelabcorner/anvil/actions/runs/36010649558>
- Head: `10490d5101801f78de17f0ab81e41f926e30d3fd`
- Artifact ID `10812316817`, digest
  `sha256:0d01e47e69624de06777407b58fa48f984278b7ff48b6ddcc2c6ea3e53ffbb56`
- Ruling artifact says `INVALID-G5B-ORDINAL`: discovery was skipped because
  floor/provenance validity could not be established. The job log identifies
  the concrete cause as HTTP 401 while the Python downloader forwarded the
  GitHub bearer token across a cross-host 302 redirect to signed blob storage;
  all corpus, replay, discovery, V1, and floor steps were skipped. B2
  interpretation is explicitly prohibited. No byte comparison from this run
  is admissible.

The corrected run is valid:

- Run: <https://github.com/thelabcorner/anvil/actions/runs/36011333908>
- Head: `a443f37c0076591dd7efd2175859630e1e20f6c9`
- Frozen implementation: `cf9306e34e98b35333b23757da88357cab92357e`
- G5B source blob: `ce8021a5f7f3d25319db1ccd22850c3144c03040`; source SHA-256:
  `910ddcf7ab8026ecca095199d921cfe823df64dd5bc9552617b9a1d73e0aa9f4`;
  prereg blob: `1a45154f26ed98b7c5bc1d15bb1b5985d456347c`; binary SHA-256:
  `ea47f30c8f315db86d4e1b94c656323df53848dd574216ec10f8eb1fc776010f`
- Frozen G5A floor: run `35985412906`, artifact ID `10801714249`, digest
  `sha256:4765b317e7c01170cffbd96b08c639e001ec1e2f317152d1f83a20a5331a3278`
- G5A source blob `2772d7eaf0a64be1fdbbb377f5d668d21dab9cbe`; corrected
  same-run replay and floor check both pass.
- The correction from invalid run to corrected run is transport/workflow-only:
  it uses unredirected authorization headers and changes no scientific arm,
  threshold, corpus, selector, backend, or implementation.
- Corrected artifact: `grotli-g5b-ordinal-36011333908`, API artifact ID
  `10812278595`, digest
  `sha256:742dc2ee8a9b683104f6c00f93f26ed132274e41c22ae5b4113aaa4ab6e266af`
- Verdict: `ORDINAL-ADVERSE`; semantic hierarchy remains open as a separate
  hypothesis, but ordinal placement is killed for this frozen arm.
- Recovered artifact directory:
  `C:\Users\SLOOSH~1\AppData\Local\Temp\openfork\g5b-36011333908-recovered-20260924`;
  machine-readable ruling, discovery/V1 rows, provenance, floor check, source
  and binary hashes were retained there. The run used Ubuntu clang 18.1.3 and
  Brotli 1.1.0; floor and invariant checks passed. Its process timing/RSS is
  diagnostic only (D3: 52.34 s, 276136 kB maximum RSS; V1: 91.26 s,
  360524 kB), not a production frontier measurement.

### Diagnostic reference-cost reproduction

Run `36061511123` (`b6a243c9657776058a37e5f0ee2eaae2ff925dff`) was queued
from `main` with 13 paired repetitions. It rebuilt the frozen candidate tag
`i10-aux-unbwt-final-v1` at `0dce534e5df40945d518c9ef4a722f4a0ecfc0e6` on
Ubuntu 24.04/clang 18.1.3/Brotli 1.1.0. Silesia completed with
`FRONT-GAP_COST`: ANVIL aux was 46,466,339 B versus xz 48,456,004 B and
Brotli q11 49,383,136 B; decode ratios were 1.7995x xz and 3.3312x Brotli,
with peak-RSS ratios 4.5717x and 2.0273x. enwik8 reproduced the byte totals
(23,537,422 B versus 24,831,648 B xz and 24,810,180 B Brotli) but its ruling
is `TIMING_BLOCKED` because the Brotli control robust CV was 0.1756; its
reported decode ratios were 3.5162x xz and 10.1499x Brotli, with peak-RSS
ratios 9.0594x and 2.3785x. A/A null CIs spanned one, but the legacy driver
did not record per-run output hashes, so this run is a diagnostic cost
reproduction, not a new frontier claim. Artifacts: Silesia `10834858417`,
digest `sha256:30574a25c718fe45828e1c6e43278f188302c66ee4975ea3e74a35cbb034ac6e`;
enwik8 `10835616518`, digest
`sha256:837cf95daa310f1e11e58813f11ae9f41a2a560a1173f372c1fd1b3fd27c1c85`.
Recovered files are under
`C:\Users\SLOOSH~1\AppData\Local\Temp\openfork\refcost-36061511123-recovered-20260924`.

## 3. Current conclusion and next actions

1. The trustworthy production frontier is still byte-oriented only for the
   legacy I9 portfolio; the auxiliary-BWT branch improves decode but remains
   front-gap in throughput and RSS. No complete production Pareto crossing is
   established.
2. G5A establishes a real representation signal: column ordering is
   material on D1-D4, but its V1 reversal and G3 failure prevent a universal
   ordering claim.
3. G5B-ORDINAL is a valid negative for positional ordinal grouping, not a
   rejection of semantic hierarchy.
4. G4 closes direct proxy expansion. The next planner attempt, if pursued,
   must be a separately frozen bounded finalist portfolio with q11 used only
   for verification, not ranking.
5. The immediate remote experiment is a same-job dense standard-corpus
   frontier run covering Silesia/enwik8, legacy and aux ANVIL, Brotli
   q1/q4/q6/q9/q11, dense zstd tiers, and xz, with paired encode/decode,
   per-process RSS, decoder size, A/A controls, and raw repetitions. The
   present standard-corpus series lacks the q1-q9 knee and full-cost
   confidence; this measurement must precede mechanism promotion.
6. After the dense frontier identifies the binding knee, run two isolated
   mechanism pilots: (a) bounded MDL/q4/q11 finalist planning, and (b) a
   paged base-plus-overlay exact dictionary with mandatory escape. Keep each
   separate from FSST, defaults, hierarchy, and the other pilot.
7. G5A's ordering result justifies locality-preserving nulls and
   reorder-after-transform controls, not another global ordinal flattening.
8. No new held-out structured corpus is currently available. Any promotion
   claim must create and lock new independent structured, mixed-validity,
   executable, and numeric validation families before measurement.

### Pending local handoff (updated 2026-09-25)

The dense-frontier vehicle and G5 paged-dictionary files remain local,
uncommitted work, so GitHub Actions cannot dispatch them until an explicitly
authorized commit/push publishes them. The previous `observe.sh` argument
mismatch is fixed: the wrapper now reads the output path as `$1`, removes stale
output, and `exec`s, matching `<wrapper> <path> -- <command>`. The workflow
deliberately keeps the immutable published-main `tools/paired_bench.py` blob
`f847c50e38d3b39b61deeb6b21bcf12e9c132fd5` rather than the local hardened blob
`bb2b2a7efcaa2959800e7ea27e7b5c6f65c1f2cd`: the pinned driver already supports
the observe hooks, the wrapper supplies freshness, and the workflow verifies
post-run bytes/SHA-256 outside timing. Adopting the hardened driver would need a
separate published tooling commit and pin change.

The G5D prototype's scientific blockers are fixed. `decode_peak_rss_kib` is now
measured at decode completion (`GetProcessMemoryInfo`/`PeakWorkingSetSize` on
Windows; `getrusage` maxrss on Linux and Darwin) with an explicit status string,
and the required invariant flags are computed rather than hard-coded: token
multiset equality from sorting each arm's ordered stream, reconstruction order
from roundtrip, component accounting from sums versus body size, fixed backend
from observed Brotli versions, family coverage from stored leaf-family tags, and
production authorization from `G5D_PRODUCTION_TRANSFORM_ID`. The G5D workflow's
required integer fields and invariant keys now match the corrected source.
Local verification: warning-clean `clang-cl /std:c++20 /O2 /DNDEBUG /W4 /WX`
compile with `psapi.lib`, `selftest` PASS for both embedded G3 and G5D, and a
smoke `measure README.md` emitted `decode_peak_rss_kib: 5416` with status
`MEASURED_WINDOWS_PEAK_WORKING_SET_AT_DECODE_COMPLETION` and all ten required
invariants true.

`docs/I10-GROTLI-G5A-RESULTS.md` already exists on published main at commit
`668beb3853d1c8aef8f156ea1e4add8f60dd4f8c` (git blob
`df57036a031b03e2c73152497dd3186d21ceea9d`, SHA-256
`e9e4240040f4d1ffe129c8c7855edf492d0432edfc93262e5bf79d68b2c7781c`,
20,210 bytes). The local `i10-aux-unbwt` branch lacks it only because it
predates that commit; no competing local copy was created. Those three values
are the G5A identity inputs required by the G5D workflow.

Non-load-bearing local checks completed: `python -m compileall -q tools tests`
PASS, `python tools/paired_bench.py --self-test-observation` PASS, YAML
`safe_load` over all 16 `.github/workflows/*.yml` PASS, and the harmonic Class A
grid recomputation matched every row exactly. A CMake configure check is blocked
locally by the missing Windows RC toolchain (`No CMAKE_RC_COMPILER could be
found`); the authoritative C++ build gate remains the pinned Ubuntu CI
workflows. No local benchmark, load-bearing build, commit, push, or production
edit was performed. Publication of the dense-frontier and G5D files is blocked
pending explicit authorization, not by a repository defect.

All CPU-heavy follow-up experiments remain reserved for GitHub Actions.
