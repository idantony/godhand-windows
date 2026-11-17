#include "game_enemy_retire.h"
#include "game_enemy_2ad.h"
#include "game_enemy_row2.h"
#include "game_hitarea_internal.h"
#include <string.h>

static uint32_t word(const uint8_t *bytes)
{
    uint32_t result;
    memcpy(&result,bytes,sizeof(result));
    return result;
}

static void put(uint8_t *bytes,uint32_t value)
{
    memcpy(bytes,&value,sizeof(value));
}

static gh_actor_motion_status stop(uint32_t *frontier,uint32_t address)
{
    *frontier=address;
    return GH_ACTOR_MOTION_UNSUPPORTED;
}

static gh_actor_motion_status mark_202(gh_enemy_death_mark *state,
    gh_enemy_ems *owner,gh_enemy_ems_live *entry,uint32_t *frontier)
{
    gh_enemy_actor *enemy=entry->enemy202;
    gh_enemy_hitareas *hitareas=entry->devil?entry->devil_hitareas:&enemy->hitareas;
    gh_enemy_solids *solids=entry->devil?entry->devil_solids:&enemy->solids;
    uint8_t *bytes=entry->actor->view.bytes;
    if(!hitareas||!solids)return stop(frontier,0x208858);
    /* 208260 returns directly. 2714B0 checks its auxiliary allocation. */
    if(word(bytes+0x1860))return stop(frontier,0x30f5c8);
    if(enemy&&enemy->tail.spring_records.pointer){
        if(gh_enemy_entry_tail_abort(&enemy->tail))return stop(frontier,0x30f5c8);
        if(entry->move202)entry->move202->spring_frame=(gh_cloth_frame){0};
    }
    static const uint16_t slots[]={0x698,0x670,0x678,0x680,0x688,0x690};
    for(;state->hitarea_cursor<6;++state->hitarea_cursor){
        unsigned i=state->hitarea_cursor;
        gh_hitarea_object *unit=&hitareas->units[i];
        const uint32_t address=i?0x1fe148:0x1fe278;
        if(word(bytes+slots[i]))return stop(frontier,address);
        if(!hitareas->created[i])continue;
        if(unit->fallback||unit->group!=(i?1:2))return stop(frontier,address);
        int original;
        if(gh_hitarea_object_clear_shapes_001fd580(unit,&original,frontier)||
           gh_hitarea_object_release_empty_001fe278(unit,frontier))
            return stop(frontier,*frontier?*frontier:address);
        if(i)put(bytes+slots[i]+4,UINT32_MAX);
        else for(unsigned j=0;j<5;++j)put(bytes+0x69c+j*4,UINT32_MAX);
        hitareas->created[i]=0;
    }

    /* These are original attachment offsets. +6EC additionally has a typed
       owner, because the native constructor does not store guest pointers. */
    static const uint16_t mark_attached[]={0x6ec,0x6f0,0x6f8,0x6fc,0x700,
        0x704,0x708,0x734,0x6f4,0x70c,0x730,0x738};
    if(enemy&&gh_enemy_timers_weapon_kind(&enemy->timers)){
        gh_actor_motion_status status=gh_enemy_ems_mark_held_weapon_00312708(
            owner,state->handle,state->voices,frontier);
        if(status)return status;
    }
    for(unsigned i=0;i<sizeof(mark_attached)/sizeof(*mark_attached);++i)
        if(word(bytes+mark_attached[i]))return stop(frontier,0x1b79b0);
    static const uint16_t mark_actor[]={0x1964,0x1968,0x196c,0x1970,0x1974,
        0x73c,0x740,0x744,0x748,0x74c,0x750,0x754,0x758};
    for(unsigned i=0;i<sizeof(mark_actor)/sizeof(*mark_actor);++i)
        if(word(bytes+mark_actor[i]))return stop(frontier,0x312708);
    if(gh_collision_solid_release_unit_001347e8(solids->manager,
        &entry->actor->view,frontier))return stop(frontier,*frontier?*frontier:0x1347e8);
    solids->unit=NULL;
    solids->sphere=NULL;
    return GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status mark_2a2(gh_enemy_death_mark *state,
    gh_enemy_ems_live *entry,uint32_t *frontier)
{
    gh_enemy2a2_pose *pose=&entry->enemy2a2->pose;
    if(state->stage==1){
        if(gh_collision_solid_release_unit_001347e8(pose->bindings.solids,
            &entry->actor->view,frontier))return stop(frontier,*frontier?*frontier:0x1347e8);
        pose->unit=NULL;
        pose->sphere=NULL;
        state->stage=2;
    }
    if(word(entry->actor->view.bytes+0x15ec))return stop(frontier,0x1b79b0);
    return GH_ACTOR_MOTION_OK;
}

static int devil_220(const gh_enemy_ems_live *entry,uint32_t kind)
{
    const gh_title_player_view *view=&entry->actor->view;
    return kind==0x220&&entry->devil&&entry->devil_motion&&entry->devil_hitareas&&
        entry->devil_solids&&entry->init&&view->size>=0x1978&&
        view->original_vtable214==0x4479c8&&word(view->bytes+0x564)==0x220;
}

static int r103_204(const gh_enemy_ems_live *entry,uint32_t kind)
{
    const gh_enemy_actor *enemy=entry->enemy202;
    const gh_title_player_view *view=&entry->actor->view;
    return kind==0x204&&enemy&&view->size>=0x1978&&view->original_vtable214==0x4479c8&&
        gh_enemy204_is_r103_entry(&enemy->entry.request,enemy->entry.b.work->room);
}

static int r104_203(const gh_enemy_ems_live *entry,uint32_t kind)
{
    const gh_enemy_actor *enemy=entry->enemy202;
    const gh_title_player_view *view=&entry->actor->view;
    return kind==0x203&&enemy&&view->size>=0x1978&&view->original_vtable214==0x4479c8&&
        gh_enemy203_is_r104_entry(&enemy->entry.request,enemy->entry.b.work->room);
}

static int scenery_2ad(const gh_enemy_ems_live *entry,uint32_t kind)
{
    const gh_title_player_view *view=&entry->actor->view;
    return kind==0x2ad&&entry->enemy2ad&&entry->enemy2ad->stage==7&&
        view->size==GH_EM2AD_BYTES&&view->original_vtable214==0x44a528;
}

gh_actor_motion_status gh_enemy_death_mark_00312708(gh_enemy_death_mark *state,
    gh_enemy_ems *owner,uint32_t handle,gh_se_voice_engine *voices,uint32_t *frontier)
{
    if(!frontier)return GH_ACTOR_MOTION_ARGUMENT;
    *frontier=0;
    if(!state||!owner||!handle||!voices||
       (state->stage&&(state->owner!=owner||state->handle!=handle||state->voices!=voices)))
        return GH_ACTOR_MOTION_ARGUMENT;
    gh_enemy_ems_live entry;
    if(gh_enemy_ems_live_at(owner,handle,&entry)||!entry.actor||
       !entry.model||entry.model->root.bytes!=entry.actor->view.bytes)
        return stop(frontier,0x312708);
    const gh_title_player_view *view=&entry.actor->view;
    if(!view->bytes||view->size<0x300)return stop(frontier,0x312708);
    const uint32_t flags=word(view->bytes+0x250);
    if(flags&0x80000000u)return GH_ACTOR_MOTION_OK;
    const uint32_t vtable=word(view->bytes+0x214);
    const unsigned kind=(unsigned)view->bytes[0x2fe]|(unsigned)view->bytes[0x2ff]<<8;
    const int em202=(kind==0x202&&vtable==0x4479c8&&entry.enemy202&&view->size>=0x1978)||
        devil_220(&entry,kind)||r103_204(&entry,kind)||r104_203(&entry,kind);
    const int r200=kind==0x2a3&&entry.enemy2a2&&entry.enemy2a2->prefix.bindings.work->room==0x200&&
        gh_enemy2a2_is_r200_entry(&entry.enemy2a2->prefix.request);
    const int em2a2=(kind==0x2a2||r200)&&vtable==0x448ca0&&entry.enemy2a2&&view->size>=0x15f0;
    const int em2ad=scenery_2ad(&entry,kind);
    if(view->original_vtable214!=vtable||(!em202&&!em2a2&&!em2ad))return stop(frontier,0x312748);
    if(!state->stage){
        if(gh_se_voice_detach_actor_002ce488(voices,entry.model)){
            const uint32_t failure=gh_se_voice_frontier(voices);
            return stop(frontier,failure?failure:0x2ce488);
        }
        state->owner=owner;
        state->voices=voices;
        state->handle=handle;
        state->stage=1;
    }
    /* 44A528's +4C target 28E418 returns directly. */
    gh_actor_motion_status result=em202?mark_202(state,owner,&entry,frontier):
        em2a2?mark_2a2(state,&entry,frontier):GH_ACTOR_MOTION_OK;
    if(result)return result;
    put(view->bytes+0x250,word(view->bytes+0x250)|0x80000002u);
    state->stage=3;
    return GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status release_actor_hitareas(gh_enemy_destructor *state,
    gh_game_init_state *init,gh_title_player_view *actor,uint32_t *frontier)
{
    if(!init||!init->hitarea_native||init->hitarea_native->owner!=init)
        return stop(frontier,0x1fe370);
    for(;state->hitarea_cursor<416;++state->hitarea_cursor){
        const unsigned i=state->hitarea_cursor;
        if(init->hitarea_native->actors[i]!=actor)continue;
        const unsigned group=i<160?1:2;
        uint8_t *bytes=group==1?init->object_pool+0x60*i:
            init->object_pool+0x3c00+0x70*(i-160);
        if(!bytes[0x4a])continue;
        gh_hitarea_object unit={init,bytes,group==1?0x60:0x70,
            init->hitarea_generation,(uint16_t)i,(uint8_t)group,0,init->hitarea_native->serials[i]};
        int original;
        if(gh_hitarea_object_clear_shapes_001fd580(&unit,&original,frontier)||
           gh_hitarea_object_release_empty_001fe278(&unit,frontier))
            return stop(frontier,*frontier?*frontier:(group==1?0x1fe148:0x1fe278));
    }
    return GH_ACTOR_MOTION_OK;
}

gh_actor_motion_status gh_enemy_destroy_prefix_mode3(gh_enemy_destructor *state,gh_enemy_ems *owner,
    uint32_t handle,gh_motion_effect_consumer effect,void *effect_context,
    gh_se_voice_engine *voices,uint32_t *frontier)
{
    if(!frontier)return GH_ACTOR_MOTION_ARGUMENT;
    *frontier=0;
    if(!state||!owner||!handle||(state->stage&&(state->owner!=owner||state->handle!=handle||
       state->effect!=effect||state->effect_context!=effect_context)))return GH_ACTOR_MOTION_ARGUMENT;
    gh_enemy_ems_live entry;
    if(gh_enemy_ems_live_at(owner,handle,&entry)||!entry.actor||!entry.model||
       entry.model->root.bytes!=entry.actor->view.bytes)return stop(frontier,0x30fa00);
    gh_title_player_view *view=&entry.actor->view;
    uint8_t *bytes=view->bytes;
    if(!bytes||view->size<0x300||(word(bytes+0x250)&0xa0000000u)!=0xa0000000u)
        return GH_ACTOR_MOTION_ARGUMENT;
    const uint32_t kind=word(bytes+0x2fc)>>16;
    const int devil=devil_220(&entry,kind);
    const int r103=r103_204(&entry,kind);
    const int r104=r104_203(&entry,kind);
    const int standard=(kind==0x202&&entry.enemy202&&view->size>=0x1978&&view->original_vtable214==0x4479c8)||
        devil||r103||r104;
    const int r200_ema3=kind==0x2a3&&entry.enemy2a2&&entry.enemy2a2->prefix.bindings.work->room==0x200&&
        gh_enemy2a2_is_r200_entry(&entry.enemy2a2->prefix.request);
    const int heavy=(kind==0x2a2||r200_ema3)&&entry.enemy2a2&&view->size>=0x15f0&&view->original_vtable214==0x448ca0;
    const int scenery=scenery_2ad(&entry,kind);
    if(!standard&&!heavy&&!scenery)return stop(frontier,0x30fa00);
    if(!state->stage){
        const uint32_t vtable=view->original_vtable214;
        if(word(bytes+0x214)!=vtable)return stop(frontier,0x30fa00);
        gh_actor_motion *motion=devil?entry.devil_motion:standard?&entry.enemy202->motion.motion:
            heavy?&entry.enemy2a2->prefix.motion:&entry.enemy2ad->motion;
        gh_actor_motion_status status=gh_actor_motion_call_open_release_002a84a8(
            &state->motion,motion,effect,effect_context);
        if(status){*frontier=0x2a84a8;return status;}
        state->owner=owner;state->handle=handle;state->effect=effect;state->effect_context=effect_context;
        state->entry=standard?0x202058:heavy?0x284a90:0x28e068;
        put(bytes+0x214,vtable);
        state->stage=1;
    }
    if(state->stage==1){
        gh_actor_motion_status status=gh_actor_motion_call_release_step_002a84a8(&state->motion);
        if(status){*frontier=state->motion.frontier?state->motion.frontier:0x2a84a8;return status;}
        state->stage=2;
    }
    if(state->stage==2){
        if(standard){
            const uint32_t dependency=word(bytes+0x564);
            /* R102 stamp18 EM0F's 42C81C entry also jumps straight to 2022A0. */
            const int r102_runner=dependency==0x20f&&entry.model->work&&
                entry.model->work->room==0x102&&(word(bytes+0x640)&0xffffu)==0x12&&
                (bytes[0x1650]==0||bytes[0x1650]==4);
            /* 0200/0202/0203 lie below the switch; 020A/0227 jump to 2022A0.
             * R104's kind0203 also branches directly there at 202094. */
            if(r103&&(dependency==0x214||dependency==0x215)){
                if(gh_enemy204_release_effect_bank_00305778(&entry.enemy202->entry.initialize))
                    return stop(frontier,0x305778);
            }else if(dependency!=0x200&&dependency!=0x202&&dependency!=0x203&&
               dependency!=0x20a&&dependency!=0x227&&!(r104&&dependency==0x250)&&
               !r102_runner&&!devil)return stop(frontier,0x202098);
        }else if(heavy){
            if(word(bytes+0x15c4))return stop(frontier,0x1c2370);
        }else{
            gh_actor_motion_status status=gh_enemy_ems_2ad_dtor_bank_00305778(owner,handle,frontier);
            if(status)return status;
            if(word(bytes+0x1550)&&
               (!voices||gh_se_voice_stop_actor_002cbba0(voices,word(bytes+0x1550))))
                return stop(frontier,0x2cbba0);
        }
        state->virtual_mark.stage=1;
        state->stage=3;
    }
    if(state->stage==3){
        gh_actor_motion_status status=standard?mark_202(&state->virtual_mark,owner,&entry,frontier):
            heavy?mark_2a2(&state->virtual_mark,&entry,frontier):GH_ACTOR_MOTION_OK;
        if(status)return status;
        put(bytes+0x214,0x44a660);
        state->stage=4;
    }
    if(state->stage==4){
        gh_game_init_state *init=devil?entry.init:standard?entry.enemy202->b.entry.init:
            heavy?entry.enemy2a2->pose.bindings.init:entry.init;
        gh_actor_motion_status status=release_actor_hitareas(state,init,view,frontier);
        if(status)return status;
        if(heavy)entry.enemy2a2->pose.hitarea_created=0;
        state->stage=5;
    }
    if(state->stage==5){
        gh_game_collision_solids *solids=devil?entry.devil_solids->manager:standard?entry.enemy202->solids.manager:
            heavy?entry.enemy2a2->pose.bindings.solids:entry.solids;
        if(gh_collision_solid_release_unit_001347e8(solids,view,frontier))
            return stop(frontier,*frontier?*frontier:0x1347e8);
        if(r103){
            gh_game_init_state *manager=entry.enemy202->b.entry.init;
            for(unsigned i=0;i<2;++i)
                if(manager->enemy_r103_boss[i]==entry.actor)
                    manager->enemy_r103_boss[i]=NULL;
        }
        put(bytes+0x214,0x44ae28);
        put(bytes+0x214,0x41ebe8);
        put(bytes+0x214,0x41eaf0);
        state->stage=6;
    }
    return stop(frontier,0x1fe9b8);
}
