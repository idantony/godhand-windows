#include "game_player_targets.h"
#include "game_scene_at_geometry.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
static uint32_t w(const void*p){uint32_t n;memcpy(&n,p,4);return n;}
static unsigned half(const uint8_t*p){return p[0]|(unsigned)p[1]<<8;}
static float number(uint32_t n){float f;memcpy(&f,&n,4);return f;}
static float add(float a,float b){volatile float n=a+b;return n;}
static float sub(float a,float b){volatile float n=a-b;return n;}
static float mul(float a,float b){volatile float n=a*b;return n;}
static float divide(float a,float b){volatile float n=a/b;return n;}
static gh_player_target_status fail(uint32_t*f,uint32_t pc,gh_player_target_status e){if(f)*f=pc;return e;}
static const uint8_t*elf_span(const void*image,size_t n,uint32_t address,size_t bytes){const uint8_t*p=image;if(!p||n<52||memcmp(p,"\177ELF\1\1\1",7)||half(p+18)!=8||half(p+42)!=32)return NULL;uint32_t ph=w(p+28);unsigned count=half(p+44);if(count>128||ph>n||(size_t)count*32>n-ph)return NULL;for(unsigned i=0;i<count;i++){const uint8_t*h=p+ph+i*32;uint32_t base=w(h+8),size=w(h+16),off=w(h+4);if(w(h)!=1||address<base||(uint64_t)address+bytes>(uint64_t)base+size)continue;uint64_t o=(uint64_t)off+address-base;if(o>n||bytes>n-o)return NULL;return p+(size_t)o;}return NULL;}
static int constants(const void*elf,size_t size,float*t){const uint8_t*p=elf_span(elf,size,0x45ea08,76);if(!p)return 0;for(unsigned i=0;i<19;i++){t[i]=number(w(p+i*4));if(!isfinite(t[i]))return 0;}return 1;}
/* Actual SDK3A1648 float atan. Coefficients are read from original image. */
static float atan_sdk(float input,const float*t){uint32_t bits;memcpy(&bits,&input,4);uint32_t magnitude=bits&0x7fffffffu;if(magnitude>0x7f800000)return add(input,input);if(magnitude>0x507fffff)return bits&0x80000000u?sub(-t[3],t[7]):add(t[3],t[7]);float x=input;int id=-1;if(magnitude<0x3ee00000){if(magnitude<=0x30ffffff)return input;}else{x=fabsf(x);if(magnitude<0x3f980000){if(magnitude<0x3f300000){id=0;x=divide(sub(add(x,x),1),add(x,2));}else{id=1;x=divide(sub(x,1),add(x,1));}}else if(magnitude<0x401c0000){id=2;x=divide(sub(x,1.5f),add(mul(x,1.5f),1));}else{id=3;x=divide(-1,x);}}
 float z=mul(x,x),q=mul(z,z),even=t[18];for(int i=16;i>=8;i-=2)even=add(t[i],mul(q,even));even=mul(z,even);float odd=t[17];for(int i=15;i>=9;i-=2)odd=add(t[i],mul(q,odd));odd=mul(q,odd);float sum=add(even,odd);if(id<0)return sub(x,mul(x,sum));float result=sub(t[id],sub(sub(mul(x,sum),t[id+4]),x));return bits&0x80000000u?-result:result;
}
static gh_player_target_status angle(const float*t,const float*from,const float*to,float heading,float limit,float*out){if(!isfinite(heading)||!isfinite(limit)||limit<0)return GH_PLAYER_TARGET_NUMERIC;for(unsigned k=0;k<3;k++)if(!isfinite(from[k])||!isfinite(to[k]))return GH_PLAYER_TARGET_NUMERIC;float x=sub(to[0],from[0]),z=sub(to[2],from[2]);if(!isfinite(x)||!isfinite(z))return GH_PLAYER_TARGET_NUMERIC;if(x==0&&z==0){*out=0;return GH_PLAYER_TARGET_OK;}float delta;if(z==0)delta=sub(x<=0?number(0xbfc90fdb):number(0x3fc90fdb),heading);else {float bearing=atan_sdk(divide(x,z),t);if(z<0)bearing=sub(bearing,number(0x40490fdb));delta=sub(bearing,heading);}if(delta>number(0x40490fdb))delta=sub(delta,number(0x40c90fdb));if(delta<number(0xc0490fdb))delta=add(delta,number(0x40c90fdb));float sign=delta>0?1:-1,absolute=delta>0?delta:-delta;float result=mul(absolute<=limit?absolute:limit,sign);if(!isfinite(result))return GH_PLAYER_TARGET_NUMERIC;*out=result;return GH_PLAYER_TARGET_OK;}
gh_player_target_status gh_player_target_angle_002da038(const void*elf,size_t size,const float*from,const float*to,float heading,float limit,float*out,uint32_t*f){if(f)*f=0;if(!from||!to||!out)return GH_PLAYER_TARGET_ARGUMENT;float t[19];if(!constants(elf,size,t))return fail(f,0x3a1648,GH_PLAYER_TARGET_ARGUMENT);fenv_t env;if(feholdexcept(&env))return GH_PLAYER_TARGET_NUMERIC;if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return GH_PLAYER_TARGET_NUMERIC;}float value;gh_player_target_status e=angle(t,from,to,heading,limit,&value);if(fesetenv(&env))return GH_PLAYER_TARGET_NUMERIC;if(e)return fail(f,0x2da038,e);*out=value;return GH_PLAYER_TARGET_OK;}
static int point(const gh_title_player_view*v,float*out){if(!v->bytes||!v->alias_f0||(uintptr_t)v->alias_f0<(uintptr_t)v->bytes||(uintptr_t)v->alias_f0-(uintptr_t)v->bytes>v->size||12>v->size-((uintptr_t)v->alias_f0-(uintptr_t)v->bytes))return 0;memcpy(out,v->alias_f0,12);for(unsigned k=0;k<3;k++)if(!isfinite(out[k]))return 0;return 1;}
static float distance(const float*a,const float*b){float x=sub(a[0],b[0]),y=sub(a[1],b[1]),z=sub(a[2],b[2]);return add(add(mul(x,x),mul(y,y)),mul(z,z));}
static int pickup_kind(unsigned id){switch(id){case 0x361:case 0x362:case 0x363:case 0x368:case 0x36d:case 0x375:case 0x377:case 0x378:case 0x379:case 0x37b:case 0x380:case 0x390:case 0x3a8:case 0x3a9:case 0x3aa:case 0x3ab:case 0x3ad:case 0x3ae:case 0x3af:case 0x3b4:case 0x3b7:return 1;default:return 0;}}
static gh_player_target_status query(const gh_player_target_bindings*b,int kind,gh_player_target*out,uint32_t*f){const gh_room_vector*v=NULL;if(!b->pools||gh_room_registry_view(b->pools,0x754c58,&v)||!v||v->count>v->capacity||(v->count&&!v->allocation.pointer))return fail(f,0x129cc8,GH_PLAYER_TARGET_ARGUMENT);gh_player_target selected={0};if(!v->count){*out=selected;return GH_PLAYER_TARGET_OK;}if(!b->resolve)return fail(f,kind==0?0x12a610:kind==1?0x12b328:0x129cc8,GH_PLAYER_TARGET_UNSUPPORTED);float origin[4],matrix[16],local[3]={0,0,kind==1?.8f:0},player_point[3],table[19];for(unsigned k=0;k<16;k++){matrix[k]=number(w(b->player->bytes+0x80+k*4));if(!isfinite(matrix[k]))return fail(f,0x30a2b0,GH_PLAYER_TARGET_NUMERIC);}if(!point(b->player,player_point))return fail(f,0x2db6b0,GH_PLAYER_TARGET_ARGUMENT);if(gh_scene_at_apply_world_001508d8(matrix,local,origin))return fail(f,0x30a2b0,GH_PLAYER_TARGET_NUMERIC);int have_constants=0;float nearest=2.25f;
 for(uint32_t i=0;i<v->count;i++){uint32_t handle=w((const uint8_t*)v->allocation.pointer+i*4);gh_title_player_view candidate={0};gh_player_target_status e=b->resolve(b->context,handle,&candidate,f);if(e)return e;if(!candidate.bytes||candidate.size<0x300)return fail(f,kind==0?0x12a610:kind==1?0x12b328:0x129cc8,GH_PLAYER_TARGET_ARGUMENT);uint32_t flags=w(candidate.bytes+0x250);if(flags&0x80008000u)continue;unsigned id=half(candidate.bytes+0x2fe);if(id<0x300||id>=0x500)continue;
  if(kind==2){if(id!=0x364)continue;if(candidate.size<0x604)return fail(f,0x1b1620,GH_PLAYER_TARGET_ARGUMENT);if(w(candidate.bytes+0x600))continue;return fail(f,0x147568,GH_PLAYER_TARGET_UNSUPPORTED);}
  if(kind==0){if(!pickup_kind(id))continue;if(candidate.size<0x666)return fail(f,0x1d07e0,GH_PLAYER_TARGET_ARGUMENT);if(!candidate.bytes[0x664])continue;}else {if(candidate.size<0x5b4)return fail(f,0x1b79d0,GH_PLAYER_TARGET_ARGUMENT);if(!(w(candidate.bytes+0x5b0)&8)||b->player->bytes[0x2f4]!=0)continue;}
  float target[3];if(!point(&candidate,target))return fail(f,0x1f7eb0,GH_PLAYER_TARGET_ARGUMENT);float squared=distance(origin,target);if(!isfinite(squared))return fail(f,0x1f7eb0,GH_PLAYER_TARGET_NUMERIC);if(squared>(kind==0?4:nearest))continue;float height=kind==0?sub(target[1],player_point[1]):fabsf(sub(player_point[1],target[1]));int lifted=kind==0&&candidate.bytes[0x665];if(lifted)height=sub(height,1);if(kind==0?(height>1.7f||height<-.5f):(height>.5f))continue;
  if(kind==1||lifted){if(!have_constants){if(!constants(b->elf,b->elf_bytes,table))return fail(f,0x3a1648,GH_PLAYER_TARGET_ARGUMENT);have_constants=1;}float delta;e=angle(table,player_point,target,number(w(b->player->bytes+0x104)),number(0x40490fdb),&delta);if(e)return fail(f,0x2da038,e);if(fabsf(delta)>(kind==0?number(0x3f060a92):number(0x3f860a92)))continue;}
  if(!b->actors)return fail(f,0x131e10,GH_PLAYER_TARGET_ARGUMENT);gh_scene_ray_request ray={.mask=7,.flags=kind==0?0x18:0x10};if(kind==0)memcpy(ray.from,player_point,12);else memcpy(ray.from,b->player->bytes+0x490,12);memcpy(ray.to,target,12);ray.from[1]=add(ray.from[1],.5f);ray.to[1]=add(ray.to[1],.5f);ray.from[3]=ray.to[3]=1;int hit;gh_scene_ray_status r=gh_scene_ray_query_00131e10(b->actors,&ray,&hit,f);if(r)return r==GH_RAY_UNSUPPORTED?GH_PLAYER_TARGET_UNSUPPORTED:GH_PLAYER_TARGET_CALLEE;if(hit)continue;selected=(gh_player_target){handle,candidate};if(kind==0){*out=selected;return GH_PLAYER_TARGET_OK;}nearest=squared;
 }*out=selected;return GH_PLAYER_TARGET_OK;}
static gh_player_target_status enter_query(const gh_player_target_bindings*b,int kind,gh_player_target*out,uint32_t*f){if(f)*f=0;if(!b||!out||!b->player||!b->player->bytes||b->player->size<0x4a0)return GH_PLAYER_TARGET_ARGUMENT;fenv_t env;if(feholdexcept(&env))return GH_PLAYER_TARGET_NUMERIC;if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return GH_PLAYER_TARGET_NUMERIC;}gh_player_target result;gh_player_target_status e=query(b,kind,&result,f);if(fesetenv(&env))return GH_PLAYER_TARGET_NUMERIC;if(!e)*out=result;return e;}
gh_player_target_status gh_player_pickup_target_0012a610(const gh_player_target_bindings*b,gh_player_target*out,uint32_t*f){return enter_query(b,0,out,f);}
gh_player_target_status gh_player_interact_target_0012b328(const gh_player_target_bindings*b,gh_player_target*out,uint32_t*f){return enter_query(b,1,out,f);}
gh_player_target_status gh_player_small_object_gate_00129cc8(const gh_player_target_bindings*b,uint32_t*f){gh_player_target unused;return enter_query(b,2,&unused,f);}

gh_player_target_status gh_enemy_target_position_00275978(const gh_title_player_view *view,float out[3])
{
 if(!view||!out||view->original_vtable214!=0x4479c8||view->size<0x168c)
  return GH_PLAYER_TARGET_UNSUPPORTED;
 if(!point(view,out))return GH_PLAYER_TARGET_ARGUMENT;
 if(view->bytes+0x1680!=view->alias_f0)memcpy(view->bytes+0x1680,out,12);
 unsigned kind=w(view->bytes+0x564);
 if(kind==0x264||kind==0x265||kind==0x26a)return GH_PLAYER_TARGET_UNSUPPORTED;
 return GH_PLAYER_TARGET_OK;
}

static int om_class_001b7a78(unsigned id,uint32_t item)
{
 if(item==0x9c3)return 0;
 if(item!=0xffff)return 1;
 return (id>=0x301&&id<=0x302)||(id>=0x309&&id<=0x30c)||id==0x325||
  (id>=0x32a&&id<=0x32c)||(id>=0x32e&&id<=0x32f)||id==0x332||id==0x33d||
  (id>=0x33f&&id<=0x345)||(id>=0x347&&id<=0x348)||(id>=0x400&&id<=0x402)||
  (id>=0x410&&id<=0x411)||(id>=0x418&&id<=0x41a)||id==0x41f||id==0x42e;
}

int gh_player_target_object_eligible_001b79e0(const gh_title_player_view *view)
{
 if(!view||!view->bytes||view->size<0x5bc)return 0;
 const uint8_t *p=view->bytes;
 const uint32_t flags=w(p+0x5b8);
 if((w(p+0x5b0)&2u)||(flags&0x2000u))return 0;
 const unsigned kind=half(p+0x2fe);
 return (flags&0x100u)||(om_class_001b7a78(kind,w(p+0x560))&&
  !(kind>=0x3da&&kind<=0x3dd)&&kind!=0x3ea);
}

static gh_player_target_status consider_focus(const gh_player_target_bindings *b,
 const float origin[3],const float candidate[3],float heading,float range,float max_height,
 float cone_scale,int check_visibility,float *cone,int *accepted,uint32_t *f)
{
 *accepted=0;
 const float x=sub(origin[0],candidate[0]),z=sub(origin[2],candidate[2]);
 const float squared=add(mul(x,x),mul(z,z));
 const float height=fabsf(sub(origin[1],candidate[1]));
 if(!isfinite(squared)||!isfinite(height))return fail(f,0x1f7eb0,GH_PLAYER_TARGET_NUMERIC);
 if(squared>mul(range,range)||height>max_height)return GH_PLAYER_TARGET_OK;

 float turn;
 gh_player_target_status status=gh_player_target_angle_002da038(b->elf,b->elf_bytes,
  origin,candidate,heading,number(0x40490fdb),&turn,f);
 if(status)return status;
 turn=fabsf(turn);
 if(turn>mul(*cone,cone_scale))return GH_PLAYER_TARGET_OK;

 if(check_visibility){
  if(!b->actors)return fail(f,0x131e10,GH_PLAYER_TARGET_ARGUMENT);
  gh_scene_ray_request ray={.mask=7,.flags=0x13c,.segmented=1};
  memcpy(ray.from,origin,12);
  memcpy(ray.to,candidate,12);
  ray.from[1]=add(ray.from[1],1);
  ray.to[1]=add(ray.to[1],1);
  ray.from[3]=ray.to[3]=1;
  int hit;
  gh_scene_ray_status result=gh_scene_ray_query_00131e10(b->actors,&ray,&hit,f);
  if(result)return result==GH_RAY_UNSUPPORTED?GH_PLAYER_TARGET_UNSUPPORTED:GH_PLAYER_TARGET_CALLEE;
  if(hit)return GH_PLAYER_TARGET_OK;
 }
 *cone=turn;
 *accepted=1;
 return GH_PLAYER_TARGET_OK;
}

static gh_player_target_status enemy_focus(const gh_player_target_bindings *b,
 const gh_game_init_state *init,const float origin[3],float heading,float range,float cone,
 int fallback,const gh_title_player_view *exclude,gh_player_target *selected,uint32_t *f)
{
 unsigned cursor=UINT8_MAX;
 if(init->enemy_top){
  for(unsigned i=0;i<64;i++)if(init->enemy_top==init->enemies+i*20){cursor=i;break;}
  if(cursor==UINT8_MAX)return fail(f,0x2910c0,GH_PLAYER_TARGET_ARGUMENT);
 }else if(w(init->enemies+0x500))return fail(f,0x2910c0,GH_PLAYER_TARGET_ARGUMENT);
 uint64_t visited=0;
 const gh_game_title_player *selected_enemy=NULL;
 while(cursor!=UINT8_MAX){
  if(cursor>=64||(visited&(UINT64_C(1)<<cursor)))return fail(f,0x291100,GH_PLAYER_TARGET_ARGUMENT);
  visited|=UINT64_C(1)<<cursor;
  const gh_game_title_player *actor=init->enemy_actors[cursor];
  if(!actor||!actor->view.bytes||actor->view.size<0x618||init->enemies[cursor*20+16]!=1)
   return fail(f,0x291100,GH_PLAYER_TARGET_ARGUMENT);
  cursor=init->enemy_next[cursor];
  const gh_title_player_view *view=&actor->view;
  if(exclude&&view->bytes==exclude->bytes)continue;
  const uint8_t *p=view->bytes;
  const unsigned kind=half(p+0x2fe);
  if(kind<0x200||kind>=0x280||(!fallback&&p[0x617]))continue;
  if(view->original_vtable214!=0x4479c8)return fail(f,0x29117c,GH_PLAYER_TARGET_UNSUPPORTED);
  if(!fallback&&(int16_t)half(p+0x54a)<1&&!(half(p+0x3ac)&4u))continue;

  float candidate[3];
  gh_player_target_status status=gh_enemy_target_position_00275978(view,candidate);
  if(status)return fail(f,0x275978,status);
  const int ignores_height=p[0x2f4]==1||(p[0x2f4]==2&&p[0x2f5]!=2);
  const unsigned dependency=w(p+0x564);
  const int extended=dependency==0x260||dependency==0x264||dependency==0x265||dependency==0x26a;
  int accepted;
  status=consider_focus(b,origin,candidate,heading,add(range,dependency==0x26a?8:extended?4:0),
   ignores_height?INFINITY:1.5f,1,!extended,&cone,&accepted,f);
  if(status)return status;
  if(accepted)selected_enemy=actor;
 }

 const gh_room_vector *registry;
 if(gh_room_registry_view(b->pools,0x754c58,&registry)||registry->count>registry->capacity||
    (registry->count&&!registry->allocation.pointer))return fail(f,0x291510,GH_PLAYER_TARGET_ARGUMENT);
 if(registry->count&&!b->resolve)return fail(f,0x291510,GH_PLAYER_TARGET_UNSUPPORTED);
 for(uint32_t i=0;i<registry->count;i++){
  const uint32_t handle=w((const uint8_t*)registry->allocation.pointer+i*4);
  gh_title_player_view view={0};
  gh_player_target_status status=b->resolve(b->context,handle,&view,f);
  if(status)return status;
  if(!view.bytes||view.size<0x300)return fail(f,0x291518,GH_PLAYER_TARGET_ARGUMENT);
  if(exclude&&view.bytes==exclude->bytes)continue;
  const uint8_t *p=view.bytes;
  const unsigned kind=half(p+0x2fe);
  if((w(p+0x250)&0x80008000u)||kind<0x300||kind>=0x500)continue;
  if(view.size<0x5bc)return fail(f,0x1b79e0,GH_PLAYER_TARGET_ARGUMENT);
  const uint32_t flags=w(p+0x5b8);
  if((!fallback&&!(flags&0x1000u))||(w(p+0x5b0)&4u)||!gh_player_target_object_eligible_001b79e0(&view))continue;

  const uint8_t *method=elf_span(b->elf,b->elf_bytes,view.original_vtable214+0x68,8);
  const uint8_t *position=elf_span(b->elf,b->elf_bytes,view.original_vtable214+0x80,8);
  if(!method||half(method)||w(method+4)!=0x2a93a8||!position||half(position)||w(position+4)!=0x2a9458)
   return fail(f,0x2915ac,GH_PLAYER_TARGET_UNSUPPORTED);
  float candidate[3];
  if(!point(&view,candidate))return fail(f,0x2a9458,GH_PLAYER_TARGET_ARGUMENT);
  const int wide=(flags&0x400u)!=0;
  int accepted;
  status=consider_focus(b,origin,candidate,heading,wide?add(range,range):range,
   wide?1.5f:1,wide?1.25f:1,!(flags&0x80u),&cone,&accepted,f);
  if(status)return status;
  if(accepted){*selected=(gh_player_target){handle,view};selected_enemy=NULL;}
 }
 if(selected_enemy){
  for(size_t i=0;i<b->pools->object_slots;i++){
   const gh_room_object_binding *binding=b->pools->objects+i;
   if(binding->object==selected_enemy){
    *selected=(gh_player_target){binding->handle,selected_enemy->view};
    return GH_PLAYER_TARGET_OK;
   }
  }
  return fail(f,0x2917a0,GH_PLAYER_TARGET_ARGUMENT);
 }
 return GH_PLAYER_TARGET_OK;
}

static gh_player_target_status focus_from(const gh_player_target_bindings *b,
 const gh_game_init_state *init,const float origin[3],float heading,float range,float cone,
 int fallback,const gh_title_player_view *exclude,gh_player_target *out,uint32_t *f)
{
 if(f)*f=0;
 if(!b||!init||!out||!origin||!b->pools||(exclude&&!exclude->bytes))
  return fail(f,0x291010,GH_PLAYER_TARGET_ARGUMENT);
 if(!isfinite(range)||range<0||!isfinite(cone)||cone<0)
  return fail(f,0x291010,GH_PLAYER_TARGET_NUMERIC);
 if(!isfinite(heading))return fail(f,0x2da038,GH_PLAYER_TARGET_NUMERIC);
 for(unsigned i=0;i<3;i++)if(!isfinite(origin[i]))return fail(f,0x1f7eb0,GH_PLAYER_TARGET_NUMERIC);
 fenv_t env;
 if(feholdexcept(&env))return fail(f,0x291010,GH_PLAYER_TARGET_NUMERIC);
 if(fesetround(FE_TOWARDZERO)){
  (void)fesetenv(&env);
  return fail(f,0x291010,GH_PLAYER_TARGET_NUMERIC);
 }
 gh_player_target selected={0};
 gh_player_target_status result=enemy_focus(b,init,origin,heading,range,cone,fallback,exclude,&selected,f);
 if(fesetenv(&env))return fail(f,0x291010,GH_PLAYER_TARGET_NUMERIC);
 if(!result)*out=selected;
 return result;
}

static gh_player_target_status focus(const gh_player_target_bindings *b,
 const gh_game_init_state *init,float range,float cone,int fallback,const gh_title_player_view *exclude,
 gh_player_target *out,uint32_t *f)
{
 if(!b||!b->player||b->player->size<0x108)return fail(f,0x291010,GH_PLAYER_TARGET_ARGUMENT);
 float origin[3];
 if(!point(b->player,origin))return fail(f,0x2db6b0,GH_PLAYER_TARGET_ARGUMENT);
 return focus_from(b,init,origin,number(w(b->player->bytes+0x104)),range,cone,fallback,exclude,out,f);
}

gh_player_target_status gh_enemy_target_from_00291010(const gh_player_target_bindings *b,
 const gh_game_init_state *init,const float origin[3],float yaw,float range,float cone,
 const gh_title_player_view *exclude,gh_player_target *out,uint32_t *f)
{
 return focus_from(b,init,origin,yaw,range,cone,0,exclude,out,f);
}

gh_player_target_status gh_player_enemy_target_00291010(const gh_player_target_bindings *b,
 const gh_game_init_state *init,float range,float cone,gh_player_target *out,uint32_t *f)
{
 return focus(b,init,range,cone,0,NULL,out,f);
}

gh_player_target_status gh_player_enemy_target_excluding_00291010(const gh_player_target_bindings *b,
 const gh_game_init_state *init,float range,float cone,const gh_title_player_view *exclude,
 gh_player_target *out,uint32_t *f)
{
 if(!exclude||!exclude->bytes)return fail(f,0x291010,GH_PLAYER_TARGET_ARGUMENT);
 return focus(b,init,range,cone,0,exclude,out,f);
}

static void attack_target_clear(const gh_title_player_view *player,gh_player_target *current)
{
 const uint32_t zero=0;
 player->bytes[0x648]=0;
 memcpy(player->bytes+0x640,&zero,4);
 *current=(gh_player_target){0};
}

static gh_player_target_status keep_attack_target(const gh_player_target_bindings *b,
 const gh_game_init_state *init,gh_player_target *current,int check_geometry,uint32_t *f)
{
 const gh_title_player_view *player=b->player;
 if((int8_t)player->bytes[0x648]<1){
  attack_target_clear(player,current);
  return GH_PLAYER_TARGET_OK;
 }
 if(!current->handle)
  return w(player->bytes+0x640)?fail(f,0x124c30,GH_PLAYER_TARGET_UNSUPPORTED):GH_PLAYER_TARGET_OK;

 gh_title_player_view *target=&current->view;
 if(!target->bytes||target->size<0x300||w(player->bytes+0x640)!=current->handle)
  return fail(f,0x124c30,GH_PLAYER_TARGET_ARGUMENT);
 unsigned kind=half(target->bytes+0x2fe);
 if(kind>=0x200&&kind<0x300){
  int member=0;
  unsigned cursor=UINT8_MAX;
  if(init->enemy_top){
   for(unsigned i=0;i<64;i++)if(init->enemy_top==init->enemies+i*20){cursor=i;break;}
   if(cursor==UINT8_MAX)return fail(f,0x2909b8,GH_PLAYER_TARGET_ARGUMENT);
  }
  uint64_t visited=0;
  while(cursor!=UINT8_MAX){
   if(cursor>=64||(visited&(UINT64_C(1)<<cursor)))return fail(f,0x2909b8,GH_PLAYER_TARGET_ARGUMENT);
   visited|=UINT64_C(1)<<cursor;
   const gh_game_title_player *entry=init->enemy_actors[cursor];
   if(entry&&entry->view.bytes==target->bytes&&init->enemies[cursor*20+16]==1){member=1;break;}
   cursor=init->enemy_next[cursor];
  }
  if(!member){attack_target_clear(player,current);return GH_PLAYER_TARGET_OK;}
  if(target->size<0x618)return fail(f,0x124c30,GH_PLAYER_TARGET_ARGUMENT);
  if(target->bytes[0x617]){attack_target_clear(player,current);return GH_PLAYER_TARGET_OK;}
  if((int16_t)half(target->bytes+0x54a)<1){
   if(target->original_vtable214!=0x4479c8)return fail(f,0x124ca8,GH_PLAYER_TARGET_UNSUPPORTED);
   if(!(half(target->bytes+0x3ac)&4)){
    attack_target_clear(player,current);
    return GH_PLAYER_TARGET_OK;
   }
  }
 }
 if(kind>=0x300&&kind<0x500&&
    ((w(target->bytes+0x250)&0x80008000u)||!gh_player_target_object_eligible_001b79e0(target))){
  attack_target_clear(player,current);
  return GH_PLAYER_TARGET_OK;
 }
 if(!check_geometry)return GH_PLAYER_TARGET_OK;

 float origin[3],position[3];
 if(!point(player,origin))return fail(f,0x124d34,GH_PLAYER_TARGET_ARGUMENT);
 if(target->original_vtable214==0x4479c8){
  gh_player_target_status status=gh_enemy_target_position_00275978(target,position);
  if(status)return fail(f,0x275978,status);
 }else{
  const uint8_t *method=elf_span(b->elf,b->elf_bytes,target->original_vtable214+0x68,8);
  const uint8_t *data=elf_span(b->elf,b->elf_bytes,target->original_vtable214+0x80,8);
  if(!method||half(method)||w(method+4)!=0x2a93a8||!data||half(data)||w(data+4)!=0x2a9458)
   return fail(f,0x124d60,GH_PLAYER_TARGET_UNSUPPORTED);
  if(!point(target,position))return fail(f,0x2a9458,GH_PLAYER_TARGET_ARGUMENT);
 }
 float x=sub(origin[0],position[0]),z=sub(origin[2],position[2]);
 float squared=add(mul(x,x),mul(z,z));
 if(!isfinite(squared))return fail(f,0x1f7eb0,GH_PLAYER_TARGET_NUMERIC);
 if(squared>.25f){
  float yaw;
  gh_player_target_status status=gh_player_target_angle_002da038(b->elf,b->elf_bytes,
   origin,position,number(w(player->bytes+0x104)),number(0x40490fdb),&yaw,f);
  if(status)return status;
  if(fabsf(yaw)>number(0x3fc90fdb)){
   attack_target_clear(player,current);
   return GH_PLAYER_TARGET_OK;
  }
 }
 if(squared>36)attack_target_clear(player,current);
 return GH_PLAYER_TARGET_OK;
}

gh_player_target_status gh_player_check_attack_target_00124c30(const gh_player_target_bindings *b,
 const gh_game_init_state *init,uint32_t *f)
{
 if(f)*f=0;
 if(!b||!init||!b->player||!b->player->bytes||b->player->size<0x649)
  return fail(f,0x124c30,GH_PLAYER_TARGET_ARGUMENT);
 gh_player_target current={0};
 if((int8_t)b->player->bytes[0x648]>0){
  current.handle=w(b->player->bytes+0x640);
  if(current.handle){
   if(!b->resolve)return fail(f,0x124c30,GH_PLAYER_TARGET_UNSUPPORTED);
   gh_player_target_status status=b->resolve(b->context,current.handle,&current.view,f);
   if(status)return status;
  }
 }
 return keep_attack_target(b,init,&current,0,f);
}

gh_player_target_status gh_player_attack_target_00124540(const gh_player_target_bindings *b,
 const gh_game_init_state *init,gh_player_target *current,uint32_t *f)
{
 if(f)*f=0;
 if(!b||!init||!current||!b->player||!b->player->bytes||b->player->size<0x649)
  return fail(f,0x124540,GH_PLAYER_TARGET_ARGUMENT);
 fenv_t environment;
 if(feholdexcept(&environment))return fail(f,0x124540,GH_PLAYER_TARGET_NUMERIC);
 if(fesetround(FE_TOWARDZERO)){
  (void)fesetenv(&environment);
  return fail(f,0x124540,GH_PLAYER_TARGET_NUMERIC);
 }
 gh_player_target_status status=keep_attack_target(b,init,current,1,f);
 if(fesetenv(&environment))return fail(f,0x124540,GH_PLAYER_TARGET_NUMERIC);
 if(status)return status;
 if(!current->handle){
  static const struct attack_probe {float range;uint32_t cone;int fallback;} ordinary[]={
   {2,0x3fc90fdb,0},{4,0x3f060a92,0},{6,0x3f060a92,0},{4,0x4016cbe4,0},
   {3,0x3f860a92,1}};
  static const struct attack_probe special[]={
   {2,0x3f060a92,1},{4,0x3f060a92,1},{4,0x40490fdb,1}};
  const int alternate=w(b->player->bytes+0x5d8)==4;
  const struct attack_probe *probes=alternate?special:ordinary;
  const size_t count=alternate?sizeof(special)/sizeof(*special):sizeof(ordinary)/sizeof(*ordinary);
  for(size_t i=0;i<count&&!current->handle;i++){
   status=focus(b,init,probes[i].range,number(probes[i].cone),probes[i].fallback,NULL,current,f);
   if(status)return status;
  }
 }
 if(current->handle){
  memcpy(b->player->bytes+0x640,&current->handle,4);
  b->player->bytes[0x648]=20;
 }
 return GH_PLAYER_TARGET_OK;
}
