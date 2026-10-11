from pathlib import Path
import re
root=Path(__file__).resolve().parents[1]
for folder in ['App','Devices']:
 for p in (root/'firmware'/folder).glob('*.[ch]'):
  for target in re.findall(r'#include\s*[<"]([^>"]+)',p.read_text()):
   if 'stm32' in target.lower() or target in ['main.h','board.h','runtime.h']:
    raise SystemExit(f'Forbidden dependency: {p}: {target}')
print('Portable dependency rules passed')
