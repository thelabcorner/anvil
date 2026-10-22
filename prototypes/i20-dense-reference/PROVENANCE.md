# I20 dense-reference reference producers -- provenance

**Owner:** I20 worker W6 (candidate C6, dense reference-grid control).
**Purpose:** supply the *dense* reference producers for the I20 gate-closure
control.  These are **reference** producers, not ANVIL mechanisms: nothing in
this directory touches src/anvil.cpp, the ANVIL wire format, or any codec
decision.  Class: adopt-class / engineering, control measurement.

## brotli_dense.cpp

A parameterised Brotli producer/consumer.  It exists because the repo's
binding helper tools/brotli_lw.cpp is hard-wired to quality 11 / window 30
and therefore cannot express the Brotli *window* and *quality* axes that the
C6 hypothesis requires.

The helper deliberately calls BrotliEncoderCompress() -- the same one-shot API
that tools/brotli_lw.cpp calls -- so that at (quality=11, lgwin=30) it is
**byte-identical** to the gate's brotli_lw arm when linked against the same
libbrotli build.  That identity is measured in-job (arms
brotli-q11-lw30-gate versus brotli-q11-lw30-distro) rather than asserted.

The decode path mirrors tools/brotli_lw.cpp exactly, including
BROTLI_DECODER_PARAM_LARGE_WINDOW, so decode time and decode peak RSS are
measured with the same geometry as the gate's arm.

**Window semantics.**  lgwin 30 is a real large-window stream, not a clamped
24.  In brotli v1.1.0 (c/enc/encode.c, pinned commit
ed738e842d2fbdf2d6459e39267a633c4a9b2f5d) BrotliEncoderCompress() does:

    BrotliEncoderSetParameter(s, BROTLI_PARAM_LGWIN, (uint32_t)lgwin);
    ...
    if (lgwin > BROTLI_MAX_WINDOW_BITS) {
      BrotliEncoderSetParameter(s, BROTLI_PARAM_LARGE_WINDOW, BROTLI_TRUE);
    }

so the gate's "Brotli q11/lgwin30" arm is a genuine lw30 arm.  The dense grid
additionally measures lw22 and lw24 so that the *window axis* is populated
rather than assumed.

**Producers used (both recorded in the run artifact):**

| build | source | used for |
|---|---|---|
| pinned | google/brotli at ed738e842d2fbdf2d6459e39267a633c4a9b2f5d (v1.1.0), built static with -DBUILD_SHARED_LIBS=OFF | the dense Brotli rows |
| distribution | libbrotli-dev from the runner image (exact version string recorded) | the binding gate arm plus the helper-equivalence control |

If the pinned build cannot be produced on the runner, the dense rows fall back
to the distribution build and the artifact records that as a **disclosed
substitution** (never a silent one).

## Other reference producers

xz, zstd and lz4 are taken from the runner image.  Their exact version strings
are captured in-job with --version and recorded in prepare-summary.json under
producers, and in the run's provenance.txt.  A pinned xz 5.4.5 source build
(the version used by the baseline run 35927623136) is additionally built so
that the *producer version delta* between the baseline run and the dense grid
is measured rather than assumed.

## What this directory must never be used for

It must not be used to produce a novelty claim, a crossing token, or a
measurement that is presented as an ANVIL result.  Every number it produces
is a **reference** number, and the C6 deliverable is a **control** measurement.
