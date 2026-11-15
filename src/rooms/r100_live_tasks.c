#include "r100_live_tasks.h"
#include "r100_watch_geometry.h"
#include "../core/game_enemy_ems.h"
#include <string.h>

static const uint32_t entries[]={0x2920,0x2650,0x2490,0x2288,0x2198,0x15b0,0x2d68,0x1bd0,0x11b0,0x1780};
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

static gh_r100_script_status center(gh_r100_live_task *s,unsigned id,float destination[3])
{
    memset(destination,0,12); /* Original caller's VF0 XYZ before the call. */
    if(!s->b.at->headers[0].bytes)return GH_R100_SCRIPT_OK;
    gh_at_native_unit unit;
    gh_r100_script_status status=find_at(s,id,&unit);
    if(status)return status;
    if(gh_scene_at_center_002c0e78(unit,destination))
        return fail(s,0x1f89a8,GH_R100_SCRIPT_CALLEE);
    return GH_R100_SCRIPT_OK;
}

static gh_r100_script_status set_enemy(gh_r100_live_task *s,gh_enemy_ems *owner,uint8_t stamp,int increment)
{
    gh_game_title_player *ignored;
    uint32_t frontier=0;
    gh_scene_placement_status status=gh_enemy_ems_set_002952d0(owner,&s->set_enemy,stamp,&ignored,&frontier);
    if(status)return fail(s,frontier?frontier:0x2952d0,
        status==GH_PLACE_UNSUPPORTED?GH_R100_SCRIPT_UNSUPPORTED:GH_R100_SCRIPT_CALLEE);
    uint8_t *counter=s->b.room_init->work.pointer;
    if(increment){
        wr(counter,rd(counter)+1u); /* Includes an original null result. */
        s->b.room_init->known_work_bytes|=15u;
    }
    s->set_enemy=(gh_enemy_ems_set_call){0};
    return GH_R100_SCRIPT_OK;
}

static gh_r100_script_status at3_payload(gh_r100_live_task *s)
{
    if(!s->b.enemies||!*s->b.enemies)return fail(s,0x2952d0,GH_R100_SCRIPT_UNSUPPORTED);
    gh_enemy_ems *owner=*s->b.enemies;
    while(s->at3_stage<2){
        gh_r100_script_status status=set_enemy(s,owner,(uint8_t)(31+s->at3_stage),1);
        if(status)return status;
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

static gh_r100_script_status enemy_actor(const gh_game_init_state *init,uint8_t stamp,
    gh_game_title_player **actor,uint32_t *frontier)
{
    if(gh_enemy_manager_get_00290868(init,stamp,actor)){
        *frontier=0x290868;
        return GH_R100_SCRIPT_UNSUPPORTED;
    }
    if(!*actor&&rd(init->enemies+0x500)&&!init->enemy_top){
        *frontier=0x295978;
        return GH_R100_SCRIPT_UNSUPPORTED;
    }
    return GH_R100_SCRIPT_OK;
}

static int actor_position(const gh_title_player_view *actor,float out[4])
{
    uintptr_t base=(uintptr_t)actor->bytes, alias=(uintptr_t)actor->alias_f0;
    if(!base||!alias||alias<base||alias-base>actor->size||16>actor->size-(alias-base))return 0;
    memcpy(out,actor->alias_f0,16);
    return 1;
}

gh_r100_script_status gh_r100_enemy_position_00295ab8(const gh_game_init_state *init,
    uint8_t stamp,const gh_game_math_globals *math,float out[4],uint32_t *frontier)
{
    if(!frontier)return GH_R100_SCRIPT_ARGUMENT;
    *frontier=0;
    if(!init||!out)return GH_R100_SCRIPT_ARGUMENT;
    gh_game_title_player *actor;
    gh_r100_script_status status=enemy_actor(init,stamp,&actor,frontier);
    if(status)return status;
    if(actor){
        if(actor_position(&actor->view,out))return GH_R100_SCRIPT_OK;
    }else if(math&&math->initialized){
        memcpy(out,math->zero_vector,16);
        return GH_R100_SCRIPT_OK;
    }
    *frontier=0x295ab8;
    return GH_R100_SCRIPT_UNSUPPORTED;
}

static gh_r100_script_status dead(gh_r100_live_task *s,uint8_t stamp,
    const gh_game_title_player **object,int *original)
{
    gh_game_title_player *actor;
    uint32_t frontier=0;
    gh_r100_script_status status=enemy_actor(s->b.init,stamp,&actor,&frontier);
    if(status)return fail(s,frontier,status);
    if(object)*object=actor;
    if(!actor){*original=1;return GH_R100_SCRIPT_OK;}
    status=gh_r100_enemy_dead_0028fb38(s->b.room,actor,original);
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

static gh_r100_script_status start_soft_event(gh_r100_live_task *s)
{
    if(!s->b.start_soft_event)return fail(s,0x2c39b0,GH_R100_SCRIPT_UNSUPPORTED);
    uint32_t frontier=0;
    gh_r100_script_status status=s->b.start_soft_event(s->b.context,&s->soft_start,0,&frontier);
    return status?fail(s,frontier?frontier:0x2c39b0,status):GH_R100_SCRIPT_OK;
}

static gh_r100_script_status set_room_camera(gh_r100_live_task *s,const char *name)
{
    if(!s->b.set_room_camera)return fail(s,0x2c3ed8,GH_R100_SCRIPT_UNSUPPORTED);
    uint32_t frontier=0;
    gh_r100_script_status status=s->b.set_room_camera(s->b.context,&s->camera_start,name,&frontier);
    return status?fail(s,frontier?frontier:0x2c3ed8,status):GH_R100_SCRIPT_OK;
}

static gh_r100_script_status program_se(gh_r100_live_task *s,int16_t program)
{
    if(!s->b.program_se)return fail(s,0x2cb8a0,GH_R100_SCRIPT_UNSUPPORTED);
    uint32_t frontier=0;
    gh_r100_script_status status=s->b.program_se(s->b.context,program,&frontier);
    return status?fail(s,frontier?frontier:0x2cb8a0,status):GH_R100_SCRIPT_OK;
}

static int sleep_one(gh_r100_live_task *s);

static gh_r100_script_status wait_camera(gh_r100_live_task *s)
{
    if(!s->b.camera_ended)return fail(s,0x2c4078,GH_R100_SCRIPT_UNSUPPORTED);
    for(;;){
        int ended=0;
        uint32_t frontier=0;
        gh_r100_script_status status=s->b.camera_ended(s->b.context,&ended,&frontier);
        if(status)return fail(s,frontier?frontier:0x2c4040,status);
        if(ended)return GH_R100_SCRIPT_OK;
        if(!sleep_one(s))return s->status;
    }
}

static gh_r100_script_status end_soft_event(gh_r100_live_task *s)
{
    if(!s->b.end_soft_event)return fail(s,0x2c3c10,GH_R100_SCRIPT_UNSUPPORTED);
    uint32_t frontier=0;
    gh_r100_script_status status=s->b.end_soft_event(s->b.context,&s->soft_end,&frontier);
    if(status)return fail(s,frontier?frontier:0x2c3c10,status);
    s->finished=1;
    return GH_R100_SCRIPT_OK;
}

static gh_r100_script_status at1_payload(gh_r100_live_task *s)
{
    float enemy[4],player[4];
    uint32_t frontier=0;
    gh_r100_script_status status=gh_r100_enemy_position_00295ab8(s->b.init,0,s->b.math,enemy,&frontier);
    if(status)return fail(s,frontier?frontier:0x295ab8,status);
    const gh_title_player_view *view=s->b.player;
    /* cPl virtual +84 is 2A9458 (F0 position), and +8C is 2A9460 (+100 angles). */
    if(!view||view->original_vtable214!=0x41d220||!actor_position(view,player))
        return fail(s,0x2a9458,GH_R100_SCRIPT_UNSUPPORTED);
    if(view->size<0x110)return fail(s,0x2a9460,GH_R100_SCRIPT_BOUNDS);
    if(!s->b.elf||!s->b.scene_tables)return fail(s,0x2da2d0,GH_R100_SCRIPT_UNSUPPORTED);
    int payload=0;
    status=gh_r100_at1_geometry(s->b.elf,s->b.elf_size,s->b.scene_tables,
        player,enemy,scalar(view->bytes+0x104),&payload,&frontier);
    if(status)return fail(s,frontier?frontier:0x2da2d0,status);
    if(!payload)return GH_R100_SCRIPT_OK;

    wr(s->b.room->active+4,rd(s->b.room->active+4)|0x20000000u);
    gh_game_title_player *actor;
    status=enemy_actor(s->b.init,0,&actor,&frontier);
    if(status)return fail(s,frontier,status);
    int already_active=0,died=0;
    status=active(s,actor,&already_active);
    if(status)return status;
    if(!already_active){
        status=dead(s,0,NULL,&died);
        if(status)return status;
    }
    if(already_active||died){
        s->finished=1;
        return GH_R100_SCRIPT_OK;
    }
    status=start_soft_event(s);
    if(status)return status;
    status=set_room_camera(s,"A00"); /* REL2B18 */
    if(status)return status;
    status=program_se(s,0x32); /* REL2B3C */
    if(status)return status;
    const gh_enemy_ems *owner=s->b.enemies?*s->b.enemies:NULL;
    gh_actor_motion_status motion=gh_enemy_ems_set_stamp_suspend_00295bc0(owner,s->b.init,0,0,&frontier);
    if(motion)return fail(s,frontier?frontier:0x295bc0,
        motion==GH_ACTOR_MOTION_UNSUPPORTED?GH_R100_SCRIPT_UNSUPPORTED:GH_R100_SCRIPT_CALLEE);
    const uint32_t position_bits[3]={0x40db851f,0x3eb851ec,0x40728f5c};
    const uint32_t yaw_bits=0xbfca3d71;
    float position[3],yaw;
    memcpy(position,position_bits,sizeof(position));
    memcpy(&yaw,&yaw_bits,sizeof(yaw));
    motion=gh_enemy_ems_set_stamp_pose_00295a08(owner,s->b.init,&s->at1_pose,0,position,yaw,&frontier);
    if(motion)return fail(s,frontier?frontier:0x295a08,
        motion==GH_ACTOR_MOTION_UNSUPPORTED?GH_R100_SCRIPT_UNSUPPORTED:GH_R100_SCRIPT_CALLEE);
    float destination[3];
    status=center(s,4,destination); /* REL2BB0 */
    if(status)return status;
    motion=gh_enemy_ems_set_stamp_goto_00295f80(owner,s->b.init,0,destination,9,1,&frontier);
    if(motion)return fail(s,frontier?frontier:0x295f80,
        motion==GH_ACTOR_MOTION_UNSUPPORTED?GH_R100_SCRIPT_UNSUPPORTED:GH_R100_SCRIPT_CALLEE);
    gh_r100_om_view door;
    status=gh_r100_find_om_0014fd98(s->b.pools,s->b.object_view,s->b.context,"dor003",&door,&frontier);
    if(status)return fail(s,frontier?frontier:0x14fd98,status);
    if(door.bytes){
        if(door.size<0x5bc)return fail(s,0x14fd98,GH_R100_SCRIPT_BOUNDS);
        wr(door.bytes+0x5b8,rd(door.bytes+0x5b8)|2u);
    }
    status=wait_camera(s);
    if(status)return status;
    if(door.bytes)wr(door.bytes+0x5b8,rd(door.bytes+0x5b8)&~2u);
    return end_soft_event(s);
}

static gh_r100_script_status at2_payload(gh_r100_live_task *s)
{
    if(!s->b.enemies||!*s->b.enemies)return fail(s,0x2952d0,GH_R100_SCRIPT_UNSUPPORTED);
    gh_enemy_ems *owner=*s->b.enemies;
    for(uint8_t stamp=29;stamp<=30;stamp++){
        gh_r100_script_status status=set_enemy(s,owner,stamp,1); /* REL27C0, REL27E0 */
        if(status)return status;
    }
    float destination[3];
    gh_r100_script_status status=center(s,0x10,destination); /* REL280C */
    if(status)return status;
    uint32_t frontier=0;
    for(uint8_t stamp=29;stamp<=30;stamp++){
        gh_actor_motion_status motion=gh_enemy_ems_start_goto_at2(owner,s->b.init,stamp,destination,&frontier);
        if(motion)return fail(s,frontier?frontier:0x295bc0,
            motion==GH_ACTOR_MOTION_UNSUPPORTED?GH_R100_SCRIPT_UNSUPPORTED:GH_R100_SCRIPT_CALLEE);
    }
    status=wait_camera(s); /* REL2890 */
    if(status)return status;
    for(uint8_t stamp=29;stamp<=30;stamp++){
        if(!s->b.player||!s->b.player->alias_f0)return fail(s,0x2db6b0,GH_R100_SCRIPT_UNSUPPORTED);
        memcpy(destination,s->b.player->alias_f0,sizeof(destination));
        gh_actor_motion_status motion=gh_enemy_ems_set_stamp_goto_00295f80(owner,s->b.init,stamp,destination,2,0,&frontier);
        if(motion)return fail(s,frontier?frontier:0x295f80,
            motion==GH_ACTOR_MOTION_UNSUPPORTED?GH_R100_SCRIPT_UNSUPPORTED:GH_R100_SCRIPT_CALLEE);
    }
    return end_soft_event(s); /* REL28D8 */
}

gh_r100_script_status gh_r100_battle_tail_00001e80(gh_r100_live_task *s)
{
    if(!s||s->entry!=0x1bd0||s->battle_stage!=2||s->battle_spawn_index!=8)
        return GH_R100_SCRIPT_ARGUMENT;
    s->frontier_relative=0;
    while(s->battle_tail_stage<10){
        unsigned gate=s->battle_tail_stage/5, step=s->battle_tail_stage%5;
        uint32_t pc=0;
        gh_r100_script_status status;
        if(step==0){
            if(!s->b.pools||!s->b.object_view){
                s->frontier_relative=1;
                return fail(s,gate?0x1f10:0x1e80,GH_R100_SCRIPT_UNSUPPORTED);
            }
            status=gh_r100_find_om_0014fd98(s->b.pools,s->b.object_view,s->b.context,
                gate?"gate01":"gate00",&s->battle_gate,&pc);
            if(status)return fail(s,pc?pc:0x14fd98,status);
            if(!s->battle_gate.identity){s->battle_tail_stage+=5;continue;}
        }else{
            const unsigned operation=step==2?2:step==4?3:0;
            const int arg0=step==2?(int)gate:step==3?1:0;
            const uint32_t original=operation==2?0x1bcfb0:operation==3?0x1bd228:0x1bd178;
            if(!s->b.battle_door)return fail(s,original,GH_R100_SCRIPT_UNSUPPORTED);
            status=s->b.battle_door(s->b.context,s->battle_gate.identity,operation,arg0,0,&pc);
            if(status)return fail(s,pc?pc:original,status);
        }
        ++s->battle_tail_stage;
    }
    if(s->battle_tail_stage==10){
        uint32_t pc=0;
        if(!s->b.program_se)return fail(s,0x2cb968,GH_R100_SCRIPT_UNSUPPORTED);
        gh_r100_script_status status=s->b.program_se(s->b.context,0x0d,&pc);
        if(status)return fail(s,pc?pc:0x2cb968,status);
        ++s->battle_tail_stage;
    }
    if(s->battle_tail_stage==11){
        uint32_t pc=0;
        if(!s->b.battle_collision)return fail(s,0x1332d0,GH_R100_SCRIPT_UNSUPPORTED);
        if(s->b.battle_collision(s->b.context,"H03_gt",0,&pc))
            return fail(s,pc?pc:0x1332d0,GH_R100_SCRIPT_CALLEE);
        ++s->battle_tail_stage;
    }
    if(s->battle_tail_stage==12){
        gh_r100_script_status status=start_soft_event(s);
        if(status)return status;
        ++s->battle_tail_stage;
    }
    if(s->battle_tail_stage==13){
        if(!s->b.at)return fail(s,0x2c2a48,GH_R100_SCRIPT_UNSUPPORTED);
        gh_r100_script_status status=center(s,0x11,s->battle_destination);
        if(status)return status;
        ++s->battle_tail_stage;
    }
    while(s->battle_tail_stage<17){
        if(!s->b.enemies||!*s->b.enemies)return fail(s,0x295bc0,GH_R100_SCRIPT_UNSUPPORTED);
        uint32_t pc=0;
        gh_actor_motion_status status=gh_enemy_ems_start_goto_battle(*s->b.enemies,s->b.init,
            (uint8_t)(26+s->battle_tail_stage-14),s->battle_destination,&pc);
        if(status)return fail(s,pc?pc:0x295bc0,GH_R100_SCRIPT_UNSUPPORTED);
        ++s->battle_tail_stage;
    }
    if(s->battle_tail_stage==17){
        gh_r100_script_status status=set_room_camera(s,"A01");
        if(status)return status;
        ++s->battle_tail_stage;
    }
    if(s->battle_tail_stage==18){
        gh_r100_script_status status=wait_camera(s);
        if(status)return status;
        ++s->battle_tail_stage;
    }
    if(s->battle_tail_stage==19){
        if(!s->b.end_soft_event)return fail(s,0x2c3c10,GH_R100_SCRIPT_UNSUPPORTED);
        uint32_t pc=0;
        gh_r100_script_status status=s->b.end_soft_event(s->b.context,&s->soft_end,&pc);
        if(status)return fail(s,pc?pc:0x2c3c10,status);
        ++s->battle_tail_stage;
    }
    while(s->battle_tail_stage<24){
        const unsigned gate=(s->battle_tail_stage-20)/2;
        uint32_t pc=0;
        gh_r100_script_status status;
        if(!(s->battle_tail_stage&1)){
            status=gh_r100_find_om_0014fd98(s->b.pools,s->b.object_view,s->b.context,
                gate?"gate01":"gate00",&s->battle_gate,&pc);
            if(status)return fail(s,pc?pc:0x14fd98,status);
        }else if(s->battle_gate.identity){
            if(!s->b.battle_door)return fail(s,0x1bd3f0,GH_R100_SCRIPT_UNSUPPORTED);
            status=s->b.battle_door(s->b.context,s->battle_gate.identity,1,0,0,&pc);
            if(status)return fail(s,pc?pc:0x1bd3f0,status);
        }
        ++s->battle_tail_stage;
    }
    if(s->battle_tail_stage==24){
        uint32_t pc=0;
        gh_r100_script_status status=s->b.battle_register?
            s->b.battle_register(s->b.context,0x1780,-1,&pc):GH_R100_SCRIPT_UNSUPPORTED;
        if(status){
            s->frontier_relative=!pc;
            return fail(s,pc?pc:0x1780,status);
        }
        ++s->battle_tail_stage;
    }
    s->finished=1;
    return GH_R100_SCRIPT_OK;
}

static gh_r100_script_status successor_escape(gh_r100_live_task *s)
{
    uint32_t pc=0;
    gh_game_title_player *heavy=NULL;
    gh_r100_script_status status=enemy_actor(s->b.init,14,&heavy,&pc);
    if(status)return fail(s,pc,status);
    if(heavy&&heavy->view.original_kind4ac==0x448a00){
        const gh_title_player_view *view=&heavy->view;
        if(!view->bytes||view->size<0x15b4||view->original_vtable214!=0x448ca0)
            return fail(s,0x289028,GH_R100_SCRIPT_UNSUPPORTED);
        wr(view->bytes+0x15b0,rd(view->bytes+0x15b0)|8u);
    }
    return GH_R100_SCRIPT_OK;
}

static gh_r100_script_status successor_after_sound(gh_r100_live_task *s)
{
    enum { ACTION13, RESUME13, RESUME14, SLEEP60, START_ESCAPE, WAIT_A05,
           SUSPEND14, SUSPEND13, CAMERA_A06, SOUND36,
           SET_23, SET_24, SET_25, GOTO_23, GOTO_24, GOTO_25, WAIT_A06, END_SOFT, PLAYER_POSE };
    gh_r100_script_status status;
    uint32_t pc=0;
    if(!s->b.enemies||!*s->b.enemies)return fail(s,0x295ce8,GH_R100_SCRIPT_UNSUPPORTED);
    gh_enemy_ems *owner=*s->b.enemies;
    if(!s->successor_tail_stage&&s->successor_short_path)s->successor_tail_stage=SET_23;
    for(;s->successor_tail_stage<=PLAYER_POSE;++s->successor_tail_stage){
        const unsigned stage=s->successor_tail_stage;
        switch(stage){
        case ACTION13:
            if(gh_enemy_ems_start_stamp_action_00295ce8(owner,s->b.init,13,&pc))
                return fail(s,pc?pc:0x295ce8,GH_R100_SCRIPT_UNSUPPORTED);
            break;
        case RESUME13: case RESUME14: case SUSPEND14: case SUSPEND13:
            if(gh_enemy_ems_set_stamp_suspend_00295bc0(owner,s->b.init,
                (stage==RESUME13||stage==SUSPEND13)?13:14,stage>=SUSPEND14,&pc))
                return fail(s,pc?pc:0x295bc0,GH_R100_SCRIPT_UNSUPPORTED);
            break;
        case SLEEP60:
            if(gh_task_threads_sleep(s->b.threads,60))return fail(s,0x2d5aa0,GH_R100_SCRIPT_THREAD);
            break;
        case START_ESCAPE:
            status=successor_escape(s);
            if(status)return status;
            break;
        case WAIT_A05: case WAIT_A06:
            status=wait_camera(s);
            if(status)return status;
            if(stage==WAIT_A05)s->camera_start=(gh_room_camera_start){0};
            break;
        case CAMERA_A06:
            status=set_room_camera(s,"A06");
            if(status)return status;
            break;
        case SOUND36:
            status=program_se(s,0x36);
            if(status)return status;
            break;
        case SET_23: case SET_24: case SET_25:
            status=set_enemy(s,owner,(uint8_t)(23+stage-SET_23),1);
            if(status)return status;
            break;
        case GOTO_23: case GOTO_24: case GOTO_25:{
            const unsigned index=stage-GOTO_23;
            static const uint8_t ids[]={0x0c,0x0e,0x0d};
            float destination[3];
            status=center(s,ids[index],destination);
            if(status)return status;
            if(gh_enemy_ems_start_goto_successor(owner,(uint8_t)(23+index),destination,&pc))
                return fail(s,pc?pc:0x295f80,GH_R100_SCRIPT_UNSUPPORTED);
            break;
        }
        case END_SOFT:
            if(!s->b.end_soft_event)return fail(s,0x2c3c10,GH_R100_SCRIPT_UNSUPPORTED);
            status=s->b.end_soft_event(s->b.context,&s->soft_end,&pc);
            if(status)return fail(s,pc?pc:0x2c3c10,status);
            break;
        case PLAYER_POSE:{
            if(!s->b.battle_player_pose)return fail(s,0x12ce60,GH_R100_SCRIPT_UNSUPPORTED);
            const uint32_t xyz[3]={0x40370a3d,0,0xc27bae14};
            const uint32_t euler[3]={0,0xc0400000,0};
            status=s->b.battle_player_pose(s->b.context,xyz,euler,&pc);
            if(status)return fail(s,pc?pc:0x12ce60,status);
            break;
        }
        }
    }
    s->finished=1;
    return GH_R100_SCRIPT_OK;
}

static gh_r100_script_status battle_successor_00001780(gh_r100_live_task *s)
{
    enum { SET_ESCAPE, WAIT_FOR_AT11, WAIT_FOR_EVENT, START_SOFT, SELECT_PATH,
           START_ESCAPE, START_CAMERA, SOUND, NEXT_CALL };
    gh_r100_script_status status;
    uint32_t pc=0;
    gh_game_title_player *heavy=NULL;
    if(s->successor_stage==SET_ESCAPE){
        status=enemy_actor(s->b.init,14,&heavy,&pc);
        if(status)return fail(s,pc,status);
        if(heavy&&heavy->view.original_kind4ac==0x448a00){
            const uint32_t bits[4]={0x416051ec,0x3eeb851f,0xc260a3d7,0x3f800000};
            float position[4];
            memcpy(position,bits,sizeof(position));
            if(gh_enemy2a2_set_escape_position_00289038(heavy,position))
                return fail(s,0x289038,GH_R100_SCRIPT_UNSUPPORTED);
        }
        ++s->successor_stage;
    }
    if(s->successor_stage==WAIT_FOR_AT11){
        int triggered;
        status=hit(s,0xb,&triggered);
        if(status)return status;
        if(!triggered){
            gh_game_title_player *enemy=NULL;
            status=enemy_actor(s->b.init,13,&enemy,&pc);
            if(status)return fail(s,pc,status);
            int is_active;
            status=active(s,enemy,&is_active);
            if(status)return status;
            if(is_active)s->successor_already_active=1;
            return GH_R100_SCRIPT_OK;
        }
        ++s->successor_stage;
    }
    if(s->successor_stage==WAIT_FOR_EVENT){
        int ready;
        status=event_ready(s,&ready);
        if(status||!ready)return status;
        wr(s->b.room->active+4,rd(s->b.room->active+4)|0x08000000u);
        ++s->successor_stage;
    }
    if(s->successor_stage==START_SOFT){
        status=start_soft_event(s);
        if(status)return status;
        ++s->successor_stage;
    }
    if(s->successor_stage==SELECT_PATH){
        int died=0;
        if(!s->successor_already_active){
            status=dead(s,13,NULL,&died);
            if(status)return status;
        }
        s->successor_short_path=(uint8_t)(died||s->successor_already_active);
        s->successor_stage=s->successor_short_path?START_ESCAPE:START_CAMERA;
    }
    if(s->successor_stage==START_ESCAPE){
        status=successor_escape(s);
        if(status)return status;
        ++s->successor_stage;
    }
    if(s->successor_stage==START_CAMERA){
        status=set_room_camera(s,s->successor_short_path?"A06":"A05");
        if(status)return status;
        ++s->successor_stage;
    }
    if(s->successor_stage==SOUND){
        status=program_se(s,s->successor_short_path?0x36:0x35);
        if(status)return status;
        ++s->successor_stage;
    }
    return successor_after_sound(s);
}

static gh_r100_script_status watch(gh_r100_live_task *s)
{
    int triggered=0,ready=0,died=0;
    gh_r100_script_status status;
    switch(s->entry){
    case 0x2920:case 0x2650:{
        static const uint8_t stamps[][3]={{19,20,0},{3,4,5}};
        const int second=s->entry==0x2650;
        unsigned alive=0;
        for(unsigned i=0;i<(second?3u:2u);i++){
            status=dead(s,stamps[second][i],NULL,&died);
            if(status)return status;
            alive+=!died;
        }
        status=hit(s,second?2:1,&triggered);
        /* REL2920 requires zero survivors, while REL2650 admits one. */
        if(status||!triggered||alive>(unsigned)second)return status;
        status=event_ready(s,&ready);
        if(status||!ready)return status;
        if(!second)return at1_payload(s);
        wr(s->b.room->active+4,rd(s->b.room->active+4)|0x10000000u);
        status=start_soft_event(s);
        if(status)return status;
        status=program_se(s,0x33); /* REL2794 */
        if(status)return status;
        status=set_room_camera(s,"A03"); /* REL27A4 */
        if(status)return status;
        return at2_payload(s);
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
        if(!s->devil_event_stage){
            const gh_game_title_player *flagged;
            status=gh_r100_first_flagged_00002cb0(s->b.init,s->b.room,&flagged);
            if(status)return fail(s,0x28fb38,status);
            if(!flagged)return GH_R100_SCRIPT_OK;
            const uint8_t *p=flagged->view.bytes;
            /* Original imported CkDevilEvent at 273640. */
            if(p[0x2f4]!=0||p[0x2f5]!=4||p[0x2f6]<2)return GH_R100_SCRIPT_OK;
            wr(s->b.room->active+4,rd(s->b.room->active+4)|0x02000000u);
            uint8_t *flags=(uint8_t*)s->b.work+0x5ec;
            wr(flags,rd(flags)|0x20000000u);
            s->devil_event_stage=1;
        }
        if(s->devil_event_stage==1){
            if(rd((const uint8_t*)s->b.work+0x5ec)&0x40000000u)return GH_R100_SCRIPT_OK;
            status=start_soft_event(s);
            if(status)return status;
            s->devil_event_stage=2;
        }
        if(s->devil_event_stage==2){
            const gh_enemy_ems *owner=s->b.enemies?*s->b.enemies:NULL;
            uint32_t frontier=0;
            gh_actor_motion_status result=gh_enemy_ems_set_stamp_suspend_00295bc0(owner,s->b.init,8,0,&frontier);
            if(result)return fail(s,frontier?frontier:0x295bc0,GH_R100_SCRIPT_CALLEE);
            s->devil_event_stage=3;
        }
        const gh_game_title_player *flagged;
        status=gh_r100_first_flagged_00002cb0(s->b.init,s->b.room,&flagged);
        if(status)return fail(s,0x28fb38,status);
        if(s->devil_event_stage==3){
            if(flagged){
                if(flagged->view.original_vtable214!=0x4479c8){
                    s->frontier_relative=1;
                    return fail(s,0x2e94,GH_R100_SCRIPT_UNSUPPORTED);
                }
                /* The authentic cEm00 VT+64 target is 275308, the same
                   leaf reached by the typed EmWrap suspend operation. */
                const gh_enemy_ems *owner=s->b.enemies?*s->b.enemies:NULL;
                uint32_t frontier=0;
                gh_actor_motion_status result=gh_enemy_ems_set_actor_suspend_00275308(
                    owner,s->b.init,flagged,0,&frontier);
                if(result)return fail(s,frontier?frontier:0x275308,GH_R100_SCRIPT_CALLEE);
            }
            s->devil_event_stage=4;
        }
        if(flagged){
            const uint8_t *p=flagged->view.bytes;
            if(p[0x2f4]==0&&p[0x2f5]==4&&p[0x2f6]>=2)return GH_R100_SCRIPT_OK;
        }
        if(!s->b.end_soft_event)return fail(s,0x2c3c10,GH_R100_SCRIPT_UNSUPPORTED);
        uint32_t frontier=0;
        status=s->b.end_soft_event(s->b.context,&s->soft_end,&frontier);
        if(status)return fail(s,frontier?frontier:0x2c3c10,status);
        uint8_t *flags=(uint8_t*)s->b.work+0x5ec;
        wr(flags,rd(flags)&~0x20000000u);
        status=gh_r100_live_register(s->owner,&s->b,0x11b0,-1,&frontier);
        if(status)return fail(s,frontier?frontier:0x2c3890,status);
        s->finished=1;
        return GH_R100_SCRIPT_OK;
    }
    case 0x11b0:{
        if(!s->post_devil_stage){
            status=hit(s,22,&triggered);
            if(status||!triggered||((const uint8_t*)s->b.room_init->work.pointer)[5])return status;
            for(uint8_t stamp=10;stamp<=11;stamp++){
                status=dead(s,stamp,NULL,&died);
                if(status||!died)return status;
            }
            status=event_ready(s,&ready);
            if(status||!ready)return status;
            wr(s->b.room->active+4,rd(s->b.room->active+4)|0x00800000u);
            s->post_devil_stage=1;
        }
        if(s->post_devil_stage==1){
            status=start_soft_event(s);
            if(status)return status;
            s->post_devil_stage=2;
        }
        if(s->post_devil_stage==2){
            status=set_room_camera(s,"A07");
            if(status)return status;
            s->post_devil_stage=3;
        }
        if(!s->b.enemies||!*s->b.enemies)return fail(s,0x2952d0,GH_R100_SCRIPT_UNSUPPORTED);
        while(s->post_devil_stage<=4){
            status=set_enemy(s,*s->b.enemies,(uint8_t)(33+s->post_devil_stage-3),1);
            if(status)return status;
            ++s->post_devil_stage;
        }
        s->frontier_relative=1;
        return fail(s,0x1318,GH_R100_SCRIPT_UNSUPPORTED); /* next original EmWrap action */
    }
    case 0x1780:return battle_successor_00001780(s);
    case 0x1bd0:{
        if(!s->battle_stage){
            const uint32_t flags=rd(s->b.room->active+4);
            if(!(flags&0x80000000u)){
                if(!(flags&0x02000000u)||((const uint8_t*)s->b.room_init->work.pointer)[5])return GH_R100_SCRIPT_OK;
                status=event_ready(s,&ready);if(status||!ready)return status;
                status=hit(s,10,&triggered);if(status||!triggered)return status;
                unsigned need=rd((const uint8_t*)s->b.work+0x594)&0x800u?9:12;
                if(rd(s->b.room->active+0x1610)<need)return GH_R100_SCRIPT_OK;
            }
            wr(s->b.room->active+4,flags|0x80000000u);
            s->battle_stage=1;
        }
        if(s->battle_stage==1){
            if(!s->b.checkpoint)return fail(s,0x2c05b0,GH_R100_SCRIPT_UNSUPPORTED);
            uint32_t pc=0;
            status=s->b.checkpoint(s->b.context,&pc);
            if(status)return fail(s,pc?pc:0x2c05b0,status);
            wr(s->b.room->active+4,rd(s->b.room->active+4)|0x40000000u);
            s->battle_stage=2;
        }
        static const uint8_t stamps[]={26,27,28,2,13,16,17,14};
        if(!s->b.enemies||!*s->b.enemies)return fail(s,0x2952d0,GH_R100_SCRIPT_UNSUPPORTED);
        while(s->battle_spawn_index<sizeof(stamps)){
            status=set_enemy(s,*s->b.enemies,stamps[s->battle_spawn_index],
                s->battle_spawn_index+1u<sizeof(stamps));
            if(status)return status;
            ++s->battle_spawn_index;
        }
        return gh_r100_battle_tail_00001e80(s);
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
    s->entry=entry;s->b=*b;s->owner=owner;
    gh_scenario_exec_status status=gh_scenario_exec_002c3890(b->scenario,b->threads,request,
        run,s,0,&s->slot,frontier);
    if(status)return GH_R100_SCRIPT_THREAD;
    s->registered=1;
    return GH_R100_SCRIPT_OK;
}
