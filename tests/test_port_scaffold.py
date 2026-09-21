import unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
class T(unittest.TestCase):
 def test_cmake_requires_spu(self):
  s=(ROOT/'port'/'CMakeLists.txt').read_text(); self.assertIn('spu_workloads.c',s.lower() if False else s); self.assertIn('spu_recomp.c',s); self.assertIn('runtime/spu',s)
 def test_cmake_is_windows(self): self.assertIn('WIN32',(ROOT/'port'/'CMakeLists.txt').read_text())

class HostLayerTests(unittest.TestCase):
 def test_mouse_override_is_exact_comet_import(self):
  s=(ROOT/'port'/'comet_host.cpp').read_text()
  self.assertIn('0x8B72CDA1u',s)
  self.assertIn('cellPadGetData(port',s)
  self.assertIn('vm_write16(ea+16',s)
  self.assertIn('VK_LBUTTON',s)
  self.assertIn('if ((int32_t)vm_read32(ea) <= 0) return;',s)
  self.assertNotIn('vm_write32(ea,8)',s)
 def test_comet_urgent_spurs_override_is_registered(self):
  s=(ROOT/'port'/'comet_host.cpp').read_text()
  self.assertIn('0x17001000u',s)
  self.assertIn('comet_spurs_add_urgent_command',s)
  self.assertIn('cellSpursAddUrgentCommand [Comet jobchain]',s)
 def test_runtime_overlay_and_graphics_settings_exist(self):
  s=(ROOT/'port'/'comet_host.cpp').read_text()
  self.assertIn('VK_F1',s)
  self.assertIn('Resolution',s)
  self.assertIn('Display mode',s)
  self.assertIn('VSync',s)
  self.assertIn('SetWindowLongPtrA',s)
 def test_main_uses_host_resolution_and_installs_override(self):
  s=(ROOT/'port'/'main.cpp').read_text()
  self.assertIn('comet_host_width()',s)
  self.assertIn('comet_host_install_hle_overrides()',s)
  self.assertIn('comet_host_pump()',s)
 def test_main_initializes_sfo_and_installs_overrides_last(self):
  s=(ROOT/'port'/'main.cpp').read_text()
  self.assertIn('cellGame_init_from_paramsfo',s)
  self.assertIn('PS3_PARAM_SFO',s)
  self.assertLess(s.index('ppu_fs_register();'),s.index('comet_host_install_hle_overrides();'))
 def test_comet_overrides_hdd_bootcheck_and_contentpermit(self):
  s=(ROOT/'port'/'comet_compat.cpp').read_text()
  self.assertIn('0xF52639EAu',s)
  self.assertIn('0x70ACEC67u',s)
  self.assertIn('CELL_GAME_GAMETYPE_HDD',s)
  self.assertIn('/dev_hdd0/game/NPEB00142/USRDIR',s)
 def test_cmake_links_current_windows_runtime_dependencies(self):
  s=(ROOT/'port'/'CMakeLists.txt').read_text()
  for lib in ['d3d12','dxgi','d3dcompiler','xinput','ole32','bcrypt']:
   self.assertIn(lib,s)
