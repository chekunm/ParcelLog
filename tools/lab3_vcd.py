"""Analyse an actual Lab3 scalar 1ns VCD; keep raw edges and provenance."""
import argparse,bisect,hashlib,json,statistics
from pathlib import Path
from analyze_vcd import read_vcd,pulses,uart
p=argparse.ArgumentParser();p.add_argument('vcd',type=Path);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
t,last=read_vcd(a.vcd)
required=['SAMPLE','FLASH','UART_TX','PIR','ADC_MARK','RESPONSE','UART_RX','MODE']
if not set(required)<=set(t):raise SystemExit('Missing Lab3 channel names; do not reuse Lab2 captures')
def summary(values):
 if not values:return {'n':0}
 v=sorted(values);return {'n':len(v),'min_us':min(v)/1000,'avg_us':statistics.mean(v)/1000,'max_us':max(v)/1000,'p99_us':v[min(len(v)-1,int(len(v)*.99))]/1000}
result={'source':str(a.vcd),'sha256':hashlib.sha256(a.vcd.read_bytes()).hexdigest(),'duration_us':last/1000,'kind':'actual recorded simulator edges; not physical hardware','channels':{}}
for name in ['SAMPLE','FLASH','ADC_MARK','RESPONSE']:
 ps=pulses(t[name]);starts=[x['start_ns'] for x in ps]
 result['channels'][name]={'width':summary([x['width_ns'] for x in ps]),'period':summary([b-a for a,b in zip(starts,starts[1:])])}
responses=[time for time,value in zip(*t['RESPONSE']) if value]
flash=pulses(t['FLASH']);pairs=[]
for edge,value in zip(*t['PIR']):
 if not value or edge==0:continue
 i=bisect.bisect_left(responses,edge)
 if i==len(responses):continue
 response=responses[i]
 # Candidate association only. An unrelated event may raise RESPONSE first.
 # Verify nearby_motion in decoded UART and mode channel before accepting it.
 pairs.append({'edge_ns':edge,'response_ns':response,'candidate_latency_us':(response-edge)/1000,'inside_flash':any(x['start_ns']<=edge<x['start_ns']+x['width_ns'] for x in flash)})
result['pir_candidates']=pairs
result['warning']='PIR→next RESPONSE is only a candidate; accept after nearby_motion UART/mode review. ISR latency is excluded from firmware metric and included by LA.'
a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(result,indent=2)+'\n')
a.output.with_suffix('.wire-uart.json').write_text(json.dumps(uart(t['UART_TX']),indent=2)+'\n')
print(json.dumps(result,indent=2))
