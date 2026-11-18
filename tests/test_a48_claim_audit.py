"""Scalar-only determinism/integrity tests; no codec builds or benchmarks."""
import csv
import json
import tempfile
import unittest
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from a48_claim_audit import audit, classify

NAMES = (
    'dickens', 'mozilla', 'mr', 'nci', 'ooffice', 'osdb',
    'reymont', 'samba', 'sao', 'webster', 'xml', 'x-ray',
)
CODECS = ('anvil-cap8', 'xz-9e', 'brotli-q11-lw30')
FIELDS = ('file', 'input_bytes', 'codec', 'compressed_bytes', 'ratio',
          'encode_MBps', 'decode_MBps', 'roundtrip')


def make_fixture(root):
    root = Path(root)
    (root/'files.txt').write_text('\n'.join(NAMES)+'\n')
    out = root / 'paired'
    out.mkdir()
    rows=[]
    timing=[]
    for name in NAMES:
        for codec, size in zip(CODECS, (200000, 300000, 400000)):
            rows.append(dict(file=name,input_bytes='1000000',codec=codec,
                             compressed_bytes=str(size),ratio=str(size/1000000),
                             encode_MBps='1.000',decode_MBps='1.000',roundtrip='OK'))
        timing.append(dict(file=name,timing_valid_all=True,
                           decode_null_ci=[0.95,1.05],encode_null_ci=[0.95,1.05]))
        for phase, n in (('dec',9),('enc',7)):
            for opponent, ctl in (('xz','xz'),('brotli','brotli_lw'),('null','anvil')):
                pairs=[dict(control_s=1.0,candidate_s=1.0) for _ in range(n)]
                cmds=['anvil','d' if phase=='dec' else 'c', name]
                ctrl=cmds if opponent=='null' else [ctl, '-dc' if phase=='dec' else '-c', name]
                summary=dict(timing_valid=True,paired_ratio_ci95=[0.95,1.05],
                    control=dict(argv=ctrl,median_s=1.0),candidate=dict(argv=cmds,median_s=1.0),pairs=pairs)
                (out/f'paired-{name}-{phase}-{opponent}.json').write_text(json.dumps(summary))
    with (root/'grid.csv').open('w',newline='') as f:
        w=csv.DictWriter(f,FIELDS);w.writeheader();w.writerows(rows)
    (root/'timing-validity.json').write_text(json.dumps(timing))
    return root/'grid.csv',root/'timing-validity.json',root/'files.txt',out


class ClaimAuditTests(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory()
        self.paths=make_fixture(self.temp.name)

    def tearDown(self):
        self.temp.cleanup()

    def check(self):
        return audit(*self.paths)

    def test_positive_is_scoped(self):
        d=self.check()
        self.assertEqual(d['errors'],[])
        self.assertEqual(d['timing_uncertainties'],[])
        self.assertEqual(d['decision'],'PROVISIONAL_SCOPED_THIN_FRONT_CROSSING_ONLY')
        self.assertEqual(d['per_file_counts']['FRONT-CROSSING'],24)
        self.assertEqual(d['aggregate_counts']['FRONT-CROSSING'],2)

    def test_bad_roundtrip_is_hard_invalid(self):
        path=self.paths[0]
        s=path.read_text().replace('0.2,1.000,1.000,OK','0.2,1.000,1.000,FAIL',1)
        path.write_text(s)
        self.assertEqual(self.check()['decision'],'INVALID_INCOMPLETE_OR_FAILED_CORRECTNESS')

    def test_missing_paired_evidence_is_invalid(self):
        (self.paths[3]/'paired-dickens-dec-null.json').unlink()
        self.assertEqual(self.check()['decision'],'INVALID_INCOMPLETE_OR_FAILED_CORRECTNESS')

    def test_untimed_reference_rename_is_invalid(self):
        path=self.paths[0]
        path.write_text(path.read_text().replace('xz-9e','xz-weak',1))
        self.assertEqual(self.check()['decision'],'INVALID_INCOMPLETE_OR_FAILED_CORRECTNESS')

    def test_raw_median_tampering_is_invalid(self):
        path=self.paths[3]/'paired-dickens-dec-xz.json'
        data=json.loads(path.read_text())
        data['control']['median_s']=1.5
        path.write_text(json.dumps(data))
        self.assertEqual(self.check()['decision'],'INVALID_INCOMPLETE_OR_FAILED_CORRECTNESS')

    def test_noisy_null_is_inconclusive(self):
        path=self.paths[1]
        data=json.loads(path.read_text())
        data[0]['encode_null_ci']=[1.01,1.05]
        path.write_text(json.dumps(data))
        self.assertEqual(self.check()['decision'],'INCONCLUSIVE_TIMING_NOT_H0')

    def test_order_of_hypotheses(self):
        p=dict(ratio='0.5',decode_MBps='2')
        qlo=dict(ratio='0.4',decode_MBps='1')
        qhi=dict(ratio='0.6',decode_MBps='3')
        self.assertEqual(classify(p,[qlo,qhi],'decode_MBps'),'FRONT-GAP')
        self.assertEqual(classify(p,[dict(ratio='0.4',decode_MBps='3')],'decode_MBps'),'DOMINATED')
        self.assertEqual(classify(dict(ratio='0.97',decode_MBps='100'),[qlo,qhi],'decode_MBps'),'DEGENERATE')
        self.assertEqual(classify(dict(ratio='0.2',decode_MBps='0.1'),[qlo,qhi],'decode_MBps'),'FRONT-CROSSING')


if __name__=='__main__':
    unittest.main()
