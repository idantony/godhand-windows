"""Offline bounded instruction oracle for user's SDK3A28F8 exp leaf.
Never a runtime backend. Reads actual ELF words; no game bytes are emitted.
Compares new native source result pairs, including Gaussian and finite samples.
CVT.W uses explicit truncation reference; arithmetic rounds binary32 separately.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess

def u32(n):return n & 0xFFFFFFFF

def signed(n):return n-0x100000000 if n&0x80000000 else n

def value(n):return struct.unpack("<f",struct.pack("<I",n))[0]

def bits(n):return struct.unpack("<I",struct.pack("<f",n))[0]

class Image:
    def __init__(self,data):
        self.data=data
        if len(data)<52 or data[:7]!=b"\x7fELF\x01\x01\x01":raise ValueError("notELF32 littleendian")
        ph,stride,count=struct.unpack_from("<I",data,28)[0],*struct.unpack_from("<HH",data,42)
        if stride<32 or not 0<count<=128 or ph+count*stride>len(data):raise ValueError("badprogramheaders")
        self.segments=[struct.unpack_from("<8I",data,ph+i*stride) for i in range(count)]
        for kind,off,_,_,extent,memory,_,_ in self.segments:
            if kind==1 and (extent>memory or off+extent>len(data)):raise ValueError("badloadsegment")
    def read(self,address,size):
        for kind,off,va,_,extent,_,_,_ in self.segments:
            if kind==1 and va<=address and address+size<=va+extent:return self.data[off+address-va:off+address-va+size]
        raise ValueError(f"unmapped leaf read {address:08X}")
    def word(self,address):return struct.unpack("<I",self.read(address,4))[0]

def leaf(image,input_bits):
    regs=[0xA5A5A5A5]*32;regs[0]=0;regs[31]=0xFFFFFFFC
    fp=[0xCDCDCDCD]*32;fp[12]=input_bits
    condition=False
    pc=0x3A28F8;steps=0
    def execute(address,delay=False):
        nonlocal condition,steps
        if not 0x3A28F8<=address<=0x3A2BA8:raise ValueError("leaf escaped code range")
        instruction=image.word(address);steps+=1
        op=instruction>>26;rs=(instruction>>21)&31;rt=(instruction>>16)&31;rd=(instruction>>11)&31;sa=(instruction>>6)&31;fn=instruction&63;imm=instruction&65535;si=imm-65536 if imm&32768 else imm
        flow=None
        if op==0:
            if fn==0:regs[rd]=u32(regs[rt]<<sa)
            elif fn==2:regs[rd]=regs[rt]>>sa
            elif fn==8:flow=(regs[rs],False)
            elif fn in (0x21,0x2D):regs[rd]=u32(regs[rs]+regs[rt])
            elif fn in (0x23,0x2F):regs[rd]=u32(regs[rs]-regs[rt])
            elif fn==0x24:regs[rd]=regs[rs]&regs[rt]
            elif fn==0x25:regs[rd]=regs[rs]|regs[rt]
            elif fn==0x2B:regs[rd]=int(regs[rs]<regs[rt])
            else:raise ValueError(f"integer fn {fn:X}")
        elif op in (4,5,20,21):
            take=(regs[rs]==regs[rt]) if op in (4,20) else (regs[rs]!=regs[rt])
            flow=(address+4+si*4 if take else address+8,op in (20,21) and not take)
        elif op==9:regs[rt]=u32(regs[rs]+si)
        elif op==10:regs[rt]=int(signed(regs[rs])<si)
        elif op==11:regs[rt]=int(regs[rs]<u32(si))
        elif op==12:regs[rt]=regs[rs]&imm
        elif op==13:regs[rt]=regs[rs]|imm
        elif op==15:regs[rt]=imm<<16
        elif op==49:fp[rt]=image.word(u32(regs[rs]+si))
        elif op==17:
            if rs==0:regs[rt]=fp[rd]
            elif rs==4:fp[rd]=regs[rt]
            elif rs==8:
                take=condition if rt&1 else not condition
                flow=(address+4+si*4 if take else address+8,bool(rt&2) and not take)
            elif rs in (16,20):
                ft=rt;fs=rd;fd=sa
                if rs==20 and fn==32:fp[fd]=bits(float(signed(fp[fs])))
                elif rs!=16:raise ValueError("unexpected floating format")
                elif fn==0:fp[fd]=bits(value(fp[fs])+value(fp[ft]))
                elif fn==1:fp[fd]=bits(value(fp[fs])-value(fp[ft]))
                elif fn==2:fp[fd]=bits(value(fp[fs])*value(fp[ft]))
                elif fn==3:fp[fd]=bits(value(fp[fs])/value(fp[ft]))
                elif fn==6:fp[fd]=fp[fs]
                elif fn==36:fp[fd]=u32(int(value(fp[fs])))
                elif fn==50:condition=value(fp[fs])==value(fp[ft])
                elif fn in (52,60):condition=value(fp[fs])<value(fp[ft])
                else:raise ValueError(f"floating fn {fn:X}")
            else:raise ValueError(f"unexpected cop1 rs {rs:X}")
        else:raise ValueError(f"opcode {op:X}")
        regs[0]=0
        if delay and flow is not None:raise ValueError("nested delay control")
        return flow
    while pc!=0xFFFFFFFC:
        if steps>1000:raise ValueError("leaf step limit")
        flow=execute(pc)
        if flow is None:pc+=4
        else:
            target,annul=flow
            if not annul:execute(pc+4,True)
            pc=target
    return fp[0],steps

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--elf",type=Path,required=True);parser.add_argument("--native",type=Path,required=True);parser.add_argument("--output",type=Path,required=True)
    args=parser.parse_args();out=args.output.resolve()
    if out.drive.upper()!="G:" or out.suffix.lower()!=".json" or out.exists():raise ValueError("freshG JSON only")
    raw=args.elf.read_bytes();image=Image(raw)
    process=subprocess.run([str(args.native.resolve()),"--oracle-vectors"],capture_output=True,text=True,timeout=60,check=True)
    rows=process.stdout.splitlines()
    if len(rows)!=4096:raise ValueError("native vector count mismatch")
    count=0;steps=0;digest=hashlib.sha256()
    for row in rows:
        pair=row.split()
        if len(pair)!=2:raise ValueError("bad native vector")
        source,result=map(lambda item:int(item,16),pair);expected,executed=leaf(image,source)
        if result!=expected:raise ValueError(f"native/raw mismatch input={source:08X} native={result:08X} actual_leaf={expected:08X}")
        digest.update(struct.pack("<II",source,result));count+=1;steps+=executed
    report=dict(schema=1,status="passed",elf_sha256=hashlib.sha256(raw).hexdigest(),entry="003A28F8",code_sha256=hashlib.sha256(image.read(0x3A28F8,0x2B4)).hexdigest(),native=str(args.native.resolve()),native_sha256=hashlib.sha256(args.native.read_bytes()).hexdigest(),comparisons=count,instructions=steps,pairs_sha256=digest.hexdigest(),scope="offline bounded raw SDK leaf vs native source; nearestbinary32 and CVT.W truncation reference, notEEhardware proof/runtime emulator")
    with out.open("x",encoding="utf-8") as stream:json.dump(report,stream,indent=2);stream.write("\n")
    print(out)

if __name__=="__main__":main()
