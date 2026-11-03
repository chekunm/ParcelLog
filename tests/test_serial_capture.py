import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

spec = importlib.util.spec_from_file_location('parcel_capture', Path(__file__).resolve().parents[1] / 'tools/serial_capture.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

class Connection:
    in_waiting = 0
    def __init__(self): self.sent = []; self.reads = 0
    def __enter__(self): return self
    def __exit__(self, *args): return False
    def write(self, data): self.sent.append(data); return len(data)
    def read(self, size):
        self.reads += 1
        return b'raw\x00uart\n' if self.reads == 1 else b''

class CaptureTests(unittest.TestCase):
    def test_capture_records_raw_bytes_and_host_schedule(self):
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / 'uart.log'; conn = Connection()
            ticks = iter([10.0, 10.0, 10.2, 10.6, 10.6])
            with patch.object(module.serial, 'serial_for_url', return_value=conn) as factory, patch.object(module.time, 'monotonic', side_effect=lambda: next(ticks)):
                result = module.capture(4506, output, .5, [{'after_seconds': 0, 'text': 'status'}])
            self.assertNotIn('write_timeout', factory.call_args.kwargs)
            self.assertEqual(output.read_bytes(), b'raw\x00uart\n')
            self.assertEqual(conn.sent, [b'status\n'])
            self.assertEqual(result['state'], 'completed')
            self.assertEqual(json.loads(output.with_suffix('.session.json').read_text())['uart_bytes'], 9)
    def test_open_failure_is_not_left_recording(self):
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / 'uart.log'
            with patch.object(module.serial, 'serial_for_url', side_effect=module.serial.SerialException('unavailable')):
                with self.assertRaises(module.serial.SerialException): module.capture(4506, output, 1, [])
            self.assertEqual(json.loads(output.with_suffix('.session.json').read_text())['state'], 'failed')
    def test_interruption_and_partial_write_are_recorded(self):
        for failure, expected in ((KeyboardInterrupt(), 'interrupted'), (OSError('incomplete'), 'failed')):
            with self.subTest(expected=expected), tempfile.TemporaryDirectory() as directory:
                output = Path(directory) / 'uart.log'
                conn = Connection()
                conn.write = lambda data: (_ for _ in ()).throw(failure)
                with patch.object(module.serial, 'serial_for_url', return_value=conn):
                    with self.assertRaises(type(failure)):
                        module.capture(4506, output, 1, [{'after_seconds': 0, 'text': 'status'}])
                self.assertEqual(json.loads(output.with_suffix('.session.json').read_text())['state'], expected)
    def test_parcel_schedule_contract(self):
        # The collector emits one newline per command and uses host-monotonic
        # offsets; invalid timing or embedded line breaks must fail before I/O.
        valid = [{'after_seconds': 0, 'text': 'status'},
                 {'after_seconds': 0, 'text': 'mark'},
                 {'after_seconds': 2.5, 'text': 'log'}]
        invalid = [None, {}, [{'after_seconds': True, 'text': 'mark'}],
                   [{'after_seconds': -1, 'text': 'mark'}],
                   [{'after_seconds': float('nan'), 'text': 'mark'}],
                   [{'after_seconds': float('inf'), 'text': 'mark'}],
                   [{'after_seconds': 1, 'text': 'status\rmark'}],
                   [{'after_seconds': 1, 'text': 'status\nmark'}],
                   [{'after_seconds': 2, 'text': 'log'}, {'after_seconds': 1, 'text': 'mark'}]]
        with patch.object(Path, 'read_text', return_value=json.dumps(valid)):
            self.assertEqual(module.load_schedule(Path('schedule.json')), valid)
        with patch.object(module.serial, 'serial_for_url') as connection:
            for item in invalid:
                with self.subTest(schedule=item), patch.object(Path, 'read_text', return_value=json.dumps(item)):
                    with self.assertRaises(ValueError):
                        module.load_schedule(Path('schedule.json'))
            connection.assert_not_called()

if __name__ == '__main__': unittest.main()
