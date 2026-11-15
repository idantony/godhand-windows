#include "r102_room.h"

#include "../core/game_om382.h"
#include "../core/game_om_frame.h"
#include "../core/game_actor_model_nodes.h"
#include "../core/game_enemy_ems.h"
#include "r100_scripts.h"
#include "r100_roof_task.h"

#include <string.h>

static gh_scene_status placement(gh_scene_placement_status status)
{
    return status==GH_PLACE_UNSUPPORTED?GH_SCENE_UNSUPPORTED:GH_SCENE_CALLEE;
}

gh_scene_status gh_r102_room_init_prefix_000001a8(gh_r102_room *state,const gh_room_module *module,
    gh_heap *heap,gh_om_factory *objects,uint32_t *frontier)
{
    if(!frontier)return GH_SCENE_ARGUMENT;
    *frontier=0x1a8;
    if(!state||!module||!module->bound||module->room!=0x102||module->init_offset!=0x1a8||
       !heap||!objects||!objects->bound||!objects->bindings.work||objects->bindings.work->room!=0x102||
       (state->work.pointer&&(state->module!=module||state->heap!=heap||state->objects!=objects)))
        return GH_SCENE_ARGUMENT;
    if(!state->work.pointer){
        *frontier=0x30f550;
        if(gh_heap_allocate(heap,8,16,&state->work))return GH_SCENE_CALLEE;
        state->module=module;
        state->heap=heap;
        state->objects=objects;
    }
    if(!state->prefix_complete){
        gh_om_object *weapon=NULL;
        gh_scene_placement_status status=gh_om_factory_create_0030fa98(objects,&state->weapon,
            0x363,UINT16_MAX,&weapon);
        if(status){
            *frontier=objects->frontier?objects->frontier:0x30fa98;
            return placement(status);
        }
        if(weapon){
            status=gh_om_factory_initialize_runtime_weapon_001c9268(objects,weapon);
            if(status){
                *frontier=objects->frontier?objects->frontier:0x1c9268;
                return placement(status);
            }
            const uint32_t position[]={0x41c67ae1,0,0xbfc147ae};
            const uint32_t euler[]={0,0x3ff5be0b,0};
            memcpy(weapon->view.alias_f0,position,sizeof(position));
            memcpy(weapon->view.bytes+0x100,euler,sizeof(euler));
        }
        state->prefix_complete=1;
    }
    *frontier=0;
    return GH_SCENE_OK;
}

enum { SEARCH_PLACEMENT, DISABLE_COLLISION, FIRST_TASK, NEXT_PLACEMENT, NEXT_TASK };

gh_scene_status gh_r102_room_prepare_om046_000002b8(gh_r102_room *state,
    gh_r100_om_scalar_view object_view,gh_r100_collision_enable set_collision,
    void *context,uint32_t *frontier)
{
    if(!frontier)return GH_SCENE_ARGUMENT;
    *frontier=0x2b8;
    if(!state||!state->prefix_complete||!state->objects||!state->objects->bound)
        return GH_SCENE_ARGUMENT;
    if(state->om046_stage==SEARCH_PLACEMENT){
        gh_r100_om_view found;
        const gh_r100_script_status status=gh_r100_find_om_0014fd98(state->objects->bindings.pools,
            object_view,context,"om046",&found,frontier);
        if(status)return status==GH_R100_SCRIPT_UNSUPPORTED?GH_SCENE_UNSUPPORTED:GH_SCENE_CALLEE;
        if(found.bytes){
            *frontier=0x2e0;
            if(found.size<0x5f4||found.bytes[0x2fe]!=0x11||found.bytes[0x2ff]!=3)
                return GH_SCENE_UNSUPPORTED;
            const uint32_t enabled=1;
            uint8_t *bytes=(uint8_t*)found.bytes;
            memcpy(bytes+0x5ec,&enabled,sizeof(enabled));
            memcpy(bytes+0x5f0,&enabled,sizeof(enabled));
            state->om046_stage=DISABLE_COLLISION;
        }else{
            state->om046_stage=NEXT_PLACEMENT;
        }
    }
    if(state->om046_stage==DISABLE_COLLISION){
        *frontier=0x2e4;
        if(!set_collision)return GH_SCENE_UNSUPPORTED;
        uint32_t original=0;
        gh_r100_init_status status=set_collision(context,"aaa209",0,&original);
        if(status){
            if(original)*frontier=original;
            return status==GH_R100_INIT_UNSUPPORTED?GH_SCENE_UNSUPPORTED:GH_SCENE_CALLEE;
        }
        state->om046_stage=FIRST_TASK;
    }
    *frontier=state->om046_stage==FIRST_TASK?0x2f8:state->om046_stage==NEXT_PLACEMENT?0x348:0x370;
    return GH_SCENE_UNSUPPORTED;
}

static void enable_om046_collision(void *context,int32_t argument)
{
    gh_r102_room *state=context;
    (void)argument;
    const gh_r102_om046_bindings *task=&state->task;
    const gh_r100_roof_bindings query={
        .pools=state->objects->bindings.pools,
        .room=task->room,
        .object_view=task->object_view,
        .context=task->context
    };
    for(;;){
        if(gh_task_threads_sleep(task->threads,1)){
            state->task_frontier=0x2d5aa0;
            return;
        }
        int broken;
        uint32_t pc=0;
        if(gh_r100_roof_is_broken_002c5168(&query,"om046",&broken,&pc)){
            state->task_frontier=pc?pc:0x2c5168;
            return;
        }
        if(!broken)continue;
        if(task->set_collision(task->context,"aaa209",1,&pc)){
            state->task_frontier=pc?pc:0x1332d0;
            return;
        }
        state->task_finished=1;
        return;
    }
}

gh_scene_status gh_r102_room_register_om046_00000898(gh_r102_room *state,
    const gh_r102_om046_bindings *bindings,uint32_t *frontier)
{
    if(!frontier)return GH_SCENE_ARGUMENT;
    *frontier=0x2f8;
    if(!state||!state->prefix_complete||!state->objects||!state->objects->bound||
       !bindings||!bindings->threads||!bindings->room||
       bindings->room!=state->objects->bindings.room_state||
       !bindings->object_view||!bindings->set_collision)
        return GH_SCENE_ARGUMENT;
    if(state->task_registered){
        if(state->task.threads!=bindings->threads||state->task.room!=bindings->room||
           state->task.scenario.bytes!=bindings->scenario.bytes||
           state->task.scenario.size!=bindings->scenario.size||
           state->task.scenario.manager!=bindings->scenario.manager||
           state->task.object_view!=bindings->object_view||
           state->task.set_collision!=bindings->set_collision||state->task.context!=bindings->context)
            return GH_SCENE_ARGUMENT;
        *frontier=0;
        return GH_SCENE_OK;
    }
    if(state->om046_stage!=FIRST_TASK)return GH_SCENE_ARGUMENT;
    state->task=*bindings;
    int32_t slot;
    uint32_t pc=0;
    const gh_scenario_exec_status status=gh_scenario_exec_002c3890(bindings->scenario,bindings->threads,
        -1,enable_om046_collision,state,0,&slot,&pc);
    if(status){
        *frontier=pc?pc:0x2c3890;
        return status==GH_SCENARIO_EXEC_MISSING?GH_SCENE_UNSUPPORTED:GH_SCENE_CALLEE;
    }
    state->task_slot=slot;
    state->task_registered=1;
    state->om046_stage=NEXT_PLACEMENT;
    *frontier=0;
    return GH_SCENE_OK;
}

gh_scene_status gh_r102_room_hide_om095_00000348(gh_r102_room *state,
    gh_r100_om_scalar_view object_view,void *context,uint32_t *frontier)
{
    if(!frontier)return GH_SCENE_ARGUMENT;
    *frontier=0x348;
    if(!state||!state->prefix_complete||!state->objects||!state->objects->bound||
       state->om046_stage<NEXT_PLACEMENT)
        return GH_SCENE_ARGUMENT;
    if(state->om046_stage==NEXT_PLACEMENT){
        const gh_om_factory *factory=state->objects;
        gh_r100_om_view found;
        const gh_r100_script_status lookup=gh_r100_find_om_0014fd98(factory->bindings.pools,
            object_view,context,"om095",&found,frontier);
        if(lookup)return lookup==GH_R100_SCRIPT_UNSUPPORTED?GH_SCENE_UNSUPPORTED:GH_SCENE_CALLEE;
        if(found.bytes){
            *frontier=0x1b7280;
            gh_actor_model *model=NULL;
            for(size_t i=0;i<factory->count;++i){
                const gh_om_object *object=&factory->objects[i];
                if(object->view.bytes!=found.bytes)continue;
                if(gh_om_factory_model_at(factory,object->handle,&model))return GH_SCENE_UNSUPPORTED;
                break;
            }
            if(!model)return GH_SCENE_UNSUPPORTED;
            for(unsigned i=0;i<model->script.mesh_count;++i){
                const gh_actor_node_packets *packet;
                if(gh_actor_model_packet_at(model,i,&packet))return GH_SCENE_CALLEE;
                uint8_t *node=packet->node;
                if(node[0x40c]==2){
                    uint32_t flags;
                    memcpy(&flags,node+0x380,sizeof(flags));
                    flags|=1;
                    memcpy(node+0x380,&flags,sizeof(flags));
                }
            }
        }
        state->om046_stage=NEXT_TASK;
    }
    *frontier=0;
    return GH_SCENE_OK;
}

static void wait_for_area11(void *context,int32_t argument)
{
    gh_r102_room *state=context;
    const gh_r102_area11_bindings *task=&state->area11;
    (void)argument;
    uint32_t flags;
    memcpy(&flags,task->room->active+4,sizeof(flags));
    if(flags&0x01000000u){
        if(!task->saved_route){state->task_frontier=0x2564;return;}
        int32_t slot;
        uint32_t frontier=0;
        if(gh_scenario_exec_002c3890(task->scenario,task->threads,-1,task->saved_route,
            task->saved_context,0,&slot,&frontier))state->task_frontier=frontier?frontier:0x2564;
        return;
    }
    uint32_t frontier=0;
    if(gh_enemy_ems_set_stamp_lock_off_00295c10(task->init,7,1,&frontier)){
        state->task_frontier=frontier;
        return;
    }
    for(;;){
        gh_at_native_unit unit;
        if(gh_scene_at_find_id_002c3118(task->areas,0x0b,&unit)){
            state->task_frontier=0x2c3118;
            return;
        }
        uint32_t hit=0;
        if(unit.bytes){
            if(unit.size<0x54){state->task_frontier=0x2c0ed0;return;}
            memcpy(&hit,unit.bytes+0x50,sizeof(hit));
        }
        if(hit&0x80000000u)break;
        if(gh_task_threads_sleep(task->threads,1)){
            state->task_frontier=0x2d5aa0;
            return;
        }
    }
    memcpy(&flags,task->room->active+4,sizeof(flags));
    flags|=0x01000000u;
    memcpy(task->room->active+4,&flags,sizeof(flags));
    gh_at_native_unit target;
    float position[3]={0};
    if(gh_scene_at_find_id_002c3118(task->areas,0x19,&target)||
       gh_scene_at_center_002c0e78(target,position)){
        state->task_frontier=0x25f8;
        return;
    }
    gh_enemy_ems *enemies=task->enemies?*task->enemies:NULL;
    gh_game_title_player *spawned;
    gh_enemy_ems_set_call set={0};
    if(gh_enemy_ems_set_002952d0(enemies,&set,0x17,&spawned,&frontier)){
        state->task_frontier=frontier?frontier:0x2952d0;
        return;
    }
    uint32_t count;
    memcpy(&count,state->work.pointer,sizeof(count));
    ++count;
    memcpy(state->work.pointer,&count,sizeof(count));
    if(gh_enemy_ems_set_stamp_goto_00295f80(enemies,task->init,0x17,position,2,0,&frontier)||
       gh_enemy_ems_start_stamp_action_00295ce8(enemies,task->init,7,&frontier)||
       gh_enemy_ems_set_stamp_lock_off_00295c10(task->init,7,0,&frontier)){
        state->task_frontier=frontier;
        return;
    }
    const float saved_position[]={-11.7f,0.1f,-4.88f};
    if(gh_enemy_set_data_position_00295380(task->room,7,saved_position)){
        state->task_frontier=0x295380;
        return;
    }
    if(gh_task_threads_sleep(task->threads,30)){
        state->task_frontier=0x2d5aa0;
        return;
    }
    if(!task->player||!task->player->alias_f0){
        state->task_frontier=0x26c0;
        return;
    }
    memcpy(position,task->player->alias_f0,sizeof(position));
    if(gh_enemy_ems_set_stamp_goto_00295f80(enemies,task->init,7,position,2,0,&frontier))
        state->task_frontier=frontier;
}

gh_scene_status gh_r102_room_register_area11_00002508(gh_r102_room *state,
    const gh_r102_area11_bindings *bindings,uint32_t *frontier)
{
    if(!frontier)return GH_SCENE_ARGUMENT;
    *frontier=0x370;
    if(!state||!state->prefix_complete||state->om046_stage!=NEXT_TASK||
       !bindings||!bindings->threads||!bindings->room||!bindings->init||!bindings->areas||
       !state->objects||state->objects->bindings.room_state!=bindings->room||
       !bindings->room->active||bindings->room->size<8)
        return GH_SCENE_ARGUMENT;
    if(state->area11_registered){
        if(state->area11.threads!=bindings->threads||state->area11.room!=bindings->room||
           state->area11.scenario.manager!=bindings->scenario.manager||
           state->area11.scenario.bytes!=bindings->scenario.bytes||
           state->area11.scenario.size!=bindings->scenario.size||
           state->area11.init!=bindings->init||state->area11.areas!=bindings->areas||
           state->area11.enemies!=bindings->enemies||state->area11.player!=bindings->player||
           state->area11.saved_route!=bindings->saved_route||state->area11.saved_context!=bindings->saved_context)
            return GH_SCENE_ARGUMENT;
        *frontier=0;
        return GH_SCENE_OK;
    }
    state->area11=*bindings;
    int32_t slot;
    uint32_t pc=0;
    const gh_scenario_exec_status status=gh_scenario_exec_002c3890(bindings->scenario,bindings->threads,
        -1,wait_for_area11,state,0,&slot,&pc);
    if(status){
        *frontier=pc?pc:0x2c3890;
        return status==GH_SCENARIO_EXEC_MISSING?GH_SCENE_UNSUPPORTED:GH_SCENE_CALLEE;
    }
    state->area11_slot=slot;
    state->area11_registered=1;
    *frontier=0;
    return GH_SCENE_OK;
}

gh_scene_status gh_r102_room_create_mount_00000488(gh_r102_room *state,uint32_t *frontier)
{
    if(!frontier)return GH_SCENE_ARGUMENT;
    *frontier=0x488;
    if(!state||!state->prefix_complete||!state->om094_complete||!state->objects||!state->objects->bound)
        return GH_SCENE_ARGUMENT;
    if(!state->mount_complete){
        gh_om_factory *factory=state->objects;
        gh_om_object *mount=NULL;
        gh_scene_placement_status status=gh_om_factory_create_0030fa98(factory,&state->mount,0x353,UINT16_MAX,&mount);
        if(status){
            *frontier=factory->frontier?factory->frontier:0x30fa98;
            return placement(status);
        }
        /* The native RoomWork sidecar retains the typed +4 reference. A
         * null result takes the original branch to 0520. */
        if(mount){
            status=gh_om_factory_initialize_runtime_mount_001a7820(factory,mount);
            if(status){
                *frontier=factory->frontier?factory->frontier:0x1a7820;
                return placement(status);
            }
            const uint32_t position[]={0x40870a3d,0x41a00000,0xc299570a};
            memcpy(mount->view.alias_f0,position,sizeof(position));
            memset(mount->view.bytes+0x100,0,12);
        }
        state->mount_complete=1;
    }
    *frontier=0;
    return GH_SCENE_OK;
}

/* 1A7EC0 seeks the first FF byte from +610, returning to slot zero
 * when the source index exceeds 15. A full original queue is a normal result. */
static void queue_mount_stamp(uint8_t *actor,uint8_t stamp)
{
    for(unsigned i=0;i<16;++i){
        const unsigned index=(unsigned)actor[0x610]+i;
        uint8_t *slot=actor+0x600+(index<16?index:0);
        if(*slot==UINT8_MAX){
            *slot=stamp;
            ++actor[0x612];
            return;
        }
    }
}

gh_scene_status gh_r102_room_mount_reinforcements_00002900(gh_r102_room *state,uint32_t *frontier)
{
    if(!frontier)return GH_SCENE_ARGUMENT;
    *frontier=0x2900;
    if(!state||!state->work.pointer||!state->mount_complete)return GH_SCENE_ARGUMENT;
    gh_om_object *mount=state->mount.object;
    if(!mount||!mount->registered||!mount->runtime_created||mount->runtime_kind!=0x353||
       !mount->view.bytes||mount->view.size<0x630||mount->view.original_vtable214!=0x4275c8)
        return GH_SCENE_UNSUPPORTED;
    uint8_t *actor=mount->view.bytes;
    for(uint8_t stamp=0x20;stamp<=0x22;++stamp)queue_mount_stamp(actor,stamp);
    uint32_t count;
    memcpy(&count,state->work.pointer,sizeof(count));
    count+=3;
    memcpy(state->work.pointer,&count,sizeof(count));
    if(actor[0x612])actor[0x615]=(uint8_t)(actor[0x615]+3u); /* Original 1A7F20. */
    *frontier=0;
    return GH_SCENE_OK;
}

gh_scene_status gh_r102_room_preload_00000520(gh_r102_room *state,uint32_t *frontier)
{
    if(!frontier)return GH_SCENE_ARGUMENT;
    *frontier=0x520;
    if(!state||!state->prefix_complete||!state->objects||!state->objects->bound)
        return GH_SCENE_ARGUMENT;
    if(!state->item3bc_complete){
        const gh_om_factory_bindings *objects=&state->objects->bindings;
        if(!objects->objects||!objects->resources||objects->resources->heap!=state->heap)
            return GH_SCENE_ARGUMENT;
        gh_object_view unused;
        uint32_t pc=0;
        const gh_object_status status=gh_game_objects_load_wait_001fe8a0(&state->item3bc_load,
            objects->objects,0x3bc,objects->resources,1,&unused,&pc);
        if(status){
            *frontier=pc?pc:0x1fe8a0;
            return status==GH_OBJECT_UNSUPPORTED?GH_SCENE_UNSUPPORTED:GH_SCENE_CALLEE;
        }
        /* The original caller ignores the returned pointer. Its acquired
         * reference remains in the same canonical holder until room exit. */
        state->item3bc_complete=1;
    }
    *frontier=0;
    return GH_SCENE_OK;
}

gh_scene_status gh_r102_room_release_prefix(gh_r102_room *state)
{
    if(!state||(state->objects&&(state->objects->runtime_pending==&state->weapon||
       state->objects->runtime_pending==&state->mount||
       state->objects->runtime_pending==&state->ticket)))return GH_SCENE_ARGUMENT;
    if(state->work.pointer&&gh_heap_release(state->heap,state->work))return GH_SCENE_CALLEE;
    *state=(gh_r102_room){0};
    return GH_SCENE_OK;
}
