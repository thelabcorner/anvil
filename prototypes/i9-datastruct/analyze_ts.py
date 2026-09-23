#!/usr/bin/env python3
"""
datastruct (I9) — STRUCTURE + RESIDUAL ENTROPY ANALYSIS of synth-timeseries.bin.

Purpose: pick the prototype's residual models BEFORE writing the counted wire.
No codec claim here; every number is an empirical order-0 entropy over a
residual transform, i.e. an UPPER BOUND on what the counted coder can reach.

STRUCTURE CORRECTION (2026-09, datastruct):
  The task brief and docs/swarm-i8-strategy.md 2.7c say "10,000 x 28-byte
  records (u64/f64/u64/u32)". That is a stride-28 sampling artifact. The file
  is 20,000 x 14-byte records: '<Q f H' = u64 ts_ms (+1000 +/- 50), f32 value
  (random walk +/- 0.1), u16 id (= i % 1000). Evidence: byte-exact
  reconstruction from tests/make_synth_corpus.py (seed 90001) and 0/20,000 id
  mismatches under the 14-B parse; the 28-B parse yields a garbage f64
  (1.16e232). The true record period is P=14.

Usage: python prototypes/i9-datastruct/analyze_ts.py [file]
"""
from __future__ import annotations
import math
import struct
import sys
from collections import Counter

import numpy as np


def H0(vals):
    """Empirical order-0 entropy in bits/symbol."""
    c = Counter(vals)
    n = sum(c.values())
    if n == 0:
        return 0.0, 0
    return -sum((v / n) * math.log2(v / n) for v in c.values()), len(c)


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else 'tests/corpus/synth-timeseries.bin'
    d = open(path, 'rb').read()
    print(f'file {path}  {len(d):,} B')
    P = 14
    assert len(d) % P == 0
    n = len(d) // P
    print(f'records = {n:,} x {P} B')

    # --- structure verification -------------------------------------------------
    ts, val, rid = [], [], []
    for i in range(n):
        t, v, r = struct.unpack_from('<QfH', d, P * i)
        ts.append(t); val.append(v); rid.append(r)
    ts = np.array(ts, dtype=np.uint64)
    val = np.array(val, dtype=np.float32)
    rid = np.array(rid, dtype=np.uint16)
    id_ok = bool((rid == (np.arange(n) % 1000)).all())
    dts = np.diff(ts.astype(np.int64))
    print(f'id == i%1000 for all records: {id_ok}')
    print(f'ts delta: min={dts.min()} max={dts.max()} distinct={len(set(dts.tolist()))}'
          f' H0={H0(dts.tolist())[0]:.3f} bits')
    print(f'f32 value: min={val.min():.6f} max={val.max():.6f} distinct={len(set(val.tolist()))}')

    # --- per-byte-column statistics --------------------------------------------
    print('\nper-column (of %d) raw and residual entropies [bits/byte, order-0]:' % P)
    print('col  raw-H  raw#  dH(prev-rec) d#   delta#  notes')
    tot = np.zeros(P)
    for j in range(P):
        col = np.frombuffer(d, dtype=np.uint8).reshape(n, P)[:, j]
        h_raw, k_raw = H0(col.tolist())
        dcol = ((col.astype(np.int32) - np.roll(col, 1)) & 0xFF)[1:]
        h_d, k_d = H0(dcol.tolist())
        note = 'const' if k_raw == 1 else ''
        print(f'{j:3d}  {h_raw:6.3f}  {k_raw:4d}  {h_d:8.3f}  {k_d:5d}   {note}')
        tot[j] = h_d
    print(f'sum dH(prev-rec) over columns = {tot.sum():.1f} bits/record '
          f'= {tot.sum()*n/8:,.0f} B over the file')

    # --- field-level models -----------------------------------------------------
    print('\nfield models:')
    dts_u = dts.astype(np.uint32)
    h, k = H0(dts_u.tolist())
    print(f'  ts delta (u32, mod 2^32)                : {h:7.3f} bits  ({k} distinct)'
          f'  -> {h*n/8:,.0f} B file')
    ddts = np.diff(dts)
    h, k = H0(ddts.tolist())
    print(f'  ts delta-of-delta (i64)                 : {h:7.3f} bits  ({k} distinct)'
          f'  -> {h*n/8:,.0f} B file')

    # f32 candidate models (stored word vs previous record word)
    # NOTE: record stride 14 is not 4-aligned, so gather explicitly.
    vw = np.array([struct.unpack_from('<I', d, P * i + 8)[0] for i in range(n)],
                  dtype=np.uint32)
    x = vw ^ np.roll(vw, 1)
    lz = np.array([max(0, 32 - int(v).bit_length()) if v else 32 for v in x[1:]],
                  dtype=np.int32)
    h_lz, k_lz = H0(lz.tolist())
    # significant bits (32-lz) for nonzero xors
    sig = np.array([32 - l for l in lz], dtype=np.int32)
    h_sig, k_sig = H0(sig.tolist())
    nz = x[1:] != 0
    print(f'  f32 XOR vs prev: lz H0={h_lz:.3f} bits ({k_lz} values); '
          f'sig-bits H0={h_sig:.3f} ({k_sig} values); nonzero {nz.sum()}/{len(nz)}')
    # bits: lz coded + (32-lz) raw bits -> estimate
    est = (h_lz + float(np.mean(sig[nz])) if nz.any() else 0.0)
    print(f'    [estimate] lz + raw significant bits = {est:.3f} bits/value')
    h_wd, k_wd = H0((((vw.astype(np.uint64) - np.roll(vw.astype(np.uint64), 1)) & 0xFFFFFFFF)[1:]).tolist())
    print(f'  f32 integer-bitpattern diff vs prev (u32): H0={h_wd:.3f} bits ({k_wd} distinct)')
    # byte-level delta per f32 byte column
    fbytes = np.frombuffer(d, dtype=np.uint8).reshape(n, P)[:, 8:12].astype(np.int32)
    tot_b = 0.0
    for b in range(4):
        dd = ((fbytes[1:, b] - fbytes[:-1, b]) & 0xFF)
        hh, kk = H0(dd.tolist())
        tot_b += hh
        print(f'    f32 byte {b}: delta H0={hh:.3f} bits ({kk} distinct)')
    print(f'    sum f32 byte deltas = {tot_b:.3f} bits/value')

    # id u16: delta per byte vs prev
    ibytes = np.frombuffer(d, dtype=np.uint8).reshape(n, P)[:, 12:14].astype(np.int32)
    tot_i = 0.0
    for b in range(2):
        dd = ((ibytes[1:, b] - ibytes[:-1, b]) & 0xFF)
        hh, kk = H0(dd.tolist())
        tot_i += hh
        print(f'  id byte {b}: delta H0={hh:.3f} bits ({kk} distinct)')
    print(f'  sum id byte deltas = {tot_i:.3f} bits/record')

    # conditional entropy of a column given the previous byte in the same column
    print('\nper-column order-1 (byte | prev same column) entropy:')
    tot1 = 0.0
    for j in range(P):
        col = np.frombuffer(d, dtype=np.uint8).reshape(n, P)[:, j].astype(np.int32)
        pairs = list(zip(col[:-1].tolist(), col[1:].tolist()))
        cnt = Counter(pairs)
        ctx = Counter(p[0] for p in pairs)
        hc = 0.0
        for (a, b), c in cnt.items():
            hc += c * -math.log2(c / ctx[a])
        hc /= len(pairs)
        tot1 += hc
        if j in (0, 1, 2, 3, 8, 9, 10, 11, 12, 13):
            print(f'  col {j:2d}: H(byte|prev)={hc:6.3f}')
    print(f'  sum over columns = {tot1:.1f} bits/record = {tot1*n/8:,.0f} B file')
    print(f'\nideal floor if ts=H0(delta) and f32=byte-delta sum and id=0: '
          f'{(H0(dts_u.tolist())[0] + tot_b)*n/8:,.0f} B')
    # entropy floor with f32 XOR lz+bits estimate
    est_f32 = h_lz + float(np.mean(sig[nz]))
    print(f'ideal floor with f32 XOR(lz+raw bits): '
          f'{(H0(dts_u.tolist())[0] + est_f32)*n/8:,.0f} B')


if __name__ == '__main__':
    main()
