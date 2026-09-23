# GATE RULING I9-4 — DEFINITIVE ratio-first decode multiples (Silesia / enwik8)

**Author:** `research-gate`. **Date:** 2026-09-12. **Swarm:** `anvil-i9-pareto`.
**Trigger:** coordinator `msg_9ea23373b4fd4f428f87b9a05359ac23` — three live
Silesia values in circulation; one citation of record required. Row owner:
`bench`; doc owner: `strategy`.
**Status: RULED. One definitive value per object; all others marked superseded.**
**FRONT-GAP verdict unchanged; the numbers get worse, not better.**

## 1. The objects (they are not interchangeable)

All four source CSVs are **UNCOMMITTED worktree artifacts** (label them).
Binary: `build\anvil.exe` sha256 prefix **`de9b4caf`** (1,481,728 B) per the
ratio-first lane's handoff record; the CSVs do **not** embed the binary id.
**Threads: not recorded** in the CSV schema, and `tools/bench_ratio.py` contains
no threading/affinity code (provisional 1t) — PENDING bench/PR-4 attestation.
All rows carry `timing_status = VALID_100MS_NONRECURSIVE`.

Exact recompute:

```powershell
python docs\gate-verify-i9-decode-multiples.py
```

| id | object | file set | ANVIL MB/s | ref codec | ref MB/s | multiple |
|---|---|---|---:|---|---:|---:|
| A | forced BWT direct | 7 BWT-routed Silesia | 13.015 | brotli-q11-lw30 | 146.329 | **11.243x** |
| A | forced BWT direct | 7 BWT-routed Silesia | 13.015 | xz-9e | 78.698 | 6.047x |
| B | forced BWT direct | 12-file Silesia | 12.260 | brotli-q11-lw30 | 137.722 | 11.233x |
| B | forced BWT direct | 12-file Silesia | 12.260 | xz-9e | 74.397 | 6.068x |
| **C** | **auto portfolio — DEFINITIVE Silesia** | **12-file Silesia** | **9.435** | **brotli-q11-lw30** | **137.722** | **14.597x** |
| **C** | **auto portfolio — DEFINITIVE Silesia** | **12-file Silesia** | **9.435** | **xz-9e** | **74.397** | **7.885x** |
| D | E2 auto-run BWT-routed subset (window artifact) | 7 BWT-routed Silesia | 7.197 | brotli-q11-lw30 | 146.329 | 20.333x |
| **E** | **enwik8 auto (== BWT-direct bytes) — DEFINITIVE enwik8** | **enwik8** | **5.890** | **brotli-q11-lw30** | **141.050** | **23.946x** |
| **E** | **enwik8 auto — DEFINITIVE enwik8** | **enwik8** | **5.890** | **xz-9e** | **82.831** | **14.062x** |
| E' | enwik8 BWT-direct row (same bytes, second window) | enwik8 | 6.730 | brotli-q11-lw30 | 141.050 | 20.960x |

BWT-routed 7 = webster, x-ray, mr, osdb, dickens, reymont, nci. Silesia 12 =
those + mozilla, nci, ooffice, samba, sao, xml. Ratios are byte-weighted
aggregates; decode MB/s = total input / sum of per-file `decompress_s`.

## 2. The citation of record (binding)

> **Silesia (12-file `--ratio-backend=auto` portfolio, bytes 46,446,995 vs xz
> 48,456,100): decode 9.435 MB/s = 14.597x slower than brotli-q11-lw30
> (137.722 MB/s), 7.885x slower than xz-9e (74.397 MB/s).**
> **enwik8 (auto = BWT direct, bytes 23,534,368 vs xz 24,831,656): decode
> 5.890 MB/s = 23.946x slower than brotli-q11-lw30 (141.050 MB/s), 14.062x
> slower than xz-9e (82.831 MB/s).**
> Artifacts: `tests/auto-routing.csv` + `tests/enwik8-bwt.csv` +
> `tests/ratio-first-standard.csv` (uncommitted); binary `de9b4caf`;
> threads PENDING; window PENDING (PR-4). **FRONT-GAP on decode.**

The enwik8 BWT-direct row (E', 20.960x) is the same output bytes decoded in a
second window; it is **not** the portfolio row and does not head the citation.
Both windows are worse than the 11.2x carried in the audit — the gap deepens.
(PR-4 owns which window is canonical once attested; the gate's citation uses the
portfolio row and will re-rule on bench's attestation.)

## 3. Supersessions (explicit)

1. **"11.2x / 13.0 MB/s" as the portfolio headline — SUPERSEDED.** It is
   recomputable, but it is object **A** (7 BWT-routed files, *forced* BWT
   direct). It remains valid for that subset only; the portfolio decodes at
   9.435 MB/s = 14.597x vs brotli (C).
2. **strategy's 14.60x / 7.89x Silesia — VERIFIED**, = object C; adopted as the
   definitive Silesia figure.
3. **strategy's 20.33x Silesia replacement — SUPERSEDED**; it is object D, the
   **E2 auto-run BWT-routed subset** (7-file BWT rows inside the auto run's
   window: 16.724858 s vs 9.248178 s in `bwt-backend-standard.csv`), not the
   portfolio and not the canonical window. bench (row owner) has stamped the
   canonical windows as **E2 for Silesia, E7 for enwik8**
   (`decisions/bwt-decode-multiple-canonical` v2).
4. **the audit's "enwik8 6.7 MB/s = 11.2x" — SUPERSEDED**; that text applied the
   Silesia subset multiple to enwik8. Definitive enwik8 = 23.946x / 14.062x
   (auto row); same-bytes variant 20.960x / 12.308x. Either way ≥20.96x.
5. The `tests/gate`-level earlier statement "Silesia 11.24x stands" is narrowed:
   it stands as the **subset-A** figure only (see ruling I9-1/regime §9, amended
   by this document).

## 4. What this does NOT change

- The bytes axis is untouched: Silesia auto 46,446,995 B < xz 48,456,100 B;
  enwik8 auto 23,534,368 B < xz 24,831,656 B. **Update (bench A7): the byte
  totals were reproduced exactly on the canonical binary `0D1E130B` (13/13
  roundtrip OK, `tests/ratio-reverify-i9.csv` sha256 `53880D50…`), so the
  de9b4caf-only provenance caveat no longer applies to the byte totals.**
- The classification is FRONT-GAP on decode either way; no crossing token is
  issued, and none may be from these unattested windows.
- All ratio-first artifacts remain **PENDING-COMMIT**; the figures above are not
  citation-grade until bench commits and attests (PR-4 fields: threads, window,
  reps).

## 5. Actions

- `bench` (row owner): rule which decode window is canonical for BWT-direct /
  auto; stamp thread counts and binary sha into the artifacts; re-run under PR-4
  if the windows disagree (they do: 5.890 vs 6.730 MB/s enwik8; 9.248 vs
  16.725 s on the 7-file Silesia set).
- `strategy` (doc owner): quote C and E, mark A/B/D/E' as above.
- This gate: re-rule if bench's attested window differs materially.

---

*Ruled by `research-gate`. Harness:
`docs/gate-verify-i9-decode-multiples.py`. Ledger: PART XIV §6.2 + addendum.*
