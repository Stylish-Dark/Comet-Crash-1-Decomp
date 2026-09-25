import sys, unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
import patch_comet_parse_pointer_diag as p

class ParsePointerDiagPatchTests(unittest.TestCase):
    def fixture(self):
        parse=[p.PARSE_ENTRY,p.PARSE_SP,p.ALLOC_CALL,p.R31_CALL]
        for site,func in p.PARSE_R23_SITES.items():
            parse.append(f'        ctx->lr = {site}; func_{func}(ctx); DRAIN_TRAMPOLINE(ctx);')
        parse += [p.STORE,'        return;','}']
        caller=[p.CALLER_ENTRY,p.CALLER_SP,p.BASELINE_CALL]
        for site,func in p.CALLER_SITES.items():
            caller.append(f'        ctx->lr = {site}; func_{func}(ctx); DRAIN_TRAMPOLINE(ctx);')
        caller += [p.FINAL_FREE,'        return;','}']
        return '\n'.join(parse+caller)+'\n'

    def test_instruments_register_sp_and_slot_lifetime(self):
        out,changed=p.patch_text(self.fixture())
        self.assertTrue(changed)
        for marker in ['[COMET-PARSE-ALLOC]','[COMET-PARSE-REG-CLOBBER]','[COMET-PARSE-STORE]','[COMET-PARSE-OUT]','[COMET-PARSE-SP-CHANGE]','[COMET-PARSE-SLOT-CHANGE]','[COMET-PARSE-FINAL]']:
            self.assertIn(marker,out)
        self.assertEqual(out.count('[COMET-PARSE-REG-CLOBBER]'),1+len(p.PARSE_R23_SITES))
        self.assertEqual(out.count('[COMET-PARSE-SP-CHANGE]'),len(p.CALLER_SITES))
        self.assertEqual(out.count('[COMET-PARSE-SLOT-CHANGE]'),len(p.CALLER_SITES))
        out2,changed2=p.patch_text(out)
        self.assertFalse(changed2)
        self.assertEqual(out2,out)

    def test_caller_watch_sites_cover_all_live_pointer_branch_calls(self):
        expected={
            '0x001301E8','0x001301F4','0x00130204','0x00130214','0x001302F0','0x00130304',
            '0x00130314','0x00130320','0x0013032C','0x00130334','0x00130340','0x0013034C',
            '0x0013035C','0x00130378','0x00130398','0x001303B0','0x001303C4','0x001303DC',
            '0x001303EC','0x001303F8','0x00130404','0x0013040C','0x00130440','0x00130488',
            '0x00130498','0x001304A4','0x001304B0','0x001304B8','0x001304C4','0x001304D0',
            '0x001304E0','0x0013050C','0x00130528','0x00130534','0x00130548',
        }
        self.assertEqual(set(p.CALLER_SITES),expected)
        self.assertEqual(len(p.CALLER_SITES),35)

    def test_drift_fails_loudly(self):
        with self.assertRaises(ValueError):
            p.patch_text('no Comet parse anchors here')

if __name__=='__main__': unittest.main()
