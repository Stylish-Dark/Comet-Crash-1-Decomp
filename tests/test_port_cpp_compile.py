import shutil, subprocess, unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
FAKE=ROOT/'tests'/'fixtures'/'fake_win'

@unittest.skipUnless(shutil.which('g++'),'g++ unavailable')
class PortCppCompileTests(unittest.TestCase):
    def test_host_units_syntax_compile_together(self):
        subprocess.run([
            'g++','-std=c++17','-fsyntax-only',
            '-I',str(FAKE),'-I',str(ROOT/'port'),
            str(ROOT/'port'/'comet_host.cpp'),
            str(ROOT/'port'/'comet_settings.cpp'),
            str(ROOT/'port'/'comet_compat.cpp'),
        ],check=True)

    def test_main_unit_syntax_compiles(self):
        subprocess.run([
            'g++','-std=c++17','-fsyntax-only','-D_WIN32',
            '-I',str(FAKE),'-I',str(ROOT/'port'),
            str(ROOT/'port'/'main.cpp'),
        ],check=True)

if __name__=='__main__': unittest.main()
