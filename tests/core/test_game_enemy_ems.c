#include "r100_live_tasks.h"
#include "bgm_requests.h"
#include "game_effect_release.h"
#include "game_enemy_other_move.h"
#include "game_enemy_ems.h"
#include "game_enemy_row2.h"
#include "game_camera_envelope.h"
#include "game_world_light.h"
#include "game_ray_static.h"
#include "disc.h"
#include <fenv.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { fprintf(stderr,"line %d: %s\n",__LINE__,#expr); return 1; } } while (0)

static gh_player_target_status resolve_target(void *context,uint32_t handle,
    gh_title_player_view *out,uint32_t *frontier)
{
    gh_enemy_ems_live live;
    if(gh_enemy_ems_live_at(context,handle,&live)||!live.actor){
        *frontier=0x291510;
        return GH_PLAYER_TARGET_UNSUPPORTED;
    }
    *out=live.actor->view;
    return GH_PLAYER_TARGET_OK;
}

static gh_actor_motion_status release_effect(void *context,const gh_title_player_view *actor,
    int32_t kind,uint32_t mode,uint32_t *frontier)
{
    unsigned slot;
    return gh_game_effect_release_00306200(context,actor,kind,mode,frontier,&slot)?
        GH_ACTOR_MOTION_UNSUPPORTED:GH_ACTOR_MOTION_OK;
}

static uint8_t *load(gh_disc *disc,const char *path,size_t *size)
{
    size_t index;
    gh_disc_entry file;
    if (gh_disc_find(disc,path,&index) || gh_disc_entry_at(disc,index,&file)) return NULL;
    uint8_t *data=malloc((size_t)file.size);
    if (!data || gh_disc_read(disc,index,0,data,(size_t)file.size)) {free(data);return NULL;}
    *size=(size_t)file.size;
    return data;
}

static uint32_t word(const uint8_t *bytes)
{
    uint32_t value;
    memcpy(&value,bytes,4);
    return value;
}

typedef struct {
    const uint8_t *room;
    unsigned calls,kind;
    uint32_t flags;
    gh_soft_event_start *state;
} watch_soft_probe;

static gh_r100_script_status watch_soft(void *context,gh_soft_event_start *state,
    unsigned kind,uint32_t *frontier)
{
    watch_soft_probe *probe=context;
    ++probe->calls;
    probe->kind=kind;
    probe->flags=word(probe->room+4);
    probe->state=state;
    *frontier=0;
    return GH_R100_SCRIPT_OK;
}

int main(int argc,char **argv)
{
    CHECK(argc==2);
    gh_disc *disc;
    CHECK(!gh_disc_open(argv[1],&disc));
    size_t room_size,model_size,language_size;
    uint8_t *room_data=load(disc,"st1/r100.dat",&room_size);
    uint8_t *model_data=load(disc,"em/em02.dat",&model_size);
    uint8_t *language_data=load(disc,"etc/core_usa.bin",&language_size);
    CHECK(room_data&&model_data&&language_data);
    gh_bundle room_bundle,language;
    gh_bundle_entry ems,idd;
    CHECK(!gh_bundle_open(room_data,room_size,&room_bundle)&&!gh_bundle_find(&room_bundle,"EMS",0,&ems));
    CHECK(!gh_bundle_open(language_data,language_size,&language)&&!gh_id_find_language_data(&language,&idd));
    size_t elf_size=(size_t)gh_disc_boot_elf_size(disc);
    uint8_t *elf=malloc(elf_size);
    CHECK(elf&&!gh_disc_read_boot_elf(disc,0,elf,elf_size));

    gh_heap *heap;
    gh_room_pools pools={0};
    gh_game_init_state *init=calloc(1,sizeof(*init));
    gh_id_state *id=calloc(1,sizeof(*id));
    CHECK(init&&id&&!gh_heap_create(64*1024*1024,&heap));
    CHECK(!gh_game_room_pools_initialize_002a5d80(&pools,&room_bundle,heap));
    gh_game_init_state_initialize(init);
    uint32_t frontier;
    CHECK(!gh_hitarea_objects_construct_001fde00(init,&frontier));
    gh_boot_work work={.room=0x100,.phase569=2,.canonical_rate574=1};
    /* Original Work bit disables enemy SE in this CPU-only fixture. */
    memcpy((uint8_t *)&work+0x5e4,&(uint32_t){0x20000000},4);
    uint8_t save_bytes[GH_CORE_SAVE_BYTES],game_state=0;
    gh_core_save_view save={save_bytes,sizeof(save_bytes)};
    float rate=1;
    CHECK(!gh_core_save_system_init(save)&&!gh_game_init_002a5b18(init,&work,&game_state,&rate,save));
    gh_id_runtime *runtime;
    gh_game_cockpit *cockpit;
    CHECK(!gh_id_system_init_002acc70(id,&idd)&&!gh_id_runtime_create(id,1,&runtime));
    CHECK(!gh_id_runtime_set_path_policy(runtime,GH_ID_PATH_REFERENCE_NEAREST));
    CHECK(!gh_game_cockpit_create(runtime,id,&init->random,&cockpit)&&!gh_game_cockpit_initialize_00140c88(cockpit,save));

    gh_game_math_globals math={0};
    gh_motion_ik_globals ik={0};
    gh_camera_envelope camera={0};
    gh_enemy_node_names names={0};
    CHECK(!gh_game_math_globals_from_elf(&math,elf,elf_size)&&!gh_game_math_static_initialize_003079c8(&math,&work,1,0xffff));
    CHECK(!gh_motion_ik_globals_from_elf(&ik,elf,elf_size)&&!gh_game_camera_envelope_from_elf(&camera,elf,elf_size,&frontier));
    CHECK(!gh_enemy_node_names_from_elf(&names,elf,elf_size));
    gh_game_world_light light={0};
    gh_game_scene_tables actors={0};
    gh_collision_scroll_mutation scroll={0};
    gh_collision_pairs pairs={0};
    gh_game_collision_solids solids={0};
    CHECK(!gh_game_world_light_construct_002d6d80(&light)&&!gh_game_world_light_initialize_with_heap_002d6f40(&light,&room_bundle,heap));
    CHECK(!gh_scene_actors_0012e670(&actors,&room_bundle));
    CHECK(!gh_collision_pairs_construct_002b8ae0(&pairs,heap)&&!gh_collision_pairs_initialize_002b8b50(&pairs));
    gh_game_collision_solids_reset_00134690(&solids);

    gh_stage_slots global_slots={0},room_slots={0};
    gh_game_objects *global,*room_objects;
    int pinned;
    CHECK(!gh_game_objects_create(&global_slots,0,&global)&&!gh_game_objects_create(&room_slots,1,&room_objects));
    CHECK(!gh_stage_slots_reset(&global_slots,heap)&&!gh_stage_slots_reset(&room_slots,heap));
    CHECK(!gh_game_objects_add_pinned_001fece0(room_objects,0x202,model_data,model_size,&pinned)&&pinned);
    CHECK(!gh_game_objects_set_file_extent(room_objects,0x202,model_data,model_size));
    gh_game_prepare prepare={0};
    gh_room_state room;
    uint8_t stop=1,scenario=0,serial=255;
    gh_em_scene_placements em={.stop_flag=&stop};
    int loaded;
    CHECK(!gh_game_prepare_002a5d40(&prepare,save,0x100,&game_state)&&!gh_room_state_select_002bebc8(&prepare,0x100,0,&room));
    CHECK(!gh_em_scene_load_native_00294b98(&em,&room,&ems,&work,0,GH_EM_NUMERIC_PCSX2_CHOPZERO_REFERENCE,&loaded)&&loaded);

    gh_em_resource_bindings resource_bindings={
        .em=&em,.room=&room,.room_objects=room_objects,.work=&work,.scenario14=&scenario
    };
    size_t weapon_extent;
    uint8_t *weapon_data=load(disc,"om/om63.dat",&weapon_extent);
    CHECK(weapon_data);
    CHECK(!gh_game_objects_add_pinned_001fece0(global,0x363,weapon_data,weapon_extent,&pinned)&&pinned);
    gh_enemy2a2_node_names heavy_names={0};
    gh_enemy_ems_bindings bindings={.actor={
        .entry={.init=init,.pools=&pools,.global_objects=global,
            .room_objects=room_objects,.work=&work,.room_heap=heap,.packet_heap=heap,
            .fog_rgb=light.scalar+4,.save=save,.scenario14=&scenario,.factory_serial=&serial,
            .policy=GH_STATIC_BINARY32_REFERENCE},
        .cockpit=cockpit,.ik=&ik,.actors=&actors,.atan_coeff=camera.atan_coeff,
        .canonical_rate=&work.canonical_rate574,.solids=&solids,.default_heap=heap,.node_names=&names,
        .shapes={&pairs,&math,GH_STATIC_BINARY32_REFERENCE},
        .numeric_policy=GH_EM_NUMERIC_PCSX2_CHOPZERO_REFERENCE,.weapon_scroll=&scroll
    },.names_2a2=&heavy_names};
    gh_enemy_ems *owner=NULL;
    gh_em_resource_prefix prefix={0};
    int detail;
    CHECK(!gh_em_resources_00294ca0(&prefix,&resource_bindings)&&prefix.stage==9&&stop==0);
    CHECK(!gh_enemy_ems_spawn_00294ca0(&owner,&prefix,&room,&bindings,&frontier,&detail)&&!owner&&serial==255);

    size_t heavy_size,aux_size;
    uint8_t *heavy=load(disc,"em/ema2.dat",&heavy_size);
    uint8_t *aux=load(disc,"em/em20.dat",&aux_size);
    CHECK(heavy&&aux);
    CHECK(!gh_game_objects_add_pinned_001fece0(room_objects,0x2a2,heavy,heavy_size,&pinned)&&pinned);
    CHECK(!gh_game_objects_set_file_extent(room_objects,0x2a2,heavy,heavy_size));
    CHECK(!gh_game_objects_add_pinned_001fece0(room_objects,0x220,aux,aux_size,&pinned)&&pinned);
    CHECK(!gh_game_objects_set_file_extent(room_objects,0x220,aux,aux_size));

    prefix=(gh_em_resource_prefix){0};
    CHECK(gh_em_resources_00294ca0(&prefix,&resource_bindings)==GH_PLACE_UNSUPPORTED);
    CHECK(prefix.stage==5&&prefix.prefix_complete&&prefix.spawn_cursor==0&&prefix.frontier==0x2905d0);
    const size_t baseline=gh_heap_used(heap);
    CHECK(!fesetround(FE_TOWARDZERO));
    CHECK(gh_enemy_ems_spawn_00294ca0(&owner,&prefix,&room,&bindings,&frontier,&detail)==GH_PLACE_UNSUPPORTED);
    CHECK(frontier==0x14b730&&prefix.spawn_cursor==5&&serial==4);
    CHECK(word(init->enemies+0x508)==5&&solids.unit_count==5);
    CHECK(!gh_enemy2a2_node_names_from_elf(&heavy_names,elf,elf_size));
    /* A saved-health write while the initializer is suspended must be read
     * by the original caller after its virtual call returns. */
    memcpy(room.active+12+5*24+22,&(int16_t){11},2);
    CHECK(gh_enemy_ems_spawn_00294ca0(&owner,&prefix,&room,&bindings,&frontier,&detail)==GH_PLACE_BOUNDS);
    CHECK(frontier==0x31ccf0&&prefix.spawn_cursor==13&&serial==9);
    const gh_room_vector *paused_registry;
    CHECK(!gh_room_registry_view(&pools,0x754c58,&paused_registry)&&paused_registry->count==10);
    uint32_t pending_weapon=word((uint8_t*)paused_registry->allocation.pointer+9*4);
    CHECK(!gh_enemy_ems_held_weapon(owner,0,pending_weapon));
    CHECK(!gh_game_objects_set_file_extent(global,0x363,weapon_data,weapon_extent));
    gh_scene_placement_status completion=gh_enemy_ems_spawn_00294ca0(&owner,&prefix,&room,&bindings,&frontier,&detail);
    if(completion)fprintf(stderr,"actual stop %08x detail %d row %u\n",frontier,detail,prefix.spawn_cursor);
    CHECK(!completion&&prefix.stage==9&&!frontier&&!detail&&prefix.spawn_cursor==room.active[8]&&owner&&serial==11);
    CHECK(word(init->enemies+0x508)==11&&word(init->enemies+0x50c)==1&&solids.unit_count==11);
    const unsigned numbers[]={0,3,4,5,6,8,10,11,18};
    const gh_room_vector *v;
    CHECK(!gh_room_registry_view(&pools,0x754c58,&v)&&v->count==12);
    for(unsigned i=0;i<9;i++){
        uint32_t handle=word((uint8_t*)v->allocation.pointer+i*4);
        gh_game_title_player *object=gh_enemy_ems_lookup(owner,handle);
        if(!object||object!=init->enemy_actors[i]||object->view.bytes[0x640]!=numbers[i]) fprintf(stderr,"slot %u handle %u actor %p expected %p stamp %u expected %u\n",i,handle,(void*)object,init->enemy_actors[i],object?object->view.bytes[0x640]:255,numbers[i]);
        CHECK(object&&object==init->enemy_actors[i]&&object->view.bytes[0x640]==numbers[i]);
    }
    gh_enemy_ems_live first_live,second_live,heavy_live;
    uint32_t first_handle=word((uint8_t*)v->allocation.pointer);
    uint32_t next_handle=word((uint8_t*)v->allocation.pointer+4);
    CHECK(!gh_enemy_ems_live_at(owner,first_handle,&first_live)&&first_live.actor==init->enemy_actors[0]&&
        first_live.enemy202&&first_live.model==&first_live.enemy202->entry.initialize.model.model&&
        first_live.move202&&first_live.physical_row==0&&first_live.shadow);
    CHECK(!gh_enemy_ems_live_at(owner,next_handle,&second_live)&&second_live.actor==init->enemy_actors[1]&&
        second_live.move202&&second_live.move202!=first_live.move202&&second_live.physical_row==2);
    CHECK(!gh_enemy_ems_live_at(owner,word((uint8_t*)v->allocation.pointer+4*4),&heavy_live)&&
        heavy_live.actor==init->enemy_actors[4]&&heavy_live.model&&heavy_live.shadow&&!heavy_live.move202);
    uint32_t reverse=0;
    CHECK(!gh_enemy_ems_handle_for_actor(owner,heavy_live.actor,&reverse)&&
        reverse==word((uint8_t*)v->allocation.pointer+4*4));
    gh_game_title_player copy=*heavy_live.actor;
    CHECK(!gh_enemy_ems_handle_for_actor(owner,&copy,&reverse)&&!reverse);
    const unsigned appearance5_rows[]={6,7,8};
    const uint32_t appearance5_flags[]={0x48800000,0x04000000,0x04000000};
    gh_object_view model_source;
    CHECK(!gh_game_objects_find(room_objects,0x202,&model_source));
    for(unsigned i=0;i<3;i++){
        gh_enemy_request request;
        int eligible;
        int16_t saved;
        CHECK(!gh_enemy_request_from_room_00294ca0(&room,appearance5_rows[i],
            bindings.actor.numeric_policy,&request,&eligible,&saved,&frontier)&&eligible);
        gh_enemy_initial_motion_selection motion;
        CHECK(!gh_enemy202_appearance5_select_00203b3c(&request,&model_source,&motion));
        CHECK(motion.motion_offset==word(model_data+0x3fc)&&motion.sequence_offset==word(model_data+0x400));
        CHECK(motion.substate==1&&motion.phase_detail==3);
        gh_game_title_player *object=init->enemy_actors[i+5];
        const uint8_t *p=object->view.bytes;
        CHECK(!memcmp(p+0x1630,request.position_xyz,12)&&word(p+0x1640)==request.yaw_bits);
        CHECK(word(p+0x1644)==appearance5_flags[i]&&word(p+0x564)==(i==2?0x203:0x200));
        CHECK(word(p+0x2f4)==0x03000100&&p[0x548]==74);
        CHECK((word(p+0x16d0)&0x200u)==(i==0?0x200u:0));
    }
    gh_enemy_request wrong={.object_kind=0x202,.dependency_kind=0x200,.appearance=5,.entry_number=8,.flags=0x04000000};
    gh_enemy_initial_motion_selection untouched={.motion_offset=42,.phase_detail=99};
    CHECK(gh_enemy202_appearance5_select_00203b3c(&wrong,&model_source,&untouched)==GH_ACTOR_MODEL_UNSUPPORTED);
    CHECK(untouched.motion_offset==42&&untouched.phase_detail==99);
    gh_game_title_player *heavy_actor=NULL,*missing=NULL;
    CHECK(!gh_enemy_manager_get_00290868(init,6,&heavy_actor)&&heavy_actor==init->enemy_actors[4]);
    CHECK((word(heavy_actor->view.bytes+0x548)>>16)==11);
    CHECK(!gh_enemy_manager_get_00290868(init,14,&missing)&&!missing);
    CHECK(!gh_enemy_manager_get_00290868(init,255,&missing)&&!missing);
    const float escape[]={8.13f,.13f,19.06f,2.0f};
    uint32_t w=word(heavy_actor->view.bytes+0x159c);
    CHECK(!gh_enemy2a2_set_escape_position_00289038(heavy_actor,escape));
    CHECK(!memcmp(heavy_actor->view.bytes+0x1590,escape,12)&&word(heavy_actor->view.bytes+0x159c)==w);
    CHECK(gh_enemy2a2_set_escape_position_00289038(init->enemy_actors[0],escape)==GH_ACTOR_MOTION_UNSUPPORTED);
    gh_enemy_request residual;
    int eligible;
    int16_t saved;
    CHECK(!gh_enemy_request_from_room_00294ca0(&room,13,bindings.actor.numeric_policy,&residual,&eligible,&saved,&frontier));
    CHECK(eligible&&residual.entry_number==18&&residual.object_kind==0x202&&residual.dependency_kind==0x202&&residual.appearance==9);
    CHECK(residual.flags==0x14000004&&residual.dependency_flags==0);
    gh_enemy_initial_motion_selection appearance9;
    CHECK(!gh_enemy202_appearance9_select_0020393c(&residual,&model_source,&appearance9));
    CHECK(appearance9.motion_offset==word(model_data+0x648)&&appearance9.sequence_offset==word(model_data+0x64c));
    CHECK(appearance9.substate==1&&!appearance9.phase_detail);
    gh_game_title_player *paused=NULL;
    CHECK(!gh_enemy_manager_get_00290868(init,18,&paused)&&paused==init->enemy_actors[8]);
    const uint8_t *paused_bytes=paused->view.bytes;
    CHECK(word(paused_bytes+0x2f4)==0x00000100&&word(paused_bytes+0x1644)==residual.flags);
    CHECK(!memcmp(paused_bytes+0x1630,residual.position_xyz,12)&&word(paused_bytes+0x1640)==residual.yaw_bits);
    uint32_t armed_handle=word((uint8_t*)v->allocation.pointer+8*4);
    uint32_t weapon_handle=word((uint8_t*)v->allocation.pointer+9*4);
    CHECK(gh_enemy_ems_lookup(owner,armed_handle)==paused);
    const gh_enemy_weapon_init *held=gh_enemy_ems_held_weapon(owner,armed_handle,weapon_handle);
    CHECK(held&&weapon_handle==pending_weapon);
    gh_enemy_ems_live weapon_live;
    CHECK(!gh_enemy_ems_live_at(owner,weapon_handle,&weapon_live)&&weapon_live.held_weapon&&
        weapon_live.physical_row==13&&weapon_live.actor==&held->weapon->object&&
        weapon_live.model==&held->model&&!weapon_live.move202&&!weapon_live.shadow);
    uint32_t heavy_handle=word((uint8_t*)v->allocation.pointer+4*4);
    CHECK(!gh_enemy_ems_live_at(owner,heavy_handle,&heavy_live)&&heavy_live.enemy2a2&&heavy_live.actor==heavy_actor);
    CHECK(weapon_live.weapon==held);
    gh_enemy_other_move heavy_move={0},weapon_move={0};
    _Alignas(16) uint8_t player_bytes[GH_TITLE_PLAYER_BYTES]={0};
    gh_title_player_view player;
    CHECK(!gh_title_player_construct_00107e40(player_bytes,sizeof(player_bytes),&player));
    gh_title_player_view *target=&player;
    work.canonical_rate574=1.0f;
    CHECK(gh_enemy2a2_move_002856c0(&heavy_move,heavy_live.enemy2a2,target)==GH_ACTOR_MOTION_UNSUPPORTED);
    CHECK(heavy_move.frontier==0x291010&&heavy_move.stage==4&&!heavy_move.original_returned);
    CHECK(word(heavy_actor->view.bytes+0x618)!=0);
    CHECK(gh_enemy2a2_move_002856c0(&heavy_move,heavy_live.enemy2a2,target)==GH_ACTOR_MOTION_UNSUPPORTED&&heavy_move.stage==4);
    gh_player_target_bindings targets={.pools=&pools,.player=target,.resolve=resolve_target,
        .context=owner,.elf=elf,.elf_bytes=elf_size};
    CHECK(!gh_enemy_other_move_bind_targets(&heavy_move,&targets));
    CHECK(gh_enemy2a2_move_002856c0(&heavy_move,heavy_live.enemy2a2,target)==GH_ACTOR_MOTION_UNSUPPORTED);
    CHECK(heavy_move.frontier==0x2866f8&&heavy_move.stage==5&&!heavy_move.original_returned);
    CHECK(word(heavy_actor->view.bytes+0x15c0)==0x43160000u);
    CHECK(!memcmp(first_live.actor->view.bytes+0x1680,first_live.actor->view.alias_f0,12));
    CHECK(gh_enemy363_move_001c9860(&weapon_move,weapon_live.weapon,NULL,NULL)==GH_ACTOR_MOTION_UNSUPPORTED);
    CHECK(weapon_move.frontier==0x306200&&weapon_move.stage==5&&!weapon_move.original_returned);
    CHECK(held->weapon->object.view.bytes[0x531]==0xff&&!held->weapon->object.view.bytes[0x2f6]);
    CHECK(word(held->hit[1].bytes+0x4c)==0&&word(held->weapon->object.view.bytes+0x5a8)==0x3f800000u);
    CHECK(gh_enemy363_move_001c9860(&weapon_move,weapon_live.weapon,NULL,NULL)==GH_ACTOR_MOTION_UNSUPPORTED&&weapon_move.stage==5);
    /* Supply the real 306200 consumer over buffers initialized from the ISO. */
    size_t core_bytes;
    uint8_t *core=load(disc,"etc/core.dat",&core_bytes);
    uint8_t *esp=calloc(1,0x100180),*emitters=calloc(1,0x26060);
    gh_game_effects *effects=calloc(1,sizeof(*effects));
    CHECK(core&&esp&&emitters&&effects);
    CHECK(!gh_game_effects_initialize_00305a68(effects,esp,0x100180,emitters,0x26060,core,core_bytes,&room_bundle));
    gh_effect_release_bindings effect_bindings={.effects=effects,.esp=esp,.esp_bytes=0x100180,
        .emitters=emitters,.emitter_bytes=0x26060};
    gh_object_opacity_environment opacity={.init=init,.current_player=target,.player_binding_known=1};
    CHECK(!gh_enemy_other_move_bind_2a2_services(&heavy_move,&opacity,release_effect,&effect_bindings,NULL,NULL));
    CHECK(!gh_enemy2a2_move_002856c0(&heavy_move,heavy_live.enemy2a2,target));
    CHECK(heavy_move.original_returned&&!heavy_move.frontier&&heavy_move.stage==20);
    const uint8_t *heavy_raw=heavy_actor->view.bytes;
    CHECK(heavy_raw[0x2f6]==1&&word(heavy_raw+0x430)==0x3f800000u&&(word(heavy_raw+0x15b0)&2u));
    CHECK(!memcmp(heavy_raw+0x490,heavy_actor->view.alias_f0,12));
    CHECK(!gh_enemy2a2_move_002856c0(&heavy_move,heavy_live.enemy2a2,target)&&word(heavy_raw+0x430)==0x3f800000u);
    gh_enemy_other_move next_heavy={0};
    CHECK(!gh_enemy_other_move_bind_targets(&next_heavy,&targets));
    CHECK(!gh_enemy_other_move_bind_2a2_services(&next_heavy,&opacity,release_effect,&effect_bindings,NULL,NULL));
    CHECK(!gh_enemy2a2_move_002856c0(&next_heavy,heavy_live.enemy2a2,target));
    CHECK(next_heavy.original_returned&&word(heavy_raw+0x430)==0x40000000u);
    CHECK(!gh_enemy363_move_001c9860(&weapon_move,weapon_live.weapon,release_effect,&effect_bindings));
    CHECK(weapon_move.original_returned&&!weapon_move.frontier&&held->weapon->object.view.bytes[0x2f6]==1);
    CHECK(!memcmp(held->weapon->object.view.bytes+0x490,held->weapon->object.view.alias_f0,12));
    CHECK(!memcmp(held->weapon->object.view.bytes+0x5d0,held->weapon->object.view.bytes+0x100,12));
    gh_enemy_other_move next_weapon={0};
    CHECK(!gh_enemy363_move_001c9860(&next_weapon,weapon_live.weapon,release_effect,&effect_bindings));
    CHECK(next_weapon.original_returned&&!next_weapon.frontier&&held->weapon->object.view.bytes[0x2f6]==1);
    free(effects);free(emitters);free(esp);free(core);
    CHECK(held&&held->parent==&paused->view&&held->weapon->resource.bytes==weapon_data);
    CHECK(held->model.external_parent==held->parent_model&&held->parent_model->root.bytes==paused_bytes);
    CHECK(!word(paused_bytes+0x6ec)&&held->attach_stage==5&&held->hit_owned==3);
    CHECK(gh_enemy_ems_held_weapon(owner,0,weapon_handle)==held);
    CHECK(gh_enemy_ems_held_weapon(owner,armed_handle,0)==held);
    CHECK(!gh_enemy_ems_held_weapon(owner,armed_handle-1,weapon_handle));
    gh_game_mt random=init->random;
    CHECK(!gh_enemy_ems_spawn_00294ca0(&owner,&prefix,&room,&bindings,&frontier,&detail));
    CHECK(!frontier&&!detail&&serial==11&&!memcmp(&random,&init->random,sizeof(random)));
    CHECK(!gh_enemy_request_from_room_00294ca0(&room,14,bindings.actor.numeric_policy,&residual,&eligible,&saved,&frontier));
    CHECK(eligible&&residual.entry_number==19&&residual.object_kind==0x202&&
        residual.dependency_kind==0x203&&residual.appearance==0&&residual.flags==0x04000000);
    for(unsigned i=0;i<2;i++){
        CHECK(!gh_enemy_request_from_room_00294ca0(&room,14+i,bindings.actor.numeric_policy,
            &residual,&eligible,&saved,&frontier)&&eligible&&residual.entry_number==19+i);
        gh_enemy_initial_motion_selection selection;
        CHECK(!gh_enemy202_appearance0_select_0020393c(&residual,&model_source,&selection));
        size_t directory=i?0x98:0x648;
        CHECK(selection.motion_offset==word(model_data+directory)&&selection.sequence_offset==word(model_data+directory+4));
        gh_game_title_player *object=gh_enemy_ems_lookup(owner,word((uint8_t*)v->allocation.pointer+(10+i)*4));
        CHECK(object&&object==init->enemy_actors[9+i]&&object->view.bytes[0x640]==19+i);
        CHECK(word(object->view.bytes+0x2f4)==0x100&&object->view.bytes[0x548]==74);
    }
    /* Original BGM checks RoomSave death, not HP or the actor suspend bit.
     * The actual R100 opening roster has no special BGM override. */
    int32_t bgm_event;
    CHECK(!gh_bgm_enemy_override_002d0cd0(init,&room,&bgm_event)&&bgm_event==-2);
    CHECK(gh_bgm_enemy_override_002d0cd0(init,NULL,&bgm_event)==GH_BGM_UNSUPPORTED);
    gh_game_title_player *music_actors[]={init->enemy_actors[0],init->enemy_actors[1],init->enemy_actors[2]};
    const uint16_t music_kinds[]={0x220,0x223,0x209};
    uint8_t *music_rows[3];
    uint8_t saved_death[3];
    uint16_t saved_kind[3];
    for(unsigned i=0;i<3;i++){
        uint8_t *actor=music_actors[i]->view.bytes;
        uint16_t stamp;
        memcpy(&stamp,actor+0x640,2);
        unsigned j=0;
        while(j<room.active[8]&&room.active[12+j*24+0x13]!=stamp)++j;
        CHECK(j<room.active[8]);
        music_rows[i]=room.active+12+j*24;
        saved_death[i]=music_rows[i][0x14];
        music_rows[i][0x14]=0;
        memcpy(&saved_kind[i],actor+0x2fe,2);
        memcpy(actor+0x2fe,&music_kinds[i],2);
    }
    const int32_t music_priority[]={1,0x2e,0x1b};
    for(unsigned i=3;i;i--){
        CHECK(!gh_bgm_enemy_override_002d0cd0(init,&room,&bgm_event)&&bgm_event==music_priority[i-1]);
        music_rows[i-1][0x14]=1;
    }
    CHECK(!gh_bgm_enemy_override_002d0cd0(init,&room,&bgm_event)&&bgm_event==-2);
    for(unsigned i=0;i<3;i++){
        music_rows[i][0x14]=saved_death[i];
        memcpy(music_actors[i]->view.bytes+0x2fe,&saved_kind[i],2);
    }
    uint32_t flags250[12], flags5a0[12];
    for (unsigned i=0; i<12; ++i) {
        gh_game_title_player *object=i==11?weapon_live.actor:init->enemy_actors[i];
        CHECK(object && object->view.size>=0x5a4);
        flags250[i]=word(object->view.bytes+0x250)&~0x8000u;
        flags5a0[i]=word(object->view.bytes+0x5a0)&~8u;
        memcpy(object->view.bytes+0x250,&flags250[i],4);
        memcpy(object->view.bytes+0x5a0,&flags5a0[i],4);
    }
    CHECK(!gh_enemy_ems_set_suspend_002948e8(owner,init,1,&frontier)&&!frontier);
    for (unsigned i=0; i<12; ++i) {
        gh_game_title_player *object=i==11?weapon_live.actor:init->enemy_actors[i];
        CHECK(word(object->view.bytes+0x250)==(flags250[i]|0x8000u)&&
            word(object->view.bytes+0x5a0)==(flags5a0[i]|8u));
    }
    CHECK(!gh_enemy_ems_unsuspend_002948e8(owner,init,&frontier)&&!frontier);
    for (unsigned i=0; i<12; ++i) {
        gh_game_title_player *object=i==11?weapon_live.actor:init->enemy_actors[i];
        CHECK(word(object->view.bytes+0x250)==flags250[i]&&word(object->view.bytes+0x5a0)==flags5a0[i]);
    }
    /* Actual REL2490 with native Scenario fiber and the same eleven-actor EMS owner. */
    gh_scene_at_manager at={0};
    CHECK(!gh_scene_at_initialize_units_002c0f90(&at,&room_bundle,heap,0x100,1));
    gh_at_native_unit at1={0},at2={0},trigger={0},destinations[2]={{0}},unit;
    CHECK(!gh_scene_at_first_unit_002c30c0(&at,&unit));
    while(unit.bytes){
        uint16_t id;memcpy(&id,unit.bytes+0x36,2);
        if(id==1)at1=unit;
        if(id==2)at2=unit;
        if(id==3)trigger=unit;
        if(id==5||id==6)destinations[id-5]=unit;
        CHECK(!gh_scene_at_next_unit_002c30c8(&at,unit,&unit));
    }
    CHECK(at1.bytes&&at2.bytes&&trigger.bytes&&destinations[0].bytes&&destinations[1].bytes);
    uint32_t saved_count=4,flag=0x80000000u;
    memcpy(room.active+0x1610,&saved_count,4);
    flag|=word(trigger.bytes+0x50);
    memcpy(trigger.bytes+0x50,&flag,4);
    gh_r100_init room_runtime={0};
    CHECK(!gh_heap_allocate(heap,12,16,&room_runtime.work));
    room_runtime.heap=heap;
    uint32_t baseline_count=7;
    memcpy(room_runtime.work.pointer,&baseline_count,4);
    gh_task_record32 records[16]={0};
    for(unsigned i=0;i<16;i++)records[i].thread_id=-1;
    gh_task_manager task_manager={.records=records,.count=16,.capacity=16,.current_index=-1};
    gh_task_threads *scenario_threads;
    CHECK(!gh_task_threads_create(&task_manager,&scenario_threads));
    uint8_t scenario_bytes[0x114]={0};
    gh_r100_live_tasks tasks={0};
    watch_soft_probe soft_probe={.room=room.active};
    const gh_r100_live_bindings live_b={.scenario={scenario_bytes,sizeof(scenario_bytes),&task_manager},
        .threads=scenario_threads,.room_init=&room_runtime,.room=&room,.at=&at,
        .init=init,.work=&work,.player=target,.enemies=&owner,.context=&soft_probe,
        .math=&math,.elf=elf,.elf_size=elf_size,.scene_tables=&actors,.start_soft_event=watch_soft};
    /* REL2920's real AT1 and original EMS stamps 19/20, with their canonical RoomSave rows. */
    uint8_t *watched_rows[2]={room.active+12+24*14,room.active+12+24*15};
    for(unsigned i=0;i<2;i++){
        gh_game_title_player *watched=NULL;
        CHECK(watched_rows[i][0x13]==19+i&&!watched_rows[i][0x14]);
        CHECK(!gh_enemy_manager_get_00290868(init,(uint8_t)(19+i),&watched)&&watched==init->enemy_actors[9+i]);
    }
    const uint32_t saved_at1_flags=word(at1.bytes+0x50), saved_room_flags=word(room.active+4);
    const uint32_t at1_hit=saved_at1_flags|0x80000000u;
    memcpy(at1.bytes+0x50,&at1_hit,4);
    CHECK(!gh_r100_live_register(&tasks,&live_b,0x2920,-1,&frontier));
    for(unsigned i=0;i<2;i++)CHECK(gh_tasks_run_matching_002d52a0(&task_manager).status==GH_TASK_OK);
    CHECK(tasks.tasks[0].entered&&!tasks.tasks[0].finished&&!tasks.tasks[0].status&&!tasks.tasks[0].frontier);
    watched_rows[0][0x14]=1;
    CHECK(gh_tasks_run_matching_002d52a0(&task_manager).status==GH_TASK_OK);
    CHECK(!tasks.tasks[0].finished&&!tasks.tasks[0].status&&!tasks.tasks[0].frontier);
    gh_game_title_player *stamp0=init->enemy_actors[0];
    CHECK(stamp0->view.original_kind4ac==0x42c7f0);
    float enemy_position[4],zero_position[4],saved_player_position[4];
    CHECK(!gh_r100_enemy_position_00295ab8(init,0,&math,enemy_position,&frontier)&&!frontier);
    CHECK(!memcmp(enemy_position,stamp0->view.alias_f0,16));
    CHECK(!gh_r100_enemy_position_00295ab8(init,UINT8_MAX,&math,zero_position,&frontier)&&!frontier);
    CHECK(!memcmp(zero_position,math.zero_vector,16));
    CHECK(gh_r100_enemy_position_00295ab8(init,UINT8_MAX,NULL,zero_position,&frontier)==GH_R100_SCRIPT_UNSUPPORTED&&frontier==0x295ab8);
    uint8_t *stamp0_save=room.active+12;
    CHECK(!stamp0_save[0x13]&&!stamp0_save[0x14]);
    const uint32_t saved_player_yaw=word(target->bytes+0x104), saved_stamp0_flags=word(stamp0->view.bytes+0x16d0);
    const uint32_t inactive_stamp0=saved_stamp0_flags&~2u, active_stamp0=saved_stamp0_flags|2u;
    memcpy(saved_player_position,target->alias_f0,16);
    memcpy(target->alias_f0,enemy_position,16);
    memcpy(target->bytes+0x104,&(uint32_t){0},4);
    memcpy(stamp0->view.bytes+0x16d0,&inactive_stamp0,4);

    watched_rows[1][0x14]=1;
    CHECK(gh_tasks_run_matching_002d52a0(&task_manager).status==GH_TASK_OK);
    CHECK(!tasks.tasks[0].finished&&tasks.tasks[0].status==GH_R100_SCRIPT_UNSUPPORTED&&tasks.tasks[0].frontier==0x2c3ed8);
    CHECK(soft_probe.calls==1&&!soft_probe.kind&&soft_probe.state==&tasks.tasks[0].soft_start);
    CHECK(soft_probe.flags==(saved_room_flags|0x20000000u)&&word(room.active+4)==soft_probe.flags);
    CHECK(!gh_task_threads_cancel(scenario_threads,tasks.tasks[0].slot));

    for(unsigned i=0;i<2;i++){
        /* Active and dead stamp0 each consume the AT1 flag and exit without starting an event. */
        memcpy(stamp0->view.bytes+0x16d0,i?&inactive_stamp0:&active_stamp0,4);
        stamp0_save[0x14]=(uint8_t)i;
        memcpy(room.active+4,&saved_room_flags,4);
        tasks.tasks[0]=(gh_r100_live_task){0};
        CHECK(!gh_r100_live_register(&tasks,&live_b,0x2920,-1,&frontier));
        for(unsigned visit=0;visit<2;visit++)CHECK(gh_tasks_run_matching_002d52a0(&task_manager).status==GH_TASK_OK);
        CHECK(tasks.tasks[0].finished&&!tasks.tasks[0].status&&!tasks.tasks[0].frontier);
        CHECK(soft_probe.calls==1&&word(room.active+4)==(saved_room_flags|0x20000000u));
        CHECK(!gh_task_threads_cancel(scenario_threads,tasks.tasks[0].slot));
    }
    stamp0_save[0x14]=0;
    memcpy(room.active+4,&saved_room_flags,4);
    memcpy(stamp0->view.bytes+0x16d0,&saved_stamp0_flags,4);
    memcpy(target->alias_f0,saved_player_position,16);
    memcpy(target->bytes+0x104,&saved_player_yaw,4);
    watched_rows[0][0x14]=watched_rows[1][0x14]=0;
    memcpy(at1.bytes+0x50,&saved_at1_flags,4);

    /* REL2650 admits one surviving member of the actual AT2 group. */
    uint8_t *at2_rows[3]={room.active+12+24*2,room.active+12+24*3,room.active+12+24*4};
    for(unsigned i=0;i<3;i++){
        gh_game_title_player *watched=NULL;
        CHECK(at2_rows[i][0x13]==3+i&&!at2_rows[i][0x14]);
        CHECK(!gh_enemy_manager_get_00290868(init,(uint8_t)(3+i),&watched)&&watched==init->enemy_actors[1+i]);
    }
    const uint32_t saved_at2_flags=word(at2.bytes+0x50), at2_hit=saved_at2_flags|0x80000000u;
    memcpy(at2.bytes+0x50,&at2_hit,4);
    CHECK(!gh_r100_live_register(&tasks,&live_b,0x2650,-1,&frontier));
    for(unsigned i=0;i<2;i++)CHECK(gh_tasks_run_matching_002d52a0(&task_manager).status==GH_TASK_OK);
    CHECK(tasks.tasks[1].entered&&!tasks.tasks[1].finished&&!tasks.tasks[1].status&&!tasks.tasks[1].frontier);
    at2_rows[0][0x14]=1;
    CHECK(gh_tasks_run_matching_002d52a0(&task_manager).status==GH_TASK_OK);
    CHECK(!tasks.tasks[1].finished&&!tasks.tasks[1].status&&!tasks.tasks[1].frontier);
    at2_rows[1][0x14]=1;
    CHECK(gh_tasks_run_matching_002d52a0(&task_manager).status==GH_TASK_OK);
    CHECK(!tasks.tasks[1].finished&&tasks.tasks[1].status==GH_R100_SCRIPT_UNSUPPORTED&&tasks.tasks[1].frontier==0x2cb8a0);
    CHECK(soft_probe.calls==2&&!soft_probe.kind&&soft_probe.state==&tasks.tasks[1].soft_start);
    CHECK(soft_probe.flags==(saved_room_flags|0x10000000u)&&word(room.active+4)==soft_probe.flags);
    CHECK(!gh_task_threads_cancel(scenario_threads,tasks.tasks[1].slot));
    at2_rows[0][0x14]=at2_rows[1][0x14]=0;
    memcpy(at2.bytes+0x50,&saved_at2_flags,4);
    memcpy(room.active+4,&saved_room_flags,4);
    CHECK(!gh_r100_live_register(&tasks,&live_b,0x2490,-1,&frontier));
    for(unsigned i=0;i<2;i++)CHECK(gh_tasks_run_matching_002d52a0(&task_manager).status==GH_TASK_OK);
    CHECK(tasks.tasks[2].entered&&!tasks.tasks[2].finished&&!tasks.tasks[2].frontier);
    CHECK(word(room_runtime.work.pointer)==7&&serial==11);
    uint16_t prior_saved_hp=1;
    memcpy(room.active+12+26*24+22,&prior_saved_hp,2);
    saved_count=5;
    memcpy(room.active+0x1610,&saved_count,4);
    CHECK(gh_tasks_run_matching_002d52a0(&task_manager).status==GH_TASK_OK);
    if(tasks.tasks[2].status)fprintf(stderr,"AT3 status%d pc%08x stage%u\n",tasks.tasks[2].status,tasks.tasks[2].frontier,tasks.tasks[2].at3_stage);
    CHECK(tasks.tasks[2].finished&&tasks.tasks[2].at3_stage==4&&!tasks.tasks[2].frontier&&serial==13);
    CHECK(word(room_runtime.work.pointer)==9&&word(room.active+0x1610)==5);
    for(unsigned i=0;i<2;i++){
        const uint8_t *record=room.active+12+24*(26+i);
        CHECK(record[0x13]==31+i&&word(record+8)==0x04000000u&&!record[0x14]);
        gh_game_title_player *spawned;
        CHECK(!gh_enemy_manager_get_00290868(init,(uint8_t)(31+i),&spawned)&&spawned);
        const uint8_t *raw=spawned->view.bytes;
        int16_t hp;memcpy(&hp,raw+0x54a,2);
        CHECK(hp>1); /* Single-SetEm does not restore RoomSave HP. */
        CHECK(word(raw+0x16d0)&0x20000000u);
        CHECK(word(raw+0x16c0)==0x497423feu&&!word(raw+0x16c4));
        CHECK(!(word(raw+0x250)&0x8000u)&&!(word(raw+0x5a0)&8u));
        const uint8_t *shape=destinations[i].bytes+4;
        CHECK((destinations[i].bytes[0x34]&1u)&&(shape[1]==1||shape[1]==2));
        float y,actual;
        memcpy(&y,shape+4,4);memcpy(&actual,raw+0x16b4,4);
        CHECK(fabs((double)actual-((double)y+0.4))<0.00001);
        for(unsigned axis=0;axis<2;axis++){
            double expected=0;
            for(unsigned vertex=0;vertex<(shape[1]==1?4u:1u);vertex++){
                float value;memcpy(&value,shape+0x10+axis*4+vertex*8,4);expected+=value;
            }
            if(shape[1]==1)expected/=4;
            memcpy(&actual,raw+0x16b0+axis*8,4);
            CHECK(fabs((double)actual-expected)<0.00002);
        }
        gh_enemy_ems_set_call duplicate={0};gh_game_title_player *none=spawned;
        CHECK(!gh_enemy_ems_set_002952d0(owner,&duplicate,(uint8_t)(31+i),&none,&frontier)&&!none);
    }
    CHECK(gh_tasks_run_matching_002d52a0(&task_manager).status==GH_TASK_OK&&word(room_runtime.work.pointer)==9);
    CHECK(!gh_task_threads_destroy(scenario_threads)&&!gh_r100_init_release(&room_runtime));
    CHECK(!gh_scene_at_units_release(&at));
    CHECK(!gh_enemy_ems_release_native(&owner,&frontier)&&!owner&&!frontier);
    CHECK(!init->enemy_top&&!init->enemy_last&&gh_collision_solid_units_empty(&solids)&&!gh_collision_pair_shapes_have_live(&pairs));
    CHECK(gh_heap_used(heap)==baseline&&!fesetround(FE_TONEAREST));
    CHECK(!gh_collision_scroll_mutation_release(&scroll));
    gh_scene_ray_triangle_state_release(&actors);
    CHECK(!gh_game_object_pool_release_001fdfa8(init,&frontier)&&!gh_collision_pairs_release(&pairs));
    CHECK(!gh_game_cockpit_destroy(cockpit));
    gh_id_runtime_destroy(runtime);
    gh_game_objects_destroy(room_objects);
    gh_game_objects_destroy(global);
    CHECK(!gh_game_world_light_release_presets(&light)&&!gh_game_room_pools_release(&pools)&&!gh_heap_used(heap)&&!gh_heap_destroy(heap));
    gh_disc_close(disc);
    free(weapon_data);free(aux);free(heavy);free(init);free(id);free(elf);free(language_data);free(model_data);free(room_data);
    puts("PASS actual R100 BGM roster/death/priority and eleven EMS actors, attached weapon, retry, full 2A2/held363 Moves, AT1 live/absent GetPos, AT1/AT2 survivor gates and kind0, and AT3 SetEm31/32, Goto, unsuspend, duplicate gates and scoped release");
    return 0;
}
