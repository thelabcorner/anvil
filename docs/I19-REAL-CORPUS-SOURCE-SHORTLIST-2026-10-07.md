# I19 Phase-0: Independent Real Data Source Shortlist (UNFROZEN)

Date: 2026-10-07. These are **candidates for origin-level acquisition**, not verified corpus members or released holdout files. No SHA-256, byte size, licensing clearance or typed-lossless conversion has been attested. No I19 efficacy job may run on this list alone.

## Candidate 1 — NOAA/NCEI GHCN Daily station histories

Source: https://www.ncei.noaa.gov/pub/data/ghcn/daily/

NOAA's GHCN Daily directory publishes station-indexed data, documented readme and metadata. Select one or more **fixed station files**, not the multi-gigabyte global tarball, after checking station ID, coverage, element types, archive size and rights. Fixed-format monthly records with 31 daily fields are structurally different from continuous numerical telemetry; do not mistake station whitespace/header compression for mathematical innovation. Need frozen station path, original-byte SHA-256, retrieval date, precise parsing, missingness and source-to-output reversibility. Avoid mutable near-current data unless exact original bytes are stored with the manifest.

Potential role: discovery OR validation OR holdout as assigned before fitting; publisher origin `noaa-ncei`.

## Candidate 2 — USGS Water Data OGC API

Source: https://api.waterdata.usgs.gov/docs/ogcapi/

The official modern OGC API exposes daily and continuous USGS sensor data, with queryable collections and JSON FeatureCollections. Freeze a bounded explicit query by monitoring-location ID, parameter/statistic IDs, *closed historic* datetime interval, ordering/pagination and pagination termination. Snapshot **raw returned pages in order**; SHA each page and the canonical container/snapshot manifest. Simply hashing a live URL is not immutable source attestation: data can be revised and pages reorder. Consider high-frequency readings to meaningfully test local difference predictors. Respect public API rate limits and any attribution/terms.

Potential role: another partition, publisher origin `usgs-waterdata`.

## Candidate 3 — UCI Air Quality experiment (dataset ID 360)

Source: https://archive.ics.uci.edu/dataset/360/air

The UCI page offers a multi-channel hourly chemical sensor time series (9,358 rows, 15 features, missing-value sentinel -200). The page currently displays CC BY 4.0 licensing metadata **but its prose states the data can be used exclusively for research and commercial use is excluded**. This is a genuine licensing conflict: preserve both statements and do **not** assume commercial redistribution permission. ANVIL experiments are research, but source rights must be explicitly settled before creating a publicly reusable benchmark archive. Do not use nearby UCI dataset 387 as an independent origin; it describes the same air-quality collection.

Potential role: another origin `uci-airquality-de-vito` subject to explicit rights resolution and binary download/hash validation.

## Methodology and blockers

- These are independent publisher/dataset origins *subject to original provenance and derivative tracing*. Distinct hosting websites are not proof of independent scientific origin; inspect original collection authority, study, instrument and date range.
- Freeze origin assignment **before** fitting mathematical codecs or profile selector thresholds. Preserve discovery/validation/heldout separately, never train on sealed holdout.
- Record the original compressed/raw acquisition bytes and typed stream separately. For CSV/JSON inputs, numerical payload compression `raw-to-typed` and source-file byte-exact archive compression `raw-to-raw` are separate competition classes. A source reconstruction map is required for claiming true lossless original-byte compression.
- Follow the manifest schema in `tools/i19_validate_corpus_manifest.py` after the candidate acquisition job has produced actual hashes, dates, sizes and rights metadata. The validator only checks manifest structure; GitHub Actions must separately attest downloaded content bytes, frozen transforms and reproducibility.
- No prospective win has been measured. The numerical encoder is still optimized against consumed synthetic examples. No general-purpose Pareto crossing or novel mechanism is authorized.

Official documentation (checked during source identification):
- NOAA: https://www.ncei.noaa.gov/pub/data/ghcn/daily/
- USGS: https://api.waterdata.usgs.gov/docs/ogcapi/
- UCI: https://archive.ics.uci.edu/dataset/360/air
