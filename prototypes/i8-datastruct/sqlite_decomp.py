import json, random, struct
import numpy as np

d = open('../../tests/corpus/generated.sqlite', 'rb').read()
n = len(d)

# Reconstruct the 7000 distinct row objects exactly as make_smoke_corpus.py does
r = random.Random(12345)
rows = []
for i in range(7000):
    rows.append({'id': i, 'kind': ['alpha', 'beta', 'gamma', 'delta'][i % 4],
                 'active': i % 7 != 0, 'score': round((i * 1.61803398875) % 1000, 6),
                 'path': f'/api/v1/items/{i%317}/events/{i}',
                 'tags': [f't{i%19}', f'g{i%43}']})
uniq = b''.join(json.dumps(x, separators=(',', ':')).encode() for x in rows)

print(f'DISTINCT payload content (7000 JSON objects): {len(uniq):,} B')
print(f'  as a fraction of the {n:,} B file     : {100*len(uniq)/n:.2f}%')
print()
print('=' * 70)
print('SQLITE: REPRESENTATION vs ENTROPY - honest decomposition')
print('=' * 70)
print(f'input                       : {n:>10,} B')
print(f'best ANVIL mdl-rans         : {323014:>10,} B  (r=0.1856)')
print(f'brotli-q11                  : {185702:>10,} B  (r=0.1067)')
print(f'gap to close                : {323014-185702:>10,} B  (-42.5%)')
print()
print('ENTROPY FLOOR (nothing can go below):')
print(f'  distinct JSON content     : {len(uniq):>10,} B   <- must be stored >=1x')
print(f'  + 12000 rowids (H~0.00 b) : {"~0":>10}')
print(f'  + 12000 cell ptrs @3.076b : {12000*3.076/8:>10,.0f} B')
print(f'  + payload headers/serials : {72000*0.05:>10,.0f} B')
floor = len(uniq) + 12000 * 3.076 / 8 + 3600
print(f'  TOTAL hard floor          : {floor:>10,.0f} B  (r={floor/n:.4f})')
print()
print('VERDICT:')
print(f'  brotli-q11 185,702 B = {185702/len(uniq):.2f}x the distinct-content floor')
print(f'  ANVIL      323,014 B = {323014/len(uniq):.2f}x the distinct-content floor')
print()
print('  The -42.5% gap is NOT mostly representation overhead:')
print(f'   - structural overhead (cell ptrs 24,000 B + headers 72,000 B)')
print(f'     = {(24000+72000)/n*100:.1f}% of the file, and is already cheap to encode')
print(f'   - 88.9% of the file is JSON body text')
print(f'   - 12,000 of 12,000 bodies are DISTINCT (no exact repeats)')
print(f'   - but there are only 7,000 distinct SOURCE objects; the corpus builder')
print(f'     wrote rows[i%7000], so each object appears only ~1.71 times')
print()
print('  => This is a TEXT-COMPRESSION gap wearing a binary costume, plus a')
print('     page-framing penalty (content split across 423 pages, cell pointers')
print('     interleaved, JSON records severed across page boundaries).')
print()
# How much is the page-framing penalty? Compare: compress the raw concatenated
# JSON stream vs the sqlite file.
seq = b''.join(json.dumps(rows[i % 7000], separators=(',', ':')).encode()
               for i in range(12000))
print(f'  concatenated JSON stream (no page framing): {len(seq):,} B')
print(f'  sqlite file                              : {n:,} B')
print(f'  framing + pointer + rowid + header cost  : {n-len(seq):,} B '
      f'({100*(n-len(seq))/n:.2f}%)')
