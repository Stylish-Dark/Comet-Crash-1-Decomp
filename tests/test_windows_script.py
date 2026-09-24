import unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
class WindowsScriptTests(unittest.TestCase):
 def test_one_command_pipeline_covers_decrypt_through_run(self):
  s=(ROOT/'scripts'/'build_and_run.cmd').read_text()
  for command in [' decrypt ',' validate ',' analyze ',' lift ',' build ',' run ']: self.assertIn(command,s)
  self.assertIn('vcvars64.bat',s)
  self.assertIn('clang-cl.exe',s)
  self.assertIn('F1 = Graphics/Input menu',s)
 def test_accepts_ps3_game_parent_or_title_root(self):
  s=(ROOT/'scripts'/'build_and_run.cmd').read_text()
  self.assertIn('PS3_GAME\\PARAM.SFO',s)
  self.assertIn('USRDIR\\EBOOT.BIN',s)
 def test_bootstrap_always_verifies_dependencies(self):
  s=(ROOT/'scripts'/'build_and_run.cmd').read_text()
  self.assertIn('Preparing pinned ps3recomp toolchain and dependencies',s)
  self.assertIn('call "%ROOT%\\scripts\\bootstrap.cmd"',s)
  self.assertNotIn('--skip-deps',s)
 def test_one_command_pipeline_uses_clean_cmake_build(self):
  s=(ROOT/'scripts'/'build_and_run.cmd').read_text()
  self.assertIn('comet_port.py" build --clean',s)

class NativeRunTimeoutTests(unittest.TestCase):
 def test_optional_native_run_timeout_is_forwarded(self):
  s=(ROOT/'scripts'/'build_and_run.cmd').read_text()
  self.assertIn('boot-timeout-seconds',s)
  self.assertIn('boot-timeout-seconds',s)
  self.assertIn('--timeout "%~2"',s)
  self.assertNotIn('RUN_TIMEOUT_ARG',s)

class DeliveryTests(unittest.TestCase):
 def test_real_exe_path_is_printed(self):
  s=(ROOT/'scripts'/'build_and_run.cmd').read_text()
  self.assertIn('[output] REAL native EXE:',s)
  self.assertIn('build\\CometCrashPC.exe',s)
 def test_portable_builder_uses_source_bundle_and_real_pipeline(self):
  s=(ROOT/'scripts'/'portable_builder.cmd').read_text()
  self.assertIn('CometCrashPC-source.bundle',s)
  self.assertIn('git clone',s)
  self.assertIn('scripts\\build_and_run.cmd',s)
  self.assertIn('build\\CometCrashPC.exe',s)
 def test_ci_uploads_builder_and_marks_scaffold_nonplayable(self):
  s=(ROOT/'.github'/'workflows'/'tests.yml').read_text()
  self.assertIn('actions/upload-artifact@v4',s)
  self.assertIn('CometCrashPC-Windows-Builder',s)
  self.assertIn('CometCrashPC-ci-scaffold',s)
  self.assertIn('README-NOT-PLAYABLE.txt',s)
  self.assertIn('git bundle create delivery/CometCrashPC-source.bundle HEAD',s)
