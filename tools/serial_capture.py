#!/usr/bin/env python3
"""Capture RFC2217 bytes and optional host-clock command events for ParcelLog."""
import argparse
from datetime import datetime
import json
from pathlib import Path
import time

import serial  # pyserial 3.5, BSD-3-Clause: https://github.com/pyserial/pyserial


def load_schedule(path):
    entries = json.loads(path.read_text()) if path else []
    if not isinstance(entries, list):
        raise ValueError('schedule must be a list')
    previous = -1.0
    for item in entries:
        if not isinstance(item, dict) or not isinstance(item.get('text'), str):
            raise ValueError('each scheduled command needs text and after_seconds')
        when = item.get('after_seconds')
        if isinstance(when, bool) or not isinstance(when, (int, float)) or not 0 <= when < float('inf') or when < previous:
            raise ValueError('command times must be finite, nonnegative and ordered')
        if '\n' in item['text'] or '\r' in item['text']:
            raise ValueError('command text must be a single line')
        previous = when
    return entries


def capture(port, output, seconds, schedule):
    if not 0 < seconds < float('inf'):
        raise ValueError('duration must be finite and positive')
    output.parent.mkdir(parents=True, exist_ok=True)
    session_path = output.with_suffix('.session.json')
    metadata = {'started_at': datetime.now().astimezone().isoformat(), 'port': port,
                'state': 'recording', 'commands': [], 'uart_bytes': 0,
                'clock': 'Host monotonic schedule; UART time_ms comes from the simulated MCU'}
    def save():
        session_path.write_text(json.dumps(metadata, indent=2) + '\n')
    save()
    started = None
    try:
        with output.open('wb') as stream, serial.serial_for_url(
                f'rfc2217://localhost:{port}', baudrate=115200, timeout=0.05) as connection:
            started = time.monotonic()
            index = 0
            while True:
                elapsed = time.monotonic() - started
                if elapsed >= seconds:
                    break
                while index < len(schedule) and elapsed >= schedule[index]['after_seconds']:
                    text = schedule[index]['text']
                    payload = (text + '\n').encode('utf-8')
                    if connection.write(payload) != len(payload):
                        raise OSError('incomplete UART command write')
                    metadata['commands'].append({'wall_seconds': elapsed, 'command': text})
                    index += 1
                incoming = connection.read(connection.in_waiting or 1)
                if incoming:
                    stream.write(incoming)
                    stream.flush()
                    metadata['uart_bytes'] += len(incoming)
        metadata.update(state='completed', end_reason='duration_elapsed')
    except KeyboardInterrupt:
        metadata.update(state='interrupted', end_reason='keyboard_interrupt')
        raise
    except Exception as error:
        metadata.update(state='failed', end_reason=f'{type(error).__name__}: {error}')
        raise
    finally:
        metadata['duration_seconds'] = 0 if started is None else time.monotonic() - started
        save()
    return metadata


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port', type=int, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--seconds', type=float, default=65)
    parser.add_argument('--schedule', type=Path)
    args = parser.parse_args()
    result = capture(args.port, args.output, args.seconds, load_schedule(args.schedule))
    print(f"Captured {result['uart_bytes']} bytes")


if __name__ == '__main__':
    main()
