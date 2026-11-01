"""Generate exact GDB probe addresses from this ELF, not from another project."""
import json,subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[1];dest=root/'docs/metrics/lab3/diagnostic';text=subprocess.check_output(['arm-none-eabi-nm','-S',str(root/'firmware/build/firmware.elf')],text=True)
symbols={}
for line in text.splitlines():
 f=line.split()
 if len(f)==4:symbols[f[3]]=(int(f[0],16),int(f[1],16))
want=['uwTick','timer_high','instrument_use_wfi','instrument_timer_irqs']
result={n:symbols[n][0] for n in want}
base,size=symbols['shipment_flash_diag'];assert size==40
for i,n in enumerate(['stage','hal_status','hal_error','page_error','sr','cr','offset','unlock_status','program_status','erase_status']):result['flash_'+n]=base+i*4
base,size=symbols['meters'];assert size==9*24
for n,index in [('command_runs',0),('sample_runs',1),('loop_runs',6)]:result[n]=base+24*index
(dest/'symbols.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
