import shutil, subprocess, tempfile, textwrap, unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]

@unittest.skipUnless(shutil.which('g++'), 'g++ unavailable')
class SettingsCppTests(unittest.TestCase):
    def test_ini_roundtrip_and_clamping(self):
        with tempfile.TemporaryDirectory() as td:
            td=Path(td)
            harness=td/'settings_test.cpp'
            harness.write_text(textwrap.dedent(r'''
                #include "comet_settings.h"
                #include <cassert>
                #include <fstream>
                int main(int argc,char**argv){
                  std::ofstream f(argv[1]);
                  f << "width=99999\nheight=200\ndisplay_mode=borderless\nvsync=off\n"
                       "frame_limit=144\nmouse_enabled=yes\nmouse_sensitivity=9.0\nmouse_stick=right\n";
                  f.close();
                  CometSettings s; assert(comet_settings_load(argv[1],s));
                  assert(s.width==7680 && s.height==360 && s.borderless && !s.vsync);
                  assert(s.frame_limit==144 && s.mouse_enabled && s.mouse_sensitivity==6.0f && s.mouse_stick==1);
                  assert(comet_settings_save(argv[2],s));
                  CometSettings t; assert(comet_settings_load(argv[2],t));
                  assert(t.width==s.width && t.height==s.height && t.mouse_stick==1 && !t.vsync);
                  return 0;
                }
            '''))
            exe=td/'settings_test'
            subprocess.run(['g++','-std=c++17','-I',str(ROOT/'port'),str(ROOT/'port/comet_settings.cpp'),str(harness),'-o',str(exe)],check=True)
            subprocess.run([str(exe),str(td/'in.ini'),str(td/'out.ini')],check=True)
