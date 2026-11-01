"""Transport correctness only. This does not execute MCU firmware."""
import importlib.util
from pathlib import Path
p=Path(__file__).resolve().parents[1]/'tools/lab3_rsp_probe.py'
s=importlib.util.spec_from_file_location('rsp_probe',p);m=importlib.util.module_from_spec(s);s.loader.exec_module(m)
class Wire:
 def __init__(self,data):self.data=data;self.sent=bytearray()
 def recv(self,n):b=self.data[:1];self.data=self.data[1:];return b
 def sendall(self,b):self.sent.extend(b)
def client(data):
 r=m.RSP.__new__(m.RSP);r.s=Wire(data);r.transcript=[];return r
r=client(b'+$01020304#8a');assert r.read(0x20000000,4)==bytes([1,2,3,4]);assert r.s.sent==b'$m20000000,4#4f+'
for bad,exception in [(b'',EOFError),(b'$OK#0',EOFError),(b'$OK#00',ValueError)]:
 try:client(bad).packet();raise AssertionError('accepted broken packet')
 except exception:pass
r=client(b'$OK#9a');r.write_word(0x20000000,1);assert b'M20000000,4:01000000' in r.s.sent
print('RSP host: fragmented checksum, ACK, little-endian memory, EOF and bad checksum passed; no MCU run')
