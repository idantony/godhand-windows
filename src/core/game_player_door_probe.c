#include "game_player_door_probe.h"
#include "sdk_rotation_trig.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
static uint32_t w(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
static float fl(const uint8_t*p){float n;memcpy(&n,p,4);return n;}
static float bits(uint32_t n){float v;memcpy(&v,&n,4);return v;}
static gh_player_door_status fail(gh_player_door_probe*s,uint32_t pc,gh_player_door_status e){s->frontier=pc;return e;}
static int gate(const gh_player_door_bindings*b){int16_t hp;memcpy(&hp,b->player->bytes+0x54a,2);int32_t enemies;memcpy(&enemies,b->init->enemies+0x514,4);return hp>0&&!(w(b->player->bytes+0x15f4)&0x400000u)&&enemies<1&&!b->init->enemies[0x53c];}
static gh_player_door_status publish(gh_player_door_probe*s,const gh_player_door_bindings*b){if(!b->publish)return fail(s,0x1f7858,GH_PLAYER_DOOR_UNSUPPORTED);uint32_t pc=0;gh_player_door_status e=b->publish(b->context,&s->pending,&pc);return e?fail(s,pc?pc:0x1f7858,e):GH_PLAYER_DOOR_OK;}
static void prepare_action(gh_player_door_probe*s,const gh_player_door_bindings*b,int wall){memcpy(b->player->bytes+0x660,s->position,12);memcpy(b->player->bytes+0x670,&s->yaw,4);s->pending=(gh_player_door_action){.player=b->player,.priority=4,.life=wall?9:8,.callback_entry=wall?0x2dccd0:0x2dca58};}
static gh_player_door_status body(gh_player_door_probe*s,const gh_player_door_bindings*b){
 uint32_t pc=0;if(s->stage==0){if(!gate(b)){s->complete=1;return GH_PLAYER_DOOR_OK;}if(!b->scene_at||!b->room)return fail(s,0x2c2b58,GH_PLAYER_DOOR_ARGUMENT);
  uintptr_t start=(uintptr_t)b->player->bytes,alias=(uintptr_t)b->player->alias_f0;if(alias<start||alias-start>b->player->size||16>b->player->size-(alias-start))return fail(s,0x2c2b58,GH_PLAYER_DOOR_ARGUMENT);float origin[3];memcpy(origin,b->player->alias_f0,12);int found;
  gh_scene_at_status ae=gh_scene_at_find_jump_002c2b58(b->scene_at,b->room,(uint16_t)b->work->room,origin,s->position,&s->yaw,&found,&pc);if(ae)return fail(s,pc?pc:0x2c2b58,ae==GH_AT_UNSUPPORTED?GH_PLAYER_DOOR_UNSUPPORTED:GH_PLAYER_DOOR_CALLEE);
  if(found){float yaw=fl(b->player->bytes+0x104);if(!isfinite(yaw)||!isfinite(s->yaw))return fail(s,0x2da1f8,GH_PLAYER_DOOR_NUMERIC);volatile float difference=s->yaw-yaw;float delta=difference;
   if(delta>bits(0x40490fdb)){volatile float v=delta-bits(0x40c90fdb);delta=v;}if(delta<bits(0xc0490fdb)){volatile float v=delta+bits(0x40c90fdb);delta=v;}float magnitude=fabsf(delta);if(magnitude>bits(0x40490fdb))magnitude=bits(0x40490fdb);
   if(magnitude<=bits(0x3fa78d36)){prepare_action(s,b,0);s->stage=1;}else s->stage=2;
  }else s->stage=2;
 }
 if(s->stage==1){gh_player_door_status e=publish(s,b);if(e)return e;s->stage=2;}
 if(s->stage==2){if(!gate(b)){s->complete=1;return GH_PLAYER_DOOR_OK;}if(!b->actors)return fail(s,0x131e10,GH_PLAYER_DOOR_UNSUPPORTED);
  float matrix[16];memcpy(matrix,b->player->bytes+0x80,64);const float from[3]={0,.5f,-.5f},to[3]={0,.5f,1};gh_scene_ray_request ray={.mask=1,.flags=0,.segmented=1,.report_limit=1};float position[4]={0,0,0,1},normal[4]={0,0,0,1};gh_scene_ray_report report={0};ray.hit_position=position;ray.hit_normal=normal;ray.native_reports=&report;
  if(gh_scene_at_apply_world_001508d8(matrix,from,ray.from)||gh_scene_at_apply_world_001508d8(matrix,to,ray.to))return fail(s,0x30a2b0,GH_PLAYER_DOOR_NUMERIC);int hit=0;gh_scene_ray_status re=gh_scene_ray_query_00131e10(b->actors,&ray,&hit,&pc);if(re)return fail(s,pc?pc:0x131e10,re==GH_RAY_UNSUPPORTED?GH_PLAYER_DOOR_UNSUPPORTED:GH_PLAYER_DOOR_CALLEE);
  if(!hit){s->complete=1;return GH_PLAYER_DOOR_OK;}if(!report.attributes||report.attribute_bytes<8)return fail(s,0x2dcbe8,GH_PLAYER_DOOR_CALLEE);if(!(w(report.attributes+4)&0x20)){s->complete=1;return GH_PLAYER_DOOR_OK;}
  gh_camera_status ce=gh_game_angle_001f8180(-normal[0],-normal[2],&s->yaw,b->numeric_policy,&pc);if(ce)return fail(s,pc?pc:0x1f8180,ce==GH_CAMERA_UNSUPPORTED?GH_PLAYER_DOOR_UNSUPPORTED:GH_PLAYER_DOOR_NUMERIC);
  float sine,cosine;if(gh_sdk_rotation_sincos_reference_0030a5d0(s->yaw,&sine,&cosine,GH_STATIC_BINARY32_REFERENCE,&pc))return fail(s,pc?pc:0x30a7b8,GH_PLAYER_DOOR_UNSUPPORTED);
  float rotation[16]={0};rotation[0]=rotation[10]=cosine;rotation[5]=rotation[15]=1;rotation[2]=-sine;rotation[8]=sine;const float local[3]={0,0,-.5f};float offset[4];if(gh_scene_at_apply_world_001508d8(rotation,local,offset))return fail(s,0x30a2b0,GH_PLAYER_DOOR_NUMERIC);
  uintptr_t begin=(uintptr_t)b->player->bytes,pos=(uintptr_t)b->player->alias_f0;if(pos<begin||pos-begin>b->player->size||16>b->player->size-(pos-begin))return fail(s,0x2dcc44,GH_PLAYER_DOOR_ARGUMENT);
  volatile float x=offset[0]+position[0],z=offset[2]+position[2];s->position[0]=x;s->position[1]=fl(b->player->alias_f0+4);s->position[2]=z;for(unsigned k=0;k<3;k++)if(!isfinite(s->position[k]))return fail(s,0x30a2b0,GH_PLAYER_DOOR_NUMERIC);prepare_action(s,b,1);s->stage=3;
 }
 if(s->stage==3){gh_player_door_status e=publish(s,b);if(e)return e;s->complete=1;}
 return GH_PLAYER_DOOR_OK;
}
gh_player_door_status gh_player_door_probe_begin(gh_player_door_probe*s){if(!s||(s->active&&!s->complete))return GH_PLAYER_DOOR_ARGUMENT;*s=(gh_player_door_probe){.identity=s,.active=1};return GH_PLAYER_DOOR_OK;}
gh_player_door_status gh_player_door_probe_002dc920_002dcad8(gh_player_door_probe*s,const gh_player_door_bindings*b){
 if(!s||!b||!s->active||s->identity!=s||!b->player||!b->player->bytes||b->player->size<0x15f8||!b->work||!b->init)return GH_PLAYER_DOOR_ARGUMENT;s->frontier=0;if(s->complete)return GH_PLAYER_DOOR_OK;
 if(s->bindings&&s->bindings!=b)return GH_PLAYER_DOOR_ARGUMENT;s->bindings=b;if(!b->current_player||b->current_player->bytes!=b->player->bytes)return fail(s,0x2db6b0,GH_PLAYER_DOOR_UNSUPPORTED);
 if(b->numeric_policy!=GH_CAMERA_NATIVE_BINARY32_REFERENCE)return fail(s,0x2c2b58,GH_PLAYER_DOOR_UNSUPPORTED);
 fenv_t env;if(feholdexcept(&env))return fail(s,0x2dc920,GH_PLAYER_DOOR_NUMERIC);if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return fail(s,0x2dc920,GH_PLAYER_DOOR_NUMERIC);}gh_player_door_status e=body(s,b);if(fesetenv(&env))return fail(s,0x2dc920,GH_PLAYER_DOOR_NUMERIC);return e;
}
