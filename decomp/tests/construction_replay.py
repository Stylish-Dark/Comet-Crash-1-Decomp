"""Headless scenarios must succeed or fail with a useful nonzero result."""
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

exe = Path(sys.argv[1]).resolve()
config = "world 16 1\nplayer 0 0 0 400 400 0\ntuning 20 100 50 10 20 7.5\nrun\n"

def run(text, args=()):
    return subprocess.run([str(exe), *args], input=text, text=True, capture_output=True)

result = run(config + "queue 0 20 2 2\nstep\nevent 0 2 0 20 3 3\nadvance 2\nexpect allocated 1\nexpect ready 1\nadvance 3\nexpect allocated 2\nexpect resource0 360\nexpect pending 0\nexpect ready 1\nadvance 5\nexpect ready 2\n")
assert result.returncode == 0, result.stderr
assert "clock 2 player0=idle" in result.stdout
assert "clock 3 player0=submitted" in result.stdout
for commands in ("expect allocated 99\n", "queue -1 20 2 2\n", "advance nan\n", "advance 2\nadvance 1\n", "queue 0 20 2 2 junk\n", "event 0 0 0 20 256 1\n"):
    result = run(config + commands)
    assert result.returncode != 0 and "line " in result.stderr, (commands, result.stdout, result.stderr)

factory_config = config.replace("tuning 20", "tuning 26")
result = run(factory_config + "queue 0 26 2 2\nstep\nexpect-mode 0 0\nfactory 0 8\nexpect-mode 0 4\nfactory 0 8\nexpect resource0 340\nadvance 2\nexpect ready 1\n")
assert result.returncode == 0, (result.stdout, result.stderr)
assert "factory changed" in result.stdout and "factory unchanged" in result.stdout
result = run(factory_config + "queue 0 26 2 2\nstep\nevent 0 1 8 26 2 2\nadvance 1\nexpect-mode 0 0\nadvance 2\nexpect-mode 0 4\nexpect events0 0\nexpect allocated 1\nexpect resource0 340\n")
assert result.returncode == 0, (result.stdout, result.stderr)
assert "clock 2 player0=submitted" in result.stdout
result = run(factory_config + "event 0 1 8 26 2 2\nadvance 2\nexpect events0 1\nexpect allocated 0\n")
assert result.returncode == 0 and "player0=retried" in result.stdout, (result.stdout, result.stderr)

for command in ("factory 0 3\n", "factory 999 4\n", "expect-mode 0 4\n", "factory 65536 4\n"):
    result = run(factory_config + "queue 0 26 2 2\nstep\n" + command)
    assert result.returncode != 0, (command, result.stdout, result.stderr)

production_config = factory_config.replace("run\n", "production 1\nunit-quota 0 0 1\nrun\n")
result = run(production_config + "queue 0 26 2 2\nstep\nadvance 2\nexpect unit-pending0 0\nadvance 4\nexpect unit-pending0 1\nexpect-unit 0 40 1\nexpect allocated 1\nexpect resource0 340\nadvance 8\nadvance 12\nexpect unit-pending0 1\n")
assert result.returncode == 0, (result.stdout, result.stderr)
for setting in ("unit-quota 0 2 1\n", "production 2\n", "unit-quota 1 0 1\n", "unit-quota 0 0 1\nunit-quota 0 0 2\n"):
    assert run(factory_config.replace("run\n", setting + "run\n")).returncode != 0

with tempfile.TemporaryDirectory() as directory:
    # Two source records: player start and already-settled structure.
    start = bytearray(56)
    start[:4] = bytes([10, 1, 0, 0])
    structure = bytearray(56)
    structure[:4] = bytes([20, 1, 2, 2])
    structure[16:20] = struct.pack(">f", 1)
    Path(directory, "level0.map").write_bytes(struct.pack(">II", 2, 0) + bytes(128) + start + structure)
    result = run(config + "expect allocated 1\nexpect display 1\nexpect ready 1\nqueue 0 20 3 3\nstep\nexpect allocated 2\nexpect display 2\nexpect resource0 380\nexpect ready 1\nadvance 2\nexpect ready 2\n", ["--map", directory, "--level", "0"])
    assert result.returncode == 0, (result.stdout, result.stderr)
    assert "live=1 display-only=0" in result.stdout

result = run(production_config + "queue 0 26 2 2\nstep\nexpect-factory-unit 0 40 2\nfactory 0 8\nexpect-factory-unit 0 44 6\nexpect allocated 1\nexpect unit-pending0 0\nexpect resource0 340\n")
assert result.returncode == 0, result.stderr
assert run(production_config + "expect-factory-unit 65535 40 2\n").returncode != 0

result = run(production_config + "queue 0 26 2 2\nstep\nexpect-factory-ground-unit 0 255 no-target\nexpect allocated 1\nexpect unit-pending0 0\nexpect resource0 340\n")
assert result.returncode == 0, result.stderr
assert run(production_config + "expect-factory-ground-unit 65535 255 no-target\n").returncode != 0
binding_config = factory_config.replace("tuning 26", "player 23 23 1 400 400 0\nunit-base 0 1\nunit-base 1 1\ntuning 26")
result = run(binding_config + "queue 0 26 2 2\nstep\nexpect-factory-ground-unit 0 1 blocked\nexpect allocated 1\nexpect resource0 340\n")
assert result.returncode == 0, result.stderr
for setting in ("unit-base 0 2\n", "unit-base 1 1\n", "unit-base 0 1\nunit-base 0 1\n", "unit-target 0 0\n", "unit-target 0 1\n"):
    assert run(factory_config.replace("run\n", setting + "run\n")).returncode != 0
override_config = binding_config.replace("player 23 23 1", "player 23 23 0").replace("run\n", "unit-target 0 1\nrun\n")
result = run(override_config + "queue 0 26 2 2\nstep\nexpect-factory-ground-unit 0 1 bound\nexpect resource0 340\n")
assert result.returncode == 0, result.stderr
assert run(override_config + "queue 0 26 2 2\nstep\nfactory 0 6\nexpect-factory-ground-unit 0 1 bound\n").returncode != 0
print("headless construction scenarios passed")

spawn_config = production_config
result = run(spawn_config + "queue 0 26 2 2\nstep\nadvance 2\nadvance 4\nspawn 0 40 3 3\nexpect unit-active0 1\nexpect unit-pending0 0\nexpect live-units 1\nexpect allocated 2\nexpect-live-unit 1 40 255\nadvance 5\nexpect ready 1\nexpect resource0 340\n")
assert result.returncode == 0, (result.stdout, result.stderr)
assert "spawn spawned entity=1" in result.stdout
full_config = spawn_config.replace("world 16", "world 1")
result = run(full_config + "queue 0 26 2 2\nstep\nadvance 2\nadvance 4\nspawn 0 40 3 3\nexpect unit-active0 0\nexpect unit-pending0 0\nexpect live-units 0\nexpect allocated 1\n")
assert result.returncode == 0 and "spawn allocation-failed" in result.stdout, (result.stdout, result.stderr)
for command in ("spawn 0 40 24 0\n", "spawn 9 40 1 1\n", "spawn 0 42 1 1\n", "spawn 0 40 -1 1\n", "expect-live-unit 65535 40 255\n"):
    result=run(spawn_config+command)
    assert result.returncode!=0 and "line " in result.stderr,(command,result.stdout,result.stderr)

transport_scenario = Path(__file__).resolve().parent.parent / "examples/ground-transport.scenario"
result = run(transport_scenario.read_text())
assert result.returncode == 0, (result.stdout, result.stderr)
assert result.stdout.count("unit-transport entity=1") == 2
for command in ("transport 2\n", "advance 20\n", "expect-unit-cell 65535 0 0\n"):
    result = run(transport_scenario.read_text() + "transport 1\n" + command)
    assert result.returncode != 0 and "line " in result.stderr
for setting in ("transport 2\n", "transport 1\ntransport 1\n"):
    assert run(config.replace("run\n", setting + "run\n")).returncode != 0

steering_scenario = Path(__file__).resolve().parent.parent / "examples/ground-steering.scenario"
result = run(steering_scenario.read_text())
assert result.returncode == 0, (result.stdout, result.stderr)

for command in ("expect-unit-near 1 0 0 0.01\n", "expect-unit-near 1 nan 0 1\n", "expect-unit-near 1 10.5 10.5 -1\n", "expect-unit-near 65535 0 0 1\n"):
    assert run(steering_scenario.read_text()+command).returncode != 0
# Dynamics is independently selectable before/after run; malformed flags fail.
result = run(production_config.replace('run\n', 'dynamics 1\nrun\n') + 'dynamics 0\ndynamics 1\n')
assert result.returncode == 0, result.stderr
assert 'dynamics 1' in result.stdout
for setting in ('dynamics 2\n','dynamics -1\n','dynamics 1\ndynamics 1\n'):
    assert run(production_config.replace('run\n', setting+'run\n')).returncode != 0
assert run(production_config+'dynamics 2\n').returncode != 0

dynamics_scenario = Path(__file__).resolve().parent.parent / "examples/ground-dynamics.scenario"
result = run(dynamics_scenario.read_text())
assert result.returncode == 0, (result.stdout,result.stderr)
assert result.stdout.count("unit-transport entity=1") == 3

result=run(production_config.replace("run\n","contacts 1\nrun\n")+"contacts 0\ncontacts 1\n")
assert result.returncode==0,result.stderr
for setting in ("contacts 2\n","contacts 1\ncontacts 1\n"):
    assert run(production_config.replace("run\n",setting+"run\n")).returncode!=0
assert run(production_config+"contacts 2\n").returncode!=0

contact_scenario=Path(__file__).resolve().parent.parent/"examples/ground-contacts.scenario"
result=run(contact_scenario.read_text());assert result.returncode==0,(result.stdout,result.stderr)
for command in ("expect-unit-separated 1 1 .1\n","expect-unit-separated 1 999 .1\n","expect-unit-separated 1 2 nan\n","expect-unit-separated 1 2 -1\n"):
    assert run(contact_scenario.read_text()+command).returncode!=0

# The same scene without contacts retains overlap; this distinguishes the feature.
result=run(contact_scenario.read_text().replace("contacts 1","dynamics 1\ncontacts 0"))
assert result.returncode!=0 and "unit separation expectation failed" in result.stderr

# Explicit readiness seed; original capture transaction changes counts/owner.
capture_config = "world 8 4\nplayer 0 0 0 400 400 0\nplayer 10 10 1 400 400 0\ntuning 26 100 50 10 20 7.5\ntuning 28 100 50 10 20 7.5\nproduction 1\nunit-base 0 1\nunit-base 1 1\nunit-quota 0 0 2\nunit-quota 1 0 4\ncontacts 1\nrun\n"
capture_commands = "queue 0 26 2 2\nstep\nqueue 1 28 8 8\nstep\nfactory 1 5\nadvance 26\nspawn 0 40 3 3\nspawn 1 45 3 3\ncapture-ready 3 1\ntransport 1\nadvance 26.01\nexpect-unit-owner 2 1\nexpect unit-active0 0\nexpect unit-active1 2\nexpect allocated 4\n"
result=run(capture_config+capture_commands)
assert result.returncode==0,(result.stdout,result.stderr)

for command in ("capture-ready 2 1\n", "capture-ready 3 2\n", "capture-ready 65536 1\n", "expect-unit-owner 2 7\n", "expect-unit-owner 999 0\n"):
 result=run(capture_config+capture_commands.split("capture-ready 3 1")[0]+command)
 assert result.returncode!=0 and "line " in result.stderr,(command,result.stdout,result.stderr)
