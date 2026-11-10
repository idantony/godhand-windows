#include "game_player_child_move.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
static uint32_t u32(const uint8_t*p){uint32_t v;memcpy(&v,p,4);return v;}
static void put(uint8_t*p,uint32_t v){memcpy(p,&v,4);}
static unsigned u16(const uint8_t*p){return p[0]|(unsigned)p[1]<<8;}
static float rf(const uint8_t*p){float v;memcpy(&v,p,4);return v;}
static float add(float a,float b){volatile float f=a+b;return f;}
static gh_title_player_status fail(gh_child_move*s,uint32_t f,gh_title_player_status e){s->frontier=f;return e;}
static gh_title_player_status copy_parent(gh_child_move*s){
 gh_game_player_springs*springs=s->bindings.cloth.springs;gh_game_player_child*c=springs->child;uint8_t*p=c->object.view.bytes;const gh_player_child_attachment*a=s->bindings.cloth.attachment;
 if(p[0x2f4]==4)return fail(s,0x1005bc,GH_PLAYER_UNSUPPORTED);
 if(u32(p+0x20d0)||u32(p+0x20a8))return fail(s,0x1005d4,GH_PLAYER_UNSUPPORTED);
 const gh_actor_model*parent=a&&a->attached?a->parent_model:NULL;
 if(parent){if(a->child!=c||c->model.external_parent!=parent||a->part_index<0||(uint32_t)a->part_index>=parent->parts_allocated||u32(p+0x20d4)!=(uint32_t)a->part_index||a->parent_part!=parent->parts+(unsigned)a->part_index||!parent->root.bytes||!parent->root.alias_f0||!c->model.parts_allocated)return fail(s,0x100608,GH_PLAYER_ARGUMENT);
  if(u32(p+0x2f0)&2){const gh_actor_part*first=c->model.parts;float delta[3];for(unsigned k=0;k<3;k++){delta[k]=add(rf(a->parent_part->alias_f0+k*4),-rf(first->alias_f0+k*4));if(!isfinite(delta[k]))return fail(s,0x100650,GH_PLAYER_UNSUPPORTED);}float positions[255][3];for(unsigned i=0;i<c->model.parts_allocated;i++)for(unsigned k=0;k<3;k++){positions[i][k]=add(rf(c->model.parts[i].bytes+0xb0+k*4),delta[k]);if(!isfinite(positions[i][k]))return fail(s,0x1006a0,GH_PLAYER_UNSUPPORTED);}for(unsigned i=0;i<c->model.parts_allocated;i++)memcpy(c->model.parts[i].bytes+0xb0,positions[i],12);gh_player_springs_status e=gh_player_springs_reset_00107c68(springs,s->bindings.cloth.matrix_policy);if(e)return fail(s,springs->frontier,GH_PLAYER_UNSUPPORTED);}
  memcpy(c->object.view.alias_f0,parent->root.alias_f0,12);memcpy(c->model.parts[0].alias_f0,a->parent_part->alias_f0,12);memcpy(p+0x100,a->parent_part->bytes+0x100,12);memcpy(p+0x24c,parent->root.bytes+0x24c,4);put(p+0x250,(u32(p+0x250)&~0x10010u)|(u32(parent->root.bytes+0x250)&0x10010u));
 }
 return GH_PLAYER_OK;
}
static gh_title_player_status run(gh_child_move*s){
 const gh_child_move_bindings*b=&s->bindings;gh_game_player_child*c=b->cloth.springs->child;uint8_t*p=c->object.view.bytes;
 if(s->stage==0){int skipped;uint32_t f;gh_title_player_status gate=gh_game_player_child_move_gate_00100530(&c->object.view,b->work,&skipped,&f);if(gate!=GH_PLAYER_OK&&gate!=GH_PLAYER_UNSUPPORTED)return fail(s,f,gate);if(skipped){s->complete=s->skipped=1;s->frontier=0;return GH_PLAYER_OK;}s->stage=1;}
 if(s->stage==1){gh_title_player_status e=copy_parent(s);if(e)return e;s->stage=2;}
 if(s->stage==2){gh_actor_model_status e=gh_actor_model_update_hierarchy_00149eb8(&c->model,b->cloth.matrix_policy);if(e)return fail(s,c->model.frontier,GH_PLAYER_UNSUPPORTED);s->stage=3;}
 if(s->stage==3){/* Actual100CE0 clears400 even when the wind branch is false. */
  if(u16(p+0x2fe)==0x101){put(p+0x20ac,u32(p+0x20ac)&~0x400u);const gh_player_child_attachment*a=b->cloth.attachment;
   if(a&&a->attached&&u16(a->parent_model->root.bytes+0x2fe)==0x100){if(!b->work5f4||b->work5f4->bytes!=a->parent_model->root.bytes||b->work5f4->size<0x3ae)return fail(s,0x2db6b0,GH_PLAYER_UNSUPPORTED);if(u16(b->work5f4->bytes+0x3ac)&0x800)return fail(s,0x147988,GH_PLAYER_UNSUPPORTED);}
  }s->stage=4;
 }
 if(s->stage==4){gh_cloth_status e=gh_cloth_step_prefix_00104010(&s->simulation,&b->cloth);if(e)return fail(s,s->simulation.frontier,GH_PLAYER_UNSUPPORTED);if(!s->simulation.complete)return fail(s,0x104010,GH_PLAYER_UNSUPPORTED);s->stage=5;}
 if(s->stage==5){if(u16(p+0x2fe)==0x101){if(!b->work5f4||b->work5f4->size<0x65c)return fail(s,0x2db6b0,GH_PLAYER_UNSUPPORTED);memset(b->work5f4->bytes+0x650,0,12);}s->complete=1;s->frontier=0;s->stage=6;}
 return GH_PLAYER_OK;
}
gh_title_player_status gh_game_player_child_move_00100530(gh_child_move*s,const gh_child_move_bindings*b){
 if(!s||!b||!b->work||!b->cloth.springs||!b->cloth.springs->initialized||!b->cloth.springs->child)return GH_PLAYER_ARGUMENT;gh_game_player_child*c=b->cloth.springs->child;if(!c->complete||!c->object.view.bytes||c->object.view.size<GH_PLAYER_CHILD_BYTES||c->model.root.bytes!=c->object.view.bytes||c->model.native_identity!=&c->model||!c->model.ready||!c->model.parts_ready)return GH_PLAYER_ARGUMENT;
 if(s->stage&&(s->bindings.work!=b->work||s->bindings.cloth.springs!=b->cloth.springs||s->bindings.cloth.attachment!=b->cloth.attachment||s->bindings.work5f4!=b->work5f4||s->bindings.cloth.templates!=b->cloth.templates||s->bindings.cloth.globals!=b->cloth.globals||s->bindings.cloth.random!=b->cloth.random||s->bindings.cloth.actors!=b->cloth.actors||s->bindings.cloth.matrix_policy!=b->cloth.matrix_policy||s->bindings.cloth.random_policy!=b->cloth.random_policy))return GH_PLAYER_ARGUMENT;if(s->complete)return GH_PLAYER_OK;if(!s->stage)s->bindings=*b;
 if(b->cloth.matrix_policy!=GH_STATIC_BINARY32_REFERENCE)return fail(s,0x100530,GH_PLAYER_UNSUPPORTED);fenv_t env;if(feholdexcept(&env))return GH_PLAYER_UNSUPPORTED;if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return GH_PLAYER_UNSUPPORTED;}gh_title_player_status result=run(s);if(fesetenv(&env))return GH_PLAYER_UNSUPPORTED;return result;
}
