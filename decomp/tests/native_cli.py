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

# Native skirmish bootstraps the same arena/simulation without an original map.
with tempfile.TemporaryDirectory() as d:
    cfg=Path(d)/'skirmish.cfg'
    text=('world 32 4\nplayer 2 12 0 400 400 0\nplayer 21 12 1 400 400 0\n'
          'tuning 26 100 50 10 20 7.5\nproduction 1\ntransport 1\n'
          'unit-base 0 1\nunit-base 1 1\nunit-quota 0 0 4\nunit-quota 1 0 4\n'
          'base-counter 0 5\nbase-counter 1 5\nopponent 1\nrun\n')
    cfg.write_text(text)
    r=subprocess.run([str(exe),'--skirmish-config',str(cfg)],capture_output=True,text=True)
    assert r.returncode==0 and 'Native skirmish bootstrap' in r.stdout,(r.returncode,r.stdout,r.stderr)
    assert 'Arena entities: 2' in r.stdout and 'display-only: 2' in r.stdout,r.stdout
    for extra in ([str(Path(d))], ['--construction-config',str(cfg)], ['--object','triangle.obj']):
        r=subprocess.run([str(exe),'--skirmish-config',str(cfg),*extra],capture_output=True,text=True)
        assert r.returncode==2,(extra,r.returncode,r.stderr)
    cfg.write_text(text+'advance 1\n')
    r=subprocess.run([str(exe),'--skirmish-config',str(cfg)],capture_output=True,text=True)
    assert r.returncode==1 and 'scenario commands' in r.stderr,(r.returncode,r.stderr)

print('native CLI checks passed')
