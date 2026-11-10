#include "game_scene_step.h"
#include "game_camera.h"
#include "game_collision_pair_shapes.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
static uint16_t h(const uint8_t*p){return (uint16_t)(p[0]|p[1]<<8);}
static uint32_t w(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static void p16(uint8_t*p,uint16_t n){p[0]=(uint8_t)n;p[1]=(uint8_t)(n>>8);}
static void p32(uint8_t*p,uint32_t n){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(n>>(i*8));}
static gh_scene_step_status fail(gh_game_scene_step*s,uint32_t a,gh_scene_step_status e,int detail){s->frontier=a;s->detail=detail;return e;}
static gh_scene_step_status call(gh_game_scene_step*s,const gh_game_scene_step_bindings*b,uint32_t a,uint32_t arg){uint32_t f=0;if(!b->external)return fail(s,a,GH_SCENE_STEP_UNSUPPORTED,0);gh_scene_step_status e=b->external(b->external_context,a,arg,&f);if(e)return fail(s,f?f:a,e,0);return GH_SCENE_STEP_OK;}
void gh_game_scene_step_construct(gh_game_scene_step*s,int32_t cursor,int16_t cooldown){if(s){*s=(gh_game_scene_step){0};s->nearcheck.cursor=cursor;s->cooldown_1b0=cooldown;}}
void gh_scene_nearcheck_reset_0031be78(gh_scene_nearcheck*s){if(s)s->pair_count=s->vertex_count=s->index_count=0;}
static int span(const gh_bundle_entry*e,uint32_t offset,uint32_t count,size_t stride,gh_scene_table_view*out){if(offset>e->size||e->size-offset<32||(size_t)count>(e->size-32-offset)/stride)return 0;out->bytes=e->bytes+32+offset;out->size=(size_t)count*stride;return 1;}
gh_scene_step_status gh_scene_nearcheck_bind_0031c1d0(gh_scene_nearcheck*s,const gh_bundle_entry*e){gh_scene_nearcheck n;if(!s)return GH_SCENE_STEP_ARGUMENT;if(!e||!e->bytes)return GH_SCENE_STEP_OK;if(e->size<4)return GH_SCENE_STEP_BOUNDS;if(memcmp(e->bytes,"PFD2",4))return GH_SCENE_STEP_OK;if(e->size<32)return GH_SCENE_STEP_BOUNDS;if(s->copy_mode)return GH_SCENE_STEP_UNSUPPORTED;n=*s;n.vertex_count=w(e->bytes+4);n.pair_count=w(e->bytes+12);n.index_count=w(e->bytes+20);
 if(!span(e,w(e->bytes+8),n.vertex_count,32,&n.vertices)||!span(e,w(e->bytes+16),n.pair_count,12,&n.pairs)||!span(e,w(e->bytes+24),n.index_count,2,&n.indices))return GH_SCENE_STEP_BOUNDS;
 /* Originalmutatesvertex+10 into a guestindexpointer. Typedindices view retains
  * thatassociation without changing gamebytes; actualquery remains frontier. */
 *s=n;return GH_SCENE_STEP_OK;
}
void gh_game_scene_step_case3_cooldown(gh_game_scene_step*s){if(s)s->cooldown_1b0=15;}
static int valid(const gh_game_scene_step_bindings*b){return b&&b->work&&b->init&&b->prepare&&b->collision&&b->actors&&b->scenario.bytes&&b->scenario.size>=0x114&&b->scenario.manager;}
static gh_scene_step_status nearcheck(gh_game_scene_step*s,const gh_game_scene_step_bindings*b){gh_scene_nearcheck*n=&s->nearcheck;const uint8_t*raw=(const uint8_t*)b->work;
 if((w(raw+0x598)&0x80000)||!n->pair_count)return GH_SCENE_STEP_OK;
 if(n->pair_count>INT32_MAX||n->pairs.size<(size_t)n->pair_count*12)return fail(s,0x2a6ff8,GH_SCENE_STEP_BOUNDS,0);
 for(unsigned iteration=0;iteration<2;iteration++){if(n->cursor>=0&&(uint32_t)n->cursor<n->pair_count){const uint8_t*p=n->pairs.bytes+(size_t)n->cursor*12;if(h(p)!=0xffff&&h(p+2)!=0xffff)return fail(s,0x131e10,GH_SCENE_STEP_UNSUPPORTED,0);}
  n->cursor=(int32_t)(((int64_t)n->cursor+1)%(int32_t)n->pair_count);
 }return GH_SCENE_STEP_OK;
}
static gh_scene_step_status collision(gh_game_scene_step*s,const gh_game_scene_step_bindings*b){gh_collision_pairs*c=b->collision;
 uint32_t frontier=0;gh_pair_shape_status e=gh_collision_pairs_step_resume_002b9d58(c,&s->collision_frame,GH_STATIC_BINARY32_REFERENCE,&frontier);
 s->collision_cursor=s->collision_frame.sweep_row;
 if(e){uint32_t detail=!s->collision_frame.stage?s->collision_frame.sweep_row:
   s->collision_frame.stage==1?s->collision_frame.geometry_link:s->collision_frame.source_link;
  return fail(s,frontier?frontier:0x2b9d58,e==GH_PAIR_SHAPE_UNSUPPORTED?GH_SCENE_STEP_UNSUPPORTED:
   e==GH_PAIR_SHAPE_ARGUMENT?GH_SCENE_STEP_ARGUMENT:GH_SCENE_STEP_CALLEE,(int)detail);}
 return GH_SCENE_STEP_OK;
}
static gh_scene_step_status actors(gh_game_scene_step*s,const gh_game_scene_step_bindings*b){if(b->actors->pending_actor_head||b->actors->pending_actor_count)return fail(s,0x12e870,GH_SCENE_STEP_UNSUPPORTED,0);if(w((const uint8_t*)b->work+0x594)&0x8000000)return fail(s,0x12e8cc,GH_SCENE_STEP_UNSUPPORTED,0);return GH_SCENE_STEP_OK;}
static void world_clock(const gh_game_scene_step_bindings*b){const uint8_t*r=(const uint8_t*)b->work;if((w(r+0x5d8)&0x2000)||(w(r+0x584)&8)||(w(r+0x5e4)&0x2000000)||b->work->room==0x21||((w(r+0x594)&0x800)&&(w(r+0x5e4)&0x8000000))||(b->work->phase569==1&&!(w(r+0x210)&1)))return;b->init->world_time++;b->prepare->global_0061a994++;}
static gh_scene_step_status enemies(gh_game_scene_step*s,const gh_game_scene_step_bindings*b){uint8_t*e=b->init->enemies;const uint8_t*raw=(const uint8_t*)b->work;
 /*294AD8 and292F68 are actualJRRA/NOP, not missing native implementations. */
 if(!s->enemy_stage){if(w(raw+0x5d8)&0x40000000)return GH_SCENE_STEP_OK;const unsigned counters[]={0x510,0x514,0x530};for(unsigned i=0;i<3;i++){uint32_t n=w(e+counters[i]);if(n)p32(e+counters[i],n-1);}for(unsigned i=0x538;i<=0x53d;i++)if(e[i])e[i]--;s->enemy_stage=1;}
 if(s->enemy_stage==1){float value,rate;memcpy(&value,e+0x544,4);memcpy(&rate,e+0x548,4);if(!isfinite(value))return fail(s,0x290250,GH_SCENE_STEP_UNSUPPORTED,0);
  if(value>0){if(!b->binary32_reference||!isfinite(rate))return fail(s,0x290250,GH_SCENE_STEP_UNSUPPORTED,0);fenv_t env;if(fegetenv(&env))return GH_SCENE_STEP_UNSUPPORTED;if(fesetround(FE_TONEAREST)){fesetenv(&env);return GH_SCENE_STEP_UNSUPPORTED;}volatile float next=value-rate;int good=isfinite(next);float stored=next;if(fesetenv(&env))return GH_SCENE_STEP_UNSUPPORTED;if(!good)return GH_SCENE_STEP_UNSUPPORTED;memcpy(e+0x544,&stored,4);}
  if(e[0x540])e[0x540]--;if(e[0x541])e[0x541]--;uint16_t n=h(e+0x542);if(n)p16(e+0x542,(uint16_t)(n-1));s->enemy_stage=2;
 }
 if(s->enemy_stage==2){static const unsigned pointers[]={0x500,0x560,0x564,0x568,0x56c,0x570,0x574,0x578,0x57c,0x580,0x584,0x588,0x58c,0x590,0x594,0x5ac,0x5b0};
  for(unsigned i=0;i<sizeof(pointers)/sizeof(*pointers);i++)if(w(e+pointers[i]))return fail(s,0x290250,GH_SCENE_STEP_UNSUPPORTED,(int)pointers[i]);if(b->init->enemy_top||b->init->enemy_last)return fail(s,0x290250,GH_SCENE_STEP_UNSUPPORTED,0);for(unsigned i=0;i<16;i++)if(b->init->enemy_kept[i])return fail(s,0x290250,GH_SCENE_STEP_UNSUPPORTED,(int)i);
  e[0x5a0]=e[0x5a1]=e[0x5a2]=0;
  /* Actual125F38 immediately returns onplayer+165C==0. Constructor leaves
   * thisfieldunwritten, so only anexplicitprovided/zero-reference player may
   * establish the value; never read arbitrary hostallocation asguestpointer. */
  gh_game_title_player*p=b->title_player;if(!p||!p->ready)return fail(s,0x2db6b0,GH_SCENE_STEP_UNSUPPORTED,0);if(p->view.initial_memory==GH_TITLE_MEMORY_UNSPECIFIED||p->view.size<0x1660||w(p->view.bytes+0x165c))return fail(s,0x125f38,GH_SCENE_STEP_UNSUPPORTED,0);
  p32(e+0x548,0x3f800000);s->enemy_stage=3;
 }
 if(!b->save.bytes||b->save.size<GH_CORE_SAVE_BYTES)return fail(s,0x291d48,GH_SCENE_STEP_ARGUMENT,0);
 uint32_t flags=w(b->save.bytes+0x14);if((int32_t)flags<0||e[0x518])return fail(s,0x291d48,GH_SCENE_STEP_UNSUPPORTED,0);if(!(flags&0x40000000)&&(w(raw+0x5e4)&0x20000000))e[0x518]=0;
 s->enemy_stage=0;return GH_SCENE_STEP_OK;
}
static gh_scene_step_status object_pool(gh_game_scene_step*s,const gh_game_scene_step_bindings*b){if(s->object_wait_acd0){s->object_wait_acd0--;return GH_SCENE_STEP_OK;}const uint8_t*p=b->init->object_pool;
 for(unsigned i=0;i<160;i++)if(p[(size_t)i*0x60+0x4a])return fail(s,0x1fceb0,GH_SCENE_STEP_UNSUPPORTED,(int)i);
 for(unsigned i=0;i<256;i++)if(p[0x3c00+(size_t)i*0x70+0x4a])return fail(s,0x1fd448,GH_SCENE_STEP_UNSUPPORTED,(int)i);return GH_SCENE_STEP_OK;
}
static int scenario_gate(const gh_game_scene_step_bindings*b){const uint8_t*r=(const uint8_t*)b->work;return (int32_t)w(r+0x58c)>=0&&!(w(r+0x5e4)&0x8000000)&&!(w(r+0x584)&8)&&!(w(r+0x5d8)&0x10000)&&!(w(r+0x590)&1);}
static gh_scene_step_status at_empty(gh_game_scene_step*s,const gh_game_scene_step_bindings*b){gh_scene_at_manager*a=b->scene_at;if(!a||!a->ready)return fail(s,0x2c1540,GH_SCENE_STEP_ARGUMENT,0);
 for(unsigned i=0;i<4;i++)if(a->units[i].count)return fail(s,0x2c30c8,GH_SCENE_STEP_UNSUPPORTED,(int)i);
 gh_at_link link=a->heads[15];unsigned steps=0;while(link.kind!=GH_AT_LINK_END){if(link.kind==GH_AT_LINK_UNIT)return fail(s,0x2c30c8,GH_SCENE_STEP_UNSUPPORTED,(int)link.index);if(link.kind!=GH_AT_LINK_SLOT||link.index>=16||++steps>16)return fail(s,0x2c30c8,GH_SCENE_STEP_BOUNDS,0);link=a->heads[link.index];}return GH_SCENE_STEP_OK;
}
static gh_scene_step_status action_empty(gh_game_scene_step*s,const gh_game_scene_step_bindings*b){const uint8_t*raw=(const uint8_t*)b->work;gh_game_action_buttons*a=b->action_buttons;
 if(!(w(raw+0x5e0)&0x100000)&&(w(raw+0x5d8)&0x400000))return GH_SCENE_STEP_OK;
 if(!a||!b->cockpit_action_flags_1414)return fail(s,0x1f75b0,GH_SCENE_STEP_ARGUMENT,0);*b->cockpit_action_flags_1414&=0xfe;
 if(!(w(raw+0x5e0)&0x100000)){gh_action_pool_link*link=a->pool+15;unsigned steps=0;uintptr_t base=(uintptr_t)a->pool;
  while(link){uintptr_t p=(uintptr_t)link;if(p<base||p-base>=sizeof(a->pool)||(p-base)%sizeof(*link)||++steps>16)return fail(s,0x1f7360,GH_SCENE_STEP_BOUNDS,0);if(link->flags==1)return fail(s,0x1f7360,GH_SCENE_STEP_UNSUPPORTED,(int)(link-a->pool));link=link->previous;}
 }
 for(unsigned i=0;i<8;i++)if((a->entries[i].flags&0x201)==1)return fail(s,a->entries[i].kind==3?0x1f6f58:0x1f7228,GH_SCENE_STEP_UNSUPPORTED,(int)i);return GH_SCENE_STEP_OK;
}
static gh_scene_step_status death_gate(gh_game_scene_step*s,const gh_game_scene_step_bindings*b){int16_t vitality;uint32_t f=0;
 if(s->death_callback_pending){gh_scene_step_status e=call(s,b,0x2c4390,0x3c2f84);if(e)return e;s->death_callback_pending=0;uint8_t*r=(uint8_t*)b->work;p32(r+0x5e0,w(r+0x5e0)|0x100000);return GH_SCENE_STEP_OK;}
 if(b->work->room==0x20)return GH_SCENE_STEP_OK;if(!b->title_player||!b->title_player->ready)return fail(s,0x2db6b0,GH_SCENE_STEP_UNSUPPORTED,0);
 if(gh_title_player_read_vitality(&b->title_player->view,&vitality,&f))return fail(s,f?f:0x2db6b0,GH_SCENE_STEP_UNSUPPORTED,0);
 if(!(b->scenario.manager->flags&2)&&vitality<1){b->scenario.manager->flags|=2;s->death_callback_pending=1;return death_gate(s,b);}
 if((b->scenario.manager->flags&2)&&vitality>0){b->scenario.manager->flags&=~2u;uint8_t*r=(uint8_t*)b->work;p32(r+0x5e0,w(r+0x5e0)&~0x100000u);}return GH_SCENE_STEP_OK;
}
typedef struct {gh_game_scene_step*s;const gh_game_scene_step_bindings*b;gh_task_step_fn original;void*context;gh_scene_step_status error;} traversal;
static int scenario_visit(void*context,gh_task_manager*m,gh_task_record32*record){traversal*t=context;uint8_t*request=t->b->scenario.bytes+0x50;
 if(m->current_index<0)return GH_THREAD_INVALID;if((uint32_t)m->current_index<t->s->scenario_task_cursor)return 0;t->s->scenario_task_cursor=(uint32_t)m->current_index;
 for(;;){p16(request,0);int e=t->original(t->context,m,record);if(e)return e;
  if(t->b->scenario_status){uint32_t frontier=0;t->error=t->b->scenario_status(t->b->scenario_status_context,&frontier);if(t->error){t->s->scenario_status_pending=1;t->s->scenario_task_cursor=(uint32_t)m->current_index+1;fail(t->s,frontier?frontier:0x2d5358,t->error,m->current_index);return GH_THREAD_UNSUPPORTED;}}
  uint16_t raw=h(request);if(!raw)return 0;int16_t frames=(int16_t)(raw<32768?(int)raw:(int)raw-65536);
  gh_thread_status status=gh_task_threads_sleep(t->b->main_threads,frames);if(status){fail(t->s,0x2d5aa0,GH_SCENE_STEP_THREAD,status);return status;}
  if(m->current_index<0||(uint32_t)m->current_index>=m->count)return 0;if(((m->flags&1)&&!(record->attributes&1))||((m->flags&2)&&!(record->attributes&2)))return 0;
 }
}
gh_scene_step_status gh_game_scene_scenario_before_002c3500(gh_game_scene_step*s,const gh_game_scene_step_bindings*b){gh_scene_step_status e;if(!s||!valid(b))return GH_SCENE_STEP_ARGUMENT;
 if(!s->scenario_before_stage){e=death_gate(s,b);if(e)return e;if(!scenario_gate(b))return GH_SCENE_STEP_OK;p32((uint8_t*)b->work+0x610,0xffff);s->scenario_before_stage=1;}
 if(s->scenario_before_stage==1){if(!(b->scenario.manager->flags&2)){e=at_empty(s,b);if(e)return e;}s->scenario_before_stage=2;}
 if(s->scenario_before_stage==2){const gh_scene_hook*hook=b->scenario_callback104;
  if(hook&&hook->kind==GH_SCENE_HOOK_NATIVE){uint32_t f=0;if(!hook->call)return fail(s,0x2c364c,GH_SCENE_STEP_ARGUMENT,0);gh_scene_status r=hook->call(hook->context,&f);if(r)return fail(s,f?f:0x2c364c,GH_SCENE_STEP_CALLEE,r);}
  else if((hook&&hook->kind==GH_SCENE_HOOK_UNRESOLVED)||w(b->scenario.bytes+0x104))return fail(s,0x2c364c,GH_SCENE_STEP_UNSUPPORTED,0);
  s->scenario_before_stage=3;
 }
 e=action_empty(s,b);if(e)return e;s->scenario_before_stage=0;return GH_SCENE_STEP_OK;
}
gh_scene_step_status gh_game_scene_tasks_002d5358(gh_game_scene_step*s,const gh_game_scene_step_bindings*b){if(!s||!b||!b->scenario.bytes||b->scenario.size<0x114||!b->scenario.manager)return GH_SCENE_STEP_ARGUMENT;gh_task_manager*m=b->scenario.manager;traversal t={s,b,m->step,m->step_context,GH_SCENE_STEP_OK};
 if(!b->main_threads||!b->scenario_threads||m->step!=gh_task_threads_step||m->step_context!=b->scenario_threads||m->count>16||m->capacity<16)return fail(s,0x2d5358,GH_SCENE_STEP_THREAD,0);
 if(s->scenario_status_pending){if(!b->scenario_status)return fail(s,0x2d5358,GH_SCENE_STEP_ARGUMENT,0);uint32_t frontier=0;gh_scene_step_status e=b->scenario_status(b->scenario_status_context,&frontier);if(e)return fail(s,frontier?frontier:0x2d5358,e,s->detail);s->scenario_status_pending=0;}
 p16(b->scenario.bytes+0x50,0);m->step=scenario_visit;m->step_context=&t;gh_task_run_result r=gh_task_threads_run_nested(b->main_threads,b->scenario_threads);m->step=t.original;m->step_context=t.context;
 if(t.error)return t.error;if(r.status!=GH_TASK_OK)return fail(s,s->frontier?s->frontier:0x2d57c8,GH_SCENE_STEP_THREAD,r.platform_error);s->scenario_task_cursor=0;return GH_SCENE_STEP_OK;
}
gh_scene_step_status gh_game_scene_scenario_move_002c3678(gh_game_scene_step*s,const gh_game_scene_step_bindings*b){gh_scene_step_status e;if(!s||!valid(b))return GH_SCENE_STEP_ARGUMENT;
 if(s->scenario_stage==0){e=death_gate(s,b);if(e)return e;if(!scenario_gate(b))return GH_SCENE_STEP_OK;s->scenario_stage=1;}
 if(s->scenario_stage==1){if(!(b->scenario.manager->flags&2)&&!(w((const uint8_t*)b->work+0x5d8)&0x100000)){
   if(!b->scene_at)return fail(s,0x2c14f8,GH_SCENE_STEP_ARGUMENT,0);b->scene_at->field60=b->scene_at->field68=b->scene_at->field6c=b->scene_at->field70=0;b->scene_at->field64=0;e=call(s,b,0x2c1618,0x5fea60);if(e)return e;
  }s->scenario_stage=2;}
 if(s->scenario_stage==2){
 if(!b->room_move||b->room_move->kind==GH_SCENE_HOOK_UNRESOLVED)return fail(s,(b->scenario.manager->flags&2)?0x2c37f0:0x2c37c0,GH_SCENE_STEP_UNSUPPORTED,0);
 if(b->room_move->kind==GH_SCENE_HOOK_NATIVE){uint32_t f=0;if(!b->room_move->call)return fail(s,0x2c37c0,GH_SCENE_STEP_ARGUMENT,0);gh_scene_status r=b->room_move->call(b->room_move->context,&f);if(r)return fail(s,f?f:0x2c37c0,GH_SCENE_STEP_CALLEE,r);}
 else if(b->room_move->kind!=GH_SCENE_HOOK_ABSENT)return GH_SCENE_STEP_ARGUMENT;s->scenario_stage=3;}
 if(s->scenario_stage==3){e=gh_game_scene_tasks_002d5358(s,b);if(e)return e;s->scenario_stage=4;}
 uint8_t*raw=(uint8_t*)b->work;
 if(s->scenario_stage==4){if((b->scenario.manager->flags&2)||(w(raw+0x5e4)&0x20000010)||!b->scenario.bytes[0x108]||!(w(raw+0x1a0)&0x800000)){s->scenario_stage=0;return GH_SCENE_STEP_OK;}p32(raw+0x5e4,w(raw+0x5e4)|0x10);s->scenario_stage=5;}
 if(s->scenario_stage==5){if(!b->fade)return fail(s,0x307208,GH_SCENE_STEP_UNSUPPORTED,0);if(!b->display||!b->display->initialized)return fail(s,0x3c1180,GH_SCENE_STEP_UNSUPPORTED,0);gh_screen_fade_context c={b->display->screen_width,b->display->gs_y_origin,w(raw+0x564),w(raw+0x5e0)};if(gh_screen_fade_start_00307208(b->fade,&c,0,8,0,0,0xff000000,15))return fail(s,0x307208,GH_SCENE_STEP_CALLEE,0);s->scenario_stage=6;}
 if(s->scenario_stage==6){e=call(s,b,0x2c3890,0x2c4ef0);if(e)return e;s->scenario_stage=7;}
 if(s->scenario_stage==7){uint32_t slot=w(b->scenario.bytes+0x10c);if(slot>=b->scenario.manager->count)return fail(s,0x2d59f0,GH_SCENE_STEP_BOUNDS,0);gh_thread_status te=gh_task_threads_cancel(b->scenario_threads,(int32_t)slot);if(te)return fail(s,0x2d59f0,GH_SCENE_STEP_THREAD,te);b->scenario.bytes[0x108]=0;s->scenario_stage=0;return GH_SCENE_STEP_OK;}
 return GH_SCENE_STEP_ARGUMENT;
}
static const uint32_t updates[][2]={{0x2b9d58,0x5e8040},{0x12e840,0x5cae50},{0x2d9be0,0x61a990},{0x296530,0x586af0},{0x2c3500,0x3c2f84},{0x138f98,0x463050},{0x290250,0x5864f0},{0x2d7560,0x604700},{0x1fdfd8,0x574380},{0x30f090,0},{0x1352f0,0x462fc0},{0x2bf2d0,0x3c2648},{0x1601d8,0x3bd930},{0x30f228,0},{0x1ffe10,0x583ec0},{0x2c3678,0x3c2f84},{0x138fc8,0x463050},{0x2c5470,0x754c50},{0x307b28,0x747b20},{0x2aefa8,0x3c23a4},{0x140e58,0x3bd6e8},{0x1fe748,0x580d20},{0x1fe748,0x5864e0},{0x2ca518,0x5fee00}};
static const uint32_t draws[][2]={{0x1538f0,0},{0x2c54e8,0x3c2fb0},{0x30f650,0},{0x2c3818,0x3c2f84},{0x2af028,0x3c23a4},{0x2fb310,0},{0x1410a8,0x3bd6e8},{0x160b90,0x3bd930}};
gh_scene_step_status gh_game_scene_step_002a6690(gh_game_scene_step*s,const gh_game_scene_step_bindings*b){gh_scene_step_status e;uint8_t*raw;if(!s||!valid(b))return GH_SCENE_STEP_ARGUMENT;raw=(uint8_t*)b->work;s->frontier=0;s->detail=0;
 if(!b->profile_counter)return fail(s,0x10000800,GH_SCENE_STEP_UNSUPPORTED,0);
 if(s->stage==0){s->profile_752cf0=b->profile_counter(b->profile_context);s->stage=1;}
 if(s->stage==1){if(w(raw+0x594)&0x800){e=call(s,b,0x201be0,0x585038);if(e)return e;}s->stage=2;}
 if(s->stage==2){e=nearcheck(s,b);if(e)return e;s->stage=3;}
 if(s->stage==3){if((w(raw+0x1a0)&0x200000)&&!s->cooldown_1b0&&!(w(raw+0x5e4)&0x2204000)&&!(w(raw+0x5d8)&0x2000000))return fail(s,0x12bab8,GH_SCENE_STEP_UNSUPPORTED,0);
  if(s->cooldown_1b0){uint16_t n=(uint16_t)((uint16_t)s->cooldown_1b0-1);s->cooldown_1b0=(int16_t)(n<32768?(int)n:(int)n-65536);}s->profile_752d44=s->profile_752cf0;s->profile_752cfc=b->profile_counter(b->profile_context);s->stage=4;
 }
 while(s->stage>=4&&s->stage<4+sizeof(updates)/sizeof(*updates)){unsigned i=s->stage-4;uint32_t a=updates[i][0];
  if(i==0){e=collision(s,b);if(!e)s->profile_752cfc-=b->profile_counter(b->profile_context);}
  else if(i==1)e=actors(s,b);else if(i==2){world_clock(b);e=GH_SCENE_STEP_OK;}
  else if(a==0x296530){if(!b->event)return fail(s,a,GH_SCENE_STEP_UNSUPPORTED,0);gh_game_event_status r=gh_game_event_update_00296530(b->event);e=r?fail(s,b->event->frontier?b->event->frontier:a,GH_SCENE_STEP_CALLEE,r):GH_SCENE_STEP_OK;}
  else if(a==0x2c3500)e=gh_game_scene_scenario_before_002c3500(s,b);
  else if(a==0x138f98){uint32_t f=0;gh_camera_status r=gh_game_camera_before_move_00138f98(b->init,&f);e=r?fail(s,f?f:a,GH_SCENE_STEP_CALLEE,r):GH_SCENE_STEP_OK;}
  else if(a==0x290250)e=enemies(s,b);
  else if(a==0x2d7560&&((w(raw+0x5d8)&0x4000)||(w(raw+0x5e0)&0x80000)))e=GH_SCENE_STEP_OK;
  else if(a==0x1fdfd8)e=object_pool(s,b);
  else if(a==0x1352f0)e=GH_SCENE_STEP_OK; /*actualJRRA/NOP*/
  else if(a==0x2c3678)e=gh_game_scene_scenario_move_002c3678(s,b);
  else if(a==0x2c5470&&(w(raw+0x5d8)&0x20000))e=GH_SCENE_STEP_OK;
  else e=call(s,b,a,updates[i][1]);if(e)return e;s->stage++;
 }
 if(s->stage==28){s->profile_752d44-=b->profile_counter(b->profile_context);s->word_5caff4=0;s->stage=29;}
 while(s->stage>=29&&s->stage<37){unsigned i=s->stage-29;e=call(s,b,draws[i][0],draws[i][1]);if(e)return e;s->stage++;}
 if(s->stage!=37)return GH_SCENE_STEP_ARGUMENT;s->profile_752cf0-=b->profile_counter(b->profile_context);s->completed_frames++;s->stage=0;s->collision_cursor=0;return GH_SCENE_STEP_OK;
}
