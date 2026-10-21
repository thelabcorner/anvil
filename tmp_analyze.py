#!/usr/bin/env python3
"""Local analysis of a downloaded I20 C2 sweep artifact (exact arithmetic only).

Reads prepare.json (and optionally summary.json) and prints the byte curve,
the per-file charged index bytes, and the Amdahl requirement table.
No codec is executed; this is arithmetic over an already-collected artifact.
"""
import json
import sys
from pathlib import Path

d = json.loads(Path(sys.argv[1]).read_text(encoding="utf-8"))
control_key = d["control_arm"]
control = int(d["by_arm"][control_key]["portfolio_bytes"])
xz = int(d["xz_total"])
br = int(d["brotli_total"])
surplus = xz - control
sub = d["subsets"]

print(f"corpus={d['corpus']} source={d['source_bytes']:,} control={control_key} "
      f"control_bytes={control:,} xz={xz:,} brotli={br:,} surplus={surplus:,}")
print(f"BWT-routed: {sub['bwt_bytes']:,} B ({len(sub['bwt_files'])} files) "
      f"{sub['bwt_files']}")
print(f"Brotli-routed: {sub['brotli_bytes']:,} B ({len(sub['brotli_files'])} files) "
      f"{sub['brotli_files']}")
print()


def key_of(k):
    cap, w = k.split("-")
    return int(cap[1:]), int(w[1:])


print("| arm | cap MiB | W | portfolio bytes | delta vs control | surplus left | "
      "index bytes | BWT sub-blocks | BWT routes | aux r min..max |")
print("|---|---:|---:|---:|---:|---:|---:|---:|---:|---|")
for k in sorted(d["by_arm"], key=key_of):
    v = d["by_arm"][k]
    rates = [r["aux_rate_min"] for r in v["per_file"] if r["route"] == "bwt"]
    delta = v["portfolio_bytes"] - control
    print(f"| {k} | {v['cap_mib']} | {v['walks']} | {v['portfolio_bytes']:,} | "
          f"{delta:+,} | {surplus - delta:+,} | {v['index_bytes_total']:,} | "
          f"{v['bwt_subblocks_total']} | {v['bwt_routes']} | "
          f"{min(rates) if rates else '-'}..{max(rates) if rates else '-'} |")

print()
print("per-file charged index bytes (4 x aux_count, summed over sub-blocks)")
arms = sorted(d["by_arm"], key=key_of)
byfile = {}
for a in arms:
    for r in d["by_arm"][a]["per_file"]:
        e = byfile.setdefault(r["file"], {"route": r["route"], "idx": {}})
        e["idx"][a] = int(r["index_bytes"])
print("| file | route | " + " | ".join(arms) + " |")
print("|---|---|" + "---:|" * len(arms))
for name, e in byfile.items():
    print(f"| {name} | {e['route']} | " + " | ".join(f"{e['idx'].get(a, 0):,}" for a in arms) + " |")
print("| **index total** | | " + " | ".join(
    f"{int(d['by_arm'][a]['index_bytes_total']):,}" for a in arms) + " |")
print("| **portfolio bytes** | | " + " | ".join(
    f"{int(d['by_arm'][a]['portfolio_bytes']):,}" for a in arms) + " |")

if len(sys.argv) > 2:
    s = json.loads(Path(sys.argv[2]).read_text(encoding="utf-8"))
    print()
    print("| arm | portfolio decode s | portfolio MB/s | BWT-path decode s | "
          "BWT-path MB/s | BWT-path speedup | fitted f | f CI95 | vs xz (CI) | "
          "vs Brotli (CI) | peak RSS MiB | RSS/xz | RSS/Brotli | G2 | G4 | G5 | G6 |")
    print("|---|---:|---:|---:|---:|---:|---:|---|---|---|---|---:|---:|---:|---|---|---|---|")
    for r in s["rows"]:
        vx = r.get("vs_xz")
        vb = r.get("vs_brotli")
        bp = r.get("bwtpath")
        f = r.get("f_lf_walk_share")
        fci = r.get("f_lf_walk_share_ci95")
        print(f"| {r['arm']} | {r.get('portfolio_decode_s', 0):.4f} | "
              f"{r.get('portfolio_mbps', 0):.2f} | {r.get('bwtpath_decode_s', 0):.4f} | "
              f"{r.get('bwtpath_mbps', 0):.2f} | {r.get('bwtpath_speedup') or 0:.4f} | "
              + (f"{f:.4f} | [{fci[0]:.4f}, {fci[1]:.4f}] | " if f is not None else "n/a | n/a | ")
              + (f"{vx['ratio']:.4f} [{vx['ci95'][0]:.4f}, {vx['ci95'][1]:.4f}] | " if vx else "n/a | ")
              + (f"{vb['ratio']:.4f} [{vb['ci95'][0]:.4f}, {vb['ci95'][1]:.4f}] | " if vb else "n/a | ")
              + (f"{r['rss_kib']/1024:.1f} | {r.get('rss_ratio_vs_xz', 0):.3f} | "
                 f"{r.get('rss_ratio_vs_brotli', 0):.3f} | " if "rss_kib" in r else "n/a | n/a | n/a | ")
              + f"{'PASS' if r['g2_bytes_below_xz'] else 'FAIL'} | "
              + (f"{'PASS' if r.get('g4_decode_ci_upper_le_2x_brotli') else 'FAIL'} | " if vb else "n/a | ")
              + (f"{'PASS' if r.get('g5_rss_ratio_le_2x_xz') else 'FAIL'} | " if "rss_ratio_vs_xz" in r else "n/a | ")
              + (f"{'PASS' if r.get('g6_rss_ratio_le_2x_brotli') else 'FAIL'} |" if "rss_ratio_vs_brotli" in r else "n/a |")
              + " |")
