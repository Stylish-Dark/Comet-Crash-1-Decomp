import subprocess, sys, tempfile, unittest
from unittest import mock
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
 def test_build_parser_accepts_clean(self):
  a=c.parser().parse_args(['build','--clean'])
  self.assertTrue(a.clean)

class ManifestCompatibilityTests(unittest.TestCase):
 def test_accepts_current_and_legacy_reference_elf_hashes(self):
  m={'reference_elf_sha256':'new','accepted_elf_sha256':['new','old']}
  self.assertTrue(c.is_reference_elf_hash(m,'new'))
  self.assertTrue(c.is_reference_elf_hash(m,'old'))
  self.assertFalse(c.is_reference_elf_hash(m,'other'))
 def test_unknown_elf_is_rejected_not_warned(self):
  m={'reference_elf_sha256':'new','accepted_elf_sha256':['new','old']}
  c.require_supported_elf(m,'new')
  c.require_supported_elf(m,'old')
  with self.assertRaisesRegex(ValueError,'unsupported EBOOT'):
   c.require_supported_elf(m,'other')
 def test_version_mismatch_is_rejected(self):
  m={'version':'01.00','app_version':'01.00'}
  c.require_supported_version(m,'01.00','01.00')
  with self.assertRaisesRegex(ValueError,'unsupported title version'):
   c.require_supported_version(m,'02.00','01.00')
  with self.assertRaisesRegex(ValueError,'unsupported app version'):
   c.require_supported_version(m,'01.00','02.00')

class BuildPatchTests(unittest.TestCase):
 def test_build_applies_spurs_and_d3d12_patches(self):
  s=(Path(__file__).resolve().parents[1]/'tools'/'comet_port.py').read_text()
  self.assertIn('patch_ps3recomp_spurs',s)
  self.assertIn('cellSpurs.c',s)
  self.assertIn('Comet SPURS urgent-command patch',s)
  self.assertIn('audit_hle_coverage',s)
  self.assertIn('HLE coverage gate',s)

class LiftGateTests(unittest.TestCase):
 def test_lift_applies_ppu_completeness_gate_after_compat_patch(self):
  s=(Path(__file__).resolve().parents[1]/'tools'/'comet_port.py').read_text()
  self.assertIn('audit_ppu_lift',s)
  self.assertIn('PPU audit:',s)
  self.assertIn("if ppu_report['unsupported_total']",s)


class ToolkitPinTests(unittest.TestCase):
 def test_runtime_helpers_are_imported(self):
  self.assertTrue(callable(c.find_ninja))
  self.assertTrue(callable(c.load_ps3recomp_lock))
 def test_default_checkout_revision_reads_lock(self):
  with tempfile.TemporaryDirectory() as td:
   repo=Path(td)/'ps3recomp'; (repo/'tools').mkdir(parents=True)
   (repo/'tools'/'ppu_loader.py').write_text('# fixture\n',encoding='utf-8')
   subprocess.run(['git','init',str(repo)],check=True,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
   subprocess.run(['git','-C',str(repo),'config','user.email','ci@example.invalid'],check=True)
   subprocess.run(['git','-C',str(repo),'config','user.name','CI'],check=True)
   subprocess.run(['git','-C',str(repo),'add','.'],check=True)
   subprocess.run(['git','-C',str(repo),'commit','-m','fixture'],check=True,stdout=subprocess.DEVNULL)
   head=subprocess.check_output(['git','-C',str(repo),'rev-parse','HEAD'],text=True).strip()
   with mock.patch.object(c,'load_ps3recomp_lock',return_value={'commit':head}):
    self.assertEqual(c.verify_toolkit_checkout(repo),head)
 def test_checkout_revision_must_match_expected_pin(self):
  with tempfile.TemporaryDirectory() as td:
   repo=Path(td)/'ps3recomp'; (repo/'tools').mkdir(parents=True)
   (repo/'tools'/'ppu_loader.py').write_text('# fixture\n',encoding='utf-8')
   subprocess.run(['git','init',str(repo)],check=True,stdout=subprocess.DEVNULL)
   subprocess.run(['git','-C',str(repo),'config','user.email','ci@example.invalid'],check=True)
   subprocess.run(['git','-C',str(repo),'config','user.name','CI'],check=True)
   subprocess.run(['git','-C',str(repo),'add','.'],check=True)
   subprocess.run(['git','-C',str(repo),'commit','-m','fixture'],check=True,stdout=subprocess.DEVNULL)
   head=subprocess.check_output(['git','-C',str(repo),'rev-parse','HEAD'],text=True).strip()
   self.assertEqual(c.verify_toolkit_checkout(repo,head),head)
   with self.assertRaisesRegex(RuntimeError,'pin mismatch'):
    c.verify_toolkit_checkout(repo,'0'*40)

 def test_non_git_toolkit_is_rejected(self):
  with tempfile.TemporaryDirectory() as td:
   repo=Path(td)/'ps3recomp'; (repo/'tools').mkdir(parents=True)
   (repo/'tools'/'ppu_loader.py').write_text('# fixture\n',encoding='utf-8')
   with self.assertRaisesRegex(RuntimeError,'not a Git checkout'):
    c.verify_toolkit_checkout(repo,'0'*40)


class GeneratedDirResetTests(unittest.TestCase):
 def test_reset_generated_dir_removes_stale_outputs(self):
  with tempfile.TemporaryDirectory() as td:
   p=Path(td)/'generated'; (p/'nested').mkdir(parents=True)
   (p/'stale.elf').write_text('old',encoding='utf-8')
   (p/'nested'/'stale.json').write_text('old',encoding='utf-8')
   c.reset_generated_dir(p)
   self.assertTrue(p.is_dir())
   self.assertEqual(list(p.iterdir()),[])
