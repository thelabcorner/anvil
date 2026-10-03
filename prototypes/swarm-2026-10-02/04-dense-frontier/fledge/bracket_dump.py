import csv

rows = list(csv.DictReader(open("tests/benchmark-suite.frozen-bdc90474.plus-xz.csv", newline="", encoding="utf-8")))
refs = ("brotli-q1", "brotli-q4", "brotli-q6", "brotli-q9", "brotli-q11",
        "zstd-1", "zstd-3", "zstd-9", "zstd-19", "xz-9e")
g = [r for r in rows if r["file"].endswith("generated.json") and r["codec"] in refs]
print("REFERENCE ROWS, generated.json:")
for r in sorted(g, key=lambda r: float(r["ratio"])):
    print("  {:<10} ratio={:<7} enc={:<9} dec={}".format(
        r["codec"], r["ratio"], r["encode_MBps"], r["decode_MBps"]))
a = [r for r in rows if r["file"].endswith("generated.json") and r["codec"].startswith("anvil-")]
print("best anvil ratio here:", min(float(r["ratio"]) for r in a))
print()
p = next(r for r in a if r["codec"] == "anvil-shape-rans")
pr, pm = float(p["ratio"]), float(p["encode_MBps"])
print("candidate anvil-shape-rans: ratio={} enc={}".format(pr, pm))
print("brackets with lo.ratio<pr, lo.enc<pm, hi.ratio>pr, hi.enc>pm:")
for lo in g:
    lr, lm = float(lo["ratio"]), float(lo["encode_MBps"])
    if not (lr < pr and lm < pm):
        continue
    for hi in g:
        hr, hm = float(hi["ratio"]), float(hi["encode_MBps"])
        if hr > pr and hm > pm and lr < hr and lm < hm:
            print("  lo={:<10}(r={},enc={})  hi={:<10}(r={},enc={})  ratio_sep={:.1%} mbps_sep={:.1%}".format(
                lo["codec"], lr, lm, hi["codec"], hr, hm,
                (hr - lr) / lr, (hm - lm) / lm))