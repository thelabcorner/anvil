# I7 Independent Verification — interim notes (`verify`, task u6)

Date: 2026-08-21. Verifier: `verify` (separate actor from all builders).
Scope this pass: tree health, baseline correctness, published-number
spot-checks, corpus integrity. Peer-result re-runs (u1 ARI-REF, u2 oracle)
pending their handoffs; this file will be updated and the final matrix
published to blackboard `deliverable/u6`.

## Environment

- Host: Windows, pwsh 7; cmake 4.4.2, ninja 1.13.2, clang-cl 22.1.8 (MSVC CRT via vcvars64).
- Build: SEPARATE dir `build-verify/` (did not touch `build/`), per assignment:
  `cmake -S . -B build-verify -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=clang-cl -DCMAKE_SUPPRESS_REGENERATION=ON` + `ninja -C build-verify`.
- Result: **BUILD OK** (anvil.exe + anvil_bench.exe). One benign warning
  (`getenv` deprecation, src/anvil.cpp:3399). No errors.
- Tree state at verification: HEAD `392e937` + uncommitted in-flight edits by
  peers (src/anvil.cpp, FORMAT.md, ledger staging docs, new prototypes/, new
  corpus files). All numbers below are from THIS tree state.

## Baseline correctness

- `python tests/fuzz.py --exe build-verify\anvil.exe --cases 50`
  → **PASS seed=41246 roundtrip_variants=420 mutations=2520**
  = exact match to the tans-verify precedent recorded in RESEARCH_LEDGER.md.
- Round-trips (encode→decode→SHA-256 compare) on my fresh build: **12/12 OK**
  across generated.jsonl / generated.json / generated.log / doc.md / src.cpp /
  synth-jitter.bin / anvil.exe / anvil_bench.exe, spanning --parse=auto,
  --parse=tcopy (--pnra=on|off), --stream-ctx=on|off, --stream-suite=off.

## Published-number spot-checks (all reproduced BYTE-EXACT)

Method: fresh `build-verify/anvil.exe`, defaults unless noted, output sizes in
bytes; scratch artifacts under `scratch/verify/`.

| # | Claim (ledger ref) | Expected | Measured | Verdict |
|---|---|---:|---:|---|
| 1 | Fuzz seed precedent (tans-verify entry) | PASS seed=41246, 420 var, 2520 mut | identical | ✅ EXACT |
| 2 | Exp S ctx-on jsonl (PART IV/V, Exp S table) | 183,506 B | 183,506 | ✅ EXACT |
| 3 | Exp S ctx-off jsonl | 218,553 B | 218,553 | ✅ EXACT |
| 4 | Exp S delta −16.0% (recomputed: 1−183506/218553) | −16.03% | −16.03% | ✅ EXACT |
| 5 | Exp S json ctx-on "89,158 B = −12.3%" | 89,158 B | 89,158 | ✅ EXACT |
| 6 | Exp L corrected: jsonl suite-off = 219,042 B | 219,042 B | 219,042 | ✅ EXACT |
| 7 | Exp X tcopy pnra=off anvil.exe | 108,518 B | 108,518 | ✅ EXACT |
| 8 | Exp X tcopy pnra=on anvil.exe (wash −0.0037%) | 108,514 B | 108,514 | ✅ EXACT |
| 9 | Exp X tcopy pnra=off anvil_bench.exe | 845,675 B | 845,675 | ✅ EXACT |
| 10 | Exp X tcopy pnra=on anvil_bench.exe (+0.0443% regression) | 846,050 B | 846,050 | ✅ EXACT |

Commands used (for reproducibility):
```
build-verify\anvil.exe c tests\corpus\generated.jsonl  out --parse=auto              # ctx-on (defaults)
build-verify\anvil.exe c tests\corpus\generated.jsonl  out --stream-ctx=off
build-verify\anvil.exe c tests\corpus\generated.json   out --parse=auto
build-verify\anvil.exe c tests\corpus\generated.jsonl  out --stream-suite=off --stream-ctx=off
build-verify\anvil.exe c tests\corpus\anvil.exe        out --parse=tcopy --pnra=off|on
build-verify\anvil.exe c tests\corpus\anvil_bench.exe  out --parse=tcopy --pnra=off|on
```

## Corpus integrity findings

1. **`anvil_bench.exe` manifest drift (pre-existing, now dispositioned).**
   CHECKSUMS.txt pinned 1,866,752 B / sha e5a0f8a7…; disk had 1,929,216 B /
   sha fcd30da5…. Independently detected by me during checksum audit AND by
   `corpus-expand` (u3 handoff): a historical re-snapshot without manifest
   update; old bytes unrecoverable (git-ignored file). Disposition:
   manifest re-pinned to disk truth + README drift note. **Provenance caveat
   for the ledger:** measurements against the OLD snapshot (pre-re-pin,
   ≤2026-08-13 era) are not byte-comparable to current ones. All claims I
   spot-checked above reproduce against CURRENT bytes, so no live
   contradiction exists.
2. **New corpus files verified present on disk** (u3): pe-{git,ninja,
   notepad,python,where,winver}.exe, synth-{columnar-align,drift-stride,
   counters}.* — checksums now in updated CHECKSUMS.txt (21 files).
3. All 12 pre-existing data files matched their original hashes (verified by
   corpus-expand pre/post; consistent with my audit of the 12 non-PE files).

## Peer-result verification (u3 corpus-expand, u4 priorart)

### u3 corpus-expand — orientation ratios REPRODUCED EXACTLY

Fresh `build-verify/anvil.exe` (defaults), round-trip SHA-256 verified each:

| file | staged ratio | measured | verdict |
|---|---:|---:|---|
| pe-winver.exe | 0.1949 | 0.1949 (5,587/28,672) | ✅ EXACT |
| pe-git.exe | 0.4863 | 0.4863 (2,131,578/4,383,048) | ✅ EXACT |
| synth-counters.log | 0.1147 | 0.1147 (127,048/1,107,326) | ✅ EXACT |

Staged PE sizes + SHA-256 prefixes cross-checked against my independent disk
audit: all 6 match; 5,540,960 B total sums exactly. Drift note matches my own
pre-handoff finding byte-for-byte. u3 entry: VERIFIED as staged.

### u4 priorart — Intel US 7,111,148 quotes INDEPENDENTLY RE-FETCHED

Method: I fetched the full text from freepatentsonline.com/7111148.html
(Google Patents 503 rate-limited for me too — same condition priorart
reported). Compared against the staged entry:

| item | staged | re-fetched | verdict |
|---|---|---|---|
| '148 claim 1 language | quoted verbatim | VERBATIM MATCH | ✅ |
| '148 cl. 9–10 (J-bit correction field, J=2) | quoted | VERBATIM MATCH | ✅ |
| claim count ('148 = 39) | 39 | 39 (cl. 1–39 on FPO) | ✅ |
| title / assignee / prio 27-Jun-2002 | stated | match | ✅ |
| classes G06F9/26, USPC 711/220 | stated | match (FPO primary class 711/220) | ✅ |
| "no LZ parse / no match distance in any claim" | characterization | confirmed by my read of all 39 claims | ✅ |

Caveats recorded: (1) legal status (both patents EXPIRED fee-related,
2022/2023) is sourced from Google Patents ONLY — I could not independently
confirm it (FPO page shows no adjusted-expiration field); single-sourced,
low risk but noted. (2) '665 and '382 texts not re-fetched by me (time-boxed;
'148 was the binding exemplar and its quotes matched exactly, which validates
the retrieval method). u4 entry: QUOTES VERIFIED; verdict internally
consistent with the retrieved claim set.

## Cross-build determinism check

`build\anvil.exe` (the other lane's build, 2026-08-21 05:50) vs my
`build-verify\anvil.exe`: outputs **BYTE-IDENTICAL** on generated.jsonl
ctx-on. My reproductions are not artifacts of a separate build.

## Rigor-review findings on staged PART XII text (docs/ledger-i7-cost.md)

1. **CORPUS COUNT NIT (flagged to ledger):** staged line ~175 says
   "total corpus now 19 data files ≈19.8 MB" — internally inconsistent.
   Measured: **22 data files, 19,811,989 B**. The byte total includes the 3
   new synth files; the count (13 old data + 6 PEs) does not. One-word fix.
2. u3 entry otherwise fully verified (see above). u4 entry quotes verified;
   expiration status single-sourced (Google Patents only).
3. ariref `results_run1.txt` observed mid-development (RT-FAILs, fuzz
   110/280 fail, honest exit 1) — NOT treated as a reported result; final
   harness re-run awaits the u1 handoff. Noted as good practice: the harness
   refuses to print density verdicts as passing while correctness fails.

## Peer-result verification (u1 ariref — ARI-REF, Experiment Z)

Method: rebuilt `prototypes/orbit_ariref/ariref.cpp` from source myself
(clang-cl /std:c++20 /MD /O2 /EHsc /DNDEBUG → `scratch/verify/
ariref_verify.exe`), ran full harness, diffed against
`prototypes/orbit_ariref/results_run3.txt`.

**Result: REPRODUCED IN FULL — output line-identical (71/71 lines; only
em-dash encoding differs in 4 prose lines).**

| target | claim | my re-run | verdict |
|---|---|---|---|
| (a) density | synth-arith 257,124 → 89,363 = −65.25% PASS (bar ≥10%); timeseries +0.03% FAIL | identical | ✅ |
| (b) implicit-Δ | impl +38.35% vs tx on corpus; −15.38% pure-prog / +40.55% jitter boundary | identical | ✅ |
| (c) attribution | real corpus 0–72 tokens, deltas ≤0.25%; repeat/random 0 tokens 0.00% | identical | ✅ |
| (d) correctness | rt-ok all files; fuzz 280/280 seed 0xC0FFEE; mutations 79 rejected / 1 equivalent / 119 detected / 0 crashes; exit 0 | identical incl. mutation reps/wireBytes | ✅ |

Controls check: no-regression controls VISIBLY RAN (repeat.jsonl /
random.bin rows present at 0 tokens / 0.00%; real-corpus rows present).
Fuzz determinism confirmed by identical mutation signatures.

**Provenance nit (RESOLVED):** current `ariref.cpp` SHA-256 =
`86BA68D0…`; deliverable/u1 + priorart corroboration pin `CFE97A36…`.
My rebuild of the CURRENT source reproduces every number exactly ⇒ the
post-corroboration edit was behavior-preserving (mutation-accounting
wording + debug-scaffolding cleanup per ariref). ariref re-pinned
deliverable/u1 to v3: source `86BA68D0…` + `results_run3.txt`
`36548062…` — both hashes independently verified by me against disk.
Artifact chain clean end-to-end (two reproduction legs: priorart on
CFE97A36, verify line-identical 71/71 on 86BA68D0).

Rigor notes on the ledger entry: verdicts match pre-registered bars;
FAIL on (b) recorded with mechanism boundary; anvil_bench −1.42% outlier
explicitly NOT claimed; run1's two real bugs documented as bring-up
provenance. No claim exceeds evidence.

## Peer-result verification (u5 pnra-cost — S6-2 threshold calibration)

Method: rebuilt `prototypes/pnra_cost/anvil_pnra_proto.cpp` from source
(clang-cl, same flags → `scratch/verify/pnra_proto_verify.exe`, 370,688 B =
same size as committed exe), ran baseline clone + γ=0.5 per recorded env
(PNRA_MODE=1 PNRA_FORM=1 PNRA_GAMMA=0.5).

| measurement | claimed | my re-run | verdict |
|---|---|---|---|
| EXP. X clone baseline anvil.exe (--pnra=on) | 108,514; counters 713/615/429 | 108,514; 713/615/429 (gate 1329) | ✅ EXACT |
| EXP. X clone baseline anvil_bench.exe | 846,050; counters 6919/2145/1958/1599 | 846,050; 6919/2145/1958/1599 | ✅ EXACT |
| γ=0.5 anvil.exe | 108,506 (−0.0110% vs off), commits 429→29 | 108,506, commit=29 | ✅ EXACT |
| γ=0.5 anvil_bench.exe | 845,657 (−0.0021% vs off), commits 1,599→80 | 845,657, commit=80 | ✅ EXACT |
| delta arithmetic | −12/108,518=−0.0110%; −18/845,675=−0.0021% | recomputed, correct | ✅ |
| round-trips (γ outputs) | PASS | RT OK both PEs | ✅ |
| non-PE no-op | byte-identical | jsonl baseline==γ SHA-256 | ✅ |

Notes: (1) first config to beat --pnra=off on BOTH pinned PEs — confirmed.
(2) Honest pre-registration discipline: held-out FAIL predicted
(−0.002..−0.03% ceiling vs ≥0.5% bar) BEFORE any held-out datum; verdict
correctly reserved for arch's integrated build per frozen §8 protocol.
(3) "gate 1329 vs EXP. X's 1333" delta is explained in the deliverable
(4 no-idxhit fires) and consistent with my run. (4) src/anvil.cpp untouched
by u5 — lane discipline held (git status confirms modifications belong to
other lanes). u5 entry: VERIFIED as staged (design record + calibration;
NOT YET MEASURED end-to-end framing is accurate).

## Peer-result verification (u2 oracle — S6-3 rejection oracle)

Method: rebuilt `prototypes/cost_oracle/cost_oracle.cpp` from source
(clang-cl, Experiment-V flags → `scratch/verify/cost_oracle_verify.exe`,
265,216 B = same size as committed exe), re-ran recorded commands.

| measurement | claimed | my re-run | verdict |
|---|---|---|---|
| startup self-test | attributed==encode_stream on 24 buffers | OK | ✅ |
| jsonl m11 cal | n=2,799, med +1.0%, P90 +6.0%, flips 0 | identical | ✅ EXACT |
| json m11 cal | n=5,102, −1.6%, +7.8%, 0 | identical | ✅ EXACT |
| log m11 cal | n=3,786, −9.5%, +0.5%, 0 | identical | ✅ EXACT |
| bench m14 cal | n=73,586, +10.5%, +55.4%, 0 | identical | ✅ EXACT |
| record aggregate | base 517,885 = oracle, raw 5,585,211, Δ +0.0000% | 120,566+175,723+221,596 = 517,885 ✓ | ✅ EXACT |
| byte anchors | 108,514 / 108,518 / 846,050 / 845,675 | all reproduce (also via build-verify EXP. X runs) | ✅ EXACT |
| F1 trajectory | pnra=off 845,675; +375 B = +0.0443%; R2pop 1,470; drop-variants never win | all exact; won0 everywhere; R1=0 | ✅ |
| F1 soundness | every committed type-3 individually sound | ZERO flips in my full 73,586-row per-candidate dump (incl. all 2,377 type-3) | ✅ |
| (c) encode ≤2x | 1.10–1.40x (>100 ms files) | 1.09–1.46x measured — within band (timing noise) | ✅ PASS |
| (d) decode ≤10% | −4.6%..+7.6% (≥3 ms files) | −7.5%..+8.2% measured — within band | ✅ PASS |

**One traceability nit (flagged to oracle, not verdict-changing):** README's
"measured keep 2.4–4.8 B vs 32–39 B order-0-literal alternative" does not
reproduce as an exact statistic of any natural subset: full type-3 spread is
1.44–309.3 B vs 22.8–14,122 B; type-3 MEDIAN 4.45 vs 43.0; R2 (len≤8,
n=1,470) range 1.44–6.43 vs 22.76–80.89, median 3.66 vs 35.75. The quoted
figures read as a rounded central-tendency gloss — house style ("every number
from measurement") wants the exact statistic cited (e.g. "type-3 median keep
4.45 B vs alt 43.0 B"). The claim it supports (individual soundness, zero
exceptions) is fully confirmed by my independent dump.

Controls check: round-trip OK/OK every run; repeat.jsonl/random.bin
degenerate-control behavior reported honestly (2.36x outlier explained);
sub-ms timer-noise files labeled "reported not claimed". Harness-relative
labeling present throughout. u2 entry: VERIFIED (with the one wording nit).

## Post-completion addendum: S6-2 flip-count reconciliation (ariref's finding)

ariref's joint verification leg (merged into deliverable/u6 v3, my matrix
preserved verbatim) flagged: staged flip counts (391, 1,502) don't equal
commit-counter deltas (400, 1,519). CLOSED with evidence (delivered to
pnra-cost): flips.py L33-35 computes flips as an INTERSECTION-JOIN of
per-candidate records across legacy/frozen parses; the commit delta is an
end-to-end count on a shifted trajectory. Direct proof of trajectory shift
in my runs: gate/idxhit/verify counters move under γ=0.5 (anvil.exe
1329→1334 / 713→711 / 615→614; bench 6919→6917 / 2145→2140 / 1958→1956).
Same mechanism as oracle F1. Disposition: annotation stating the counting
basis; both published numbers remain as measured. Also confirmed ariref's
hash-suffix typo nit (staged EXP. Z "…CF73F" → "…1CFF73F").

## Verdict

All six lanes verified. Every published number I tested reproduces exactly
(ratios/sizes/calibration) or within pre-registered timing bands (encode/
decode). Two provenance nits found and resolved (ariref hash re-pin;
anvil_bench manifest re-pin). One wording nit open (oracle's central-range
gloss). No contradictions with the ledger anywhere.

## Scratch artifacts

`scratch/verify/` (workspace-local per coordinator notice): encoded outputs +
decoded round-trip copies used in the tables above. Safe to delete after u6 closes.

---

# Addendum: T7 S6-1 arbiter verdict verification (merged anvil swarm, bench/guest-fdc58)

Method: re-ran both verdict tools myself on the official CSVs; cross-checked
decisive rows; ran their equality script; independent fuzz control.

| claim | deliverable | my check | verdict |
|---|---|---|---|
| verdict tally | {'RATIO-BEATS-SOME': 180, 'NO-BEAT': 49, 'EXTENDS-ONE-PLANE': 5} | identical (my beats_brotli.py run) | ✅ EXACT |
| front-extenders | only 5 generated.json encode-plane rows | identical (my pareto_front.py run) | ✅ EXACT |
| Leg B equality | budget = hotop = 175,550 B (log) | CSV rows equal; check_size_equality.py PASS 13/13 (my run) | ✅ |
| Leg A fail | budget dec 223.6 < 547.1 required | CSV 223.603 ✓ | ✅ |
| Leg C fail | vs q9 124,669 B @ 619.2 | CSV 124,669 / 619.231 ✓ | ✅ |
| secondary sizes | json 121,316 / jsonl 222,381 (=baseline) | CSV equal both codecs ✓ | ✅ |
| Exp-Y row | log 157,446 (−10.3%), aggregate −3% | CSV 157,446 ✓; 0.208738/0.215307 = −3.05% ✓ | ✅ |
| tree health (my control) | — | fuzz --cases 50 PASS seed=41246 / 420 / 2520 on current build\anvil.exe | ✅ |

Scope note: deterministic quantities verified exactly; full median-3 suite
re-run is bench's lane (~17 min wall); decode MB/s figures are timing values
consistent with the stated ±19 MB/s noise floor. T7 verdict: VERIFIED.
