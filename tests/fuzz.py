#!/usr/bin/env python3
from __future__ import annotations
import argparse, csv, hashlib, os, random, struct, subprocess, tempfile, zlib
from pathlib import Path


def crc32(data: bytes) -> int:
    return zlib.crc32(data) & 0xFFFFFFFF


def run(cmd, ok=True):
    p=subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if ok and p.returncode:
        raise RuntimeError(f"command failed: {' '.join(map(str,cmd))}\n{p.stderr.decode(errors='replace')}")
    return p


def read_uvar(blob: bytes, pos: int):
    x=0; shift=0
    for _ in range(10):
        if pos>=len(blob): raise ValueError('truncated uvar')
        b=blob[pos]; pos+=1; x|=(b&0x7f)<<shift
        if not (b&0x80): return x,pos
        shift+=7
    raise ValueError('uvar overflow')


def put_uvar(x: int) -> bytes:
    out=bytearray()
    while True:
        b=x&0x7f; x>>=7
        if x: b|=0x80
        out.append(b)
        if not x: return bytes(out)


def ratio_layout(blob: bytes):
    """Return offsets for the first rev-2 mode-17 block in a valid file."""
    if len(blob)<5 or blob[:4]!=b'ANV0' or blob[4]!=2: raise ValueError('not rev2')
    p=5
    block_size,p=read_uvar(blob,p); total,p=read_uvar(blob,p)
    blen,p=read_uvar(blob,p)
    if p>=len(blob): raise ValueError('truncated block mode')
    mode=blob[p]; p+=1
    plen,p=read_uvar(blob,p)
    if p+4>len(blob): raise ValueError('truncated crc')
    p+=4
    payload=p
    if mode!=17 or payload+plen!=len(blob): raise ValueError('expected single mode17 block')
    if plen<3: raise ValueError('short ratio payload')
    transform_off=payload; backend_off=payload+1; xlen_off=payload+2
    xlen,xlen_end=read_uvar(blob,xlen_off)
    return {
        'block_size': block_size, 'total': total, 'blen': blen, 'plen': plen,
        'transform_off': transform_off, 'backend_off': backend_off,
        'xlen_off': xlen_off, 'xlen_end': xlen_end, 'xlen': xlen,
        'backend_payload_off': xlen_end,
    }


def bwt_inner_layout(blob: bytes):
    """Parse a single, un-subblocked backend-2 payload for test assertions."""
    lay=ratio_layout(blob)
    if blob[lay['transform_off']] != 0 or blob[lay['backend_off']] != 2:
        raise ValueError('expected direct/BWT ratio payload')
    p=lay['backend_payload_off']
    e=len(blob)
    if p>=e: raise ValueError('truncated BWT payload')
    if blob[p] == 0xff:
        raise ValueError('subblocked BWT payload not supported by this helper')
    if blob[p] == 0xfe:
        tag_off=p; p+=1
        if p>=e: raise ValueError('truncated auxiliary postcoder')
        post=blob[p]; p+=1
        rate_off=p; rate,p=read_uvar(blob,p); rate_end=p
        count_off=p; count,p=read_uvar(blob,p); count_end=p
        index_off=p
        index_bytes=4*count
        if p+index_bytes>e: raise ValueError('truncated auxiliary indexes')
        indexes=[struct.unpack_from('<I',blob,p+4*i)[0] for i in range(count)]
        p+=index_bytes
        return {
            **lay, 'aux': True, 'tag_off': tag_off, 'post': post,
            'rate': rate, 'rate_off': rate_off, 'rate_end': rate_end,
            'count': count, 'count_off': count_off, 'count_end': count_end,
            'index_off': index_off, 'indexes': indexes, 'postdata_off': p,
        }
    post=blob[p]; p+=1
    primary_off=p; primary,p=read_uvar(blob,p)
    return {
        **lay, 'aux': False, 'post': post, 'primary': primary,
        'primary_off': primary_off, 'postdata_off': p,
    }


def replace_uvar_same_width(blob: bytes, lo: int, hi: int, value: int) -> bytes:
    enc=put_uvar(value)
    if len(enc)!=hi-lo: raise ValueError('replacement changes uvar width')
    return blob[:lo]+enc+blob[hi:]


def require_reject(exe: Path, bad: Path, dec: Path, label: str):
    p=run([exe,'d',bad,dec,'--quiet'],ok=False)
    if p.returncode==0: raise RuntimeError(f'adversarial rev2 case accepted: {label}')


def adversarial_rev2(exe: Path, td: Path) -> int:
    """Deterministic malformed-header cases for the rev-2 mode-17 contract."""
    src=td/'rev2-source.bin'; packed=td/'rev2-valid.anv'; dec=td/'rev2.out'; bad=td/'rev2-bad.anv'
    data=(b'ANVIL-RATIO-REV2\x00'*4096)[:65536]
    src.write_bytes(data)
    run([exe,'c',src,packed,'--parse=ratio','--ratio-context=off','--ratio-lines=off','--quiet'])
    blob=packed.read_bytes(); lay=ratio_layout(blob)
    if blob[lay['transform_off']]!=0 or blob[lay['backend_off']]!=1:
        raise RuntimeError('deterministic rev2 seed did not use direct/Brotli')
    run([exe,'d',packed,dec,'--quiet'])
    if dec.read_bytes()!=data: raise RuntimeError('deterministic rev2 seed roundtrip failed')

    cases=[]
    m=bytearray(blob); m[lay['transform_off']]=0xff; cases.append(('unknown-transform-id',bytes(m)))
    m=bytearray(blob); m[lay['transform_off']]=4; cases.append(('invalid-transform-id-4',bytes(m)))
    m=bytearray(blob); m[lay['backend_off']]=0xff; cases.append(('unknown-backend-id',bytes(m)))
    m=bytearray(blob); m[lay['backend_off']]=0; cases.append(('reserved-backend-id-0',bytes(m)))
    if lay['xlen']<=1: raise RuntimeError('unexpected tiny rev2 transformed size')
    cases.append(('transformed-size-smaller',replace_uvar_same_width(blob,lay['xlen_off'],lay['xlen_end'],lay['xlen']-1)))
    cases.append(('transformed-size-larger',replace_uvar_same_width(blob,lay['xlen_off'],lay['xlen_end'],lay['xlen']+1)))
    # Same-width value deliberately above decode bound 2*blen+4096.
    huge=2*lay['blen']+4097
    cases.append(('transformed-size-over-bound',replace_uvar_same_width(blob,lay['xlen_off'],lay['xlen_end'],huge)))
    cases.append(('truncated-backend-payload',blob[:-1]))

    # Rev 2 intentionally rejects all legacy nonzero block modes before payload decode.
    mode_off=None; p=5
    _,p=read_uvar(blob,p); _,p=read_uvar(blob,p); _,p=read_uvar(blob,p); mode_off=p
    m=bytearray(blob); m[mode_off]=10; cases.append(('rev2-legacy-mode',bytes(m)))

    for label,mut in cases:
        bad.write_bytes(mut); require_reject(exe,bad,dec,label)
    return len(cases)


def put_u32le(x: int) -> bytes:
    return struct.pack('<I', x & 0xFFFFFFFF)


def bwt_layout(blob: bytes):
    """Return offsets for the first rev-2 mode-17 backend-2 (BWT) block in a valid file."""
    if len(blob) < 5 or blob[:4] != b'ANV0' or blob[4] != 2:
        raise ValueError('not rev2')
    p = 5
    block_size, p = read_uvar(blob, p)
    total, p = read_uvar(blob, p)
    blen, p = read_uvar(blob, p)
    if p >= len(blob):
        raise ValueError('truncated block mode')
    mode = blob[p]; p += 1
    plen, p = read_uvar(blob, p)
    if p + 4 > len(blob):
        raise ValueError('truncated crc')
    p += 4
    payload = p
    if mode != 17 or payload + plen != len(blob):
        raise ValueError('expected single mode17 block')
    if plen < 3:
        raise ValueError('short ratio payload')
    transform_off = payload
    backend_off = payload + 1
    xlen_off = payload + 2
    xlen, xlen_end = read_uvar(blob, xlen_off)
    # backend payload begins right after transformed_size uvar
    bp_off = xlen_end
    postcoder_off = bp_off
    primary, primary_end = read_uvar(blob, bp_off + 1)
    return {
        'block_size': block_size, 'total': total, 'blen': blen, 'plen': plen,
        'transform_off': transform_off, 'backend_off': backend_off,
        'xlen_off': xlen_off, 'xlen_end': xlen_end, 'xlen': xlen,
        'backend_payload_off': bp_off, 'postcoder_off': postcoder_off,
        'primary_off': bp_off + 1, 'primary_end': primary_end, 'primary': primary,
        'backend_payload_end': len(blob),
    }


def build_rev2(block: bytes, backend_payload: bytes, block_size: int = 134217728,
               transformed_size: int | None = None):
    """Build a single-block rev-2 file wrapping a backend-2 payload."""
    if transformed_size is None:
        transformed_size = len(block)
    payload = bytes([0, 2]) + put_uvar(transformed_size) + backend_payload  # transform=0, backend=2
    out = bytearray(b'ANV0')
    out += bytes([2])            # revision
    out += put_uvar(block_size)
    out += put_uvar(len(block))  # total
    out += put_uvar(len(block))  # blen
    out += bytes([17])           # mode 17
    out += put_uvar(len(payload))
    out += put_u32le(crc32(block))
    out += payload
    return bytes(out)


def bwt_payload(postcoder: int, primary: int, sub: bytes) -> bytes:
    """Backend-2 payload: postcoder byte, primary uvar, postcoder sub-payload."""
    return bytes([postcoder]) + put_uvar(primary) + sub


def require_reject_bwt(exe: Path, bad: Path, dec: Path, label: str):
    p = run([exe, 'd', bad, dec, '--quiet'], ok=False)
    # nonzero exit required; but decode may also need --quiet to suppress output. We still
    # treat nonzero as reject. Some cases might produce output but nonzero rc is the signal.
    if p.returncode == 0:
        raise RuntimeError(f'adversarial BWT case accepted: {label}')


def adversarial_bwt(exe: Path, td: Path) -> tuple[int, list[str]]:
    """Deterministic malformed-payload matrix for backend-2 (BWT) postcoders.

    Covers every malformed case required by E0. Returns (count, list_of_skipped).
    Degenerate round-trip cases (1-byte / all-equal uniform inputs) are exercised
    as true encode->decode round-trips and asserted byte-exact; they are no longer
    skipped now that arch-bwt's libsais degenerate-input fix (n->1 mapping, n==1
    raw short-circuit) is landed and verified.
    """
    skipped: list[str] = []
    src = td / 'bwt-source.bin'
    packed = td / 'bwt-valid.anv'
    dec = td / 'bwt.out'
    bad = td / 'bwt-bad.anv'
    # Use the BWT backend directly so the seed is a genuine mode-17 backend-2 block.
    data = (b'The quick brown fox jumps over the lazy dog. ' * 400)
    src.write_bytes(data)
    run([exe, 'c', src, packed, '--parse=ratio', '--ratio-backend=bwt',
         '--ratio-context=off', '--ratio-lines=off', '--quiet'])
    blob = packed.read_bytes()
    lay = bwt_layout(blob)
    # The encoder may fall back to raw mode-0 for tiny inputs; ensure we got a BWT block.
    if blob[lay['backend_off']] != 2:
        raise RuntimeError('deterministic BWT seed did not use backend 2')
    run([exe, 'd', packed, dec, '--quiet'])
    if dec.read_bytes() != data:
        raise RuntimeError('deterministic BWT seed roundtrip failed')
    # Snapshot the whole valid backend payload for structural mutation.
    bp = blob[lay['backend_payload_off']:lay['backend_payload_end']]
    xlen = lay['xlen']
    postcoder = bp[0]

    cases: list[tuple[str, bytes]] = []

    # --- identity / postcoder selection ---
    m = bytearray(blob); m[lay['postcoder_off']] = 0xFF
    cases.append(('unknown-postcoder-id', bytes(m)))

    # --- primary index bounds ---
    # primary == output length (== transformed_size): must reject
    cases.append(('primary-eq-output-length',
                  blob[:lay['primary_off']] + put_uvar(xlen) + blob[lay['primary_end']:]))
    # primary > output length: must reject
    cases.append(('primary-gt-output-length',
                  blob[:lay['primary_off']] + put_uvar(xlen + 1) + blob[lay['primary_end']:]))

    # --- truncated / trailing backend payload (postcoder 0) ---
    cases.append(('truncated-backend-payload', blob[:-1]))
    # backend trailing bytes past the postcoder-0 streams (decoder requires p==e)
    cases.append(('backend-trailing-bytes', blob + b'\x00\x00'))

    # --- arithmetic postcoder (1/2): malformed bit-count / padding / truncation ---
    # These reject on bit_count math BEFORE any arithmetic decode, so a hand-built
    # postcoder-1 payload (no valid arithmetic stream needed) suffices.
    def arith_case(name, nbits, payload_bytes, primary=1, postcoder=1):
        # Bit-count checks reject before any arithmetic decode, so the block content
        # is irrelevant for rejection; use a small placeholder block.
        tsz = max(1, (nbits + 7) // 8)
        blk = bytes([0]) * tsz
        sub = put_uvar(nbits) + payload_bytes
        pl = bwt_payload(postcoder, primary, sub)
        return name, build_rev2(blk, pl)

    # zero arithmetic bit count
    cases.append(arith_case('zero-arith-bit-count', 0, b''))
    # bit count larger than payload*8
    cases.append(arith_case('bit-count-gt-payload*8', 17, b'\x00\x00'))  # need=3, n=2 -> 17>16
    # bit count requiring fewer bytes than payload (byte-count mismatch)
    cases.append(arith_case('bit-count-byte-mismatch-fewer', 8, b'\x00\x00'))  # need=1, n=2
    # bit count requiring more bytes than payload (byte-count mismatch)
    cases.append(arith_case('bit-count-byte-mismatch-more', 17, b'\x00'))  # need=3, n=1
    # nonzero final padding bits (bit_count=15, 2 bytes, last byte low bit set)
    cases.append(arith_case('nonzero-padding-bits', 15, b'\x00\x01'))
    # truncated arithmetic payload: bit_count needs 5 bytes but only 3 present -> byte mismatch
    cases.append(arith_case('truncated-arith-payload', 40, b'\x00\x00\x00'))

    # --- postcoder 0 MTF/RLE structural malformations (mutate the real seed region) ---
    # Flip the postcoder id to 0 if it isn't already, so these target postcoder 0.
    if postcoder != 0:
        seed = bytearray(blob); seed[lay['postcoder_off']] = 0
        bp = bytes(seed)[lay['backend_payload_off']:lay['backend_payload_end']]
    # token/run overflow and stream truncation are exercised by byte-level flips/drops
    # in the postcoder-0 payload region, plus explicit from-scratch postcoder-0 builds.

    # Build a tiny from-scratch postcoder-0 block for the MTF overflow / run overflow /
    # stream-trailing / size-mismatch cases, using a real 4-byte block "AAAA".
    # MTF of "AAAA" (after BWT="AAAA", primary chosen to be nonzero to avoid the
    # open primary==0 bug; we use primary=3 which roundtrips) -> tokens=[0], runs=[uvar(3)].
    def p0_block(block, primary, tok_stream: bytes, run_stream: bytes):
        sub = put_uvar(len(tok_stream)) + tok_stream + put_uvar(len(run_stream)) + run_stream
        pl = bwt_payload(0, primary, sub)
        return build_rev2(block, pl)

    tok = bytes([0]) + put_uvar(1) + bytes([0])      # raw stream: 1 token byte 0
    run_stream = bytes([0]) + put_uvar(1) + put_uvar(3)    # raw stream: uvar(3)
    # MTF token output overflow: token stream decodes to > expected (raw_n=5 > 4)
    tok_over = bytes([0]) + put_uvar(5) + put_uvar(5) + bytes([0] * 5)
    run_empty = bytes([0]) + put_uvar(0)
    cases.append(('mtf-token-output-overflow',
                  p0_block(b'AAAA', 3, tok_over, run_empty)))
    # zero-run output overflow: run value 1000 -> run length 1001 > 4
    run_big = bytes([0]) + put_uvar(1) + put_uvar(1000)
    cases.append(('zero-run-output-overflow',
                  p0_block(b'AAAA', 3, tok, run_big)))
    # truncated token substream: declared len 10 but only 1 byte
    cases.append(('truncated-token-substream',
                  p0_block(b'AAAA', 3, bytes([0]) + put_uvar(10) + tok[:1], run_empty)))
    # truncated run substream: declared len 10 but 0 bytes
    cases.append(('truncated-run-substream',
                  p0_block(b'AAAA', 3, tok, bytes([0]) + put_uvar(10))))
    # token stream trailing bytes (extra bytes past declared tok_len)
    tok_extra = tok + b'\x00\x00'
    cases.append(('token-stream-trailing-bytes',
                  p0_block(b'AAAA', 3, tok_extra, run_stream)))
    # run stream trailing bytes
    run_extra = run_stream + b'\x00\x00'
    cases.append(('run-stream-trailing-bytes',
                  p0_block(b'AAAA', 3, tok, run_extra)))
    # token/run consumption mismatch (reconstructs fewer than expected)
    tok_short = bytes([0]) + put_uvar(1) + bytes([0])
    run_short = bytes([0]) + put_uvar(1) + put_uvar(1)  # run len 2
    cases.append(('token-run-size-mismatch',
                  p0_block(b'AAAA', 3, tok_short, run_short)))

    # --- malformed nested stream codec inside postcoder 0 ---
    # A stream-suite mode with a broken inner header (mode byte 0x7F unknown, or
    # truncated model) embedded in the token stream must be rejected by decode_stream.
    # token stream: [mode=7 (reap, disallowed nesting at depth 0?)] -> actually mode 7/8
    # only allowed at depth>0; at top level mode>=7 && depth==0 is allowed (F4 gates depth>0).
    # Use a clearly invalid stream mode byte 0x09.
    bad_nested = bytes([0x09]) + put_uvar(0)
    cases.append(('malformed-nested-stream-codec',
                  p0_block(b'AAAA', 3, bad_nested, run_empty)))
    # truncated inner stream header (mode byte only, no raw_n)
    cases.append(('truncated-nested-stream-header',
                  p0_block(b'AAAA', 3, bytes([0]), run_empty)))

    # --- raw BWT postcoder (3): decoded-length mismatch ---
    def p3_block(block, primary, raw_bytes: bytes):
        # raw stream: mode 0, uvar(n), n bytes
        sub = bytes([0]) + put_uvar(len(raw_bytes)) + raw_bytes
        pl = bwt_payload(3, primary, sub)
        return build_rev2(block, pl)

    # "AAAA" BWT is "AAAA"; primary 3 roundtrips. Mismatch: wrong raw length.
    cases.append(('raw-bwt-decoded-length-mismatch',
                  p3_block(b'AAAA', 3, b'AAB')))  # claims 3 bytes, block expects 4
    # raw stream trailing bytes
    cases.append(('raw-bwt-stream-trailing-bytes',
                  p3_block(b'AAAA', 3, b'AAAA' + b'\x00')))

    # --- transformed-size mismatch around inverse BWT ---
    # Build a file whose envelope transformed_size disagrees with the postcoder payload
    # length requirement. Use postcoder 3 expecting 4 but declare xlen=5.
    sub = bytes([0]) + put_uvar(4) + b'AAAA'
    pl = bwt_payload(3, 3, sub)
    cases.append(('transformed-size-too-large',
                  build_rev2(b'AAAA', pl, transformed_size=5)))
    # transformed_size too small vs declared primary/streams (xlen=3 < 4 needed)
    cases.append(('transformed-size-too-small',
                  build_rev2(b'AAAA', pl, transformed_size=3)))

    # --- degenerate 1-byte and all-equal cases (roundtrip, not reject) ---
    # These are genuine encode->decode round-trips through backend 2 and must be
    # byte-exact. The encoder may emit raw mode-0 for tiny inputs, so we verify the
    # round-trip regardless of which backend was chosen (BWT or fallback) -- the
    # point is that NO degenerate input crashes or corrupts.
    degenerate_inputs = {
        'degenerate-1-byte-roundtrip': b'A',
        'degenerate-all-equal-roundtrip': b'AAAA' * 4,
    }
    for label, din in degenerate_inputs.items():
        ds = td / f'{label}.bin'; dp = td / f'{label}.anv'; dout = td / f'{label}.out'
        ds.write_bytes(din)
        run([exe, 'c', ds, dp, '--parse=ratio', '--ratio-backend=bwt',
             '--ratio-context=off', '--ratio-lines=off', '--quiet'])
        run([exe, 'd', dp, dout, '--quiet'])
        if dout.read_bytes() != din:
            raise RuntimeError(f'{label} did not round-trip exactly')

    for label, mut in cases:
        bad.write_bytes(mut)
        require_reject_bwt(exe, bad, dec, label)
    return len(cases), skipped


def aux_bwt_roundtrip(exe: Path, td: Path) -> int:
    """I10-1A direct coverage for additive auxiliary-index BWT framing."""
    n=0
    data=(b'I10 auxiliary inverse BWT must stay causal and byte-exact. ' * 2200)
    src=td/'aux-source.bin'; src.write_bytes(data)
    dec=td/'aux.out'

    # CLI contract is strict: typos must never silently enable a new wire format.
    bad_cli=run([exe,'c',src,td/'aux-bad-cli.anv','--parse=ratio',
                 '--ratio-backend=bwt','--bwt-aux=banana','--quiet'],ok=False)
    if bad_cli.returncode==0 or b'--bwt-aux must be on or off' not in bad_cli.stderr:
        raise RuntimeError('invalid --bwt-aux value was not rejected explicitly')
    n+=1

    # Default-off and explicit-off must be literally identical.
    default_p=td/'aux-default.anv'
    off_p=td/'aux-off.anv'
    common=['--parse=ratio','--ratio-backend=bwt','--ratio-context=off',
            '--ratio-lines=off','--bwt-post=2','--quiet']
    run([exe,'c',src,default_p,*common])
    run([exe,'c',src,off_p,*common,'--bwt-aux=off'])
    if default_p.read_bytes()!=off_p.read_bytes():
        raise RuntimeError('--bwt-aux default changed legacy BWT bytes')
    n+=1

    # Compare the only changed representation component: the v2 auxiliary
    # header/indexes. BWT/postcoder data itself must remain identical.
    on_p=td/'aux-on.anv'
    run([exe,'c',src,on_p,*common,'--bwt-aux=on'])
    legacy_blob=off_p.read_bytes(); aux_blob=on_p.read_bytes()
    legacy=bwt_inner_layout(legacy_blob); aux=bwt_inner_layout(aux_blob)
    if legacy['aux'] or not aux['aux']:
        raise RuntimeError('BWT auxiliary framing detection mismatch')
    if legacy['post']!=2 or aux['post']!=2:
        raise RuntimeError('forced postcoder 2 changed under auxiliary framing')
    if not aux['indexes'] or aux['indexes'][0]!=legacy['primary']:
        raise RuntimeError('auxiliary I[0] does not match legacy primary index')
    if legacy_blob[legacy['postdata_off']:] != aux_blob[aux['postdata_off']:]:
        raise RuntimeError('libsais_bwt_aux changed BWT/postcoder payload bytes')
    run([exe,'d',on_p,dec,'--quiet'])
    if dec.read_bytes()!=data:
        raise RuntimeError('auxiliary BWT roundtrip mismatch')
    n+=1

    # Every currently-supported decoder-visible postcoder must work through v2.
    for pid in (1,2,3):
        packed=td/f'aux-post{pid}.anv'
        run([exe,'c',src,packed,'--parse=ratio','--ratio-backend=bwt',
             '--ratio-context=off','--ratio-lines=off',f'--bwt-post={pid}',
             '--bwt-aux=on','--quiet'])
        lay=bwt_inner_layout(packed.read_bytes())
        if not lay['aux'] or lay['post']!=pid:
            raise RuntimeError(f'auxiliary forced postcoder {pid} wire mismatch')
        run([exe,'d',packed,dec,'--quiet'])
        if dec.read_bytes()!=data:
            raise RuntimeError(f'auxiliary forced postcoder {pid} roundtrip mismatch')
        n+=1

    # Tiny/policy-boundary inputs exercise r selection and degenerate handling.
    for size in (1,2,3,17,1023,1024,1025,4097):
        tiny=td/f'aux-n{size}.bin'
        tiny_data=bytes((i*29+11)&255 for i in range(size))
        tiny.write_bytes(tiny_data)
        packed=td/f'aux-n{size}.anv'
        run([exe,'c',tiny,packed,'--parse=ratio','--ratio-backend=bwt',
             '--ratio-context=off','--ratio-lines=off','--bwt-aux=on','--quiet'])
        run([exe,'d',packed,dec,'--quiet'])
        if dec.read_bytes()!=tiny_data:
            raise RuntimeError(f'auxiliary BWT size={size} roundtrip mismatch')
        n+=1

    # Outer 0xFF subblock framing must compose with inner 0xFE auxiliary payloads.
    sub_data=(b'aux-subblock-composition-0123456789' * 9000)[:262144]
    sub_src=td/'aux-subblock.bin'; sub_src.write_bytes(sub_data)
    sub_p=td/'aux-subblock.anv'
    run([exe,'c',sub_src,sub_p,'--parse=ratio','--ratio-backend=bwt',
         '--ratio-context=off','--ratio-lines=off','--bwt-post=2',
         '--bwt-aux=on','--bwt-subblock=32768','--quiet'])
    sub_blob=sub_p.read_bytes(); sl=ratio_layout(sub_blob)
    p=sl['backend_payload_off']; e=len(sub_blob)
    if p>=e or sub_blob[p]!=0xff:
        raise RuntimeError('auxiliary subblock test did not emit outer 0xFF frame')
    p+=1; nsub,p=read_uvar(sub_blob,p)
    if nsub<2:
        raise RuntimeError('auxiliary subblock test emitted fewer than two subblocks')
    decoded_sum=0
    for _ in range(nsub):
        dlen,p=read_uvar(sub_blob,p); plen,p=read_uvar(sub_blob,p)
        if plen<=0 or p+plen>e:
            raise RuntimeError('invalid BWT subblock payload length')
        if sub_blob[p]!=0xfe:
            raise RuntimeError('BWT subblock did not contain inner auxiliary frame')
        decoded_sum+=dlen; p+=plen
    if p!=e or decoded_sum!=len(sub_data):
        raise RuntimeError('auxiliary subblock framing accounting mismatch')
    run([exe,'d',sub_p,dec,'--quiet'])
    if dec.read_bytes()!=sub_data:
        raise RuntimeError('auxiliary BWT subblock roundtrip mismatch')
    n+=1

    # Malformed v2 headers/indexes must fail before inverse reconstruction.
    blob=aux_blob; lay=aux
    cases=[]
    m=bytearray(blob); m[lay['tag_off']+1]=0xfd
    cases.append(('aux-unknown-postcoder',bytes(m)))
    cases.append(('aux-nonpow2-rate',
                  replace_uvar_same_width(blob,lay['rate_off'],lay['rate_end'],lay['rate']+1)))
    cases.append(('aux-wrong-index-count',
                  replace_uvar_same_width(blob,lay['count_off'],lay['count_end'],lay['count']+1)))
    m=bytearray(blob); struct.pack_into('<I',m,lay['index_off'],0)
    cases.append(('aux-zero-index',bytes(m)))
    m=bytearray(blob); struct.pack_into('<I',m,lay['index_off'],len(data)+1)
    cases.append(('aux-index-over-output',bytes(m)))
    bad=td/'aux-bad.anv'
    for label,mut in cases:
        bad.write_bytes(mut)
        require_reject_bwt(exe,bad,dec,label)
        n+=1

    return n


def forced_postcoder_roundtrip(exe: Path, td: Path) -> int:
    """Jackson's registry-coverage rule, applied to backend-2 postcoders.

    Every decoder-visible postcoder ID (0..4) gets a DIRECT forced
    encode->decode test, not just whatever the smallest-candidate selector
    picks. Selection-based tests are insufficient: a broken candidate rots
    silently when the selector never chooses it (this is exactly how postcoder
    0 and 4 went unexercised until forced).

    - IDs 1/2/3 are supported: must encode with --bwt-post=N and round-trip
      byte-exact. Their sizes+sha256 are recorded as goldens.
    - IDs 0/4 are disabled (known encoder/decoder mismatch): the encoder must
      REJECT up front with a clear message (rc != 0, "not supported yet"),
      never ship a corrupt or crashing decode. A postcoder that errors cleanly
      is acceptable; one that emits undecodable output is not.

    Returns the number of forced assertions made. Raises on any failure.
    """
    import csv
    inputs = {
        'tiny': b'A',
        'all-equal': b'ABCD' * 5000,           # forces QLFC zero-runs (the failing case)
        'text': (b'Measure what matters; reject corruption; roundtrip exactly. ' * 2000),
        'random': bytes([(i * 31 + 7) & 0xFF for i in range(20000)]),
    }
    supported = {1, 2, 3}
    disabled = {0, 4}
    golden_dir = Path(__file__).resolve().parent / 'bwt-golden'
    golden_dir.mkdir(exist_ok=True)
    rows = []
    n = 0
    for pid in range(5):
        for name, data in inputs.items():
            ds = td / f'fp-{pid}-{name}.bin'
            dp = td / f'fp-{pid}-{name}.anv'
            dout = td / f'fp-{pid}-{name}.out'
            ds.write_bytes(data)
            rc = run([exe, 'c', ds, dp, '--parse=ratio', '--ratio-backend=bwt',
                      '--ratio-context=off', '--ratio-lines=off',
                      f'--bwt-post={pid}', '--quiet'], ok=False)
            n += 1
            if pid in supported:
                if rc.returncode != 0:
                    raise RuntimeError(
                        f'forced postcoder {pid} ({name}) failed to encode: '
                        f'{rc.stderr.decode(errors="replace").strip()[:80]}')
                rd = run([exe, 'd', dp, dout, '--quiet'], ok=False)
                if rd.returncode != 0:
                    raise RuntimeError(
                        f'forced postcoder {pid} ({name}) failed to decode: '
                        f'{rd.stderr.decode(errors="replace").strip()[:80]}')
                if dout.read_bytes() != data:
                    raise RuntimeError(f'forced postcoder {pid} ({name}) round-trip mismatch')
                comp = dp.read_bytes()
                pc = emitted_postcoder(comp)
                if pc is not None and pc != pid:
                    raise RuntimeError(
                        f'forced postcoder {pid} ({name}) emitted postcoder id '
                        f'{pc} on the wire (force flag ignored)')
                rows.append((f'post{pid}', name, len(data), len(comp),
                             hashlib.sha256(data).hexdigest(),
                             hashlib.sha256(comp).hexdigest()))
            elif pid in disabled:
                # Must never ship undecodable output. Two acceptable outcomes:
                #  - clean up-front rejection ("not supported yet") for inputs > 1 byte;
                #  - a byte-exact round-trip for the 1-byte degenerate short-circuit
                #    (encoder emits a raw-stream payload before the rejection loop).
                if rc.returncode == 0:
                    rd = run([exe, 'd', dp, dout, '--quiet'], ok=False)
                    if rd.returncode != 0 or dout.read_bytes() != data:
                        raise RuntimeError(
                            f'forced postcoder {pid} ({name}) was ENCODED but did not '
                            f'round-trip -- must reject up front or decode exactly')
                    pc = emitted_postcoder(dp.read_bytes())
                    if pc is not None and pc != pid:
                        raise RuntimeError(
                            f'forced postcoder {pid} ({name}) emitted postcoder id '
                            f'{pc} on the wire (force flag ignored)')
                else:
                    if 'not supported yet' not in rc.stderr.decode(errors='replace'):
                        raise RuntimeError(
                            f'forced postcoder {pid} ({name}) rejected without the expected '
                            f'message: {rc.stderr.decode(errors="replace").strip()[:80]}')
            else:
                raise RuntimeError(f'unknown postcoder id in test: {pid}')
    with open(golden_dir / 'bwt-postcoders.csv', 'w', newline='') as fh:
        w = csv.writer(fh)
        w.writerow(['postcoder', 'input', 'input_size', 'compressed_size',
                    'input_sha256', 'compressed_sha256'])
        for r in rows:
            w.writerow(r)
    return n


def golden_bwt(exe: Path, td: Path) -> int:
    """Generate golden backend-2 files for tiny/repetitive/random/text inputs.

    Roundtrips each input through backend 2, writes the compressed `.anv` artifacts
    into `tests/bwt-golden/` (git-ignored), and records sizes + sha256 hashes into
    `tests/bwt-golden.csv`. Returns the number of golden entries written.
    """
    import csv
    golden_dir = Path(__file__).resolve().parent / 'bwt-golden'
    golden_dir.mkdir(exist_ok=True)
    inputs = {
        'tiny': b'ANVIL',
        'repetitive': b'ABCD' * 5000,
        'random': bytes([(i * 31 + 7) & 0xFF for i in range(20000)]),
        'text': (b'Measure what matters; reject corruption; roundtrip exactly. ' * 1500),
    }
    out_csv = golden_dir / 'bwt-golden.csv'
    rows = []
    for name, data in inputs.items():
        src = golden_dir / f'g-{name}.bin'
        src.write_bytes(data)
        packed = golden_dir / f'g-{name}.anv'
        dec = golden_dir / f'g-{name}.out'
        run([exe, 'c', src, packed, '--parse=ratio', '--ratio-backend=bwt',
             '--ratio-context=off', '--ratio-lines=off', '--quiet'])
        run([exe, 'd', packed, dec, '--quiet'])
        got = dec.read_bytes()
        if hashlib.sha256(got).digest() != hashlib.sha256(data).digest():
            raise RuntimeError(f'golden BWT roundtrip failed: {name}')
        comp = packed.read_bytes()
        rows.append((name, len(data), len(comp),
                     hashlib.sha256(data).hexdigest(),
                     hashlib.sha256(comp).hexdigest()))
    with open(out_csv, 'w', newline='') as fh:
        w = csv.writer(fh)
        w.writerow(['name', 'input_size', 'compressed_size',
                    'input_sha256', 'compressed_sha256'])
        for r in rows:
            w.writerow(r)
    return len(rows)


def iter_blocks(blob: bytes):
    """Yield (blen, mode, plen, payload_off) for every block of a valid ANVIL file."""
    if len(blob) < 5 or blob[:4] != b'ANV0':
        raise ValueError('not anv0')
    p = 5
    _, p = read_uvar(blob, p)          # block_size
    total, p = read_uvar(blob, p)
    seen = 0
    while seen < total:
        blen, p = read_uvar(blob, p)
        if p >= len(blob):
            raise ValueError('truncated block mode')
        mode = blob[p]; p += 1
        plen, p = read_uvar(blob, p)
        if p + 4 > len(blob):
            raise ValueError('truncated crc')
        p += 4
        yield blen, mode, plen, p
        p += plen
        seen += blen


def emitted_postcoder(blob: bytes):
    """Return the backend-2 postcoder id of a single mode-17 block, or None.

    Returns None when the file is not a single mode-17 backend-2 block (e.g. a raw
    short-circuit for degenerate input), so a forced-postcoder test can assert the
    emitted id when the backend-2 path is actually taken while still accepting a
    byte-exact raw round-trip on the 1-byte degenerate case.
    """
    if len(blob) < 5 or blob[:4] != b'ANV0' or blob[4] != 2:
        return None
    blocks = list(iter_blocks(blob))
    if len(blocks) != 1 or blocks[0][1] != 17:
        return None
    _, _, plen, off = blocks[0]
    if plen < 3 or blob[off + 1] != 2:
        return None
    _, q = read_uvar(blob, off + 2)    # skip transform, backend, transformed_size
    return blob[q] if q < off + plen else None


def registry_coverage_roundtrip(exe: Path, td: Path) -> dict:
    """Direct forced encode->decode coverage for every decoder-visible registry ID.

    Jackson's registry-coverage rule: selection-based tests are insufficient -- a
    broken candidate rots silently when the selector never chooses it (this is how
    BWT postcoders 0/4 stayed broken). Every block mode and every rev-2 transform /
    backend id gets a DIRECT forced encode->decode here, and the test asserts the
    EMITTED wire id, so a force flag that is silently ignored fails instead of
    passing. Stream-suite codec ids are covered by stream_suite_coverage (they have
    no per-codec force flag). Returns a dict of family -> forced-assertion count.
    """
    text = (b'the quick brown fox jumps over the lazy dog\n') * 2000
    lines = b''.join(f'{i:06d},colA,colB,{i*7%1000}\n'.encode() for i in range(4000))
    ari16 = bytes((i * 31 + 7) & 0xFF for i in range(65536))
    # Deterministic high-entropy input: sha256 chain is stable across runs and
    # incompressible to ANVIL, so the encoder takes its raw fallback (mode 0).
    inc = b''.join(hashlib.sha256(i.to_bytes(4, 'little')).digest() for i in range(4096))
    inputs = {'text': text, 'lines': lines, 'ari16': ari16, 'inc': inc}
    for name, data in inputs.items():
        (td / f'reg-{name}.bin').write_bytes(data)

    def encdec(name: str, flags: list[str]) -> bytes:
        src = td / f'reg-{name}.bin'
        packed = td / 'reg.anv'; dec = td / 'reg.out'
        run([exe, 'c', src, packed, '--quiet', *flags])
        run([exe, 'd', packed, dec, '--quiet'])
        if dec.read_bytes() != src.read_bytes():
            raise RuntimeError(f'registry coverage round-trip mismatch: {name} {flags}')
        return packed.read_bytes()

    block_modes = [
        ('inc',   ['--parse=greedy', '--entropy=rans', '--literal=o0'], 0),
        ('text',  ['--parse=greedy', '--entropy=arith', '--literal=o0'], 1),
        ('text',  ['--parse=greedy', '--entropy=arith', '--literal=o1'], 2),
        ('text',  ['--parse=greedy', '--entropy=arith', '--literal=g4'], 3),
        ('text',  ['--parse=greedy', '--entropy=arith', '--literal=g8'], 4),
        ('text',  ['--parse=greedy', '--entropy=arith', '--literal=g16'], 5),
        ('text',  ['--parse=greedy', '--entropy=rans', '--literal=o0'], 10),
        ('text',  ['--parse=sparse', '--entropy=rans'], 11),
        ('text',  ['--parse=shape', '--entropy=rans'], 12),
        ('text',  ['--parse=topology', '--entropy=rans'], 13),
        ('text',  ['--parse=tcopy', '--entropy=rans'], 14),
        ('text',  ['--parse=hotop', '--entropy=rans'], 15),
        ('ari16', ['--parse=greedy', '--entropy=rans', '--ariref=on'], 16),
        ('text',  ['--parse=ratio'], 17),
    ]
    for name, flags, want in block_modes:
        blob = encdec(name, flags)
        got = [m for _, m, _, _ in iter_blocks(blob)]
        if want not in got:
            raise RuntimeError(
                f'registry coverage: {flags} emitted block modes {got}, expected {want} '
                f'(force flag no longer selects this mode)')

    transforms = [
        # name, flags, want_transform, want_backend, allow_reject
        ('text',  ['--parse=ratio', '--ratio-context=off', '--ratio-lines=off',
                   '--ratio-backend=brotli'], 0, 1, False),
        ('lines', ['--parse=ratio', '--ratio-context=on', '--ratio-lines=off',
                   '--ratio-backend=bwt'], 1, 2, False),
        ('lines', ['--parse=ratio', '--ratio-context=off', '--ratio-lines=on',
                   '--ratio-backend=brotli'], 2, 1, False),
        # Transform 2 with backend 2 is a stable-registry cross-combination: the
        # encoder can emit it, so it must round-trip (or be cleanly refused). No
        # emitted-id assertion here -- avoiding the combination is a valid fix.
        ('lines', ['--parse=ratio', '--ratio-context=off', '--ratio-lines=on',
                   '--ratio-backend=bwt'], None, None, True),
        # Transform 3 is experimental (--bwt-lzp=on, default OFF). It must either
        # emit a genuine transform-3 block that round-trips, or be cleanly
        # rejected -- never emit an undecodable transform-3 block.
        ('lines', ['--parse=ratio', '--bwt-lzp=on', '--ratio-context=off',
                   '--ratio-lines=off', '--ratio-backend=bwt'], 3, 2, True),
    ]
    for name, flags, want_t, want_b, allow_reject in transforms:
        src = td / f'reg-{name}.bin'
        packed = td / 'regx.anv'; dec = td / 'regx.out'
        rc = run([exe, 'c', src, packed, '--quiet', *flags], ok=False)
        if rc.returncode != 0:
            err = rc.stderr.decode(errors='replace')
            if allow_reject and any(k in err.lower() for k in
                                    ('not supported', 'unsupported', 'disabled', 'not yet')):
                continue
            raise RuntimeError(
                f'registry coverage: {flags} failed to encode: {err.strip()[:80]}')
        run([exe, 'd', packed, dec, '--quiet'])
        if dec.read_bytes() != src.read_bytes():
            raise RuntimeError(f'registry coverage round-trip mismatch: {flags}')
        if want_t is not None:
            blob = packed.read_bytes()
            bl = list(iter_blocks(blob))
            if len(bl) != 1 or bl[0][1] != 17:
                raise RuntimeError(
                    f'registry coverage: {flags} expected one mode-17 block, got '
                    f'{[m for _, m, _, _ in bl]}')
            _, _, plen, off = bl[0]
            payload = blob[off:off + plen]
            got_t, got_b = payload[0], payload[1]
            if (got_t, got_b) != (want_t, want_b):
                raise RuntimeError(
                    f'registry coverage: {flags} emitted transform/backend '
                    f'({got_t},{got_b}), expected ({want_t},{want_b})')

    return {'block_modes': len(block_modes), 'transforms': len(transforms)}


def stream_suite_coverage(exe: Path, td: Path) -> tuple[set, set]:
    """Exercise the stream-codec registry (suite mode bytes 0..8) end to end.

    The encoder exposes NO per-codec force flag: each substream codec is chosen by
    the J = L + lambda*C objective. So unlike --bwt-post / --parse=, this namespace
    cannot be forced directly. We drive a deterministic matrix, assert that the
    codecs the encoder CAN emit actually appear on the wire (so a codec that stops
    being selected fails the gate loudly instead of rotting behind the selector),
    and report the ids still unreachable on this matrix. Returns (observed, missing)
    over suite ids 0..8. ids 4 (Huffman) and 7 (RePair) are not selected here and
    are documented as selection-only in FORMAT.md.
    """
    data = b''.join(f'{i:06d},colA,colB,{i*7%1000}\n'.encode() for i in range(4000))
    (td / 'suite.bin').write_bytes(data)

    def scan(payload: bytes, n: int, skip: int) -> set:
        p = skip; out: set[int] = set()
        for _ in range(n):
            zn, p = read_uvar(payload, p)
            if zn == 0:
                continue
            if p >= len(payload):
                raise RuntimeError('stream-suite scan underflow')
            out.add(payload[p]); p += zn
        return out

    def hotop_scan(payload: bytes) -> set:
        p = 1                                  # num_states byte
        K, p = read_uvar(payload, p)
        for _ in range(K):
            p += 1                             # kind
            _, p = read_uvar(payload, p)       # len
            p += 1                             # shape
        return scan(payload, 9, p)

    matrix = [
        (['--parse=greedy', '--entropy=rans'], 10, 5, 0, None),
        (['--parse=sparse', '--entropy=rans'], 11, 7, 0, None),
        (['--parse=shape', '--entropy=rans'], 12, 8, 1, None),
        (['--parse=tcopy', '--entropy=rans'], 14, 8, 0, None),
        (['--parse=hotop', '--entropy=rans'], 15, 9, 0, 'hotop'),
        (['--parse=hotop', '--entropy=rans', '--hotop-rlzp=on'], 15, 9, 0, 'hotop'),
    ]
    observed: set[int] = set()
    packed = td / 'suite.anv'; dec = td / 'suite.out'
    for flags, want_mode, nstream, skip, kind in matrix:
        run([exe, 'c', td / 'suite.bin', packed, '--quiet', *flags])
        run([exe, 'd', packed, dec, '--quiet'])
        if dec.read_bytes() != data:
            raise RuntimeError(f'stream-suite round-trip mismatch: {flags}')
        blob = packed.read_bytes()
        target = [(off, plen) for _, m, plen, off in iter_blocks(blob) if m == want_mode]
        if not target:
            raise RuntimeError(f'stream-suite coverage: {flags} did not emit block mode {want_mode}')
        for off, plen in target:
            payload = blob[off:off + plen]
            observed |= hotop_scan(payload) if kind == 'hotop' else scan(payload, nstream, skip)
    return observed, set(range(9)) - observed


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
    combos=[('greedy','arith',[]),('dp','arith',[]),('greedy','rans',[]),('dp','rans',[]),('sparse','rans',[]),
            ('tcopy','rans',[]),('tcopy','rans',['--pnra=on']),('ratio','rans',[])]
    total=0; mutated=0; deterministic=0
    with tempfile.TemporaryDirectory(prefix='anvil-fuzz-') as td:
        td=Path(td)
        deterministic=adversarial_rev2(exe,td)
        deterministic_bwt, _ = adversarial_bwt(exe, td)
        aux_bwt = aux_bwt_roundtrip(exe, td)
        golden = golden_bwt(exe, td)
        # Jackson's registry-coverage rule: every decoder-visible postcoder ID
        # (0..4) gets a DIRECT forced encode->decode test, not just the selector's
        # smallest-candidate pick. Selection-based coverage let postcoders 0/4 rot.
        forced_post = forced_postcoder_roundtrip(exe, td)
        reg = registry_coverage_roundtrip(exe, td)
        suite_observed, suite_missing = stream_suite_coverage(exe, td)
        for idx,data in enumerate(patterns(rng,args.cases)):
            src=td/'in.bin'; src.write_bytes(data)
            for parse,entropy,extra in combos:
                packed=td/'x.anv'; dec=td/'out.bin'
                run([exe,'c',src,packed,f'--parse={parse}','--literal=o0',f'--entropy={entropy}','--quiet',*extra])
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
    print(f'PASS seed={args.seed} roundtrip_variants={total} mutations={mutated} '
          f'deterministic_rev2={deterministic} deterministic_bwt={deterministic_bwt} '
          f'aux_bwt={aux_bwt} golden_bwt={golden} forced_postcoders={forced_post} '
          f'registry_block_modes={reg["block_modes"]} registry_transforms={reg["transforms"]} '
          f'suite_modes={sorted(suite_observed)} suite_unforceable={sorted(suite_missing)}')

if __name__=='__main__': main()
