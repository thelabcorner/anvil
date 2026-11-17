# zlib pinned replay engine provenance

ANVIL vendors a minimal subset of zlib **only** as its pinned DEFLATE
replay engine for the P4.1 transform-4 lane (decoder never searches;
a replay is accepted only on byte-exact output). No other ANVIL code
links this copy; the rest of the codec keeps using the distribution
zlib, if any.

- Upstream: `https://github.com/madler/zlib`
- Tag: `v1.3.1` (upstream release, Mark Adler, 2024-01-22; annotated tag
  object carrying a PGP signature block)
- Tag object: `925af44f3cde53c6b076611c297850091b5dc7bb`
- Commit: `51b7f2abdade71cd9bb0e7a373ef2610ec6f9daf`
- License: zlib license (see `LICENSE` in this directory)
- Vendored files: `src/{adler32,compress,crc32,deflate,inffast,inflate,inftrees,trees,uncompr,zutil}.c`,
  `include/{zlib,zconf,crc32,deflate,inffast,inffixed,inflate,inftrees,trees,zutil}.h`,
  `LICENSE`, `README.upstream`
- Excluded on purpose: gz* file API (`gzclose/gzlib/gzread/gzwrite` — the
  replay decoder never touches the filesystem), `infback.c` (raw inflate
  callback API — the harness uses `inflate()` with explicit `windowBits`
  only), build scaffolding (`CMakeLists.txt`, `configure`, `Makefile.in`,
  `contrib/`, `doc/`, `examples/`, `test/`, `win32/`, etc.)

The source was copied unmodified from that pinned upstream tag on 2026-11-17.
Engine ID for transform 4 (immutable): `zlib-1.3.1-pinned-v1`. Any future
engine or version is a new ID, never a silent substitution.
