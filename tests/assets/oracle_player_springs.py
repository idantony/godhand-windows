"""Independent binary32 oracle for actual default-costume 1038E0 spring setup.

Generate the bounded native input with test_game_player_springs ISO ELF DUMP.
Then run: python oracle_player_springs.py ELF DUMP --report G:/.../result.json
DUMP contains user-owned model input and native output; keep it outside Git.
The oracle reads actual ELF index tables and reconstructs positions/lengths
without calling the native implementation. This is an explicit numerical
reference, not a hardware-VU oracle; Python sqrt is rounded to binary32.
"""
import argparse, struct, math, pathlib, json, hashlib
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('elf',type=pathlib.Path)
parser.add_argument('dump',type=pathlib.Path)
parser.add_argument('--report',type=pathlib.Path)
args=parser.parse_args()
elf=args.elf.read_bytes()
data=args.dump.read_bytes()
if len(elf)<52 or elf[:7]!=b'\x7fELF\x01\x01\x01': raise ValueError('Expected ELF32 little-endian input')
if len(data)!=64+41*76+44*0xa0: raise ValueError('Unexpected native dump extent')
def f(x): return struct.unpack('<f',struct.pack('<f',x))[0]
def words(b):return struct.unpack('<'+'f'*(len(b)//4),b)
def bits(x):return struct.unpack('<I',struct.pack('<f',x))[0]
def add(x,y):return f(x+y)
def mul(x,y):return f(x*y)
def length(v):return f(math.sqrt(add(add(mul(v[0],v[0]),mul(v[1],v[1])),mul(v[2],v[2]))))
def mapped(va,n):
    ph=struct.unpack_from('<I',elf,28)[0];stride,count=struct.unpack_from('<HH',elf,42)
    for i in range(count):
        typ,off,addr,_,files,mem,_,_=struct.unpack_from('<8I',elf,ph+i*stride)
        if typ==1 and addr<=va and va+n<=addr+files:return elf[off+va-addr:off+va-addr+n]
    raise ValueError(hex(va))
arrays=[mapped(a,40) if a else bytes([255])*40 for a in [0x3bbf80,0x3bbff8,0,0,0,0x3bbfa8,0x3bbfd0]]
root=words(data[:64]);parts=[(words(data[64+i*76:128+i*76]),words(data[128+i*76:140+i*76])) for i in range(41)]
actual=data[64+41*76:]
position={};direction={};lens={};radius={};local={}
for row,id in enumerate(arrays[0]):
    if id==255:continue
    target,copy=arrays[6][row],arrays[5][row]
    if target!=255:
        p,q=parts[id][1],parts[target][1];d=[add(p[k],-q[k]) for k in range(3)]
        lens[id]=length(d);position[id]=q;local[id]=[add(q[k],-p[k]) for k in range(3)]
        l=length(local[id]);direction[id]=[mul(x,f(1.0/l)) for x in local[id]] if l>0 else [0.0]*3
    elif copy!=255:
        lens[id]=lens[copy];local[id]=local[copy];direction[id]=direction[copy]
        delta=[add(add(mul(root[k],local[id][0]),mul(root[4+k],local[id][1])),mul(root[8+k],local[id][2])) for k in range(3)]
        position[id]=[add(parts[id][1][k],delta[k]) for k in range(3)]
    else:raise AssertionError('default source domain unexpectedly changed')
for row,id in enumerate(arrays[0]):
    radius[id]=[mul(length([add(position[id][k],-position[neighbor][k]) for k in range(3)]),0.5) if neighbor!=255 else 0.0 for neighbor in [arrays[j][row] for j in range(1,5)]]
checks=0
for id in position:
    for off,values in [(0,local[id]),(0x10,direction[id]),(0x40,position[id]),(0x60,[lens[id]]),(0x20,radius[id])]:
        for k,x in enumerate(values):
            observed=struct.unpack_from('<I',actual,id*0xa0+off+4*k)[0]
            assert observed==bits(x),(id,hex(off+4*k),hex(observed),hex(bits(x)))
            checks+=1
result={'result':'PASS','comparisons':checks,'scope':'40 real default-costume springs: local, direction, target/copy worldposition, length, 4 adjacency radii','model_parts':41,'numeric_policy':'independent Python binary64 sqrt rounded to explicit binary32, each source add/mul/div separately rounded; not hardware bitexact claim','elf_sha256':hashlib.sha256(elf).hexdigest(),'input_sha256':hashlib.sha256(data).hexdigest()}
if args.report: args.report.write_text(json.dumps(result,indent=2),encoding='utf-8')
print(json.dumps(result))


