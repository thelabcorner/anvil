# S6-1 VERDICT SKELETON — v9 (research-gate)

> Provenance: authored live in swarm anvil-i7-decode blackboard
> `deliverable/verdict-skeleton-s6-1` (key v9). Persisted to repo after
> swarm-store outage. Aligned to pre-reg s6-1.md v1.2. Remaining placeholder:
> bench t7 only.

RECONCILIATION CLOSED: fuzz-verify closure per pre-committed ruling —
ATTRIBUTION 120/120 clusters accounted (110 head-classified as precision
transitions with corrected at-head lookback; 10 gap-merge artifacts
deep-scanned: synth-columnar-align 2/2 matching declared multiset;
generated.jsonl 36/36 declared flips located, the 37th byte-context candidate
excluded as coincidental re-encoded-body collision); 0 undeclared flips. COUNT
DRIFT BENIGN: arch's 23rd file = README.md (outside corpus manifest).
HOLD LIFTED; bench frozen-for-bench pinged. Citation:
deliverable/s61-flip-manifest + scratch/fuzz-verify/{reconcile_budget_diffs.py,
deep_attribution.py, flag-controls.json}.

GATE REVIEW OF ARCH'S VERDICT DOC v3: CONFORMS (mechanism per constants;
controls executed; flips restated precisely; classification implementation-era;
sequencing respected). Tie-break ruling recorded (keep fc23d9a; selection
determinism → FORMAT.md sign-off). Bench arbiter spec grounded:
compressed-bytes EQUALITY per file + decode-within-noise.

## §0 BUILD IDENTITY
- Verdict build: fc23d9a (--hotop-budget=on|off default OFF;
  encode_stream_budget() on 9 hot-op book streams)
- Baseline binding: tests/benchmark-suite.pre-s61.csv @ HEAD 392e937
- Verdict rows: --parse=hotop --hotop-budget=on --hotop-rlzp=off
- C_decode calibration: t3 fits fixed pre-measurement (raw 0.1 bulk / rANS 6.0
  / huffman 4.3 / defexc 3.2 / ctx 7.5 ns/B); KNOWN LIMITATION: flat rANS fit
  misses size-dependent symtab effect (§6.4)
- Selection determinism: FORMAT.md documents tie-break orders (legacy: cu
  order; budget: candidate order) — <format sign-off ref>
- Wire surface: WIRE-INVISIBLE

## §1 CONTROLS — ALL PASS
flag-off byte-identity vs freeze-HEAD (THE control; log 175,550 EXACT);
budget-on SIZE-equality 22/22 (no raw flips); round-trip 308/308 incl.
truncation; fuzz 910/7280 PASS; ASan+UBSan clean; old-file compat via goldens;
flip manifest RECONCILED (120/120 clusters, 0 undeclared, count drift benign).
Bench t7 arbiter: compressed-bytes EQUALITY per file + decode-within-noise —
<PENDING>

## §2 LEG A ≥547.1 MB/s: FAIL (zero raw-flips; decode unchanged within noise —
decode-perf median-7 234.4 vs band 231.6-240.4). Bench confirm: <PENDING>
## §3 LEG B ≤175,550 B: MET AT SIZE-EQUALITY. Bench confirm: <PENDING>
## §4 LEG C q9 both-planes: FAIL. pareto: <DOMINATED pending>
## §5 SECONDARY FILES: <PENDING bench — expected size-equal>

## §6 ATTRIBUTION (final narration)
1. FLIPS: ZERO raw-flips. Budget output SIZE-identical to legacy on all 22
   files. Byte-level: 139 precision flips across 20 files, ALL exact-L-ties,
   logical_eq=1 + dec_eq=1 (zero token/residual change). Wire-level
   reconciliation INDEPENDENTLY CLOSED: 120/120 diff clusters attributed, 0
   undeclared; count drift = README.md.
2. WHY (arithmetic necessity): λ=0.01 B/μs decode term spans ~0.02-0.66 B on
   10-20 KB streams — decides only near-exact ties; Linux trade implied WTP
   460.2 B/μs = 4.66 orders above λ (machine-verified); lits short ~4,462x;
   masks/resid raw trades ratio-infeasible under bar B by ~4 orders.
3. WHAT WAS TESTED/ANSWERED: the CONSERVATIVE-BUDGET question — no free or
   near-free decode wins at λ=0.01 on this host. Small-λ limit behaving exactly
   as its constants say. NEITHER "formulation discredited" NOR "Linux trade
   reproduced."
4. FINE-GRAIN CAVEAT (recorded): precision flips carry measured ~5-12%/B
   small-stream delta (rANS-4096 symtab pressure); end-to-end ~0.001% of
   decode — inside noise; flat-6.0 limitation recorded for future pre-regs.
5. Ratio held at size-equality because selections never changed wire cost —
   router safety claim CONFIRMED.
6. Isolation: rlzp=off rows; wire-invisible; no other mechanism in build.

## §7 CONFOUNDS
timing CV (inside 5-19% band — no signal); allocator/setup (n/a — decode path
untouched); CPU drift (paired same-build); build skew (flag-off tie); Exp-Y
co-presence (rlzp=off).
ANCHOR-INTEGRITY SENTENCE (mandatory): "The Linux crossing was measured on a
different, larger log where ANVIL started ~12% ahead of q9; Windows
generated.log starts 41% behind. This verdict tests the mechanism's trade on
Windows references; it neither reproduces nor contradicts the Linux outcome."

## §8 VERDICT LINE (A-FAIL branch, final form)
"S6-1 VERDICT: LEG A FAIL / LEG B MET AT SIZE-EQUALITY / LEG C FAIL / TIER-2
NOT MET. The whole-codec budget at frozen λ=0.01 produced zero raw-flips —
output size-identical to legacy on all 22 files (139 precision flips, all
exact-L-ties, decoder-equivalent content, wire-level reconciliation 120/120
clusters / 0 undeclared) — because the frozen constant encodes a
willingness-to-pay ~4.66 orders below the Linux decision that motivated the
experiment: S6-1 answered the conservative-budget question (NO free or
near-free decode wins at λ=0.01 on this host). Recorded implementation-era,
NOT a math failure: formulation reproduces faithfully, EXP. L's J-faithfulness
stands, baseline already sits at the J-optimum at λ=0.01. Measured explanation
for bar-A unreachability via stream budget alone: t3 floor profile (CRC32 44%
+ masks/resid materialization 26%; raw masks/resid trades ratio-infeasible
under bar B by ~4 orders). Tie-break ruling recorded: fc23d9a kept; selection
determinism spec'd in FORMAT.md. DO-NOT-RE-BURN SCOPE: retires
stream-budget-at-λ=0.01-as-decode-lever on this host; does NOT retire higher-λ
budget selection (own pre-reg; reference λ ~44.6 B/μs) nor decoder-floor legs
(S6-1b v2, sanctioned continuation). Bench t7 confirmation: <PENDING>"

Skeleton owner: research-gate.
Version history: v1 freeze → v2 tiers → v3 erratum sync + wire-invisible +
golden-set mapping → v4 corpus/amendment-4-operative/baseline-chain → v5
interpretive (mdvar-already-raw) → v6 λ-limit precision + arch outcome → v7
evidence correction (size-vs-byte) + tie-break ruling + determinism item +
fine-grain caveat → v8 flip-manifest reconciliation folded → v9 fuzz-verify
closure complete; only bench t7 placeholder remains.
