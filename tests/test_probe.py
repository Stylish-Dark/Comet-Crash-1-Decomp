import struct,sys,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
import ps3_probe as p
class T(unittest.TestCase):
 def test_header_rejects_junk(self):
  with self.assertRaises(ValueError): p.parse_elf64_header(b'nope')
 def test_header_parses_ppc64(self):
  b=bytearray(64); b[:6]=b'\x7fELF\x02\x02'; struct.pack_into('>H',b,18,21); struct.pack_into('>Q',b,24,0x1234); struct.pack_into('>H',b,54,56)
  h=p.parse_elf64_header(bytes(b)); self.assertEqual(h['machine'],21); self.assertEqual(h['entry'],0x1234)
 def test_spu_scan(self):
  b=bytearray(256); o=32; b[o:o+6]=b'\x7fELF\x01\x02'; struct.pack_into('>H',b,o+18,23); struct.pack_into('>I',b,o+24,0x80); struct.pack_into('>I',b,o+28,52); struct.pack_into('>H',b,o+42,32); struct.pack_into('>H',b,o+44,0)
  x=p.scan_spu_images(bytes(b),[]); self.assertEqual(len(x),1); self.assertEqual(x[0]['entry'],'0x00000080')

 def test_parse_opd_reads_ps3_32bit_opd_pairs(self):
  b=bytearray(0x400)
  # Code and .opd live in separate PT_LOADs. PS3 PPU OPDs are 8-byte
  # (u32 code, u32 TOC) pairs even though the container is ELF64.
  ph=[{'type':p.PT_LOAD,'flags':5,'offset':0x000,'vaddr':0x10000,'filesz':0x100,'memsz':0x100},
      {'type':p.PT_LOAD,'flags':6,'offset':0x200,'vaddr':0x230000,'filesz':0x200,'memsz':0x200}]
  # 40 descriptors so the old density threshold cannot accidentally hide it.
  for i in range(40):
   struct.pack_into('>II',b,0x220+i*8,0x10000+(i%32)*4,0x230080)
  r=p.parse_opd(bytes(b),ph)
  self.assertEqual(r['file_offset'],'0x00000220')
  self.assertEqual(r['descriptor_count'],40)
  self.assertEqual(r['unique_function_count'],32)
  self.assertEqual(r['module_toc'],'0x00230080')

def _put_cstr(b, off, s):
 b[off:off+len(s)+1]=s.encode('ascii')+b'\0'

class ImportProbeTests(unittest.TestCase):
 def test_parse_imports_reads_library_nid_and_stub(self):
  b=bytearray(0x500)
  ph=[{'type':p.PT_LOAD,'flags':5,'offset':0,'vaddr':0x1000,'filesz':len(b),'memsz':len(b)},
      {'type':p.PT_PRX_STUB,'flags':0,'offset':0,'vaddr':0x1100,'filesz':0,'memsz':0}]
  # The PT_PRX_STUB points to a 0x20-byte header whose last two words bound
  # the module-stub descriptor table.
  struct.pack_into('>II',b,0x100+24,0x1200,0x1220)
  d=0x200
  b[d]=0x20
  struct.pack_into('>H',b,d+6,1)
  struct.pack_into('>III',b,d+0x10,0x1300,0x1320,0x1340)
  _put_cstr(b,0x300,'sys_io')
  struct.pack_into('>I',b,0x320,0x8B72CDA1)
  struct.pack_into('>I',b,0x340,0x1500)
  imports=p.parse_imports(bytes(b),ph)
  self.assertEqual(imports,[{'library':'sys_io','nid':'0x8B72CDA1','stub':'0x00001500'}])

 def test_direct_branch_callsites_find_bl_to_import_stub(self):
  b=bytearray(0x200)
  ph=[{'type':p.PT_LOAD,'flags':5,'offset':0,'vaddr':0x1000,'filesz':len(b),'memsz':len(b)}]
  pc=0x1040; target=0x1100
  disp=(target-pc)&0x03FFFFFC
  struct.pack_into('>I',b,0x40,0x48000001|disp)
  found=p.scan_direct_branch_calls(bytes(b),ph,{target})
  self.assertEqual(found,{target:[pc]})
