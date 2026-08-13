#!/usr/bin/env python3
from __future__ import annotations
import argparse, hashlib, os, random, subprocess, tempfile
from pathlib import Path


def run(cmd, ok=True):
    p=subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if ok and p.returncode:
        raise RuntimeError(f"command failed: {' '.join(map(str,cmd))}\n{p.stderr.decode(errors='replace')}")
    return p


def patterns(rng: random.Random, count: int):
    fixed=[b'',b'\0',b'a',b'a'*3,b'a'*4,b'a'*100000,bytes(range(256)),bytes(range(256))*256,
           (b'abc123XYZ\n'*8192), os.urandom(65536)]
    yield from fixed
    for _ in range(count):
        n=rng.randrange(0,32769)
        kind=rng.randrange(6)
        if kind==0: data=bytes(rng.randrange(256) for _ in range(n))
        elif kind==1:
            motif=bytes(rng.randrange(256) for _ in range(rng.randrange(1,65)))
            data=(motif*((n+len(motif)-1)//len(motif)))[:n]
        elif kind==2: data=bytes((i*17+i//31)&255 for i in range(n))
        elif kind==3: data=(b'{"key":123,"name":"anvil","ok":true}\n'*((n//40)+1))[:n]
        elif kind==4:
            buf=bytearray(os.urandom(n))
            for __ in range(min(64,n//16)):
                if n<16: break
                a=rng.randrange(0,n-8); b=rng.randrange(0,n-8); l=min(rng.randrange(4,128),n-a,n-b); buf[b:b+l]=buf[a:a+l]
            data=bytes(buf)
        else: data=bytes([rng.randrange(8)])*n
        yield data


def mutate(rng: random.Random, blob: bytes, n: int):
    """Yield random single/multi-byte mutations of a valid file. A strict
    decoder must either reject the mutation (nonzero exit) or reconstruct the
    exact same bytes (CRC still matches); accepting a different output is a bug."""
    b = bytearray(blob)
    for _ in range(n):
        m = bytearray(b)
        for __ in range(rng.randrange(1, 4)):
            op = rng.randrange(3)
            pos = rng.randrange(len(m))
            if op == 0:
                m[pos] ^= 1 << rng.randrange(8)          # bit flip
            elif op == 1:
                m[pos] = rng.randrange(256)               # byte overwrite
            else:
                del m[pos]                                # byte drop (shifts framing)
        yield bytes(m)


def main():
    ap=argparse.ArgumentParser(); ap.add_argument('--exe',default=str(Path(__file__).parents[1]/'anvil')); ap.add_argument('--cases',type=int,default=120); ap.add_argument('--seed',type=int,default=0xA11E); ap.add_argument('--mutations',type=int,default=6,help='mutations per valid file (0 to disable)'); args=ap.parse_args()
    exe=Path(args.exe); rng=random.Random(args.seed)
    combos=[('greedy','arith'),('dp','arith'),('greedy','rans'),('dp','rans'),('sparse','rans')]
    total=0; mutated=0
    with tempfile.TemporaryDirectory(prefix='anvil-fuzz-') as td:
        td=Path(td)
        for idx,data in enumerate(patterns(rng,args.cases)):
            src=td/'in.bin'; src.write_bytes(data)
            for parse,entropy in combos:
                packed=td/'x.anv'; dec=td/'out.bin'
                run([exe,'c',src,packed,f'--parse={parse}','--literal=o0',f'--entropy={entropy}','--quiet'])
                run([exe,'d',packed,dec,'--quiet'])
                got=dec.read_bytes()
                if hashlib.sha256(got).digest()!=hashlib.sha256(data).digest(): raise RuntimeError(f'roundtrip mismatch case={idx} {parse}/{entropy}')
                blob=packed.read_bytes()
                # Representative truncation checks. Any truncation of a valid file must fail.
                if len(blob)>1:
                    points={0,1,len(blob)//4,len(blob)//2,max(0,len(blob)-1)}
                    for cut in points:
                        bad=td/'trunc.anv'; bad.write_bytes(blob[:cut]); p=run([exe,'d',bad,dec,'--quiet'],ok=False)
                        if p.returncode==0: raise RuntimeError(f'truncation accepted case={idx} cut={cut}')
                # Mutation checks. Accepting a mutation with different output is a bug;
                # rejecting it (or accepting with identical output) is correct.
                if args.mutations and len(blob)>8:
                    for m_i,mut in enumerate(mutate(rng,blob,args.mutations)):
                        bad=td/'mut.anv'; bad.write_bytes(mut)
                        p=run([exe,'d',bad,dec,'--quiet'],ok=False)
                        if p.returncode==0:
                            out=dec.read_bytes()
                            if out!=data: raise RuntimeError(f'mutation accepted with DIFFERENT output case={idx} {parse}/{entropy} mut={m_i}')
                        mutated+=1
                total+=1
    print(f'PASS seed={args.seed} roundtrip_variants={total} mutations={mutated}')

if __name__=='__main__': main()
