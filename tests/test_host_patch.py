import sys, unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import patch_ps3recomp_host as p
class HostPatchTests(unittest.TestCase):
 def test_vsync_present_becomes_runtime_switch(self):
  src='before\n    '+p.OLD+'\nafter\n'
  out,changed=p.patch_text(src)
  self.assertTrue(changed)
  self.assertIn('COMET_VSYNC',out)
  self.assertIn('comet_sync_interval',out)
  out2,changed2=p.patch_text(out)
  self.assertFalse(changed2); self.assertEqual(out2,out)
 def test_changed_upstream_fails_loudly(self):
  with self.assertRaises(ValueError): p.patch_text('no present here')

class HostDiagnosticPatchTests(unittest.TestCase):
 def test_all_verified_init_failures_gain_specific_diagnostics(self):
  src='\n'.join(item[1] for item in p.DIAG_PATCHES)
  out,count=p.patch_diagnostics_text(src)
  self.assertEqual(count,len(p.DIAG_PATCHES))
  for _,_,_,marker in p.DIAG_PATCHES:
   self.assertIn(marker,out)
  out2,count2=p.patch_diagnostics_text(out)
  self.assertEqual(count2,0)
  self.assertEqual(out2,out)
 def test_missing_verified_diagnostic_site_fails_loudly(self):
  src='\n'.join(item[1] for item in p.DIAG_PATCHES[:-1])
  with self.assertRaisesRegex(ValueError,'upstream changed'):
   p.patch_diagnostics_text(src)
