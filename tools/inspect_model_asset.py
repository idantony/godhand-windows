"""Extract only selected small original model and validated SCR subrange."""
import argparse,json,hashlib,struct
from pathlib import Path
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--project',type=Path,default=Path(__file__).resolve().parents[1]);p=ap.parse_args().project.resolve()
 index=json.loads((p/'analysis/asset-index.json').read_text(encoding='utf-8'));e=next(e for e in index['entries'] if e['path']=='om/om0e.dat')
 with open(index['source_iso']['path'],'rb') as f:f.seek(e['iso_offset']);b=f.read(e['size'])
 if hashlib.sha256(b).hexdigest()!=e['sha256']:raise ValueError('hash mismatch')
 count=struct.unpack_from('<I',b)[0];offsets=struct.unpack_from('<'+'I'*count,b,4)
 if count!=3 or offsets!=(0x20,0x2e0,0x6e0) or b[0x2e0:0x2e4]!=b'scr\0':raise ValueError('unknown model container')
 dest=p/'reference/model';dest.mkdir(parents=True,exist_ok=True)
 for name,data in [('om0e.dat',b),('om0e.scr',b[0x2e0:0x6e0])]:
  output=dest/name
  if output.exists() and output.read_bytes()!=data:raise ValueError('Existing private model differs; refusing overwrite')
  if not output.exists():output.write_bytes(data)
 report=dict(source=e,sample=str(dest/'om0e.scr'),container=dict(count=count,offsets=offsets,tags=['TM3','MD','DAT']),
  mesh=dict(descriptor_offset_scr=0x370,data_delta=-0x350,data_offset_scr=0x20,name='_00_glas',data_magic='mdb',data_num=2),
  evidence='MIT upstream include/godhand/cModel.h cModelScript.meshNum@8/meshOfs@16, cMeshInfo.dataOfs signed @0/name@8/flags@0x34/layer@0x43/alpha@0x58, cMeshData.num@8',
  implemented='Native SCR mesh descriptor views and signed data-relative address resolution, checked against real om0e sample',
  unknown=['mdb stream attribute offsets/counts not established','Texture name City01_t observed in TM3; material binding semantics pending','Original part/skin/animation encoding not inferred from float-looking arrays'],
  next='Trace cModel node setup dataOfs consumer into mdb draw-data branch num2; establish VIF UNPACK addresses and scale/format before converting actual vertices. Do not treat double frame packets as vertex layout.',private='Raw model samples and this derived report remain private; no Git',validation='Generated extraction inventory only; authored evidence and native verification are separate')
 (p/'analysis/model-assets.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8');print('Extracted original 1760B model; SCR1024B mesh points backward to mdb')
if __name__=='__main__':main()
