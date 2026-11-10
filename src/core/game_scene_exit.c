#include "game_scene_exit.h"
#include "game_scene_aux.h"
#include "scene_at_units.h"
#include <string.h>
static uint32_t w(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static void put(uint8_t*p,uint32_t n){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(n>>(8*i));}
static gh_scene_exit_status fail(gh_game_scene_exit*s,uint32_t pc,gh_scene_exit_status e){s->frontier=pc;return e;}
static gh_scene_exit_status call(gh_game_scene_exit*s,const gh_game_scene_exit_bindings*b,uint32_t pc,int*result){
 if(!b->external)return fail(s,pc,GH_SCENE_EXIT_UNSUPPORTED);uint32_t f=0;int ignored=0;uint32_t arg=0;switch(pc){case 0x2c0038:case 0x2bf948:arg=0x3c264c;break;case 0x2caa00:case 0x2cab58:case 0x2ca910:arg=0x5fee00;break;case 0x12e800:arg=0x5cae50;break;case 0x138d40:arg=0x463050;break;case 0x141628:arg=0x3bd6e8;break;case 0x1612a8:arg=0x3bd930;break;case 0x2ad108:arg=0x3c2388;break;case 0x201018:arg=0x583f20;break;default:break;}
 gh_scene_exit_status e=b->external(b->context,pc,arg,result?result:&ignored,&f);return e?fail(s,f?f:pc,e):GH_SCENE_EXIT_OK;
}
gh_scene_exit_status gh_scene_exit_effects_00305b90(uint8_t*a,size_t az,uint8_t*b,size_t bz,uint32_t*frontier){
 if(frontier)*frontier=0;if(!a||az<0x100180||!b||bz<0x26060)return GH_SCENE_EXIT_ARGUMENT;
 for(unsigned i=0;i<256;i++){unsigned n=(i>>5)*4;uint32_t m=0x80000000u>>(i&31);if((w(b+0x26000+n)&m)&&!(w(b+0x26020+n)&m)&&!(w(b+0x26040+n)&m)){if(frontier)*frontier=0x302918;return GH_SCENE_EXIT_UNSUPPORTED;}}
 for(unsigned i=0;i<1024;i++){unsigned n=(i>>5)*4;uint32_t m=0x80000000u>>(i&31);if((w(a+0x100000+n)&m)&&!(w(a+0x100080+n)&m)&&!(w(a+0x100100+n)&m)){if(frontier)*frontier=0x2fbe50;return GH_SCENE_EXIT_UNSUPPORTED;}}
 return GH_SCENE_EXIT_OK;
}
static gh_scene_exit_status movie_stop_all(gh_game_scene_exit*s,const gh_game_scene_exit_bindings*b){
 if(!b->movies)return fail(s,0x2b4da8,GH_SCENE_EXIT_ARGUMENT);
 for(unsigned i=0;i<3;i++){gh_movie_track_view v;if(gh_movie_pool_track_at(b->movies,i,&v))return fail(s,0x2b4da8,GH_SCENE_EXIT_ARGUMENT);if(v.native_player&&gh_movie_pool_stop_002b4d68(b->movies,b->loop->work,v.cached_handle))return fail(s,0x2d42e0,GH_SCENE_EXIT_CALLEE);}
 uint8_t*p=(uint8_t*)b->loop->work;put(p+0x5e4,w(p+0x5e4)&~0x2000u);return GH_SCENE_EXIT_OK;
}
static gh_scene_exit_status hook(gh_game_scene_exit*s,gh_scene_hook*h,uint8_t*raw,uint32_t pc){
 if(!h){if(w(raw))return fail(s,pc,GH_SCENE_EXIT_UNSUPPORTED);return GH_SCENE_EXIT_OK;}
 if(h->kind==GH_SCENE_HOOK_ABSENT){if(w(raw))return fail(s,pc,GH_SCENE_EXIT_UNSUPPORTED);return GH_SCENE_EXIT_OK;}
 if(h->kind!=GH_SCENE_HOOK_NATIVE||!h->call)return fail(s,pc,GH_SCENE_EXIT_UNSUPPORTED);
 uint32_t f=0;gh_scene_status e=h->call(h->context,&f);if(e)return fail(s,f?f:pc,GH_SCENE_EXIT_CALLEE);
 h->kind=GH_SCENE_HOOK_ABSENT;h->call=NULL;put(raw,0);return GH_SCENE_EXIT_OK;
}
static gh_scene_exit_status scenario_stop(gh_game_scene_exit*s,const gh_game_scene_exit_bindings*b){
 if(!b->scenario.bytes||b->scenario.size<0x114||!b->scenario.manager||!b->scenario_threads)return fail(s,0x2c3858,GH_SCENE_EXIT_ARGUMENT);gh_scene_exit_status e;
 if(!s->scenario_stage){e=hook(s,b->scenario_pending_f4,b->scenario.bytes+0xf4,0x2c4de0);if(e)return e;s->scenario_stage=1;}
 if(s->scenario_stage==1){e=hook(s,b->scenario_exit_ec,b->scenario.bytes+0xec,0x2c3820);if(e)return e;s->scenario_stage=2;}
 while(s->scenario_cursor<b->scenario.manager->count){if(gh_task_threads_cancel(b->scenario_threads,(int32_t)s->scenario_cursor)!=GH_THREAD_OK)return fail(s,0x2d59f0,GH_SCENE_EXIT_THREAD);s->scenario_cursor++;}
 s->scenario_stage=0;s->scenario_cursor=0;return GH_SCENE_EXIT_OK;
}
static gh_scene_exit_status dead_objects(gh_game_scene_exit*s,const gh_game_scene_exit_bindings*b){
 const gh_room_vector*v=NULL;if(gh_room_registry_view(b->pools,0x754c30,&v)||!v)return fail(s,0x30f8b0,GH_SCENE_EXIT_ARGUMENT);if(v->count)return call(s,b,0x30f8b0,NULL);return GH_SCENE_EXIT_OK;
}
static gh_scene_exit_status event_clear(gh_game_scene_exit*s,gh_game_event*e){
 if(!e||!e->initialized)return fail(s,0x296350,GH_SCENE_EXIT_ARGUMENT);e->fields.text_token=0;e->fields.state=e->fields.phase=e->fields.unknown06=e->fields.unknown07=0;e->fields.flags=0;e->fields.data_number=0;return GH_SCENE_EXIT_OK;
}
static gh_scene_exit_status phase1(gh_game_scene_exit*s,const gh_game_scene_exit_bindings*b){
 uint8_t*p=(uint8_t*)b->loop->work;gh_scene_exit_status e;uint32_t f=0;
 for(;;){switch(s->operation){
 case 0:put(p+0x5e4,w(p+0x5e4)&~0x2000000u);break;
 case 1:gh_scene_background_mode_0030e5c0(b->loop->work,1,0,0,0);break;
 case 2:if(!b->fade)return fail(s,0x3076e0,GH_SCENE_EXIT_ARGUMENT);gh_screen_fade_kill_003076e0(b->fade);break;
 case 3:if((e=movie_stop_all(s,b)))return e;break;
 case 4:e=gh_scene_exit_effects_00305b90(b->scene_buffer_a,b->scene_buffer_a_size,b->scene_buffer_b,b->scene_buffer_b_size,&f);if(e)return fail(s,f?f:0x305b90,e);break;
 case 5:if((e=scenario_stop(s,b)))return e;break;
 case 6:{uint16_t room=b->loop->work->room;int special=(room>=0x40&&room<=0x42)||room==0x4e||room==0x4f;
  if((w(b->save.bytes+0x14)&0x4000000)&&special){if((e=call(s,b,0x2c0038,NULL)))return e;}
  else if(w(p+0x584)&0x4000000){if((e=call(s,b,0x2bf948,NULL)))return e;}}break;
 case 7:if((e=event_clear(s,b->event)))return e;break;
 case 8:if((e=call(s,b,0x2caa00,NULL)))return e;break;
 case 9:if(!b->module)return fail(s,0x2be818,GH_SCENE_EXIT_ARGUMENT);if(gh_room_module_release(b->module))return fail(s,0x2be818,GH_SCENE_EXIT_CALLEE);break;
 case 10:if((e=dead_objects(s,b)))return e;break;
 case 11:if((e=call(s,b,0x12e800,NULL)))return e;break;
 case 12:put(p+0x598,w(p+0x598)&~0x2000000u);break;
 case 13:if((e=call(s,b,0x138d40,NULL)))return e;break;
 case 14:break; /*actual141620 JRRA/NOP*/
 case 15:if((e=call(s,b,0x141628,NULL)))return e;break;
 case 16:if((e=call(s,b,0x1612a8,NULL)))return e;break;
 case 17:if(!b->scene_at)return fail(s,0x2c1270,GH_SCENE_EXIT_ARGUMENT);for(unsigned i=0;i<8;i++)if(b->scene_at->jump_slots[i].bytes)return fail(s,0x30f5a0,GH_SCENE_EXIT_UNSUPPORTED);
  /* The verified empty-jump quit also ends this room's native list companion.
   * Retaining it would bind the next room to the old bundle/room identity. */
  if(gh_scene_at_units_release(b->scene_at))return fail(s,0x2c1270,GH_SCENE_EXIT_CALLEE);break;
 case 18:if((e=call(s,b,0x2ad108,NULL)))return e;break;
 default:return GH_SCENE_EXIT_OK;
 }s->operation++;}
}
static gh_scene_exit_status phase5(gh_game_scene_exit*s,const gh_game_scene_exit_bindings*b){
 static const uint32_t calls[]={0x201018,0x2ca910,0x1fef30,0x1fef30,0x1ff028,0x30f5f0,0x150078,0x1492c8,0x2a9528,0x1fdfa8,0x2a71f0,0x2a7280,0x2d73e0};
 static const uint32_t args[]={0x583f20,0x5fee00,0x580d20,0x5864e0,0x5864e0,0,0,0,0x754240,0x574380,0x5e8040,0x5e8040,0x604700};
 while(s->operation<13){uint32_t pc=calls[s->operation],f=0;int ignored=0;
  if(!b->external)return fail(s,pc,GH_SCENE_EXIT_UNSUPPORTED);
  gh_scene_exit_status e=b->external(b->context,pc,args[s->operation],&ignored,&f);if(e)return fail(s,f?f:pc,e);s->operation++;
 }
 uint8_t*p=(uint8_t*)b->loop->work;uint16_t next=(uint16_t)(p[0x5b4]|(uint16_t)p[0x5b5]<<8),old=(uint16_t)(p[0x5b0]|(uint16_t)p[0x5b1]<<8);
 p[0x5bb]=p[0x5b9];p[0x5b6]=(uint8_t)old;p[0x5b7]=(uint8_t)(old>>8);p[0x5b9]=p[0x5ba];p[0x5b0]=(uint8_t)next;p[0x5b1]=(uint8_t)(next>>8);
 if(!(w(b->save.bytes+0x14)&0x4000000)||next){
  if(next==0x20){put(p+0x584,w(p+0x584)|0x8000);s->operation=0;return GH_SCENE_EXIT_OK;}
  if((w(p+0x598)&0x40)&&(old>>8)!=(next>>8)&&(old>>8)&&(next>>8))b->loop->game_state[0]=2;
  else if(w(b->save.bytes+0xb8c)&1){b->loop->game_state[0]=2;put(b->save.bytes+0xb8c,w(b->save.bytes+0xb8c)&~1u);}
  else b->loop->game_state[0]=3;
 }else b->loop->game_state[0]=3;
 b->loop->game_state[1]=b->loop->game_state[2]=b->loop->game_state[3]=0;s->phase=0;s->operation=0;return GH_SCENE_EXIT_OK;
}

gh_scene_exit_status gh_game_scene_exit_002a67d0(gh_game_scene_exit*s,const gh_game_scene_exit_bindings*b){
 if(!s||!b||!b->loop||!b->loop->work||!b->save.bytes||b->save.size<GH_CORE_SAVE_BYTES)return GH_SCENE_EXIT_ARGUMENT;
 if(b->system_menu&&b->system_menu->error)return fail(s,b->system_menu->frontier,GH_SCENE_EXIT_CALLEE);
 uint8_t phase=b->loop->game_state[1];if(s->phase!=phase){if(s->operation)return fail(s,0x2a67d0,GH_SCENE_EXIT_ARGUMENT);s->phase=phase;}s->frontier=0;gh_scene_exit_status e;
 switch(phase){
 case 0:if(!b->system_menu)return fail(s,0x2b65b0,GH_SCENE_EXIT_ARGUMENT);if(gh_system_menu_start_002b65b0(b->system_menu,&b->system_menu_bindings))return fail(s,b->system_menu->frontier?b->system_menu->frontier:0x2b65b0,GH_SCENE_EXIT_CALLEE);break;
 case 1:if((e=phase1(s,b)))return e;s->operation=0;break;
 case 2:if(!s->operation){if((e=dead_objects(s,b)))return e;s->operation=1;}
  {int ready=0;if((e=call(s,b,0x2cab58,&ready)))return e;if(ready!=1){uint32_t f=0;if(gh_game_objects_update_001fe748(b->objects,&f)||gh_game_objects_update_001fe748(b->room_objects,&f))return fail(s,f?f:0x1fe748,GH_SCENE_EXIT_CALLEE);s->operation=0;s->completed_calls++;return GH_SCENE_EXIT_OK;}}s->operation=0;break;
 case 3:case 4:break;
 case 5:e=phase5(s,b);if(!e)s->completed_calls++;return e;
 default:s->completed_calls++;return GH_SCENE_EXIT_OK;
 }
 b->loop->game_state[1]++;s->phase=b->loop->game_state[1];s->completed_calls++;return GH_SCENE_EXIT_OK;
}
