"""Read small real TIM2 assets from indexed ISO; no bulk extraction."""
import argparse,json,hashlib,struct
from pathlib import Path
def main():
 parser=argparse.ArgumentParser(description=__doc__)
 parser.add_argument('--project',type=Path,default=Path(__file__).resolve().parents[1])
 args=parser.parse_args()
 root=args.project.resolve()
 index=json.loads((root/'analysis/asset-index.json').read_text(encoding='utf-8'))
 textures=[]
 with open(index['source_iso']['path'],'rb') as f:
  for e in index['entries']:
   if not e['path'].endswith('.tm2'):continue
   f.seek(e['iso_offset']);b=f.read(min(e['size'],80))
   if b[:4]!=b'TIM2':raise ValueError('Unknown texture header')
   textures.append(dict(path=e['path'],size=e['size'],sha256=e['sha256'],header_hex=b[:64].hex()))
  selected=next(e for e in index['entries'] if e['path']=='etc/subscr/face_hm00.tm2')
  f.seek(selected['iso_offset']);blob=f.read(selected['size'])
 if hashlib.sha256(blob).hexdigest()!=selected['sha256']:raise ValueError('Asset hash mismatch')
 if len(blob)!=2176 or blob[:6]!=b'TIM2\x04\x00' or struct.unpack_from('<HH',blob,36)!=(64,64):
  raise ValueError('Selected texture differs from the verified first-picture layout')
 dest=root/'reference/render/face_hm00.tm2';dest.parent.mkdir(parents=True,exist_ok=True)
 if dest.exists() and hashlib.sha256(dest.read_bytes()).hexdigest()!=selected['sha256']:
  raise ValueError('Existing private texture differs; refusing overwrite')
 if not dest.exists():dest.write_bytes(blob)
 report=dict(schema_version=1,source_iso=index['source_iso'],selected=selected,textures=textures,
  sample=str(dest),validated_planes=dict(width=64,height=64,image_offset=64,image_size=2048,clut_offset=2112,clut_size=64),
  evidence=['vendor/god-hand-decomp/include/godhand/tim2.h','vendor/god-hand-decomp/src/cod/0015e0a0.c','vendor/god-hand-decomp/include/godhand/cModel.h'],
  render_boundary='TIM2 first-picture -> image and CLUT views -> func_0015E0A0 -> func_0015E510 image upload and func_0015E8B8 CLUT upload -> GS TEX0. Native parser replaces neither game model logic nor GS.',
  model_boundary='cModel nodes contain two 0xB0 packets by frame parity, packet headers and GIF register words. Mesh/animation dat formats not yet decoded.',
  next='Use the native restricted decoder and optional GPU asset QA tool. Authored transfer/CLUT evidence remains in render-path.json; this generated inventory does not replace it.',
  limitations=['Generated plane inventory only; final blend/shader correctness is not established','Model and animation container semantics remain pending','No game scene or original renderer completion claim','Private texture sample and derived header data must stay excluded from Git'])
 (root/'analysis/render-assets.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
 print('Indexed',len(textures),'TIM2 files; extracted',len(blob),'bytes')
if __name__=='__main__':main()
