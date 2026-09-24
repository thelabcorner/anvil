# ANVIL I10 - GROTLI G4 Planner Fidelity Results

**Closed:** 2026-09-24  
**Preregistration:** `I10-GROTLI-G4-PLANNER-FIDELITY-PREREG.md`, frozen revision r5  
**Frozen implementation:** public SHA `6c556dc522112aff4b3566990965e000418c5a57`  
**Remote run:** `35952830106`  
**Frozen G4 source SHA-256:** `08c2a9861e25511b15e00ae12e2a4cee8288e0c5974bca855e493b38b5683c85`  
**Frozen binary SHA-256:** `b2851bfcaa8cf41f74073f6d91b947da036fbcd66a61b05f8e9eacc892f1e577`  
**Frozen G3 reference:** public SHA `1a3d18fed76adb6fb33264e1994f9c357306b3fa`  
**Decision:** **NO-GO-G4**

---

## 0. Executive ruling

G4 asked whether a cheap deterministic scoring surface could reproduce the frozen
G3 q11 leaf oracle closely enough to remove almost all ranking-time q11 calls.

The experiment itself is fully valid:

- pinned G4 implementation identity: PASS
- warning-clean build + adversarial selftests: PASS
- separately built frozen G3 source/binary identity: PASS
- frozen G2 diagnostic build: PASS
- D1-D4 corpus identity: PASS
- G4 O11 versus independently built frozen-G3 carrier identity: **16/16 exact**
- Q2 pairwise separability probe: **SEPARABLE-ENOUGH**
- frozen r5 ruling executed mechanically: PASS

The hypothesis failed.

> **NO-GO-G4 - no cheap proxy replaces O11 at carrier level.**

No new validation corpus was opened. G3 V1 was not consumed as G4 evidence.

---

## 1. Hard semantic identity

For all four discovery files and all four frozen G3 regional portfolio masks
(RAW / DICT / INT / MIXED), the G4 O11 carrier and the separately built frozen G3
reference had:

- identical complete bytes;
- identical carrier length;
- identical carrier SHA-256.

Result: **16 / 16 exact carrier hash equalities**.

This is the critical provenance result: G4 measured planner substitution against
the actual frozen G3 representation, not a reimplementation that merely looked
similar.

---

## 2. Frozen proxy gates

Required for an adoptable proxy:

- aggregate regret <= **0.25%**
- per-file regret <= **0.50%**
- weighted leaf agreement >= **95%**
- no ranking-time q11 calls
- mandatory planner speedup >= **5x**
- target speedup >= **10x**

Measured summary:

| Proxy | Aggregate regret | Worst-file regret | Measured speedup vs O11 | Adoptable |
|---|---:|---:|---:|---|
| S0 | -2.0353% | 7.0802% | 1.0218x | **no** |
| S1 | -1.1453% | 7.0802% | 0.4614x | **no** |
| S2 | -0.2120% | 1.5388% | 0.0591x | **no** |

Negative aggregate regret is not a rescue: the frozen test is a fidelity test, not
permission to exploit a proxy's accidental cross-file aggregate. All three proxies
miss the per-file fidelity gate; S0/S1 also miss agreement, and all three fail the
mandatory speedup gate.

S2 is closest on byte fidelity, yet still misses the worst-file threshold by roughly
3x and is approximately **17x slower than O11** under the frozen speedup definition.

Therefore:

- `DSTAR = None`
- `ISTAR = None`
- Q1 = **FAIL**

The S3 finalist stage consequently collapses to raw Brotli only; it performs no
additional q11 ranking calls.

---

## 3. Q2: separability is not the blocker

The bounded pairwise probe was defined only on frozen O11 `REGION_MIXED`.

Measured:

- pairs: **24**
- max pair gain: **0.277632%** (threshold 0.5%)
- aggregate pair gain: **0.057144%** (threshold 0.25%)
- verdict: **SEPARABLE-ENOUGH**

This matters because it rejects a tempting post-hoc story: G4 did **not** fail because
pairwise leaf interactions obviously require a joint combinatorial planner.

The measured blocker is simpler and harsher: the tested cheap local score surfaces
do not reproduce downstream q11 carrier choices with sufficient fidelity *and*
cost advantage.

---

## 4. Correct response to the failure

The frozen preregistration already specifies the disposition:

> **NO-GO-G4 -> return to representation/anatomy work; do not expand the typed
> basis to rescue the proxy hypothesis.**

Accordingly:

- do not tune S0/S1/S2 after seeing D1-D4;
- do not add a learned planner inside G4;
- do not reinterpret Q2 as a planner win;
- do not spend a new held-out corpus validating a failed discovery hypothesis;
- do not integrate G4 into production ANVIL.

G4 is closed as a useful negative result.

---

## 5. Next lane

The architecture resumes at **G5A - ordering attribution**.

G5A must hold the leaf vocabulary fixed and answer a more fundamental question:

> How much of the measured Grotli/ANVIL representation effect comes from changing
> the order and locality of the exact same scalar byte chunks before Brotli?

The clean experiment is not another planner. It is a common reversible envelope
whose structured token chunks are byte-identical and count-identical across arms,
with only their permutation changed:

1. source-frame order;
2. shape-grouped row order;
3. shape-grouped column order.

That makes the causal variable literally the ordering of the same charged bytes.
