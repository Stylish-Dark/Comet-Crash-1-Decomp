import tempfile, unittest, sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import bootstrap_ps3recomp as b
class T(unittest.TestCase):
 def test_lock(self):
  x=b.load_lock(); self.assertEqual(len(x['commit']),40); self.assertIn('ps3recomp',x['repository'])
 def test_fresh_commands_pin(self):
  with tempfile.TemporaryDirectory() as d:
   q=Path(d)/'x'; c=b.build_bootstrap_commands(q,b.load_lock()); self.assertEqual(c[0][1],'clone'); self.assertIn('--detach',c[1])
 def test_dependency_command(self):
  c=b.build_dependency_command(Path('x'),'python'); self.assertEqual(c[:4],['python','-m','pip','install']); self.assertTrue(str(c[-1]).endswith('requirements.txt'))
 def test_local_dependency_command(self):
  c=b.build_local_dependency_command('python'); self.assertEqual(c[:4],['python','-m','pip','install']); self.assertTrue(str(c[-1]).endswith('requirements.txt'))
