import json, os, pathlib, subprocess, sys, tempfile

TMP = pathlib.Path(tempfile.mkdtemp(prefix="i20-an-test-"))
os.chdir(TMP)

caps = [128, 16]
walks = [1024, 4096, 8192, 16384, 32768, 65536]
CW, CC = 1024, 128


def arm_key(c, w):
    return f"c{c}-w{w}"


bwt_files = ["webster", "nci", "dickens", "osdb", "mr", "x-ray", "reymont"]
brotli_files = ["mozilla", "samba", "sao", "ooffice", "xml"]
sizes = {"webster": 41458703, "nci": 33553445, "dickens": 10192446,
         "osdb": 10085684, "mr": 9970564, "x-ray": 8474240, "reymont": 6627202,
         "mozilla": 51220480, "samba": 21606400, "sao": 7251944,
         "ooffice": 6152192, "xml": 5345280}
prep = {
    "schema": 2, "corpus": "silesia", "source_bytes": sum(sizes.values()),
    "file_count": len(files) if False else len(sizes), "walks": walks, "caps_mib": caps,
    "control_walks": CW, "control_cap_mib": CC, "control_arm": arm_key(CC, CW),
    "expected_control_bytes": 46466339, "control_identity": "OK",
    "expected_legacy_bytes": 46446995, "legacy_identity": "OK",
    "legacy_total": 46446995, "legacy_per_file": [],
    "xz_total": 48456004, "brotli_total": 49383136, "xz_surplus": 1989665,
    "by_arm": {}, "attribution": [], "identity": {}, "reference_rows": [],
    "decode_manifests": {},
    "subsets": {"bwt_files": bwt_files, "brotli_files": brotli_files,
                "bwt_bytes": sum(sizes[f] for f in bwt_files),
                "brotli_bytes": sum(sizes[f] for f in brotli_files)},
    "source": [],
    "checks": {"control_portfolio_identity": {"ok": True, "detail": ""}},
}
for cap in caps:
    for w in walks:
        per_file, idx_total, subs = [], 0, 0
        for f in list(sizes):
            route = "bwt" if f in bwt_files else "brotli"
            nsub = 1 + (sizes[f] - 1) // (cap * 1024 * 1024)
            aux = 0
            rem = sizes[f]
            for i in range(nsub):
                ln = min(cap * 1024 * 1024, rem)
                rem -= ln
                t = (ln + w - 1) // w
                r = 2
                while r < t:
                    r <<= 1
                aux += 1 + (ln - 1) // r
            ib = 4 * aux if route == "bwt" else 0
            idx_total += ib
            subs += nsub if route == "bwt" else 0
            per_file.append({"file": f, "route": route, "selected_bytes": 1000,
                              "index_bytes": ib, "aux_rate_min": 1024, "aux_rate_max": 1024,
                              "aux_count": aux, "bwt_subblocks": nsub, "bwt_postcoder": 2})
        total = 46466339 + idx_total + (0 if cap == 128 else 472650)
        prep["by_arm"][arm_key(cap, w)] = {
            "arm": arm_key(cap, w), "cap_mib": cap, "walks": w,
            "portfolio_bytes": total, "bwt_routes": 7, "bwt_routed_files": bwt_files,
            "index_bytes_total": idx_total, "bwt_subblocks_total": subs,
            "per_file": per_file}
pathlib.Path("prepare.json").write_text(json.dumps(prep, indent=2))

r = subprocess.run([sys.executable, os.environ["ANVIL_REPO"] + "/tmp_analyze.py",
                    "prepare.json"], capture_output=True, text=True, encoding="utf-8",
                   errors="replace")
pathlib.Path("out.txt").write_text(r.stdout + "\nSTDERR\n" + r.stderr, encoding="utf-8")
print("rc", r.returncode)
