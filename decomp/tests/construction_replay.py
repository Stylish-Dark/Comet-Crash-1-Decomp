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
for command in ("factory 0 3\n", "factory 999 4\n", "expect-mode 0 4\n", "factory 65536 4\n"):
    result = run(factory_config + "queue 0 26 2 2\nstep\n" + command)
    assert result.returncode != 0, (command, result.stdout, result.stderr)

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
print("headless construction scenarios passed")
