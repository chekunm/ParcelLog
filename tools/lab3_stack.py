"""Compute the largest known-frame call chain, explicitly not a full stack bound."""
import json,re,subprocess
from pathlib import Path
root=Path(__file__).resolve().parents[1];out=root/'docs/metrics/lab3';m=json.loads((out/'memory.json').read_text())
frames={x['function'].rsplit(':',1)[-1]:x['bytes'] for x in m['frames']}
dis=subprocess.check_output(['arm-none-eabi-objdump','-d',str(root/'firmware/build/firmware.elf')],text=True)
edges={};name=None
for line in dis.splitlines():
 head=re.match(r'^\w+ <([^>]+)>:',line)
 if head:name=head[1];edges.setdefault(name,set())
 call=re.search(r'\bblx?\s+\w+ <([^>]+)>',line)
 if call and name:edges[name].add(call[1])
def known(name,seen):
 if name in seen:return (0,[name+' [recursive/unknown]'])
 children=[known(c,seen|{name}) for c in edges.get(name,()) if c in frames]
 best=max(children,default=(0,[]),key=lambda x:x[0]);return frames.get(name,0)+best[0],[name,*best[1]]
choices=[known(n,set()) for n in edges if n in frames];value,path=max(choices,key=lambda x:x[0])
unknown=sorted({c for n,cs in edges.items() if n in frames for c in cs if c not in frames})
report={'kind':'partial static call-chain estimate, not a whole-program upper bound','largest_known_chain_bytes':value,'chain':[{'function':n,'frame_bytes':frames.get(n)} for n in path],'calls_without_su':unknown,'exception_hardware_frame_bytes':32,'limits':'ELF calls may be tail-optimized; static .su lacks libc/assembly frames and indirect callback destinations. Full bound including IRQ nesting unknown. Compare with runtime painted reserve=2048 before certifying margin.'}
(out/'stack-static.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
