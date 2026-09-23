"""Measure the SQLite page-framing penalty empirically.

Question the coordinator's assignment asks directly: "say honestly how much of
the -42.5% gap is representation vs entropy."

Two bounds:
  LOWER BOUND on the framing penalty: compress the payload stream with the
  framing REMOVED (payloads concatenated). If even that beats the sqlite file
  by X%, then X% is attributable to framing.
  UPPER BOUND check: also compress the sqlite file with the cell-pointer
  arrays and free space ZEROED (structure removed, content kept in place). If
  that is barely smaller, structure bytes are already cheap and the gap is
  TEXT-compression depth, not structure.

Uses brotli via the anvil_bench binary? No - use python brotli if available,
else report sizes only. Round-trip is not applicable (this is a measurement of
substitutes, not a codec).
"""
import json, random, struct, subprocess, os, sys

d = open('../../tests/corpus/generated.sqlite', 'rb').read()
n = len(d)
ps = 4096

r = random.Random(12345)
rows = []
for i in range(7000):
    rows.append({'id': i, 'kind': ['alpha', 'beta', 'gamma', 'delta'][i % 4],
                 'active': i % 7 != 0, 'score': round((i * 1.61803398875) % 1000, 6),
                 'path': f'/api/v1/items/{i%317}/events/{i}',
                 'tags': [f't{i%19}', f'g{i%43}']})

try:
    import brotli
    HAVE = True
except ImportError:
    HAVE = False
    print('no python brotli - install or use anvil_bench')


def sz(b):
    return len(b)


def comp(b, q=11):
    if not HAVE:
        return None
    return len(brotli.compress(b, quality=q))


seq = b''.join(json.dumps(rows[i % 7000], separators=(',', ':')).encode()
               for i in range(12000))
open('tmp_payload_stream.bin', 'wb').write(seq)

# variant: sqlite file with cell pointer arrays + free space zeroed
z = bytearray(d)
nptr = 0
for pi in range(1, n // ps):
    off = pi * ps
    if z[off] != 13:
        continue
    nc = struct.unpack_from('>H', z, off + 3)[0]
    cc = struct.unpack_from('>H', z, off + 5)[0] or 65536
    # zero the pointer array
    base = off + 8
    for c in range(nc):
        p = base + 2 * c
        if p + 2 <= len(z):
            z[p] = 0
            z[p + 1] = 0
            nptr += 1
    # zero free space between header+ptrs and cell content
    fstart = base + 2 * nc
    fend = off + cc
    for p in range(fstart, min(fend, len(z))):
        z[p] = 0
open('tmp_sqlite_nostruct.bin', 'wb').write(bytes(z))

print(f'{"variant":<38} {"raw B":>10} {"brotli-q11":>11} {"ratio":>8}')
print('-' * 70)
for name, blob in (('sqlite (as-is)', d),
                   ('sqlite ptr-array+free zeroed', bytes(z)),
                   ('payload stream (framing removed)', seq)):
    c = comp(blob)
    print(f'{name:<38} {len(blob):>10,} {c if c else "-":>11} '
          f'{(c/len(blob)) if c else 0:>8.4f}')
print()
if HAVE:
    a = comp(d)
    b = comp(bytes(z))
    c = comp(seq)
    print(f'framing penalty (q11, as-is -> payload stream): '
          f'{100*(a-c)/a:+.2f}%  ({a:,} -> {c:,} B)')
    print(f'structure-bytes penalty (as-is -> ptrs+free zeroed): '
          f'{100*(a-b)/a:+.2f}%  ({a:,} -> {b:,} B)')
    print()
    print('NOTE: the payload stream is a DIFFERENT file (shorter input), so the')
    print('comparison is a BOUND on framing cost, not an achievable codec result.')
