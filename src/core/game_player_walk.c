/* Actual patchedSLUS215.03 10C300, source/table evidence tracked per batch.
 * Numeric policy follows existing explicit nativebinary32 reference. */
#include "game_player_walk.h"
#include "game_player_idle_input.h"
#include "game_player_root_delta.h"
#include "id_layout_rotation.h"
#include "game_scene_at_geometry.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
static uint32_t word(const void*p){uint32_t n;memcpy(&n,p,4);return n;}
static void put(void*p,uint32_t n){memcpy(p,&n,4);}
static unsigned half(const uint8_t*p){return p[0]|(unsigned)p[1]<<8;}
static float number(const void*p){float n;memcpy(&n,p,4);return n;}
static void scalar(void*p,float n){memcpy(p,&n,4);}
static gh_player_frame_status fail(gh_player_walk_call*s,uint32_t pc,gh_player_frame_status e){s->frontier=pc;return e;}
static gh_player_frame_status body(gh_player_walk_call*s,const gh_player_frame_bindings*b){
 uint8_t*p=b->player->view.bytes;uint32_t pc=0;
 if(!s->phase){uint16_t raw=(uint16_t)half(p+0x54a);int16_t hp;memcpy(&hp,&raw,2);if(hp<1){p[0x54a]=1;p[0x54b]=0;}
  uint32_t step=half(p+0x5f2)==3?0x3fcccccd:0x3fa66666;put(p+0x5a8,step);put(p+0x444,step^((half(p+0x434)&4)?0x80000000u:0));s->phase=p[0x2f6]==0?1:p[0x2f6]==1?4:8;
 }
 if(s->phase==1){memset(p+0x5e0,0,4);if((int8_t)p[0x648]>10)p[0x648]=10;s->phase=2;}
 if(s->phase==2){if(!b->auxiliary_bindings_known)return fail(s,0x10c3c0,GH_PLAYER_FRAME_UNSUPPORTED);if(b->aux_6a0||word(p+0x6a0))return fail(s,0x2a8578,GH_PLAYER_FRAME_UNSUPPORTED);
  if(!b->resource||b->resource_bytes<0x60)return fail(s,0x10c300,GH_PLAYER_FRAME_ARGUMENT);size_t motion=word(b->resource+0x58),candidate=word(b->resource+0x5c);if(motion>=b->resource_bytes||candidate>=b->resource_bytes)return fail(s,0x2a8578,GH_PLAYER_FRAME_ARGUMENT);
  s->motion=(gh_player_motion_request){.player=b->player,.model=b->model,.work=b->work,.resource=b->resource,.resource_bytes=b->resource_bytes,.motion_offset=motion,.sequence_candidate_offset=candidate,.start_frame_bits=0,.blend_frames=10,.flags=0,.release_previous_sequence=1,.rate=b->rate};s->phase=3;
 }
 if(s->phase==3){if(!b->model||b->model->root.bytes!=p||!b->initialize_motion)return fail(s,0x2a8578,GH_PLAYER_FRAME_UNSUPPORTED);gh_player_init_status e=b->initialize_motion(b->motion_context,&s->motion,&pc);if(e)return fail(s,pc?pc:0x2a8578,e==GH_PLAYER_INIT_UNSUPPORTED?GH_PLAYER_FRAME_UNSUPPORTED:GH_PLAYER_FRAME_CALLEE);p[0x2f6]=(uint8_t)(p[0x2f6]+1u);s->phase=4;}
 if(s->phase==4){if(!b->ground_globals||!b->ground_globals->tuning_loaded)return fail(s,0x10c42c,GH_PLAYER_FRAME_UNSUPPORTED);put(p+0x15f0,word(&b->ground_globals->walk_turn_speed));if(word(p+0x5d8)==4)return fail(s,0x2da1f8,GH_PLAYER_FRAME_UNSUPPORTED);
  if(p[0x61f]){float angle=number(p+0x608),signed_angle=number(p+0x604),heading=number(p+0x104),maximum=b->ground_globals->walk_turn_cap;if(!isfinite(angle)||!isfinite(signed_angle)||!isfinite(heading)||!isfinite(maximum))return fail(s,0x10c4b0,GH_PLAYER_FRAME_NUMERIC);float constant;uint32_t bits=0x3f060a92;memcpy(&constant,&bits,4);volatile float delta=angle-constant;float positive=delta<0?0:delta;volatile float product=positive*.4f;float turn=maximum<product?maximum:product;if(signed_angle<0)turn=-turn;volatile float sum=heading+turn;float wrapped;if(gh_id_angle_reference_001f7d48(sum,&wrapped))return fail(s,0x1f7d48,GH_PLAYER_FRAME_UNSUPPORTED);scalar(p+0x104,wrapped);
  }s->phase=5;
 }
 if(s->phase==5){if(!b->advance_motion||!b->model||b->model->root.bytes!=p||!b->rate)return fail(s,0x157c18,GH_PLAYER_FRAME_UNSUPPORTED);if(!s->motion.player)s->motion=(gh_player_motion_request){.player=b->player,.model=b->model,.work=b->work,.resource=b->resource,.resource_bytes=b->resource_bytes,.rate=b->rate};gh_player_frame_status e=b->advance_motion(b->motion_context,&s->motion,&s->motion_ended,&pc);if(e)return fail(s,pc?pc:0x157c18,e);s->phase=6;}
 if(s->phase==6){gh_player_delta_status e=gh_player_root_rotation_0014f9f0(&b->player->view,b->work,GH_STATIC_BINARY32_REFERENCE,&pc);if(e)return fail(s,pc,e==GH_PLAYER_DELTA_NUMERIC?GH_PLAYER_FRAME_NUMERIC:e==GH_PLAYER_DELTA_UNSUPPORTED?GH_PLAYER_FRAME_UNSUPPORTED:GH_PLAYER_FRAME_ARGUMENT);s->phase=7;}
 if(s->phase==7){gh_player_delta_status e=gh_player_root_translation_0014f928(&b->player->view,b->work,GH_STATIC_BINARY32_REFERENCE,&pc);if(e)return fail(s,pc,e==GH_PLAYER_DELTA_NUMERIC?GH_PLAYER_FRAME_NUMERIC:e==GH_PLAYER_DELTA_UNSUPPORTED?GH_PLAYER_FRAME_UNSUPPORTED:GH_PLAYER_FRAME_ARGUMENT);s->phase=8;}
 if(s->phase==8){float magnitude=number(p+0x60c);if(p[0x61f]){if(!isfinite(magnitude))return fail(s,0x10c548,GH_PLAYER_FRAME_NUMERIC);if(magnitude<10000)p[0x1601]=2;}s->phase=9;}
 if(s->phase==9){if(!b->idle_input||!b->idle_input_bindings)return fail(s,0x123938,GH_PLAYER_FRAME_UNSUPPORTED);const gh_player_frame_bindings*q=&b->idle_input_bindings->frame;if(q->player!=b->player||q->init!=b->init||q->work!=b->work||q->current_player!=b->current_player)return fail(s,0x123938,GH_PLAYER_FRAME_ARGUMENT);if(!s->input_started){gh_player_frame_status e=gh_player_idle_input_begin(b->idle_input);if(e)return fail(s,0x123938,e);s->input_started=1;}gh_player_frame_status e=gh_game_player_idle_input_00123938(b->idle_input,b->idle_input_bindings,&s->input_result);if(e)return fail(s,b->idle_input->frontier?b->idle_input->frontier:0x123938,e);s->phase=10;}
 if(s->phase==10){if(!s->input_result){gh_player_frame_status e=gh_game_player_idle_cleanup_0012bfb8(b->idle_input);if(e)return fail(s,b->idle_input->frontier?b->idle_input->frontier:0x12bfb8,e);}s->phase=11;s->complete=1;s->frontier=0;return GH_PLAYER_FRAME_OK;}
 return fail(s,0x10c300,GH_PLAYER_FRAME_ARGUMENT);
}
gh_player_frame_status gh_game_player_walk_0010c300(gh_player_walk_call*s,const gh_player_frame_bindings*b){
 if(!s||!b||!b->player||!b->player->view.bytes||b->player->view.size<GH_TITLE_PLAYER_BYTES||!b->work||!b->init)return GH_PLAYER_FRAME_ARGUMENT;if(s->complete)return GH_PLAYER_FRAME_BUSY;
 if(b->numeric_policy!=GH_CAMERA_NATIVE_BINARY32_REFERENCE)return fail(s,0x10c300,GH_PLAYER_FRAME_UNSUPPORTED);
 if(s->bound&&(s->source_bindings!=b||s->motion.player!=b->player||s->motion.model!=b->model||s->motion.work!=b->work||s->motion.resource!=b->resource||s->motion.resource_bytes!=b->resource_bytes||s->motion.rate!=b->rate||s->init_owner!=b->init||s->current_owner!=b->current_player||(s->input_owner&&s->input_owner!=b->idle_input)||(s->input_bindings_owner&&s->input_bindings_owner!=b->idle_input_bindings)||(s->tuning_owner&&s->tuning_owner!=b->ground_globals)))return GH_PLAYER_FRAME_BUSY;
 if(!s->bound){s->source_bindings=b;s->init_owner=b->init;s->current_owner=b->current_player;s->motion=(gh_player_motion_request){.player=b->player,.model=b->model,.work=b->work,.resource=b->resource,.resource_bytes=b->resource_bytes,.rate=b->rate};s->bound=1;}
 if(!s->input_owner)s->input_owner=b->idle_input;if(!s->input_bindings_owner)s->input_bindings_owner=b->idle_input_bindings;if(!s->tuning_owner)s->tuning_owner=b->ground_globals;
 fenv_t env;if(feholdexcept(&env))return GH_PLAYER_FRAME_NUMERIC;if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return GH_PLAYER_FRAME_NUMERIC;}gh_player_frame_status e=body(s,b);if(fesetenv(&env))return GH_PLAYER_FRAME_NUMERIC;return e;
}
gh_player_frame_status gh_player_walk_begin(gh_player_walk_call*s){if(!s)return GH_PLAYER_FRAME_ARGUMENT;if(s->bound&&!s->complete)return GH_PLAYER_FRAME_BUSY;memset(s,0,sizeof(*s));return GH_PLAYER_FRAME_OK;}
