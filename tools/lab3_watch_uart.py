"""Sample a growing UART log without interacting with the simulator."""
import argparse
import datetime
import json
import os
from pathlib import Path
import re
import time

p = argparse.ArgumentParser()
p.add_argument('--log', type=Path, required=True)
p.add_argument('--output', type=Path, required=True)
p.add_argument('--pid', type=int, required=True)
p.add_argument('--interval', type=float, default=30)
a = p.parse_args()
started = time.monotonic()
with a.output.open('x') as out:
    while True:
        try:
            os.kill(a.pid, 0)
            alive = True
        except ProcessLookupError:
            alive = False
        data = a.log.read_bytes() if a.log.exists() else b''
        values = re.findall(rb'tick \[(\d+) ms\]', data[-65536:])
        item = {'at': datetime.datetime.now().astimezone().isoformat(),
                'watch_elapsed_s': round(time.monotonic() - started, 3),
                'bytes': len(data), 'last_tick_ms': int(values[-1]) if values else None,
                'collector_alive': alive}
        out.write(json.dumps(item) + '\n')
        out.flush()
        if not alive:
            break
        time.sleep(a.interval)
