/* Original100FB8. MIT upstream eaae18d0dfab1f9a0df276ae5ca1581ad326faf3.
 * Actual patched ELF write oracle: cloth-template-raw-write-oracle-private.json.
 * Persistent host owner replaces four original contiguous BSS arrays. */
#include "game_cloth_templates.h"
#include <string.h>
#include <float.h>
_Static_assert(sizeof(float)==4&&FLT_RADIX==2&&FLT_MANT_DIG==24,"binary32 required");
_Static_assert(sizeof(gh_cloth_sphere)==0x30,"cloth record stride");
_Static_assert(offsetof(gh_cloth_sphere,blend)==4,"blend offset");
_Static_assert(offsetof(gh_cloth_sphere,offset_a)==0x10,"offset A");
_Static_assert(offsetof(gh_cloth_sphere,offset_b)==0x20,"offset B");
typedef struct {uint8_t a,b;float blend,radius,ax,ay,az,bx,by,bz;} initial_sphere;
static const initial_sphere initial[42]={
 {17,17,1.0f,0.15f,0.0f,-0.2f,0.0f,0.0f,-0.2f,0.0f},
 {17,17,1.0f,0.15f,0.0f,-0.3f,0.0f,0.0f,-0.3f,0.0f},
 {18,18,1.0f,0.11f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f},
 {18,19,0.66f,0.11f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f},
 {18,19,0.33f,0.11f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f},
 {19,19,1.0f,0.11f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f},
 {19,20,0.33f,0.11f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f},
 {22,22,1.0f,0.11f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f},
 {22,23,0.66f,0.11f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f},
 {22,23,0.33f,0.11f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f},
 {23,23,1.0f,0.11f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f},
 {23,24,0.33f,0.11f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f},
 {17,17,1.0f,0.17f,0.0f,-0.18f,0.0f,0.0f,-0.18f,0.0f},
 {18,18,1.0f,0.11f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f},
 {18,19,0.66f,0.11f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f},
 {18,19,0.33f,0.11f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f},
 {19,19,1.0f,0.11f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f},
 {19,20,0.33f,0.11f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f},
 {22,22,1.0f,0.11f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f},
 {22,23,0.66f,0.11f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f},
 {22,23,0.33f,0.11f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f},
 {23,23,1.0f,0.11f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f},
 {23,24,0.33f,0.11f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f},
 {17,17,1.0f,0.15f,0.0f,-0.15f,0.0f,0.0f,-0.15f,0.0f},
 {18,18,1.0f,0.115f,0.02f,0.04f,0.0f,0.02f,0.04f,0.0f},
 {18,18,1.0f,0.12f,0.02f,0.04f,-0.05f,0.02f,0.04f,-0.05f},
 {18,19,0.1f,0.1f,0.01f,0.0f,0.0f,0.01f,0.0f,0.0f},
 {22,22,1.0f,0.115f,-0.02f,0.04f,0.0f,-0.02f,0.04f,0.0f},
 {22,22,1.0f,0.115f,-0.02f,0.04f,-0.05f,-0.02f,0.04f,-0.05f},
 {22,23,0.1f,0.1f,-0.01f,0.0f,0.0f,-0.01f,0.0f,0.0f},
 {17,17,1.0f,0.18f,0.0f,-0.15f,0.0f,0.0f,-0.15f,0.0f},
 {17,17,1.0f,0.15f,0.0f,-0.3f,0.0f,0.0f,-0.3f,0.0f},
 {18,18,1.0f,0.15f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f},
 {18,19,0.66f,0.15f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f},
 {18,19,0.33f,0.15f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f},
 {19,19,1.0f,0.15f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f},
 {19,20,0.33f,0.15f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f},
 {22,22,1.0f,0.15f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f},
 {22,23,0.66f,0.15f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f},
 {22,23,0.33f,0.15f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f},
 {23,23,1.0f,0.15f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f},
 {23,24,0.33f,0.15f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f},
};
static uint32_t u32(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static unsigned u16(const uint8_t*p){return p[0]|(unsigned)p[1]<<8;}
gh_cloth_status gh_cloth_templates_from_elf(gh_cloth_templates*s,const void*data,size_t n){
 if(!s||!data||s->loaded)return GH_CLOTH_ARGUMENT;const uint8_t*b=data;
 if(n<52||memcmp(b,"\177ELF\1\1\1",7)||u16(b+18)!=8)return GH_CLOTH_BOUNDS;
 size_t ph=u32(b+28);unsigned stride=u16(b+42),count=u16(b+44);
 if(stride<32||count>128||ph>n||(size_t)count*stride>n-ph)return GH_CLOTH_BOUNDS;
 gh_cloth_templates value={0};int found=0;
 for(unsigned i=0;i<count;i++){
  const uint8_t*p=b+ph+(size_t)i*stride;uint32_t base=u32(p+8),files=u32(p+16),memory=u32(p+20),off=u32(p+4);
  if(u32(p)!=1||base>0x460e00||(uint64_t)0x460e00+sizeof(value.records)>(uint64_t)base+memory)continue;
  if(found||files>memory||off>n||files>n-off)return GH_CLOTH_BOUNDS;found=1;
  size_t delta=0x460e00-base;
  if(delta<files){size_t available=files-delta;if(available>sizeof(value.records))available=sizeof(value.records);memcpy(value.records,b+off+delta,available);}
 }
 if(!found)return GH_CLOTH_BOUNDS;
 /* Separate original461600 header/461608 output storage must also be mapped.
  * ActualcoldELF has this range in BSS. Reject filebacked nonzero initialdata
  * until an explicit loader for that changed baseline exists. */
 found=0;for(unsigned i=0;i<count;i++){const uint8_t*p=b+ph+(size_t)i*stride;uint32_t base=u32(p+8),files=u32(p+16),memory=u32(p+20);if(u32(p)!=1||base>0x461600||(uint64_t)0x461608+sizeof(value.geometry.records)>(uint64_t)base+memory)continue;if(found||(uint64_t)base+files>0x461600)return GH_CLOTH_BOUNDS;found=1;}
 if(!found)return GH_CLOTH_BOUNDS;value.loaded=1;*s=value;return GH_CLOTH_OK;
}
gh_cloth_status gh_cloth_templates_initialize_00100fb8(gh_cloth_templates*s,int init,int priority){
 if(!s||!s->loaded)return GH_CLOTH_ARGUMENT;if(priority!=0xffff||!init)return GH_CLOTH_OK;
 for(unsigned i=0;i<42;i++){gh_cloth_sphere*p=s->records+i;const initial_sphere*q=initial+i;memset(p,0,sizeof(*p));
  p->bone_a=q->a;p->bone_b=q->b;p->blend=q->blend;p->radius=q->radius;
  p->offset_a[0]=q->ax;p->offset_a[1]=q->ay;p->offset_a[2]=q->az;p->offset_a[3]=1;
  p->offset_b[0]=q->bx;p->offset_b[1]=q->by;p->offset_b[2]=q->bz;p->offset_b[3]=1;
 }s->initialized=1;return GH_CLOTH_OK;
}
gh_cloth_status gh_cloth_templates_for_costume(gh_cloth_templates*s,unsigned costume,gh_cloth_sphere**out,size_t*count){
 if(!s||!s->loaded||!s->initialized||!out||!count)return GH_CLOTH_ARGUMENT;
 static const uint8_t starts[4]={0,12,23,30},counts[4]={12,11,7,12};
 if(costume>7)return GH_CLOTH_UNSUPPORTED;unsigned i=costume>>1;*out=s->records+starts[i];*count=counts[i];return GH_CLOTH_OK;
}
