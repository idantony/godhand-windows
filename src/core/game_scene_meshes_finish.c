/* Readable restricted original static-scroll constructor continuation.
 * Native semantic packet records replace guest DMA pointer representation;
 * no VU/GS execution or texture upload is acknowledged here. */
#include "game_scene_meshes.h"
#include "game_stage.h"
#include "id_layout_rotation.h"
#include <string.h>
#include <fenv.h>
#include <math.h>
static uint16_t u16(const uint8_t*p){return (uint16_t)(p[0]|(unsigned)p[1]<<8);}
static uint32_t u32(const uint8_t*p){return (uint32_t)u16(p)|(uint32_t)u16(p+2)<<16;}
static void w32(uint8_t*p,uint32_t x){memcpy(p,&x,4);}
static void wf(uint8_t*p,float x){memcpy(p,&x,4);}
static void w16(uint8_t*p,uint16_t x){memcpy(p,&x,2);}
static float f32(const uint8_t*p){float x;memcpy(&x,p,4);return x;}
static gh_scene_mesh_status fail(gh_scene_meshes*s,uint32_t f,int detail,gh_scene_mesh_status e){s->frontier=f;s->detail=detail;return e;}
static gh_scene_mesh_status elf_constants(const void*raw,size_t n,uint32_t address,unsigned words,uint32_t*out){
 if(!raw||n<52)return GH_MESH_BOUNDS;const uint8_t*b=raw;
 if(memcmp(b,"\177ELF\1\1\1",7)||u16(b+18)!=8)return GH_MESH_UNSUPPORTED;
 size_t ph=u32(b+28);unsigned stride=u16(b+42),count=u16(b+44),found=0;
 if(stride<32||count>128||ph>n||(size_t)count*stride>n-ph)return GH_MESH_BOUNDS;
 for(unsigned i=0;i<count;i++){const uint8_t*p=b+ph+(size_t)i*stride;if(u32(p)!=1)continue;
  uint32_t off=u32(p+4),base=u32(p+8),files=u32(p+16),mem=u32(p+20);
  if(files>mem||off>n||files>n-off||(uint64_t)base+mem>UINT64_C(0x100000000))return GH_MESH_BOUNDS;
  if(base<=address&&(uint64_t)address+(uint64_t)words*4<=(uint64_t)base+mem){
   if(found++)return GH_MESH_BOUNDS;uint32_t delta=address-base;
   for(unsigned j=0;j<words;j++){uint8_t word[4]={0};for(unsigned k=0;k<4;k++)if((uint64_t)delta+4*j+k<files)word[k]=b[(size_t)off+delta+4*j+k];out[j]=u32(word);}
  }
 }
 return found?GH_MESH_OK:GH_MESH_BOUNDS;
}
static float mul(float a,float b){volatile float x=a*b;return x;}
static float add(float a,float b){volatile float x=a+b;return x;}
static void matrix(float*out,const float*a,const float*b){float m[16];
 for(unsigned col=0;col<4;col++){for(unsigned row=0;row<4;row++){
  float value=mul(a[row],b[col*4]);value=add(value,mul(a[4+row],b[col*4+1]));
  value=add(value,mul(a[8+row],b[col*4+2]));m[col*4+row]=add(value,mul(a[12+row],b[col*4+3]));
 }}
 memcpy(out,m,sizeof(m));
}
/* 147728 order0: SDK premultiplies X,Y,Z. 147B60/147BF8 use
 * 30A2E0 column-major T*R*S. Coefficients at3C3C90 verified below. */
static gh_scene_mesh_status transform(const float*angle,const float*position,float*out){
 float sx,cx,sy,cy,sz,cz;
 if(gh_id_sincos_reference(angle[0],&sx,&cx)||gh_id_sincos_reference(angle[1],&sy,&cy)||gh_id_sincos_reference(angle[2],&sz,&cz))return GH_MESH_UNSUPPORTED;
 float rx[16]={1,0,0,0,0,cx,sx,0,0,add(0,-sx),cx,0,0,0,0,1};
 float ry[16]={cy,0,add(0,-sy),0,0,1,0,0,sy,0,cy,0,0,0,0,1};
 float rz[16]={cz,sz,0,0,add(0,-sz),cz,0,0,0,0,1,0,0,0,0,1};
 float identity[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},rotation[16],translation[16];
 memcpy(translation,identity,sizeof(identity));memcpy(translation+12,position,12);
 matrix(rotation,rx,identity);matrix(rotation,ry,rotation);matrix(rotation,rz,rotation);
 matrix(out,translation,rotation);matrix(out,out,identity);return GH_MESH_OK;
}
static uint32_t half_bits(uint16_t h){
 uint32_t mantissa=h&1023u,exponent;
 if(!(h&0x7c00u)){exponent=1;if(!mantissa)exponent=UINT32_C(0xffffff90);
  else{do{mantissa<<=1;exponent--;}while(!(mantissa&1024u));mantissa&=1023u;}}
 else exponent=(h>>10)&31u;
 return ((uint32_t)h&0x8000u)<<16|(exponent+0x70u)*0x800000u|mantissa<<13;
}
gh_scene_mesh_status gh_scene_meshes_finish_object_reference(gh_scene_meshes*s,gh_scene_mesh_object*o,const gh_stage_textures*textures,
 const gh_game_world_light*light,const void*elf,size_t elf_bytes){
 if(!s||!textures||!light||!light->constructed)return GH_MESH_ARGUMENT;
 if(!o)return GH_MESH_ARGUMENT;if(o->constructor_complete)return GH_MESH_OK;
 if(!s->prepared||!o->factory_ready||!o->handle||!o->node||!o->packet_allocation.pointer)return GH_MESH_BUSY;
 const gh_scene_mesh_descriptor*d=&o->descriptor;const uint8_t*desc=d->script.bytes+d->mesh.descriptor_offset;
 if((d->mesh.flags&~0xc0174320u)||d->node_flags!=0x1001000u||!s->tables->scroll_texture.bytes)return fail(s,0x31d3d0,0,GH_MESH_UNSUPPORTED);
 /* Observed unit-scale, static object profile. In the bounded [-3,3]
  * Euler domain the preceding1F7DA0 range reduction subtracts exactlyzero. */
 float angle[3];for(unsigned i=0;i<3;i++){angle[i]=f32(desc+0x1c+4*i);
  if(u32(desc+0x10+4*i)!=0x3f800000u||!isfinite(angle[i])||fabsf(angle[i])>3.0f||u32(desc+0x4c+4*i))return fail(s,0x147728,0,GH_MESH_UNSUPPORTED);
 }
 if((u32(desc+0x3c)&15u)>5u)return fail(s,0x319e00,0,GH_MESH_UNSUPPORTED);
 float distance=f32(desc+0x44);if(!isfinite(distance)||distance<0||distance>3276.0f)return fail(s,0x3187d8,0,GH_MESH_UNSUPPORTED);
 uint32_t constant[3],coeff[4];gh_scene_mesh_status e=elf_constants(elf,elf_bytes,0x3bd890,3,constant);if(e)return fail(s,0x3187d8,0,e);
 e=elf_constants(elf,elf_bytes,0x3c3c90,4,coeff);if(e)return fail(s,0x30a5d0,0,e);
 const uint32_t supported[]={0x362e9c14,0xb94fb21f,0x3c08873e,0xbe2aaaa4};
 if(memcmp(coeff,supported,sizeof(coeff)))return fail(s,0x30a5d0,0,GH_MESH_UNSUPPORTED);
 gh_scene_mesh_material material[16]={0};int16_t min[3]={32767,32767,32767},max[3]={-32768,-32768,-32768};
 for(unsigned i=0;i<d->submesh_count;i++){gh_scene_mesh_stream stream;e=gh_scene_mesh_stream_at(d,i,&stream);if(e)return fail(s,0x155678,(int)i,e);
  gh_stage_channel ch;gh_stage_status status=gh_stage_texture_channel(textures,stream.material,&ch);
  if(status!=GH_STAGE_OK&&status!=GH_STAGE_UNAVAILABLE)return fail(s,0x155678,status,GH_MESH_UNSUPPORTED);
  material[i].owner=textures;material[i].slot=stream.material;material[i].tm3_override_required=stream.material!=255;
  if(!status){material[i].initial_tex0=ch.tex0;material[i].initial_resolved=1;}
  else if(stream.material==255){
   /* 155678 copies the cold TEX0 slot into the packet even for FF.
    * The authoritative stage channel has no intervening writer. */
   uint32_t tex0[2];
   e=elf_constants(elf,elf_bytes,0x752b08,2,tex0);
   if(e)return fail(s,0x155678,(int)i,e);
   material[i].initial_tex0=(uint64_t)tex0[0]|(uint64_t)tex0[1]<<32;
   material[i].initial_resolved=1;
  }
  for(size_t j=0;j<stream.vertices;j++){int16_t xyz[3];uint16_t raww;e=gh_scene_mesh_position(&stream,j,xyz,&raww);if(e)return e;
   for(unsigned axis=0;axis<3;axis++){if(xyz[axis]<min[axis])min[axis]=xyz[axis];if(xyz[axis]>max[axis])max[axis]=xyz[axis];}}
 }
 float center[3],extent[3],position[3],world[16];int32_t distance_tenths;fenv_t saved;
 if(feholdexcept(&saved))return fail(s,0x148f50,0,GH_MESH_UNSUPPORTED);
 if(fesetround(FE_TONEAREST)){fesetenv(&saved);return fail(s,0x148f50,0,GH_MESH_UNSUPPORTED);}
 for(unsigned i=0;i<3;i++){volatile float lo=(float)min[i]/100.0f,hi=(float)max[i]/100.0f;
  volatile float difference=hi-lo,half=difference*.5f,sum=half+lo;extent[i]=half;center[i]=sum;position[i]=f32(desc+0x28+4*i);}
 volatile float scaled_distance=distance*10.0f;distance_tenths=(int32_t)scaled_distance;
 e=transform(angle,position,world);
 int restore=fesetenv(&saved);if(restore)return fail(s,0x148f50,0,GH_MESH_UNSUPPORTED);
 if(e)return fail(s,0x30a5d0,0,e);
 for(unsigned i=0;i<3;i++)if(!isfinite(position[i]))return fail(s,0x1476d0,0,GH_MESH_UNSUPPORTED);
 /* Actual14C328 alloc3A0 when skeletal-node-count2B4=0. */
 if(gh_heap_allocate(s->heap,0x3a0,64,&o->object_packet_allocation)!=GH_MEM_OK)return fail(s,0x14c328,0,GH_MESH_MEMORY);
 uint8_t*p=o->allocation.pointer,*node=o->node;
 memcpy(o->materials,material,sizeof(material));o->fog_rgb=u32(light->scalar+4)&0xffffffu;
 for(unsigned axis=0;axis<3;axis++){wf(p+0x260+4*axis,center[axis]);wf(p+0x26c+4*axis,extent[axis]);
  memcpy(p+0x110+4*axis,desc+0x10+4*axis,4);memcpy(p+0x100+4*axis,desc+0x1c+4*axis,4);
  wf(p+0xb0+4*axis,position[axis]);w32(p+0x310+4*axis,constant[axis]);w32(p+0x120+4*axis,0x3f800000);
 }
 p[0x2b1]=1;p[0x2fd]=d->mesh.layer;w32(p+0x2f8,d->mesh_index);w32(p+0x250,0x400480);
 /* Actual first descriptor3187D8: use programs10 in both parity buffers;
  * node negative-ID form suppresses its first program transfer. */
 o->node_program[0]=o->node_program[1]=0x8000000au;o->object_program[0]=o->object_program[1]=10;
 uint32_t flags=u32(node+0x380),config=u32(desc+0x3c)&15u;
 switch(config){case 0:flags|=0x200000;break;case 2:flags|=0x20000;break;case 3:flags|=2;break;case 4:flags|=0x40000;break;case 5:flags|=0x10000000;break;default:flags&=0xefd9fffd;break;}
 if(d->mesh.flags&0x80000000u){flags|=0x4000;w32(p+0x2c4,1);}w32(node+0x380,flags);
 if(d->mesh.flags&0x40000u){flags|=0x2000;w32(p+0x250,u32(p+0x250)|0x10000u);}
 if(d->mesh.flags&0x100000u)flags|=0x800000;w32(node+0x380,flags);
 p[0x32c]=1;p[0x32e]=255;w16(p+0x330,65535);w32(p+0x2f0,u32(p+0x2f0)|0x800u);
 w32(p+0x324,half_bits(u16(desc+0x3e)));w32(p+0x328,half_bits(u16(desc+0x40)));
 w32(p+0x254,0xc0000000);w16(p+0x2ae,(uint16_t)distance_tenths);p[0x2b0]=(uint8_t)((distance_tenths*15)/100);w32(p+0x2b8,half_bits(u16(desc+0x48)));
 uint32_t model_flags=u32(p+0x350)&~0x10000000u;
 /* 318B20–318B3C routes flag5 meshes to the late Scenario update. */
 if(d->mesh.flags&0x20u)model_flags|=0x20000000u;
 w32(p+0x350,model_flags);
 /* 149EB8: no skeletal nodes in this static constructor. */
 memcpy(p+0x80,world,sizeof(world));
 o->constructor_complete=1;o->render_ready=0;s->frontier=0;s->detail=0;
 return GH_MESH_OK;
}
gh_scene_mesh_status gh_scene_meshes_finish_first_reference(gh_scene_meshes*s,const gh_stage_textures*t,const gh_game_world_light*l,const void*elf,size_t n){
 if(!s)return GH_MESH_ARGUMENT;return gh_scene_meshes_finish_object_reference(s,&s->first,t,l,elf,n);
}
