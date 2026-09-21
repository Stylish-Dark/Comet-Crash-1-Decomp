import sys, unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))

class SelfDecryptTests(unittest.TestCase):
    def test_free_npdrm_metadata_layer_matches_known_vector(self):
        import decrypt_comet_self as d
        encrypted=bytes.fromhex(
            '14272b2b84738d6e9e5e7a9efc119f79'
            '1e5c01f7734a9c68c2240a73cc0ef379'
            'a1a8e82d90e1cecf27281c779f32d8e7'
            'b9c19ae02e7f8238b1b0eea6e239687a')
        expected=(bytes.fromhex('00112233445566778899aabbccddeeff') + b'\0'*16 +
                  bytes.fromhex('ffeeddccbbaa99887766554433221100') + b'\0'*16)
        self.assertEqual(d.decrypt_metadata_info(encrypted), expected)

if __name__=='__main__': unittest.main()
