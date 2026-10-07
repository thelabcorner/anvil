# ANVIL I19 — Source-attested Phase-0/0b closeout

**Date:** 2026-10-07. **Classification:** SOURCE-LOCKED / TYPED-PROJECTION-VALIDATED / EFFICACY-BLOCKED. **Not a compression result, independent heldout certification, novel mechanism, or Class-A Pareto crossing.**

## Exact GitHub Actions evidence

- Acquisition [run 37704571734](https://github.com/thelabcorner/anvil/actions/runs/37704571734): SUCCESS, source `c7e423e24ab94e1156f1245c1a4a06397ef15a70`. 18/18 contract tests, four SHA-256 response-body checks, no encoding. Artifact ID `11519365364`, ZIP SHA256 `6cc52833320758c5b09d8bc7376a5ab8e1785828db847966a921e3f462939f8c`.
- Typed semantics [run 37704897319](https://github.com/thelabcorner/anvil/actions/runs/37704897319): SUCCESS, source `c09375511cc56f582c5147204bdaa2a2450e3e4f`. 24/24 synthetic contract tests, exact byte-lock checks, four nonheldout typed outputs; UCI ZIP opaque SHA check only. Artifact ID `11518883939`, ZIP SHA256 `904682d6a289d0e91673640905cf8a80ff2eab87bc47f68a1418db6a42ee9106`.
- Phase-0b independent evidence auditor: `tools/i19_phase0b_evidence_audit.py` rehashes every typed byte stream, validates source/provenance, checks SCIENCE GATE flags, enforces allowlisted artifact contents and rejects heldout payload leaks. Result: `PHASE0B_EVIDENCE_AUDIT_PASS` with **112,620 typed bytes**, **four streams**, **no efficacy**.
- Both artifacts expire **2027-01-05**. Downloaded read-only working copies are retained outside all worktrees at `../ANVIL-I19-EVIDENCE-20261007/` and `../ANVIL-I19-SEMANTIC-EVIDENCE-20261007/`. Neither local copies nor expiring Actions artifacts are durable, immutable long-term archives.

## Original sources: exact raw response-body identities

| Split | Dataset | Raw bytes | SHA-256 |
|---|---|---:|---|
| Discovery | NOAA USW00003947 .dly | 3,839,940 | `860de07f2beb77d1a0c293c9b5985d1ff40f2c051550250a81812896366e4329` |
| Validation | USGS station 01646500 daily water discharge | 297,561 | `58a5ded558dcd68f9ca3199bbda8fe73a01717d57a954c34cc8a595aa6807cc7` |
| Validation | NASA POWER point daily T2M | 67,388 | `8cba63a1b19fa9e914a5895285b29d55dd0b562d26e3163598d68c64a933009d` |
| Provisional pseudo-heldout, not sealed | UCI electricity dataset 235 ZIP | 20,640,916 | `9f84b46ade8a2d8e1286ec4b2b6c2987a45a755c59f263be3b3b3d10dfbda3ff` |

The bytes represent HTTP *entity bodies*, not any provenance claim about unobservable upstream database snapshots. Fixed historical API queries can be revised. NOAA station history may change and is admissible only as the captured byte-identical snapshot. USGS legacy WaterServices is due to retire in 2027; direct URL replay is not a durable source of evidence.

## Derived typed streams — exact selected numerical values, NOT original-source compression

| Role | Stream | Type | Samples | Typed bytes | SHA-256 |
|---|---|---|---:|---:|---|
| Discovery | NOAA TMAX | int16le | 20,119 | 40,238 | `5fa00acbcc1d7e6f7341158e778dc1859cfb9aa838b7ab5617d214b37e968674` |
| Discovery | NOAA TMIN | int16le | 20,119 | 40,238 | `74186bb2e58580be7b011a00e6bde9694a215d8e31d6d9ea679804137dcadf1f` |
| Validation | USGS discharge | int32le x1000 | 4,018 | 16,072 | `43216110c8a695493117902e066b8c42ce05c1b252d8e304fcc22ef875217590` |
| Validation | NASA T2M | int32le x1000 | 4,018 | 16,072 | `83d9d384e4d2063724769b078cbac51b23400a0a8ed10c40d3e3a23870684b49` |

NOAA values preserve all 31 day positions per monthly record and encode missing sentinel `-9999`; 393 missing slots were present in each TMAX/TMIN stream. The missing day slots are *not* real observed values; naive difference modeling across these boundaries can create artificial innovation patterns. Source DLY station/element/month flags, other elements, and raw layout are **not** in the typed outputs. USGS/NASA fixed-point conversions use exact `Decimal` scaling and fail for non-representable values. Their JSON schema, times, API metadata and all other fields likewise are not reconstructed from typed bytes.

This is a **typed numerical projection** comparison class. Do not claim byte-exact lossless compression of the source DLY/JSON unless a separate byte-identical source-reconstruction map and its complete archive cost are measured.

## Holdout integrity — explicit failure to seal

The UCI ZIP was included in a downloadable standard GitHub Actions artifact. Its content has not been opened by Phase-0b, but it is objectively observable by authorized viewers (and may be publicly visible). This source is therefore **not a certified blinded holdout**. Treat it as an origin-labeled *pseudo-heldout/provisional reserve*; do not count it toward the independent sealed Phase-3 gate. Obtain a completely unexamined independent source with access controls *before* the final preregistration, or establish defensible blinded custodianship and leakage proof; a file name and boolean do not provide a seal.

The validator currently checks declared `frozen` and source-family separation, not cryptographic access controls. Its structural PASS alone is insufficient to establish heldout credibility.

## Licensing and attribution research

- [UCI electricity dataset 235](https://archive.ics.uci.edu/dataset/235/individual+household+electric+power+consumption) shows **CC BY 4.0**, attribution required; cite Georges Hebrail and Alice Berard, DOI `10.24432/C58K54`. This does *not* clear other UCI datasets with conflicting restrictions.
- [USGS copyright guidance](https://www.usgs.gov/faqs/are-usgs-reportspublications-copyrighted) says USGS-authored information/data is normally US public domain with requested attribution, subject to third-party-content exceptions; [Water Data citation](https://waterdata.usgs.gov/citation/) supplies recommended site/source citation and access-date format.
- [NASA POWER referencing](https://power.larc.nasa.gov/docs/referencing/) requests project and data citations including version/access date and redistribution notification. Need inspect response metadata to fix product/version accurately before publication.
- NOAA original station data rights and release/version attribution need verification beyond the open data directory; research-use availability is not permission to omit correct provenance or derivative attribution.

**No irreversible mutation:** primary ANVIL and earlier I18 worktrees untouched; only the I19 research branch is modified. The registered I18 workflow filename is a **branch-local GitHub-dispatch bridge**, not a replacement for I18 in main. Never merge this bridge over the normal I18 workflow.

## Adjudication and next science gate

**Result:** Four raw-source SHA acquisitions and exact typed projections succeeded; complete byte and dtype evidence audited. **Class-A Pareto crossings remain 0.**

**Still blocked:** immutable archive outside Actions expiry, clear rights/attribution, stronger discovery and negative controls, clean sealed holdout, admitted frozen manifest, and a fully pre-registered same-runner typed-codec baseline and oracle-work comparison. No compression ratio, decode speed, encoding time, memory, or binary-size improvements have yet been measured on I19 real data.
