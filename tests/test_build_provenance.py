import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
import comet_port as c


class BuildProvenanceTests(unittest.TestCase):
    def test_git_state_tracks_commit_and_dirty_flag(self):
        with tempfile.TemporaryDirectory() as td:
            repo=Path(td)/'repo'
            repo.mkdir()
            subprocess.run(['git','init',str(repo)],check=True,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
            subprocess.run(['git','-C',str(repo),'config','user.email','ci@example.invalid'],check=True)
            subprocess.run(['git','-C',str(repo),'config','user.name','CI'],check=True)
            p=repo/'x.txt'; p.write_text('one\n',encoding='utf-8')
            subprocess.run(['git','-C',str(repo),'add','x.txt'],check=True)
            subprocess.run(['git','-C',str(repo),'commit','-m','fixture'],check=True,stdout=subprocess.DEVNULL)
            clean=c.git_state(repo)
            self.assertEqual(len(clean['commit']),40)
            self.assertFalse(clean['dirty'])
            p.write_text('two\n',encoding='utf-8')
            dirty=c.git_state(repo)
            self.assertEqual(dirty['commit'],clean['commit'])
            self.assertTrue(dirty['dirty'])

    def test_build_provenance_counts_generated_units(self):
        with tempfile.TemporaryDirectory() as td:
            root=Path(td)
            recomp=root/'recomp'; recomp.mkdir()
            (recomp/'ppu_recomp_000.cpp').write_text('// x\n')
            (recomp/'ppu_recomp_001.cpp').write_text('// x\n')
            spu=root/'spu'; (spu/'a').mkdir(parents=True); (spu/'b').mkdir()
            (spu/'a'/'spu_recomp.c').write_text('/* a */\n')
            (spu/'b'/'spu_recomp.c').write_text('/* b */\n')
            hle={'imports':171,'covered_imports':171,'missing':[]}
            prov=c.make_build_provenance(
                project_root=root,
                ps3recomp_commit='a'*40,
                hle=hle,
                recomp=recomp,
                spu=spu,
            )
            self.assertEqual(prov['ps3recomp_commit'],'a'*40)
            self.assertEqual(prov['hle_coverage']['covered_imports'],171)
            self.assertEqual(prov['generated_ppu_chunks'],2)
            self.assertEqual(prov['generated_spu_units'],2)

    def test_write_build_provenance_is_stable_json(self):
        with tempfile.TemporaryDirectory() as td:
            build=Path(td)
            path=c.write_build_provenance(build,{'schema_version':1,'z':2})
            data=json.loads(path.read_text(encoding='utf-8'))
            self.assertEqual(data['schema_version'],1)
            self.assertEqual(data['z'],2)

    def test_run_path_embeds_build_provenance(self):
        src=(ROOT/'tools'/'comet_port.py').read_text(encoding='utf-8')
        self.assertIn("provenance_path=a.build/'build_provenance.json'",src)
        self.assertIn("'build_provenance':build_provenance",src)
        self.assertIn("'build_provenance':(metadata or {}).get('build_provenance')",src)


if __name__=='__main__':
    unittest.main()
