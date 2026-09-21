import sys, unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import comet_port as c
class T(unittest.TestCase):
 def test_spu_lift_command(self):
  q=c.spu_lift_command(Path('images'),Path('sdk'),Path('lifted'),Path('reg.c'))
  s=' '.join(map(str,q)); self.assertIn('build_spu_workloads.py',s); self.assertIn('--images',q); self.assertIn('--constructor',q); self.assertIn('comet_crash_spu_register_all',q)
 def test_cmake_has_spu_paths(self):
  q=c.cmake_configure_command(Path('sdk'),Path('ppu'),Path('spu'),Path('reg.c'),Path('build'))
  s=' '.join(map(str,q)); self.assertIn('SPU_LIFTED_DIR=',s); self.assertIn('SPU_REGISTRY=',s); self.assertIn('clang-cl',s)
 def test_parser_commands(self):
  p=c.parser()
  for name in ['decrypt','validate','probe','analyze','lift','build','run']:
   with self.subTest(name=name): self.assertEqual(p.parse_args([name]+({'decrypt':['x'],'validate':['x'],'probe':['x'],'analyze':['g','e'],'lift':['e'],'build':[],'run':['g','e']}[name])).cmd,name)

class ManifestCompatibilityTests(unittest.TestCase):
 def test_accepts_current_and_legacy_reference_elf_hashes(self):
  m={'reference_elf_sha256':'new','accepted_elf_sha256':['new','old']}
  self.assertTrue(c.is_reference_elf_hash(m,'new'))
  self.assertTrue(c.is_reference_elf_hash(m,'old'))
  self.assertFalse(c.is_reference_elf_hash(m,'other'))

class BuildPatchTests(unittest.TestCase):
 def test_build_applies_spurs_and_d3d12_patches(self):
  s=(Path(__file__).resolve().parents[1]/'tools'/'comet_port.py').read_text()
  self.assertIn('patch_ps3recomp_spurs',s)
  self.assertIn('cellSpurs.c',s)
  self.assertIn('Comet SPURS urgent-command patch',s)
  self.assertIn('audit_hle_coverage',s)
  self.assertIn('HLE coverage gate',s)
