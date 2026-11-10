#include "r100_counter_task.h"
#include <string.h>
static uint32_t rd(const uint8_t*p){uint32_t x;memcpy(&x,p,4);return x;}
static void wr(uint8_t*p,uint32_t x){memcpy(p,&x,4);}
static gh_r100_script_status fail(gh_r100_counter_task*s,uint32_t f,int relative,gh_r100_script_status e){s->frontier=f;s->frontier_relative=(uint8_t)relative;s->status=e;return e;}
gh_r100_script_status gh_r100_counter_bind(gh_r100_counter_task*s,const gh_r100_counter_bindings*b){
 if(!s||s->bound||!b||!b->room_init||!b->room_init->work.pointer||!b->room||!b->room->active||b->room->size<0x1614||!b->init||!b->threads||gh_task_threads_manager(b->threads)!=b->scenario.manager||!b->scenario.bytes||b->scenario.size<0x114)return GH_R100_SCRIPT_ARGUMENT;
 s->b=*b;s->slot=-1;s->bound=1;return GH_R100_SCRIPT_OK;
}
static gh_r100_script_status count(gh_r100_counter_task*s,uint32_t*out,int*has614){
 if(!rd(s->b.init->enemies+0x500)&&!s->b.init->enemy_top){*out=0;*has614=0;return GH_R100_SCRIPT_OK;} /* actual1028 return0 beforelist */
 if(!s->b.count_alive)return fail(s,0x1028,1,GH_R100_SCRIPT_UNSUPPORTED);uint32_t f=0;gh_r100_script_status e=s->b.count_alive(s->b.context,out,has614,&f);if(e)return fail(s,f?f:0x1028,f?0:1,e);
 if(*out>64||(*has614!=0&&*has614!=1))return fail(s,0x1028,1,GH_R100_SCRIPT_BOUNDS);return GH_R100_SCRIPT_OK;
}
static gh_r100_script_status sleep1(gh_r100_counter_task*s){s->sleep_calls++;gh_thread_status e=gh_task_threads_sleep(s->b.threads,1);return e?fail(s,0x2d5aa0,0,GH_R100_SCRIPT_THREAD):GH_R100_SCRIPT_OK;}
static gh_r100_script_status advance(gh_r100_counter_task*s){
 s->frontier=0;s->frontier_relative=0;s->status=GH_R100_SCRIPT_OK;gh_r100_script_status e;uint8_t*w=s->b.room_init->work.pointer,*free0=s->b.room->active+0x1610;
 if(!s->stage){e=sleep1(s);if(e)return e;s->stage=1;}
 if(s->stage==1){uint32_t alive=0;int flag=0;e=count(s,&alive,&flag);if(e)return e;if(flag){w[5]=1;s->b.room_init->known_work_bytes|=1u<<5;}wr(w,alive+rd(free0));s->b.room_init->known_work_bytes|=15;s->stage=2;}
 for(;;){w[5]=0;s->b.room_init->known_work_bytes|=1u<<5;uint32_t alive=0;int flag=0;e=count(s,&alive,&flag);if(e)return e;if(flag)w[5]=1;wr(free0,rd(w)-alive);e=sleep1(s);if(e)return e;}
}
static void counter(void*ctx,int32_t arg){gh_r100_counter_task*s=ctx;(void)arg;s->entered=1;for(;;){s->status=advance(s);if(sleep1(s))return;}}
gh_r100_script_status gh_r100_counter_register_000010f8(gh_r100_counter_task*s,int32_t request,int32_t*slot){
 if(!s||!s->bound||!slot)return GH_R100_SCRIPT_ARGUMENT;if(s->registered){*slot=s->slot;return GH_R100_SCRIPT_OK;}
 uint32_t f=0;gh_scenario_exec_status e=gh_scenario_exec_002c3890(s->b.scenario,s->b.threads,request,counter,s,0,slot,&f);if(e)return fail(s,f?f:0x2c3890,0,GH_R100_SCRIPT_THREAD);s->slot=*slot;s->registered=1;return GH_R100_SCRIPT_OK;
}
