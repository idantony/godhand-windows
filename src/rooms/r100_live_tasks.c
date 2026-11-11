#include "r100_live_tasks.h"
#include "../core/game_enemy_ems.h"
#include <string.h>
#include <fenv.h>
#include <math.h>

static const uint32_t entries[]={0x2920,0x2650,0x2490,0x2288,0x2198,0x15b0,0x2d68,0x1bd0};
static uint32_t rd(const void *p){uint32_t n;memcpy(&n,p,4);return n;}
static uint16_t half(const void *p){uint16_t n;memcpy(&n,p,2);return n;}
static void wr(void *p,uint32_t n){memcpy(p,&n,4);}

static int index_of(uint32_t entry)
{
    for(unsigned i=0;i<sizeof(entries)/sizeof(entries[0]);i++)if(entries[i]==entry)return (int)i;
    return -1;
}
int gh_r100_live_entry(uint32_t entry){return index_of(entry)>=0;}

static gh_r100_script_status fail(gh_r100_live_task *s,uint32_t pc,gh_r100_script_status status)
{
    s->frontier=pc;
    return status;
}

static gh_r100_script_status find_at(gh_r100_live_task *s,unsigned id,gh_at_native_unit *unit)
{
    if(gh_scene_at_first_unit_002c30c0(s->b.at,unit))
        return fail(s,0x2c30c0,GH_R100_SCRIPT_BOUNDS);
    size_t seen=0,capacity=gh_scene_at_native_unit_count(s->b.at);
    while(unit->bytes){
        if(++seen>capacity||unit->size<0x54)return fail(s,0x2c3118,GH_R100_SCRIPT_BOUNDS);
        if(half(unit->bytes+0x36)==id)break;
        if(gh_scene_at_next_unit_002c30c8(s->b.at,*unit,unit))
            return fail(s,0x2c30c8,GH_R100_SCRIPT_BOUNDS);
    }
    return GH_R100_SCRIPT_OK;
}

static gh_r100_script_status hit(gh_r100_live_task *s,unsigned id,int *original)
{
    gh_at_native_unit unit;
    gh_r100_script_status status=find_at(s,id,&unit);
    if(!status)*original=unit.bytes&&(rd(unit.bytes+0x50)&0x80000000u);
    return status;
}

static float scalar(const void *p){float n;memcpy(&n,p,4);return n;}
static float add(float a,float b){volatile float n=a+b;return n;}
static float mul(float a,float b){volatile float n=a*b;return n;}

static gh_r100_script_status center(gh_r100_live_task *s,unsigned id,float destination[3])
{
    memset(destination,0,12); /* Original caller's VF0 XYZ before the call. */
    if(!s->b.at->headers[0].bytes)return GH_R100_SCRIPT_OK;
    gh_at_native_unit unit;
    gh_r100_script_status status=find_at(s,id,&unit);
    if(status||!unit.bytes||!(unit.bytes[0x34]&1u))return status;
    const uint8_t *shape=unit.bytes+4;
    fenv_t env;
    if(feholdexcept(&env))return fail(s,0x1f89a8,GH_R100_SCRIPT_CALLEE);
    if(fesetround(FE_TOWARDZERO)){(void)fesetenv(&env);return fail(s,0x1f89a8,GH_R100_SCRIPT_CALLEE);}
    if(shape[1]==1||shape[1]==2){
        destination[0]=scalar(shape+0x10);destination[1]=scalar(shape+4);destination[2]=scalar(shape+0x14);
        if(shape[1]==1){
            for(unsigned k=1;k<4;k++){
                destination[0]=add(destination[0],scalar(shape+0x10+k*8));
                destination[2]=add(destination[2],scalar(shape+0x14+k*8));
            }
            destination[0]=mul(destination[0],.25f);destination[2]=mul(destination[2],.25f);
        }
    }
    destination[1]=add(destination[1],.4f);
    if(fesetenv(&env)||!isfinite(destination[0])||!isfinite(destination[1])||!isfinite(destination[2]))
        return fail(s,0x1f89a8,GH_R100_SCRIPT_CALLEE);
    return GH_R100_SCRIPT_OK;
}

static gh_r100_script_status at3_payload(gh_r100_live_task *s)
{
    if(!s->b.enemies||!*s->b.enemies)return fail(s,0x2952d0,GH_R100_SCRIPT_UNSUPPORTED);
    gh_enemy_ems *owner=*s->b.enemies;
    uint8_t *counter=s->b.room_init->work.pointer;
    while(s->at3_stage<2){
        gh_game_title_player *ignored;
        uint32_t frontier=0;
        gh_scene_placement_status status=gh_enemy_ems_set_002952d0(owner,&s->at3_set,
            (uint8_t)(31+s->at3_stage),&ignored,&frontier);
        if(status)return fail(s,frontier?frontier:0x2952d0,
            status==GH_PLACE_UNSUPPORTED?GH_R100_SCRIPT_UNSUPPORTED:GH_R100_SCRIPT_CALLEE);
        wr(counter,rd(counter)+1u); /* Increment after each call, including original null result. */
        s->b.room_init->known_work_bytes|=15u;
        s->at3_set=(gh_enemy_ems_set_call){0};
        ++s->at3_stage;
    }
    while(s->at3_stage<4){
        float destination[3];
        gh_r100_script_status status=center(s,3u+s->at3_stage,destination);
        if(status)return status;
        uint32_t frontier=0;
        gh_actor_motion_status result=gh_enemy_ems_start_goto_at3(owner,
            (uint8_t)(29+s->at3_stage),destination,&frontier);
        if(result)return fail(s,frontier?frontier:0x295ce8,
            result==GH_ACTOR_MOTION_UNSUPPORTED?GH_R100_SCRIPT_UNSUPPORTED:GH_R100_SCRIPT_CALLEE);
        ++s->at3_stage;
    }
    s->finished=1;
    return GH_R100_SCRIPT_OK;
}

static gh_r100_script_status dead(gh_r100_live_task *s,uint8_t stamp,
    const gh_game_title_player **object,int *original)
{
    gh_game_title_player *actor;
    if(gh_enemy_manager_get_00290868(s->b.init,stamp,&actor))
        return fail(s,0x290868,GH_R100_SCRIPT_UNSUPPORTED);
    if(!actor&&rd(s->b.init->enemies+0x500)&&!s->b.init->enemy_top)
        return fail(s,0x295978,GH_R100_SCRIPT_UNSUPPORTED);
    if(object)*object=actor;
    if(!actor){*original=1;return GH_R100_SCRIPT_OK;}
    gh_r100_script_status status=gh_r100_enemy_dead_0028fb38(s->b.room,actor,original);
    return status?fail(s,0x28fb38,status):GH_R100_SCRIPT_OK;
}

static gh_r100_script_status event_ready(gh_r100_live_task *s,int *original)
{
    *original=0;
    if(rd((const uint8_t*)s->b.work+0x5e4)&0x40000000u)return GH_R100_SCRIPT_OK;
    const gh_title_player_view *player=s->b.player;
    if(!player||!player->bytes||player->size<0x15f8)return fail(s,0x12baf0,GH_R100_SCRIPT_UNSUPPORTED);
    *original=(rd(player->bytes+0x15f4)&0x400u)||(!player->bytes[0x2f4]&&player->bytes[0x2f5]<4);
    return GH_R100_SCRIPT_OK;
}

static gh_r100_script_status active(gh_r100_live_task *s,const gh_game_title_player *actor,int *original)
{
    *original=0;
    if(!actor)return GH_R100_SCRIPT_OK;
    const gh_title_player_view *v=&actor->view;
    if(v->original_kind4ac==0x448a00)return GH_R100_SCRIPT_OK; /* cEma2 != cEm00. */
    if(v->original_kind4ac!=0x42c7f0||!v->bytes||v->size<0x16d4)
        return fail(s,0x295f28,GH_R100_SCRIPT_UNSUPPORTED);
    *original=(rd(v->bytes+0x16d0)&2u)!=0;
    return GH_R100_SCRIPT_OK;
}

static gh_r100_script_status watch(gh_r100_live_task *s)
{
    int triggered=0,ready=0,died=0;
    gh_r100_script_status status;
    switch(s->entry){
    case 0x2920:case 0x2650:{
        static const uint8_t stamps[][3]={{19,20,0},{3,4,5}};
        const int second=s->entry==0x2650;
        unsigned deaths=0;
        for(unsigned i=0;i<(second?3u:2u);i++){
            status=dead(s,stamps[second][i],NULL,&died);
            if(status)return status;
            deaths+=(unsigned)died;
        }
        status=hit(s,second?2:1,&triggered);
        if(status||!triggered||deaths>(unsigned)second)return status;
        status=event_ready(s,&ready);
        if(status||!ready)return status;
        if(!second)return fail(s,0x295ab8,GH_R100_SCRIPT_UNSUPPORTED); /* positive EmWrap GetPos. */
        wr(s->b.room->active+4,rd(s->b.room->active+4)|0x10000000u);
        return fail(s,0x2c39b0,GH_R100_SCRIPT_UNSUPPORTED);
    }
    case 0x2490:
        if(!s->at3_stage){
            status=hit(s,3,&triggered);
            if(status||!triggered)return status;
            if((int32_t)rd(s->b.room->active+0x1610)<=4)return GH_R100_SCRIPT_OK;
        }
        return at3_payload(s);
    case 0x2288:case 0x2198:{
        status=event_ready(s,&ready);
        if(status||!ready)return status;
        status=hit(s,s->entry==0x2288?7:8,&triggered);
        if(status)return status;
        if(!triggered){status=hit(s,9,&triggered);if(status)return status;}
        if(!triggered)return GH_R100_SCRIPT_OK;
        if(s->entry==0x2198)return fail(s,0x295ce8,GH_R100_SCRIPT_UNSUPPORTED);
        const gh_game_title_player *a,*b;
        status=dead(s,10,&a,&died);if(status)return status;
        if(died){s->finished=1;return GH_R100_SCRIPT_OK;}
        status=dead(s,11,&b,&died);if(status)return status;
        if(died){s->finished=1;return GH_R100_SCRIPT_OK;}
        status=active(s,a,&ready);if(status)return status;
        if(!ready){status=active(s,b,&ready);if(status)return status;}
        if(ready){s->finished=1;return GH_R100_SCRIPT_OK;}
        return fail(s,0x2c39b0,GH_R100_SCRIPT_UNSUPPORTED);
    }
    case 0x15b0:{
        const gh_game_title_player *a;
        status=dead(s,18,&a,&died);if(status)return status;
        if(!died){status=active(s,a,&ready);if(status)return status;}
        if(died||ready){s->finished=1;return GH_R100_SCRIPT_OK;}
        status=hit(s,20,&triggered);if(status)return status;
        if(triggered){
            if((int32_t)rd((const uint8_t*)s->b.work+0x5ec)<0)
                return fail(s,0x295ce8,GH_R100_SCRIPT_UNSUPPORTED);
            s->finished=1;return GH_R100_SCRIPT_OK;
        }
        status=hit(s,23,&triggered);
        if(status||!triggered)return status;
        return fail(s,0x296130,GH_R100_SCRIPT_UNSUPPORTED);
    }
    case 0x2d68:{
        if(!s->entered)return GH_R100_SCRIPT_ARGUMENT;
        const gh_game_title_player *flagged;
        status=gh_r100_first_flagged_00002cb0(s->b.init,s->b.room,&flagged);
        if(status)return fail(s,0x28fb38,status);
        if(!flagged)return GH_R100_SCRIPT_OK;
        const uint8_t *p=flagged->view.bytes;
        if(p[0x2f4]!=0||p[0x2f5]!=4||p[0x2f6]<2)return GH_R100_SCRIPT_OK;
        s->frontier_relative=1;
        return fail(s,0x2e04,GH_R100_SCRIPT_UNSUPPORTED);
    }
    case 0x1bd0:{
        const uint32_t flags=rd(s->b.room->active+4);
        if(!(flags&0x80000000u)){
            if(!(flags&0x02000000u)||((const uint8_t*)s->b.room_init->work.pointer)[5])return GH_R100_SCRIPT_OK;
            status=event_ready(s,&ready);if(status||!ready)return status;
            status=hit(s,10,&triggered);if(status||!triggered)return status;
            unsigned need=rd((const uint8_t*)s->b.work+0x594)&0x800u?9:12;
            if(rd(s->b.room->active+0x1610)<need)return GH_R100_SCRIPT_OK;
        }
        wr(s->b.room->active+4,flags|0x80000000u);
        if(!s->b.checkpoint)return fail(s,0x2c05b0,GH_R100_SCRIPT_UNSUPPORTED);
        uint32_t pc=0;
        status=s->b.checkpoint(s->b.context,&pc);
        if(status)return fail(s,pc?pc:0x2c05b0,status);
        wr(s->b.room->active+4,rd(s->b.room->active+4)|0x40000000u);
        return fail(s,0x2952d0,GH_R100_SCRIPT_UNSUPPORTED);
    }
    default:return fail(s,s->entry,GH_R100_SCRIPT_UNSUPPORTED);
    }
}

static int sleep_one(gh_r100_live_task *s)
{
    if(!gh_task_threads_sleep(s->b.threads,1))return 1;
    s->status=fail(s,0x2d5aa0,GH_R100_SCRIPT_THREAD);
    return 0;
}

static void run(void *context,int32_t argument)
{
    (void)argument;
    gh_r100_live_task *s=context;
    s->entered=1;
    if(s->entry!=0x1bd0&&!sleep_one(s))return;
    if(s->entry==0x2d68){
        int died;
        s->status=dead(s,8,NULL,&died);
        if(s->status){while(sleep_one(s)){}return;}
        if(died){s->finished=1;return;}
    }
    while(!s->finished){
        if(!s->status)s->status=watch(s);
        if(!s->finished&&!sleep_one(s))return;
    }
}

gh_r100_script_status gh_r100_live_register(gh_r100_live_tasks *owner,
    const gh_r100_live_bindings *b,uint32_t entry,int32_t request,uint32_t *frontier)
{
    if(!frontier)return GH_R100_SCRIPT_ARGUMENT;
    *frontier=0;
    int index=index_of(entry);
    if(!owner||index<0||!b||!b->threads||!b->room||!b->room->active||b->room->size<0x1614||
       !b->room_init||!b->room_init->work.pointer||!b->init||!b->work||!b->at)
        return GH_R100_SCRIPT_ARGUMENT;
    gh_r100_live_task *s=&owner->tasks[index];
    if(s->registered)return GH_R100_SCRIPT_OK;
    s->entry=entry;s->b=*b;
    gh_scenario_exec_status status=gh_scenario_exec_002c3890(b->scenario,b->threads,request,
        run,s,0,&s->slot,frontier);
    if(status)return GH_R100_SCRIPT_THREAD;
    s->registered=1;
    return GH_R100_SCRIPT_OK;
}
