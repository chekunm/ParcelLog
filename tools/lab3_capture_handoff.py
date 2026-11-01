"""Start a second UART capture only after the first capture closes normally."""
import argparse
import os
from pathlib import Path
import subprocess
import sys
import time

p = argparse.ArgumentParser()
p.add_argument('--first-session', type=Path, required=True)
p.add_argument('--first-pid', type=int, required=True)
p.add_argument('--output', type=Path, required=True)
p.add_argument('--schedule', type=Path, required=True)
p.add_argument('--seconds', type=float, default=1200)
a = p.parse_args()
while not a.first_session.exists():
    try:
        os.kill(a.first_pid, 0)
    except ProcessLookupError:
        raise SystemExit('first capture exited without session metadata; no handoff')
    time.sleep(0.2)
subprocess.run([sys.executable, str(Path(__file__).with_name('lab3_pilot_capture.py')),
                '--output', str(a.output), '--schedule', str(a.schedule),
                '--seconds', str(a.seconds)], check=True)
