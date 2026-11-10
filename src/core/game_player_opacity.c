/* Actual patched SLUS_215.03 00123160; upstream eaae18d0 MIT provenance.
 * Native aliases replace guest F0/68C. Only original opacity scalars change. */
#include "game_player_opacity.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
static uint32_t word(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
static unsigned half(const uint8_t*p){return p[0]|(unsigned)p[1]<<8;}
static float number(const uint8_t*p){float n;memcpy(&n,p,4);return n;}
static void store(uint8_t*p,uint32_t n){memcpy(p,&n,4);}
static float add(float a,float b){volatile float n=a+b;return n;}
static float sub(float a,float b){volatile float n=a-b;return n;}
static float mul(float a,float b){volatile float n=a*b;return n;}
static float divide(float a,float b){volatile float n=a/b;return n;}
static gh_player_opacity_status fail(gh_player_opacity_call*c,uint32_t pc,gh_player_opacity_status e){c->frontier=pc;return e;}
gh_player_opacity_status gh_player_opacity_globals_from_elf(gh_player_opacity_globals*g,const void*raw,size_t bytes){
 if(!g||!raw)return GH_OPACITY_ARGUMENT;if(g->loaded)return GH_OPACITY_BUSY;const uint8_t*b=raw;
 if(bytes<52||memcmp(b,"\177ELF\1\1\1",7)||half(b+18)!=8)return GH_OPACITY_ARGUMENT;
 size_t table=word(b+28);unsigned stride=half(b+42),count=half(b+44);if(stride<32||count>128||table>bytes||count>(bytes-table)/stride)return GH_OPACITY_BOUNDS;
 const uint8_t*source=NULL;
 for(unsigned i=0;i<count;i++){const uint8_t*p=b+table+(size_t)i*stride;if(word(p)!=1)continue;
  uint32_t offset=word(p+4),base=word(p+8),files=word(p+16),memory=word(p+20);
  if(files>memory||offset>bytes||files>bytes-offset||(uint64_t)base+memory>UINT64_C(0x100000000))return GH_OPACITY_BOUNDS;
  if(base<=0x3bd490&&(uint64_t)0x3bd4a0<=(uint64_t)base+files){if(source)return GH_OPACITY_BOUNDS;source=b+offset+(0x3bd490-base);}
 }
 if(!source)return GH_OPACITY_BOUNDS;for(unsigned i=0;i<4;i++)if(!isfinite(number(source+i*4)))return GH_OPACITY_NUMERIC;
 memcpy(g->bytes,source,16);g->loaded=1;return GH_OPACITY_OK;
}
static void publish(uint8_t*p,uint32_t alpha,int transparent){store(p+0x24c,alpha);uint32_t f=word(p+0x250);store(p+0x250,transparent?f|16u:f&~16u);}
static gh_player_opacity_status calculate(gh_player_opacity_call*c,const gh_title_player_view*v,const gh_player_opacity_bindings*b){
 const uint8_t*p=v->bytes;float alpha=1;
 if(word(p+0x15f4)&0x1000){c->opacity_bits=0;c->transparent=1;return GH_OPACITY_OK;}
 if(!b->init)return fail(c,0x138f88,GH_OPACITY_UNSUPPORTED);
 if((uint32_t)b->init->camera.mode&1u){
  const gh_game_pl_camera*camera=b->camera;
  if(!camera||!camera->constructed||camera->identity!=&b->init->camera)return fail(c,0x1231d4,GH_OPACITY_UNSUPPORTED);
  uint32_t mode=word(camera->fields+0x520);if(mode!=0&&mode!=1){c->stage=2;return GH_OPACITY_OK;}
  if(mode==1||(word(camera->fields+0x528)&2u)){
   if(b->policy!=GH_CAMERA_NATIVE_BINARY32_REFERENCE)return fail(c,0x1231e8,GH_OPACITY_UNSUPPORTED);
   if(!b->globals||!b->globals->loaded)return fail(c,mode?0x12322c:0x123348,GH_OPACITY_UNSUPPORTED);
   if(!v->alias_f0)return fail(c,0x1231ec,GH_OPACITY_ARGUMENT);
   float x=number(camera->fields+0x210),z=number(camera->fields+0x218),px=number(v->alias_f0),pz=number(v->alias_f0+8);
   unsigned offset=mode?0:8;float near=number(b->globals->bytes+offset),far=number(b->globals->bytes+offset+4);
   if(!isfinite(x)||!isfinite(z)||!isfinite(px)||!isfinite(pz)||!isfinite(near)||!isfinite(far))return fail(c,0x123220,GH_OPACITY_NUMERIC);
   fenv_t env;if(fegetenv(&env)||fesetround(FE_TONEAREST))return fail(c,0x123220,GH_OPACITY_UNSUPPORTED);
   float dx=sub(x,px),dz=sub(z,pz),sum=add(mul(dx,dx),mul(dz,dz)),distance=sqrtf(sum),denominator=sub(far,near);
   alpha=divide(sub(distance,near),denominator);int valid=isfinite(sum)&&isfinite(distance)&&denominator!=0&&isfinite(alpha);
   if(valid){if(alpha<0)alpha=0;if(alpha>=1)alpha=1;}
   if(fesetenv(&env))return fail(c,0x123220,GH_OPACITY_UNSUPPORTED);
   if(!valid)return fail(c,0x123248,GH_OPACITY_NUMERIC);
  }
 }
 memcpy(&c->opacity_bits,&alpha,4);c->transparent=alpha<=0.99f;return GH_OPACITY_OK;
}
gh_player_opacity_status gh_player_opacity_00123160(gh_player_opacity_call*c,const gh_title_player_view*v,const gh_player_opacity_bindings*b){
 if(!c||!v||!b||!v->bytes||v->size<0x15f8)return GH_OPACITY_ARGUMENT;
 if(c->player_bytes&&c->player_bytes!=v->bytes)return GH_OPACITY_ARGUMENT;c->player_bytes=v->bytes;c->frontier=0;
 if(c->stage==2)return GH_OPACITY_OK;if(c->stage>2)return GH_OPACITY_ARGUMENT;
 if(!c->stage){gh_player_opacity_status e=calculate(c,v,b);if(e||c->stage==2)return e;publish(v->bytes,c->opacity_bits,c->transparent);c->stage=1;}
 if(!b->auxiliary_binding_known)return fail(c,0x123194,GH_OPACITY_UNSUPPORTED);
 if(b->aux_68c){if(!b->aux_68c->bytes||b->aux_68c->size<0x254)return fail(c,0x12319c,GH_OPACITY_BOUNDS);publish(b->aux_68c->bytes,c->opacity_bits,c->transparent);}
 else if(word(v->bytes+0x68c))return fail(c,0x123194,GH_OPACITY_UNSUPPORTED);
 c->stage=2;return GH_OPACITY_OK;
}
