#include "r100_counter_task.h"
#include <string.h>
static uint32_t rd(const uint8_t*p){uint32_t x;memcpy(&x,p,4);return x;}
static void wr(uint8_t*p,uint32_t x){memcpy(p,&x,4);}
static gh_r100_script_status fail(gh_r100_counter_task*s,uint32_t f,int relative,gh_r100_script_status e){s->frontier=f;s->frontier_relative=(uint8_t)relative;s->status=e;return e;}
gh_r100_script_status gh_r100_counter_bind(gh_r100_counter_task*s,const gh_r100_counter_bindings*b){
 if(!s||s->bound||!b||!b->room_init||!b->room_init->work.pointer||!b->room||!b->room->active||b->room->size<0x1614||!b->init||!b->threads||gh_task_threads_manager(b->threads)!=b->scenario.manager||!b->scenario.bytes||b->scenario.size<0x114)return GH_R100_SCRIPT_ARGUMENT;
 s->b=*b;s->slot=-1;s->bound=1;return GH_R100_SCRIPT_OK;
}
static uint16_t half(const void *p)
{
    uint16_t value;
    memcpy(&value,p,2);
    return value;
}

static int placement_dead(const gh_room_state *room,int16_t stamp)
{
    if(stamp==0xff)return 0;
    for(unsigned i=0;i<room->active[8];i++){
        const uint8_t *record=room->active+12+i*24;
        if(record[0x13]==stamp)return record[0x14]==1;
    }
    return 1;
}

gh_r100_script_status gh_r100_count_alive_00001028(const gh_game_init_state *init,
    const gh_room_state *room,uint32_t *alive,int *has614)
{
    if(!init||!room||!room->active||!alive||!has614)return GH_R100_SCRIPT_ARGUMENT;
    if(!init->enemy_top){
        if(rd(init->enemies+0x500))return GH_R100_SCRIPT_UNSUPPORTED;
        *alive=0;*has614=0;
        return GH_R100_SCRIPT_OK;
    }
    if(room->size<12||room->active[8]>64||12+(size_t)room->active[8]*24>room->size)
        return GH_R100_SCRIPT_BOUNDS;
    unsigned index=64;
    for(unsigned i=0;i<64;i++){
        if(init->enemy_top==init->enemies+i*20){index=i;break;}
    }
    uint64_t visited=0;
    uint32_t count=0;
    int flag=0;
    while(index!=UINT8_MAX){
        if(index>=64||(visited&(UINT64_C(1)<<index)))return GH_R100_SCRIPT_UNSUPPORTED;
        visited|=UINT64_C(1)<<index;
        if(init->enemies[index*20+16]!=1)return GH_R100_SCRIPT_UNSUPPORTED;
        const gh_game_title_player *actor=init->enemy_actors[index];
        index=init->enemy_next[index];
        if(!actor)continue;
        if(!actor->view.bytes||actor->view.size<0x300)return GH_R100_SCRIPT_BOUNDS;
        const uint8_t *p=actor->view.bytes;
        unsigned kind=half(p+0x2fe);
        if(kind<0x200||kind>=0x2a0)continue;
        if(actor->view.size<0x642)return GH_R100_SCRIPT_BOUNDS;
        if(placement_dead(room,(int16_t)half(p+0x640)))continue;
        ++count;
        if(p[0x614]==1)flag=1;
    }
    *alive=count;*has614=flag;
    return GH_R100_SCRIPT_OK;
}

static gh_r100_script_status count(gh_r100_counter_task*s,uint32_t*out,int*has614){
 if(!rd(s->b.init->enemies+0x500)&&!s->b.init->enemy_top){*out=0;*has614=0;return GH_R100_SCRIPT_OK;}
 uint32_t f=0;
 gh_r100_script_status e=s->b.count_alive?s->b.count_alive(s->b.context,out,has614,&f):
  gh_r100_count_alive_00001028(s->b.init,s->b.room,out,has614);
 if(e)return fail(s,f?f:0x1028,f?0:1,e);
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
