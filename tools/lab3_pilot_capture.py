"""Capture a real Wokwi UART capability pilot, with bounded connect retries."""
from pathlib import Path
import argparse,datetime,hashlib,json,time,serial
p=argparse.ArgumentParser();p.add_argument('--output',type=Path,required=True);p.add_argument('--seconds',type=float,default=65);p.add_argument('--profile',choices=['frozen','diagnostic'],default='frozen');p.add_argument('--schedule',type=Path);a=p.parse_args()
schedule=[(2,'status'),(3,'mark'),(4,'log'),(6,'stats'),(10,'imuaddr 0x69'),(14,'imuaddr 0x68'),(16,'status'),(25,'stats'),(30,'erase CONFIRM'),(35,'mark'),(36,'log'),(40,'stats'),(55,'stats')]
if a.profile=='diagnostic':schedule=[(.5,'status'),(1,'mark'),(1.5,'log'),(2,'erase CONFIRM'),(3,'mark'),(3.5,'log'),(4,'stats'),(7,'stats')]
if a.schedule:
 schedule=[(item['after_seconds'],item['text']) for item in json.loads(a.schedule.read_text())]
 if schedule!=sorted(schedule,key=lambda item:item[0]):raise ValueError('schedule must be ordered')
started=datetime.datetime.now().astimezone().isoformat();a.output.parent.mkdir(parents=True,exist_ok=True);deadline=time.monotonic()+90
while True:
 try:c=serial.serial_for_url('rfc2217://localhost:4505',baudrate=115200,timeout=.05);break
 except (OSError,serial.SerialException):
  if time.monotonic()>deadline:raise
  time.sleep(.1)
print('connected',flush=True);epoch=time.monotonic();commands=[];raw=bytearray();reason='duration_elapsed'
try:
 with c, a.output.open('wb') as out:
  while time.monotonic()-epoch<a.seconds:
   elapsed=time.monotonic()-epoch
   if schedule and elapsed>=schedule[0][0]:
    due,text=schedule.pop(0);c.write((text+'\n').encode());commands.append({'elapsed_wall_s':elapsed,'text':text});print('sent',text,flush=True)
   try:b=c.read(c.in_waiting or 1)
   except serial.SerialException as e:reason=str(e);break
   if b:out.write(b);out.flush();raw.extend(b)
except KeyboardInterrupt:
 reason='interrupted_after_capture'
meta={'started_at':started,'duration_seconds':time.monotonic()-epoch,'end_reason':reason,'commands':commands,'bytes':len(raw),'sha256':hashlib.sha256(raw).hexdigest(),'source':'Actual RFC2217 bytes from ParcelLog Wokwi at localhost:4505; host schedule is not MCU time'}
a.output.with_suffix('.session.json').write_text(json.dumps(meta,indent=2)+'\n');print(meta,flush=True)
