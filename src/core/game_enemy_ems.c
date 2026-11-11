#include "game_enemy_ems.h"
#include "game_enemy_row2.h"
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
                if(!entry->enemy202)entry->enemy202=calloc(1,sizeof(*entry->enemy202));
                if(!entry->enemy202)return GH_PLACE_MEMORY;
                status=gh_enemy_actor_enter_002905d0(entry->enemy202,&bindings->actor,
                    &request,0,0,&entry->actor);
                if(status)*frontier=entry->enemy202->frontier;
                else if(entry->actor){
                    entry->handle=entry->enemy202->entry.handle;
                    entry->registry_owner=&entry->enemy202->entry;
                }
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

gh_game_title_player *gh_enemy_ems_lookup(const gh_enemy_ems *owner,uint32_t handle)
{
    if(!owner||!handle||owner->running||owner->closing)return NULL;
    for(unsigned i=0;i<EMS_ROWS;i++){
        const ems_entry *entry=&owner->entries[i];
        if(entry->entered&&entry->handle==handle&&actor_registered(owner,entry))return entry->actor;
    }
    return NULL;
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
        if(!entry->entered||!entry->enemy202||!actor_registered(owner,entry))continue;
        gh_enemy_weapon_init *held=gh_enemy_timers_held_weapon_runtime(&entry->enemy202->timers);
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

/* Actual 2A9350 and the own-object prefix of 275308 with argument zero. */
static void clear_enemy_suspend(const gh_title_player_view *view)
{
    uint32_t flags = object_word(view->bytes + 0x250) & ~0x8000u;
    memcpy(view->bytes + 0x250, &flags, sizeof(flags));
    flags = object_word(view->bytes + 0x5a0) & ~8u;
    memcpy(view->bytes + 0x5a0, &flags, sizeof(flags));
}

static gh_actor_motion_status unsuspend_entry(const ems_entry *entry, uint32_t *frontier)
{
    const gh_title_player_view *view = &entry->actor->view;
    if (entry->enemy2a2) {
        *frontier = 0x2a9350;
        if (!suspend_view(view, 0x448ca0, 0x5a4)) return GH_ACTOR_MOTION_ARGUMENT;
        clear_enemy_suspend(view);
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
    clear_enemy_suspend(view);
    if (weapon) clear_enemy_suspend(weapon);
    return GH_ACTOR_MOTION_OK;
}

gh_actor_motion_status gh_enemy_ems_unsuspend_002948e8(const gh_enemy_ems *owner,
    const gh_game_init_state *manager, uint32_t *frontier)
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
                gh_actor_motion_status status = unsuspend_entry(entry, frontier);
                if (status) return status;
            }
        }
        index = manager->enemy_next[index];
    }
    *frontier = 0;
    return GH_ACTOR_MOTION_OK;
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
