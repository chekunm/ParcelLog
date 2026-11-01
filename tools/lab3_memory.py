"""Run inside the firmware image after build; preserve raw binutils output."""
import json,subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[1];build=root/'firmware/build';dest=root/'docs/metrics/lab3'
def run(args):return subprocess.check_output(args,cwd=root,text=True)
elf=str(build/'firmware.elf')
sections=run(['arm-none-eabi-size','-A',elf]);symbols=run(['arm-none-eabi-nm','--size-sort','-S','--radix=d',elf])
frames=[]
for file in sorted(build.glob('*.su')):
 for line in file.read_text().splitlines():
  fields=line.split('\t')
  if len(fields)==3:
   frames.append({'function':fields[0],'bytes':int(fields[1]),'type':fields[2]})
frames.sort(key=lambda x:x['bytes'],reverse=True)
values={}
for line in sections.splitlines():
 f=line.split()
 if len(f)==3 and f[0].startswith('.') and f[1].isdigit():values[f[0]]=int(f[1])
size_fields=run(['arm-none-eabi-size',elf]).splitlines()[-1].split()
flash=int(size_fields[0])+int(size_fields[1])
ram=sum(values.get(n,0) for n in ['.data','.bss','._user_heap_stack'])
report={'kind':'static ARM ELF analysis, not runtime memory','sections':values,'code_flash_load_bytes':flash,'journal_reserved_bytes':4096,'code_region_bytes':28672,'code_flash_free_bytes':28672-flash,'static_data_bss_bytes':values.get('.data',0)+values.get('.bss',0),'heap_stack_reservation_bytes':values.get('._user_heap_stack',0),'linked_ram_bytes':ram,'ram_unreserved_bytes':12288-ram,'frames':frames,'runtime_stack_bytes':None,'runtime_heap_arena_bytes':None,'stack_limit_bytes':2048,'limits':'Function frames are not full call depth. Library assembly/libc frames, exception stacking and nesting are not certified by .su. Runtime watermark required.'}
(dest/'memory.json').write_text(json.dumps(report,indent=2)+'\n')
(dest/'memory.log').write_text(run(['arm-none-eabi-gcc','--version'])+run(['arm-none-eabi-size',elf])+sections+'\nLargest symbols, decimal bytes:\n'+'\n'.join(symbols.splitlines()[-25:])+'\n\nObject-file upper bounds before linker GC:\n'+run(['arm-none-eabi-size',*map(str,sorted(build.glob('*.o')))])+'\nFunction stack frames (largest first, includes unlinked vendor functions):\n'+json.dumps(frames[:25],indent=2)+'\n')
print(json.dumps({k:v for k,v in report.items() if k not in ('sections','frames')},indent=2))
