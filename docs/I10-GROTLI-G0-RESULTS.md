# ANVIL I10 — GROTLI-ANVIL-0 Results

**Closed:** 2026-09-23
**Ruling:** **NO-GO-REPRESENTATION for frozen G0 vXOR**
**Broader SRS ruling:** **not falsified; not automatically authorized by G0**
**Remote run:** 35935415091
**Frozen implementation:** public `7b161f0ec134cbc1e5ad37e71621f0ead2dd940d`
**Production source changed:** no

---

## 0. What was tested

G0 tested one deliberately narrow historical Grotli mechanism:

> exact line-record-aligned vertical XOR, padded to the maximum record length,
> serialized row-major, then compressed with the exact same Brotli q11/lgwin30
> backend as the raw control.

Every reconstruction byte was charged:

- carrier magic/version;
- decoded length;
- record count;
- stride;
- every record length;
- matrix length;
- complete padded XOR matrix;
- common prototype envelope.

Raw Brotli remained the selected fallback.

This was a causal representation test, not a broad SRS test.

---

## 1. Exact results

| Family | Source | Raw Brotli envelope | vXOR+Brotli envelope | Delta | Padding expansion | vXOR zeros | Selected |
|---|---:|---:|---:|---:|---:|---:|---|
| D1 Amazon cellphone NDJSON | 277,673 B | 40,126 B | 141,171 B | **+251.82%** | 1.394x | 31.05% | raw |
| D2 CDISC ADaM NDJSON | 615,350 B | 25,029 B | 122,857 B | **+390.86%** | 14.011x | 95.37% | raw |
| V1 GH Archive 10 MiB | 10,485,760 B | 1,292,757 B | 4,342,951 B | **+235.94%** | 35.144x | 96.81% | raw |

All vXOR candidates round-tripped exactly.

The malformed/trailing/truncated carrier self-tests passed.

The raw fallback therefore preserved the portfolio byte result on every file,
but the frozen vXOR representation contributed no selected win.

---

## 2. Pre-registered ruling

G0 required:

1. exact roundtrip;
2. >=5% vXOR+Brotli improvement on two independent discovery families;
3. aggregate selected improvement;
4. >=1% held-out improvement;
5. raw fallback;
6. acceptable decode cost;
7. malformed-wire/allocation safety.

Conditions 2, 3 and 4 fail decisively.

Therefore the correct ruling is:

> **NO-GO-REPRESENTATION for the frozen padded row-major record-vXOR carrier.**

Do not tune this G0 carrier and call the rerun G0.

Do not assign it an ANVIL transform ID.

---

## 3. Why it failed

The result is more informative than a generic "vXOR lost."

### 3.1 Outlier stride can make the logical carrier enormous

D2:

- source: 615,350 B;
- max record: 7,233 B;
- padded matrix: 8,621,736 B;
- padding alone: 8,006,386 B.

V1:

- source: 10,485,760 B;
- max record: 32,818 B;
- padded matrix: 368,513,322 B;
- padding alone: 358,027,562 B.

The transform created highly compressible zeros, but asking Brotli to encode
hundreds of megabytes of synthetic padding is still economically terrible.

### 3.2 Padding is not the whole failure

D1 is the important control.

Its padding expansion is only 1.394x, yet vXOR+Brotli is still 3.52x the raw
Brotli envelope.

Therefore the failure cannot be explained away as only an outlier-stride bug.

Raw Brotli already exploits repeated JSON keys, punctuation, lexical fragments,
and cross-record substrings extremely well.

Row-major XOR destroys many of those literal/dictionary matches and replaces
them with residual bytes whose zero density is only 31%.

This is a **representation mismatch**.

### 3.3 Very high zero density is not sufficient

D2 and V1 have 95-97% zero vXOR matrices.

Yet both lose badly after complete accounting.

This is a useful warning for future anatomy/oracle work:

> **zero density, H0 reduction, sparse-change masks, or any other local statistic
> is not a substitute for final downstream compressed bytes.**

The final backend's match/context structure matters.

### 3.4 H0 itself predicted the D1 problem

D1:

- raw idealized H0 bytes: ~200,920;
- vXOR-carrier idealized H0 bytes: ~275,135.

Even before Brotli, the frozen carrier moved the source in the wrong
zero-order direction.

For D2/V1, H0 per matrix byte becomes low because padding dominates, but the
carrier becomes so much larger that total idealized H0 bytes also increase.

---

## 4. What this says about Grotli

It does **not** say that the Grotli architectural lesson was wrong.

It says the historical record-position vXOR primitive does not survive an exact
wire on these independent real datasets.

The stronger Grotli lesson remains:

> **reorganize data into a representation where a mature backend sees simpler
> streams, then arbitrate against raw Brotli using exact final bytes.**

The G0 failure sharpens what "better representation" must mean.

It cannot merely align bytes by record position.

It likely needs to preserve or expose **semantic/shape equivalence**:

- keys with keys;
- one field's values with the same field's values;
- numeric values in numeric coordinate systems;
- low-cardinality strings as IDs;
- nullability as bitmaps;
- repeated shapes as shape IDs;
- high-cardinality/raw regions left in the order where Brotli already performs
  well.

That is the difference between blind record-position prediction and the richer
Grotli/SRS architecture.

---

## 5. Why SRS remains a viable research direction

There are now three independent evidence classes.

### Historical Grotli evidence

The old Grotli work measured large wins on some structured workloads from
columnization + typed representation before Brotli/Zstd.

Those results motivated this lane but are not enough by themselves for a new
production ANVIL route.

### Modern systems evidence

FastLanes, ALP, Pcodec, FSST, BtrBlocks, CLP, LogPrism and related work all
support representation selection / decomposition / typed transforms as a
serious compression architecture.

### Direct close prior art

DataCortex independently implements a JSON/NDJSON-specific form of:

- structure/schema inference;
- selective columnization;
- typed leaves;
- raw/preprocessed backend candidates;
- exact smallest-output arbitration.

Its author-reported GH Archive result is only context until independently
reproduced, but it demonstrates that the *richer* architecture is materially
different from G0's blind padded vXOR.

---

## 6. The next experiment must not be "fix G0"

The preregistration explicitly forbids outcome-driven G0 retuning.

Therefore do not immediately try:

- group sizes 64/256/1024;
- compact padding omission;
- length delta coding;
- transposition;
- field parsing;

under the G0 label.

Those would answer different questions.

Instead create a new experiment with a new preregistration.

The strongest next question is:

> **If the encoder is given or can infer a byte-exact record shape and separates
> repeated fields into homogeneous streams, is there a complete structured
> carrier that Brotli q11 compresses smaller than raw Brotli q11?**

That directly tests the real Grotli/SRS thesis rather than repairing a failed
primitive.

---

## 7. Recommended next gate: G1-CEILING

Before building automatic structure discovery, establish the representation
ceiling.

Use a schema/parse-aware **oracle lane** on D1/D2:

- parse records;
- preserve exact lexical reconstruction;
- separate structural skeleton from field-value streams;
- preserve record shape IDs;
- do not normalize numbers/escapes/whitespace;
- begin with RAW field-value streams only;
- compress the complete carrier with Brotli q11/lgwin30;
- compare against raw Brotli q11/lgwin30.

Then add typed leaves one at a time only after the pure structure-separation
ceiling is measured:

1. dictionary/enum;
2. integer FOR/delta/DoD;
3. null/default bitmaps;
4. string symbolization;
5. float/Gorilla/ALP-like exact representations.

This decomposition is essential.

If **structure separation alone** wins, SRS has a strong base.

If it loses but one typed leaf produces the win, the opportunity is
type-specific rather than general schema decomposition.

If even an oracle schema + typed portfolio cannot beat raw Brotli, stop the
broad JSON SRS lane.

---

## 8. Router lesson

The raw fallback behaved exactly as intended.

That is strategically important.

A future ANVIL representation compiler does not need every expert to win.

Its aggregate property can be:

    selected(X) = min(
        raw_Brotli(X),
        structured_Brotli(X),
        BWT(X),
        replay(X),
        ...
    )

A failed expert costs encoder search time but need not cost archive bytes.

The real engineering challenge is therefore:

> make expert discovery cheap enough that the encoder can search useful
> representations without repeating Grotli v4's planner explosion.

That remains a viable and high-value problem.

---

## 9. Final G0 interpretation

G0 produced a strong negative result:

> **blind padded row-major record-vXOR is not the bridge from Grotli to ANVIL.**

But it also removed a dangerous shortcut.

The promising bridge is now narrower and clearer:

> **typed/shape-aware representation selection, scored by actual downstream
> bytes, with raw Brotli as a permanent fallback.**

That is the architecture worth testing next.
