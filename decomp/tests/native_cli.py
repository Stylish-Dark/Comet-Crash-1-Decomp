import subprocess,sys,tempfile,struct
from pathlib import Path
exe=Path(sys.argv[1])
for args in [[],['--bogus'],['--assets'],['--frames','-1'],['--viewer']]:
    result=subprocess.run([str(exe),*args],capture_output=True,text=True)
    assert result.returncode==2,(args,result.returncode,result.stdout,result.stderr)
with tempfile.TemporaryDirectory() as d:
    root=Path(d);(root/'level0.map').write_bytes(struct.pack('>II',0,0)+bytes(128))
    r=subprocess.run([str(exe),str(root),'0'],capture_output=True,text=True)
    assert r.returncode==0 and 'Extent: 24' in r.stdout,(r.stdout,r.stderr)
    r=subprocess.run([str(exe),str(root),'invalid'],capture_output=True,text=True)
    assert r.returncode==2
    r=subprocess.run([str(exe),'--assets',str(root),'--validate-assets'],capture_output=True,text=True)
    assert r.returncode==1 and 'cannot open model' in r.stderr,(r.returncode,r.stderr)
    (root/'triangle.obj').write_text('v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 3\n')
    r=subprocess.run([str(exe),'--assets',str(root),'--object','triangle.obj'],capture_output=True,text=True)
    assert r.returncode==0 and '1 triangles' in r.stdout,(r.stdout,r.stderr)
print('native CLI checks passed')
