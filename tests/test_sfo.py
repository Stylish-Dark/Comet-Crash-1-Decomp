import struct,sys,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from sfo import parse_sfo_bytes
class T(unittest.TestCase):
 def test_rejects_bad(self):
  with self.assertRaises(ValueError): parse_sfo_bytes(b'bad')
 def test_string(self):
  key=b'TITLE_ID\0'; val=b'NPEB00142\0'; kb=0x24; db=kb+len(key); b=bytearray(db+len(val)); b[:4]=b'\0PSF'; struct.pack_into('<III',b,8,kb,db,1); struct.pack_into('<HHIII',b,0x14,0,0x0204,len(val),len(val),0); b[kb:kb+len(key)]=key;b[db:db+len(val)]=val; self.assertEqual(parse_sfo_bytes(bytes(b))['TITLE_ID'],'NPEB00142')
