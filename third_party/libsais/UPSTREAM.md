# libsais upstream provenance

ANVIL vendors the 8-bit `libsais` implementation only as its suffix-array/BWT
construction primitive. The BWT post-transform representation, entropy coding,
backend framing, safety checks, and candidate selection in ANVIL are independent
ANVIL code; `libbsc` is not vendored or linked.

- Upstream: `https://github.com/IlyaGrebnov/libsais`
- Tag: `v2.10.4`
- Commit: `ce90878d784b5ff7d019300535675e4a2e22aae0`
- License: Apache License 2.0 (see `LICENSE` in this directory)
- Vendored files: `include/libsais.h`, `src/libsais.c`, `LICENSE`

The source was copied unmodified from that pinned upstream tag on 2026-09-07.
