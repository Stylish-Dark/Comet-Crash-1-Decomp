import shutil, subprocess, tempfile, textwrap, unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]

class CompatHleSourceTests(unittest.TestCase):
    def test_exact_comet_missing_nids_are_registered(self):
        s=(ROOT/'port'/'comet_compat.cpp').read_text()
        required={
            '0x3A33C1FDu':'_cellGcmFunc15',
            '0x0D3C22CEu':'cellRescSetWaitFlip',
            '0xFDB8F926u':'sys_net_free_thread_context',
            '0x6EE62ED2u':'sceNpManagerGetContentRatingFlag',
            '0xB1E0718Bu':'sceNpManagerGetAccountRegion',
            '0xB9F93BBBu':'sceNpScoreCreateTitleCtx',
            '0x259113B8u':'sceNpScoreDestroyTitleCtx',
            '0x6F5E8143u':'sceNpScoreCreateTransactionCtx',
            '0xC5F4CF82u':'sceNpScoreDestroyTransactionCtx',
            '0x1672170Eu':'sceNpScoreRecordScore',
            '0x05D65DFFu':'sceNpScoreGetRankingByNpId',
            '0xFBC82301u':'sceNpScoreGetRankingByRange',
            '0xEE5B20D9u':'sceNpScoreAbortTransaction',
            '0x48BD97C7u':'sceNpTrophyAbortHandle',
            '0x2F457571u':'cellVideoExportInitialize2',
            '0x81296524u':'cellVideoExportFromFile',
            '0xC15BE817u':'cellVideoExportFinalize',
            '0x122E0D0Fu':'cellVideoUploadInitialize',
            '0x39651E01u':'cellRecOpen',
            '0x4AC76585u':'cellRecClose',
            '0x5B45439Du':'cellRecStop',
            '0x964CD1B8u':'cellRecStart',
            '0xDBF22BD1u':'cellRecQueryMemSize',
        }
        for nid,name in required.items():
            self.assertIn(nid,s,name)
            self.assertIn(name,s,nid)

    def test_optional_services_do_not_silently_fake_success(self):
        s=(ROOT/'port'/'comet_compat.cpp').read_text()
        self.assertIn('SCE_NP_COMMUNITY_ERROR_INVALID_ONLINE_ID',s)
        self.assertIn('CELL_VIDEO_UPLOAD_ERROR_SERVICE_UNAVAILABLE',s)
        self.assertIn('CELL_REC_ERROR_FATAL',s)
        self.assertIn('CELL_VIDEO_EXPORT_UTIL_ERROR_INITIALIZE',s)

    def test_core_compat_has_real_wait_behavior(self):
        s=(ROOT/'port'/'comet_compat.cpp').read_text()
        self.assertIn('cellGcmSetWaitFlip()',s)

@unittest.skipUnless(shutil.which('g++'),'g++ unavailable')
class CompatHleCompileTests(unittest.TestCase):
    def test_compat_unit_syntax_compiles(self):
        with tempfile.TemporaryDirectory() as td:
            td=Path(td)
            (td/'ppu_recomp.h').write_text(textwrap.dedent('''
                #pragma once
                #include <stdint.h>
                struct ppu_context { uint64_t gpr[32]; };
            '''))
            subprocess.run([
                'g++','-std=c++17','-fsyntax-only','-I',str(td),'-I',str(ROOT/'port'),
                str(ROOT/'port'/'comet_compat.cpp')
            ],check=True)

if __name__=='__main__': unittest.main()
