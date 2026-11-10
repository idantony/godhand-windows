"""Offline function-level raw instruction oracle for six original type6 passes.
Reads user ELF + native numeric packet dump. No game interpreter is shipped or
used by runtime. Only three bounded allocation/OT packet producers execute;
platform allocation and enqueue are explicit observational test hooks.
"""
import argparse, hashlib, json, math, struct, sys
from pathlib import Path
sys.dont_write_bytecode = True
MASK=(1<<64)-1

def signed(x,n):
 x&=(1<<n)-1
 return x-(1<<n) if x>>(n-1) else x

def f32(x): return struct.unpack('<f',struct.pack('<f',x))[0]
def floatbits(x): return struct.unpack('<I',struct.pack('<f',x))[0]
def floating(x): return struct.unpack('<f',struct.pack('<I',x))[0]

class Oracle:
 def __init__(self,elf):
  self.elf=elf; self.reg=[0]*32;self.fp=[0]*32;self.memory={};self.blocks=[];self.steps=0
  offset=struct.unpack_from('<I',elf,28)[0];stride,count=struct.unpack_from('<HH',elf,42)
  if elf[:7]!=b'\x7fELF\x01\x01\x01' or stride<32 or count>128:raise ValueError('ELF header')
  for i in range(count):
   kind,pos,addr,_,files,memory,_,_=struct.unpack_from('<8I',elf,offset+i*stride)
   if kind==1:
    if pos+files>len(elf) or files>memory:raise ValueError('ELF extent')
    self.blocks.append((addr,memory,elf[pos:pos+files]))
 def load(self,a,n):
  a&=0xffffffff;out=bytearray(n)
  for j in range(n):
   if a+j in self.memory:out[j]=self.memory[a+j];continue
   for base,size,raw in self.blocks:
    if base<=a+j<base+size:
     index=a+j-base;out[j]=raw[index] if index<len(raw) else 0;break
  return int.from_bytes(out,'little')
 def store(self,a,n,value):
  a&=0xffffffff
  for j,b in enumerate((value&((1<<(n*8))-1)).to_bytes(n,'little')):self.memory[a+j]=b
 def setreg(self,i,v):
  if i:self.reg[i]=v&MASK
 def execute(self,start,args):
  if start not in (0x2e9808,0x2e8b38,0x2e83d8):raise ValueError('unapproved function')
  self.reg=[0]*32;self.fp=[0]*32;self.reg[29]=0x1fff0000;self.reg[31]=0xfffffffc
  for i,value in enumerate(args):self.reg[4+i]=value&MASK
  pc=start;delay=None;queued=[];allocation=0x10000000
  end={0x2e9808:0x2e9aec,0x2e8b38:0x2e8ee8,0x2e83d8:0x2e8b38}[start]
  for step in range(30000):
   if pc==0xfffffffc:self.steps+=step;return queued
   if pc==0x15b0f0:
    self.setreg(2,allocation);pc=self.reg[31]&0xffffffff;continue
   if pc==0x31a650:
    queued.append((self.reg[5],self.reg[6],self.reg[7]));pc=self.reg[31]&0xffffffff;continue
   if not start<=pc<end:raise ValueError(f'out-of-leaf pc {pc:X}')
   ins=self.load(pc,4);op=ins>>26;rs=(ins>>21)&31;rt=(ins>>16)&31;rd=(ins>>11)&31;sa=(ins>>6)&31;fn=ins&63;imm=ins&0xffff;si=signed(imm,16);a=self.reg[rs];b=self.reg[rt];prior=delay;delay=None
   if op==0:
    if fn==0:self.setreg(rd,signed((b&0xffffffff)<<sa,32))
    elif fn==2:self.setreg(rd,signed((b&0xffffffff)>>sa,32))
    elif fn==3:self.setreg(rd,signed(b,32)>>sa)
    elif fn==8:delay=a&0xffffffff
    elif fn==0xb:
     if b:self.setreg(rd,a)
    elif fn==0x21:self.setreg(rd,signed(a+b,32))
    elif fn==0x23:self.setreg(rd,signed(a-b,32))
    elif fn==0x25:self.setreg(rd,a|b)
    elif fn==0x2d:self.setreg(rd,a+b)
    elif fn==0x2f:self.setreg(rd,a-b)
    elif fn==0x2a:self.setreg(rd,int(signed(a,64)<signed(b,64)))
    elif fn==0x38:self.setreg(rd,b<<sa)
    elif fn==0x3a:self.setreg(rd,b>>sa)
    elif fn==0x3c:self.setreg(rd,b<<(sa+32))
    elif fn==0x3e:self.setreg(rd,b>>(sa+32))
    else:raise ValueError(f'R {fn:X} at {pc:X}')
   elif op==1:
    if rt!=1:raise ValueError('REGIMM')
    if signed(a,64)>=0:delay=(pc+4+(si<<2))&0xffffffff
   elif op in (2,3):
    if op==3:self.setreg(31,pc+8)
    delay=((pc+4)&0xf0000000)|((ins&0x3ffffff)<<2)
   elif op in (4,5):
    if (a==b)==(op==4):delay=(pc+4+(si<<2))&0xffffffff
   elif op==9:self.setreg(rt,signed(a+si,32))
   elif op==10:self.setreg(rt,int(signed(a,64)<si))
   elif op==12:self.setreg(rt,a&imm)
   elif op==13:self.setreg(rt,a|imm)
   elif op==15:self.setreg(rt,signed(imm<<16,32))
   elif op in (0x1e,0x23,0x25,0x37):
    size={0x1e:16,0x23:4,0x25:2,0x37:8}[op];value=self.load(a+si,size)
    self.setreg(rt,signed(value,32) if op==0x23 else value)
   elif op in (0x1f,0x29,0x2b,0x3f):self.store(a+si,{0x1f:16,0x29:2,0x2b:4,0x3f:8}[op],b)
   elif op==0x31:self.fp[rt]=self.load(a+si,4)
   elif op==0x39:self.store(a+si,4,self.fp[rt])
   elif op==0x11:
    if rs==0:self.setreg(rt,signed(self.fp[rd],32))
    elif rs==4:self.fp[rd]=b&0xffffffff
    elif rs==16:
     fd=(ins>>6)&31;fs=(ins>>11)&31;ft=(ins>>16)&31
     if fn==2:self.fp[fd]=floatbits(f32(floating(self.fp[fs])*floating(self.fp[ft])))
     elif fn==0x24:
      value=floating(self.fp[fs])
      if not math.isfinite(value) or not -2147483648<=value<2147483648:raise ValueError('CVT domain')
      self.fp[fd]=int(value)&0xffffffff # explicit pinned ChopZero reference
     else:raise ValueError(f'FPU {fn:X}')
    else:raise ValueError(f'COP1 {rs:X}')
   else:raise ValueError(f'op {op:X} at {pc:X}')
   pc=prior if prior is not None else (pc+4)&0xffffffff
  raise ValueError('instruction bound')

def read_dump(path):
 result={'taps':{},'vertices':{},'states':{}}
 for line in path.read_text().splitlines():
  words=line.split()
  if not words:continue
  if words[0]=='display':result['display']=tuple(map(int,words[1:]))
  elif words[0]=='tap':result['taps'][int(words[1])]=(int(words[2],16),int(words[3],16))
  elif words[0]=='state':result['states'][int(words[1])]=tuple(int(x,16) for x in words[2:7])+(int(words[7]),int(words[8],16),int(words[9],16))
  elif words[0]=='vertex':result['vertices'][tuple(map(int,words[1:4]))]=tuple(map(int,words[4:9]))+(int(words[9],16),)
 return result

def main():
 p=argparse.ArgumentParser();p.add_argument('--elf',type=Path,required=True);p.add_argument('--native-dump',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 if a.output.exists():raise ValueError('refuse overwrite')
 raw=a.elf.read_bytes();native=read_dump(a.native_dump);cpu=Oracle(raw);w,h=native['display'];cpu.store(0x3c1180,4,w);cpu.store(0x3c118c,4,h)
 for i,(offset,weight) in native['taps'].items():cpu.store(0x61b540+i*4,4,offset);cpu.store(0x61b5c0+i*4,4,weight)
 entries=[(0x2e9808,[0x20000000,4,12,2]),(0x2e9808,[0x20000000,4,12,1]),(0x2e9808,[0x20000000,4,12,0]),(0x2e8b38,[0x20000000,4,11]),(0x2e83d8,[10,0x3b00,0x3a00,0,1]),(0x2e83d8,[10,0x3a00,0x3900,1,0])];compared=0
 for packet,(entry,args) in enumerate(entries):
  queued=cpu.execute(entry,args)
  if len(queued)!=1 or queued[0][:2]!=(4,12 if packet<3 else 11 if packet==3 else 10):raise ValueError('OT queue')
  base=queued[0][2]
  alpha,test,zbuf,mask,tex0,writes,restore_mask,restore_zbuf=native['states'][packet]
  if (cpu.load(base+0x60,8),cpu.load(base+0x50,8),cpu.load(base+0x40,8),cpu.load(base+0x30,8)>>32)!=(alpha,test,zbuf,mask):raise ValueError(f'header GS state packet {packet}')
  if writes and cpu.load(base+0x98,8)!=tex0:raise ValueError(f'TEX0 packet {packet}')
  trailer=0xe0 if packet<3 else 0x130 if packet==3 else 0x1030
  if (cpu.load(base+trailer,8)>>32,cpu.load(base+trailer+16,8))!=(restore_mask,restore_zbuf):raise ValueError(f'restore state packet {packet}')
  for key,expected in native['vertices'].items():
   if key[0]!=packet:continue
   _,q,k=key
   if packet<3:
    xyz=cpu.load(base+0xa0+k*8,8);rgba=cpu.load(base+0x98,4);uv=0
   else:
    offset=base+0xa0+q*0xa0+k*0x18;uv=cpu.load(offset,8);rgba=cpu.load(offset+8,4);xyz=cpu.load(offset+16,8)
   actual=(xyz&0xffff,(xyz>>16)&0xffff,(xyz>>32)&0xffffff,uv&0xffff,(uv>>16)&0xffff,rgba)
   if actual!=expected:raise ValueError(f'packet {packet} quad {q} vertex {k}: {actual} != {expected}')
   compared+=1
 a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps({'schema':1,'status':'passed','elf_sha256':hashlib.sha256(raw).hexdigest(),'native_dump_sha256':hashlib.sha256(a.native_dump.read_bytes()).hexdigest(),'raw_leaf_functions':['002E9808','002E8B38','002E83D8'],'packets':6,'vertices_compared':compared,'instructions':cpu.steps,'numeric_policy':'native binary32 nearest operations + explicit pinned EE ChopZero CVT.W reference','limits':['offline bounded test only; no runtime interpreter','native Gaussian uniforms supplied; Gaussian math independently verified by separateSDKexp/raw proof','composite seventh packet manually verified but not executed by this three-leaf oracle','noGPU/pixel-output or actualgameTrans claim']},indent=2)+'\n');print(f'PASS six actualraw packets: {compared} vertices, {cpu.steps} instructions')
if __name__=='__main__':main()
