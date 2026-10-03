#!/usr/bin/env python3
"""Q2 arm registry: 5 retained FRONT-GAP configurations x a 2x2 factorial, + references.

Isolated prototype. Not wired to production. Not dispatchable.

Design authority: docs/swarm-2026-10-02/Q2-PARITY-PLAN-SPACE-BUNNY.md (Rev 3).

Coordinator constraints encoded here:
  * All FIVE retained FRONT-GAP configurations are preserved with their EXACT original
    bench settings. Nothing is collapsed onto one stream-lambda. src/anvil.cpp is not
    read for the configuration; tools/bench_native.cpp is the authority.
  * NO materiality threshold and NO epsilon exists anywhere in this file or in
    q2_factorial.py. Contrasts are published as exact signed byte deltas plus relative
    percentages against a declared denominator.
  * Timing, parallelism and the reference codecs are DESCRIPTIVE / NON-CLASSIFYING.

Candidate design -- a COMPLETE, CROSSED 2x2 in two binary factors,
{geometry: small|large} x {incompressibility gate: on|off}, run independently within EACH of
the five retained configurations:

    G0 = small / on      G1 = large / on
    G3 = small / off     G2 = large / off

Because the design is complete and crossed, EVERY main effect holds the other factor fixed
exactly. No contrast is confounded.

Contrast algebra (frozen here, verified in code):

    geometry effect, gate held ON   = G1 - G0
    geometry effect, gate held OFF  = G2 - G3
    gate effect, geometry held SMALL = G0 - G3   # G0 and G3 share block size AND block count
    gate effect, geometry held LARGE = G1 - G2   # G1 and G2 share block size AND block count
    interaction = (G1 - G2) - (G0 - G3) == (G1 - G0) - (G2 - G3)
        # tests whether the GEOMETRY EFFECT DEPENDS ON GATE STATE

Naming discipline (binding). The two gate contrasts are GATE effects: the factor they vary is
the incompressibility router, not block geometry. They are never window effects, geometry
effects, or geometry controls.

Rejected claim, recorded so it cannot be reintroduced: any assertion that G0 - G3 is a "pure
window effect" (or any window / reach / geometry effect) is REJECTED on the design's face --
G0 and G3 have identical block size, so that contrast does not vary geometry at all. Equally
rejected is the converse error that only the interaction holds a factor fixed: G1 - G0 and
G2 - G3 each already hold the gate fixed.
"""
from __future__ import annotations

import argparse
import csv
import json
import math
import os
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, Mapping

# ---------------------------------------------------------------------------
# Frozen constants, each with its source.
# ---------------------------------------------------------------------------

SMALL_BLOCK_BYTES = 262144        # src/anvil.cpp:3769 default; o.block_size is never
                                  # assigned by tools/bench_native.cpp:24-30
LARGE_BLOCK_BYTES = 67108864      # src/anvil.cpp:5002 rev-1 cap; a single block for
                                  # every Class-A file (all <= 2,815,267 B)
REV1_BLOCK_CAP_BYTES = 67108864   # src/anvil.cpp:5002
REV2_BLOCK_CAP_BYTES = 134217728  # src/anvil.cpp:4999-5000 -- EXCLUDED (B1)

ANVIL_BLOCK_FRAMING_BYTES = 11   # FORMAT.md:25-33. DESCRIPTIVE ANNOTATION ONLY.
                                  # Never used as a decision floor or threshold.
KBTW_AUX_TARGET_WALKS = 1024     # src/anvil.cpp:3913
AUX_RATE_MIN = 2                 # src/anvil.cpp:4252

PINNED_DECODE_THREADS = 1        # src/anvil.cpp:3796 default, pinned explicitly

# src/anvil.cpp:4959 -- if(getenv("ANVIL_STREAM_LAMBDA")) opt.stream_lambda=std::atof(env);
# This executes BEFORE the --stream-lambda= parse at src/anvil.cpp:4977, so an explicit
# flag wins. That precedence is a source reading, not a guarantee: arm identity must be
# argv-determined, so Q2 ASSERTS the variable is unset instead of relying on ordering.
# src/anvil.cpp:4662 then propagates opt.stream_lambda into the g_stream_lambda global
# (src/anvil.cpp:1471), which is what actually weights the stream suite.
STREAM_LAMBDA_ENV = "ANVIL_STREAM_LAMBDA"
ENV_VARS_MUST_BE_UNSET: tuple[str, ...] = (STREAM_LAMBDA_ENV,)

# The retained harness baseline: tools/bench_native.cpp:24 declares the default parameter
# stream_lambda=0.04 and assigns anvil::g_stream_lambda = stream_lambda at :30. It is NOT
# the CLI default (0.01, src/anvil.cpp:3783). Recorded explicitly so the baseline cannot be
# confused with the CLI default in either direction. The retained FRONT-GAP configurations
# span lambda 0.04, 0.00 and 0.01 -- all three are preserved, none is collapsed.
RETAINED_HARNESS_BASELINE_STREAM_LAMBDA = 0.04   # tools/bench_native.cpp:24,30
CLI_DEFAULT_STREAM_LAMBDA = 0.01                 # src/anvil.cpp:3783

# B1: rev-2 / BWT is excluded. The aux sampling rate exists only there, so excluding
# rev-2 pins the aux geometry dimension by construction (B5).
FORBIDDEN_ARG_SUBSTRINGS: tuple[str, ...] = (
    "--parse=ratio",
    "--ratio-backend",
    "--bwt-aux=on",
)
# "--bwt-aux=off" is REQUIRED (the pin), not forbidden.

EVIDENCE_ROLE = "discovery"       # queue edit E1; the Class-A corpus

# The two Class-A corpus slots that are this build's own outputs (not git-tracked).
# identity_role is recorded and never asserted against historical hashes (gate G-B).
BUILD_OUTPUT_SLOTS: tuple[str, ...] = ("anvil.exe", "anvil_bench.exe")

# ---- Q2 corpus population --------------------------------------------------
# The population is DERIVED MECHANICALLY from the frozen Class-A CSV, which contains exactly
# 13 files. tests/corpus/CHECKSUMS.txt has since grown to 24 entries and the public mirror
# intentionally omits the later PE/synth expansion, so loading all 24 would silently change
# the population and pool objects the retained grid never contained.
Q0_FROZEN_FILE_COUNT = 13
# Cross-check only. frozen_population() is the authority; this tuple must agree with it.
Q0_FROZEN_FILES: tuple[str, ...] = (
    "anvil.exe", "anvil_bench.exe", "doc.md", "generated.json", "generated.jsonl",
    "generated.log", "generated.repeat.jsonl", "generated.sqlite", "random.bin", "src.cpp",
    "synth-arith.bin", "synth-jitter.bin", "synth-timeseries.bin",
)

CANDIDATE_CELLS: tuple[str, ...] = ("G0", "G1", "G2", "G3")
REFERENCE_ARMS: tuple[str, ...] = ("R1", "R3")

DECISION_TOKENS: tuple[str, ...] = ("PARITY-GEOMETRY-REPORTED",)

CLASSIFYING_AXES: tuple[str, ...] = ("complete_bytes",)
NON_CLASSIFYING_AXES: tuple[str, ...] = (
    "encode_MBps", "decode_MBps", "peak_rss_encode_kib", "peak_rss_decode_kib",
    "segment_count", "max_parallel_decode_units", "binary_bytes",
)


# ---------------------------------------------------------------------------
# The five retained FRONT-GAP configurations, with EXACT original settings.
#
# Authority: tools/bench_native.cpp. Signature (line 24):
#   bench_anvil(src, name, parse, lit, entropy, reps,
#               shape_states=28, stream_lambda=0.04,
#               channels=false, pnra=false, hotop_rlzp=false, hotop_budget=false)
# and the five call sites (lines 70-74).
#
# These are CLI translations, not re-derivations. stream_lambda is set through
# anvil::g_stream_lambda at bench_native.cpp:30, whereas the CLI default is 0.01
# (src/anvil.cpp:3783, parsed at :4977 via std::stod) -- so each value is passed
# explicitly and none is collapsed.
# ---------------------------------------------------------------------------

@dataclass(frozen=True)
class CandidateConfig:
    config_id: str
    frozen_row: str              # row name in the frozen Class-A grid
    bench_line: int              # tools/bench_native.cpp call site
    parse: str
    literal: str
    entropy: str
    shape_states: int
    stream_lambda: float         # the exact value bench_native.cpp passed
    bench_defaults_pinned: tuple[tuple[str, str], ...]


_BD = (
    ("--channels", "off"),        # src/anvil.cpp:3779 default false; bench leaves unset
    ("--pnra", "off"),            # :3780 default false
    ("--hotop-rlzp", "off"),      # :3785 default false
    ("--hotop-budget", "off"),    # :3786 default false
    ("--boundary", "off"),        # :3777 default false
    ("--stream-suite", "on"),     # :3781 default true
    ("--chain", "48"),            # :3770 default 48
    ("--max-match", "65535"),     # :3771 default 65535
    ("--surprise", "12"),         # :3775 default 12
)

CANDIDATE_CONFIGS: tuple[CandidateConfig, ...] = (
    CandidateConfig("mdl-l0.04", "anvil-mdl-rans", 70,
                    "mdl", "o0", "rans", 28, 0.04, _BD),
    CandidateConfig("mdl-l0.00", "anvil-mdl-rans-l0", 71,
                    "mdl", "o0", "rans", 28, 0.00, _BD),
    CandidateConfig("mdl-l0.01", "anvil-mdl-rans-l001", 72,
                    "mdl", "o0", "rans", 28, 0.01, _BD),
    CandidateConfig("shape-l0.04", "anvil-shape-rans", 73,
                    "shape", "o0", "rans", 28, 0.04, _BD),
    CandidateConfig("shape-l0.00", "anvil-shape-rans-l0", 74,
                    "shape", "o0", "rans", 28, 0.00, _BD),
)

CONFIG_IDS: tuple[str, ...] = tuple(c.config_id for c in CANDIDATE_CONFIGS)

# The retained twin structure, read out of the frozen Class-A CSV over all 13 files:
# the three mdl configurations are byte-identical to each other on EVERY file, and the
# two shape configurations are byte-identical to each other on EVERY file. (On the two
# store-class files all five coincide.) This is a declared identity expectation used to
# measure "retained-twin fidelity" -- see q2_factorial.py::retained_twin_fidelity.
# It is RECORDED, not enforced: a divergence is an observation, not an invalidation.
RETAINED_TWIN_GROUPS: dict[str, tuple[str, ...]] = {
    "T-MDL": ("mdl-l0.04", "mdl-l0.00", "mdl-l0.01"),
    "T-SHAPE": ("shape-l0.04", "shape-l0.00"),
}
RETAINED_TWIN_BASIS = (
    "tests/benchmark-suite.frozen-bdc90474.plus-xz.csv, all 13 files: the three "
    "anvil-mdl-rans* rows carry identical compressed_bytes, and the two anvil-shape-rans* "
    "rows carry identical compressed_bytes. Exactly two distinct byte values per file."
)

# ---- Reference producer identity (fix 4) -----------------------------------
# HONESTY REQUIREMENT. The retained Class-A Brotli rows were produced by an UNVERSIONED
# libbrotli on the Windows host and -- crucially -- at BROTLI_DEFAULT_WINDOW, i.e. lgwin 22
# (tools/bench_native.cpp:39). R1 deliberately uses an EXPLICIT lgwin 30. Therefore R1 does
# NOT reproduce the frozen Brotli bar and MUST NOT be compared against the frozen grid.
#
# R1/R3 are a SAME-RUN MATCHED GEOMETRY-CONTROL PAIR: both produced by one pinned producer,
# in one job, so D_ref = SUMpayloads(R3) - bytes(R1) prices the generic 256 KiB
# fragmentation tax against a non-zero denominator. That is their entire role.
#
# The pinned producer identity is NOT supplied and cannot be established offline without
# network ambiguity. Rather than weaken the requirement, the reference half is placed on a
# HARD HOLD and its residual is withheld until the coordinator supplies the pin.
FROZEN_BROTLI_WINDOW = "BROTLI_DEFAULT_WINDOW (lgwin 22), tools/bench_native.cpp:39"
R1_EXPLICIT_LGWIN = 30
BROTLI_PRODUCER_PINNED = False
PRODUCER_HOLD = "HOLD-BROTLI-PRODUCER-UNPINNED"
PRODUCER_HOLD_REASON = (
    "The Brotli producer must be pinned end-to-end (version + commit/tag + a verified artefact "
    "identity) rather than taken from an unversioned libbrotli-dev package. No such pinned "
    "identity is available to this lane, and inventing one offline would be fabrication. The "
    "reference half is therefore HELD, not weakened: the geometry factorial contrasts depend on "
    "no reference arm and stand regardless."
)


def check_producer_pin() -> list[str]:
    if not BROTLI_PRODUCER_PINNED:
        return [f"{PRODUCER_HOLD}: {PRODUCER_HOLD_REASON}"]
    return []


# Geometry x gate cell definitions.
CELL_GEOMETRY: dict[str, tuple[int, str]] = {
    "G0": (SMALL_BLOCK_BYTES, "on"),
    "G1": (LARGE_BLOCK_BYTES, "on"),
    "G2": (LARGE_BLOCK_BYTES, "off"),
    "G3": (SMALL_BLOCK_BYTES, "off"),
}
CELL_DESCRIPTION: dict[str, str] = {
    "G0": "small geometry, incompressibility gate ON  (frozen Class-A geometry)",
    "G1": "large geometry, incompressibility gate ON",
    "G2": "large geometry, incompressibility gate OFF",
    "G3": "small geometry, incompressibility gate OFF",
}


# ---------------------------------------------------------------------------
# Contrast algebra -- frozen.
# ---------------------------------------------------------------------------

CONTRASTS: tuple[tuple[str, str, str, str], ...] = (
    ("geom_effect_gate_on", "G1", "G0",
     "geometry main effect with the incompressibility gate HELD FIXED at ON"),
    ("geom_effect_gate_off", "G2", "G3",
     "geometry main effect with the incompressibility gate HELD FIXED at OFF"),
    ("gate_effect_at_small", "G0", "G3",
     "incompressibility-gate main effect with geometry HELD FIXED at small (G0 and G3 "
     "share block size and therefore block count); explicitly NOT a window effect, NOT a "
     "geometry effect and NOT a geometry control"),
    ("gate_effect_at_large", "G1", "G2",
     "incompressibility-gate main effect with geometry HELD FIXED at large (G1 and G2 "
     "share block size and therefore block count); explicitly NOT a window effect, NOT a "
     "geometry effect and NOT a geometry control"),
)
INTERACTION_ID = "interaction"
INTERACTION_TERMS: tuple[str, ...] = ("(G1 - G2)", "- (G0 - G3)")
INTERACTION_IDENTITY: tuple[str, ...] = ("(G1 - G0)", "- (G2 - G3)")

# Declared denominator for every published relative percentage. No threshold.
PCT_DENOMINATOR = "bytes(G0) of the same configuration and file"


# ---------------------------------------------------------------------------
# Geometry derivations (mirror src/anvil.cpp)
# ---------------------------------------------------------------------------

def derive_aux_rate(n: int) -> int:
    """Mirror bwt_aux_rate_for_size (src/anvil.cpp:4247-4256).

    The aux sampling rate is a DETERMINISTIC FUNCTION of the outer block length:
    r = next_pow2(ceil(n / kBwtAuxTargetWalks)), r >= 2. It has no independent dial,
    and it exists only on the rev-2 BWT path. Excluding rev-2 (B1) therefore pins it.
    Retained so a future rev-2 arm is visibly a different geometry, not a relabelling.
    """
    if n <= 1:
        return 0
    target = (n + KBTW_AUX_TARGET_WALKS - 1) // KBTW_AUX_TARGET_WALKS
    rate = AUX_RATE_MIN
    while rate < target:
        rate <<= 1
    return rate


def block_count(n: int, block: int) -> int:
    """src/anvil.cpp:4672 -- blen = min(block_size, remaining); final block is short."""
    if n <= 0:
        return 0
    return max(1, math.ceil(n / block))


def structural_capacity(nblocks: int, decode_threads: int = PINNED_DECODE_THREADS,
                        layout: str = "independent-block stream") -> dict:
    """Block-parallel decode capacity, emitted SYMBOLICALLY.

    Formula: ``min(N, nblocks) / block_count``  (src/anvil.cpp:4908)

    The numerator is how many independently decodable blocks can run concurrently; the
    denominator is the structural block count. Emitting only the ``decode_threads=1``
    instantiation would collapse a structural property into a pinning artifact, so the symbolic
    form is authoritative and the instantiated value is reported beside it as a special case.

    DESCRIPTIVE ONLY. No timing is measured, implied, or permitted from this field.
    """
    return {
        "formula": "min(N, nblocks) / block_count",
        "N": f"decode-thread count (symbolic; pinned N = {decode_threads} for this run)",
        "nblocks": nblocks,
        "block_count": nblocks,
        "symbolic": f"min(N, nblocks={nblocks}) / block_count={nblocks}",
        "evaluated_at_pinned_N": f"min({decode_threads},{nblocks}) / {nblocks}"
                                 f" = {min(decode_threads, nblocks)}/{nblocks}",
        "layout": layout,
        "note": "the SYMBOLIC form is authoritative; the pinned-N evaluation is the "
                "decode_threads=1 special case and must not be read as the structural capacity "
                "of the geometry",
        "source": "src/anvil.cpp:4908 -- nthreads = min(decode_threads, segs.size())",
        "classifying": False,
        "timing_implied": False,
    }


def decode_units(segments: int, decode_threads: int = PINNED_DECODE_THREADS) -> int:
    """src/anvil.cpp:4908 -- nthreads = min(decode_threads, segs.size()).

    Descriptive / non-classifying: block geometry is coupled to decoder segment
    parallelism and that coupling is invisible to byte measurement.
    """
    return min(decode_threads, segments)


def sign_label(delta: int) -> str:
    """Threshold-free sign label for an already-published integer. Adds no threshold."""
    return "ZERO" if delta == 0 else ("NEGATIVE" if delta < 0 else "POSITIVE")


# ---------------------------------------------------------------------------
# Arm model
# ---------------------------------------------------------------------------

@dataclass(frozen=True)
class CandidateArm:
    config: CandidateConfig
    cell: str

    @property
    def cell_id(self) -> str:
        return f"{self.config.config_id}@{self.cell}"

    @property
    def block_bytes(self) -> int:
        return CELL_GEOMETRY[self.cell][0]

    @property
    def negate(self) -> str:
        return CELL_GEOMETRY[self.cell][1]

    def config_argv(self) -> list[str]:
        """The exact original bench settings, as CLI arguments."""
        argv = [
            f"--parse={self.config.parse}",
            f"--literal={self.config.literal}",
            f"--entropy={self.config.entropy}",
            f"--shape-states={self.config.shape_states}",
            f"--stream-lambda={self.config.stream_lambda:g}",
        ]
        argv += [f"{flag}={value}" for flag, value in self.config.bench_defaults_pinned]
        return argv

    def encode_argv(self, src: Path, out: Path) -> list[str]:
        return [
            "anvil", "c", str(src), str(out),
            *self.config_argv(),
            f"--block={self.block_bytes}",
            f"--negate={self.negate}",
            f"--decode-threads={PINNED_DECODE_THREADS}",
            "--bwt-aux=off",
        ]

    def decode_argv(self, src: Path, out: Path) -> list[str]:
        """Geometry and gate are read back from the container; threads and aux are
        decoder-side options and are pinned identically."""
        return [
            "anvil", "d", str(src), str(out),
            f"--decode-threads={PINNED_DECODE_THREADS}",
            "--bwt-aux=off",
        ]

    def geometry(self, source_bytes: int) -> dict:
        segs = block_count(source_bytes, self.block_bytes)
        return {
            "cell_id": self.cell_id,
            "config_id": self.config.config_id,
            "frozen_row": self.config.frozen_row,
            "bench_source": f"tools/bench_native.cpp:{self.config.bench_line}",
            "cell": self.cell,
            "cell_description": CELL_DESCRIPTION[self.cell],
            "geometry_label": "small" if self.block_bytes == SMALL_BLOCK_BYTES else "large",
            "outer_block_bytes": self.block_bytes,
            "negate_gate": self.negate,
            "source_bytes": source_bytes,
            "block_count": segs,
            "max_reach_bytes": min(source_bytes, self.block_bytes - 1),
            "framing_bytes_descriptive": ANVIL_BLOCK_FRAMING_BYTES * segs,
            "framing_basis": "11 B/block (FORMAT.md:25-33); DESCRIPTIVE ANNOTATION ONLY, "
                             "never a decision floor",
            "config_argv": self.config_argv(),
            "stream_lambda": self.config.stream_lambda,
            "shape_states": self.config.shape_states,
            "aux_sampling_rate": {
                "policy": "absent",
                "pinned": True,
                "basis": "rev-2/BWT-only path excluded (B1); --bwt-aux=off pinned on every arm",
                "aux_rate_if_bwt_path": derive_aux_rate(min(source_bytes, self.block_bytes)),
                "not_applicable": True,
            },
            "decode_threads": PINNED_DECODE_THREADS,
            "segment_count": segs,
            "structural_capacity": structural_capacity(segs),
            "parallelism_classifying": False,
        }


@dataclass(frozen=True)
class ReferenceArm:
    arm_id: str
    brotli_lgwin: int
    streams: int                 # 1 = whole file; -1 = ceil(n / block_bytes)
    block_bytes: int
    role: str

    def encode_argv(self, helper: str, src: Path, out: Path) -> list[str]:
        return [
            helper, "c", str(src), str(out),
            "--q=11",
            f"--lgwin={self.brotli_lgwin}",
            f"--streams={self.streams}",
            f"--block={self.block_bytes}",
        ]

    def decode_argv(self, helper: str, src: Path, out: Path, expected: int) -> list[str]:
        return [helper, "d", str(src), str(out), str(expected)]

    def streams_for(self, source_bytes: int) -> int:
        return block_count(source_bytes, self.block_bytes) if self.streams == -1 else self.streams

    def geometry(self, source_bytes: int) -> dict:
        streams = self.streams_for(source_bytes)
        return {
            "cell_id": self.arm_id,
            "config_id": None,
            "frozen_row": None,
            "cell": self.arm_id,
            "cell_description": self.role,
            "geometry_label": "whole-file" if self.streams == 1 else "split-256k",
            "outer_block_bytes": self.block_bytes,
            "negate_gate": None,
            "brotli_lgwin": self.brotli_lgwin,
            "independent_streams": streams,
            "source_bytes": source_bytes,
            "block_count": streams,
            "max_reach_bytes": (min(source_bytes, (1 << self.brotli_lgwin) - 1)
                                if self.streams == 1
                                else min(source_bytes, self.block_bytes - 1)),
            "aux_sampling_rate": {"policy": "absent", "pinned": True,
                                  "basis": "Brotli has no aux-index concept",
                                  "not_applicable": True},
            "role": self.role,
            "classifying": False,
            "identity": "same-run matched geometry-control pair member (R1 with R3); "
                        "NOT a reproduction of the retained Class-A Brotli rows",
            "matches_frozen_reference_row": False,
            "frozen_reference_window": FROZEN_BROTLI_WINDOW,
            "explicit_lgwin": self.brotli_lgwin,
            "producer_pin": "PINNED" if BROTLI_PRODUCER_PINNED else PRODUCER_HOLD,
            "comparability_rule": "comparable to the other arm of this pair ONLY. Never compared "
                                  "against the frozen grid's Brotli rows, whose window differs.",
            "structural_capacity": structural_capacity(
                streams, layout=("single whole-file stream; no block-parallel structure"
                                 if self.streams == 1 else "split stream layout")),
            "note": "reference arm: descriptive / non-classifying",
        }


REFERENCE: dict[str, ReferenceArm] = {
    "R1": ReferenceArm("R1", R1_EXPLICIT_LGWIN, 1, LARGE_BLOCK_BYTES,
                       "same-run geometry CONTROL, whole file at explicit lgwin 30. NOT a "
                       "reproduction of the retained Class-A Brotli bar, which was measured at "
                       "BROTLI_DEFAULT_WINDOW (lgwin 22); the two are never compared"),
    "R3": ReferenceArm("R3", 30, -1, SMALL_BLOCK_BYTES,
                       "same-run geometry CONTROL, ceil(n/256 KiB) independent streams at "
                       "lgwin 30; with R1 it prices the generic 256 KiB fragmentation tax"),
}

CANDIDATE_ARMS: tuple[CandidateArm, ...] = tuple(
    CandidateArm(cfg, cell) for cfg in CANDIDATE_CONFIGS for cell in CANDIDATE_CELLS
)


# ---------------------------------------------------------------------------
# Gates that must hold before any arm may run
# ---------------------------------------------------------------------------

def check_arg_strings() -> list[str]:
    """B1 / B5 and gates G-A, G-I, G-K. Empty list == clean."""
    violations: list[str] = []
    for arm in CANDIDATE_ARMS:
        argv = arm.encode_argv(Path("SRC"), Path("OUT"))
        joined = " ".join(argv)
        for bad in FORBIDDEN_ARG_SUBSTRINGS:
            if bad in joined:
                violations.append(f"{arm.cell_id}: forbidden arg {bad!r}")
        for required in ("--bwt-aux=off", f"--decode-threads={PINNED_DECODE_THREADS}"):
            if required not in argv:
                violations.append(f"{arm.cell_id}: missing pin {required}")
        if not any(a.startswith("--block=") for a in argv):
            violations.append(f"{arm.cell_id}: no explicit --block")
        if not any(a.startswith("--negate=") for a in argv):
            violations.append(f"{arm.cell_id}: gate factor not set")
        if not any(a.startswith("--stream-lambda=") for a in argv):
            violations.append(f"{arm.cell_id}: original stream-lambda not passed (G-A)")
        if not any(a.startswith("--shape-states=") for a in argv):
            violations.append(f"{arm.cell_id}: original shape-states not passed (G-A)")
    # No configuration may be collapsed onto another: the retained FRONT-GAP set spans
    # lambda {0.04, 0.00, 0.01} across {mdl, shape}, and every (parse, lambda) pair must
    # remain distinct. This is asserted, not documented.
    lams = {c.config_id: c.stream_lambda for c in CANDIDATE_CONFIGS}
    pairs = {(c.parse, c.stream_lambda) for c in CANDIDATE_CONFIGS}
    if len(pairs) != len(CANDIDATE_CONFIGS):
        violations.append(f"configurations collapsed onto duplicate (parse, lambda): {lams}")
    if len(CANDIDATE_CONFIGS) != 5:
        violations.append(
            f"expected 5 retained FRONT-GAP configurations, got {len(CANDIDATE_CONFIGS)}")
    observed_lambdas = {c.stream_lambda for c in CANDIDATE_CONFIGS}
    if not {0.04, 0.0, 0.01} <= observed_lambdas:
        violations.append(
            f"retained lambda span incomplete: expected {{0.04, 0.0, 0.01}}, "
            f"observed {sorted(observed_lambdas)}")
    for c in CANDIDATE_CONFIGS:
        argv = CandidateArm(c, "G0").config_argv()
        if f"--stream-lambda={c.stream_lambda:g}" not in argv:
            violations.append(f"{c.config_id}: exact lambda {c.stream_lambda:g} not in argv")
    amap = retained_config_argv_map()
    if sorted(amap) != sorted(c.frozen_row for c in CANDIDATE_CONFIGS):
        violations.append(
            f"retained_config_argv_map keys {sorted(amap)} do not match retained rows")
    for arm in REFERENCE.values():
        if not any(a.startswith("--lgwin=") for a in arm.encode_argv("H", Path("S"), Path("O"))):
            violations.append(f"{arm.arm_id}: lgwin must be explicit, never a library default")
    return violations


def retained_config_argv_map() -> dict:
    """Machine-readable map: retained_config_id -> the EXACT argv that reproduces it.

    `retained_config_id` is the row name in the retained Class-A grid, so this map is the
    join key between the frozen CSV and any Q2 arm. The argv is the complete encode
    argument vector for that configuration; the two experimental factors (--block,
    --negate) and the pinned transport options are added per cell. Nothing here is
    inferred, defaulted, or collapsed onto the harness baseline.
    """
    out: dict[str, dict] = {}
    for c in CANDIDATE_CONFIGS:
        arm = CandidateArm(c, "G0")
        out[c.frozen_row] = {
            "config_id": c.config_id,
            "retained_config_id": c.frozen_row,
            "bench_source": f"tools/bench_native.cpp:{c.bench_line}",
            "bench_call": (
                f'bench_anvil(src, "{c.frozen_row}", "{c.parse}", "{c.literal}", '
                f'"{c.entropy}", reps, shape_states={c.shape_states}, '
                f"stream_lambda={c.stream_lambda:g})"
            ),
            "parse": c.parse,
            "literal": c.literal,
            "entropy": c.entropy,
            "shape_states": c.shape_states,
            "stream_lambda": c.stream_lambda,
            "config_argv": arm.config_argv(),
            "argv_template": arm.config_argv() + [
                "--block=<GEOMETRY_BYTES>", "--negate=<on|off>",
                f"--decode-threads={PINNED_DECODE_THREADS}", "--bwt-aux=off",
            ],
            "stream_lambda_is_explicit": True,
            "lambda_source": "tools/bench_native.cpp:24,30 (harness), NOT the CLI default",
        }
    return out


def check_env_identity(env: Mapping[str, str] | None = None) -> list[str]:
    """Assert that no environment variable can alter arm identity.

    src/anvil.cpp:4959 reads ANVIL_STREAM_LAMBDA into opt.stream_lambda. The explicit
    --stream-lambda flag is parsed later (:4977) and therefore wins, but Q2 does not rely
    on that ordering: the variable must be absent, so identity stays argv-determined.
    """
    environ = os.environ if env is None else env
    return [f"{name} is set to {environ[name]!r}; arm identity must be argv-determined"
            for name in ENV_VARS_MUST_BE_UNSET if environ.get(name) is not None]


def verify_interaction_identity() -> dict:
    """Symbolic check of  interaction == (G1-G0) - (G2-G3);  gate G-J.

    Exact integer arithmetic on two independent assignments, so a sign slip in either
    form is caught.
    """
    results = []
    for vals in ({"G0": 11, "G1": 97, "G2": 43, "G3": 5},
                 {"G0": -3, "G1": 0, "G2": 8, "G3": 19},
                 {"G0": 0, "G1": 0, "G2": 0, "G3": 0}):
        lhs = (vals["G1"] - vals["G2"]) - (vals["G0"] - vals["G3"])
        rhs = (vals["G1"] - vals["G0"]) - (vals["G2"] - vals["G3"])
        results.append({"values": vals, "lhs": lhs, "rhs": rhs, "ok": lhs == rhs})
    return {
        "identity": " ".join(INTERACTION_IDENTITY) + " == " + " ".join(INTERACTION_TERMS),
        "ok": all(r["ok"] for r in results),
        "checks": results,
    }


def monotonicity_floor_note(source_bytes: int) -> dict:
    """Exact non-threshold annotation: what geometry does to block count."""
    return {
        "blocks_small": block_count(source_bytes, SMALL_BLOCK_BYTES),
        "blocks_large": block_count(source_bytes, LARGE_BLOCK_BYTES),
        "framing_bytes_small": ANVIL_BLOCK_FRAMING_BYTES * block_count(source_bytes, SMALL_BLOCK_BYTES),
        "framing_bytes_large": ANVIL_BLOCK_FRAMING_BYTES * block_count(source_bytes, LARGE_BLOCK_BYTES),
        "note": "descriptive annotation only; NOT a decision floor and NOT a threshold",
    }


# ---------------------------------------------------------------------------
# Plan emission
# ---------------------------------------------------------------------------

@dataclass(frozen=True)
class CorpusFile:
    name: str
    source_bytes: int
    sha256: str
    identity_role: str = "tracked"
    evidence_role: str = EVIDENCE_ROLE


def plan(corpus: Iterable[CorpusFile], helper: str = "q2_brotli_geom") -> dict:
    violations = check_arg_strings()
    identity = verify_interaction_identity()
    rows: list[dict] = []
    for cf in corpus:
        for arm in CANDIDATE_ARMS:
            geo = arm.geometry(cf.source_bytes)
            rows.append({
                **geo,
                "side": "candidate",
                "file": cf.name,
                "source_bytes": cf.source_bytes,
                "source_sha256": cf.sha256,
                "identity_role": cf.identity_role,
                "evidence_role": cf.evidence_role,
                "argv_encode": arm.encode_argv(Path("SRC"), Path("OUT")),
                "argv_decode": arm.decode_argv(Path("SRC"), Path("OUT")),
                "monotonicity_note": monotonicity_floor_note(cf.source_bytes),
            })
        for arm in REFERENCE.values():
            geo = arm.geometry(cf.source_bytes)
            rows.append({
                **geo,
                "side": "reference",
                "file": cf.name,
                "source_bytes": cf.source_bytes,
                "source_sha256": cf.sha256,
                "identity_role": cf.identity_role,
                "evidence_role": cf.evidence_role,
                "argv_encode": arm.encode_argv(helper, Path("SRC"), Path("OUT")),
                "argv_decode": None,
            })
    candidate_rows = sum(1 for _ in corpus) * len(CANDIDATE_ARMS)
    return {
        "schema": 3,
        "protocol": "Q2-PARITY-2X2-MULTICONFIG",
        "dispatch_authorized": False,
        "class": "adopt-class {engineering} -- measurement parity",
        "purpose": "measurement parity only; not novelty, not tuning, not configuration change",
        "evidence_role": EVIDENCE_ROLE,
        "promotion_authorized": False,
        "decision_tokens": list(DECISION_TOKENS),
        "materiality_threshold": None,
        "epsilon": None,
        "epsilon_policy": "no epsilon parameter exists in this project; the retained "
                          "bracket predicate is evaluated at eps = 0.0 only, and only "
                          "inside the baseline diagnostic",
        "pct_denominator": PCT_DENOMINATOR,
        "configurations": [
            {
                "config_id": c.config_id,
                "frozen_row": c.frozen_row,
                "bench_source": f"tools/bench_native.cpp:{c.bench_line}",
                "parse": c.parse, "literal": c.literal, "entropy": c.entropy,
                "shape_states": c.shape_states, "stream_lambda": c.stream_lambda,
                "argv": CandidateArm(c, "G0").config_argv(),
            }
            for c in CANDIDATE_CONFIGS
        ],
        "cells": {k: {"block_bytes": v[0], "negate_gate": v[1], "description": CELL_DESCRIPTION[k]}
                  for k, v in CELL_GEOMETRY.items()},
        "contrast_algebra": [
            {"id": cid, "treated": t, "control": c, "description": d}
            for cid, t, c, d in CONTRASTS
        ],
        "interaction": {
            "id": INTERACTION_ID,
            "definition": " ".join(INTERACTION_TERMS),
            "identity": " ".join(INTERACTION_IDENTITY),
            "identity_check": identity,
        },
        "geometry_constants": {
            "small_block_bytes": SMALL_BLOCK_BYTES,
            "large_block_bytes": LARGE_BLOCK_BYTES,
            "rev1_block_cap_bytes": REV1_BLOCK_CAP_BYTES,
            "rev2_block_cap_bytes_EXCLUDED": REV2_BLOCK_CAP_BYTES,
            "anvil_block_framing_bytes": ANVIL_BLOCK_FRAMING_BYTES,
            "kbtw_aux_target_walks": KBTW_AUX_TARGET_WALKS,
            "decode_threads": PINNED_DECODE_THREADS,
        },
        "axes": {
            "classifying": list(CLASSIFYING_AXES),
            "non_classifying": list(NON_CLASSIFYING_AXES),
        },
        "counts": {
            "configurations": len(CANDIDATE_CONFIGS),
            "cells_per_configuration": len(CANDIDATE_CELLS),
            "candidate_cells_total": len(CANDIDATE_ARMS),
            "reference_arms": len(REFERENCE),
            "candidate_rows_for_this_corpus": candidate_rows,
        },
        "excluded": [
            "rev-2 (--parse=ratio): different representation, backend-confounded (B1)",
            "BWT backend: per-block libsais re-run biases backend-2-vs-backend-1 at 256 KiB (B1)",
            "aux-index sampling-rate axis: pinned by exclusion (B5)",
        ],
        "gates": {
            "arg_strings": violations,
            "interaction_identity_ok": identity["ok"],
            "config_count_ok": len(CANDIDATE_CONFIGS) == 5,
            "lambda_span_ok": {0.04, 0.0, 0.01} <= {c.stream_lambda for c in CANDIDATE_CONFIGS},
            "env_identity": {
                "env_vars_must_be_unset": list(ENV_VARS_MUST_BE_UNSET),
                "violations": check_env_identity(),
            },
            "producer_pin": {
                "brotli_producer_pinned": BROTLI_PRODUCER_PINNED,
                "hold": None if BROTLI_PRODUCER_PINNED else PRODUCER_HOLD,
                "violations": check_producer_pin(),
            },
        },
        "reference_identity": {
            "role": "same-run matched geometry-control pair",
            "matches_frozen_reference_row": False,
            "frozen_reference_window": FROZEN_BROTLI_WINDOW,
            "explicit_lgwin": R1_EXPLICIT_LGWIN,
            "producer_pin_required": True,
            "producer_pin_state": "PINNED" if BROTLI_PRODUCER_PINNED else PRODUCER_HOLD,
            "withheld_while_unpinned": ["descriptive_only_reference_residual"],
        },
        "retained_config_id_to_argv": retained_config_argv_map(),
        "retained_harness_baseline_stream_lambda": RETAINED_HARNESS_BASELINE_STREAM_LAMBDA,
        "cli_default_stream_lambda": CLI_DEFAULT_STREAM_LAMBDA,
        "aggregate_all_roles": None,
        "aggregate_heldout_only": None,
        "rows": rows,
    }


def frozen_population(suite: Path) -> list[str]:
    """The Q2 corpus population, derived MECHANICALLY from the frozen Class-A CSV.

    That CSV is the retained grid Q2 must be comparable to, and it contains exactly 13 files.
    CHECKSUMS.txt is NOT used to enumerate the population: it has 24 entries, including a later
    PE/synth expansion the public mirror omits and the retained grid never measured. Those files
    are silently out of scope, not silently included.
    """
    names: list[str] = []
    with suite.open(newline="", encoding="utf-8-sig") as handle:
        for row in csv.DictReader(handle):
            name = Path(row["file"].replace("\\", "/")).name
            if name not in names:
                names.append(name)
    return sorted(names)


def _read_checksums(corpus_dir: Path) -> dict[str, tuple[int, str]]:
    """name -> (bytes, sha256) from the corpus manifest. Advisory: it supplies identities for
    the frozen population, it does not define the population."""
    out: dict[str, tuple[int, str]] = {}
    checksums = corpus_dir / "CHECKSUMS.txt"
    for line in checksums.read_text(encoding="utf-8").splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        parts = line.split()
        if len(parts) != 3:
            continue
        sha, size_b, name = parts
        out[name] = (int(size_b), sha)
    return out


def population_report(suite: Path, corpus_dir: Path) -> dict:
    """Fail-closed derivation of the 13-file population, verified against CHECKSUMS."""
    names = frozen_population(suite)
    checksums = _read_checksums(corpus_dir)
    problems: list[str] = []
    if len(names) != Q0_FROZEN_FILE_COUNT:
        problems.append(f"frozen suite yields {len(names)} files, expected {Q0_FROZEN_FILE_COUNT}")
    if tuple(names) != Q0_FROZEN_FILES:
        problems.append(f"frozen population != declared Q0_FROZEN_FILES: {names}")
    missing = [n for n in names if n not in checksums]
    if missing:
        problems.append(f"frozen files absent from CHECKSUMS.txt: {missing}")
    for n in names:
        if n in checksums:
            want_bytes = next((int(r["input_bytes"]) for r in csv.DictReader(
                suite.open(newline="", encoding="utf-8-sig")) if
                Path(r["file"].replace("\\", "/")).name == n), None)
            if want_bytes is not None and checksums[n][0] != want_bytes:
                problems.append(f"{n}: CHECKSUMS bytes {checksums[n][0]} != frozen input_bytes "
                                f"{want_bytes}")
    return {
        "population": names,
        "count": len(names),
        "checksums_entries_present": len(checksums),
        "checksums_entries_not_in_population": sorted(set(checksums) - set(names)),
        "build_output_slots": [n for n in names if n in BUILD_OUTPUT_SLOTS],
        "tracked_slots": [n for n in names if n not in BUILD_OUTPUT_SLOTS],
        "problems": problems,
        "ok": not problems,
        "policy": "population comes from the frozen Class-A CSV, never from CHECKSUMS.txt; "
                  "CHECKSUMS supplies identities only. Files outside the frozen 13 are out of "
                  "scope, not silently included.",
    }


def load_corpus(corpus_dir: Path, suite: Path) -> list[CorpusFile]:
    """The 13 frozen-population files only, each verified against CHECKSUMS.txt.

    Build-output slots are flagged `identity_role='build-output-under-test'`: their identity is
    RECORDED, never asserted against the historical Windows hashes (gate G-B).
    """
    report = population_report(suite, corpus_dir)
    if not report["ok"]:
        raise ValueError("corpus population check failed:\n  "
                         + "\n  ".join(report["problems"]))
    checksums = _read_checksums(corpus_dir)
    return [
        CorpusFile(
            name=name,
            source_bytes=checksums[name][0],
            sha256=checksums[name][1],
            identity_role=("build-output-under-test" if name in BUILD_OUTPUT_SLOTS else "tracked"),
        )
        for name in report["population"]
    ]


def main() -> int:
    ap = argparse.ArgumentParser(description="Q2 arm registry (2x2 x 5 configurations)")
    ap.add_argument("--list-arms", action="store_true")
    ap.add_argument("--verify-identity", action="store_true")
    ap.add_argument("--emit-retained-map", action="store_true",
                    help="machine-readable retained_config_id -> argv map")
    ap.add_argument("--check-env", action="store_true",
                    help="assert no environment variable can alter arm identity")
    ap.add_argument("--plan", type=Path, help="corpus dir containing CHECKSUMS.txt")
    ap.add_argument("--frozen-suite", type=Path,
                    help="frozen Class-A suite CSV; REQUIRED with --plan and the sole authority "
                         "for the 13-file population")
    ap.add_argument("--population-report", action="store_true",
                    help="derive and verify the frozen population, then exit")
    ap.add_argument("--helper", default="q2_brotli_geom")
    ap.add_argument("--out", type=Path)
    args = ap.parse_args()

    if args.verify_identity:
        result = verify_interaction_identity()
        print(json.dumps(result, indent=2, sort_keys=True))
        return 0 if result["ok"] else 1

    if args.check_env:
        violations = check_env_identity()
        print(json.dumps({
            "env_vars_must_be_unset": list(ENV_VARS_MUST_BE_UNSET),
            "violations": violations,
            "ok": not violations,
            "note": "src/anvil.cpp:4959 reads ANVIL_STREAM_LAMBDA; :4977 parses the explicit "
                    "flag afterwards. Q2 requires the variable to be absent so identity is "
                    "argv-determined, independent of that ordering.",
        }, indent=2, sort_keys=True))
        return 1 if violations else 0

    if args.emit_retained_map:
        print(json.dumps({
            "schema": 1,
            "retained_harness_baseline_stream_lambda": RETAINED_HARNESS_BASELINE_STREAM_LAMBDA,
            "cli_default_stream_lambda": CLI_DEFAULT_STREAM_LAMBDA,
            "retained_lambda_span": sorted({c.stream_lambda for c in CANDIDATE_CONFIGS}),
            "retained_configs": retained_config_argv_map(),
        }, indent=2, sort_keys=True))
        return 1 if check_arg_strings() else 0

    if args.population_report:
        if not args.plan or not args.frozen_suite:
            ap.error("--population-report requires --plan and --frozen-suite")
        rep = population_report(args.frozen_suite, args.plan)
        print(json.dumps(rep, indent=2, sort_keys=True))
        return 0 if rep["ok"] else 1

    if args.list_arms or not args.plan:
        print("configurations (exact original bench settings, tools/bench_native.cpp):")
        for c in CANDIDATE_CONFIGS:
            print(f"  {c.config_id:14s} <- {c.frozen_row:22s} ({c.bench_line})  "
                  f"--parse={c.parse} --shape-states={c.shape_states} --stream-lambda={c.stream_lambda:g}")
        print("\ncells (geometry x incompressibility gate), run within EVERY configuration:")
        for cell, (block, neg) in CELL_GEOMETRY.items():
            print(f"  {cell}  block={block:<9d} negate={neg:<4s} {CELL_DESCRIPTION[cell]}")
        print("\ncontrast algebra (exact signed bytes; relative percentages vs "
              f"{PCT_DENOMINATOR}):")
        for cid, treated, control, desc in CONTRASTS:
            print(f"  {cid:22s} = {treated} - {control}   {desc}")
        print(f"  {INTERACTION_ID:22s} = {' '.join(INTERACTION_TERMS)}"
              f"  == {' '.join(INTERACTION_IDENTITY)}")
        print("\nComplete crossed 2x2: every main effect holds the other factor fixed exactly.\n"
              "  geom_effect_gate_on/off : gate held ON / OFF.\n"
              "  gate_effect_at_small/large: geometry held small / large, so block size AND\n"
              "      block count are identical within each pair. Nothing is confounded.\n"
              "  interaction            : does the geometry effect DEPEND ON gate state?\n"
              "\nNOTE: 'gate_effect_at_small' / 'gate_effect_at_large' are GATE effects. They are\n"
              "      NOT window effects and NOT geometry effects, and are never named as such.\n"
              "      REJECTED CLAIM: calling G0 - G3 a 'pure window effect' is wrong -- G0 and G3\n"
              "      share an identical block size, so that contrast never varies geometry.")
        print("\nreference arms (descriptive / non-classifying):")
        for arm in REFERENCE.values():
            print(f"  {arm.arm_id}  lgwin={arm.brotli_lgwin} streams={arm.streams}  -- {arm.role}")
        violations = check_arg_strings()
        identity = verify_interaction_identity()
        print(f"\ncounts: {len(CANDIDATE_CONFIGS)} configurations x {len(CANDIDATE_CELLS)} cells "
              f"= {len(CANDIDATE_ARMS)} candidate cells, + {len(REFERENCE)} reference arms")
        print(f"G-A/G-I/G-K arg-string violations : {violations or 'none'}")
        print(f"G-J interaction identity ok         : {identity['ok']}")
        envv = check_env_identity()
        print(f"G-N env identity ok                 : {not envv} "
              f"(must be unset: {', '.join(ENV_VARS_MUST_BE_UNSET)})")
        print(f"retained lambda span               : "
              f"{sorted({c.stream_lambda for c in CANDIDATE_CONFIGS})} "
              f"(harness baseline {RETAINED_HARNESS_BASELINE_STREAM_LAMBDA}, "
              f"CLI default {CLI_DEFAULT_STREAM_LAMBDA})")
        print("materiality threshold              : NONE (none invented; none applied)")
        print("epsilon                            : NONE (no parameter exists)")
        return 1 if violations or not identity["ok"] else 0

    if not args.frozen_suite:
        ap.error("--frozen-suite is REQUIRED: the 13-file population is derived from it, "
                 "never from CHECKSUMS.txt")
    corpus = load_corpus(args.plan, args.frozen_suite)
    manifest = plan(corpus, helper=args.helper)
    manifest["population"] = population_report(args.frozen_suite, args.plan)
    text = json.dumps(manifest, indent=2, sort_keys=True) + "\n"
    if args.out:
        args.out.parent.mkdir(parents=True, exist_ok=True)
        args.out.write_text(text, encoding="utf-8")
    else:
        sys.stdout.write(text)
    gates = manifest["gates"]
    return 1 if (gates["arg_strings"] or not gates["interaction_identity_ok"]
                 or not gates["config_count_ok"] or not gates["lambda_span_ok"]
                 or gates["env_identity"]["violations"]) else 0


if __name__ == "__main__":
    raise SystemExit(main())
