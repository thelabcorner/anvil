# Independent all-axis Pareto claim audit — supplementary, NOT the frozen I9 gate

**Status:** source implementation and 24 adversarial **scalar** tests passed (read-only Python in a temporary directory; no codec builds, benchmarks, CI runs or changes to a running experiment). **Claim scope:** one source file / one hosted job / a declared set of mainstream reference modes. This is NOT a universal-statistical dominance proof.

## Why the extra audit exists

The frozen `docs/gate-verify-i9.py` assigns FRONT-CROSSING / FRONT-GAP / DOMINATED / DEGENERATE using TWO separate (compressed ratio, encode MB/s) and (compressed ratio, decode MB/s) planes. Those labels are useful for their historical objective but do **not** establish that one ANVIL mode improves compressed bytes, encode time, decode time, peak RSS, decoder binary size, source/decoder dependencies, streaming, random access or worst-case safety at once.

Real A48 partial artifact [run 44380412243 / artifact 14480494484](https://github.com/thelabcorner/anvil/actions/runs/44380412243/artifacts/14480494484) illustrates why: on `dickens`, ANVIL achieved 2,636,635 compressed bytes (smaller than Brotli q11 at 2,827,779 B) and a frozen thin-grid 2D FRONT-CROSSING, but decode throughput was **20.405 MB/s against Brotli's 226.142 MB/s**. That is ~11.08x slower decode — a genuine tradeoff, not all-axis Pareto dominance. That job was cancelled before finishing 12 files and never produced a legitimate full R-2 H1/H0.

## Input contract

`tools/pareto_allaxis_audit.py` takes ONE JSON manifest scoped to a single byte-identical input. Source identities, paired raw timing artifacts, memory evidence and decoder distributions must be pinned; declarations alone are not independent verification. A machine producing these manifests must provide independently inspectable immutable backing artifacts.

```json
{
  "schema": 1,
  "scope": {
    "file": "dickens",
    "input_bytes": 10192446,
    "input_sha256": "<64-hex-sha256-of-source>",
    "source_commit": "<40-hex-ANVIL-source-commit>",
    "run_id": "<github-actions-numeric-run-id>",
    "runner_fingerprint": "ubuntu-24.04 / Clang18 / logical CPU topology / image revision",
    "benchmark_tool_sha256": "<64-hex-hash-of-paired-benchmark-driver>"
  },
  "candidate": {
    "codec": "anvil-vN",
    "complete_wire_bytes": 12345,
    "decoder_distribution_bytes": 45678,
    "decoder_footprint_includes_dependencies": true,
    "binary_sha256": "<64-hex-binary-hash>",
    "code_commit": "<40-hex-candidate-commit>",
    "rss_evidence_sha256": "<64-hex-raw-rss-artifact-hash>",
    "decoded_sha256": "<same-as-scope.input_sha256>",
    "peak_rss_samples_bytes": [1234567, 1234570, 1234565],
    "hard_gates": {
      "roundtrip": "pass", "malformed_input": "pass",
      "wire_compatibility": "pass", "streaming": "pass"
    },
    "capabilities": {"streaming": true, "random_access": false}
  },
  "required_references": ["brotli-q11", "xz-9e", "zstd-19"],
  "references": [
    {"codec": "brotli-q11", "...": "same structural fields as candidate"},
    {"codec": "xz-9e", "...": "..."},
    {"codec": "zstd-19", "...": "..."}
  ],
  "paired": [
    {
      "reference": "brotli-q11",
      "encode": {
        "run_id": "<same-as-scope.run_id>",
        "raw_pairs": 9, "raw_evidence_sha256": "<64-hex-raw-pair-artifact-hash>",
        "timing_valid": true, "ci95": [0.90, 0.98],
        "null_pre_ci95": [0.98, 1.02], "null_post_ci95": [0.99, 1.03]
      },
      "decode": {"...": "same fields, with candidate-time/reference-time CI"}
    },
    {"reference": "xz-9e", "encode": {}, "decode": {}},
    {"reference": "zstd-19", "encode": {}, "decode": {}}
  ]
}
```

**Independent reference-set control:** `required_references` within the candidate's manifest must match a separately frozen evaluator-controlled lock file, e.g. `{"schema":1,"required_references":["brotli-q11-lw30","xz-9e","zstd-19"]}`. The command requires `--reference-lock` and a separately recorded SHA-256 of that file. The lock must be committed/frozen **before** experimental evaluation and verified against that source-controlled commitment; a candidate cannot establish dominance by omitting strong baselines from its own manifest. This program checks supplied lock bytes and set equality; it does **not** independently authenticate a GitHub commit or prove that the lock represents every mainstream codec. Do not fabricate that source freeze or assemble an easy comparator set after observing the candidate.

**The example above is a schema illustration with placeholders, deliberately NOT valid evidence.** `decoder_distribution_bytes` must include the complete decoder and all necessary runtime/model/dictionary assets, not just the bare CLI executable. `complete_wire_bytes` must include headers, indices, framing, and all source-specific side information. `peak_rss_samples_bytes` are process peaks from at least three separate invocations on the same runner; a single "/usr/bin/time" observation is **insufficient**.

Raw paired timing CIs are expressed as **candidate elapsed time divided by reference elapsed time**. An upper bound <=1 is required to establish observed timing nonregression, and upper bound <1 gives significant timing improvement at the supplied 95% level. Both before/after A/A nulls must include 1.0 and the time-arm run IDs must match the scope. Source, pairing, firmware/runner and code provenance must be verified separately; self-declared SHA strings are only traceability, not authentication.

## Statuses and claim boundaries

- `OBSERVED_SCOPE_DOMINANCE`: all required metrics have evidence, candidate is no worse in every axis and strictly better in at least one on the *declared file, references and runner*. This is a sampled engineering claim, not formal dominance of all data distributions.
- `OBSERVED_NONINFERIOR_NO_STRICT_GAIN`: no strict improvement measured.
- `OBSERVED_REGRESSION_OR_HARD_FAIL`: a known failed correctness/safety/capability gate or a supported regression on bytes, decoding time, encoding time, RSS or distribution footprint. Do NOT downgrade this to 'inconclusive'.
- `INCOMPLETE_OR_STATISTICALLY_INCONCLUSIVE`: missing raw provenance, insufficient RSS samples, overlapping memory samples, unverified hard gate, invalid/noisy timing, or CI that crosses 1. It is not H0.
- `ALL_REQUIRED_REFERENCES_OBSERVED_SCOPE_DOMINANCE`: every explicitly required reference mode has observed scope dominance; still NOT all files, all levels, all codecs, or a frozen Class-A crossing.
- Any empty, duplicate, missing or extra reference set invalidates global required-reference coverage. A one-file result must not be aggregated naively into a broad claim.

## Executable command

```bash
python3 tools/pareto_allaxis_audit.py results/one-file-samejob-evidence.json \
  --reference-lock path/to/PREREGISTERED-reference-set.json \
  --reference-lock-sha256 HASH_FROM_AN_INDEPENDENT_PRIOR_SOURCE_FREEZE \
  --out results/all-axis-scope-audit.json
python3 -m unittest discover -s tests -p 'test_pareto_allaxis_audit.py' -v
```

CLI exit codes are intentionally meaningful: **0 only when all declared references have observed scope dominance; 1 for regression, inconclusive or not fully dominated; 2 for malformed/inconsistent reference coverage or schema.** A CI consumer must never interpret valid JSON output or an inconclusive result as experiment success.

These scripts do **not** dispatch GitHub Actions, modify benchmarks, synthesize data or replace existing gates. To achieve ANVIL's actual objective, build a separate corpus-wide promotion protocol with exact immutable file coverage, independently verified source families, proper simultaneous same-job comparisons, multiple runner replications, decoder-only distribution footprint, full memory/fuzz/safety tests, and distinguish baseline codec levels. The old Class-A/2D 435/28/5/0 result is preserved, not reinterpreted.

## Independent adversarial scalar checks

24 deterministic cases pass: positive scoped observation, bytes-regressed/time-faster counterexample, time-regressed/bytes-better counterexample, CI crossing, null drift, one-shot RSS, overlapping RSS, unambiguously worse RSS, omitted decoder assets, lost random access, failed roundtrip as HARD FAIL, missing reference class, mismatched run ID, absent raw evidence hash, corrupt decoded output, incomplete all-reference win, missing runner fingerprint, missing RSS evidence SHA, spoofed candidate/reference codec identity, three fail-closed CLI tests and two externally frozen reference-set counterexamples (valid scope exit 0, regression/inconclusive exit 1, invalid reference coverage exit 2).

**Decision:** PROCEED_WITH_GUARDS on this standalone read-only validator; no execution of a full benchmark or general-purpose Pareto promotion is claimed.
