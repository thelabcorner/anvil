# Ledger standing rules — Gorilla / vXOR / timeseries-auto-detect entries

Author: `ledger` (scribe). Issued 2026-08-21 per the coordinator's GROTLI ×
GORILLA × ANVIL research briefing. Applies to ALL future Gorilla / vXOR /
timeseries-auto-detect entries staged for RESEARCH_LEDGER.md (PART XIII and
beyond). These rules operationalize the briefing's ledger directive; they bind
staging, not experimentation.

**R1 — Derivation-class tag mandatory on every figure.** Each number in a
staged entry carries exactly one tag:

- `{measured-exact}` — produced by executing code on a stated input,
  reproducible from the recorded command.
- `{measured-approx}` — measured with a stated approximation/confidence
  caveat (sampled, H0 estimated, load-sensitive timing).
- `{modelled-capacity}` — analytic/arithmetic capacity bound NOT obtained by
  execution (e.g. 5-tier DoD bit counts, Shannon-window derivations).

Untagged numbers are bounced back to the author before staging.

**R2 — Headline gate.** No DOMINANT / Pareto / front-crossing headline may
appear in an entry unless the underlying comparison includes a MEASURED
smooth-telemetry f64 result (the Gorilla sweet spot). Modelled-capacity
numbers alone cannot support a dominance claim — the project's
no-claim-without-measurement norm, unchanged.

**R3 — Pre-registration discipline.** The three coordinator-named gates are
frozen BEFORE any harness code lands, and entries for un-pre-registered
mechanisms are refused staging per house style:

- (a) Gorilla sweet-spot definition (smooth f64 only; random must remain a
  measured 0%-win negative with CI tripwire);
- (b) vXOR record-aligned frame-detection target vs the greedy-failure lesson
  (86% vs 17–23%);
- (c) ε_strict + AUTOSQUEEZE + ASCII-guard as mandatory no-worse guards.

**R4 — Provenance of ported figures.** grotli.ts / FINAL.md live OUTSIDE this
repo. Any figure ported from them must be re-executed inside the workspace
against our own harness before it may be tagged `{measured-exact}`;
quoted-from-external numbers are tagged `{external-reported}` and can never
back a claim.

**R5 — Cross-lane tie-ins.** ARI-REF transmitted-Δ results keep their EXP. Z
conventions (token-economics harness, no Pareto claim) until a real container
lands. Gorilla comparisons on synth-arith / synth-timeseries /
synth-columnar-align must state tier-matching explicitly (strategy's
matched-tier Pareto table).

**R6 — Timestamps lane honesty.** Delta-of-delta timestamp results inherit the
Gorilla paper's own framing: report the cadence/σ assumptions next to every
reduction figure (the 98.3% headline is meaningless without its 60 s-cadence,
σ=0.3 s context).
