from pathlib import Path
import re,json,hashlib
root=Path(__file__).resolve().parents[1]
for folder in ['App','Devices']:
 for p in (root/'firmware'/folder).glob('*.[ch]'):
  for target in re.findall(r'#include\s*[<"]([^>"]+)',p.read_text()):
   if 'stm32' in target.lower() or target in ['main.h','board.h','runtime.h']:
    raise SystemExit(f'Forbidden dependency: {p}: {target}')
print('Portable dependency rules passed')

manifest=json.loads((root/'docs/lab2/validation/vendor-sha256.json').read_text())
for name,expected in manifest['files'].items():
 if hashlib.sha256((root/name).read_bytes()).hexdigest()!=expected:
  raise SystemExit('Vendor source changed: '+name)
print('Vendored HAL/CMSIS SHA256 passed')
