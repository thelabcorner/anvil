#!/usr/bin/env python3
from pathlib import Path
import argparse,json,random,sqlite3
ap=argparse.ArgumentParser();ap.add_argument('out',nargs='?',default='tests/corpus-generated');a=ap.parse_args();root=Path(a.out);root.mkdir(parents=True,exist_ok=True)
r=random.Random(12345);rows=[]
for i in range(7000): rows.append({'id':i,'kind':['alpha','beta','gamma','delta'][i%4],'active':i%7!=0,'score':round((i*1.61803398875)%1000,6),'path':f'/api/v1/items/{i%317}/events/{i}','tags':[f't{i%19}',f'g{i%43}']})
(root/'generated.json').write_text(json.dumps(rows,separators=(',',':')))
with (root/'generated.log').open('w') as f:
    for i in range(18000): f.write(f'2026-08-12T19:{(i//60)%60:02d}:{i%60:02d}.123Z level={"INFO" if i%13 else "WARN"} worker={i%16} request={i:08x} route=/v1/item/{i%317} latency_ms={(i*37)%900} status={200 if i%29 else 503}\n')
(root/'random.bin').write_bytes(bytes(r.randrange(256) for _ in range(262144)))
db=root/'generated.sqlite'; db.unlink(missing_ok=True); c=sqlite3.connect(db);c.execute('pragma page_size=4096');c.execute('create table events(id integer primary key, category text, payload text, value real)');c.executemany('insert into events values(?,?,?,?)',[(i,['alpha','beta','gamma'][i%3],json.dumps(rows[i%len(rows)],separators=(',',':')),(i*0.125)%100) for i in range(12000)]);c.commit();c.close()
for p in sorted(root.iterdir()):print(p,p.stat().st_size)
