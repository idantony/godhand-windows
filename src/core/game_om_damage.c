/* Actualpatched1B7568/1B7380; nativealiases replace hit+34 source pointer. */
#include "game_om_damage.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
static uint32_t word(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
static unsigned half(const uint8_t*p){return p[0]|(unsigned)p[1]<<8;}
static int signed_half(const uint8_t*p){unsigned n=half(p);return n<32768?(int)n:(int)n-65536;}
static int64_t signed_word(uint32_t n){return n<UINT32_C(0x80000000)?(int64_t)n:(int64_t)n-INT64_C(4294967296);}
static void put(uint8_t*p,uint32_t n){memcpy(p,&n,4);}static void sh(uint8_t*p,uint16_t n){memcpy(p,&n,2);}
static float add(float a,float b){volatile float n=a+b;return n;}static float mul(float a,float b){volatile float n=a*b;return n;}static float divide(float a,float b){volatile float n=a/b;return n;}
static gh_om_damage_status fail(uint32_t*f,uint32_t pc,gh_om_damage_status e){if(f)*f=pc;return e;}
static gh_om_damage_status validate(const gh_title_player_view*v,const gh_hitarea_object*h){gh_title_player_view*owner=NULL;if(!v||!v->bytes||v->size<0x5b8||!h||h->group!=2||h->size<0x64||gh_hitarea_object_actor(h,&owner)||!owner||owner->bytes!=v->bytes)return GH_OM_DAMAGE_ARGUMENT;return GH_OM_DAMAGE_OK;}
gh_om_damage_status gh_om_classify_damage_001b7568(const gh_title_player_view*v,const gh_hitarea_object*h,uint32_t*f){
 if(f)*f=0;gh_om_damage_status e=validate(v,h);if(e)return e;uint8_t*p=v->bytes;int attack=signed_half(h->bytes+0x46);if(attack>=0x2e&&attack<0x30)put(p+0x5b4,word(p+0x5b4)|1u);
 int skip=attack==7||attack==8||attack==0x15||attack==0x13||attack==0x18||attack==0x1c||attack==0x21||attack==0x22||attack==0x29||attack==0x2a||attack==0x2b||attack==0x31;
 if(!skip){const gh_title_player_view*source=NULL;if(gh_hitarea_object_last_hit_actor(h,&source))return fail(f,0x1b75f0,GH_OM_DAMAGE_UNSUPPORTED);if(!source&&word(h->bytes+0x34))return fail(f,0x1b75f0,GH_OM_DAMAGE_UNSUPPORTED);
  if(source){if(!source->bytes||source->size<0x300)return fail(f,0x1b75fc,GH_OM_DAMAGE_UNSUPPORTED);unsigned kind=half(source->bytes+0x2fe);if(kind>=0x100&&kind<0x1ff&&signed_word(word(h->bytes+0x4c))>=11)put(p+0x5b4,word(p+0x5b4)|2u);}
 }
 if((uint32_t)(attack-8)<4||attack==0x1c||attack==0x30||attack==0x52)put(p+0x5b4,word(p+0x5b4)|4u);return GH_OM_DAMAGE_OK;
}
static gh_om_damage_status force(gh_om_damage_call*c,const gh_title_player_view*v,gh_static_transform_policy policy,uint32_t*f){
 if(policy!=GH_STATIC_BINARY32_REFERENCE)return fail(f,0x101e28,GH_OM_DAMAGE_UNSUPPORTED);
 fenv_t env;if(feholdexcept(&env))return fail(f,0x101e28,GH_OM_DAMAGE_NUMERIC);if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return fail(f,0x101e28,GH_OM_DAMAGE_NUMERIC);}
 gh_om_damage_status e=GH_OM_DAMAGE_OK;float out[3]={0,0,0},sum=0;
 for(unsigned i=0;i<3;i++)if(!isfinite(c->direction[i])){e=GH_OM_DAMAGE_NUMERIC;break;}
 if(!e){sum=add(add(mul(c->direction[0],c->direction[0]),mul(c->direction[1],c->direction[1])),mul(c->direction[2],c->direction[2]));float len=sqrtf(sum);if(!isfinite(len))e=GH_OM_DAMAGE_NUMERIC;else if(len>0){float inverse=divide(1,len);for(unsigned i=0;i<3;i++)out[i]=mul(c->direction[i],inverse);}}
 int64_t damage=signed_word(c->damage_bits);if(damage>60)damage=60;if(damage<-16777216)e=GH_OM_DAMAGE_UNSUPPORTED;
 if(!e){float factor=(float)damage;for(unsigned i=0;i<3;i++){out[i]=mul(out[i],factor);if(!isfinite(out[i]))e=GH_OM_DAMAGE_NUMERIC;}}
 if(fesetenv(&env))e=GH_OM_DAMAGE_NUMERIC;if(e)return fail(f,damage<-16777216?0x1b74f8:0x101e28,e);memcpy(v->bytes+0x5c0,out,12);return GH_OM_DAMAGE_OK;
}
gh_om_damage_status gh_om_check_damage_001b7380(gh_om_damage_call*c,const gh_title_player_view*v,const gh_hitarea_object*h,const uint8_t*double_damage,gh_static_transform_policy policy,int*out,uint32_t*f){
 if(f)*f=0;if(!c||!v||!v->bytes||v->size<0x5cc||!out)return GH_OM_DAMAGE_ARGUMENT;*out=0;
 if(c->bound&&(c->actor_bytes!=v->bytes||c->has_hit!=(h!=NULL)||(h&&(c->hit.bytes!=h->bytes||c->hit.state!=h->state||c->hit.generation!=h->generation||c->hit.serial!=h->serial))))return GH_OM_DAMAGE_ARGUMENT;
 if(!c->bound){c->actor_bytes=v->bytes;c->has_hit=h!=NULL;if(h)c->hit=*h;c->bound=1;}c->frontier=0;
 if(c->phase==4){*out=c->original_hit;return GH_OM_DAMAGE_OK;}if(c->phase>4)return GH_OM_DAMAGE_ARGUMENT;
 uint8_t*p=v->bytes;
 if(c->phase==0){if((word(p+0x5b8)&2)||!h){c->phase=4;return GH_OM_DAMAGE_OK;}if(validate(v,h))return GH_OM_DAMAGE_ARGUMENT;if(!(word(h->bytes+0x60)&1)){c->phase=4;return GH_OM_DAMAGE_OK;}c->phase=1;}else if(h&&validate(v,h))return GH_OM_DAMAGE_ARGUMENT;
 if(c->phase==1){uint32_t pc=0;gh_om_damage_status e=gh_om_classify_damage_001b7568(v,h,&pc);if(e){c->frontier=pc;if(f)*f=pc;return e;}if((word(p+0x5b8)&0x800)&&!(word(p+0x5b4)&3)){c->phase=4;return GH_OM_DAMAGE_OK;}c->phase=2;}
 if(c->phase==2){uint32_t damage=word(h->bytes+0x4c);if(damage&&!double_damage){c->frontier=0x1b740c;return fail(f,c->frontier,GH_OM_DAMAGE_UNSUPPORTED);}if(double_damage&&*double_damage)damage<<=1;c->damage_bits=damage;
  uint32_t after=(uint32_t)signed_half(p+0x54a)-damage;int64_t hp=signed_word(after);sh(p+0x54a,hp<signed_half(p+0x548)?(uint16_t)after:(uint16_t)half(p+0x548));put(p+0x54c,0x40400000);memcpy(c->direction,h->bytes+0x10,12);c->phase=3;
 }
 if(c->phase==3){uint32_t pc=0;gh_om_damage_status e=force(c,v,policy,&pc);if(e){c->frontier=pc;if(f)*f=pc;return e;}c->original_hit=1;c->phase=4;}
 *out=c->original_hit;return GH_OM_DAMAGE_OK;
}
