"""Analysis-only REL wrapper, never execute the resulting guest ELF."""
import argparse,hashlib,json,struct
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('boot',type=Path);p.add_argument('rel',type=Path);p.add_argument('output',type=Path);p.add_argument('--report',type=Path);a=p.parse_args()
boot=bytearray(a.boot.read_bytes());source=a.rel.read_bytes();rel=bytearray(source)
assert boot[:7]==b'\x7fELF\x01\x01\x01' and rel[:4]==b'SNR2'
h=struct.unpack_from('<15I',rel);assert h[11]==len(rel) and h[3]+12*h[4]<=len(rel) and h[1]+12*h[2]<=len(rel)
def string(off):
 if not off:return ''
 assert off<len(rel);end=rel.find(0,off);assert end>=off;return rel[off:end].decode('ascii')
syms=[]
for i in range(h[4]):
 name,addr,unknown,kind,pad=struct.unpack_from('<IIHBB',rel,h[3]+12*i)
 syms.append(dict(index=i,name=string(name),address=addr,type=kind,analysis_target=addr if addr else (0x02000000+i*0x10000 if name else 0)))
patches=[]
for i in range(h[2]):
 site,kind,index,pad,addend=struct.unpack_from('<IBHBI',rel,h[1]+12*i)
 assert pad==0 and index<len(syms) and site+4<=len(rel)
 target=(syms[index]['analysis_target']+addend)&0xffffffff;word=struct.unpack_from('<I',rel,site)[0]
 if kind==2:word=target
 elif kind==4:word=(word&0xfc000000)|((target>>2)&0x03ffffff)
 elif kind==5:word=(word&0xffff0000)|(((target+0x8000)>>16)&0xffff)
 elif kind==6:word=(word&0xffff0000)|(target&0xffff)
 else:raise ValueError(('unsupported relocation',kind,hex(site)))
 struct.pack_into('<I',rel,site,word);patches.append(dict(offset=site,type=kind,symbol=index,addend=addend,analysis_target=target,name=syms[index]['name']))
phoff=struct.unpack_from('<I',boot,28)[0];stride,count=struct.unpack_from('<HH',boot,42);assert stride==32
first=min(struct.unpack_from('<8I',boot,phoff+32*i)[1] for i in range(count) if struct.unpack_from('<I',boot,phoff+32*i)[0]==1)
at=phoff+32*count;assert at+32<=first and not any(boot[at:at+32])
off=(len(boot)+4095)&~4095;boot.extend(bytes(off-len(boot)));boot.extend(rel)
struct.pack_into('<8I',boot,at,1,off,0,0,len(rel),len(rel),7,0x1000);struct.pack_into('<H',boot,44,count+1)
a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_bytes(boot)
report=dict(analysis_only=True,source_rel_sha256=hashlib.sha256(source).hexdigest(),wrapper_sha256=hashlib.sha256(boot).hexdigest(),method='OriginalbootELF plus REL PT_LOAD at0; imports synthetic02000000+symbolindex*10000; local file offsets unchanged; no guest execution',reloc_schema='u32site,u8type,u16symbol,u8pad,u32addend',symbols=syms,relocations=patches)
(a.report or a.output.with_suffix('.json')).write_text(json.dumps(report,indent=2)+'\n');print(report['wrapper_sha256'])
