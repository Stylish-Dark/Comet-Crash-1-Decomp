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

class PpuCompletenessAuditTests(unittest.TestCase):
    def test_known_comet_shift_holes_are_clean_after_patch(self):
        import patch_ppu_lift as patch
        import audit_ppu_lift as audit
        src='''void f(ppu_context* ctx) {\n/* TODO: vsrab v1, v11, v13 */;\n/* TODO: vsrb v12, v11, v12 */;\n}\n'''
        out, stats=patch.patch_text(src)
        self.assertEqual(stats,{'vsrab':1,'vsrb':1})
        self.assertEqual(audit.audit_text(out),[])

    def test_unknown_ppu_todo_is_a_hard_audit_failure(self):
        import audit_ppu_lift as audit
        holes=audit.audit_text('''\n/* TODO: mystery v1, v2, v3 */;\n''')
        self.assertEqual(len(holes),1)
        self.assertEqual(holes[0]['line'],2)
        self.assertEqual(holes[0]['instruction'],'mystery v1, v2, v3')

    def test_unsupported_spr_noop_is_a_hard_audit_failure(self):
        import audit_ppu_lift as audit
        holes=audit.audit_text('''\n/* mfspr r3, 999: unsupported SPR -- no-op */;\n''')
        self.assertEqual(len(holes),1)
        self.assertEqual(holes[0]['line'],2)
        self.assertEqual(holes[0]['kind'],'unsupported-spr-noop')
        self.assertEqual(holes[0]['instruction'],'mfspr r3, 999')

    def test_directory_audit_requires_generated_chunks(self):
        import audit_ppu_lift as audit
        import tempfile
        with tempfile.TemporaryDirectory() as td:
            with self.assertRaises(FileNotFoundError):
                audit.audit_path(Path(td))

    def test_directory_audit_is_encoding_agnostic(self):
        import audit_ppu_lift as audit
        import tempfile
        with tempfile.TemporaryDirectory() as td:
            p=Path(td)/'ppu_recomp_000.cpp'
            # 0x97 is a CP-1252 en dash and is invalid standalone UTF-8.
            p.write_bytes(b'// cp1252 \x97 comment\n/* TODO: mystery v1, v2, v3 */;\n')
            r=audit.audit_path(Path(td))
            self.assertEqual(r['unsupported_total'],1)
            self.assertEqual(r['unsupported'][0]['line'],2)
            self.assertEqual(r['unsupported'][0]['instruction'],'mystery v1, v2, v3')

    def test_directory_audit_reports_file_and_line(self):
        import audit_ppu_lift as audit
        import tempfile
        with tempfile.TemporaryDirectory() as td:
            p=Path(td)/'ppu_recomp_000.cpp'
            p.write_text('ok\n/* TODO: strange r1, r2 */;\n',encoding='utf-8')
            r=audit.audit_path(Path(td))
            self.assertEqual(r['source_files'],1)
            self.assertEqual(r['unsupported_total'],1)
            self.assertEqual(r['unsupported'][0]['file'],str(p))
            self.assertEqual(r['unsupported'][0]['line'],2)
