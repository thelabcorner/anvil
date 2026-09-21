# Project ANVIL Research Audit — 2026-09-07

Status: **research checkpoint, not a release note**  
Branch at audit: `blocksplit-exp`  
Primary scope: all measured ANVIL work through the revision-2 backend registry and the first direct-BWT Silesia pass.

This folder exists because ANVIL accumulated enough experiments that the project could no longer safely infer its current state from narrative memory. Several historical mistakes came from doing exactly that: generated-corpus wins were over-weighted, stale tallies were propagated, an un-attributed ratio change survived multiple sessions, and a raw `EXTENDS_FRONT` flag was once read as stronger evidence than the reference matrix justified.

The audit therefore separates **what was measured**, **what worked mechanically**, **what worked at project level**, **what failed**, **what is still only a hypothesis**, and **what should be funded next**.

## Documents

| document | purpose |
|---|---|
| [01-evidence-and-methodology.md](01-evidence-and-methodology.md) | evidence hierarchy, benchmark integrity, compiler/timing caveats, claim rules |
| [02-findings-history.md](02-findings-history.md) | experiment-by-experiment synthesis of the project so far |
| [03-current-frontier.md](03-current-frontier.md) | canonical Silesia/enwik8 position, BWT results, backend-routing headroom |
| [04-architecture-and-safety.md](04-architecture-and-safety.md) | current codec/container architecture, format state, decoder safety, implementation debt |
| [05-what-worked.md](05-what-worked.md) | mechanisms and engineering practices worth retaining |
| [06-do-not-reburn.md](06-do-not-reburn.md) | failed/closed lanes, why they failed, and conditions required to reopen them |
| [07-research-portfolio.md](07-research-portfolio.md) | ranked inside-the-box and outside-the-box investment portfolio |
| [08-next-experiments.md](08-next-experiments.md) | concrete next experiments, controls, stop conditions, deliverables |
| [09-inside-outside-the-box.md](09-inside-outside-the-box.md) | explicit opportunity-space exploration: conservative extensions vs reframings |
| [SOURCES.md](SOURCES.md) | source-of-truth artifacts and external prior-art/reference links |

## Executive conclusion

ANVIL has produced **many valid mechanism-level results but no demonstrated general-purpose frontier crossing yet**. The strongest old work improved a narrow representation, parser, model, or decoder stage while the global bottleneck moved elsewhere. The new third-party corpus program changed the project more than another round of local tuning would have:

1. The old generated JSON/JSONL/log wins are useful **self-tests and mechanism probes**, not evidence of generalization.
2. On canonical Silesia, the pre-BWT ratio path is essentially Brotli q11/lw30 plus framing on 11 of 12 files. Its one real transform win is `mr`, and even there xz is smaller.
3. On enwik8, the old ratio path is Brotli plus 31 bytes. There is no transform win.
4. The current strongest new signal is not a transform. It is **backend heterogeneity**: direct BWT beats Brotli on 7 of 12 Silesia files while losing badly on the other 5.
5. Therefore the highest expected-value near-term architecture is no longer “invent one universal compressor.” It is a **reversible representation layer plus explicit backend portfolio plus cheap content-adaptive routing**.
6. That conclusion **is now a measured hybrid result (E2/E7)**: `--ratio-backend=auto` beats xz on Silesia (46,446,995 B, −2,009,105 B vs xz; +159 B over the direct-BWT/Brotli oracle = framing) and enwik8 (23,534,368 B, −1.30 MB vs xz), combined 69,981,363 B — all sha256 roundtrip-verified. It passes the bytes axis and fails decode + peakmem → classified **FRONT-GAP (cost)**, not FRONT-CROSSING.
7. BWT backend ID 2 is **validated** (E0): `FORMAT.md` documents backend 2 with the measured-validation subsection; the deterministic malformed-payload fuzz suite includes 24 BWT adversarial + 4 golden cases and passes with **zero skips** on the current binary. The degenerate 1-byte/all-equal round-trip bug is fixed (libsais n→1 mapping + n==1 short-circuit).
8. Throughput comparisons are now **apples-to-apples (E1 DONE)**: the build is normalized to clang-cl 22.1.8 with BWT + references sharing one compiler/flags, removing the earlier MSVC-vs-clang confound. The remaining cross-axis gap is real, not a toolchain artifact: BWT decodes ~13–16 MB/s vs xz ~79 MB/s.

## Current third-party byte standings

### Silesia, 211,938,580 bytes canonical total

| codec | total bytes | status |
|---|---:|---|
| xz `-9e` | **48,456,100** | measured, exact roundtrip |
| ANVIL ratio, pre-BWT table | **49,328,971** | measured, exact roundtrip |
| Brotli q11/lw30 | **49,383,136** | measured, exact roundtrip |
| zstd ultra-22 long27 | **52,522,343** | measured, exact roundtrip |

The 54,165-byte ANVIL-vs-Brotli aggregate advantage comes almost entirely from one file, `mr`. Eleven files are Brotli plus 30–31 bytes of ANVIL framing.

### enwik8, 100,000,000 bytes canonical

| codec | bytes | status |
|---|---:|---|
| Brotli q11/lw30 | **24,810,180** | measured |
| ANVIL ratio | **24,810,211** | measured, +31 B |
| xz `-9e` | **24,831,656** | measured |
| zstd ultra-22 long27 | **25,333,695** | measured |

### Direct BWT backend on Silesia

Backend ID 2, direct transform only, wins 7/12 files but loses the aggregate because `mozilla` is catastrophic for whole-file BWT.

| property | result |
|---|---:|
| BWT direct total | 52,030,185 B |
| files smaller than Brotli | **7 / 12** |
| largest win | `webster`, −1,022,697 B vs Brotli |
| largest loss | `mozilla`, +4,017,510 B vs Brotli |
| oracle min(BWT,Brotli), using measured file sizes | ~46.45 MB |

The oracle figure is a **research target, not a result**. It motivates measuring the already-implemented `--ratio-backend=auto` router.

## The most important project-level lesson

ANVIL repeatedly found useful structure that failed to become compressed-byte or frontier value. Examples include synchronized record-period discovery, PNRA invariant candidates, finite-difference detection, topology models, and decoder fusion. The project should now treat the pipeline as a sequence of economic gates:

> **detect structure → represent it cheaply → encode it cheaply → decode it cheaply → beat a strong reference on real data**

A success at any earlier arrow is not evidence about the later arrows.

That single rule explains most of the project’s false starts and should govern all new work.

## Current strategic posture

### Protect

- canonical third-party benchmark discipline;
- exact roundtrip + strict malformed-input rejection;
- explicit transform/backend IDs;
- ablation-driven mechanism claims;
- negative-result ledger and “do not reburn” rules;
- size-based fallback/router behavior.

### Push now

1. Finish backend-2 format and fuzz completeness. **DONE (E0):** FORMAT.md backend-2 documented, BWT adversarial fuzz green, goldens in `tests/bwt-golden/`.
2. Normalize compiler/toolchain before any BWT speed claim. **DONE (E1):** pinned clang-cl 22.1.8 build.
3. Measure whole-file Brotli/BWT auto routing on Silesia and enwik8. **DONE (E2/E7):** Silesia 46,446,995 B (beats xz by 2,009,105 B), enwik8 23,534,368 B (beats xz by 1.30 MB). FRONT-GAP (decode + peakmem fail), not a crossing.
4. **Block-local routing regret — CLOSED by NEGATIVE (E4):** block routing is worse than whole-file on every file (webster +894 KB at 4 MiB despite BWT winning 10/10 blocks; mozilla/samba/ooffice/sao have zero BWT-winning blocks). Per-block model warmup dominates. Do **not** pursue block/sub-file routing; reopen only on a near-zero-warmup backend or cross-block model carryover.
5. **QLFC/LZP postcoders — NO-GO (E6, measured):** QLFC +3.23% bytes (0/13 win), LZP +0.37% (none of the BWT headline winners). Do not implement `--bwt-post=4/5` as ratio gains.
6. **Highest-EV next bet: P4.1 DEFLATE reconstruction** — the only byte win that costs no decode regression and the only route into mozilla/samba/ooffice/sao per E4. Then a fixed-point context-mixing backend (P3.1), and/or faster BWT inverse throughput.

### Hold / defer

- neural compression: separate program, not this lane;
- more generated-JSON tuning without a third-party transfer hypothesis;
- decode-only work that does not move bytes on cells where the old Route-B math already closes the lane;
- statistical zero-bit derived-parameter transforms; the project’s own math audit closed that family except for exact transforms.

## Definition of “current truth”

When this audit conflicts with older prose, use this priority order:

1. canonical current CSV / exact executable output;
2. current source + current `FORMAT.md` where they agree;
3. this audit’s explicit derivations from those artifacts;
4. `RESEARCH_LEDGER.md` for historical experiments;
5. old `docs/CONTEXT.md` / swarm briefs for historical context only.

Old documents are not deleted because their mistakes are part of the research record. They are not automatically current.
