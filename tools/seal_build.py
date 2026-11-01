"""Bind UART banner to exact own sources, vendor manifest and build settings."""
from pathlib import Path
import hashlib, json, subprocess
root=Path(__file__).resolve().parents[1]
files=sorted(p for folder in ('firmware/App','firmware/Devices','firmware/Platform','firmware/Core') for p in (root/folder).rglob('*') if p.is_file() and p.suffix in ('.c','.h','.s') and p.name!='build_version.h')
files += [root/n for n in ('firmware/Makefile','firmware/STM32C031C6Tx_FLASH.ld','firmware/startup_stm32c031xx.s','docs/lab2/validation/vendor-sha256.json','diagram.json','wokwi.toml','docker/Dockerfile')]
vendor=json.loads((root/'docs/lab2/validation/vendor-sha256.json').read_text())['files']
files += [root/name for name in vendor]
manifest={str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest() for p in files}
digest=hashlib.sha256(json.dumps(manifest,sort_keys=True).encode()).hexdigest()
head=subprocess.check_output(['git','rev-parse','HEAD'],cwd=root,text=True).strip()
header=root/'firmware/Platform/build_version.h'
text=f'#ifndef BUILD_VERSION_H\n#define BUILD_VERSION_H\n#define BUILD_GIT "{head[:12]}+lab3-worktree"\n#define BUILD_SOURCE "{digest[:16]}"\n#endif\n'
if not header.exists() or header.read_text()!=text: header.write_text(text)
out=root/'docs/metrics/lab3/source-manifest.json'
out.parent.mkdir(parents=True,exist_ok=True)
out.write_text(json.dumps({'head':head,'source_sha256':digest,'files':manifest},ensure_ascii=False,indent=2)+'\n')
print(f'ParcelLog source={digest} head={head} (uncommitted worktree)')
