#include "game_player_post_init.h"
#include "game_actor_finish_model.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
static void ww(uint8_t*p,uint32_t v){memcpy(p,&v,4);}
static float value(const uint8_t*p){float v;memcpy(&v,p,4);return v;}
static gh_player_init_status fail(gh_game_player_post_init*s,uint32_t pc,gh_player_init_status e){s->frontier=pc;return e;}
gh_player_init_status gh_game_heat_clear_mode_002a9cc8(gh_game_init_state*s,gh_static_transform_policy policy,uint32_t*f){
 if(f)*f=0;if(!s)return GH_PLAYER_INIT_ARGUMENT;if(policy!=GH_STATIC_BINARY32_REFERENCE){if(f)*f=0x2a9cc8;return GH_PLAYER_INIT_UNSUPPORTED;}s->heat[0x10]=0;float current=value(s->heat+4),drain=value(s->heat+12);if(!isfinite(current)||!isfinite(drain)){if(f)*f=0x2a9cc8;return GH_PLAYER_INIT_UNSUPPORTED;}
 fenv_t saved;if(feholdexcept(&saved)){if(f)*f=0x2a9cc8;return GH_PLAYER_INIT_UNSUPPORTED;}if(fesetround(FE_TONEAREST)){(void)fesetenv(&saved);if(f)*f=0x2a9cc8;return GH_PLAYER_INIT_UNSUPPORTED;}volatile float difference=current-drain;float delta=difference;if(delta<0)delta=0;uint32_t result;memcpy(&result,&delta,4);int numeric=isfinite(delta);if(fesetenv(&saved)||!numeric){if(f)*f=0x2a9cc8;return GH_PLAYER_INIT_UNSUPPORTED;}ww(s->heat+8,result);return GH_PLAYER_INIT_OK;
}
static gh_player_init_status model_finish(gh_game_player_post_init*s,gh_static_transform_policy policy){
 gh_actor_model_status e=gh_actor_finish_model_0014a038(s->model,policy);
 if(!e)return GH_PLAYER_INIT_OK;
 return fail(s,s->model->frontier?s->model->frontier:0x14a038,
  e==GH_ACTOR_MODEL_UNSUPPORTED?GH_PLAYER_INIT_UNSUPPORTED:e==GH_ACTOR_MODEL_BOUNDS?GH_PLAYER_INIT_BOUNDS:
  e==GH_ACTOR_MODEL_ARGUMENT?GH_PLAYER_INIT_ARGUMENT:GH_PLAYER_INIT_CALLEE);
}
gh_player_init_status gh_game_player_post_initialize_00107f58(gh_game_player_post_init*s,gh_game_title_player*p,gh_actor_model*m,gh_game_init_state*init,gh_boot_work*work,gh_core_save_view save,gh_static_transform_policy policy,int*original){
 if(!s||!p||!p->view.bytes||p->view.size<GH_TITLE_PLAYER_BYTES||!m||m->root.bytes!=p->view.bytes||!init||!work||!original||(save.bytes&&save.size<GH_CORE_SAVE_BYTES))return GH_PLAYER_INIT_ARGUMENT;*original=0;if(s->bound&&(s->player!=p||s->model!=m||s->init!=init||s->work!=work||s->save.bytes!=save.bytes||s->save.size!=save.size))return GH_PLAYER_INIT_ARGUMENT;if(s->complete){*original=1;return GH_PLAYER_INIT_OK;}if(!s->bound){s->player=p;s->model=m;s->init=init;s->work=work;s->save=save;s->bound=1;}
 uint8_t*raw=p->view.bytes;
 if(!s->stage){gh_player_init_status e=model_finish(s,policy);if(e)return e;s->stage=1;}
 if(s->stage==1){uint32_t pc=0;gh_player_init_status e=gh_game_heat_clear_mode_002a9cc8(init,policy,&pc);if(e)return fail(s,pc?pc:0x2a9cc8,e);s->stage=2;}
 if(s->stage==2){ww(raw+0x5ec,0x72);ww(raw+0x5e8,0x72);if(!p->view.alias_f0)return GH_PLAYER_INIT_ARGUMENT;memcpy(raw+0x490,p->view.alias_f0,12);memcpy(raw+0x630,raw+0x490,12);s->stage=3;}
 if(s->stage==3){if(save.bytes&&save.bytes[0x157]){s->pending_object_id=0x376;return fail(s,0x12c8d8,GH_PLAYER_INIT_UNSUPPORTED);}s->pending_object_id=0;s->stage=4;}
 if(s->stage==4){static const uint32_t multipliers[]={0x3f800000,0x3f800000,0x3f99999a,0x3fb33333,0x3fcccccd,0x3fe66666,0x40000000,0x400ccccd,0x4019999a};unsigned stage=((const uint8_t*)work)[0x5b1];ww(raw+0x600,stage<9?multipliers[stage]:0x3f800000);ww(raw+0x690,0);memset(raw+0x650,0,12);ww(raw+0x674,0);s->stage=5;}
 s->complete=1;s->frontier=0;*original=1;return GH_PLAYER_INIT_OK;
}
