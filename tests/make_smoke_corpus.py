#!/usr/bin/env python3
from pathlib import Path
import argparse,json,random,sqlite3
ap=argparse.ArgumentParser();ap.add_argument('out',nargs='?',default='tests/corpus-generated');a=ap.parse_args();root=Path(a.out);root.mkdir(parents=True,exist_ok=True)
r=random.Random(12345);rows=[]
for i in range(7000): rows.append({'id':i,'kind':['alpha','beta','gamma','delta'][i%4],'active':i%7!=0,'score':round((i*1.61803398875)%1000,6),'path':f'/api/v1/items/{i%317}/events/{i}','tags':[f't{i%19}',f'g{i%43}']})
(root/'generated.json').write_text(json.dumps(rows,separators=(',',':')))
with (root/'generated.log').open('w') as f:
    for i in range(18000): f.write(f'2026-08-12T19:{(i//60)%60:02d}:{i%60:02d}.123Z level={"INFO" if i%13 else "WARN"} worker={i%16} request={i:08x} route=/v1/item/{i%317} latency_ms={(i*37)%900} status={200 if i%29 else 503}\n')
# SPARSE-REF stress corpus (research ask, agenda §5): long mostly-identical
# JSON-lines records; a few bytes change per record at consistent positions
# (same fields) and the changed content is not repeated; every 200 records one
# structural drift (an extra field) shifts the correction mask.
r2=random.Random(424242)
with (root/'generated.jsonl').open('w') as f:
    for i in range(12000):
        sess=r2.randrange(0x10000)
        rec=(f'{{"id":"{i:06x}","ts":"2026-08-12T19:{(i//60)%60:02d}:{i%60:02d}.{(i*37)%1000:03d}Z",'
             f'"kind":"{["alpha","beta","gamma","delta"][i%4]}","event":"receive",'
             f'"route":"/api/v1/items/317/events","region":"us-east-1","user":17,"status":"OK",'
             f'"value":{round((i*1.61803398875)%1000,3)},"note":"checksum {(i*2654435761)%0x10000:04x}",'
             f'"session":"{sess:04x}","flags":"0x0000"}}')
        if i>0 and i%200==0: rec=rec.replace('"flags":"0x0000"','"flags":"0x0000","extra":"drift"')
        f.write(rec+'\n')
# Pure-repetition control: identical records repeated. SPARSE-REF must equal
# exact-LZ here (no regression) — the ablation baseline (agenda §1.4).
with (root/'generated.repeat.jsonl').open('w') as f:
    rep='{"id":"0000a1","ts":"2026-08-12T19:04:02.123Z","kind":"alpha","event":"receive","route":"/api/v1/items/317/events","region":"us-east-1","user":17,"status":"OK","value":874.123,"note":"checksum 9f2c","session":"a1b2","flags":"0x0000"}\n'
    for _ in range(4000): f.write(rep)
(root/'random.bin').write_bytes(bytes(r.randrange(256) for _ in range(262144)))
db=root/'generated.sqlite'; db.unlink(missing_ok=True); c=sqlite3.connect(db);c.execute('pragma page_size=4096');c.execute('create table events(id integer primary key, category text, payload text, value real)');c.executemany('insert into events values(?,?,?,?)',[(i,['alpha','beta','gamma'][i%3],json.dumps(rows[i%len(rows)],separators=(',',':')),(i*0.125)%100) for i in range(12000)]);c.commit();c.close()
for p in sorted(root.iterdir()):print(p,p.stat().st_size)
