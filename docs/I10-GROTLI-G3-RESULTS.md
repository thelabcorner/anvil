# ANVIL I10 - GROTLI G3 Regionized Residual Representation Results

**Closed:** 2026-09-24  
**Frozen implementation:** public SHA `1a3d18fed76adb6fb33264e1994f9c357306b3fa`  
**Remote run:** `35947013432`  
**Frozen binary SHA-256:** `ac980f35feeefde0007c8f3aa1b5fd0c2262866ff66526b8c7d100c5117ab537`  
**Production source changed:** no  
**Discovery ruling:** **PASS-G3-REGION**  
**Held-out ruling:** **PASS-G3-NARROW**

---

## 0. Executive ruling

G3 asked whether the G2 structured representation could become local and recoverable:
valid line-framed records are represented structurally while malformed or incomplete
frames remain exact raw residual data.

The discovery result passed every frozen gate.

- D3 contained **11,228 structured frames** and **1 raw residual frame**.
- D3's best regionized arm was **1,240,155 B**, versus **1,292,757 B** raw Brotli:
  **-4.0690%**.
- The four-family routed portfolio was **1,384,654 B** versus **1,466,769 B**
  raw Brotli: **-5.5984%**.
- D2 and D4 remained >=5% wins.

This closes the G2 availability bottleneck on the discovery corpus: a single malformed
record no longer forces a 10 MiB object to raw fallback.

The independent held-out result did **not** generalize. On frozen Sino-US DrugQA V1,
the best G3 regionized representation was **2,179,615 B**, while raw Brotli was
**2,112,235 B**: **+3.1900% worse**. V1 contained no raw residual frames, so this is
not a framing failure. The final frozen classification is therefore:

> **PASS-G3-NARROW** - regionization is a real, causal discovery mechanism, but the
> current representation is class-specific rather than broadly validated.

No production transform ID is authorized.

---

## 1. Discovery result

| Family | Raw Brotli | G2 whole | G3 RAW | G3 DICT | G3 INT | G3 MIXED | Selected | Delta vs raw |
|---|---:|---:|---:|---:|---:|---:|---|---:|
| D1 Amazon | 40,126 | 39,299 | 39,460 | **39,277** | 39,460 | **39,277** | G3 DICT | **-2.1158%** |
| D2 CDISC | 25,029 | 20,903 | 25,964 | **20,861** | 26,044 | 20,891 | G3 DICT | **-16.6527%** |
| D3 GH Archive 10 MiB | 1,292,757 | 1,292,757 | 1,240,884 | **1,240,155** | 1,247,531 | 1,245,816 | G3 DICT | **-4.0690%** |
| D4 CROVIA | 108,857 | **84,361** | 87,089 | 84,431 | 86,794 | **84,361** | G2 WHOLE | **-22.5029%** |

Aggregate routed portfolio:

- raw Brotli: **1,466,769 B**
- selected: **1,384,654 B**
- saved: **82,115 B**
- delta: **-5.598359%**

The frozen discovery requirement was <= -3%, so G3 passed with material headroom.

---

## 2. D3 is the causal G3 result

G2 failed broadly because D3 represented roughly 88% of the aggregate raw-Brotli
bytes and its whole-object parser rejected the file after encountering an unterminated
JSON string.

G3 changes only the availability model:

- parsing is per frame;
- exact G2 shape identity is retained;
- the G2 leaf vocabulary is retained;
- invalid frames are carried raw;
- reconstruction order is explicit and charged;
- raw Brotli remains available.

With **11,228 structured frames + 1 residual frame**, D3 moves from a raw-only
1,292,757 B control to a best regionized result of 1,240,155 B.

That **52,602 B** reduction is sufficient to move the frozen aggregate from the G2
failure to a G3 discovery pass without adding a new leaf family.

---

## 3. Held-out V1 result

Frozen V1 identity:

- repository: DodgeLU/Sino-US-DrugQA
- commit: `cd026a27a5e039863de9080b8d7d95acaf1f5a39`
- path: `data/release/all.jsonl`
- git blob: `8d9e6acf3f534194487532fa60d4f1d43fe71d00`
- bytes: **15,374,047**
- SHA-256: `757b9bc7e5ee2d38ab5ed43877b1d87809cb5131621d106671bc27996da1c499`

Measured frozen result:

| Arm | Complete bytes | Delta vs raw |
|---|---:|---:|
| RAW_BROTLI | **2,112,235** | **0.0000%** |
| G3_REGION_RAW | 2,184,665 | +3.4291% |
| G3_REGION_DICT | **2,179,615** | **+3.1900%** |
| G3_REGION_INT | 2,184,665 | +3.4291% |
| G3_REGION_MIXED | **2,179,615** | **+3.1900%** |

Diagnostics:

- frames: **11,444**
- structured frames: **11,444**
- raw residual frames: **0**
- structured coverage: **100%**
- exact shapes: **3**
- integer-specialized leaves selected: **0**
- G3 DICT/MIXED exact-dictionary leaves: **19 / 48**

The frozen V1 gate required G3 regionized bytes <= -1% versus raw. It failed by a
wide margin, so raw Brotli correctly wins final arbitration.

This is important negative evidence: the G3 failure on V1 cannot be blamed on
malformed input or insufficient structured coverage. A fully structured,
low-shape-count population can still present coordinates that are worse for Brotli
after the current representation.

---

## 4. What G3 establishes - and what it does not

Established:

1. **Local recoverability solves a real availability problem.**
2. The same mature Brotli backend can improve materially when the representation
   exposes useful locality.
3. Raw fallback protects populations where representation hurts.
4. Discovery gains are large enough to justify continued representation research.

Not established:

1. broad generalization of the current G3 carrier;
2. that column ordering is itself responsible for the gain;
3. that exact flat shape grouping is the right structural abstraction;
4. that the current q11 leaf oracle is production-affordable.

Those are separate causal questions and must remain separate experiments.

---

## 5. Consequence

G3 is retained as **research evidence**, not promoted to the production format.

The next completed lane, G4, tests planner fidelity without changing representation.
After G4's ruling, the architecture returns to representation anatomy: G5A must
isolate ordering/locality while holding parsing, exact shapes, scalar bytes, raw
residuals, metadata accounting, and Brotli constant.
