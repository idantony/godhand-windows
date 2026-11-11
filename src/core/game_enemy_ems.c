#include "game_effect_create.h"
#include "game_actor_external_parent.h"
#include "game_enemy_retire.h"
#include "game_enemy_ems.h"
#include "game_enemy_row2.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

enum { EMS_ROWS=64, EMS_ROW_BYTES=24, EMS_ROWS_OFFSET=12 };
typedef struct {
    gh_enemy_entry entry;
    gh_enemy2a2_virtual_init init;
} ems_2a2;

typedef struct {
    gh_enemy_actor *enemy202;
    gh_enemy202_move move202;
    gh_enemy_other_move other_move;
    gh_enemy_timers retiring_weapon;
    ems_2a2 *enemy2a2;
    gh_game_title_player *actor;
    uint32_t handle;
    const void *registry_owner;
    uint8_t entered;
} ems_entry;

struct gh_enemy_ems {
    gh_em_resource_prefix *prefix;
    const gh_room_state *room;
    const gh_enemy_ems_bindings *bindings;
    const uint8_t *page;
    ems_entry entries[EMS_ROWS];
    uint8_t running, closing;
};

static gh_scene_placement_status motion_error(gh_actor_motion_status status)
{
    switch(status) {
    case GH_ACTOR_MOTION_ARGUMENT: return GH_PLACE_ARGUMENT;
    case GH_ACTOR_MOTION_BUSY: return GH_PLACE_BUSY;
    case GH_ACTOR_MOTION_BOUNDS: return GH_PLACE_BOUNDS;
    default: return GH_PLACE_UNSUPPORTED;
    }
}

static int valid_page(const gh_room_state *room)
{
    return room->size>=EMS_ROWS_OFFSET && room->active[8]<=EMS_ROWS &&
        room->size>=EMS_ROWS_OFFSET+(size_t)room->active[8]*EMS_ROW_BYTES;
}

static int actor_registered(const gh_enemy_ems *owner,const ems_entry *entry)
{
    void *object=NULL;
    return entry->actor && entry->handle &&
        !gh_room_object_lookup(owner->bindings->actor.entry.pools,entry->handle,
            GH_ROOM_OBJECT_ENEMY,entry->registry_owner,&object) &&
        object==entry->actor;
}

static gh_actor_motion_status model_error(gh_actor_model_status status)
{
    switch(status){
    case GH_ACTOR_MODEL_OK: return GH_ACTOR_MOTION_OK;
    case GH_ACTOR_MODEL_ARGUMENT: return GH_ACTOR_MOTION_ARGUMENT;
    case GH_ACTOR_MODEL_BOUNDS: return GH_ACTOR_MOTION_BOUNDS;
    case GH_ACTOR_MODEL_BUSY: return GH_ACTOR_MOTION_BUSY;
    case GH_ACTOR_MODEL_UNSUPPORTED: return GH_ACTOR_MOTION_UNSUPPORTED;
    default: return GH_ACTOR_MOTION_CALLEE;
    }
}

static gh_actor_motion_status enter_2a2(ems_entry *slot,const gh_enemy_ems_bindings *b,
    const gh_enemy_request *request,uint32_t *frontier)
{
    const gh_enemy_actor_bindings *actor=&b->actor;
    const gh_enemy_entry_bindings *entry_bindings=&actor->entry;
    ems_2a2 *heavy=slot->enemy2a2;
    gh_enemy_entry *entry=&heavy->entry;
    int ready;
    gh_actor_model_status status=gh_enemy_entry_prepare_2a2_002905d0(
        entry,entry_bindings,request,0,0,&ready);
    if(status){*frontier=entry->frontier;return model_error(status);}
    if(!ready)return GH_ACTOR_MOTION_OK;
    const gh_enemy2a2_init_bindings init={
        .work=entry_bindings->work,.pools=entry_bindings->pools,.packet_heap=entry_bindings->packet_heap,
        .fog=entry_bindings->fog_rgb,.cockpit=actor->cockpit,.random=&entry_bindings->init->random,
        .policy=entry_bindings->policy,.ik=actor->ik
    };
    const gh_enemy2a2_pose_bindings pose={
        .rate=actor->canonical_rate,.actors=actor->actors,.atan_coeff=actor->atan_coeff,
        .init=entry_bindings->init,.shapes=actor->shapes,.solids=actor->solids,.solid_heap=actor->default_heap
    };
    int result;
    status=gh_enemy2a2_initialize_00284af8(&heavy->init,&entry->object.view,request,0,
        &entry->resource,&init,&pose,b->names_2a2,actor->numeric_policy,&result);
    if(status){
        if(!b->names_2a2->initialized && heavy->init.pose.frontier==0x31d000){
            *frontier=0x14b730;
            return GH_ACTOR_MOTION_UNSUPPORTED;
        }
        *frontier=heavy->init.frontier?heavy->init.frontier:0x284af8;
        return model_error(status);
    }
    if(result!=1){
        /* The original failure calls the gameplay destructor. Native teardown
         * below is for cancellation only and cannot stand in for that call. */
        *frontier=0x290770;
        return GH_ACTOR_MOTION_UNSUPPORTED;
    }
    slot->actor=&entry->object;
    slot->handle=entry->handle;
    slot->registry_owner=entry;
    return GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status release_2a2(ems_entry *entry,uint32_t *frontier)
{
    ems_2a2 *heavy=entry->enemy2a2;
    if(heavy->entry.published){
        unsigned index=(unsigned)heavy->entry.slot;
        const gh_game_init_state *manager=heavy->entry.b.init;
        if(index>=EMS_ROWS||!manager||manager->enemy_actors[index]!=&heavy->entry.object){
            *frontier=0x290770;
            return GH_ACTOR_MOTION_BUSY;
        }
    }
    gh_actor_model_status status=gh_enemy2a2_release_native(&heavy->init);
    if(status){*frontier=heavy->init.frontier;return model_error(status);}
    status=gh_enemy_entry_abort_prefix(&heavy->entry);
    if(status){*frontier=heavy->entry.frontier;return model_error(status);}
    free(heavy);
    entry->enemy2a2=NULL;
    return GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status enter_202(ems_entry *entry,const gh_enemy_ems_bindings *bindings,
    const gh_enemy_request *request,uint32_t *frontier)
{
    if(!entry->enemy202)entry->enemy202=calloc(1,sizeof(*entry->enemy202));
    if(!entry->enemy202)return GH_ACTOR_MOTION_CALLEE;
    gh_actor_motion_status status=gh_enemy_actor_enter_002905d0(entry->enemy202,
        &bindings->actor,request,0,0,&entry->actor);
    if(status)*frontier=entry->enemy202->frontier;
    else if(entry->actor){
        entry->handle=entry->enemy202->entry.handle;
        entry->registry_owner=&entry->enemy202->entry;
    }
    return status;
}

static gh_scene_placement_status spawn(gh_enemy_ems *owner,uint32_t *frontier,int *detail)
{
    gh_em_resource_prefix *prefix=owner->prefix;
    const gh_room_state *room=owner->room;
    const gh_enemy_ems_bindings *bindings=owner->bindings;
    while(prefix->spawn_cursor<room->active[8]) {
        unsigned index=prefix->spawn_cursor;
        *detail=(int)index;
        ems_entry *entry=&owner->entries[index];
        gh_enemy_request request;
        int16_t saved;
        int spawn_row;
        gh_scene_placement_status error=gh_enemy_request_from_room_00294ca0(
            room,index,bindings->actor.numeric_policy,&request,&spawn_row,&saved,frontier);
        if(error)return error;
        if(!spawn_row){
            if(entry->enemy202||entry->enemy2a2)return GH_PLACE_BUSY;
            prefix->spawn_cursor++;
            continue;
        }

        if(!entry->entered){
            gh_actor_motion_status status;
            if(request.object_kind==0x202 &&
                (gh_enemy202_is_r100_supported_profile(&request) ||
                 (request.dependency_kind==0x20a&&(request.appearance==0||request.appearance==3)))) {
                status=enter_202(entry,bindings,&request,frontier);
                if(status&&!entry->enemy202)return GH_PLACE_MEMORY;
            }else if(index==5 && request.entry_number==6 && request.object_kind==0x2a2 &&
                request.dependency_kind==0x2a2 && request.appearance==0 && bindings->names_2a2){
                if(!entry->enemy2a2)entry->enemy2a2=calloc(1,sizeof(*entry->enemy2a2));
                if(!entry->enemy2a2)return GH_PLACE_MEMORY;
                status=enter_2a2(entry,bindings,&request,frontier);
            }else return GH_PLACE_UNSUPPORTED;
            if(status){*detail=status;return motion_error(status);}
            entry->entered=1;
        }

        if(room->active!=owner->page||!valid_page(room)||index>=room->active[8])
            return GH_PLACE_BUSY;
        if(entry->actor){
            if(!actor_registered(owner,entry))return GH_PLACE_BUSY;
            /* The original caller reads saved HP after the possibly sleeping call. */
            const uint8_t *row=room->active+EMS_ROWS_OFFSET+(size_t)index*EMS_ROW_BYTES;
            uint16_t raw=(uint16_t)(row[22]|((uint16_t)row[23]<<8));
            saved=(int16_t)(raw<32768?(int)raw:(int)raw-65536);
            gh_actor_motion_status status=gh_enemy_apply_saved_vitality_00294f34(entry->actor,saved);
            if(status){*frontier=0x294f34;*detail=status;return motion_error(status);}
        }else{
            if(entry->handle)return GH_PLACE_ARGUMENT;
            if(entry->enemy202){
                gh_actor_motion_status status=gh_enemy_actor_shutdown_native(entry->enemy202);
                if(status){*detail=status;return motion_error(status);}
                free(entry->enemy202);
                entry->enemy202=NULL;
            }
        }
        if(!entry->actor&&entry->enemy2a2){
            gh_actor_motion_status status=release_2a2(entry,frontier);
            if(status){*detail=status;return motion_error(status);}
        }
        prefix->spawn_cursor++;
    }
    prefix->stage=9;
    prefix->frontier=0;
    prefix->detail=0;
    return GH_PLACE_OK;
}

gh_scene_placement_status gh_enemy_ems_spawn_00294ca0(gh_enemy_ems **slot,
    gh_em_resource_prefix *prefix,const gh_room_state *room,const gh_enemy_ems_bindings *bindings,
    uint32_t *frontier,int *detail)
{
    if(!slot||!prefix||!room||!room->active||!bindings||!frontier||!detail ||
        !bindings->actor.entry.pools)return GH_PLACE_ARGUMENT;
    *frontier=0;
    *detail=0;
    gh_enemy_ems *owner=*slot;
    if(owner&&(owner->running||owner->closing||owner->prefix!=prefix||owner->room!=room||
        owner->bindings!=bindings||owner->page!=room->active))return GH_PLACE_BUSY;
    if(prefix->stage==9)return GH_PLACE_OK;
    *frontier=0x2905d0;
    *detail=(int)prefix->spawn_cursor;
    if(prefix->stage!=5||!prefix->prefix_complete||prefix->page!=room->active)return GH_PLACE_BUSY;
    if(!valid_page(room)||prefix->spawn_cursor>room->active[8])return GH_PLACE_BOUNDS;
    if(!owner){
        owner=calloc(1,sizeof(*owner));
        if(!owner)return GH_PLACE_MEMORY;
        owner->prefix=prefix;
        owner->room=room;
        owner->page=room->active;
        owner->bindings=bindings;
        *slot=owner;
    }
    owner->running=1;
    gh_scene_placement_status status=spawn(owner,frontier,detail);
    owner->running=0;
    if(status){
        if(!*frontier)*frontier=0x2905d0;
        prefix->frontier=*frontier;
        prefix->detail=*detail;
    }else{
        *frontier=0;
        *detail=0;
    }
    return status;
}

gh_scene_placement_status gh_enemy_ems_set_002952d0(gh_enemy_ems *owner,
    gh_enemy_ems_set_call *call,uint8_t stamp,gh_game_title_player **original,uint32_t *frontier)
{
    if(!owner||!call||!original||!frontier)return GH_PLACE_ARGUMENT;
    *original=NULL;*frontier=0;
    if(owner->running||owner->closing||owner->page!=owner->room->active||!valid_page(owner->room))
        return GH_PLACE_BUSY;
    if(call->stage&&(call->owner!=owner||call->page!=owner->page||call->stamp!=stamp))
        return GH_PLACE_BUSY;
    if(call->stage==2){
        if(call->index==UINT8_MAX)return GH_PLACE_OK;
        ems_entry *entry=&owner->entries[call->index];
        if(entry->actor&&!actor_registered(owner,entry))return GH_PLACE_BUSY;
        *original=entry->actor;
        return GH_PLACE_OK;
    }
    if(!call->stage){
        uint32_t loaded;
        memcpy(&loaded,owner->page,4);
        call->owner=owner;call->page=owner->page;call->stamp=stamp;call->index=UINT8_MAX;
        if(!(loaded&0x80000000u)||stamp==0xff){call->stage=2;return GH_PLACE_OK;}
        unsigned index=0;
        uint8_t *row=owner->room->active+EMS_ROWS_OFFSET;
        for(;index<owner->page[8];index++,row+=EMS_ROW_BYTES)if(row[0x13]==stamp)break;
        if(index==owner->page[8]||row[0x14]==1){call->stage=2;return GH_PLACE_OK;}
        gh_game_title_player *existing;
        if(gh_enemy_manager_get_00290868(owner->bindings->actor.entry.init,stamp,&existing)){
            *frontier=0x290868;return GH_PLACE_BUSY;
        }
        if(existing){call->stage=2;return GH_PLACE_OK;}
        ems_entry *entry=&owner->entries[index];
        if(entry->enemy202||entry->enemy2a2||entry->entered)return GH_PLACE_BUSY;
        row[11]&=0x7fu; /* Original SetEm activates this RoomSave row before CreateEm. */
        int spawn_row;int16_t ignored_saved_hp;
        gh_scene_placement_status status=gh_enemy_request_from_room_00294ca0(owner->room,index,
            owner->bindings->actor.numeric_policy,&call->request,&spawn_row,&ignored_saved_hp,frontier);
        if(status)return status;
        if(!spawn_row){*frontier=0x2951b0;return GH_PLACE_UNSUPPORTED;}
        call->index=(uint8_t)index;call->stage=1;
    }
    ems_entry *entry=&owner->entries[call->index];
    const gh_enemy_request *request=&call->request;
    const int heavy=request->entry_number==14&&request->object_kind==0x2a2&&
        request->dependency_kind==0x2a3&&!request->appearance&&!request->flags&&!request->dependency_flags;
    if(!gh_enemy202_is_r100_supported_profile(request)&&(!heavy||!owner->bindings->names_2a2)){
        *frontier=0x2905d0;return GH_PLACE_UNSUPPORTED;
    }
    if(heavy&&!entry->enemy2a2){
        entry->enemy2a2=calloc(1,sizeof(*entry->enemy2a2));
        if(!entry->enemy2a2){*frontier=0x2905d0;return GH_PLACE_MEMORY;}
    }
    gh_actor_motion_status status=heavy?enter_2a2(entry,owner->bindings,request,frontier):
        enter_202(entry,owner->bindings,request,frontier);
    if(status){
        if(!*frontier)*frontier=0x2905d0;
        return entry->enemy202||entry->enemy2a2?motion_error(status):GH_PLACE_MEMORY;
    }
    if(entry->actor&&!actor_registered(owner,entry))return GH_PLACE_BUSY;
    if(!entry->actor&&entry->enemy202){
        status=gh_enemy_actor_shutdown_native(entry->enemy202);
        if(status)return motion_error(status);
        free(entry->enemy202);entry->enemy202=NULL;
    }
    if(!entry->actor&&entry->enemy2a2){
        status=release_2a2(entry,frontier);
        if(status)return motion_error(status);
    }
    entry->entered=entry->actor!=NULL;
    *original=entry->actor;
    if(!entry->actor)call->index=UINT8_MAX;
    call->stage=2;
    return GH_PLACE_OK;
}

gh_game_title_player *gh_enemy_ems_lookup(const gh_enemy_ems *owner,uint32_t handle)
{
    if(!owner||!handle||owner->running||owner->closing)return NULL;
    for(unsigned i=0;i<EMS_ROWS;i++){
        const ems_entry *entry=&owner->entries[i];
        if(entry->entered&&entry->handle==handle&&actor_registered(owner,entry))return entry->actor;
    }
    return NULL;
}

gh_scene_placement_status gh_enemy_ems_handle_for_actor(const gh_enemy_ems *owner,
    const gh_game_title_player *actor,uint32_t *handle)
{
    if(!actor||!handle)return GH_PLACE_ARGUMENT;
    *handle=0;
    if(!owner||owner->running||owner->closing)return GH_PLACE_OK;
    for(unsigned i=0;i<EMS_ROWS;i++){
        const ems_entry *entry=&owner->entries[i];
        if(!entry->entered||entry->actor!=actor)continue;
        if(!actor_registered(owner,entry))return GH_PLACE_BUSY;
        *handle=entry->handle;
        break;
    }
    return GH_PLACE_OK;
}

gh_scene_placement_status gh_enemy_ems_live_at(gh_enemy_ems *owner,uint32_t handle,
    gh_enemy_ems_live *out)
{
    if(!out||!handle)return GH_PLACE_ARGUMENT;
    *out=(gh_enemy_ems_live){0};
    if(!owner||owner->running||owner->closing)return GH_PLACE_OK;
    for(unsigned i=0;i<EMS_ROWS;i++){
        ems_entry *entry=&owner->entries[i];
        if(!entry->entered||entry->handle!=handle)continue;
        if(!actor_registered(owner,entry))return GH_PLACE_BUSY;
        out->actor=entry->actor;
        out->physical_row=i;
        if(entry->enemy202){
            out->model=&entry->enemy202->entry.initialize.model.model;
            out->shadow=&entry->enemy202->shadow.shadow;
            out->enemy202=entry->enemy202;
            out->move202=&entry->move202;
            out->marker_id=&entry->enemy202->tail.base.cockpit_base;
        }else if(entry->enemy2a2){
            out->model=&entry->enemy2a2->init.prefix.model.model;
            out->shadow=&entry->enemy2a2->init.pose.shadow;
            out->enemy2a2=&entry->enemy2a2->init;
            out->marker_id=&entry->enemy2a2->init.prefix.base.cockpit_base;
            out->other_move=&entry->other_move;
        }
        return out->model?GH_PLACE_OK:GH_PLACE_UNSUPPORTED;
    }
    for(unsigned i=0;i<EMS_ROWS;i++){
        ems_entry *entry=&owner->entries[i];
        gh_enemy_weapon_init *held=gh_enemy_timers_held_weapon_runtime(&entry->retiring_weapon);
        if(!held&&entry->entered&&entry->enemy202&&actor_registered(owner,entry))
            held=gh_enemy_timers_held_weapon_runtime(&entry->enemy202->timers);
        if(!held||!held->weapon||held->weapon->handle!=handle)continue;
        void *registered=NULL;
        if(gh_room_object_lookup(owner->bindings->actor.entry.pools,handle,GH_ROOM_OBJECT_ENEMY_WEAPON,
            held->weapon,&registered)||registered!=&held->weapon->object)return GH_PLACE_BUSY;
        out->actor=&held->weapon->object;
        out->model=&held->model;
        out->physical_row=i;
        out->held_weapon=1;
        out->weapon=held;
        out->other_move=&entry->other_move;
        return GH_PLACE_OK;
    }
    return GH_PLACE_OK;
}

const gh_enemy_weapon_init *gh_enemy_ems_held_weapon(const gh_enemy_ems *owner,
    uint32_t enemy_handle,uint32_t weapon_handle)
{
    if(!owner||(!enemy_handle&&!weapon_handle)||owner->running||owner->closing)return NULL;
    for(unsigned i=0;i<EMS_ROWS;i++){
        const ems_entry *entry=&owner->entries[i];
        if(!entry->enemy202||!entry->entered||!actor_registered(owner,entry)||
            (enemy_handle&&entry->handle!=enemy_handle))continue;
        const gh_enemy_weapon_init *held=gh_enemy_timers_held_weapon(&entry->enemy202->timers);
        if(held&&(!weapon_handle||held->weapon->handle==weapon_handle))return held;
    }
    return NULL;
}

gh_actor_motion_status gh_enemy_manager_get_00290868(const gh_game_init_state *manager,
    uint8_t number,gh_game_title_player **actor)
{
    if(!manager||!actor)return GH_ACTOR_MOTION_ARGUMENT;
    *actor=NULL;
    if(number==UINT8_MAX||!manager->enemy_top)return GH_ACTOR_MOTION_OK;

    unsigned index=EMS_ROWS;
    for(unsigned i=0;i<EMS_ROWS;i++)
        if(manager->enemy_top==manager->enemies+i*20){index=i;break;}
    uint64_t visited=0;
    while(index!=UINT8_MAX){
        if(index>=EMS_ROWS||(visited&(UINT64_C(1)<<index)))return GH_ACTOR_MOTION_BUSY;
        visited|=UINT64_C(1)<<index;
        const uint8_t *record=manager->enemies+index*20;
        gh_game_title_player *object=manager->enemy_actors[index];
        if(record[16]!=1||!object||!object->view.bytes||object->view.size<0x642)
            return GH_ACTOR_MOTION_BUSY;
        const uint8_t *stamp=object->view.bytes+0x640;
        if((uint16_t)(stamp[0]|((uint16_t)stamp[1]<<8))==number){
            *actor=object;
            return GH_ACTOR_MOTION_OK;
        }
        index=manager->enemy_next[index];
    }
    return GH_ACTOR_MOTION_OK;
}

gh_actor_motion_status gh_enemy2a2_set_escape_position_00289038(gh_game_title_player *actor,
    const float position[4])
{
    if(!actor||!actor->view.bytes||!position)return GH_ACTOR_MOTION_ARGUMENT;
    if(actor->view.size<GH_EM2A2_BYTES||actor->view.original_vtable214!=0x448ca0)
        return GH_ACTOR_MOTION_UNSUPPORTED;
    uint8_t *destination=actor->view.bytes+0x1590;
    if((const void *)position!=destination)memmove(destination,position,3*sizeof(float));
    return GH_ACTOR_MOTION_OK;
}

static uint32_t object_word(const uint8_t *bytes)
{
    uint32_t value;
    memcpy(&value, bytes, sizeof(value));
    return value;
}

static int suspend_view(const gh_title_player_view *view, uint32_t vtable, size_t bytes)
{
    return view->bytes && view->size >= bytes && view->original_vtable214 == vtable &&
        object_word(view->bytes + 0x214) == vtable;
}

/* Actual 2A9350 and the own-object prefix of 275308. */
static void set_enemy_suspend_flags(const gh_title_player_view *view, int suspend)
{
    uint32_t flags = object_word(view->bytes + 0x250);
    flags = suspend ? flags | 0x8000u : flags & ~0x8000u;
    memcpy(view->bytes + 0x250, &flags, sizeof(flags));
    flags = object_word(view->bytes + 0x5a0);
    flags = suspend ? flags | 8u : flags & ~8u;
    memcpy(view->bytes + 0x5a0, &flags, sizeof(flags));
}

static gh_actor_motion_status set_entry_suspend(const ems_entry *entry, int suspend,
    uint32_t *frontier)
{
    const gh_title_player_view *view = &entry->actor->view;
    if (entry->enemy2a2) {
        *frontier = 0x2a9350;
        if (!suspend_view(view, 0x448ca0, 0x5a4)) return GH_ACTOR_MOTION_ARGUMENT;
        set_enemy_suspend_flags(view, suspend);
        return GH_ACTOR_MOTION_OK;
    }
    *frontier = 0x275308;
    if (!entry->enemy202 || !suspend_view(view, 0x4479c8, 0x1978))
        return GH_ACTOR_MOTION_UNSUPPORTED;
    static const uint16_t attachment_offsets[] = {
        0x6ec, 0x6f0, 0x6f8, 0x6fc, 0x700, 0x704, 0x708, 0x734, 0x6f4, 0x70c,
        0x1964, 0x196c, 0x1970, 0x1974, 0x740, 0x744, 0x748, 0x74c, 0x750
    };
    for (unsigned i = 0; i < sizeof(attachment_offsets) / sizeof(*attachment_offsets); ++i) {
        if (object_word(view->bytes + attachment_offsets[i])) {
            *frontier = 0x275384 + i * 0x20;
            return GH_ACTOR_MOTION_UNSUPPORTED;
        }
    }
    const gh_enemy_weapon_init *held = gh_enemy_timers_held_weapon(&entry->enemy202->timers);
    const gh_title_player_view *weapon = held ? &held->weapon->object.view : NULL;
    if (weapon && !suspend_view(weapon, 0x42aa98, 0x5a4)) {
        *frontier = 0x275384;
        return GH_ACTOR_MOTION_UNSUPPORTED;
    }
    set_enemy_suspend_flags(view, suspend);
    if (weapon) set_enemy_suspend_flags(weapon, suspend);
    return GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status find_stamp_entry(const gh_enemy_ems *owner,
    const gh_game_init_state *manager,uint8_t stamp,const ems_entry **entry,uint32_t *frontier)
{
    *entry=NULL;
    gh_game_title_player *actor;
    gh_actor_motion_status status=gh_enemy_manager_get_00290868(manager,stamp,&actor);
    if(status){*frontier=0x290868;return status;}
    if(!actor)return GH_ACTOR_MOTION_OK;
    if(!owner||owner->running||owner->closing||owner->bindings->actor.entry.init!=manager)
        return GH_ACTOR_MOTION_UNSUPPORTED;
    for(unsigned i=0;i<EMS_ROWS;i++){
        const ems_entry *candidate=&owner->entries[i];
        if(candidate->actor!=actor)continue;
        if(!candidate->entered||!actor_registered(owner,candidate))return GH_ACTOR_MOTION_UNSUPPORTED;
        *entry=candidate;
        return GH_ACTOR_MOTION_OK;
    }
    return GH_ACTOR_MOTION_UNSUPPORTED;
}

gh_actor_motion_status gh_enemy_ems_set_actor_suspend_00275308(const gh_enemy_ems *owner,
    const gh_game_init_state *manager,const gh_game_title_player *actor,int suspend,uint32_t *frontier)
{
    if(!frontier)return GH_ACTOR_MOTION_ARGUMENT;
    *frontier=0x275308;
    if(!manager||!actor||(suspend!=0&&suspend!=1))return GH_ACTOR_MOTION_ARGUMENT;
    if(!owner||owner->running||owner->closing||owner->bindings->actor.entry.init!=manager||
       actor->view.original_vtable214!=0x4479c8)return GH_ACTOR_MOTION_UNSUPPORTED;
    for(unsigned i=0;i<EMS_ROWS;++i){
        const ems_entry *entry=owner->entries+i;
        if(entry->actor!=actor)continue;
        if(!entry->entered||!actor_registered(owner,entry))return GH_ACTOR_MOTION_BUSY;
        gh_actor_motion_status status=set_entry_suspend(entry,suspend,frontier);
        if(!status)*frontier=0;
        return status;
    }
    return GH_ACTOR_MOTION_UNSUPPORTED;
}

gh_actor_motion_status gh_enemy_ems_set_stamp_suspend_00295bc0(const gh_enemy_ems *owner,
    const gh_game_init_state *manager,uint8_t stamp,int suspend,uint32_t *frontier)
{
    if(!frontier)return GH_ACTOR_MOTION_ARGUMENT;
    *frontier=0x295bc0;
    if(!manager||(suspend!=0&&suspend!=1))return GH_ACTOR_MOTION_ARGUMENT;
    const ems_entry *entry;
    gh_actor_motion_status status=find_stamp_entry(owner,manager,stamp,&entry,frontier);
    if(!status&&entry)status=set_entry_suspend(entry,suspend,frontier);
    if(!status)*frontier=0;
    return status;
}

gh_actor_motion_status gh_enemy_ems_set_stamp_pose_00295a08(const gh_enemy_ems *owner,
    const gh_game_init_state *manager,gh_enemy_stamp_pose *call,uint8_t stamp,const float position[3],
    float yaw,uint32_t *frontier)
{
    if(!frontier)return GH_ACTOR_MOTION_ARGUMENT;
    *frontier=0x295a08;
    if(!manager||!call||!position||!isfinite(position[0])||!isfinite(position[1])||
       !isfinite(position[2])||!isfinite(yaw))return GH_ACTOR_MOTION_ARGUMENT;
    const ems_entry *entry;
    gh_actor_motion_status status=find_stamp_entry(owner,manager,stamp,&entry,frontier);
    if(status)return status;
    if(call->stage&&(!entry||call->actor!=entry->actor||
       memcmp(call->position,position,sizeof(call->position))||memcmp(&call->yaw,&yaw,sizeof(yaw))))
        return GH_ACTOR_MOTION_BUSY;
    if(!entry){*frontier=0;return GH_ACTOR_MOTION_OK;}
    gh_enemy_actor *enemy=entry->enemy202;
    const gh_title_player_view *view=&entry->actor->view;
    if(!enemy||!suspend_view(view,0x4479c8,0x1978))return GH_ACTOR_MOTION_UNSUPPORTED;
    gh_actor_model *model=&enemy->entry.initialize.model.model;
    uint8_t *bytes=view->bytes;
    if(!model->bound||!model->parts_ready||model->root.bytes!=bytes||
       model->root.alias_f0!=view->alias_f0||!view->alias_f0)return GH_ACTOR_MOTION_ARGUMENT;
    const gh_static_transform_policy policy=enemy->b.entry.policy;
    if(!call->stage){
        call->actor=entry->actor;
        memcpy(call->position,position,sizeof(call->position));
        call->yaw=yaw;
        memcpy(view->alias_f0,call->position,sizeof(call->position));
        memcpy(bytes+0x490,call->position,sizeof(call->position));
        memcpy(bytes+0xb0,call->position,sizeof(call->position));
        call->stage=1;
    }
    if(call->stage==1){
        gh_actor_model_status model_status=gh_actor_model_update_hierarchy_00149eb8(model,policy);
        if(model_status){*frontier=model->frontier?model->frontier:0x149eb8;return model_error(model_status);}
        call->stage=2;
    }
    if(call->stage==2){
        const gh_motion_solve_bindings solve={.model=model,.work=enemy->b.entry.work,.actors=enemy->b.actors,
            .ray=enemy->b.ray,.context=enemy->b.ray_context,.ik_globals=enemy->b.ik,.atan_coeff=enemy->b.atan_coeff};
        status=gh_actor_motion_solve_00152a20(&call->solve,&solve,policy,frontier);
        if(status){if(!*frontier)*frontier=0x152a20;return status;}
        call->stage=3;
    }
    if(call->stage==3){
        gh_actor_model_status model_status=gh_actor_finish_model_resume_0014a038(&call->finish,model,policy);
        if(model_status){*frontier=model->frontier?model->frontier:0x14a038;return model_error(model_status);}
        call->stage=4;
    }
    if(call->stage==4){
        /* Original 276228 follows these attachments before applying the new yaw. */
        static const struct {uint16_t offset;uint32_t entry;} attachments[]={
            {0x6ec,0x1cf3a8},{0x6f0,0x1cf3a8},{0x6f8,0x1cf3a8},{0x6fc,0x1cf3a8},
            {0x700,0x1cf3a8},{0x704,0x1cf3a8},{0x708,0x1cf3a8},{0x734,0x1cf3a8},
            {0x6f4,0x1cf3a8},{0x70c,0x1c2378},{0x740,0x27d300},{0x744,0x27e950},
            {0x748,0x280db8},{0x74c,0x283070},{0x750,0x283070}
        };
        if(gh_enemy_timers_held_weapon(&enemy->timers)){*frontier=0x1cf3a8;return GH_ACTOR_MOTION_UNSUPPORTED;}
        for(unsigned i=0;i<sizeof(attachments)/sizeof(*attachments);i++){
            if(!object_word(bytes+attachments[i].offset))continue;
            *frontier=attachments[i].entry;
            return GH_ACTOR_MOTION_UNSUPPORTED;
        }
        const float angles[3]={0.0f,call->yaw,0.0f};
        memcpy(bytes+0x100,angles,sizeof(angles));
        call->stage=5;
    }
    if(call->stage==5){
        gh_actor_model_status model_status=gh_actor_model_update_hierarchy_00149eb8(model,policy);
        if(model_status){*frontier=model->frontier?model->frontier:0x149eb8;return model_error(model_status);}
        call->stage=6;
    }
    *frontier=0;
    return GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status set_enemy_goto(const gh_title_player_view *view,
    const float destination[3],uint32_t mode,int immediate,uint32_t *frontier)
{
    uint8_t *bytes=view->bytes;
    int16_t hp;
    memcpy(&hp,bytes+0x54a,2);
    if(hp<=0||view->original_kind4ac!=0x42c7f0)return GH_ACTOR_MOTION_OK;
    const uint32_t timer=0x497423feu;
    memmove(bytes+0x16b0,destination,12);
    memcpy(bytes+0x16c0,&timer,4);
    memcpy(bytes+0x16c4,&mode,4);
    if(immediate)memcpy(bytes+0x2f4,(const uint8_t[]){0,0x13,0,0},4);
    if(object_word(bytes+0x738)){*frontier=0x1d0408;return GH_ACTOR_MOTION_UNSUPPORTED;}
    return GH_ACTOR_MOTION_OK;
}

gh_actor_motion_status gh_enemy_ems_set_stamp_goto_00295f80(const gh_enemy_ems *owner,
    const gh_game_init_state *manager,uint8_t stamp,const float destination[3],uint32_t mode,int immediate,uint32_t *frontier)
{
    if(!frontier)return GH_ACTOR_MOTION_ARGUMENT;
    *frontier=0x295f80;
    if(!manager||!destination||!isfinite(destination[0])||!isfinite(destination[1])||
       !isfinite(destination[2]))return GH_ACTOR_MOTION_ARGUMENT;
    if(!(((mode==0||mode==2)&&immediate==0)||(mode==9&&immediate==1)))
        return GH_ACTOR_MOTION_UNSUPPORTED;
    const ems_entry *entry;
    gh_actor_motion_status status=find_stamp_entry(owner,manager,stamp,&entry,frontier);
    if(status)return status;
    if(entry){
        const gh_title_player_view *view=&entry->actor->view;
        if(!entry->enemy202||!suspend_view(view,0x4479c8,0x1978))return GH_ACTOR_MOTION_UNSUPPORTED;
        status=set_enemy_goto(view,destination,mode,immediate,frontier);
    }
    if(!status)*frontier=0;
    return status;
}

static void start_enemy_action_00272570(uint8_t *bytes)
{
    const uint32_t flags=object_word(bytes+0x16d0)|0x20000000u;
    memcpy(bytes+0x16d0,&flags,4);
}

gh_actor_motion_status gh_enemy_ems_start_goto_at2(const gh_enemy_ems *owner,
    const gh_game_init_state *manager,uint8_t stamp,const float destination[3],uint32_t *frontier)
{
    if(!frontier)return GH_ACTOR_MOTION_ARGUMENT;
    *frontier=0x295bc0;
    if(!manager||!destination||!isfinite(destination[0])||!isfinite(destination[1])||
       !isfinite(destination[2]))return GH_ACTOR_MOTION_ARGUMENT;
    const ems_entry *entry;
    gh_actor_motion_status status=find_stamp_entry(owner,manager,stamp,&entry,frontier);
    if(status)return status;
    if(!entry){*frontier=0;return GH_ACTOR_MOTION_OK;}
    const gh_title_player_view *view=&entry->actor->view;
    status=set_entry_suspend(entry,0,frontier);
    if(status)return status;
    *frontier=0x295ce8;
    if(!entry->enemy202||!suspend_view(view,0x4479c8,0x1978))return GH_ACTOR_MOTION_UNSUPPORTED;
    start_enemy_action_00272570(view->bytes);
    status=set_enemy_goto(view,destination,0,0,frontier);
    if(status)return status;
    /* 296070 selects the same cEm202 class before 26E7A8. */
    if(view->original_kind4ac==0x42c7f0)memmove(view->bytes+0x1660,destination,12);
    *frontier=0;
    return GH_ACTOR_MOTION_OK;
}

gh_actor_motion_status gh_enemy_ems_start_goto_at3(const gh_enemy_ems *owner,uint8_t stamp,
    const float destination[3],uint32_t *frontier)
{
    if(!owner||!destination||!frontier)return GH_ACTOR_MOTION_ARGUMENT;
    *frontier=0;
    if(owner->running||owner->closing)return GH_ACTOR_MOTION_BUSY;
    gh_game_title_player *actor;
    gh_actor_motion_status status=gh_enemy_manager_get_00290868(owner->bindings->actor.entry.init,stamp,&actor);
    if(status){*frontier=0x290868;return status;}
    if(!actor)return GH_ACTOR_MOTION_OK; /* All three cEmWrap calls recheck missing. */
    const ems_entry *entry=NULL;
    for(unsigned i=0;i<EMS_ROWS;i++)if(owner->entries[i].actor==actor){entry=&owner->entries[i];break;}
    if(!entry||!entry->enemy202||!entry->entered||!actor_registered(owner,entry)||
       !suspend_view(&actor->view,0x4479c8,0x1978)){
        *frontier=0x295ce8;return GH_ACTOR_MOTION_UNSUPPORTED;
    }
    start_enemy_action_00272570(actor->view.bytes);
    status=set_enemy_goto(&actor->view,destination,0,0,frontier);
    if(status)return status;
    status=set_entry_suspend(entry,0,frontier);
    if(!status)*frontier=0;
    return status;
}

gh_actor_motion_status gh_enemy_ems_set_suspend_002948e8(const gh_enemy_ems *owner,
    const gh_game_init_state *manager, int suspend, uint32_t *frontier)
{
    if (!frontier) return GH_ACTOR_MOTION_ARGUMENT;
    *frontier = 0x2948e8;
    if (!manager) return GH_ACTOR_MOTION_ARGUMENT;
    if (!manager->enemy_top) {
        if (object_word(manager->enemies + 0x500)) return GH_ACTOR_MOTION_UNSUPPORTED;
        *frontier = 0;
        return GH_ACTOR_MOTION_OK;
    }
    if (!owner || owner->running || owner->closing || owner->bindings->actor.entry.init != manager)
        return GH_ACTOR_MOTION_UNSUPPORTED;

    unsigned index = EMS_ROWS;
    for (unsigned i = 0; i < EMS_ROWS; ++i)
        if (manager->enemy_top == manager->enemies + i * 20) { index = i; break; }
    uint64_t visited = 0;
    while (index != UINT8_MAX) {
        *frontier = 0x2948e8;
        if (index >= EMS_ROWS || (visited & (UINT64_C(1) << index))) return GH_ACTOR_MOTION_BUSY;
        visited |= UINT64_C(1) << index;
        const gh_game_title_player *actor = manager->enemy_actors[index];
        if (actor) {
            if (!actor->view.bytes || actor->view.size < 0x300) return GH_ACTOR_MOTION_BOUNDS;
            const uint8_t *kind_bytes = actor->view.bytes + 0x2fe;
            unsigned kind = (unsigned)kind_bytes[0] | (unsigned)kind_bytes[1] << 8;
            if (kind >= 0x200 && kind < 0x300) {
                const ems_entry *entry = NULL;
                for (unsigned i = 0; i < EMS_ROWS; ++i)
                    if (owner->entries[i].actor == actor) { entry = &owner->entries[i]; break; }
                if (!entry || !entry->entered || !actor_registered(owner, entry))
                    return GH_ACTOR_MOTION_UNSUPPORTED;
                gh_actor_motion_status status = set_entry_suspend(entry, suspend, frontier);
                if (status) return status;
            }
        }
        index = manager->enemy_next[index];
    }
    *frontier = 0;
    return GH_ACTOR_MOTION_OK;
}

gh_actor_motion_status gh_enemy_ems_unsuspend_002948e8(const gh_enemy_ems *owner,
    const gh_game_init_state *manager, uint32_t *frontier)
{
    return gh_enemy_ems_set_suspend_002948e8(owner, manager, 0, frontier);
}

gh_actor_motion_status gh_enemy_ems_release_native(gh_enemy_ems **slot,uint32_t *frontier)
{
    if(!slot||!frontier)return GH_ACTOR_MOTION_ARGUMENT;
    *frontier=0;
    gh_enemy_ems *owner=*slot;
    if(!owner)return GH_ACTOR_MOTION_OK;
    owner->closing=1;
    owner->running=0;
    for(unsigned i=EMS_ROWS;i>0;i--){
        ems_entry *entry=&owner->entries[i-1];
        if(entry->retiring_weapon.held){
            *frontier=0x1c91b0;
            return GH_ACTOR_MOTION_UNSUPPORTED;
        }
        if(entry->enemy2a2){
            gh_actor_motion_status status=release_2a2(entry,frontier);
            if(status)return status;
        }
        if(entry->enemy202){
            gh_actor_motion_status status=gh_enemy_actor_shutdown_native(entry->enemy202);
            if(status){*frontier=entry->enemy202->frontier;return status;}
            free(entry->enemy202);
        }
        *entry=(ems_entry){0};
    }
    free(owner);
    *slot=NULL;
    return GH_ACTOR_MOTION_OK;
}

static void retire_put_word(uint8_t *bytes,uint32_t value)
{
    memcpy(bytes,&value,4);
}

static ems_entry *retire_find(gh_enemy_ems *owner,uint32_t handle)
{
    for(unsigned i=0;i<EMS_ROWS;i++)
        if(owner->entries[i].entered&&owner->entries[i].handle==handle)return &owner->entries[i];
    return NULL;
}

static gh_enemy_entry *retire_entry(ems_entry *entry)
{
    return entry->enemy202?&entry->enemy202->entry:entry->enemy2a2?&entry->enemy2a2->entry:NULL;
}

static gh_actor_motion_status retire_stop(uint32_t *frontier,uint32_t pc)
{
    *frontier=pc;
    return GH_ACTOR_MOTION_UNSUPPORTED;
}

static int slot_index(const gh_game_init_state *manager,const void *slot)
{
    if(!slot)return -1;
    for(unsigned i=0;i<64;i++)if(slot==manager->enemies+i*20)return (int)i;
    return -2;
}

static int unlink_slot(gh_game_init_state *manager,gh_enemy_entry *actor,int commit)
{
    const unsigned slot=(unsigned)actor->slot;
    if(slot>=64||manager->enemy_actors[slot]!=&actor->object||
       manager->enemies[slot*20+16]!=1)return 0;
    int at=slot_index(manager,manager->enemy_top);
    const int last=slot_index(manager,manager->enemy_last);
    if(at<0||last<0)return 0;
    uint64_t visited=0;
    int previous=-1;
    unsigned count=0,same_kind=0;
    const uint32_t kind=object_word(manager->enemies+slot*20+12);
    while(at>=0){
        unsigned i=(unsigned)at;
        if(i>=64||(visited&(UINT64_C(1)<<i))||!manager->enemy_actors[i]||
           manager->enemies[20*i+16]!=1||
           manager->enemy_prev[i]!=(previous<0?UINT8_MAX:(unsigned)previous))
            return 0;
        visited|=UINT64_C(1)<<i;
        if(object_word(manager->enemies+20*i+12)==kind)++same_kind;
        ++count;
        previous=at;
        at=manager->enemy_next[i]==UINT8_MAX?-1:manager->enemy_next[i];
    }
    const uint32_t kind_count=object_word(manager->enemies+0x50c);
    if(previous!=last||!(visited&(UINT64_C(1)<<slot))||
       count!=object_word(manager->enemies+0x508)||!same_kind||!kind_count)
        return 0;
    if(!commit)return 1;
    const unsigned prev=manager->enemy_prev[slot],next=manager->enemy_next[slot];
    if(prev==UINT8_MAX)manager->enemy_top=next==UINT8_MAX?NULL:manager->enemies+20*next;
    else manager->enemy_next[prev]=(uint8_t)next;
    if(next==UINT8_MAX)manager->enemy_last=prev==UINT8_MAX?NULL:manager->enemies+20*prev;
    else manager->enemy_prev[next]=(uint8_t)prev;
    uint8_t *raw=manager->enemies+20*slot;
    retire_put_word(raw+4,0);
    retire_put_word(raw+12,UINT32_MAX);
    retire_put_word(raw+8,0);
    raw[16]=0;
    retire_put_word(raw,0);
    manager->enemy_actors[slot]=NULL;
    manager->enemy_prev[slot]=manager->enemy_next[slot]=UINT8_MAX;
    if(same_kind==1)retire_put_word(manager->enemies+0x50c,kind_count-1);
    retire_put_word(manager->enemies+0x508,count-1);
    actor->published=0;
    return 1;
}

gh_actor_motion_status gh_enemy_ems_remove_00290770(gh_enemy_ems *owner,gh_enemy_manager_remove *call,
    uint32_t handle,gh_se_voice_engine *voices,int *original,uint32_t *frontier)
{
    if(!original||!frontier)return GH_ACTOR_MOTION_ARGUMENT;
    *original=0;*frontier=0;
    if(!owner||!call||!handle||!voices||owner->running||owner->closing||
       (call->stage&&(call->owner!=owner||call->handle!=handle)))return GH_ACTOR_MOTION_ARGUMENT;
    if(call->stage==2){*original=call->original_return;return GH_ACTOR_MOTION_OK;}
    ems_entry *entry=retire_find(owner,handle);
    gh_enemy_entry *actor=entry?retire_entry(entry):NULL;
    if(!actor||!actor->published){
        call->owner=owner;call->handle=handle;call->stage=2;
        return GH_ACTOR_MOTION_OK;
    }
    if(!actor_registered(owner,entry)||!actor->object.view.bytes||actor->object.view.size<0x642)
        return retire_stop(frontier,0x290770);
    gh_game_init_state *manager=actor->b.init;
    if(!manager||!unlink_slot(manager,actor,0))return retire_stop(frontier,0x290988);
    if(!call->stage){
        const uint8_t *bytes=actor->object.view.bytes;
        const unsigned kind=bytes[0x2fe]|(unsigned)bytes[0x2ff]<<8;
        if(kind!=0x202&&kind!=0x2a2)return retire_stop(frontier,0x2956f0);
        /* 2956F0 only stores surviving 220/223 actors. */
        unsigned stamp=bytes[0x640]|(unsigned)bytes[0x641]<<8;
        if(stamp<64){
            if(!owner->room||owner->page!=owner->room->active||!valid_page(owner->room))
                return retire_stop(frontier,0x2bef60);
            for(unsigned i=0;i<owner->page[8];i++){
                uint8_t *row=owner->room->active+EMS_ROWS_OFFSET+i*EMS_ROW_BYTES;
                if(row[0x13]!=stamp)continue;
                int16_t hp;
                memcpy(&hp,bytes+0x54a,2);
                if(hp<1)hp=1;
                memcpy(row+0x16,&hp,2);
                break;
            }
        }
        call->owner=owner;call->handle=handle;call->stage=1;
    }
    gh_actor_motion_status status=gh_enemy_death_mark_00312708(&call->mark,owner,handle,voices,frontier);
    if(status)return status;
    if(!unlink_slot(manager,actor,1))return retire_stop(frontier,0x290988);
    call->stage=2;call->original_return=1;*original=1;
    return GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status retire_resource(gh_enemy_entry *actor,const gh_enemy_retire_bindings *b,
    uint32_t *frontier)
{
    gh_game_objects *holder=actor->b.room_objects;
    const uint32_t kind=actor->request.object_kind;
    uint8_t *reference=&actor->room_reference;
    int ready=0;
    gh_object_status e=gh_game_objects_check_ready_001ff238(holder,kind,&actor->b.resources,&ready,frontier);
    if(e&&e!=GH_OBJECT_MISSING)return retire_stop(frontier,*frontier?*frontier:0x1fe9b8);
    if(!ready){
        holder=actor->b.global_objects;reference=&actor->global_reference;
        e=gh_game_objects_check_ready_001ff238(holder,kind,&actor->b.resources,&ready,frontier);
        if(e&&e!=GH_OBJECT_MISSING)return retire_stop(frontier,*frontier?*frontier:0x1fe9b8);
    }
    if(!ready)return retire_stop(frontier,0x1fe9b8);
    if(!*reference||actor->preload_reference||(&actor->room_reference!=reference&&actor->room_reference)||
       (&actor->global_reference!=reference&&actor->global_reference))
        return retire_stop(frontier,0x1fed78);
    if(gh_game_objects_release_event_001fed78(holder,kind,actor->resource.bytes,
       b->release_sound_slot,b->sound_context,frontier))return retire_stop(frontier,*frontier?*frontier:0x1fed78);
    *reference=0;
    return GH_ACTOR_MOTION_OK;
}

static int same_retire_bindings(const gh_enemy_retire_bindings *a,const gh_enemy_retire_bindings *b)
{
    return a->effects==b->effects&&a->dynamic==b->dynamic&&a->release_effect==b->release_effect&&
        a->effect_context==b->effect_context&&a->release_sound_slot==b->release_sound_slot&&
        a->sound_context==b->sound_context;
}

static gh_actor_motion_status retire_mode3(gh_enemy_ems *owner,gh_enemy_full_retirement *call,
    uint32_t handle,const gh_title_player_view *detached,const gh_enemy_retire_bindings *b,uint32_t *frontier)
{
    if(!frontier)return GH_ACTOR_MOTION_ARGUMENT;
    *frontier=0;
    if(!owner||!call||!b||!b->effects||!b->dynamic||!handle||owner->running||owner->closing||
       (call->stage&&(call->owner!=owner||call->handle!=handle||!same_retire_bindings(&call->bindings,b))))
        return GH_ACTOR_MOTION_ARGUMENT;
    if(call->stage==7)return GH_ACTOR_MOTION_OK;
    ems_entry *entry=retire_find(owner,handle);
    gh_enemy_entry *actor=entry?retire_entry(entry):NULL;
    if(!actor||!actor_registered(owner,entry)||(detached&&actor->published))
        return retire_stop(frontier,0x290770);
    if(actor->published&&(!actor->b.init||!unlink_slot(actor->b.init,actor,0)))
        return retire_stop(frontier,0x290988);
    if(detached&&call->stage<6&&
       (detached->bytes!=actor->object.view.bytes||detached->size!=actor->object.view.size))
        return GH_ACTOR_MOTION_ARGUMENT;
    gh_room_pools *pools=actor->b.pools;
    gh_actor_model *model=entry->enemy202?&entry->enemy202->entry.initialize.model.model:
        &entry->enemy2a2->init.prefix.model.model;
    if(!call->stage){
        const gh_room_vector *primary,*secondary;
        if(!actor->object.view.bytes||actor->object.view.size<0x300||
           (object_word(actor->object.view.bytes+0x250)&0xa0000000u)!=0xa0000000u||
           !model->ready||model->root.bytes!=actor->object.view.bytes||
           gh_room_registry_view(pools,0x754c30,&primary)||gh_room_registry_view(pools,0x754c58,&secondary)||
           !b->dynamic->bound||b->dynamic->tables!=owner->bindings->actor.actors)
            return GH_ACTOR_MOTION_ARGUMENT;
        if(model->external_owner||model->external_parts||model->external_borrowers||model->sound_borrowers)
            return retire_stop(frontier,0x1496c0);
        int borrowed;
        if(gh_effect_create_actor_has_live_refs(b->effects,&actor->object.view,&borrowed)||borrowed)
            return retire_stop(frontier,0x305b90);
        unsigned primary_matches=0,secondary_matches=0,at=0;
        for(unsigned i=0;i<primary->count;i++)
            primary_matches+=object_word((uint8_t *)primary->allocation.pointer+4*i)==handle;
        for(unsigned i=0;i<secondary->count;i++){
            if(object_word((uint8_t *)secondary->allocation.pointer+4*i)!=handle)continue;
            ++secondary_matches;at=i;
        }
        if(primary_matches!=1||secondary_matches!=(unsigned)!detached)
            return retire_stop(frontier,0x312db8);
        if(!detached){
            uint8_t *bytes=secondary->allocation.pointer;
            memmove(bytes+4*at,bytes+4*(at+1),4*(size_t)(secondary->count-at-1));
            --pools->vectors[3].count;
        }
        call->owner=owner;call->handle=handle;call->bindings=*b;call->stage=1;
    }
    if(call->stage==1){
        gh_actor_motion_status status=gh_enemy_destroy_prefix_mode3(&call->destructor,owner,handle,
            b->release_effect,b->effect_context,frontier);
        if(status!=GH_ACTOR_MOTION_UNSUPPORTED||*frontier!=0x1fe9b8||call->destructor.stage!=6)
            return status?status:retire_stop(frontier,0x14e5d0);
        call->stage=2;
    }
    if(call->stage==2){
        gh_actor_motion_status status=retire_resource(actor,b,frontier);
        if(status)return status;
        call->stage=3;
    }
    if(call->stage==3){
        if(gh_collision_pair_shapes_retire_model_native(owner->bindings->actor.shapes.pairs,model)||
           gh_hitarea_forget_actor_native(actor->b.init,&actor->object.view))
            return retire_stop(frontier,0x1496c0);
        gh_scene_dynamic_forget_sphere_actor(b->dynamic,&actor->object.view);
        call->stage=4;
    }
    if(call->stage==4){
        gh_actor_model_status status=gh_actor_model_retire_001496c0(model,&call->model,frontier);
        if(status)return *frontier?model_error(status):retire_stop(frontier,0x1496c0);
        call->stage=5;
    }
    if(call->stage==5){
        /* Standalone State5 deletes the registered actor directly. Retire
         * the native manager borrow before freeing that same wrapper. */
        if(actor->published&&!unlink_slot(actor->b.init,actor,1))
            return retire_stop(frontier,0x290988);
        if(gh_game_title_player_release(&actor->object))return retire_stop(frontier,0x30f5a0);
        call->stage=6;
    }
    return GH_ACTOR_MOTION_OK;
}

gh_actor_motion_status gh_enemy_ems_destroy_detached_mode3(gh_enemy_ems *owner,
    gh_enemy_full_retirement *call,uint32_t handle,const gh_title_player_view *actor,
    const gh_enemy_retire_bindings *bindings,uint32_t *frontier)
{
    if(!actor||!actor->bytes)return GH_ACTOR_MOTION_ARGUMENT;
    return retire_mode3(owner,call,handle,actor,bindings,frontier);
}

gh_actor_motion_status gh_enemy_ems_finish_detached_mode3(gh_enemy_ems *owner,
    gh_enemy_full_retirement *call,uint32_t handle,const void *registry_owner,uint32_t *frontier)
{
    if(!frontier||!owner||!call||call->stage!=6||call->owner!=owner||call->handle!=handle)
        return GH_ACTOR_MOTION_ARGUMENT;
    ems_entry *entry=retire_find(owner,handle);
    gh_enemy_entry *actor=entry?retire_entry(entry):NULL;
    if(!actor||actor->published||!registry_owner||entry->registry_owner!=registry_owner||
       gh_room_object_unregister(actor->b.pools,handle,registry_owner))
        return retire_stop(frontier,0x30f7d0);
    free(entry->enemy202);
    free(entry->enemy2a2);
    gh_enemy_timers retiring_weapon=entry->retiring_weapon;
    *entry=(ems_entry){.retiring_weapon=retiring_weapon};
    call->stage=7;
    return GH_ACTOR_MOTION_OK;
}

gh_actor_motion_status gh_enemy_ems_mark_held_weapon_00312708(gh_enemy_ems *owner,
    uint32_t enemy_handle,gh_se_voice_engine *voices,uint32_t *frontier)
{
    if(!owner||!voices||!frontier)return GH_ACTOR_MOTION_ARGUMENT;
    ems_entry *entry=retire_find(owner,enemy_handle);
    if(!entry||!entry->enemy202||entry->retiring_weapon.held)
        return retire_stop(frontier,0x1b79b0);
    gh_enemy_timers *timers=&entry->enemy202->timers;
    gh_enemy_weapon_init *held=gh_enemy_timers_held_weapon_runtime(timers);
    if(!held||!held->weapon||held->attach_stage!=5||!held->model.ready||
       held->parent!=&entry->actor->view||
       held->parent_model!=&entry->enemy202->entry.initialize.model.model||
       held->model.external_owner!=held)
        return retire_stop(frontier,0x1b79b0);
    uint8_t *actor=held->weapon->object.view.bytes;
    if(!actor||held->weapon->object.view.size!=GH_ENEMY_WEAPON_363_BYTES||
       held->model.root.bytes!=actor||object_word(actor+0x214)!=0x42aa98||
       held->weapon->kind<0x361||held->weapon->kind>0x363||
       (object_word(actor+0x2fc)>>16)!=held->weapon->kind)
        return retire_stop(frontier,0x312748);
    if(!(object_word(actor+0x250)&0x80000000u)){
        if(gh_se_voice_detach_actor_002ce488(voices,&held->model)){
            uint32_t pc=gh_se_voice_frontier(voices);
            return retire_stop(frontier,pc?pc:0x2ce488);
        }
        /* Original VT+4C 14E928 returns before publishing death. */
        retire_put_word(actor+0x250,object_word(actor+0x250)|0x80000002u);
    }
    /* 208858 clears +6EC. End only the corresponding native hierarchy
     * borrow; the same registered weapon survives to its own 30F8B0 visit. */
    if(gh_actor_model_clear_external_parent(&held->model,held))
        return retire_stop(frontier,0x1cf160);
    held->parent=NULL;
    held->parent_model=NULL;
    entry->retiring_weapon.held=timers->held;
    timers->held=NULL;
    return GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status retire_weapon_hit(gh_enemy_weapon_init *held,unsigned i,uint32_t *frontier)
{
    if(!(held->hit_owned&(1u<<i)))return GH_ACTOR_MOTION_OK;
    gh_hitarea_object *hit=&held->hit[i];
    int original;
    if(gh_hitarea_object_clear_shapes_001fd580(hit,&original,frontier)||
       gh_hitarea_object_release_empty_001fe278(hit,frontier))
        return retire_stop(frontier,*frontier?*frontier:i?0x1fe148:0x1fe370);
    held->hit_owned&=(uint8_t)~(1u<<i);
    *hit=(gh_hitarea_object){0};
    return GH_ACTOR_MOTION_OK;
}

gh_actor_motion_status gh_enemy_ems_retire_weapon_0030f8b0(gh_enemy_ems *owner,
    gh_enemy_full_retirement *call,uint32_t handle,const gh_enemy_retire_bindings *bindings,
    uint32_t *frontier)
{
    if(!owner||!call||!bindings||!frontier||!handle||owner->running||owner->closing||
       (call->stage&&(call->owner!=owner||call->handle!=handle||
        !same_retire_bindings(&call->bindings,bindings))))return GH_ACTOR_MOTION_ARGUMENT;
    *frontier=0;
    if(call->stage==7)return GH_ACTOR_MOTION_OK;
    gh_enemy_timers *timers=NULL;
    gh_enemy_weapon_init *held=NULL;
    for(unsigned i=0;i<EMS_ROWS;i++){
        gh_enemy_weapon_init *candidate=gh_enemy_timers_held_weapon_runtime(&owner->entries[i].retiring_weapon);
        if(candidate&&candidate->weapon->handle==handle){
            held=candidate;
            timers=&owner->entries[i].retiring_weapon;
            break;
        }
    }
    if(!held||!timers)return retire_stop(frontier,0x1c91b0);
    gh_enemy_weapon *weapon=held->weapon;
    gh_title_player_view *view=&weapon->object.view;
    uint8_t *actor=view->bytes;
    const uint16_t kind=weapon->kind;
    gh_room_pools *pools=weapon->bindings.pools;
    if(kind<0x361||kind>0x363)return retire_stop(frontier,0x1c91b0);
    if(!call->stage){
        const gh_room_vector *primary,*secondary;
        if(!actor||view->size!=GH_ENEMY_WEAPON_363_BYTES||object_word(actor+0x214)!=0x42aa98||
           (object_word(actor+0x2fc)>>16)!=kind||
           (object_word(actor+0x250)&0xa0000000u)!=0xa0000000u||object_word(actor+0x650)||
           !held->model.ready||held->model.external_owner||held->model.external_borrowers||
           held->model.sound_borrowers||gh_room_registry_view(pools,0x754c30,&primary)||
           gh_room_registry_view(pools,0x754c58,&secondary))
            return retire_stop(frontier,0x1c91b0);
        uint8_t *bytes=secondary->allocation.pointer;
        unsigned at=0,primary_matches=0,secondary_matches=0;
        for(unsigned i=0;i<primary->count;i++)
            primary_matches+=object_word((uint8_t*)primary->allocation.pointer+4*i)==handle;
        for(unsigned i=0;i<secondary->count;i++)if(object_word(bytes+4*i)==handle){
            at=i;
            ++secondary_matches;
        }
        if(primary_matches!=1||secondary_matches!=1)return retire_stop(frontier,0x30f9a4);
        memmove(bytes+4*at,bytes+4*(at+1),4*(size_t)(secondary->count-at-1));
        --pools->vectors[3].count;
        call->owner=owner;call->handle=handle;call->bindings=*bindings;call->stage=1;
    }
    if(call->stage==1){
        gh_actor_motion_status status=retire_weapon_hit(held,1,frontier);
        if(status)return status;
        actor[0x654]=actor[0x655]=actor[0x656]=actor[0x657]=0xff;
        call->stage=2;
    }
    if(call->stage==2){
        if(!bindings->release_effect)return retire_stop(frontier,0x306200);
        gh_actor_motion_status status=bindings->release_effect(bindings->effect_context,view,8,2,frontier);
        if(status)return status;
        call->stage=3;
    }
    if(call->stage==3){
        if(object_word(actor+0x68c))return retire_stop(frontier,0x1b79b0);
        retire_put_word(actor+0x214,0x428a20);view->original_vtable214=0x428a20;
        uint64_t key;
        memcpy(&key,actor+0x540,sizeof(key));
        uint32_t changed;
        if(gh_collision_scroll_set_key_enabled_001331b8(held->actors,held->scroll,key,0,&changed))
            return retire_stop(frontier,0x1331b8);
        if(gh_collision_solid_release_unit_001347e8(owner->bindings->actor.solids,view,frontier))
            return retire_stop(frontier,*frontier?*frontier:0x1347e8);
        gh_actor_motion_status status=retire_weapon_hit(held,0,frontier);
        if(status)return status;
        call->stage=4;
    }
    if(call->stage==4){
        gh_game_objects *resources=weapon->bindings.room_objects;
        int ready=0;
        gh_object_status status=gh_game_objects_check_ready_001ff238(resources,kind,
            &weapon->bindings.resources,&ready,frontier);
        if(status!=GH_OBJECT_OK&&status!=GH_OBJECT_MISSING)return retire_stop(frontier,*frontier?*frontier:0x1fe9b8);
        if(!ready){
            resources=weapon->bindings.global_objects;
            status=gh_game_objects_check_ready_001ff238(resources,kind,&weapon->bindings.resources,&ready,frontier);
            if(status!=GH_OBJECT_OK&&status!=GH_OBJECT_MISSING)return retire_stop(frontier,*frontier?*frontier:0x1fe9b8);
        }
        if(!ready||resources!=weapon->resource_owner)return retire_stop(frontier,0x1fe9b8);
        if(gh_game_objects_release_event_001fed78(resources,kind,weapon->resource.bytes,
            bindings->release_sound_slot,bindings->sound_context,frontier))
            return retire_stop(frontier,*frontier?*frontier:0x1fed78);
        weapon->resource_owner=NULL;
        call->stage=5;
    }
    if(call->stage==5){
        retire_put_word(actor+0x214,0x41eaf0);view->original_vtable214=0x41eaf0;
        if(gh_collision_pair_shapes_retire_model_native(held->shapes.pairs,&held->model)||
           gh_hitarea_forget_actor_native(weapon->bindings.init,view))
            return retire_stop(frontier,0x1496c0);
        gh_actor_model_status status=gh_actor_model_retire_001496c0(&held->model,&call->model,frontier);
        if(status)return *frontier?model_error(status):retire_stop(frontier,0x1496c0);
        call->stage=6;
    }
    if(weapon->object.view.bytes&&gh_game_title_player_release(&weapon->object))
        return retire_stop(frontier,0x30f5a0);
    if(gh_room_object_unregister(pools,handle,weapon))return retire_stop(frontier,0x30fa08);
    weapon->handle=0;
    gh_actor_motion_status status=gh_enemy_timers_finish_held_retirement(timers);
    if(status)return retire_stop(frontier,0x1c91b0);
    call->stage=7;
    return GH_ACTOR_MOTION_OK;
}

gh_actor_motion_status gh_enemy_ems_retire_0030f8b0(gh_enemy_ems *owner,gh_enemy_full_retirement *call,
    uint32_t handle,const gh_enemy_retire_bindings *bindings,uint32_t *frontier)
{
    gh_actor_motion_status status=retire_mode3(owner,call,handle,NULL,bindings,frontier);
    if(status||call->stage==7)return status;
    ems_entry *entry=retire_find(owner,handle);
    return gh_enemy_ems_finish_detached_mode3(owner,call,handle,entry?entry->registry_owner:NULL,frontier);
}
