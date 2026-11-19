# A75 — Corpus lineage and independence gate

**Date:** November 18, 2026. This is a research-integrity correction, not a codec mechanism or performance result.

## Exact source evidence

At candidate commit `2c5eeb9e570b1e1162cbff98d29e0a978edeca92`, `tests/make_smoke_corpus.py` deterministically emits all four fixtures:

- `generated.json`: 7,000 records with `id=i`, kind cycle 4, active period 7, golden-ratio arithmetic score, path `i%317`, tag cycles 19 and 43.
- `generated.jsonl`: 12,000 records with the same kind/score mechanisms, deterministic timestamp/checksum, session seed 424242, and a drift field every 200 rows.
- `generated.sqlite`: 12,000 rows whose payload literally reuses the `rows` array generated for `generated.json`.
- `generated.repeat.jsonl`: pure repetition control from the same script.

Independent, deterministic regeneration exactly matched the frozen Git blob SHA-1 identifiers:

| Corpus | Bytes | Git blob SHA-1 |
|---|---:|---|
| generated.json | 827,664 | `6d9fe60874ae6382ccc211ac7367af694591e2d1` |
| generated.jsonl | 2,803,267 | `1e0337271ac57ed6fc07bb93532275ace0a9f3bd` |

The generator source blob is `d3009a6ad1eff1883bc7785419673daab38ea256`. These are Git blob object IDs, NOT SHA-256. The SQLite and repetition fixture blob identifiers are pinned in the manifest.

## Claim boundary

These inputs remain valid stress and regression data. They are NOT independent heldout evidence for a general-purpose compressor, even if their serialization differs. The historical frozen Brotli q11 generated.json compressed size (79,172 B) and ANVIL best size (89,589 B) remain measurements on a synthetic fixture. Encoding the visible generator formulas might win dramatically while proving nothing about other independently authored sources.

Moreover, equality after `json.loads` is not bit-exact losslessness. Duplicate JSON keys, key order, numeric lexemes, escaped characters and whitespace must remain reconstructible from the transmitted wire format.

## Guard and verification

- `tests/corpus/corpus-provenance-v1.json` records each source file Git blob and generator Git blob.
- `tools/corpus_lineage_gate.py` verifies the actual local file bytes and generator identity; it detects both identical human family labels and identical source path/blob IDs, even if different family labels are supplied.
- `tests/test_corpus_lineage_gate.py` exercises nine scalar cases, including a forged family label, stale corpus/generator hashes, path escapes, and duplicated input.
- Different generator IDs receive `DISJOINT_DECLARED_LINEAGES_INDEPENDENCE_NOT_PROVEN`, not an assertion of independence.

Run from the checked-out repository:

```bash
python3 -m unittest discover -s tests -p 'test_corpus_lineage_gate.py' -v
python3 tools/corpus_lineage_gate.py --manifest tests/corpus/corpus-provenance-v1.json --repo-root . --discovery tests/corpus/generated.json --validation tests/corpus/generated.jsonl tests/corpus/generated.sqlite --require-disjoint-lineage
```

The strict command is EXPECTED to exit 1 and report `SAME_FAMILY_TRANSFER_NOT_INDEPENDENT_HOLDOUT`. Without the strict flag, the same-family result is informational and exits 0, permitting ordinary exploratory stress tests.

## Research protocol

Use generated fixtures for discovery only. Before tuning, freeze independently sourced unrelated structured datasets with upstream provenance and hashes; then demand byte-exact raw reconstruction, complete wire + reconstruction instructions + dictionary + program bytes, matched encode/decode/RSS/decoder-size controls, and unmodified heldout evaluation. The absence of shared generator labels is a necessary screening condition, never sufficient proof of statistical independence.

**Disposition:** VERIFY provenance / REFINE general mechanism; no new CI, codec build, sweep or workflow dispatch is part of this change.
