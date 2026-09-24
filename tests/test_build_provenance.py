import hashlib
import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
import comet_port as c


def init_repo(path: Path) -> None:
    subprocess.run(['git','init',str(path)],check=True,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
    subprocess.run(['git','-C',str(path),'config','user.email','ci@example.invalid'],check=True)
    subprocess.run(['git','-C',str(path),'config','user.name','CI'],check=True)


def commit_all(path: Path, message: str='fixture') -> str:
    subprocess.run(['git','-C',str(path),'add','.'],check=True)
    subprocess.run(['git','-C',str(path),'commit','-m',message],check=True,stdout=subprocess.DEVNULL)
    return subprocess.check_output(['git','-C',str(path),'rev-parse','HEAD'],text=True).strip()


class BuildProvenanceTests(unittest.TestCase):
    def test_git_state_tracks_exact_tracked_diff(self):
        with tempfile.TemporaryDirectory() as td:
            repo=Path(td)/'repo'; repo.mkdir(); init_repo(repo)
            p=repo/'x.txt'; p.write_text('one\n',encoding='utf-8')
            head=commit_all(repo)
            clean=c.git_state(repo)
            self.assertEqual(clean['commit'],head)
            self.assertFalse(clean['dirty'])
            self.assertEqual(clean['tracked_diff_sha256'],hashlib.sha256(b'').hexdigest())
            p.write_text('two\n',encoding='utf-8')
            dirty=c.git_state(repo)
            self.assertEqual(dirty['commit'],head)
            self.assertTrue(dirty['dirty'])
            self.assertNotEqual(dirty['tracked_diff_sha256'],clean['tracked_diff_sha256'])

    def test_build_provenance_binds_executable_and_generated_units(self):
        with tempfile.TemporaryDirectory() as td:
            root=Path(td)
            recomp=root/'recomp'; recomp.mkdir()
            (recomp/'ppu_recomp_000.cpp').write_text('// x\n')
            (recomp/'ppu_recomp_001.cpp').write_text('// x\n')
            spu=root/'spu'; (spu/'a').mkdir(parents=True); (spu/'b').mkdir()
            (spu/'a'/'spu_recomp.c').write_text('/* a */\n')
            (spu/'b'/'spu_recomp.c').write_text('/* b */\n')
            exe=root/'CometCrashPC.exe'; exe.write_bytes(b'native-fixture')
            hle={'imports':171,'covered_imports':171,'missing':[]}
            prov=c.make_build_provenance(
                project_root=root,
                ps3recomp_commit='a'*40,
                hle=hle,
                recomp=recomp,
                spu=spu,
                executable=exe,
            )
            self.assertEqual(prov['schema_version'],2)
            self.assertEqual(prov['ps3recomp_commit'],'a'*40)
            self.assertEqual(prov['hle_coverage']['covered_imports'],171)
            self.assertEqual(prov['generated_ppu_chunks'],2)
            self.assertEqual(prov['generated_spu_units'],2)
            self.assertEqual(prov['native_executable']['size'],len(b'native-fixture'))
            self.assertEqual(prov['native_executable']['sha256'],hashlib.sha256(b'native-fixture').hexdigest())

    def _verified_fixture(self, root: Path):
        init_repo(root)
        tracked=root/'tracked.txt'; tracked.write_text('source\n',encoding='utf-8')
        commit_all(root)
        recomp=root/'recomp'; recomp.mkdir()
        (recomp/'ppu_recomp_000.cpp').write_text('// fixture\n')
        spu=root/'spu'; (spu/'a').mkdir(parents=True)
        (spu/'a'/'spu_recomp.c').write_text('/* fixture */\n')
        exe=root/'CometCrashPC.exe'; exe.write_bytes(b'exe-v1')
        hle={'imports':171,'covered_imports':171,'missing':[]}
        pin='b'*40
        prov=c.make_build_provenance(
            project_root=root,ps3recomp_commit=pin,hle=hle,
            recomp=recomp,spu=spu,executable=exe,
        )
        path=c.write_build_provenance(root,prov)
        return tracked,exe,path,pin,prov

    def test_verify_build_provenance_accepts_exact_snapshot(self):
        with tempfile.TemporaryDirectory() as td:
            root=Path(td)
            _,exe,path,pin,prov=self._verified_fixture(root)
            got=c.verify_build_provenance(path,exe,root,pin)
            self.assertEqual(got['native_executable'],prov['native_executable'])

    def test_verify_build_provenance_rejects_swapped_executable(self):
        with tempfile.TemporaryDirectory() as td:
            root=Path(td)
            _,exe,path,pin,_=self._verified_fixture(root)
            exe.write_bytes(b'exe-v2')
            with self.assertRaisesRegex(RuntimeError,'executable .*mismatch'):
                c.verify_build_provenance(path,exe,root,pin)

    def test_verify_build_provenance_rejects_source_change(self):
        with tempfile.TemporaryDirectory() as td:
            root=Path(td)
            tracked,exe,path,pin,_=self._verified_fixture(root)
            tracked.write_text('changed\n',encoding='utf-8')
            with self.assertRaisesRegex(RuntimeError,'tracked source diff changed'):
                c.verify_build_provenance(path,exe,root,pin)

    def test_verify_build_provenance_rejects_toolchain_change(self):
        with tempfile.TemporaryDirectory() as td:
            root=Path(td)
            _,exe,path,_,_=self._verified_fixture(root)
            with self.assertRaisesRegex(RuntimeError,'ps3recomp provenance mismatch'):
                c.verify_build_provenance(path,exe,root,'c'*40)

    def test_write_build_provenance_is_stable_json(self):
        with tempfile.TemporaryDirectory() as td:
            build=Path(td)
            path=c.write_build_provenance(build,{'schema_version':2,'z':2})
            data=json.loads(path.read_text(encoding='utf-8'))
            self.assertEqual(data['schema_version'],2)
            self.assertEqual(data['z'],2)

    def test_run_path_enforces_and_embeds_provenance(self):
        src=(ROOT/'tools'/'comet_port.py').read_text(encoding='utf-8')
        start=src.index('def cmd_run(a):')
        body=src[start:src.index('def parser():',start)]
        self.assertIn('verify_build_provenance(',body)
        self.assertIn("'provenance_verification':provenance_verification",body)
        self.assertLess(body.index('verify_build_provenance('),body.index('run_logged('))


if __name__=='__main__':
    unittest.main()
