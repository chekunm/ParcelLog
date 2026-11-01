"""Read actual paused simulated MCU state through standard GDB remote protocol.

Run only while this project owns its GUI slot. No MCU values are synthesized.
The optional WFI write is an explicit diagnostic experiment on this simulation.
"""
from pathlib import Path
import argparse,datetime,hashlib,json,socket,struct,time,zlib

class RSP:
    def __init__(self,port):
        self.s=socket.create_connection(('127.0.0.1',port),timeout=5);self.s.settimeout(10)
        self.transcript=[]
    def packet(self):
        while True:
            start=self.s.recv(1)
            if not start:raise EOFError('GDB disconnected before packet')
            if start==b'$':break
        data=bytearray()
        while True:
            c=self.s.recv(1)
            if not c:raise EOFError('GDB disconnected')
            if c==b'#':break
            data+=c
        checksum=b''
        while len(checksum)<2:
            part=self.s.recv(2-len(checksum))
            if not part:raise EOFError('GDB disconnected in checksum')
            checksum+=part
        if len(checksum)!=2 or int(checksum,16)!=sum(data)%256:raise ValueError('GDB checksum mismatch')
        self.s.sendall(b'+');reply=data.decode('ascii');self.transcript.append({'rx':reply});return reply
    def send(self,text,reply=True):
        b=text.encode('ascii');self.transcript.append({'tx':text});self.s.sendall(b'$'+b+b'#'+f'{sum(b)%256:02x}'.encode())
        return self.packet() if reply else None
    def stop(self):self.s.sendall(b'\x03');return self.packet()
    def read(self,address,n):
        result=self.send(f'm{address:x},{n:x}')
        if result.startswith('E'):raise RuntimeError(result)
        data=bytes.fromhex(result)
        if len(data)!=n:raise ValueError('Short GDB memory read')
        return data
    def word(self,address):return int.from_bytes(self.read(address,4),'little')
    def write_word(self,address,value):
        result=self.send(f'M{address:x},4:{struct.pack("<I",value).hex()}')
        if result!='OK':raise RuntimeError(result)
    def run(self):self.send('c',False)
    def close(self):self.s.close()

def main():
    p=argparse.ArgumentParser();p.add_argument('--output',type=Path,required=True);p.add_argument('--symbols',type=Path,required=True);p.add_argument('--port',type=int,default=33345);p.add_argument('--seconds',type=float,default=3);p.add_argument('--wfi',type=int,choices=[0,1]);p.add_argument('--resume-at-end',action='store_true');a=p.parse_args()
    a.output.mkdir(parents=True,exist_ok=False);symbols=json.loads(a.symbols.read_text());r=RSP(a.port);result={'started_at':datetime.datetime.now().astimezone().isoformat(),'kind':'Actual ParcelLog MCU GDB snapshot','port':a.port,'seconds_per_phase':a.seconds,'snapshots':[]}
    def snapshot(label):
        record={'label':label,'wall_monotonic':time.monotonic(),'stop_reply':r.send('?'),'registers_raw':r.send('g'),'words':{}}
        for name,address in symbols.items():
            try:record['words'][name]={'address':address,'value':r.word(address)}
            except Exception as error:record['words'][name]={'address':address,'error':str(error)}
        for name,address,n in [('flash-registers',0x40022000,64),('tim3-registers',0x40000400,64),('systick-registers',0xe000e010,16),('nvic-enable',0xe000e100,4),('nvic-pending',0xe000e200,4),('scb-icsr',0xe000ed04,4),('flash-page14',0x08007000,2048),('flash-page15',0x08007800,2048)]:
            try:
                data=b''.join(r.read(address+offset,min(256,n-offset)) for offset in range(0,n,256));file=a.output/(label+'-'+name+'.bin');file.write_bytes(data)
                record[name]={'address':address,'length':n,'sha256':hashlib.sha256(data).hexdigest(),'all_ff':all(b==255 for b in data),'ff_bytes':data.count(255),'crc32':f'{zlib.crc32(data):08x}'}
            except Exception as error:record[name]={'address':address,'error':str(error)}
        result['snapshots'].append(record)
    try:
        result['capabilities']=r.send('qSupported');snapshot('initial')
        r.run();time.sleep(a.seconds);result['stop1']=r.stop();snapshot('busy')
        if a.wfi is not None:
            address=symbols['instrument_use_wfi'];r.write_word(address,a.wfi);result['wfi_written']=a.wfi
            r.run();time.sleep(a.seconds);result['stop2']=r.stop();snapshot('wfi-phase')
        if a.resume_at_end:r.run()
    finally:
        result['transcript']=r.transcript;(a.output/'rsp.json').write_text(json.dumps(result,indent=2)+'\n');r.close()
    print(a.output/'rsp.json')
if __name__=='__main__':main()
