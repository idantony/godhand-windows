"""Read only selected startup sound payloads, without audio playback/extraction.
Writes a private JSON evidence report only. Large bgm00.afs reads are bounded
per selected member header, never bulk loaded. No codecs are inferred from suffix.
"""
import argparse, hashlib, json, struct
from pathlib import Path
PROJECT=Path(__file__).resolve().parents[1]
def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output',type=Path,default=PROJECT/'analysis/sound-startup-samples.json')
    args=ap.parse_args()
    index=json.loads((PROJECT/'analysis/asset-index.json').read_text())
    records=[]
    with open(index['source_iso']['path'],'rb') as iso:
        for name in ['se/core.ses','se/pl00.ses','se/core.afs','se/pl00.afs','bgm/bgm00.afd','bgm/bgm00.afs']:
            e=next(x for x in index['entries'] if x['path']==name)
            def read(off,length):
                if off<0 or length<0 or off+length>e['size']: raise ValueError('bounds')
                iso.seek(e['iso_offset']+off);b=iso.read(length)
                if len(b)!=length:raise ValueError('short read')
                return b
            r={'path':name,'size':e['size'],'iso_offset':e['iso_offset']}
            if e['size']<4000000:
                b=read(0,e['size']);r['sha256']=hashlib.sha256(b).hexdigest()
            if name.endswith('.ses'):
                assert b[:4]==b'SES\0' and b[4:8]==bytes.fromhex('cdcc8c3f') and b[12]==0
                r['sections']=[]
                for pos in [0x10,0x18,0x20,0x28]:
                    off,size=struct.unpack_from('<II',b,pos);assert off+size<=len(b)
                    r['sections'].append({'header_offset':pos,'offset':off,'size':size,'sha256':hashlib.sha256(b[off:off+size]).hexdigest()})
                off=struct.unpack_from('<I',b,0x30)[0];r['external_afs']=b[off:b.index(0,off)].decode('ascii')
            if name.endswith('.afs'):
                h=read(0,8);assert h[:4]==b'AFS\0';count=struct.unpack_from('<I',h,4)[0]
                assert count<=100000 and 8+count*8<=e['size']
                table=read(8,count*8);r['members']=[]
                for i in range(count):
                    off,size=struct.unpack_from('<II',table,i*8);assert off+size<=e['size']
                    h=read(off,min(size,40));m={'index':i,'offset':off,'size':size,'header40_hex':h.hex()}
                    if h[:2]==b'\x80\0' and len(h)>=20:
                        m.update(encoding=h[4],block_bytes=h[5],bits=h[6],channels=h[7],rate=struct.unpack_from('>I',h,8)[0],samples=struct.unpack_from('>I',h,12)[0],version=h[18],flags=h[19])
                    r['members'].append(m)
            records.append(r)
    report={'source_iso':index['source_iso'],'scope':'Selected full SES/small AFS hashes; large BGM AFS directory and 40-byte member headers only. No PCM decoding or playback.','files':records}
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    print('Sound payload evidence:',args.output)
if __name__=='__main__':main()
