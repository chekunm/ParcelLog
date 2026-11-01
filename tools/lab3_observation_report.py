"""Summarize passive wall-clock snapshots; does not assert MCU timing accuracy."""
import argparse
import json
from pathlib import Path
import statistics

p = argparse.ArgumentParser()
p.add_argument('observations', type=Path)
p.add_argument('--output', type=Path, required=True)
a = p.parse_args()
rows = [json.loads(line) for line in a.observations.read_text().splitlines() if line.strip()]
intervals = []
for before, after in zip(rows, rows[1:]):
    if before['last_tick_ms'] is None or after['last_tick_ms'] is None:
        continue
    intervals.append({'wall_s': round(after['watch_elapsed_s'] - before['watch_elapsed_s'], 3),
                      'mcu_tick_ms': after['last_tick_ms'] - before['last_tick_ms'],
                      'uart_bytes': after['bytes'] - before['bytes']})
advances = [row['mcu_tick_ms'] for row in intervals]
result = {'samples': len(rows), 'first': rows[0] if rows else None,
          'last': rows[-1] if rows else None,
          'intervals': intervals,
          'tick_delta_ms': {'min': min(advances), 'max': max(advances),
                            'mean': statistics.mean(advances)} if advances else None,
          'nonadvancing_intervals': sum(value <= 0 for value in advances),
          'note': 'Snapshots describe last UART lines seen, including buffering and sampling granularity; use raw session and VCD for acceptance.'}
a.output.write_text(json.dumps(result, indent=2) + '\n')
print(json.dumps({key: value for key, value in result.items() if key != 'intervals'}, indent=2))
