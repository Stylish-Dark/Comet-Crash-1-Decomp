import sys, unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))

class PpuPatchTests(unittest.TestCase):
    def test_vector_byte_right_shifts_replace_todos_alias_safely(self):
        import patch_ppu_lift as p
        src='''void f(ppu_context* ctx) {\n/* TODO: vsrab v1, v11, v13 */;\n/* TODO: vsrb v12, v11, v12 */;\n}\n'''
        out, stats=p.patch_text(src)
        self.assertNotIn('TODO: vsrab',out)
        self.assertNotIn('TODO: vsrb',out)
        self.assertEqual(stats,{'vsrab':1,'vsrb':1})
        self.assertIn('int8_t',out)
        self.assertIn('memcpy',out)  # temp copies keep vD==vA/vB correct

    def test_unrelated_todo_is_untouched(self):
        import patch_ppu_lift as p
        src='/* TODO: mystery v1, v2, v3 */;\n'
        out, stats=p.patch_text(src)
        self.assertEqual(out,src)
        self.assertEqual(stats,{'vsrab':0,'vsrb':0})

class SpuReachabilityTests(unittest.TestCase):
    def test_unreachable_unsupported_word_does_not_fail(self):
        import audit_spu_lift as a
        src='''
void foo_spu_func_00000010(spu_context* ctx) { foo_spu_func_00000020(ctx); }
void foo_spu_func_00000020(spu_context* ctx) { SPU_RET(ctx); }
void foo_spu_func_00000030(spu_context* ctx) { spu_unsupported(0x30, ".word"); }
'''
        r=a.audit_text(src,0x10)
        self.assertEqual(r['reachable_functions'],2)
        self.assertEqual(r['unsupported_total'],1)
        self.assertEqual(r['unsupported_reachable'],0)
        self.assertEqual(r['unsupported_unreachable'],1)

    def test_reachable_unsupported_is_reported(self):
        import audit_spu_lift as a
        src='''
void foo_spu_func_00000010(spu_context* ctx) { foo_spu_func_00000020(ctx); }
void foo_spu_func_00000020(spu_context* ctx) { spu_unsupported(0x24, "mystery"); }
'''
        r=a.audit_text(src,0x10)
        self.assertEqual(r['unsupported_reachable'],1)
        self.assertEqual(r['reachable_unsupported'][0]['mnemonic'],'mystery')
