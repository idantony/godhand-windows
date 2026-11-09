"""Inspect actual startup packs; retain only small private directory/font samples."""
import argparse,json,hashlib,struct
from pathlib import Path
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--project',type=Path,default=Path(__file__).resolve().parents[1]);p=ap.parse_args().project.resolve()
 idx=json.loads((p/'analysis/asset-index.json').read_text(encoding='utf-8'));base=json.loads((p/'analysis/disc-baseline.json').read_text(encoding='utf-8'));elf=Path(base['boot']['local_path']).read_bytes()
 def image(a,n):
  s=next(s for s in base['boot']['elf']['program_headers'] if s['vaddr']<=a<s['vaddr']+s['filesz']);o=s['offset']+a-s['vaddr'];return elf[o:o+n]
 reports=[];dest=p/'reference/core';dest.mkdir(parents=True,exist_ok=True)
 with open(idx['source_iso']['path'],'rb') as f:
  for name in ['etc/core.bin','etc/core_usa.bin']:
   e=next(e for e in idx['entries'] if e['path']==name);f.seek(e['iso_offset']);b=f.read(e['size'])
   if hashlib.sha256(b).hexdigest()!=e['sha256']:raise ValueError('changed asset')
   count=struct.unpack_from('<I',b)[0];offsets=list(struct.unpack_from('<'+'I'*count,b,4));entries=[]
   for i,start in enumerate(offsets):
    end=offsets[i+1] if i+1<count else len(b);tag=b[4+count*4+i*4:8+count*4+i*4].split(b'\0')[0].decode('ascii');entries.append(dict(tag=tag,offset=start,extent=end-start,sha256=hashlib.sha256(b[start:end]).hexdigest()))
   (dest/(Path(name).stem+'-directory.bin')).write_bytes(b[:4+count*8])
   first=next(e for e in entries if e['tag']=='TM2');(dest/(Path(name).stem+'-first.tm2')).write_bytes(b[first['offset']:first['offset']+first['extent']])
   reports.append(dict(source=e,entries=entries))
 report=dict(schema_version=1,bundles=reports,original_search=dict(address='0x31CCF0',sha256=hashlib.sha256(image(0x31ccf0,0xd0)).hexdigest(),semantics='count@0, relative offsets@4+4*i, NUL four-byte tags@4+4*count+4*i; strcmp at3A5678; return base+offset of zero-based matching occurrence. No pointer mutation/relocation.'),callback_tags=[dict(address=hex(a),tag=image(a,4).split(b'\0')[0].decode('ascii')) for a in [0x44ace0,0x44ace8,0x44b100]],font_binding='Actual 2A4970 loads language work+558 and uses SearchData tags at44ACE0 and44ACE8, occurrences; binding internals2A4CB0 not implemented',menu_binding='Actual2AE990 loads language work+558, SearchData tag44B100 occurrence0 then1; consumer structures not implemented',validation='MSVC /std:c11 /W4 /WX PASS real disc core6 entries and USA37 entries; core TM2 and language TM2[30] accepted by native tim2 reader; IDD@592D0; missing occurrence/bad count rejected',boundaries=['Monotone offsets required by this host view for bounded extents; nonmonotone tables unsupported','Extent includes padding; do not treat it as exact semantic payload length','No fake font/menu/ID initializer success; consumer object layouts pending','Private directory/font samples and this report contain game-derived data; exclude from Git'])
 (p/'analysis/core-bundle-assets.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8');print('Core/language tables verified; callback tags',report['callback_tags'])
if __name__=='__main__':main()
