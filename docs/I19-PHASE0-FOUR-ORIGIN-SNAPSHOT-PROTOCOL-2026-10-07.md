# I19 Phase 0 — Four-Origin Snapshot Protocol

Date: 2026-10-07. Branch: research/anvil-i19-origin-lock-20261007.
Scientific status: NO I19 EFFICACY MEASURED; NO HELDOUT CERTIFICATION; NO FRONTIER PROMOTION.

## Purpose

I18 is a valid synthetic discovery, not evidence of independent numerical generalization. Phase 0 acquires reproducible exact source-body snapshots and SHA-256 hashes. Selection is fixed in prototypes/i19-origin-lock/sources.json. The acquisition result is NOT automatically an admitted or frozen manifest. The output candidate-manifest.UNFROZEN.json sets frozen=false so efficacy tools must reject it.

## Four proposed origins

| Role | Origin | Dataset | Scientific limitations |
|---|---|---|---|
| Discovery | NOAA/NCEI | GHCN daily station USW00003947 | Mixed elements, flags, missing values; one station is not a population |
| Validation | USGS | Potomac site 01646500 fixed-date daily discharge | Legacy WaterServices endpoint retires in early 2027; source must be snapshotted |
| Validation | NASA POWER | Daily modeled T2M, fixed coordinates and 2000–2010 | Modeled/reanalysis, not an independent station reading; climate process dependence possible |
| Heldout | UCI dataset 235 | Individual household electricity ZIP | Access to a normal GitHub artifact is not a cryptographic heldout seal |

NOAA, USGS, NASA, UCI are distinct publishers/dataset families, but measurement independence is not guaranteed. Dataset 360 (UCI Air Quality) is deliberately excluded because of conflicting commercial-use/license statements.

## GitHub Actions protocol

Manual workflow: .github/workflows/anvil-i19-phase0-origin-acquisition.yml.

**GitHub manual-dispatch registration:** GitHub does not register a new
workflow_dispatch workflow ID until it exists on the default branch. For this
I19-only branch, .github/workflows/anvil-i18-budgeted-fusion.yml is replaced
with a two-step bridge that invokes the I19 reusable workflow via
workflow_call. Dispatch the *registered I18 workflow ID* with --ref pointing to
research/anvil-i19-origin-lock-20261007. That run must show only the Phase-0
job and may never run I18. Do not merge this temporary filename overlay into
default: the original I18 workflow is preserved on the measured I18 branch.

1. Checkout exact commit, verify Git blob identities and run cheap contract tests.
2. Fetch each nominated HTTPS response with host-restricted redirects, 60-second request timeout, source-specific byte cap, no credentials.
3. Save original downloaded entity bytes in raw/<source-id>.source and record SHA-256, size, timestamp, URL, HTTP metadata, source Git commit, assigned split, and pending rights review.
4. Refuse unexpected redirects, partial files, oversize, empty responses, and HTTP errors. Capture failures without silently substituting another source.
5. Independently rehash raw files and retain full evidence artifacts, including failures, for 90 days.
6. Never run compressors, preprocess measurements, decode heldout, adjust candidate thresholds, or report Pareto efficacy during Phase 0.

The actual GitHub artifact is NOT immutable beyond retention; retain a durable, immutable copy before freezing the corpus. The normal Actions artifact is visible to permitted readers: heldout_unsealed=true is an intentional disclosure, not an endorsement of leakage.

## Next admission gates

### Phase-0b: separately source-locked numeric-semantic gate

The successful Phase-0 acquisition is frozen by the hash-only
prototypes/i19-origin-lock/source-lock.json. The exact run is 37704571734,
commit c7e423e, artifact 11519365364. The artifact is retained locally
outside the working tree under ANVIL-I19-EVIDENCE-20261007.

The follow-up tools/i19_phase0b_semantic.py checks all four source hashes
and exact sizes, then parses only three non-heldout sources. NOAA emits
independent TMAX/TMIN signed-16 arrays in monthly 31-slot order; USGS emits
fixed-point daily discharge signed-32 values; NASA emits fixed-point daily
T2M signed-32 values sorted by date. Decimal conversion is exact and fails
if precision exceeds scale 1000. The UCI ZIP receives only an identity hash
check: **no archive listing, extraction, record parse, or numeric examination**.
The typed stream projections cannot reconstruct the source text/JSON and
therefore are never valid standalone raw-byte lossless competition evidence.

Workflow .github/workflows/anvil-i19-phase0b-semantic.yml downloads the
exact previous Actions evidence using read-only Actions permissions. Its
retained output includes only typed nonheldout projections, status, provenance
and a semantic report, not the heldout original. The branch-specific legacy
dispatch bridge temporarily targets this new workflow. Phase-0b has no codec
measurements and cannot advance a Pareto claim.

- [ ] All four sources acquired and their saved original-byte SHA-256 verified.
- [ ] Check upstream response semantics, API data/release identity, licensing and attribution; a HTTP 200 body alone is insufficient.
- [ ] Preserve immutable original snapshots past Actions retention.
- [ ] Freeze complete origin/dataset/release/source-size/hash/rights manifest with truthful source-versus-typed semantics.
- [ ] Create independently validated deterministic typed extraction with explicit missingness and record mapping. Typed-only payload compression is NOT byte-exact original CSV/JSON/ZIP compression.
- [ ] Provide restricted heldout storage and a one-time preregistered unseal gate.
- [ ] Freeze decoder-work and encoder-work oracle thresholds and same-runner typed comparator suite before efficacy measurements.
- [ ] Run paired timed comparisons, RSS and decoder size accounting, fuzz/sanitizers on GitHub Actions only.
- [ ] Keep general-purpose Class-A frontier classification at zero until independently certified.

## Source documents

- NOAA: https://www.ncei.noaa.gov/pub/data/ghcn/daily/
- USGS migration: https://waterdata.usgs.gov/blog/api-waterservices-decom
- NASA POWER: https://power.larc.nasa.gov/docs/services/api/temporal/daily/
- UCI 235: https://archive.ics.uci.edu/dataset/235/individual+household+electric+power+consumption
