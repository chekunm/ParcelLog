"""Parse actual MCU stats; never infer timing from host arrival time."""
import argparse,hashlib,json,re
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--scenario',choices=['S1','S2','S3']);p.add_argument('log',type=Path);p.add_argument('--output',type=Path,required=True);p.add_argument('--identity-log',type=Path);p.add_argument('--wall-session',type=Path);a=p.parse_args()
s=a.log.read_text(errors='replace')
if 'HOST ' in s:raise SystemExit('Host functional output is not MCU measurement data')
identity=a.identity_log.read_text(errors='replace') if a.identity_log else s
if 'HOST ' in identity:raise SystemExit('Host functional output is not MCU measurement data')
versions=re.findall(r'BUILD git=(\S+) source=(\S+)',identity)
if not versions:raise SystemExit('Missing BUILD banner; preserve startup bytes and retry capture')
rows=[];memory=[]
for line in s.splitlines():
 if line.startswith('STAT ') or line.startswith('METRICS '):
  row=dict(re.findall(r'(\w+)=([^\s]+)',line))
  for k,v in list(row.items()):
   if re.fullmatch(r'-?\d+',v):row[k]=int(v)
  if line.startswith('STAT '):rows.append(row)
  else:
   elapsed=(row['elapsed_hi']<<32)+row['elapsed_lo'];sleep=(row['sleep_hi']<<32)+row['sleep_lo']
   row['elapsed_us']=elapsed;row['sleep_us']=sleep
   if row.get('clock')=='TIM3_1MHz':
    row['active_cpu_estimate_pct']=round(100*(elapsed-sleep)/elapsed,3) if elapsed else None
   else:
    row['active_cpu_estimate_pct']=None
    row['cpu_limit']='SysTick quantization and WFI state do not support a CPU estimate from this record'
   memory.append(row)
if not rows or not memory:raise SystemExit('No complete stats dump; capture stats through STAT task=pir_latency')
result={'kind':'MCU/Wokwi UART measurement; environment must be set in origin manifest','source_log':str(a.log),'sha256':hashlib.sha256(a.log.read_bytes()).hexdigest(),'builds':versions,'stats':rows,'resources':memory,'limits':'Check clock per METRICS: SysTick_1ms has 1 ms resolution; a zero interval is not WCET. TIM3_1MHz is nominal 1 us and must be validated. Aggregate maxima are not a hard WCET proof; CPU estimates exclude part of exception overhead; command latency ends before wire TX; PIR starts at ISR, not physical edge'}
if a.identity_log:
 result['identity_log']={'path':str(a.identity_log),'sha256':hashlib.sha256(a.identity_log.read_bytes()).hexdigest()}
 result['identity_limit']='Banner comes from a separate capture segment; same uninterrupted run must be verified independently.'
if a.scenario:
 latest={r['task']:r for r in rows}
 resources=memory[-1];target=36000 if a.scenario=='S2' else 3000
 checks={'periodic_count':latest.get('sample',{}).get('n',0)>=target}
 if a.scenario=='S2':
  checks['mcu_elapsed_1h']=resources['elapsed_us']>=3600000000
  session=a.wall_session or a.log.with_suffix('.session.json')
  checks['wall_elapsed_1h']=session.exists() and json.loads(session.read_text()).get('duration_seconds',0)>=3600
  result['wall_session']=str(session)
 if a.scenario=='S3':
  checks['pir_event_count']=latest.get('pir_latency',{}).get('n',0)>=30
  checks['pir_firmware_max_120ms']=latest.get('pir_latency',{}).get('max_us',2**32)<=120000
  checks['command_max_20ms']=latest.get('command_latency',{}).get('max_us',2**32)<=20000
 result['scenario']=a.scenario;result['acceptance_checks']=checks
 result['acceptance_limit']='UART checks alone do not validate the physical GPIO latency, during-flash coincidence, power cut or two-method comparison.'
a.output.write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
