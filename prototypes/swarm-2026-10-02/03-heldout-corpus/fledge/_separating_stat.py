import importlib.util, sys, pathlib
p = pathlib.Path(sys.argv[1])
spec = importlib.util.spec_from_file_location("sfx", p); m = importlib.util.module_from_spec(spec)
sys.modules["sfx"] = m; spec.loader.exec_module(m)
F = m.FIXTURES
idxs = [m.Index(c, F[c]) for c in sorted(F)]
m.mark_rare(idxs, rare_max=2)
by = {i.corpus_id: i for i in idxs}
print(f"{'pair':30} {'|rareA|':>8} {'|rareB|':>8} {'inter':>6} {'cont':>7} {'jacc':>7} {'screen':>7}  verdict")
rows=[]
for a,b in [("v-textdupA","v-textdupB"),("v-textdupA","v-textdupC"),
            ("v-a","v-b"),("v-exact-a","v-exact-b"),("v-boiler1","v-boiler2"),
            ("v-rareA","v-rareB"),("v-a","v-unique"),("v-license","v-licensed")]:
    ia, ib = by[a], by[b]
    ra, rb = ia.rare_grams, ib.rare_grams
    inter = len(ra & rb)
    cont = inter/min(len(ra),len(rb)) if min(len(ra),len(rb)) else 0
    jac  = inter/len(ra|rb) if (ra or rb) else 0
    sc   = ia.jaccard_screen(ib)
    tl   = ia.text_like, ib.text_like
    rows.append((a,b,cont,jac,sc,tl))
    print(f"{a+' vs '+b:30} {len(ra):8} {len(rb):8} {inter:6} {cont:7.4f} {jac:7.4f} {sc:7.4f}  textlike={tl[0]:.2f}/{tl[1]:.2f}")
print()
print("C5 must FIRE   : textdupA/B (genuine near-dup), v-a/v-b, exact-a/b")
print("C5 must ABSTAIN: textdupA/C (same envelope, different values), boiler*, rare*, license, a/unique")
print()
print("NOTE: v-rareA/B and boiler pairs are already excluded by the text-likeness")
print("      gate (binary / prevalence). The statistic must separate the two")
print("      TEXT cases: textdupA/B (fire) from textdupA/C (abstain).")
