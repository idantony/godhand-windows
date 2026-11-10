/* Actual ELF bounded render prefix. Upstream MIT pin
 * eaae18d0dfab1f9a0df276ae5ca1581ad326faf3. */
#include "game_render_steps.h"
#include <string.h>
_Static_assert(sizeof(gh_render_draw_scalars)==0x2c,"original draw scalar span");
static uint32_t u32(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static unsigned u16(const uint8_t*p){return p[0]|(unsigned)p[1]<<8;}
static gh_render_step_status mapped(const uint8_t*e,size_t z,uint32_t va,void*out,size_t n){
 if(z<52)return GH_RENDER_STEP_BOUNDS;
 if(memcmp(e,"\177ELF\1\1\1",7)||u16(e+18)!=8)return GH_RENDER_STEP_FORMAT;
 uint32_t table=u32(e+28);unsigned stride=u16(e+42),count=u16(e+44);
 if(stride<32||count>128||table>z||count>(z-table)/stride)return GH_RENDER_STEP_BOUNDS;
 for(unsigned i=0;i<count;i++){const uint8_t*p=e+table+(size_t)i*stride;
  uint32_t off=u32(p+4),base=u32(p+8),files=u32(p+16),mem=u32(p+20);
  if(u32(p)!=1||base>va||(uint64_t)va+n>(uint64_t)base+mem)continue;
  if(files>mem||off>z||files>z-off)return GH_RENDER_STEP_BOUNDS;
  uint32_t delta=va-base;memset(out,0,n);if(delta<files){size_t take=files-delta;if(take>n)take=n;memcpy(out,e+off+delta,take);}return GH_RENDER_STEP_OK;
 }return GH_RENDER_STEP_BOUNDS;
}
gh_render_step_status gh_render_draw_env_from_elf(gh_render_draw_env*out,const void*data,size_t z){
 if(!out||!data)return GH_RENDER_STEP_ARGUMENT;uint8_t pointer[4],raw[0x34];
 gh_render_step_status r=mapped(data,z,0x3c2fb0,pointer,4);if(r)return r;
 if(u32(pointer)!=0x5fec20)return GH_RENDER_STEP_UNSUPPORTED;
 r=mapped(data,z,u32(pointer),raw,sizeof(raw));if(r)return r;
 if(u32(raw)||u32(raw+4))return GH_RENDER_STEP_UNSUPPORTED;
 gh_render_draw_env n={0};memcpy(&n.scalar,raw+8,sizeof(n.scalar));n.ready=1;*out=n;return GH_RENDER_STEP_OK;
}
gh_render_step_status gh_render_draw_init_002c54e8(gh_render_draw_env*s){
 if(!s||!s->ready)return GH_RENDER_STEP_ARGUMENT;
 s->scalar.shift_x=s->scalar.shift_y=s->scalar.scale_x=s->scalar.scale_y=1;
 s->scalar.alpha=s->scalar.red=s->scalar.green=s->scalar.blue=1;
 s->scalar.bank=6;s->scalar.depth=5;s->scalar.clip=0xffff;return GH_RENDER_STEP_OK;
}
static gh_render_step_status stop(uint32_t*f,uint32_t address,gh_render_step_status r){if(f)*f=address;return r;}
gh_render_step_status gh_render_messages_002af028(const gh_game_scene_ui*s,uint32_t*f){
 if(f)*f=0;if(!s||!s->ready||!s->root)return GH_RENDER_STEP_ARGUMENT;
 if(s->active_head==-1)return GH_RENDER_STEP_OK;
 if(s->active_head<0||s->active_head>=16)return stop(f,0x2af028,GH_RENDER_STEP_BOUNDS);
 return stop(f,0x2b16f0,GH_RENDER_STEP_UNSUPPORTED);
}
gh_render_step_status gh_render_particles_002fb310(const gh_boot_work*w,
 const gh_game_effects*s,const uint8_t*a,size_t z,uint32_t*f,uint32_t*slot){
 if(f)*f=0;if(slot)*slot=UINT32_MAX;if(!w)return GH_RENDER_STEP_ARGUMENT;
 const uint8_t*r=(const uint8_t*)w;
 if((u32(r+0x5e0)&0x10000000)||(u32(r+0x58c)&0x400))return GH_RENDER_STEP_OK;
 if(!s||!s->ready||!a)return GH_RENDER_STEP_ARGUMENT;
 if(z<0x100180)return GH_RENDER_STEP_BOUNDS;
 for(unsigned i=0;i<1024;i++){unsigned o=(i>>5)*4;uint32_t mask=0x80000000u>>(i&31);
  if((u32(a+0x100000+o)&mask)&&!(u32(a+0x100080+o)&mask)&&!(u32(a+0x100100+o)&mask)){
   /* Actor+114 and virtual consumer need typed bindings before we can know
    * whether this eligible slot is further suppressed. Do not claim its
    * potential2FB470 call has already happened. */
   if(slot)*slot=i;return stop(f,0x2fb310,GH_RENDER_STEP_UNSUPPORTED);
  }
 }return GH_RENDER_STEP_OK;
}
gh_render_step_status gh_render_cockpit_001410a8_gate(const gh_boot_work*w,uint32_t*f){
 if(f)*f=0;if(!w)return GH_RENDER_STEP_ARGUMENT;const uint8_t*r=(const uint8_t*)w;
 if(u32(r+0x58c)&0x80000000)return GH_RENDER_STEP_OK;
 if(!(u32(r+0x584)&8)||(u32(r+0x594)&0x800))return stop(f,(u32(r+0x5e0)&0x2000000)?0x13f208:0x13cf20,GH_RENDER_STEP_UNSUPPORTED);
 return stop(f,0x1410a8,GH_RENDER_STEP_UNSUPPORTED);
}
gh_render_step_status gh_render_menu_00160b90_gate(const gh_boot_work*w,uint32_t*f){
 if(f)*f=0;if(!w)return GH_RENDER_STEP_ARGUMENT;const uint8_t*r=(const uint8_t*)w;
 if((u32(r+0x58c)&0x80000000)||(u32(r+0x5dc)&0x100000))return GH_RENDER_STEP_OK;
 return stop(f,0x160d50,GH_RENDER_STEP_UNSUPPORTED);
}
