# prototypes/i9-deflate — P4.1 DEFLATE reconstruction probe

Lane `deflate` (swarm `anvil-i9-pareto`, I9). Prototype only — **not an ANVIL
wire format**; no `src/` integration.

**Result: GO** — mozilla charged recovery 1,362,177 B (bar: >~430 KB);
see `RESULTS.md` for the full write-up and `PRIOR-ART-NOTE.md` for the novelty
disposition.

## Tools

| file | purpose |
|---|---|
| `census.py` | enumerate embedded DEFLATE: ZIP (incl. zip64/SFX base), gzip members, PNG IDAT, whitelist-header raw zlib, CAB MSZIP; verifies every stream by decompression + CRC-32/Adler-32 |
| `replay.py` | precomp-style exact replay: zlib parameter search (wbits × level × memLevel × strategy) to bit-exactness; valid/diff/brute classification; diff proxy |
| `ceiling.py` | region-local ceiling: concatenated deflate payloads vs plaintexts under brotli q11 / xz -9e / zstd 22 |
| `transform.py` | in-situ transformed representation + side table; **proves the inverse is sha256-identical to the source** |
| `analyze.py` | consolidates census/replay/ceiling/transform into `summary-<file>.json`; diff-mode projection; per-container attribution |

## Artifacts (`results/`)

Key files and sha256 (first 16 hex):

| artifact | bytes | sha256[:16] |
|---|---:|---|
| `census-mozilla.json` | 774,560 | 343D7A89C7E7CC91 |
| `census-mozilla.csv` | 225,369 | 05A20F9B6625B07D |
| `census-samba.json` | 44,475 | 8516E22BFBCE6564 |
| `census-sao.json` | 311 | 913D3130165F43CB |
| `census-ooffice.json` | 315 | 71A2FD8DCF9BE0ED |
| `replay-mozilla.csv` | 191,008 | 825B91E2A58967D6 |
| `replay-mozilla-zip156160.csv` (bounded region) | 38,514 | 604F8AFEAD4D9AEB |
| `census-mozilla-ceiling.json` | 813 | FF3C88E2FC4B5685 |
| `census-samba-ceiling.json` | 794 | 1DEA54D6FAFA560F |
| `transformed-mozilla.bin` | 57,034,135 | B3E16040501E5DEA |
| `transform-mozilla.json` | 633 | 638DB0C2D231000E |
| `mozilla-anvil-brotli.anv` | 13,806,173 | 175C68610E213BB7 |
| `transformed-mozilla-anvil-brotli.anv` | 12,443,996 | B3954DFC4E57C5B8 |
| `mozilla-anvil-brotli-0D1E130B.anv` (canonical binary) | 13,806,173 | 175C68610E213BB7 |
| `transformed-mozilla-anvil-brotli-0D1E130B.anv` (canonical binary) | 12,443,996 | B3954DFC4E57C5B8 |

Canonical binary: `build\anvil.exe` sha256 `0D1E130BA2FBC43ACCE46E423BDD2AE732A2CA56A419B45929E26A6487C1DDAB`
(src/anvil.cpp `62BC6631…`, HEAD fc23d9a). Canonical `.anv` files are byte-identical
to the 8EAE1FB3 decode-verified run, so the roundtrip verification transfers.

**Sha-move re-verification (src `38409E26`, `build\anvil.exe` `E8AA2E48`, ALLOC-only):**
both archives decode rc=0 sha256-exact on the new canonical decoder
(`dec-orig-E8AA2E48.bin` / `dec-transformed-E8AA2E48.bin`); byte totals unchanged per
encoder-identity gate 494/494 (arch/coordinator). Full re-encode deferred to a quiet
byte slot — see RESULTS.md §5.5.
| `dec-orig.bin` (green-binary decode == source) | 51,220,480 | 657FC3764B0C75AC |
| `dec-transformed.bin` (green-binary decode == T) | 57,034,135 | B3E16040501E5DEA |
| `summary-mozilla.json` | 4,784 | 2B26D9B9D9236B4F |

Source sha256 (mozilla): `657fc3764b0c75ac9de9623125705831ebbfbe08fed248df73bc2dc66e2a963b`.
ANVIL binary used: `build\anvil.exe` sha256 `DA24665C52BC250C1B4C2FBA91D7407A1E2FD7259FC125C107547C0F3FBBE80F`
(dirty-tree build; **decode is broken by the team's `decode_one_block` blocker**,
so all ANVIL figures are encode-only — see RESULTS §5).

**Post-decode-fix re-verification:** both encodes were reproduced byte-identically
and both `.anv` files decoded byte-exact (sha256) on the fixed-source binary
`build-i9-format\anvil.exe` sha256 `8EAE1FB327AD759B1CF80BE765B59FA7EFB8AB843F908341AC2283652BFDD105`
(src/anvil.cpp `62BC6631…`, HEAD fc23d9a). See RESULTS §5.5. The coordinator is
having arch rebuild the shared `./build` from this source; final citation should
name that canonical binary sha.

## Reproduce

See `RESULTS.md` §7 for the exact command sequence. Minimal path:

```powershell
. .\env.ps1
python prototypes\i9-deflate\census.py scratch\ratio-first\corpora\silesia\mozilla --out-dir prototypes\i9-deflate\results
python prototypes\i9-deflate\replay.py prototypes\i9-deflate\results\census-mozilla.json --src scratch\ratio-first\corpora\silesia\mozilla --out-dir prototypes\i9-deflate\results
python prototypes\i9-deflate\transform.py prototypes\i9-deflate\results\census-mozilla.json prototypes\i9-deflate\results\replay-mozilla.csv --src scratch\ratio-first\corpora\silesia\mozilla --out-dir prototypes\i9-deflate\results
python prototypes\i9-deflate\ceiling.py prototypes\i9-deflate\results\census-mozilla.json --src scratch\ratio-first\corpora\silesia\mozilla --out-dir prototypes\i9-deflate\results
python prototypes\i9-deflate\analyze.py --results prototypes\i9-deflate\results --src scratch\ratio-first\corpora\silesia\mozilla --name mozilla
```

Requires Python 3.12 + `brotli`, `zstandard` (present on this host).
