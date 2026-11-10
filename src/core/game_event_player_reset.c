#include "game_event_player_reset.h"
#include "game_actor_motion_step.h"
#include "game_player_post_init.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
static uint32_t rd(const void*p){uint32_t n;memcpy(&n,p,4);return n;}
static void wr(void*p,uint32_t n){memcpy(p,&n,4);}
static float mul(float a,float b){volatile float n=a*b;return n;}
static float add(float a,float b){volatile float n=a+b;return n;}
static int finite_value(float f){uint32_t u;memcpy(&u,&f,4);return (u&0x7f800000)!=0x7f800000;}
static gh_game_event_status fail(gh_event_player_reset*s,uint32_t pc,gh_game_event_status e){s->frontier=pc;return e;}
gh_game_event_status gh_event_weapon_follow_001cf3a8(const gh_event_weapon_attachment*a,gh_static_transform_policy policy,uint32_t*f){
 if(!a||!f||!a->weapon||!a->weapon->bytes||a->weapon->size<0x620)return GH_EVENT_ARGUMENT;*f=0;
 if(!a->parent)return GH_EVENT_OK;
 if(!a->parent->bytes||a->parent->size<0x2b5||!a->parent_model||a->parent_model->root.bytes!=a->parent->bytes)return GH_EVENT_ARGUMENT;
 if(a->parent_part<0||(uint32_t)a->parent_part>=a->parent->bytes[0x2b4])return GH_EVENT_OK;
 const gh_actor_part*part;if(gh_actor_model_part_at(a->parent_model,(uint32_t)a->parent_part,&part)){*f=0x14bfe0;return GH_EVENT_BOUNDS;}if(!part->bytes)return GH_EVENT_OK;
 uint8_t*p=a->weapon->bytes;uintptr_t base=(uintptr_t)p,alias=(uintptr_t)a->weapon->alias_f0;if(!alias||alias<base||alias-base>a->weapon->size||16>a->weapon->size-(alias-base))return GH_EVENT_BOUNDS;
 /* Exact3-lane copy precedes original SDK apply; W is the existing live lane. */
 memmove(a->weapon->alias_f0,p+0x610,12);float m[16],v[4],out[4];memcpy(m,part->bytes+0x80,64);memcpy(v,a->weapon->alias_f0,16);
 if(policy!=GH_STATIC_BINARY32_REFERENCE){*f=0x30a2b0;return GH_EVENT_UNSUPPORTED;}
 for(unsigned i=0;i<16;i++)if(!finite_value(m[i])){*f=0x30a2b0;return GH_EVENT_UNSUPPORTED;}for(unsigned i=0;i<4;i++)if(!finite_value(v[i])){*f=0x30a2b0;return GH_EVENT_UNSUPPORTED;}
 fenv_t env;if(feholdexcept(&env)){*f=0x30a2b0;return GH_EVENT_UNSUPPORTED;}if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);*f=0x30a2b0;return GH_EVENT_UNSUPPORTED;}
 for(unsigned row=0;row<4;row++)out[row]=add(add(add(mul(m[row],v[0]),mul(m[4+row],v[1])),mul(m[8+row],v[2])),mul(m[12+row],v[3]));
 int valid=1;for(unsigned i=0;i<4;i++)if(!finite_value(out[i]))valid=0;if(fesetenv(&env)||!valid){*f=0x30a2b0;return GH_EVENT_UNSUPPORTED;}
 memcpy(a->weapon->alias_f0,out,16);memmove(p+0x100,a->parent->bytes+0x100,12);memcpy(p+0x24c,a->parent->bytes+0x24c,4);wr(p+0x250,(rd(p+0x250)&~0x10u)|(rd(a->parent->bytes+0x250)&0x10u));return GH_EVENT_OK;
}
gh_game_event_status gh_event_player_reset_open(gh_event_player_reset*s,const gh_event_player_reset_bindings*b){
 if(!s||!b||!b->prefix||!b->prefix->player_abort||!b->prefix->player_abort->player||!b->motion||!b->motion_bindings.model||!b->motion_bindings.work||!b->init||!b->canonical_rate)return GH_EVENT_ARGUMENT;
 if(s->active&&!s->complete)return GH_EVENT_UNSUPPORTED;
 const gh_title_player_view*p=&b->prefix->player_abort->player->view;if(!p->bytes||p->size<0x1660||b->motion_bindings.model->root.bytes!=p->bytes||b->solve.model!=b->motion_bindings.model||b->solve.work!=b->motion_bindings.work)return GH_EVENT_ARGUMENT;
 memset(s,0,sizeof(*s));s->b=*b;s->active=1;return GH_EVENT_OK;
}
gh_game_event_status gh_event_player_reset_step_0012c418(gh_event_player_reset*s){
 if(!s||!s->active)return GH_EVENT_ARGUMENT;if(s->complete)return GH_EVENT_OK;
 gh_event_player_reset_bindings*b=&s->b;gh_actor_model*m=b->motion_bindings.model;uint8_t*p=b->prefix->player_abort->player->view.bytes;uint32_t pc=0;s->frontier=0;
 if(s->stage==0){gh_game_event_status e=gh_event_player_clear_0012c418(&s->prefix,b->prefix);if(e&&(e!=GH_EVENT_UNSUPPORTED||s->prefix.stage!=4||s->prefix.frontier!=0x2a8578))return fail(s,s->prefix.frontier,e);s->stage=1;}
 if(s->stage==1){if(!b->motion_bindings.resource||b->motion_bindings.resource_bytes<0x30)return fail(s,0x2a8578,GH_EVENT_BOUNDS);size_t motion=rd(b->motion_bindings.resource+0x28),sequence=rd(b->motion_bindings.resource+0x2c);gh_actor_motion_status e=gh_actor_motion_call_open_002a8578(&s->motion_call,b->motion,&b->motion_bindings,motion,sequence,0);if(e){s->detail=e;return fail(s,0x2a8578,GH_EVENT_UNSUPPORTED);}s->stage=2;}
 if(s->stage==2){gh_actor_motion_status e=gh_actor_motion_call_step_002a8578(&s->motion_call);if(e){s->detail=e;return fail(s,s->motion_call.frontier?s->motion_call.frontier:0x2a8578,GH_EVENT_UNSUPPORTED);}s->stage=3;}
 if(s->stage==3){gh_actor_motion_status e=gh_actor_motion_step_00157c18(b->motion,*b->canonical_rate,b->policy,&s->ended,&pc);if(e){s->detail=e;return fail(s,pc?pc:0x157c18,GH_EVENT_UNSUPPORTED);}s->stage=4;}
 if(s->stage==4){gh_actor_model_status e=gh_actor_model_update_hierarchy_00149eb8(m,b->policy);if(e){s->detail=e;return fail(s,m->frontier?m->frontier:0x149eb8,GH_EVENT_UNSUPPORTED);}s->stage=5;}
 if(s->stage==5){gh_actor_motion_status e=gh_actor_motion_solve_00152a20(&s->solve,&b->solve,b->policy,&pc);if(e){s->detail=e;return fail(s,pc?pc:0x152a20,GH_EVENT_UNSUPPORTED);}s->stage=6;}
 if(s->stage==6){gh_actor_model_status e=gh_actor_finish_model_0014a038(m,b->policy);if(e){s->detail=e;return fail(s,m->frontier?m->frontier:0x14a038,GH_EVENT_UNSUPPORTED);}s->stage=7;}
 if(s->stage==7){if(!s->weapon_captured){if(!b->weapon_slot||!b->weapon_slot->known)return fail(s,0x1cf3a8,GH_EVENT_UNSUPPORTED);if(b->weapon_slot->current){s->weapon=*b->weapon_slot->current;s->weapon_present=1;}s->weapon_captured=1;}
  if(!s->weapon_present)s->stage=9;else{gh_game_event_status e=gh_event_weapon_follow_001cf3a8(&s->weapon,b->policy,&pc);if(e)return fail(s,pc?pc:0x1cf3a8,e);s->stage=8;}}
 if(s->stage==8){if(!s->weapon.weapon_model||s->weapon.weapon_model->root.bytes!=s->weapon.weapon->bytes)return fail(s,0x149eb8,GH_EVENT_UNSUPPORTED);gh_actor_model_status e=gh_actor_model_update_hierarchy_00149eb8(s->weapon.weapon_model,b->policy);if(e){s->detail=e;return fail(s,s->weapon.weapon_model->frontier?s->weapon.weapon_model->frontier:0x149eb8,GH_EVENT_UNSUPPORTED);}s->stage=9;}
 if(s->stage==9){wr(p+0x15f4,(rd(p+0x15f4)&~0x10u)|0x20u);s->stage=10;}
 if(s->stage==10){gh_player_init_status e=gh_game_heat_clear_mode_002a9cc8(b->init,b->policy,&pc);if(e){s->detail=e;return fail(s,pc?pc:0x2a9cc8,GH_EVENT_UNSUPPORTED);}s->stage=11;}
 if(s->stage==11){wr(p+0x640,0);s->complete=1;s->frontier=0;}
 return GH_EVENT_OK;
}
