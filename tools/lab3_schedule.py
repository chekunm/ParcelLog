"""Prepare commands only. Creating a schedule does not run the MCU."""
import argparse,json
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('scenario',choices=['S1','S2','S3','uart-limit','fault']);p.add_argument('--output',type=Path,required=True);p.add_argument('--rate',type=int,default=10);a=p.parse_args()
items=[]
def send(t,s):items.append({'after_seconds':t,'text':s})
if a.scenario=='S1':send(305,'stats')
elif a.scenario=='S2':
 for t in range(15,3600,120):send(t,'mark')
 for t in range(60,3601,600):send(t,'stats')
 send(3605,'stats');send(3610,'log')
elif a.scenario=='S3':
 # On a disposable test log only. PB1/MPU/light stimuli must be applied in
 # Wokwi or by hardware; timestamps below are host scheduling, not MCU time.
 for t in range(5,305,10):
  send(t,'log');send(t+2,'erase CONFIRM');send(t+2.1,'mark')
 for n in range(3000):send(5+n/10,'status')
 send(310,'stats')
elif a.scenario=='fault':send(5,'imuaddr 0x69');send(12,'status');send(15,'imuaddr 0x68');send(25,'stats')
else:
 if not 1<=a.rate<=2000:raise SystemExit('rate must be 1..2000 commands/s')
 for n in range(a.rate*10):send(5+n/a.rate,'status')
 send(17,'stats')
items.sort(key=lambda x:x['after_seconds']);a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(items,indent=2)+'\n');print(f'{len(items)} commands saved, no run performed')
