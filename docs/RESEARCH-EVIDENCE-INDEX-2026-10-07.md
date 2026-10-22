# ANVIL — Research Evidence Index and Publication Claim Register

**As-of:** 2026-10-07. **Status:** navigation and interpretation, **not** a frozen experiment, proof of independent replication, or substitute for downloaded artifacts.
**Canonical priority:** pinned CI source + outputs → specific experiment prereg/ruling → original ledger entry → paper-style synthesis → README → historical handoffs.
**Default claim:** **0 confirmed general-purpose complete Pareto crossings** in the last frozen Class-A matrix (435 dominated, 28 degenerate, 5 unresolved of 468). R2 Gate A Q1a-XRUN **PASS** does not imply R2 Gate B corpus admission or I19 efficacy.

## Quick map — what may be cited

| ID | Experiment and location | Exact source of record | What evidence supports | What it does not support |
|---|---|---|---|---|
| H9 | I9 Silesia/enwik8 [results](anvil-i9-findings.md) | Original frozen I9 canonical ledger/corpus manifests | Selected archive-byte lead over named q11/xz configurations | Whole speed/memory frontier crossing |
| H10 | I10 BWT [results](I10-AUX-UNBWT-RESULTS.md) | Same-runner frozen I10 results | Auxiliary-index inverse BWT improves ANVIL decode for charged bytes | External-codec dominance |
| HG | I10 Grotli [G1](I10-GROTLI-G1-RESULTS.md), [G3](I10-GROTLI-G3-RESULTS.md), [G5A](I10-GROTLI-G5A-RESULTS.md), [G5B](I10-GROTLI-G5B-ORDINAL-RESULTS.md) | Experiment-specific frozen matrix and artifacts | Causal locality ordering on selected structured discovery data | Generality, G4 planner win, generic ordinal superiority |
| R2A | Q1a-XRUN | [Run 37694740386](https://github.com/thelabcorner/anvil/actions/runs/37694740386), `c93ce7b5ebff801bd0088b10a13a2cb962f7ebc3` | Attested pipeline Gate A PASS; 135/135 Q1a checks | Gate B / admissible independent Class-A corpus |
| N16 | I16 residual fusion [adjudication](I17-I18-SOURCE-ATTESTED-EXPERIMENT-ADJUDICATION-2026-10-07.md) | [Run 37701888318](https://github.com/thelabcorner/anvil/actions/runs/37701888318), `9ca860f45d7dad1b617267372e378f18a93f2721` | Synthetic time-series AVI6 120,884 B vs q11 134,718 B | Beats q5 (q5 120,594 B); mechanism novelty |
| N17 | I17 fast profile [prereg](I17-FAST-PROFILE-PREREG-2026-10-07.md) | [Run 37701773624](https://github.com/thelabcorner/anvil/actions/runs/37701773624), `a26d770c400cd05b26195180ee4da157a8ee9c73` | Work/ratio tradeoff, synthetic 7-file results and 14 roundtrips | General-purpose domination or byte equivalence on q11-friendly inputs |
| N18a | I18 failed first run | [Run 37702906749](https://github.com/thelabcorner/anvil/actions/runs/37702906749), `40cdb8412dc4ff86c6171583a9d26f7c97c96196` | Compile-path regression / **invalid premeasurement** | Any codec efficacy |
| N18 | I18 [complete closeout](I18-BUDGET-GATED-FUSION-RESULTS-2026-10-07.md) | [Run 37703141058](https://github.com/thelabcorner/anvil/actions/runs/37703141058), `7ec50e8dadd1b3cc12d815c80007a9fcc2bfd490`, artifact **11518358032** | AVH2 19,445 B synthetic arithmetic; exact 21 roundtrips, paired speed comparisons and limited RSS | Fresh real-source efficacy, independent generality, decoder-only binary parity, statistical frontier certification |
| N19-0 | I19 Phase 0 [protocol](I19-PHASE0-FOUR-ORIGIN-SNAPSHOT-PROTOCOL-2026-10-07.md) | [Run 37704571734](https://github.com/thelabcorner/anvil/actions/runs/37704571734), `c7e423e24ab94e1156f1245c1a4a06397ef15a70`, artifact **11519365364** | Four HTTP original entity bodies acquired and SHA-verified | Immutable upstream database identity, permanent archival, holdout blindness |
| N19-0b | I19 Phase 0b [closeout](I19-PHASE0-SOURCE-ATTESTATION-CLOSEOUT-2026-10-07.md) | [Run 37704897319](https://github.com/thelabcorner/anvil/actions/runs/37704897319), `c09375511cc56f582c5147204bdaa2a2450e3e4f`, artifact **11518883939** | Four exact typed projections totalling 112,620 B; independent content audit | Parent DLY/JSON exact reconstruction, heldout validation, ANY I19 compression result |

**Artifact expiration:** I19 acquisition and semantics artifacts scheduled **2027-01-05**; preserve original bytes under independently attestable, durable custody before claiming long-term replication. Record the SHA of each archive *and internal content*, not merely the workflow UI. When a GitHub artifact metadata digest is quoted, label it as GitHub-reported rather than independently computed.

## Science vs implementation inventory

- **Main checkout:** `i10-aux-unbwt` is a legacy active research branch with substantial unrelated dirty files and build outputs; do not reset or try to merge all branches to make the latest experiment “current.” The main `src/anvil.cpp` executable is not assumed to contain AVI6/AVH2.
- **Frozen I16:** `research/anvil-i16-residual-fusion-20261007` (source at N16).
- **Frozen I17:** `research/anvil-i17-fast-profile-20261007` (source at N17).
- **I18 documentation:** `research/anvil-i18-budgeted-fusion-20261007`, research branch head reported `c943ae3`; **measured** codec source is N18, not necessarily the documentation head.
- **I19 provenance + admission tooling:** `research/anvil-i19-origin-lock-20261007`, latest audited doc/tool checkpoint `0509898` prior to this documentation campaign. Branch-local replacement of the registered `anvil-i18-budgeted-fusion.yml` is a **dispatch bridge** and must **never** be merged over canonical I18 CI.
- **Current synthesis:** [ANVIL research synthesis](ANVIL-RESEARCH-SYNTHESIS-AND-OPEN-QUESTIONS-2026-10-07.md) — narrative with methods, limits, precise hypotheses and references.
- **Experimental controls:** [remote benchmark protocol](github-actions-benchmark-protocol.md), [operational GitHub Actions guide](GITHUB-ACTIONS-BENCHMARKING.md), [I19 design](I19-REAL-NUMERIC-WORK-BUDGET-EXPERIMENT-DESIGN-2026-10-07.md), [oracle draft](I19-PHASE1-DECODER-CHEAP-RECONSTRUCTION-ORACLE-DRAFT-2026-10-07.md), [admission validator](../tools/i19_validate_efficacy_admission.py).
- **Historical experiments:** [RESEARCH_LEDGER](../RESEARCH_LEDGER.md) is append-only in spirit, [CONTEXT](CONTEXT.md) is historical, [Pareto strategy](ANVIL-PARETO-FRONTIER-RESEARCH-STRATEGY-2026-10-07.md) is ideation, [frontier composition map](ANVIL-FRONTIER-COMPOSITION-RESEARCH-MAP-2026-10-07.md) contains time-of-writing I14 intentions.
- **Copyright and source-attribution:** [I19 source closeout](I19-PHASE0-SOURCE-ATTESTATION-CLOSEOUT-2026-10-07.md), [source shortlist](I19-REAL-CORPUS-SOURCE-SHORTLIST-2026-10-07.md).

## Machine-checkable experimental identity (next release requirement)

Each future efficacy bundle should provide a single versioned machine-readable **experiment descriptor** in its artifact, with:
- `experiment_id`, `stage`, `status`, prereg commit/blob hash and immutable decision rule;
- exact code commit/tree hash and compiler/flags, linked dependency identities, runner architecture;
- source `origin_id`, publisher, release, original HTTP response SHA/size and conversion script SHA;
- typed `dtype/endian/shape/missingness`, data bytes SHA, and source-to-typed lossless reconstruction policy;
- unique split and contamination/exposure state; explicit `heldout_sealed=false` unless independently attested;
- baseline codec *version* plus window/block/frame and optimized or reference implementation identity;
- encoded format, mode, complete byte-count source, archive and decoded output hashes;
- individual raw paired timing samples, digest-observation method, workload selection, RSS method, decoder-only footprint and uncertainty;
- GitHub workflow run/attempt/jobs/artifact IDs and archive hash, retention/archival custodian, license and citation.

No claimed frontier status may be inferred solely from the descriptor, by a metadata schema validator, or from CI `success`. The descriptor must pass an independent reader-side recomputation against archived bytes.

## Reproduction sequence (lightweight developer; expensive steps only in Actions)

1. Checkout exact measured source commit; read its frozen prereg and original source-ID map. **Do not assume the latest research branch head is identical to tested source.**
2. Download the named run artifact, verify archive digest **if accessible**, internal file hashes, provenance and exact source/corpus hashes.
3. Execute the experiment-specific independent evidence auditor; distinguish whether it verifies source hashes, typed projections, roundtrips, or actual codec outcome.
4. Inspect `benchmark.tsv` / `decision.json` / raw per-repetition evidence for the original complete-wire and runtime claims. A narrative Markdown summary cannot supply missing paired samples.
5. For a rerun, use the frozen Action with matching source and dependencies; compare deterministic bytes/hashes first, then timing **within** that same run under the statistical protocol. Do not substitute cross-run MB/s.
6. Re-adjudicate based on preregistered acceptance gates, record failed attempts, contradictions and all non-wins. Protect origin splits and heldout custody.
7. Publish a versioned erratum rather than editing a signed historical result in place.

## Submission readiness checklist

| Item | Status as of this document |
|---|---|
| Historical frozen Class-A ruling | CLOSED: 0 complete crossings |
| I18 independent source / CI evidence on synthetic seven | CLOSED (discovery only) |
| I18 broad fuzz and sanitizer/malformed-wire hardening | OPEN |
| I18 matched memory + decoder-only binary/reference comparisons | OPEN |
| I19 original response hash/typed projection check | CLOSED (Phase 0/0b only) |
| I19 rights/source release attribution and immutable long-term original archival | OPEN |
| I19 unexposed independent sealed heldout and multiple discovery origins | OPEN |
| I19 preregistered typed-codec benchmark with matched execution budget | NOT RUN / BLOCKED |
| I19 independent validation and heldout efficacy | NOT RUN |
| Novel mechanism beyond crowded numerical-transform prior art | NOT ESTABLISHED |

**Internal publication stance:** write “source-attested synthetic discovery” for I18; “reproducible data preparation” for I19; **never** “world-leading lossless compression,” “general-purpose Pareto victory,” or “novel mathematical codec” without later source-controlled independent evidence.
