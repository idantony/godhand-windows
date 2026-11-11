#include "r100_scripts.h"
#include <string.h>
static uint32_t rd(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
static unsigned half(const uint8_t*p){return p[0]|(unsigned)p[1]<<8;}
static void wr(uint8_t*p,uint32_t n){memcpy(p,&n,4);}
static gh_r100_script_status fail(gh_r100_scripts*s,uint32_t pc,int relative,gh_r100_script_status e){s->frontier=pc;s->frontier_relative=(uint8_t)relative;s->status=e;return e;}
gh_r100_script_status gh_r100_get_clear_num_001fa6c0(gh_core_save_view save,const gh_boot_work*w,int16_t*out){
 if(!w||!out)return GH_R100_SCRIPT_ARGUMENT;if(!save.bytes){*out=0;return GH_R100_SCRIPT_OK;}if(save.size<0x12)return GH_R100_SCRIPT_BOUNDS;
 unsigned raw=half(save.bytes+0x10);int value=raw<32768?(int)raw:(int)raw-65536;if((w->flags59c&0x40000)&&!value)value=1;*out=(int16_t)value;return GH_R100_SCRIPT_OK;
}
gh_r100_script_status gh_r100_scripts_bind(gh_r100_scripts*s,const gh_r100_script_bindings*b){
 if(!s||s->bound||!b||!b->work||!b->room||!b->room->active||b->room->size<8||!b->disc||!b->threads||!b->scenario.bytes||!b->scenario.manager||gh_task_threads_manager(b->threads)!=b->scenario.manager||b->scenario.size<0x114||b->play.work!=b->work||!b->play.event||!b->play.config||b->soft.work!=b->work||b->soft.scenario.bytes!=b->scenario.bytes||b->soft.scenario.manager!=b->scenario.manager)return GH_R100_SCRIPT_ARGUMENT;
 s->b=*b;s->bound=1;s->slot=-1;return GH_R100_SCRIPT_OK;
}
static gh_r100_script_status other(gh_r100_scripts*s,uint32_t rel){if(!s->b.register_other)return fail(s,rel,1,GH_R100_SCRIPT_UNSUPPORTED);uint32_t f=0;gh_r100_script_status e=s->b.register_other(s->b.context,rel,-1,&f);return e?fail(s,f?f:rel,f?0:1,e):GH_R100_SCRIPT_OK;}
static gh_r100_script_status enemy(gh_r100_scripts*s,unsigned entry,const void**out){
 if(!s->b.init)return fail(s,0x290868,0,GH_R100_SCRIPT_ARGUMENT);
 if(!rd(s->b.init->enemies+0x500)&&!s->b.init->enemy_top){*out=NULL;return GH_R100_SCRIPT_OK;} /* actualGetEm emptylinkedlist */
 if(!s->b.get_enemy)return fail(s,0x290868,0,GH_R100_SCRIPT_UNSUPPORTED);uint32_t f=0;gh_r100_script_status e=s->b.get_enemy(s->b.context,(uint8_t)entry,out,&f);return e?fail(s,f?f:0x290868,0,e):GH_R100_SCRIPT_OK;
}
static gh_r100_script_status find_gate(gh_r100_scripts*s,const char*name,gh_r100_om_view*out){
 *out=(gh_r100_om_view){0};if(!s->b.pools)return fail(s,0x14fd98,0,GH_R100_SCRIPT_ARGUMENT);const gh_room_vector*v;if(gh_room_registry_view(s->b.pools,0x754c58,&v))return fail(s,0x14fd98,0,GH_R100_SCRIPT_BOUNDS);
 uint64_t key=0;for(unsigned k=0;k<8&&name[k];k++)key|=(uint64_t)(uint8_t)name[k]<<(k*8);
 for(uint32_t i=0;i<v->count;i++){if(!s->b.object_view)return fail(s,0x14fd98,0,GH_R100_SCRIPT_UNSUPPORTED);gh_r100_om_view o={0};uint32_t f=0;gh_r100_init_status e=s->b.object_view(s->b.context,rd((const uint8_t*)v->allocation.pointer+(size_t)i*4),&o,&f);if(e)return fail(s,f?f:0x14fd98,0,GH_R100_SCRIPT_CALLEE);if(!o.bytes||o.size<0x540||!o.identity)return fail(s,0x14fd98,0,GH_R100_SCRIPT_BOUNDS);unsigned id=half(o.bytes+0x2fe);uint64_t bits=(uint64_t)rd(o.bytes+0x538)|(uint64_t)rd(o.bytes+0x53c)<<32;if(id>=0x300&&id<0x500&&bits==key){*out=o;break;}}
 return GH_R100_SCRIPT_OK;
}
gh_r100_script_status gh_r100_script_setup_00000bc0(gh_r100_scripts*s){
 if(!s||!s->bound)return GH_R100_SCRIPT_ARGUMENT;uint32_t flags=rd(s->b.room->active+4);gh_r100_script_status e;s->frontier=0;s->frontier_relative=0;s->status=GH_R100_SCRIPT_OK;
 while(s->setup_stage<18){unsigned k=s->setup_stage;
  if(k<8){uint32_t rel=0;switch(k){case 0:if(!(flags&0x20000000))rel=0x2920;break;case 1:if(!(flags&0x10000000))rel=0x2650;break;case 2:rel=0x2490;break;case 3:rel=0x2288;break;case 4:rel=0x2198;break;case 5:rel=0x15b0;break;case 6:rel=!(flags&0x2000000)?0x2d68:!(flags&0x800000)?0x11b0:0;break;case 7:rel=!(flags&0x40000000)?0x1bd0:!(flags&0x8000000)?0x1780:0;break;default:break;}if(rel){e=other(s,rel);if(e)return e;}}
  else if(k==8||k==9){e=enemy(s,k==8?6:14,&s->enemies[k-8]);if(e)return e;}
  else if(k==10||k==11){unsigned i=k-10;if(s->enemies[i]){if(!s->b.set_escape)return fail(s,0xbc0,1,GH_R100_SCRIPT_UNSUPPORTED);static const uint32_t pos[2][4]={{0x4102147b,0x3e051eb8,0x41987ae1,0x3f800000},{0x416051ec,0x3eeb851f,0xc260a3d7,0x3f800000}};float v[4];memcpy(v,pos[i],16);uint32_t f=0;e=s->b.set_escape(s->b.context,s->enemies[i],v,&f);if(e)return fail(s,f?f:0xbc0,f?0:1,e);}}
  else if(k==12){if(flags&0x40000000){if(!s->b.collision)return fail(s,0x1332d0,0,GH_R100_SCRIPT_UNSUPPORTED);uint32_t f=0;gh_r100_init_status r=s->b.collision(s->b.context,"H03_gt",0,&f);if(r)return fail(s,f?f:0x1332d0,0,GH_R100_SCRIPT_CALLEE);}}
  else if(k==13||k==15){e=find_gate(s,k==13?"gate00":"gate01",&s->gates[k==13?0:1]);if(e)return e;}
  else if(k==14||k==16){unsigned i=k==14?0:1;if(s->gates[i].bytes){if(!s->b.door)return fail(s,0xbc0,1,GH_R100_SCRIPT_UNSUPPORTED);uint32_t f=0;
   if(!s->gate_stage){if(flags&0x40000000){e=s->b.door(s->b.context,s->gates[i].identity,2,0,1,&f);if(e)return fail(s,f?f:0xbc0,f?0:1,e);}s->gate_stage=1;}
   if(s->gate_stage==1){e=s->b.door(s->b.context,s->gates[i].identity,0,1,0,&f);if(e)return fail(s,f?f:0xbc0,f?0:1,e);s->gate_stage=2;}
   if(s->gate_stage==2){e=s->b.door(s->b.context,s->gates[i].identity,1,0,0,&f);if(e)return fail(s,f?f:0xbc0,f?0:1,e);s->gate_stage=3;}
  }s->gate_stage=0;}  s->setup_stage++;
 }
 return GH_R100_SCRIPT_OK;
}
static gh_r100_script_status sleep_one(gh_r100_scripts*s){s->sleep_calls++;gh_thread_status e=gh_task_threads_sleep(s->b.threads,1);if(e)return fail(s,0x2d5aa0,0,GH_R100_SCRIPT_THREAD);return GH_R100_SCRIPT_OK;}
static gh_r100_script_status advance(gh_r100_scripts*s){
 gh_r100_script_status e;uint8_t*w=(uint8_t*)s->b.work;s->frontier=0;s->frontier_relative=0;s->status=GH_R100_SCRIPT_OK;
 if(!s->stage){wr(s->b.room->active+4,rd(s->b.room->active+4)|0x1000000);size_t index;gh_disc_error d=gh_disc_find(s->b.disc,"event/ev01.dat",&index);if(d==GH_DISC_NOT_FOUND)s->stage=6;else if(d)return fail(s,0x2011f8,0,GH_R100_SCRIPT_DISC);else s->stage=1;}
 if(s->stage==1){while(rd(w+0x5e4)&0x10){e=sleep_one(s);if(e)return e;}s->stage=2;}
 if(s->stage==2){gh_soft_event_status x=gh_scenario_start_soft_event_002c39b0(&s->soft_start,&s->b.soft,4);if(x)return fail(s,s->soft_start.frontier?s->soft_start.frontier:0x2c39b0,0,GH_R100_SCRIPT_CALLEE);s->stage=3;}
 if(s->stage==3){uint32_t f=0;gh_game_event_status x=gh_game_event_play_start_00296458(&s->b.play,1,&f);if(x)return fail(s,f?f:0x296458,0,GH_R100_SCRIPT_CALLEE);s->stage=4;}
 if(s->stage==4){while((s->b.play.event->fields.flags&1)||s->b.play.event->release_resume){e=sleep_one(s);if(e)return e;}s->stage=5;}
 if(s->stage==5){if(!s->b.end_soft_002c3c10)return fail(s,0x2c3c10,0,GH_R100_SCRIPT_UNSUPPORTED);uint32_t f=0;e=s->b.end_soft_002c3c10(s->b.context,&f);if(e)return fail(s,f?f:0x2c3c10,0,e);s->stage=6;}
 if(s->stage==6){if(!s->b.checkpoint_002c05b0)return fail(s,0x2c05b0,0,GH_R100_SCRIPT_UNSUPPORTED);uint32_t f=0;e=s->b.checkpoint_002c05b0(s->b.context,&f);if(e)return fail(s,f?f:0x2c05b0,0,e);s->stage=7;}
 if(s->stage==7){e=other(s,0x10f8);if(e)return e;s->stage=8;}
 if(s->stage==8){e=gh_r100_script_setup_00000bc0(s);if(e)return e;s->stage=9;}
 s->finished=1;return GH_R100_SCRIPT_OK;
}
static void script9b0(void*context,int32_t argument){gh_r100_scripts*s=context;(void)argument;s->entered=1;
 while(!s->finished){s->status=advance(s);if(s->status!=GH_R100_SCRIPT_OK){/* Explicitnativefrontier: retainfiber/owner forinspection, no normalexit. */if(sleep_one(s))return;}}
 /* Originalscript tail isTask.exit andhasno successor. Nativeentryreturn
  * retiresitsfiber usingtheexistingbackend's actualnormal-return adapter. */
}
gh_r100_script_status gh_r100_scripts_register_000009b0(gh_r100_scripts*s,int32_t request,int32_t*slot){
 if(!s||!s->bound||!slot)return GH_R100_SCRIPT_ARGUMENT;if(s->registered){*slot=s->slot;return GH_R100_SCRIPT_OK;}uint32_t f=0;gh_scenario_exec_status e=gh_scenario_exec_002c3890(s->b.scenario,s->b.threads,request,script9b0,s,0,slot,&f);if(e)return fail(s,f?f:0x2c3890,0,GH_R100_SCRIPT_THREAD);s->slot=*slot;s->registered=1;s->status=GH_R100_SCRIPT_OK;s->frontier=0;s->frontier_relative=0;return GH_R100_SCRIPT_OK;
}
