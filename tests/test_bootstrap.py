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
 def test_existing_checkout_is_restored_to_clean_pin(self):
  with tempfile.TemporaryDirectory() as d:
   q=Path(d)/'x'; (q/'.git').mkdir(parents=True)
   lock=b.load_lock(); c=b.build_bootstrap_commands(q,lock)
   joined=[' '.join(x) for x in c]
   self.assertTrue(any('remote set-url origin' in x and lock['repository'] in x for x in joined))
   self.assertTrue(any('checkout --detach '+lock['commit'] in x for x in joined))
   self.assertTrue(any('reset --hard '+lock['commit'] in x for x in joined))
   self.assertTrue(any('clean -ffd' in x for x in joined))
 def test_dependency_command(self):
  c=b.build_dependency_command(Path('x'),'python'); self.assertEqual(c[:4],['python','-m','pip','install']); self.assertTrue(str(c[-1]).endswith('requirements.txt'))
 def test_local_dependency_command(self):
  c=b.build_local_dependency_command('python'); self.assertEqual(c[:4],['python','-m','pip','install']); self.assertTrue(str(c[-1]).endswith('requirements.txt'))
