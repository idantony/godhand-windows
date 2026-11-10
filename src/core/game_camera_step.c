/* Original readable camera manager path slice. Upstream MIT LICENSE.upstream;
 * pin eaae18d0dfab1f9a0df276ae5ca1581ad326faf3. Actual patched ELF evidence
 * analysis/game-camera-step-evidence.json; no guest-address execution. */
#include "game_camera_step.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
static uint32_t word(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static void put(uint8_t*p,uint32_t n){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(n>>(8*i));}
static float number(const uint8_t*p){uint32_t n=word(p);float x;memcpy(&x,&n,4);return x;}
static void store(uint8_t*p,float x){uint32_t n;memcpy(&n,&x,4);put(p,n);}
static float blend(float old,float target,float weight){volatile float delta=target-old;volatile float product=delta*weight;volatile float result=old+product;return result;}
static gh_camera_status stop(uint32_t*f,uint32_t pc,gh_camera_status e){if(f)*f=pc;return e;}
void gh_game_camera_path_construct_00137a48(gh_acd_scene_binding*p){
 if(!p)return;p->records=NULL;p->record_bytes=0;p->field30=0;p->frontier=0;
 memset(p->scalar04,0,sizeof(p->scalar04));put(p->scalar04+0x18,0x3f87ae14);
}
gh_camera_status gh_game_camera_path_select_00137bb8(gh_acd_scene_binding*p,const gh_title_player_view*player,int*out,uint32_t*f){
 if(f)*f=0;if(!p||!out||!player||!player->bytes||player->size<GH_TITLE_PLAYER_BYTES||player->alias_f0!=player->bytes+0xb0)return GH_CAMERA_ARGUMENT;
 *out=0;p->frontier=0;
 if(!p->records)return GH_CAMERA_OK; /* original preserves selection here */
 if(word(p->scalar04+8)){p->frontier=p->field30?0x137bb8:0x1f83e8;return stop(f,p->frontier,GH_CAMERA_UNSUPPORTED);}
 p->field30=0;return GH_CAMERA_OK; /* real nonnull file with zero regions */
}
gh_camera_status gh_game_camera_path_step_00137f38(gh_game_pl_camera*c,gh_acd_scene_binding*p,const gh_title_player_view*player,uint32_t*f){
 int original;fenv_t env;if(f)*f=0;if(!c||!c->constructed||!p)return GH_CAMERA_ARGUMENT;
 gh_camera_status e=gh_game_camera_path_select_00137bb8(p,player,&original,f);if(e)return e;
 if(p->field30)return stop(f,0x137f38,GH_CAMERA_UNSUPPORTED);
 if(c->policy!=GH_CAMERA_NATIVE_BINARY32_REFERENCE)return stop(f,0x137f38,GH_CAMERA_UNSUPPORTED);
 float old450=number(c->fields+0x450),old460=number(c->fields+0x460);
 float target450=number(p->scalar04+0x10),target460=number(p->scalar04+0xc),weight=number(p->scalar04+0x14);
 if(!isfinite(old450)||!isfinite(old460)||!isfinite(target450)||!isfinite(target460)||!isfinite(weight))return stop(f,0x137f38,GH_CAMERA_NUMERIC);
 if(feholdexcept(&env))return stop(f,0x137f38,GH_CAMERA_NUMERIC);
 if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return stop(f,0x137f38,GH_CAMERA_NUMERIC);}
 float v450=blend(old450,target450,weight),v460=blend(old460,target460,weight);
 if(fesetenv(&env))return stop(f,0x137f38,GH_CAMERA_NUMERIC);
 if(!isfinite(v450)||!isfinite(v460))return stop(f,0x137f38,GH_CAMERA_NUMERIC);
 store(c->fields+0x450,v450);store(c->fields+0x460,v460);put(c->fields+0x444,word(p->scalar04+0x18));return GH_CAMERA_OK;
}
gh_camera_status gh_game_camera_step_00138fc8(gh_game_pl_camera*c,gh_game_init_state*init,gh_acd_scene_binding*p,const gh_title_player_view*player,const gh_boot_work*work,uint32_t*f){
 if(f)*f=0;if(!c||!init||!work||c->identity!=&init->camera)return GH_CAMERA_ARGUMENT;
 gh_camera_status e=gh_game_camera_path_step_00137f38(c,p,player,f);if(e)return e;
 if((uint32_t)init->camera.mode&4u)return GH_CAMERA_OK;
 if(init->active_camera!=c->identity||word(c->fields+0x35c)!=0x41dbb0)return stop(f,0x138600,GH_CAMERA_UNSUPPORTED);
 /* Actual139C88 high/sign gate precedes subtype dispatch. Persistent view
  * scalars remain untouched at a consumer we have not recovered yet. */
 if(!(word((const uint8_t*)work+0x5d8)&0x80800000u)){
  uint32_t mode=word(c->fields+0x520);
  if(mode==1)return stop(f,0x13b388,GH_CAMERA_UNSUPPORTED);
  if(mode)return stop(f,0x139c88,GH_CAMERA_UNSUPPORTED);
  if(player->initial_memory==GH_TITLE_MEMORY_UNSPECIFIED)return stop(f,0x139fa8,GH_CAMERA_UNSUPPORTED);
  return stop(f,word(player->bytes+0x5d8)==4?0x13ad90:0x139ff8,GH_CAMERA_UNSUPPORTED);
 }
 /* 138558 genuine early returns when both shake counters are zero. */
 if(c->fields[0x278]||c->fields[0x264])return stop(f,0x138558,GH_CAMERA_UNSUPPORTED);
 return stop(f,0x138740,GH_CAMERA_UNSUPPORTED); /* real projection/matrix work */
}
