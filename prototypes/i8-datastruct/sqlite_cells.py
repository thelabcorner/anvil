import struct
import numpy as np
from collections import Counter

d = open('../../tests/corpus/generated.sqlite', 'rb').read()
ps = 4096
n = len(d)
npages = n // ps


def rv(b, p):
    v = 0
    for i in range(8):
        if p + i >= len(b):
            return None, p
        x = b[p + i]
        if i == 7:
            v = (v << 8) | x
            return v, p + 8
        v = (v << 7) | (x & 0x7F)
        if not (x & 0x80):
            return v, p + i + 1
    return None, p


def ent(a):
    a = np.asarray(a)
    if a.size == 0:
        return 0.0
    u, c = np.unique(a, return_counts=True)
    p = c.astype(float) / a.size
    return float(-(p * np.log2(p)).sum())


types = Counter()
for i in range(npages):
    types[d[i * ps]] += 1
print('page type histogram (raw type byte):')
for k, v in types.most_common():
    print(f'   type {k:<4}: {v} pages')

leaf = [i for i in range(npages) if d[i * ps] == 13]
print(f'\ntable-leaf pages (type 13): {len(leaf)}')

# find the JSON payload
pat = b'{"id"'
mj = d.find(pat)
print(f'first JSON payload at byte {mj}')
print('sample:', d[mj:mj + 160])

# Count occurrences - rows repeat every 7000
print(f'\nJSON record count (approx): {d.count(pat):,}')

# Parse table-leaf pages properly
print('\n--- table-leaf cell parse ---')
ncell_total = 0
rowids = []
ptr_deltas = []
hdrlens = []
serials = []
bodies = []
for pi in leaf:
    off = pi * ps
    nc = struct.unpack_from('>H', d, off + 3)[0]
    ncell_total += nc
    prev = None
    for c in range(nc):
        pp = off + 8 + 2 * c
        if pp + 2 > n:
            break
        celloff = struct.unpack_from('>H', d, pp)[0]
        if prev is not None:
            ptr_deltas.append(celloff - prev)
        prev = celloff
        cp = off + celloff
        if cp < 1 or cp + 2 > min(n, off + ps):
            continue
        plen, q = rv(d, cp)
        if plen is None:
            continue
        rowid, q2 = rv(d, q)
        if rowid is None:
            continue
        rowids.append(rowid)
        payload = d[q2:q2 + plen]
        if len(payload) < 1 or q2 + plen > n:
            continue
        hl, r = rv(payload, 0)
        if hl is None:
            continue
        st = []
        while r < hl and r < len(payload):
            s, r = rv(payload, r)
            if s is None:
                break
            st.append(s)
        serials.append(tuple(st))
        hdrlens.append(hl)
        bodies.append(payload[hl:])

print(f'cells total (incl. skipped): {ncell_total:,}')
print(f'parsed rowids : {len(rowids):,}')
print(f'parsed bodies : {len(bodies):,}')
if rowids:
    rd = [rowids[i + 1] - rowids[i] for i in range(len(rowids) - 1)]
    print(f'rowid H(raw)={ent(rowids):.3f} b  H(delta)={ent(rd):.4f} b '
          f'distinct={len(set(rd))}')
if ptr_deltas:
    print(f'ptr   H(delta)={ent(ptr_deltas):.3f} b distinct={len(set(ptr_deltas))}'
          f'  top={Counter(ptr_deltas).most_common(5)}')
if hdrlens:
    print(f'payload hdr_len: total={sum(hdrlens):,} B  H={ent(hdrlens):.3f} b '
          f'distinct={len(set(hdrlens))}')
if serials:
    print(f'serial-type tuples: distinct={len(set(serials))} '
          f'top={Counter(serials).most_common(3)}')
if bodies:
    tb = sum(len(b) for b in bodies)
    print(f'body bytes: {tb:,}  ({100 * tb / n:.2f}% of file)')
    print(f'body sample: {bodies[0][:120]}')
    # how many DISTINCT bodies?
    ub = len(set(bodies))
    print(f'DISTINCT bodies: {ub:,} of {len(bodies):,}')
