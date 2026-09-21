import json, tempfile, unittest
from pathlib import Path
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import audit_hle_coverage as a

class CoverageTests(unittest.TestCase):
    def test_parse_generated_and_override_nids(self):
        generated='''
        ps3_hle_register(0x11111111u, "one", (void*)one);
        ps3_hle_register_ctx(0x22222222u, "two", two);
        '''
        overrides='''
        constexpr uint32_t NID_X = 0x33333333u;
        constexpr uint32_t NID_DECLARED_ONLY = 0x44444444u;
        reg(NID_X, "x", handler);
        ps3_hle_register_ctx(0x55555555u, "literal", handler2);
        '''
        self.assertEqual(a.parse_registered_nids(generated),{0x11111111,0x22222222})
        self.assertEqual(a.parse_override_nids(overrides),{0x33333333,0x55555555})

    def test_audit_reports_only_uncovered(self):
        imports=[{'library':'a','nid':'0x11111111'},{'library':'b','nid':'0x22222222'},{'library':'c','nid':'0x33333333'}]
        missing=a.uncovered_imports(imports,{0x11111111,0x33333333})
        self.assertEqual([x['nid'] for x in missing],['0x22222222'])


    def test_declared_but_unregistered_override_does_not_count_as_coverage(self):
        overrides='''
        constexpr uint32_t NID_REAL = 0x11111111u;
        constexpr uint32_t NID_STALE = 0x22222222u;
        reg(NID_REAL, "real", handler);
        '''
        covered=a.parse_override_nids(overrides)
        self.assertEqual(covered,{0x11111111})
        imports=[{'library':'a','nid':'0x11111111'},{'library':'b','nid':'0x22222222'}]
        self.assertEqual([x['nid'] for x in a.uncovered_imports(imports,covered)],['0x22222222'])

    def test_real_comet_override_sources_include_known_port_overrides(self):
        root=Path(__file__).resolve().parents[1]
        nids=set()
        for p in [root/'port/comet_host.cpp',root/'port/comet_compat.cpp']:
            nids |= a.parse_override_nids(p.read_text())
        for nid in [0x8B72CDA1,0x17001000,0x3A33C1FD,0x0D3C22CE,0x39651E01]:
            self.assertIn(nid,nids)

if __name__=='__main__': unittest.main()
