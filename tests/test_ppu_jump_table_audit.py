import struct, sys, tempfile, unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))


def make_elf(path: Path) -> tuple[int, list[int]]:
    data = bytearray(0x300)
    data[:16] = b'\x7fELF\x02\x02\x01' + bytes(9)
    struct.pack_into('>HHIQQQIHHHHHH', data, 16,
                     2, 21, 1, 0x1100, 64, 0, 0, 64, 56, 1, 0, 0, 0)
    struct.pack_into('>IIQQQQQQ', data, 64,
                     1, 5, 0x100, 0x1000, 0x1000, 0x200, 0x200, 0x1000)
    def put(addr: int, word: int):
        struct.pack_into('>I', data, 0x100 + addr - 0x1000, word)

    bctr = 0x1110
    put(0x1100, 0x7C09502E)
    put(0x1104, 0x7C0007B4)
    put(0x1108, 0x7D405214)
    put(0x110C, 0x7D4903A6)
    put(bctr,   0x4E800420)
    base = bctr + 4
    targets = [0x1140, 0x1150, 0x1160]
    for i, target in enumerate(targets):
        put(base + i * 4, (target - base) & 0xFFFFFFFF)
    put(base + len(targets) * 4, 0x7FFFFFFF)
    path.write_bytes(data)
    return bctr, targets


class PpuJumpTableAuditTests(unittest.TestCase):
    def test_structural_inline_table_is_matched_to_generated_switch(self):
        import audit_ppu_jump_tables as a
        with tempfile.TemporaryDirectory() as td:
            td = Path(td); elf = td / 'EBOOT.ELF'; generated = td / 'generated'; generated.mkdir()
            bctr, targets = make_elf(elf)
            body = ''.join(f'case 0x{x:08X}u: goto loc_{x:08X};' for x in targets)
            (generated / 'ppu_recomp_000.cpp').write_text(
                f'switch ((uint32_t)ctx->ctr) {{ {body} default: ps3_indirect_call(ctx); return; }} return;\n',
                encoding='utf-8')
            r = a.audit(elf, generated)
            self.assertEqual(r['structural_inline_tables'], 1)
            self.assertEqual(r['generated_ctr_switches'], 1)
            self.assertEqual(r['missing_inline_tables'], 0)
            self.assertEqual(r['candidates'][0]['bctr'], bctr)

    def test_missing_generated_switch_is_a_hard_audit_failure(self):
        import audit_ppu_jump_tables as a
        with tempfile.TemporaryDirectory() as td:
            td = Path(td); elf = td / 'EBOOT.ELF'; generated = td / 'generated'; generated.mkdir()
            _, targets = make_elf(elf)
            body = ''.join(f'case 0x{x:08X}u: goto loc_{x:08X};' for x in targets[:-1])
            (generated / 'ppu_recomp_000.cpp').write_text(
                f'switch ((uint32_t)ctx->ctr) {{ {body} default: ps3_indirect_call(ctx); return; }} return;\n',
                encoding='utf-8')
            r = a.audit(elf, generated)
            self.assertEqual(r['structural_inline_tables'], 1)
            self.assertEqual(r['missing_inline_tables'], 1)
            self.assertFalse(r['candidates'][0]['recovered'])

if __name__ == '__main__':
    unittest.main()
