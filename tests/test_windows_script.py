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
