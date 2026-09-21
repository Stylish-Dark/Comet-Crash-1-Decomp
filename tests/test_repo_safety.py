import sys, unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import repo_safety as r
class T(unittest.TestCase):
 def test_blocks_game_data(self):
  for p in ['EBOOT.BIN','x.pkg','game/foo.txt','foo/param.sfo','x.edat']: self.assertTrue(r.is_forbidden(p),p)
 def test_allows_source(self):
  for p in ['tools/comet_port.py','port/main.cpp','docs/STATUS.md']: self.assertFalse(r.is_forbidden(p),p)
