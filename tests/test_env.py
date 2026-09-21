import os, sys, tempfile, unittest
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"tools"))
import check_env

class EnvironmentDiscoveryTests(unittest.TestCase):
    def test_find_ninja_prefers_path(self):
        with patch.object(check_env.shutil,"which",return_value="C:/Tools/ninja.exe"):
            self.assertEqual(check_env.find_ninja(),"C:/Tools/ninja.exe")

    def test_find_ninja_uses_python_package_binary_when_not_on_path(self):
        with tempfile.TemporaryDirectory() as td:
            exe=Path(td)/("ninja.exe" if os.name=="nt" else "ninja")
            exe.write_bytes(b"")
            fake=SimpleNamespace(BIN_DIR=td)
            with patch.object(check_env.shutil,"which",return_value=None), patch.dict(sys.modules,{"ninja":fake}):
                self.assertEqual(check_env.find_ninja(),str(exe))

if __name__=="__main__": unittest.main()
