#include "../core/game_om300_retire.h"
#include "../core/game_object_sound.h"
#include "../core/game_player_square_frame.h"
#include "../core/game_player_attack_hitarea.h"
#include "../core/game_enemy_move.h"
#include "game_services.h"
#include "../core/game_effects_frame.h"
#include "../core/game_enemy_2ad.h"
#include "../core/game_scene_message_create.h"
#include "../core/game_cockpit.h"
#include "../core/game_objects.h"
#include "../core/game_prepare.h"
#include "../core/game_room_state.h"
#include "../core/game_room_pack.h"
#include "../core/game_room_pools.h"
#include "../core/game_camera.h"
#include "../rooms/r100.h"
#include "../rooms/r100_init.h"
#include "../core/scene_at_units.h"
#include "../core/game_om_factory.h"
#include "../core/game_om_frame.h"
#include "../core/game_om382.h"
#include "../core/game_player_factory.h"
#include "../core/game_actor_scroll_links.h"
#include "../core/game_ray_static.h"
#include "../core/game_player_init.h"
#include "../core/game_actor_model.h"
#include "../core/frame_state.h"
#include <fenv.h>
#include "../core/game_collision_solid_units.h"
#include "../core/game_player_init_tail.h"
#include "../core/game_player_springs.h"
#include "../core/game_player_spawn.h"
#include "../core/game_actor_matrix.h"
#include "../core/game_om_initialize.h"
#include "../core/game_scene_at_geometry.h"
#include "../core/sdk_rotation_trig.h"
#include "../core/game_hitarea_shapes.h"
#include "../core/game_actor_motion_step.h"
#include <math.h>
#include "../core/game_actor_motion_call.h"
#include "../core/game_actor_motion_solve.h"
#include "../core/game_effect_release.h"
#include "../rooms/r100_roof_task.h"
#include "../rooms/r100_counter_task.h"
#include "../rooms/r100_live_tasks.h"
#include "../rooms/r100_init_tail.h"
#include "../rooms/r003_event.h"
#include "../rooms/r003_flags.h"
#include "../rooms/r105_room.h"
#include "../rooms/r102_room.h"
#include "../core/game_cockpit_tutorial.h"
#include "../core/game_player_abort.h"
#include "../core/game_om3eb.h"
#include "../core/game_enemy_request.h"
#include "../core/game_enemy_row2.h"
#include "../core/game_enemy_resources.h"
#include "../core/game_enemy_ems.h"
#include "../core/game_enemy_retire.h"
#include "../core/game_room_death_dispatch.h"
#include "../core/game_event_player_reset.h"
#include "../core/game_scenario_end_soft_event.h"
#include "../core/game_event_accessory_move.h"
#include "../core/game_event_actor_bust.h"
#include "../core/game_event_pendulum.h"
#include "../core/game_player_frame.h"
#include "../core/game_player_idle_input.h"
#include "../core/game_effect_create.h"
#include "../core/game_esp14.h"
#include "../core/game_player_door_probe.h"
#include "../core/game_action_button_set.h"
#include "../core/game_player_child_update.h"
#include "../core/game_cloth_wind.h"
#include "../core/game_actor_attached_sound.h"
#include "../core/game_actor_aux.h"
#include "../core/game_actor_model_light.h"
#include "../core/game_player_targets.h"
#include "../core/game_enemy_motion.h"
#include "../core/game_event_resource.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
typedef struct gh_services_esp14_slot {
    gh_game_services *services;
    struct gh_services_esp14_slot *retired_next;
    gh_esp14_instance instance;
    gh_effect_create_call nested_emitter;
    gh_actor_render_call render;
    uint32_t serial;
} gh_services_esp14_slot;

struct gh_game_services {
 gh_game_services_bindings b;gh_game_cockpit *cockpit;
 gh_game_objects *objects,*room_objects;gh_game_prepare prepare;
 gh_task_record32 scenario_records[16];gh_task_manager scenario;gh_task_threads *scenario_threads;
 uint8_t scenario_scalar[0x114]; /*canonical5FEB00scalarspan;managerhasitsowntypedowner*/
 gh_room_state room_state;gh_room_pack room_pack;gh_room_pools pools;gh_game_pl_camera camera;
 gh_room_module module;gh_collision_pairs collision_pairs;unsigned case3_stage;int sound_load_started;
 uint8_t case3_reloaded_player;
 gh_game_effects effects;gh_scene_filters filters;
 gh_game_world_light world_light;gh_game_model_packets model_packets;
 gh_game_collision_solids collision_solids;gh_game_scene_tables scene_tables;gh_game_title_player title_player;
 gh_game_checkpoint checkpoint;gh_game_scene_ui scene_ui;gh_scene_at_manager scene_at;gh_game_action_buttons action_buttons;
 gh_em_scene_placements em_placements;gh_om_scene_placements om_placements;gh_acd_scene_binding acd;
 gh_em_resource_prefix em_resources;gh_em_resource_bindings em_resource_bindings;
 gh_enemy_ems *enemies;gh_enemy_ems_bindings enemy_bindings;
 gh_enemy_node_names enemy_nodes;gh_enemy2a2_node_names enemy2a2_nodes;
 gh_object_opacity_environment enemy_opacity;
 gh_actor_render_call *enemy_renders;
 gh_actor_render_call devil_render;
 gh_game_devil_camera devil_camera;
 gh_game_scene_step step;gh_r020_title*title;gh_scene_hook scenario_exit,scenario_callback104;
 gh_id_base results_base;
 gh_id_entry *results_entries[34];
 uint16_t results_state,results_phase,results_clear_variant,results_save_target;
 uint32_t results_time_bonus,results_continue_bonus,results_npc_fine,results_kill_bonus,results_total_bonus;
 gh_r207_allocation results_idd;
 uint32_t results_dvd_job;
 uint8_t results_pair_bound;
 gh_scene_meshes meshes;
 gh_room_death_dispatch scene_death;
 gh_room_death_bindings scene_death_bindings;
 gh_r100_init r100;
 gh_r100_roof_task r100_roof;
 gh_r100_scripts r100_scripts;gh_r100_counter_task r100_counter;gh_r100_live_tasks r100_live;
 gh_soft_event_end r100_soft_end;
 gh_r003_event r003;gh_soft_event_end r003_soft_end;
 gh_r105_room r105;
#ifdef GH_NATIVE_STARTUP_QA
 uint8_t qa_r105_exit_fixture,qa_r105_exit_pose_applied;
#endif
 gh_r102_room r102;
 gh_r207_allocation r200_work;
 gh_object_resource_load r200_load;
 gh_om_runtime_create r200_catapult;
 uint8_t r200_init_stage,r200_entry_started;
 uint32_t r200_task_frontier;
 gh_r207_allocation r104_work;
 gh_om_runtime_create r104_reward;
 uint32_t r104_task_frontier;
 uint8_t r104_initialized;
 gh_r207_allocation r103_work;uint32_t r103_task_frontier;uint8_t r103_initialized;
 gh_at_native_unit r103_message_areas[2];
 gh_r207_allocation r101_work;uint8_t r101_init_stage,r101_gate_task_active,r101_side_door_pose_stage,r101_side_door_entry_stage;uint32_t r101_task_frontier;
 gh_om376_paper r105_paper;
 gh_enemy2ad_action r105_scenery_action;
 gh_motion_sound_request r105_scenery_task_sound;
 uint32_t r105_scenery_handle,r105_scenery_task_frontier;
 gh_scene_step_status r105_scenery_task_status;
 gh_r100_init_tail r100_tail;gh_r100_init_tail_bindings r100_tail_bindings;
 gh_cockpit_tutorial tutorial;gh_cockpit_tutorial_action tutorial_action;gh_object_resource_load tutorial_load;
 gh_om_runtime_create tutorial_create[4];gh_om_object *tutorial_objects[4];
 gh_game_player_abort soft_player_abort;
 uint32_t player_grab_attacker;
 uint8_t player_grab_escape_stamp;
 gh_event_player_reset event_player_reset;gh_event_clear_bindings event_player_prefix;
 gh_player_abort_bindings event_player_abort;gh_event_weapon_slot player_weapon,event_paper_slot;
 gh_event_weapon_attachment event_paper_attachment;
 gh_om_factory om_factory;
 gh_om_runtime_create roulette_projectile_create;
 gh_om300_full_retirement om_retirement;gh_object_sound_binding om_retire_sound;
 struct {
  gh_om_object *object;
  gh_actor_model *model;
  gh_hitarea_object give;
  gh_actor_motion motion;
  gh_actor_motion_call idle_motion;
  uint32_t handle;
  int shape_index;
  uint8_t stage,give_created,move_stage;
  float flight_previous[3];
 } player_prop;
 gh_enemy_manager_remove enemy_removal;
 gh_enemy_full_retirement enemy_retirement;
 gh_object_sound_binding enemy_retire_sound;
 const uint8_t *retiring_enemy_actor;
 unsigned retiring_enemy_row;
 gh_om_frame_state *om_frames;size_t om_frames_capacity;
 gh_actor_render_call player_render,child_render,*om_renders;size_t om_renders_capacity;
 gh_om_initializer_bindings om_initializer;
 gh_hitarea_shape_bindings om_shapes;gh_scene_dynamic_owner dynamic_objects;
 gh_game_player_factory player_factory;
 gh_object_resource_bindings object_resources;
 gh_event_actor_roster *event_actor_roster;
 gh_event_actor_extra *event_actor_extra;
 struct {
  gh_event_accessory_move move;
  gh_shadow_call shadow;
  gh_event_pendulum_call pendulum;
 } event_actor_move[GH_EVENT_ACTOR_ROSTER_CAPACITY + 1];
 gh_event_pendulum_call event_accessory_cloth[3];
 gh_actor_render_call event_actor_render[GH_EVENT_ACTOR_ROSTER_CAPACITY + 1];
 gh_actor_render_call event_accessory_render[3];
 gh_esp14_provider model_effect;
 gh_services_esp14_slot *esp14_slots[1024], *esp14_retired;
 size_t esp14_limit;
 gh_game_player_init player_init;gh_actor_model player_model;
 gh_game_player_init_tail player_tail;gh_game_player_child player_child;
 gh_player_child_attachment player_attachment;
 gh_player_child_tail child_tail;gh_game_player_child *work5f8;
 gh_game_player_spawn player_spawn;
 gh_player_square_frame player_square;
 gh_player_square_frame_bindings player_square_bindings;
 gh_player_target player_square_target;
 uint8_t player_square_camera_pose_done,player_interact_camera_pending;
 gh_actor_motion player_motion;
 gh_actor_motion_call player_motion_call;
 gh_game_player_frame player_frame;gh_effect_release_bindings player_frame_effects;
 gh_player_idle_input player_idle_input;gh_player_idle_input_bindings player_idle_bindings;
 gh_effect_create_call player_effect_create;
 uint8_t player_damage_vibration[32][16];
 gh_effect_create_call om_break_effect;
 const gh_actor_model *om_break_effect_owner;
 gh_player_door_probe player_door;gh_player_door_bindings player_door_bindings;gh_bundle player_door_room;
 gh_child_wind_update child_update;
 gh_actor_attached_sound player_sound,enemy_sound,om_sound;gh_player_tail_target_slot player_target_644;
 gh_motion_sound_request ema2_near_sound;
 gh_game_update_diagnostic update_diagnostic;
 gh_motion_solve player_motion_solve;
 uint64_t ui_resource_epoch;
 gh_game_door door;uint8_t system_menu_enabled;uint32_t room_texture_channel;int room_texture_ready;
 gh_stage_object_init old_children,old_cockpit;void *old_context;
 int owns_children,owns_textures,owns_skills,stage_attempted;
 gh_game_children *created_children;gh_stage_textures *created_textures;
 uint32_t cockpit_pool_end,cockpit_jobs_end;int cockpit_snapshot;
 uint32_t frontier;int detail,frontier_module_code;gh_game_services_status status;uint32_t main_word4;
 gh_game_main_record main_records[32];
 uint8_t scene_buffer_a[0x100180],scene_buffer_b[0x26060];
};
static uint32_t u32(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static void w32(uint8_t*p,uint32_t n){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(n>>(8*i));}
static gh_game_services_status fail(gh_game_services*s,gh_game_services_status e,uint32_t entry,int detail){if(s->frontier!=entry)s->frontier_module_code=0;s->frontier=entry;s->detail=detail;s->status=e;return e;}
static int in_pack(const void*p,uintptr_t base,size_t bytes){uintptr_t v=(uintptr_t)p;return p&&v>=base&&v-base<bytes;}
static int detach_bundle(gh_bundle_entry*e,uintptr_t base,size_t bytes){if(!in_pack(e->bytes,base,bytes))return 0;*e=(gh_bundle_entry){0};return 1;}
static void detach_table(gh_scene_table_view*v,uintptr_t base,size_t bytes){if(in_pack(v->bytes,base,bytes))*v=(gh_scene_table_view){0};}
static int detach_effect(gh_effect_view*v,uintptr_t base,size_t bytes){if(!in_pack(v->bytes,base,bytes))return 0;*v=(gh_effect_view){0};return 1;}
static gh_game_services_status release_r104_room_init(gh_game_services *s)
{
 if(s->r104_work.pointer&&gh_heap_release(s->b.assets->heap,s->r104_work)!=GH_MEM_OK)
  return fail(s,GH_SERVICES_ASSET,0x30ee08,0);
 s->r104_work=(gh_r207_allocation){0};
 s->r104_reward=(gh_om_runtime_create){0};
 s->r104_task_frontier=0;
 s->r104_initialized=0;
 return GH_SERVICES_OK;
}

static gh_game_services_status release_r103_room_init(gh_game_services *s)
{
 if(s->r103_work.pointer&&gh_heap_release(s->b.assets->heap,s->r103_work)!=GH_MEM_OK)
  return fail(s,GH_SERVICES_ASSET,0x30ee08,0);
 s->r103_work=(gh_r207_allocation){0};s->r103_task_frontier=0;s->r103_initialized=0;
 memset(s->r103_message_areas,0,sizeof(s->r103_message_areas));
 return GH_SERVICES_OK;
}

static gh_game_services_status release_r101_room_init(gh_game_services *s)
{
 if(s->r101_work.pointer&&gh_heap_release(s->b.assets->heap,s->r101_work)!=GH_MEM_OK)
  return fail(s,GH_SERVICES_ASSET,0x30ee08,0);
 s->r101_work=(gh_r207_allocation){0};s->r101_init_stage=0;s->r101_gate_task_active=0;s->r101_side_door_pose_stage=0;s->r101_side_door_entry_stage=0;s->r101_task_frontier=0;
 return GH_SERVICES_OK;
}
static gh_game_services_status release_r200_room_init(gh_game_services *s)
{
 if(s->r200_work.pointer&&gh_heap_release(s->b.assets->heap,s->r200_work)!=GH_MEM_OK)
  return fail(s,GH_SERVICES_ASSET,0x30ee08,0);
 s->r200_work=(gh_r207_allocation){0};
 s->r200_load=(gh_object_resource_load){0};
 s->r200_catapult=(gh_om_runtime_create){0};
 s->r200_init_stage=s->r200_entry_started=0;
 s->r200_task_frontier=0;
 return GH_SERVICES_OK;
}
static gh_game_services_status finalize_room_lifetime(gh_game_services*s){
 if(!s->room_pack.allocation.pointer)return GH_SERVICES_OK;
 if(s->enemies)return fail(s,GH_SERVICES_FRONTIER,0x290770,0);
 /* Actual2A6210 resets parentroomheap754220 immediately BEFORE31CC48.
  * Native allocations are split by owner: retire old bounded views here,
  * never destroy the host heap which also owns persistent/defaultheap data. */
 if(s->module.bound||s->title||s->pools.initialized||s->scenario_exit.kind!=GH_SCENE_HOOK_ABSENT)
  return fail(s,GH_SERVICES_BUSY,0x2a94f0,0);
 for(unsigned i=0;i<16;i++)if(s->scenario_records[i].running)return fail(s,GH_SERVICES_BUSY,0x2d59f0,0);
 for(size_t i=0;i<s->pools.object_slots;i++)if(s->pools.objects[i].object)return fail(s,GH_SERVICES_FRONTIER,0x30f8b0,0);
 if(s->meshes.descriptors)return fail(s,GH_SERVICES_FRONTIER,0x30f8b0,0);
 gh_scene_placement_status oms=gh_om_factory_release_retired(&s->om_factory);
 if(oms)return fail(s,GH_SERVICES_BUSY,0x30f8b0,oms);
 free(s->om_frames);s->om_frames=NULL;s->om_frames_capacity=0;
 free(s->om_renders);s->om_renders=NULL;s->om_renders_capacity=0;
 memset(s->tutorial_create,0,sizeof(s->tutorial_create));
 memset(s->tutorial_objects,0,sizeof(s->tutorial_objects));
 s->tutorial_action=(gh_cockpit_tutorial_action){0};
 s->tutorial_load=(gh_object_resource_load){0};
 gh_om_scene_construct_002a7308(&s->om_placements);
 if(s->r102.work.pointer){
  const gh_scene_status status=gh_r102_room_release_prefix(&s->r102);
  if(status)return fail(s,GH_SERVICES_BUSY,0x2a94f0,status);
 }
 if(s->r003.work.pointer){
  gh_r003_status status=gh_r003_event_release(&s->r003);
  if(status)return fail(s,GH_SERVICES_BUSY,0x2a94f0,status);
  s->r003_soft_end=(gh_soft_event_end){0};
 }
 if(s->r105.work.pointer){
  const gh_scene_status status=gh_r105_room_release(&s->r105);
  if(status)return fail(s,GH_SERVICES_BUSY,0x2a94f0,status);
 }
 if(gh_r100_init_release(&s->r100))return fail(s,GH_SERVICES_ASSET,0x208,0);
 if(release_r101_room_init(s)||release_r103_room_init(s)||release_r104_room_init(s)||
    release_r200_room_init(s))return s->status;
 s->r105_paper=(gh_om376_paper){0};
 {gh_tutorial_status e=gh_cockpit_tutorial_release(&s->tutorial);if(e)return fail(s,GH_SERVICES_ASSET,s->tutorial.frontier?s->tutorial.frontier:0x147188,e);}
 if(s->b.id->pool_used!=s->cockpit_pool_end||s->b.id->job_count!=s->cockpit_jobs_end)return fail(s,GH_SERVICES_BUSY,0x2aa078,0);
 if(s->b.sound_scene->job||s->b.sound_scene->loading.pointer)return fail(s,GH_SERVICES_BUSY,0x2cabe8,0);
 uintptr_t base=(uintptr_t)s->room_pack.allocation.pointer;size_t bytes=s->room_pack.allocated_bytes;
 if(!bytes||base>UINTPTR_MAX-bytes)return fail(s,GH_SERVICES_ASSET,0x2a94f0,0);
 gh_stage_status ce=gh_stage_texture_forget_source_range(s->b.stage->textures,(const void*)base,bytes);
 if(ce)return fail(s,GH_SERVICES_ASSET,0x2a94f0,ce);
 for(unsigned i=0;i<5;i++){gh_font_slot*f=&s->b.font->slots[i];int changed=0;
  if(in_pack(f->mst,base,bytes)){f->mst=NULL;f->mst_size=0;changed=1;}
  if(in_pack(f->msa,base,bytes)){f->msa=NULL;f->msa_size=0;changed=1;}if(changed)f->bound=0;
 }
 int changed=detach_bundle(&s->b.menu->data,base,bytes);changed|=detach_bundle(&s->b.menu->texture_data,base,bytes);if(changed)s->b.menu->tables_bound=0;
 for(unsigned i=0;i<32;i++){changed=detach_bundle(&s->b.id->pair[i].data,base,bytes);changed|=detach_bundle(&s->b.id->pair[i].texture_data,base,bytes);if(changed)s->b.id->pair[i].bound=0;}
 if(detach_bundle(&s->scene_ui.room_data,base,bytes))s->scene_ui.ready=0;
 detach_table(&s->scene_tables.environment,base,bytes);detach_table(&s->scene_tables.scroll_texture,base,bytes);
 detach_bundle(&s->scene_tables.akt,base,bytes);detach_bundle(&s->scene_tables.akg,base,bytes);
 for(unsigned i=0;i<8;i++)detach_table(&s->scene_tables.actor_links[i].view,base,bytes);
 changed=0;for(unsigned i=0;i<4;i++){changed|=detach_bundle(&s->scene_at.headers[i],base,bytes);
  if(in_pack(s->scene_at.units[i].bytes,base,bytes)){s->scene_at.units[i]=(gh_at_unit_view){0};changed=1;}}
 for(unsigned i=0;i<8;i++)if(in_pack(s->scene_at.jump_slots[i].bytes,base,bytes)){s->scene_at.jump_slots[i]=(gh_at_unit_view){0};changed=1;}
 if(changed)s->scene_at.ready=0;
 detach_bundle(&s->em_placements.file,base,bytes);detach_bundle(&s->em_placements.source_entries,base,bytes);
 if(in_pack(s->acd.records,base,bytes)){s->acd.records=NULL;s->acd.record_bytes=0;}
 s->em_resources=(gh_em_resource_prefix){0};
 s->em_resource_bindings=(gh_em_resource_bindings){0};
 s->enemy_bindings=(gh_enemy_ems_bindings){0};
 gh_scene_nearcheck_release(&s->step.nearcheck);
 changed=detach_effect(&s->effects.ear,base,bytes)|detach_effect(&s->effects.ssd,base,bytes);
 for(unsigned i=0;i<64;i++){changed|=detach_effect(&s->effects.resources[i].tbl0,base,bytes);changed|=detach_effect(&s->effects.resources[i].tbl1,base,bytes);changed|=detach_effect(&s->effects.emitter_entries[i],base,bytes);}
 for(unsigned i=0;i<1250;i++)changed|=detach_effect(&s->effects.emd_tables[i],base,bytes);if(changed)s->effects.ready=0;
 for(unsigned i=0;i<8;i++)detach_bundle(&s->world_light.slots[i],base,bytes);
 detach_bundle(&s->world_light.area,base,bytes);detach_bundle(&s->world_light.table,base,bytes);s->world_light.ready=0;
 detach_bundle(&s->b.sound_scene->seh,base,bytes);detach_bundle(&s->b.sound_scene->bmh,base,bytes);detach_bundle(&s->b.sound_scene->ts,base,bytes);
 if(s->b.motion_camera&&in_pack(s->b.motion_camera->motion.bytes,base,bytes)){
  s->b.motion_camera->motion=(gh_motion_view){0};
  s->b.motion_camera->move_stage=0;
 }
 for(unsigned i=0;i<sizeof(s->r100_live.tasks)/sizeof(s->r100_live.tasks[0]);i++)
  if(in_pack(s->r100_live.tasks[i].camera_start.bindings.room.bytes,base,bytes))
   s->r100_live.tasks[i].camera_start=(gh_room_camera_start){0};
 if(s->room_pack.room!=0x20){
  /* Room actors have already run their original destructors. The native
   * continuations hold borrowed state for those particular lifetimes. */
  if(s->player_factory.complete||s->player_model.ready||s->work5f8||s->player_attachment.attached||
     gh_game_player_reset_room_cursors(&s->player_init,&s->player_tail,&s->player_spawn,
      &s->title_player,&s->player_child))return fail(s,GH_SERVICES_BUSY,0x30f8b0,0);
  s->player_motion=(gh_actor_motion){0};
  s->player_motion_call=(gh_actor_motion_call){0};
  s->player_motion_solve=(gh_motion_solve){0};
  s->player_frame=(gh_game_player_frame){0};
  s->player_idle_input=(gh_player_idle_input){0};
  s->player_idle_bindings=(gh_player_idle_input_bindings){0};
  s->player_square=(gh_player_square_frame){0};
  s->player_square_bindings=(gh_player_square_frame_bindings){0};
  s->player_square_target=(gh_player_target){0};
  s->player_square_camera_pose_done=0;
  s->player_interact_camera_pending=0;
  s->player_target_644=(gh_player_tail_target_slot){0};
  s->player_door=(gh_player_door_probe){0};
  s->player_door_bindings=(gh_player_door_bindings){0};
  s->player_door_room=(gh_bundle){0};
  s->player_effect_create=(gh_effect_create_call){0};
  s->player_render=(gh_actor_render_call){0};
  s->child_render=(gh_actor_render_call){0};
  s->child_update=(gh_child_wind_update){0};
 }
 gh_room_pack_status pe=gh_game_room_pack_release(&s->room_pack);if(pe)return fail(s,GH_SERVICES_ASSET,0x2a94f0,pe);
 s->room_texture_ready=0;s->sound_load_started=0;++s->ui_resource_epoch;
 /* Defaultheap TitlePlayer is deliberately retained: its exact destructor
  * belongs to its own source lifecycle, not this roomheap reset. */
 return GH_SERVICES_OK;
}
static gh_scene_status room_init_native(void*p,uint32_t*frontier){
 gh_game_services*s=p;
 if(s->b.id->pool_used!=s->cockpit_pool_end||s->b.id->job_count!=s->cockpit_jobs_end){if(frontier)*frontier=0x2a9ea0;return GH_SCENE_CALLEE;}
 gh_r020_title_bindings b={.work=s->b.work,.save=s->b.save,.random=&s->b.init->random,
  .dvd=s->b.assets->dvd,.room_heap=s->b.assets->heap,.id=s->b.id,.runtime=*s->b.runtime,
  .bgm=s->b.bgm_requests,.fade=s->b.fade,.threads=s->b.threads,
  .room_jump={s->b.assets->room_jump.pointer,s->b.assets->room_jump_size,0},.scenario_exit=&s->scenario_exit,
  .display=s->b.display,.reference_profile=s->b.title_reference_profile};
 gh_r020_status e=gh_r020_title_init(&b,&s->title);if(e){gh_r020_title_view v;if(s->title&&!gh_r020_title_inspect(s->title,&v)){if(frontier)*frontier=v.frontier;s->frontier=v.frontier;s->frontier_module_code=v.frontier_relative;}else {if(frontier)*frontier=0x1a0;s->frontier=0x1a0;s->frontier_module_code=1;}return e==GH_R020_UNSUPPORTED?GH_SCENE_UNSUPPORTED:GH_SCENE_CALLEE;}if(frontier)*frontier=0;return GH_SCENE_OK;
}
static gh_scene_status room_move_native(void*p,uint32_t*frontier){
 gh_game_services*s=p;if(!s->title){if(frontier)*frontier=0x220;return GH_SCENE_UNSUPPORTED;}
 gh_r020_status e=gh_r020_title_move(s->title,&s->b.title_callbacks);gh_r020_title_view v;
 if(!gh_r020_title_inspect(s->title,&v)){if(frontier)*frontier=v.frontier;s->frontier=v.frontier;s->frontier_module_code=v.frontier_relative;}else {if(frontier)*frontier=0x650;s->frontier=0x650;s->frontier_module_code=1;}
 return e?(e==GH_R020_UNSUPPORTED?GH_SCENE_UNSUPPORTED:GH_SCENE_CALLEE):GH_SCENE_OK;
}
static uint32_t room_results_random(void *context)
{
 uint32_t value=0;
 (void)gh_game_mt_next_0030f348(context,&value);
 return value;
}

static gh_scene_status room_results_release(void *context,uint32_t *frontier)
{
 gh_game_services *s=context;
 if(s->results_base.entries){
  int original,freed;
  if(gh_id_base_release_002aa078(*s->b.runtime,&s->results_base,&original,&freed)){
   *frontier=0x2aa078;return GH_SCENE_CALLEE;
  }
  gh_id_base_clear_002a9df8(&s->results_base);
 }
 if(s->results_pair_bound){
  s->b.id->pair[0x10]=(gh_id_pair){0};
  s->results_pair_bound=0;
 }
 if(s->results_idd.pointer){
  if(gh_dvd_check(s->b.assets->dvd,s->results_dvd_job)){
   *frontier=0x201160;return GH_SCENE_CALLEE;
  }
  if(gh_heap_release(s->b.assets->heap,s->results_idd)){
   *frontier=0x2a9680;return GH_SCENE_CALLEE;
  }
  s->results_idd=(gh_r207_allocation){0};
 }
 memset(s->results_entries,0,sizeof(s->results_entries));
 s->results_dvd_job=0;
 *frontier=0;
 return GH_SCENE_OK;
}

/* Original R021 Init REL01A0 invokes 03E8 to prepare game-clear IDD. */
static gh_scene_status room_results_init(void *context,uint32_t *frontier)
{
 static const char *const paths[]={
  "id/idgameclear.idd","id/local/US/idgameclear_US.idd","id/local/EN/idgameclear_EN.idd",
  "id/local/FR/idgameclear_FR.idd","id/local/GE/idgameclear_GE.idd","id/local/IT/idgameclear_IT.idd",
  "id/local/SP/idgameclear_SP.idd"
 };
 gh_game_services *s=context;
 if(s->b.work->room!=0x21||s->module.init_offset!=0x1a0||s->results_idd.pointer||
    s->b.id->pair[0x10].bound)return GH_SCENE_ARGUMENT;
 gh_dvd_jobs *dvd=s->b.assets->dvd;
 size_t index=0;
 const unsigned language=(unsigned)s->b.work->language;
 const char *path=language<sizeof(paths)/sizeof(paths[0])?paths[language]:paths[0];
 *frontier=0x2acfa8;
 gh_disc_error found=gh_disc_find(dvd->disc,path,&index);
 if(found==GH_DISC_NOT_FOUND){path=paths[0];found=gh_disc_find(dvd->disc,path,&index);}
 if(found)return GH_SCENE_CALLEE;
 gh_disc_entry entry;
 if(gh_disc_entry_at(dvd->disc,index,&entry)||entry.size>SIZE_MAX)return GH_SCENE_CALLEE;
 *frontier=0x200cb0;
 s->results_dvd_job=gh_dvd_read_alloc(dvd,path,&s->results_idd,s->b.assets->heap,0,0,0,0);
 if(!s->results_dvd_job)return GH_SCENE_CALLEE;
 *frontier=0x201160;
 while(gh_dvd_check(dvd,s->results_dvd_job)){
  if(gh_dvd_pump(dvd)!=GH_DVD_OK)return GH_SCENE_CALLEE;
 }
 if(dvd->failed_id==s->results_dvd_job)return GH_SCENE_CALLEE;
 const gh_bundle_entry image={.bytes=s->results_idd.pointer,.size=(size_t)entry.size};
 *frontier=0x2accf8;
 if(gh_id_set_data_002accf8(s->b.id,0x10,&image))return GH_SCENE_CALLEE;
 s->results_pair_bound=1;
 gh_id_base_clear_002a9df8(&s->results_base);
 int original;
 *frontier=0x2a9ea0;
 gh_id_error status=gh_id_base_initialize_002a9ea0(*s->b.runtime,&s->results_base,0x10,0,
     room_results_random,&s->b.init->random,&original);
 if(status||original!=1){
  if(s->results_base.first_unported)*frontier=s->results_base.first_unported;
  return status==GH_ID_UNSUPPORTED?GH_SCENE_UNSUPPORTED:GH_SCENE_CALLEE;
 }
 *frontier=0x2abbe8;
 if(gh_id_base_restart_anim(&s->results_base,room_results_random,&s->b.init->random))
  return GH_SCENE_CALLEE;
 *frontier=0x2aa410;
 for(unsigned i=0;i<34;++i){
  int fallback;
  if(gh_id_base_get_work_002aa410(*s->b.runtime,&s->results_base,(int)i,
       &s->results_entries[i],&fallback)||fallback)return GH_SCENE_CALLEE;
 }
 for(unsigned i=1;i<=6;++i)s->results_entries[i]->pos_x=532.0f;
 const unsigned difficulty=gh_core_save_difficulty(s->b.save);
 if(difficulty==2){
  s->results_entries[30]->color_base=0x802e2e2eu;
  s->results_entries[31]->color_base=0x802e2e2eu;
  s->results_entries[32]->color_base=0x802e2e2eu;
 }else if(!difficulty){
  s->results_entries[32]->color_base=0x802e2e2eu;
  s->results_entries[33]->color_base=0x802e2e2eu;
 }
 uint8_t *work=(uint8_t *)s->b.work;
 if(u32(work+0x594)&0x02000000u)w32(s->b.save.bytes+0x20,999999u);
 s->results_entries[29]->seed=u32(s->b.save.bytes+0x20);
 /* REL0274 tests the persisted Results bit before setting it again.
  * Re-entry after the event starts the Save/Map branch at state 9. */
 s->results_state=(u32(work+0x5e4)&0x40000u)?9:0;
 s->results_phase=0;
 if(s->results_state==9){
  if(!s->b.fade||!s->b.display)return GH_SCENE_ARGUMENT;
  const gh_screen_fade_context fade={s->b.display->screen_width,
   s->b.display->gs_y_origin,u32(work+0x564),u32(work+0x5e0)};
  if(gh_screen_fade_start_00307208(s->b.fade,&fade,0,10,0,0xff000000u,0xff000000u,0)){
   *frontier=0x307208;return GH_SCENE_UNSUPPORTED;
  }
 }
 s->scenario_exit=(gh_scene_hook){GH_SCENE_HOOK_NATIVE,room_results_release,s};
 w32(work+0x5d8,u32(work+0x5d8)|0xf8040000u);
 w32(work+0x5e0,u32(work+0x5e0)|0xfe200000u);
 w32(work+0x5e4,u32(work+0x5e4)|0x40000u);
 *frontier=0;
 return GH_SCENE_OK;
}

/* REL0BF8 counts stage HMS from the existing WorldTime+4 owner.
 * For the original R104 predecessor, bonus thresholds are REL2D4C. */
static void room_results_time_award(gh_game_services *s)
{
 const uint32_t ticks=s->prepare.global_0061a994;
 const uint32_t minutes=s->results_entries[9]->seed+60*s->results_entries[10]->seed;
 const uint32_t payout=minutes<30?10000u:minutes<60?5000u:minutes<90?3000u:0;
 s->results_time_bonus=payout*s->results_clear_variant;
 w32(s->b.save.bytes+0xbbc,u32(s->b.save.bytes+0xbbc)+ticks);
}

static void room_results_count_time(gh_game_services *s)
{
 const uint32_t seconds=s->prepare.global_0061a994/30;
 const uint32_t goal[]={seconds%60,seconds/60%60,seconds/3600};
 int completed=1;
 for(unsigned unit=0;unit<3;++unit){
  gh_id_entry *number=s->results_entries[8+unit];
  number->seed+=5;
  if(number->seed>goal[unit])number->seed=goal[unit];
  else if(goal[unit])completed=0;
 }
 if(completed){
  room_results_time_award(s);
  s->results_phase=3;
 }
}

static void room_results_slide_heading(gh_game_services *s,unsigned entry)
{
 gh_id_entry *heading=s->results_entries[entry];
 float origin;
 memcpy(&origin,heading->source+8,sizeof(origin));
 heading->pos_x-=80.0f;
 if(heading->pos_x<origin){
  heading->pos_x=origin;
  s->results_phase=2;
 }
}

static gh_scene_status room_results_effect(gh_game_services *s,unsigned program,uint32_t *frontier)
{
 const gh_r020_move_callbacks *callbacks=&s->b.title_callbacks;
 *frontier=0x2cb8a0;
 if(!callbacks->program_se||callbacks->program_se(callbacks->context,0,program,frontier))
  return GH_SCENE_UNSUPPORTED;
 return GH_SCENE_OK;
}

/* REL2228 advances by the largest increment that remains strictly
 * below the target, then plays the count sound before testing completion. */
static gh_scene_status room_results_tally_award(gh_game_services *s,unsigned entry,uint32_t award,
    int fine,unsigned next_phase,uint32_t *frontier)
{
 gh_id_entry *number=s->results_entries[entry];
 static const unsigned steps[]={100000,10000,1000,100,10};
 unsigned increment=1;
 for(unsigned i=0;i<sizeof(steps)/sizeof(steps[0]);++i){
  if(number->seed+steps[i]<award){increment=steps[i];break;}
 }
 const gh_scene_status status=room_results_effect(s,0x165,frontier);
 if(status)return status;
 number->seed+=increment;
 if(number->seed>award){
  number->seed=fine?0u-award:award;
  s->results_phase=(uint16_t)next_phase;
 }
 return GH_SCENE_OK;
}

static gh_scene_status room_results_count_continue_or_npc(gh_game_services *s,uint32_t *frontier)
{
 const int npc=s->results_state==3;
 const unsigned count_entry=npc?14:12;
 const unsigned award_entry=count_entry-1;
 const uint8_t *work=(const uint8_t *)s->b.work;
 if(work[0x5b7]!=1||(u32(work+0x1a0)&0x10000000u)){
  *frontier=npc?0x19a8:0x16b4;
  s->frontier_module_code=1;
  return GH_SCENE_UNSUPPORTED;
 }
 uint32_t *award=npc?&s->results_npc_fine:&s->results_continue_bonus;
 switch(s->results_phase){
 case 0:{
  const gh_scene_status status=room_results_effect(s,0x161,frontier);
  if(status)return status;
  s->results_entries[count_entry]->seed=0;
  s->results_entries[award_entry]->seed=0;
  s->results_phase=1;
 }
 /* fall through as the original jump-table leaf does */
 case 1:room_results_slide_heading(s,s->results_state);break;
 case 2:{
  uint16_t raw;
  memcpy(&raw,s->b.save.bytes+(npc?0x96:0x12),sizeof(raw));
  const int32_t count=npc?(int16_t)raw:raw;
  if(count<0)return GH_SCENE_ARGUMENT;
  gh_id_entry *number=s->results_entries[count_entry];
  number->seed+=5;
  if(number->seed<=(uint32_t)count)break;
  number->seed=(uint32_t)count;
  *award=npc?(uint32_t)count*3000u:
     count<=0?50000u:count<=3?30000u:count<=9?10000u:count<=15?5000u:0u;
  s->results_phase=3;
  break;
 }
 case 3:return room_results_tally_award(s,award_entry,*award,npc,4,frontier);
 case 4:++s->results_state;s->results_phase=0;break;
 default:return GH_SCENE_ARGUMENT;
 }
 return GH_SCENE_OK;
}

static gh_scene_status room_results_tally_kills(gh_game_services *s,uint32_t *frontier)
{
 static const unsigned tiers[]={1,2,3,5};
 static const unsigned prices[]={100,150,200,300};
 if(u32((const uint8_t *)s->b.work+0x1a0)&0x10000000u){
  *frontier=0x1298;s->frontier_module_code=1;return GH_SCENE_UNSUPPORTED;
 }
 switch(s->results_phase){
 case 0:{
  const gh_scene_status status=room_results_effect(s,0x161,frontier);
  if(status)return status;
  for(unsigned i=16;i<=20;++i)s->results_entries[i]->seed=0;
  for(unsigned i=0;i<4;++i)
   s->results_entries[25+i]->seed=prices[i]*s->results_clear_variant;
  s->results_kill_bonus=0;
  s->results_phase=1;
 }
 /* fall through as the original jump-table leaf does */
 case 1:room_results_slide_heading(s,4);break;
 case 2:{
  int complete=1;
  uint32_t bonus=0;
  for(unsigned i=0;i<4;++i){
   int16_t count;
   memcpy(&count,s->b.save.bytes+0x8c+2*(tiers[i]-1),sizeof(count));
   if(count<0)return GH_SCENE_ARGUMENT;
   gh_id_entry *number=s->results_entries[16+i];
   number->seed+=5;
   if(number->seed>(uint32_t)count)number->seed=(uint32_t)count;
   complete&=number->seed==(uint32_t)count;
   bonus+=(uint32_t)count*prices[i]*s->results_clear_variant;
  }
  if(complete){s->results_kill_bonus=bonus;s->results_phase=3;}
  break;
 }
 case 3:return room_results_tally_award(s,20,s->results_kill_bonus,0,4,frontier);
 case 4:++s->results_state;s->results_phase=0;break;
 default:return GH_SCENE_ARGUMENT;
 }
 return GH_SCENE_OK;
}

/* REL09E4 slides the original TOTAL row and 0AD0 calls
 * cCoreSave::addGold(amount,1) only on the phase-three Cross edge. */
static gh_scene_status room_results_total(gh_game_services *s,uint32_t *frontier)
{
 const uint8_t *work=(const uint8_t *)s->b.work;
 const int confirm=(u32(work+0x1a0)&0x10000000u)!=0;
 switch(s->results_phase){
 case 0:{
  const gh_scene_status status=room_results_effect(s,0x161,frontier);
  if(status)return status;
  s->results_entries[15]->seed=0;
  s->results_phase=1;
 }
 /* fall through as the original jump-table leaf does */
 case 1:{
  const int32_t total=(int32_t)(s->results_continue_bonus+s->results_kill_bonus-s->results_npc_fine);
  s->results_total_bonus=total<0?0u:total>999999?999999u:(uint32_t)total;
  room_results_slide_heading(s,5);
  if(confirm){
   const gh_scene_status status=room_results_effect(s,0x15e,frontier);
   if(status)return status;
   gh_id_entry *heading=s->results_entries[5];
   memcpy(&heading->pos_x,heading->source+8,sizeof(heading->pos_x));
   s->results_phase=2;
  }
  break;
 }
 case 2:return room_results_tally_award(s,15,s->results_total_bonus,0,3,frontier);
 case 3:{
  if(!confirm)break;
  const gh_scene_status status=room_results_effect(s,0x15e,frontier);
  if(status)return status;
  uint8_t *save=s->b.save.bytes;
  s->results_entries[15]->seed=s->results_total_bonus;
  const uint32_t count=u32(save+0x24);
  if(count<16)w32(save+0x28+count*4,s->results_total_bonus);
  w32(save+0x24,0);
  const int32_t sum=(int32_t)(u32(save+0x20)+s->results_total_bonus);
  const uint32_t gold=(u32(work+0x594)&0x02000000u)||sum>999999?999999u:sum<0?0u:(uint32_t)sum;
  w32(save+0x20,gold);
  s->results_entries[29]->seed=gold;
  s->results_phase=4;
  break;
 }
 case 4:++s->results_state;s->results_phase=0;break;
 default:return GH_SCENE_ARGUMENT;
 }
 return GH_SCENE_OK;
}

static gh_scene_status room_results_open_save(gh_game_services *s,uint32_t *frontier)
{
 const uint8_t *work=(const uint8_t *)s->b.work;
 const uint32_t unlocked=u32(s->b.save.bytes+0xa4);
 unsigned stage=1;
 while(stage<9&&((u32(work+0x59c)&0x80000u)||(unlocked&(1u<<stage))))++stage;
 /* REL1C40 takes the last contiguous cleared stage. REL1FB0 for 0200
  * saves the authored stage jump, then points the card at the EV08 room. */
 *frontier=0x1fb0;s->frontier_module_code=1;
 if(stage!=2||!s->b.event_reserve||!s->b.children||!*s->b.children)return GH_SCENE_UNSUPPORTED;
 const uint16_t destination=0x200;
 gh_game_assets *assets=s->b.assets;
 *frontier=0x2008b8;s->frontier_module_code=0;
 if(gh_game_door_set_casino_jump_point_002008b8(&s->door,assets->room_jump.pointer,
     assets->room_jump_size,destination,0))return GH_SCENE_CALLEE;
 gh_event_reserve_work reserve={.flags=3,.mode=8,.destination_room=destination};
 memcpy(reserve.position,s->door.backup,12);
 memcpy(&reserve.yaw,s->door.backup+12,4);
 *s->b.event_reserve=reserve;
 s->door.backup[16]=3;
 s->door.backup[17]=0;
 const gh_save_map_bindings map={.work=s->b.work,.player=&s->title_player,
  .map_room=destination,.target_room=3};
 int started=0;
 *frontier=0x160020;
 if(gh_game_children_save_map_open_variant_00160020(*s->b.children,&map,1,&started,frontier))
  return GH_SCENE_CALLEE;
 s->results_state=10;
 return GH_SCENE_OK;
}

static gh_scene_status room_results_finish_save(gh_game_services *s,uint32_t *frontier)
{
 if(!s->results_save_target){
  /* REL0794 resolves the separate casino entry table for this menu choice. */
  *frontier=0x0794;s->frontier_module_code=1;return GH_SCENE_UNSUPPORTED;
 }
 if(!s->b.begin_event_room_jump)return GH_SCENE_ARGUMENT;
 const uint16_t room=(uint16_t)(s->door.backup[16]|(uint16_t)s->door.backup[17]<<8);
 const gh_game_assets *assets=s->b.assets;
 float position[3],yaw;
 *frontier=0x2be998;s->frontier_module_code=0;
 if(gh_game_room_jump_set_next_pose_002be998(s->b.work,assets->room_jump.pointer,
    assets->room_jump_size,room,0,position,&yaw))return GH_SCENE_CALLEE;
 *frontier=0x2c4548;
 if(s->b.begin_event_room_jump(s->b.event_room_context,room,position,yaw,0,0,1,frontier))
  return GH_SCENE_CALLEE;
 uint8_t *work=(uint8_t *)s->b.work;
 w32(work+0x5e4,u32(work+0x5e4)&~0x40000u);
 *frontier=0;
 return GH_SCENE_OK;
}

static gh_scene_status room_results_move(void *context,uint32_t *frontier)
{
 gh_game_services *s=context;
 const gh_r020_move_callbacks *callbacks=&s->b.title_callbacks;
 if(!s->b.screen_state_5cae40||
    (s->results_state<8&&!s->results_base.initialized)){
  *frontier=0x02e8;s->frontier_module_code=1;return GH_SCENE_UNSUPPORTED;
 }
 if(s->results_state==0){
  uint16_t clears;
  memcpy(&clears,s->b.save.bytes+0x10,sizeof(clears));
  const uint8_t *work=(const uint8_t *)s->b.work;
  if(!clears&&(u32(work+0x59c)&0x40000u))clears=1;
  s->results_clear_variant=clears?2:1;
  s->results_state=1;
 }else if(s->results_state==1&&s->results_phase<2){
  if(s->results_phase==0){
   *frontier=0x2cb8a0;
   if(!callbacks->program_se||callbacks->program_se(callbacks->context,0,0x161,frontier))
    return GH_SCENE_UNSUPPORTED;
   for(unsigned i=7;i<=10;++i)s->results_entries[i]->seed=0;
   s->results_phase=1;
  }
  room_results_slide_heading(s,1);
 }else if(s->results_state==1){
  const uint8_t *work=(const uint8_t *)s->b.work;
  if(work[0x5b7]!=1){
   *frontier=0x0cec;s->frontier_module_code=1;return GH_SCENE_UNSUPPORTED;
  }
  switch(s->results_phase){
  case 2:room_results_count_time(s);break;
  case 3:++s->results_phase;break;
  case 4:++s->results_state;s->results_phase=0;break;
  default:*frontier=0x0b38;s->frontier_module_code=1;return GH_SCENE_UNSUPPORTED;
  }
  if(u32(work+0x1a0)&0x10000000u){
   *frontier=0x0d98;s->frontier_module_code=1;return GH_SCENE_UNSUPPORTED;
  }
 }else if(s->results_state==2||s->results_state==3){
  const gh_scene_status status=room_results_count_continue_or_npc(s,frontier);
  if(status)return status;
 }else if(s->results_state==4){
  const gh_scene_status status=room_results_tally_kills(s,frontier);
  if(status)return status;
 }else if(s->results_state==5){
  const gh_scene_status status=room_results_total(s,frontier);
  if(status)return status;
 }else if(s->results_state==6){
  ++s->results_state;
 }else if(s->results_state==7){
  const uint8_t *work=(const uint8_t *)s->b.work;
  if(!s->b.fade||!s->b.display)return GH_SCENE_ARGUMENT;
  const gh_screen_fade_context fade={s->b.display->screen_width,
   s->b.display->gs_y_origin,u32(work+0x564),u32(work+0x5e0)};
  if(gh_screen_fade_start_00307208(s->b.fade,&fade,0,10,0,0,0xff000000u,15)){
   *frontier=0x307208;return GH_SCENE_UNSUPPORTED;
  }
  ++s->results_state;
 }else if(s->results_state==8&&(gh_screen_fade_status_00307718(s->b.fade)&4u)){
  const uint8_t *work=(const uint8_t *)s->b.work;
  uint8_t *save=s->b.save.bytes;
  /* REL06A4 resets per-stage data before releasing the results ID Base.
   * REL1D10 then records cumulative stage 1 via original 1FBF90. */
  if(!s->results_phase){
   if(gh_game_stage_clear_002bf8c8(&s->prepare,s->b.save,s->b.work->room)){
    *frontier=0x2bf8c8;return GH_SCENE_UNSUPPORTED;
   }
   w32(save+0xb8c,u32(save+0xb8c)|1u);
   s->results_phase=1;
  }
  const gh_scene_status status=room_results_release(s,frontier);
  if(status)return status;
  if(work[0x5b7]!=1){
   *frontier=0x1d10;s->frontier_module_code=1;return GH_SCENE_UNSUPPORTED;
  }
  /* 1FBF90 includes bits 0 and 1 when recording stage 1. */
  w32(save+0xa4,3);
  /* REL1D78 clears Event+38, reserves EV07 and asks cScenario for R002.
   * REL2A68 reads bit 3, clear in the newly authored flags, so force is 1. */
  *frontier=0x1d78;
  if(!s->b.event_reserve||!s->b.begin_event_room_jump)return GH_SCENE_UNSUPPORTED;
  *s->b.event_reserve=(gh_event_reserve_work){.flags=3,.mode=7,.destination_room=0x21};
  if(s->b.work->room!=2){
   const float zero[3]={0};
   *frontier=0x2c4548;
   if(s->b.begin_event_room_jump(s->b.event_room_context,2,zero,0,0,0,1,frontier))
    return GH_SCENE_CALLEE;
  }
  *s->b.screen_state_5cae40=15;
  *frontier=0;
  return GH_SCENE_OK;
 }else if(s->results_state==9){
  const gh_scene_status status=room_results_open_save(s,frontier);
  if(status)return status;
 }else if(s->results_state==10){
  const uint8_t *manager;
  size_t bytes;
  *frontier=0x0760;s->frontier_module_code=1;
  if(gh_game_children_manager_view(*s->b.children,&manager,&bytes)||bytes<0x866)
   return GH_SCENE_ARGUMENT;
  if(u32(manager+0x48)==UINT32_MAX){
   s->results_save_target=(uint16_t)(manager[0x864]|(uint16_t)manager[0x865]<<8);
   s->results_state=11;
  }
 }else if(s->results_state==11){
  const gh_scene_status status=room_results_finish_save(s,frontier);
  if(status)return status;
 }
 *s->b.screen_state_5cae40=15;
 gh_id_frontier detail={0};
 gh_id_error id=gh_id_jobs_move(&s->results_base,NULL,NULL,&detail);
 if(id){*frontier=detail.address?detail.address:0x2aaa40;return GH_SCENE_UNSUPPORTED;}
 gh_id_draw_job jobs[80];
 size_t count;
 id=gh_id_jobs_collect_visible(&s->results_base,jobs,80,&count,&detail);
 if(id){*frontier=detail.address?detail.address:0x2aac28;return GH_SCENE_UNSUPPORTED;}
 for(size_t i=0;i<count;++i){
  *frontier=jobs[i].consumer_address;
  if(!callbacks->draw_entry||callbacks->draw_entry(callbacks->context,&s->results_base,&jobs[i],frontier))
   return GH_SCENE_UNSUPPORTED;
 }
 *frontier=0;
 return GH_SCENE_OK;
}

/* Original R104PreInit at REL01A0 requests one skipped EMS bind for
 * the intro route. The other paths leave the reset byte untouched. */
static gh_scene_status room_r104_preinit(void *context,uint32_t *frontier)
{
 gh_game_services *s=context;
 if(!s||!frontier||!s->module.bound||s->module.room!=0x104||
    !s->room_state.active||s->room_state.size<8||!s->b.em_set_first_byte)
  return GH_SCENE_ARGUMENT;
 *frontier=0;
 if(!(u32((const uint8_t *)s->b.work+0x590)&0x200000u)&&
    !(u32(s->room_state.active+4)&0x80000000u))
  *s->b.em_set_first_byte=1;
 return GH_SCENE_OK;
}

static gh_scene_status room_r100_preinit(void*p,uint32_t*frontier){
 gh_game_services*s=p;if(frontier)*frontier=0;
 return gh_r100_preinit_000001a0(s->b.work,&s->room_state,s->b.em_set_first_byte);
}
static gh_scene_status room_r100_move(void*p,uint32_t*frontier){
 (void)p;if(frontier)*frontier=0;return gh_r100_move_00000740();
}
static gh_r100_init_status room_r100_om_view(void*p,uint32_t handle,gh_r100_om_view*out,uint32_t*frontier){
 gh_game_services*s=p;gh_title_player_view v;const gh_title_player_view *player=NULL;
 /* Original shared OM registry also contains ID100. Let SearchOM read and
  * reject its real ID before considering the name, rather than failing a
  * class cast simply because the first canonical entry is the player. */
 if(gh_game_player_factory_scalar_view(&s->player_factory,handle,&player)==GH_PLAYER_OK){
  *out=(gh_r100_om_view){player->bytes,player->size,player->bytes};return GH_R100_INIT_OK;
 }
 if(s->player_child.handle==handle&&s->player_child.complete){
  const gh_title_player_view *child=&s->player_child.object.view;
  *out=(gh_r100_om_view){child->bytes,child->size,child->bytes};return GH_R100_INIT_OK;
 }
 gh_enemy_ems_live enemy;
 if(gh_enemy_ems_live_at(s->enemies,handle,&enemy)){if(frontier)*frontier=0x14fd98;return GH_R100_INIT_CALLEE;}
 if(enemy.actor){const gh_title_player_view *view=&enemy.actor->view;
  *out=(gh_r100_om_view){view->bytes,view->size,view->bytes};return GH_R100_INIT_OK;}
 gh_scene_placement_status e=gh_om_factory_object_view(&s->om_factory,handle,&v);
 if(e){if(frontier)*frontier=0x14fd98;return GH_R100_INIT_UNSUPPORTED;}
 *out=(gh_r100_om_view){v.bytes,v.size,v.bytes};return GH_R100_INIT_OK;
}
static gh_player_init_status player_model_initialize(void*p,gh_game_title_player*player,gh_boot_work*work,
 const gh_bundle_entry*geometry,const gh_bundle_entry*texture,uint32_t mode,int*original,uint32_t*frontier){
 gh_game_services*s=p;
 if(player!=&s->title_player||work!=s->b.work)return GH_PLAYER_INIT_ARGUMENT;
 gh_actor_model_status bind=gh_actor_model_bind_packet_inputs(&s->player_model,s->b.assets->heap,s->world_light.scalar+4);
 if(bind)return GH_PLAYER_INIT_CALLEE;
 gh_actor_model_status e=gh_actor_model_initialize_00149738(&s->player_model,&player->view,work,&s->pools,geometry,texture,0,mode,GH_STATIC_BINARY32_REFERENCE,original);
 if(frontier)*frontier=s->player_model.frontier;
 return !e?GH_PLAYER_INIT_OK:e==GH_ACTOR_MODEL_UNSUPPORTED?GH_PLAYER_INIT_UNSUPPORTED:GH_PLAYER_INIT_CALLEE;
}
static gh_title_player_status player_child_initialize(void*p,gh_game_player_child*child,int*original,uint32_t*frontier){
 gh_game_services*s=p;
 if(child!=&s->player_child)return GH_PLAYER_ARGUMENT;
 /* Original default packet heap and 604704 are the same owners used by
  * the parent model and world light, borrowed through construction. */
 gh_actor_model_status bind=gh_actor_model_bind_packet_inputs(&child->model,s->b.assets->heap,s->world_light.scalar+4);
 if(bind)return GH_PLAYER_BOUNDS;
 gh_title_player_status e;
 if(!s->child_tail.complete)e=gh_game_player_child_initialize_00100378(child,original);
 else e=GH_PLAYER_UNSUPPORTED;
 if((e==GH_PLAYER_UNSUPPORTED&&child->frontier==0x100420)||s->child_tail.complete){
  if(!s->b.player_shared_vector){if(frontier)*frontier=0x100420;return GH_PLAYER_UNSUPPORTED;}
  e=gh_game_player_child_finish_00100420(&s->child_tail,child,s->b.player_shared_vector,&s->work5f8,s->b.save,
   s->b.boot_elf,s->b.boot_elf_bytes,GH_STATIC_BINARY32_REFERENCE,original);
 }
 if(frontier)*frontier=child->frontier;
 return e;
}
static gh_effect_release_bindings player_effect_storage(gh_game_services*s){
 return (gh_effect_release_bindings){.effects=&s->effects,.esp=s->scene_buffer_a,.emitters=s->scene_buffer_b,
  .esp_bytes=sizeof(s->scene_buffer_a),.emitter_bytes=sizeof(s->scene_buffer_b),.actor_match=gh_effect_create_actor_match,.context=&s->effects};
}
static gh_effect_status create_model_emitter(void *context,const gh_esp14_nested_request *request,
 const gh_actor_model *model,int *original_return,uint32_t *frontier)
{
 gh_services_esp14_slot *slot=context;
 gh_game_services *s=slot->services;
 gh_effect_create_call *call=&slot->nested_emitter;
 if(!request||!original_return||!frontier||model!=&slot->instance.child.model||
    request->request.actor!=&slot->instance.child.object.view)return GH_EFFECT_ARGUMENT;
 *frontier=0x306528;
 if(!call->active){
  gh_effect_status status=gh_effect_create_nested_begin_002f1d3c(call,&request->request,
   request->group_74,request->config_40,request->config_48);
  if(status)return status;
 }
 const gh_effect_create_bindings bindings={.storage=player_effect_storage(s),.work=s->b.work,
  .random=&s->b.init->random,.model=model};
 gh_effect_status status=gh_game_effect_create_00306528(call,&bindings);
 if(status){if(call->frontier)*frontier=call->frontier;return status;}
 if(!call->complete)return GH_EFFECT_ARGUMENT;
 *original_return=call->original_return;
 *frontier=0;
 return GH_EFFECT_OK;
}

static gh_esp14_bindings model_effect_bindings(gh_services_esp14_slot *slot)
{
 gh_game_services *s=slot->services;
 return (gh_esp14_bindings){
  .model={.pools=&s->pools,.work=s->b.work,.factory_serial=s->b.factory_serial,
   .packet_heap=s->b.assets->heap,.fog_rgb=s->world_light.scalar+4,
   .policy=GH_STATIC_BINARY32_REFERENCE,.canonical_rate=s->b.rate},
  .effects=&s->effects,.atan_coeff=s->b.camera_envelope?s->b.camera_envelope->atan_coeff:NULL,
  .nested_create=create_model_emitter,.context=slot
 };
}

static gh_services_esp14_slot *model_effect_slot(gh_game_services *s,const gh_effect_esp_view *view,unsigned index)
{
 if(index>=sizeof(s->esp14_slots)/sizeof(*s->esp14_slots)||(view->type!=4&&view->type!=0x14)||!view->serial)return NULL;
 gh_services_esp14_slot *slot=s->esp14_slots[index];
 return slot&&slot->serial==view->serial?slot:NULL;
}

static gh_effect_status initialize_model_effect(void *context,const gh_effect_esp_view *view,unsigned index,
 uint32_t parameter_40,int *original_return,uint32_t *frontier)
{
 gh_game_services *s=context;
 *frontier=view&&view->type==4?0x2e64b0:0x2f1d58;
 if(!view||(view->type!=4&&view->type!=0x14)||!view->serial||index>=sizeof(s->esp14_slots)/sizeof(*s->esp14_slots))
  return GH_EFFECT_ARGUMENT;
 if(!s->esp14_slots[index]){
  gh_services_esp14_slot *slot=calloc(1,sizeof(*slot));
  if(!slot)return GH_EFFECT_BOUNDS;
  slot->services=s;
  slot->serial=view->serial;
  s->esp14_slots[index]=slot;
  if(s->esp14_limit<=index)s->esp14_limit=(size_t)index+1;
 }
 gh_services_esp14_slot *slot=model_effect_slot(s,view,index);
 if(!slot)return GH_EFFECT_ARGUMENT;
 const gh_esp14_bindings bindings=model_effect_bindings(slot);
 if(view->type==4)
  return gh_esp04_initialize_002e64b0(&slot->instance,&bindings,view,index,parameter_40,original_return,frontier);
 return gh_esp14_initialize_002f1d58(&slot->instance,&bindings,view,index,parameter_40,original_return,frontier);
}

static gh_effect_status move_model_effect(void *context,const gh_effect_esp_view *view,unsigned index,
 int active,uint32_t *frontier)
{
 gh_services_esp14_slot *slot=model_effect_slot(context,view,index);
 if(!slot)return GH_EFFECT_ARGUMENT;
 const gh_esp14_bindings bindings=model_effect_bindings(slot);
 if(view->type==4)
  return gh_esp04_move_after_base_002e6868(&slot->instance,&bindings,view,active,frontier);
 return gh_esp14_move_after_base_002f2250(&slot->instance,&bindings,view,active,frontier);
}

static gh_effect_status trans_model_effect(void *context,const gh_effect_esp_view *view,unsigned index,uint32_t *frontier)
{
 gh_services_esp14_slot *slot=model_effect_slot(context,view,index);
 if(!slot)return GH_EFFECT_ARGUMENT;
 return gh_esp14_trans_002f2758(&slot->instance,view,frontier);
}

static gh_effect_status detach_model_effect_sound(void *context,gh_actor_model *model,uint32_t *frontier)
{
 gh_game_services *s=context;
 *frontier=0x2cbe18;
 if(!s->b.voices)return GH_EFFECT_UNSUPPORTED;
 gh_voice_status status=gh_se_voice_detach_actor_002ce488(s->b.voices,model);
 if(status){
  uint32_t detail=gh_se_voice_frontier(s->b.voices);
  if(detail)*frontier=detail;
  return GH_EFFECT_UNSUPPORTED;
 }
 *frontier=0;
 return GH_EFFECT_OK;
}

static gh_effect_status destroy_model_effect(void *context,const gh_effect_esp_view *view,
 unsigned index,uint32_t *frontier)
{
 gh_game_services *s=context;
 gh_services_esp14_slot *slot=model_effect_slot(s,view,index);
 if(!slot){*frontier=0x2f27e8;return GH_EFFECT_UNSUPPORTED;}
 gh_effect_status status=gh_esp14_destroy_hidden_002f27e8(&slot->instance,
     detach_model_effect_sound,s,frontier);
 if(status)return status;
 /* The current effect traversal clears the original active bit immediately
  * after returning. A new serial may then reuse the slot. */
 s->esp14_slots[index]=NULL;
 slot->retired_next=s->esp14_retired;
 s->esp14_retired=slot;
 while(s->esp14_limit&&!s->esp14_slots[s->esp14_limit-1])s->esp14_limit--;
 return GH_EFFECT_OK;
}

static gh_effect_status collect_model_effects(void *context,uint32_t *frontier)
{
 gh_game_services *s=context;
 gh_services_esp14_slot **link=&s->esp14_retired;
 while(*link){
  gh_services_esp14_slot *slot=*link;
  int collected=0;
  gh_effect_status status=gh_esp14_collect_retired(&slot->instance,&collected,frontier);
  if(status)return status;
  if(collected){*link=slot->retired_next;free(slot);}
  else link=&slot->retired_next;
 }
 return GH_EFFECT_OK;
}

static int model_effect_registered(gh_game_services *s,gh_services_esp14_slot *slot,uint32_t handle)
{
 if(!slot||slot->instance.child.handle!=handle)return 0;
 gh_esp14_model *child=&slot->instance.child;
 void *registered=NULL;
 return gh_room_object_lookup(&s->pools,handle,GH_ESP_MODEL_KIND,child,&registered)==GH_POOL_OK&&
        registered==&child->object;
}

static gh_services_esp14_slot *model_effect_actor(gh_game_services *s,uint32_t handle)
{
 if(!handle)return NULL;
 for(size_t i=0;i<s->esp14_limit;i++){
  gh_services_esp14_slot *slot=s->esp14_slots[i];
  if(model_effect_registered(s,slot,handle))return slot;
 }
 for(gh_services_esp14_slot *slot=s->esp14_retired;slot;slot=slot->retired_next)
  if(model_effect_registered(s,slot,handle))return slot;
 return NULL;
}

static gh_effect_status shutdown_model_effect(gh_game_services *s,gh_services_esp14_slot *slot)
{
 if(slot->instance.child.model.sound_borrowers){
  uint32_t frontier;
  gh_effect_status status=detach_model_effect_sound(s,&slot->instance.child.model,&frontier);
  if(status)return status;
 }
 return gh_esp14_model_dispose(&slot->instance.child);
}
static gh_actor_motion_status release_actor_motion_effect(gh_game_services *s,const gh_title_player_view *actor,
 int32_t kind,uint32_t mode,uint32_t *frontier){
 gh_effect_release_bindings b=player_effect_storage(s);unsigned slot=0;
 gh_effect_status e=gh_game_effect_release_00306200(&b,actor,kind,mode,frontier,&slot);
 return !e?GH_ACTOR_MOTION_OK:e==GH_EFFECT_UNSUPPORTED?GH_ACTOR_MOTION_UNSUPPORTED:GH_ACTOR_MOTION_CALLEE;
}
static gh_actor_motion_status om_motion_effect(void *context,const gh_title_player_view *actor,
 int32_t kind,uint32_t mode,uint32_t *frontier)
{
 return release_actor_motion_effect(context,actor,kind,mode,frontier);
}

static gh_actor_motion_status player_motion_effect(void*p,const gh_title_player_view*actor,int32_t kind,uint32_t mode,uint32_t*frontier){
 gh_game_services*s=p;if(!actor||actor->bytes!=s->title_player.view.bytes)return GH_ACTOR_MOTION_ARGUMENT;
 return release_actor_motion_effect(s,actor,kind,mode,frontier);
}
static gh_actor_motion_status enemy_motion_effect(void*p,const gh_title_player_view*actor,int32_t kind,uint32_t mode,uint32_t*frontier){
 gh_game_services*s=p;int owned=0;
 for(unsigned i=0;actor&&i<64;i++){
  const gh_game_title_player *enemy=s->b.init->enemy_actors[i];
  if(enemy&&enemy->view.bytes==actor->bytes){owned=1;break;}
 }
 if(!owned)return GH_ACTOR_MOTION_ARGUMENT;
 return release_actor_motion_effect(s,actor,kind,mode,frontier);
}
typedef struct {
 gh_game_services *services;
 const gh_title_player_view *registered;
} gh_held_weapon_effect_context;

static gh_actor_motion_status held_weapon_motion_effect(void *context,const gh_title_player_view *actor,
 int32_t kind,uint32_t mode,uint32_t *frontier){
 const gh_held_weapon_effect_context *binding=context;
 if(actor!=binding->registered)return GH_ACTOR_MOTION_ARGUMENT;
 return release_actor_motion_effect(binding->services,actor,kind,mode,frontier);
}
static gh_game_services_status enemy_place_error(gh_scene_placement_status e){
 return !e?GH_SERVICES_OK:e==GH_PLACE_UNSUPPORTED?GH_SERVICES_FRONTIER:
    e==GH_PLACE_MEMORY?GH_SERVICES_MEMORY:e==GH_PLACE_BUSY?GH_SERVICES_BUSY:GH_SERVICES_ASSET;
}
gh_game_services_status gh_game_services_enemy_set_all_00294ca0(gh_game_services*s,uint32_t*f,int*detail){
 if(!s||!f||!detail)return GH_SERVICES_ARGUMENT;*f=0;*detail=0;
 if(!s->em_placements.stop_flag||!s->room_state.active||!s->room_objects){
  *f=0x294ca0;return GH_SERVICES_FRONTIER;
 }
 if(!s->em_resources.bindings)s->em_resource_bindings=(gh_em_resource_bindings){
  .em=&s->em_placements,.room=&s->room_state,.room_objects=s->room_objects,.work=s->b.work,
  .scenario14=s->scenario_scalar+0x14,.banks=s->b.enemy_banks,.resources=s->object_resources};
 gh_scene_placement_status e=gh_em_resources_00294ca0(&s->em_resources,&s->em_resource_bindings);
 if(!e)return GH_SERVICES_OK;
 if(e!=GH_PLACE_UNSUPPORTED||s->em_resources.frontier!=0x2905d0){
  *f=s->em_resources.frontier?s->em_resources.frontier:0x294ca0;
  *detail=s->em_resources.detail;return enemy_place_error(e);
 }
 if(!s->enemy_nodes.initialized&&gh_enemy_node_names_from_elf(&s->enemy_nodes,s->b.boot_elf,s->b.boot_elf_bytes)){
  *f=0x14b730;return GH_SERVICES_FRONTIER;
 }
 if(!s->enemy2a2_nodes.initialized&&gh_enemy2a2_node_names_from_elf(&s->enemy2a2_nodes,s->b.boot_elf,s->b.boot_elf_bytes)){
  *f=0x14b730;return GH_SERVICES_FRONTIER;
 }
 if(!s->enemy_bindings.actor.entry.init)s->enemy_bindings=(gh_enemy_ems_bindings){
  .actor={.entry={.init=s->b.init,.pools=&s->pools,.room_objects=s->room_objects,.global_objects=s->objects,
    .work=s->b.work,.room_heap=s->b.assets->heap,.packet_heap=s->b.assets->heap,.fog_rgb=s->world_light.scalar+4,
    .save=s->b.save,.scenario14=s->scenario_scalar+0x14,.borrowed586aa4=s->b.init->enemies+0x5b4,
    .factory_serial=s->b.factory_serial,.banks=s->b.enemy_banks,.resources=s->object_resources,
    .policy=GH_STATIC_BINARY32_REFERENCE,.effects=&s->effects,
    .boot_elf=s->b.boot_elf,.boot_elf_bytes=s->b.boot_elf_bytes},
   .cockpit=s->cockpit,.ik=s->b.motion_ik_globals,.actors=&s->scene_tables,
   .atan_coeff=s->b.camera_envelope?s->b.camera_envelope->atan_coeff:NULL,.canonical_rate=&s->b.work->canonical_rate574,
   .effect=enemy_motion_effect,.effect_context=s,.spawn_effect=&s->enemy_bindings.effects_2ad,
   .solids=&s->collision_solids,.default_heap=s->b.assets->heap,
   .node_names=&s->enemy_nodes,.shapes={&s->collision_pairs,s->b.math_globals,GH_STATIC_BINARY32_REFERENCE},
   .numeric_policy=GH_EM_NUMERIC_PCSX2_CHOPZERO_REFERENCE,.weapon_scroll=&s->r100.collision},
  .names_2a2=&s->enemy2a2_nodes,.action_buttons=&s->action_buttons,
  .projectile_factory=&s->om_factory,
  .effects_2ad={.storage=player_effect_storage(s),.work=s->b.work,.random=&s->b.init->random,
    .atan_coeff=s->b.camera_envelope?s->b.camera_envelope->atan_coeff:NULL}};
 return enemy_place_error(gh_enemy_ems_spawn_00294ca0(&s->enemies,&s->em_resources,&s->room_state,&s->enemy_bindings,f,detail));
}
static int original_em27_grab_attacker(const gh_game_services *s,const gh_enemy_actor *enemy)
{
 if(!enemy||!enemy->entry.published||enemy->entry.b.init!=s->b.init)return 0;
 const gh_title_player_view *actor=&enemy->entry.object.view;
 const uint8_t *raw=actor->bytes;
 if(!raw||actor->size<=0x1650||actor->original_vtable214!=0x4479c8||
    u32(raw+0x564)!=0x227||raw[0x641])return 0;
 return (s->b.work->room==0x105&&raw[0x640]==2&&!raw[0x1650])||
    (s->b.work->room==0x101&&(raw[0x640]==0x1e||raw[0x640]==0x26)&&raw[0x1650]==6);
}

/* The opening R102 EM00 uses the paired selector 2 grab. */
static int r102_em00_grab_attacker(const gh_game_services *s,const gh_enemy_actor *enemy)
{
 if(s->b.work->room!=0x102||!enemy||!enemy->entry.published||enemy->entry.b.init!=s->b.init)return 0;
 const gh_title_player_view *view=&enemy->entry.object.view;
 const uint8_t *actor=view->bytes;
 return actor&&view->size>0x1650&&view->original_vtable214==0x4479c8&&
    u32(actor+0x564)==0x200&&(u32(actor+0x640)&0xffffu)==11&&actor[0x1650]==5;
}

static int r103_grab_attacker(const gh_game_services *s,const gh_enemy_actor *enemy,uint16_t dependency)
{
 return enemy&&enemy->entry.published&&enemy->entry.b.init==s->b.init&&
    enemy->entry.request.dependency_kind==dependency&&
    gh_enemy204_is_r103_entry(&enemy->entry.request,s->b.work->room);
}

static int r102_east_grab_attacker(const gh_game_services *s,const gh_enemy_actor *enemy)
{
 if(s->b.work->room!=0x102||!enemy||!enemy->entry.published||enemy->entry.b.init!=s->b.init)return 0;
 const gh_title_player_view *view=&enemy->entry.object.view;
 const uint8_t *actor=view->bytes;
 return actor&&view->size>0x1650&&view->original_vtable214==0x4479c8&&
    u32(actor+0x564)==0x200&&(u32(actor+0x640)&0xffffu)==3&&actor[0x1650]==1;
}

static int r102_gate_em0a_grab_attacker(const gh_game_services *s,const gh_enemy_actor *enemy)
{
 if(s->b.work->room!=0x102||!enemy||!enemy->entry.published||enemy->entry.b.init!=s->b.init)return 0;
 const gh_title_player_view *view=&enemy->entry.object.view;
 const uint8_t *p=view->bytes;
 return p&&view->size>=0x16c8&&view->original_vtable214==0x4479c8&&u32(p+0x564)==0x20a&&
    (u32(p+0x640)&0xffffu)==2&&p[0x1650]==5&&!u32(p+0x16c4);
}

static int r102_area11_short_grab_attacker(const gh_game_services *s,const gh_enemy_actor *enemy)
{
 if(s->b.work->room!=0x102||!enemy||!enemy->entry.published||enemy->entry.b.init!=s->b.init)return 0;
 const gh_title_player_view *view=&enemy->entry.object.view;
 const uint8_t *p=view->bytes;
 return p&&view->size>=0x16c8&&view->original_vtable214==0x4479c8&&
    u32(p+0x564)==0x200&&(u32(p+0x640)&0xffffu)==7&&!p[0x1650]&&u32(p+0x16c4)==2;
}

static int r102_area11_grab_attacker(const gh_game_services *s,const gh_enemy_actor *enemy)
{
 if(s->b.work->room!=0x102||!enemy)return 0;
 const gh_title_player_view *view=&enemy->entry.object.view;
 const uint8_t *actor=view->bytes;
 return actor&&view->size>=0x16c8&&view->original_vtable214==0x4479c8&&
    u32(actor+0x564)==0x202&&(u32(actor+0x640)&0xffffu)==0x17&&!actor[0x1650]&&u32(actor+0x16c4)==2;
}

static int r100_wave_em02_grab_attacker(const gh_game_services *s,const gh_enemy_actor *enemy)
{
 if(s->b.work->room!=0x100||!enemy)return 0;
 const gh_title_player_view *view=&enemy->entry.object.view;
 const uint8_t *actor=view->bytes;
 if(!actor||view->size<0x16c8||view->original_vtable214!=0x4479c8||
    u32(actor+0x564)!=0x202||actor[0x1650])return 0;
 const unsigned stamp=u32(actor+0x640)&0xffffu;
 const uint32_t mode=u32(actor+0x16c4);
 return (stamp==29&&mode==2)||(stamp==32&&!mode);
}

static int r101_side_long_grab_attacker(const gh_game_services *s,const gh_enemy_actor *enemy)
{
 if(s->b.work->room!=0x101||!enemy||!enemy->entry.published||enemy->entry.b.init!=s->b.init)return 0;
 const gh_title_player_view *view=&enemy->entry.object.view;
 const uint8_t *actor=view->bytes;
 return actor&&view->size>0x1650&&view->original_vtable214==0x4479c8&&
    actor[0x1650]==6&&
    ((u32(actor+0x564)==0x202&&(u32(actor+0x640)&0xffffu)==0x27)||
     (u32(actor+0x564)==0x203&&(u32(actor+0x640)&0xffffu)==0x28));
}

static int player_motion_request_matches(gh_game_services*s,const gh_player_motion_request*r){
 if(!r||r->player!=&s->title_player||r->model!=&s->player_model||
    r->work!=s->b.work||r->rate!=s->b.rate)return 0;
 if(r->resource==s->player_factory.resource.bytes&&r->resource_bytes==s->player_factory.file_bytes)return 1;
 const uint8_t *p=s->title_player.view.bytes;
 gh_enemy_ems_live live={0};
 if(!p||!s->player_grab_attacker||
    gh_enemy_ems_live_at(s->enemies,s->player_grab_attacker,&live)||!live.enemy202)return 0;
 const unsigned selector=p[0x2f5],phase=p[0x2f6];
 if(p[0x2f4]==1&&selector==0xf&&!phase&&p[0x2f7]<=1&&
    r103_grab_attacker(s,live.enemy202,0x215)){
  const size_t row=p[0x2f7]?0x1bb0:0x1bb8;
  const gh_object_view *resource=&live.enemy202->entry.resource;
  return resource->bytes&&resource->file_extent_known&&resource->file_extent>=row+8&&
    resource->file_extent<=resource->size&&r->resource==resource->bytes&&
    r->resource_bytes==resource->size&&r->motion_offset==u32(resource->bytes+row)&&
    r->sequence_candidate_offset==u32(resource->bytes+row+4)&&!r->start_frame_bits&&
    !r->blend_frames&&!r->flags&&!r->release_previous_sequence;
 }
 const int long_grab=selector==0xb&&
     (r102_area11_grab_attacker(s,live.enemy202)||r101_side_long_grab_attacker(s,live.enemy202)||r103_grab_attacker(s,live.enemy202,0x203)||
      r103_grab_attacker(s,live.enemy202,0x214));
 const int arena=r103_grab_attacker(s,live.enemy202,0x200);
 const int west_death=selector==0xb&&phase==4&&r100_wave_em02_grab_attacker(s,live.enemy202);
 const int short_grab=selector==0xa&&
     (original_em27_grab_attacker(s,live.enemy202)||r102_em00_grab_attacker(s,live.enemy202)||
      r102_east_grab_attacker(s,live.enemy202)||r102_area11_short_grab_attacker(s,live.enemy202)||arena);
 const int em0a_grab=selector==8&&(r102_gate_em0a_grab_attacker(s,live.enemy202)||
     r103_grab_attacker(s,live.enemy202,0x20a));
 const int r104_grab=selector==9&&
     gh_enemy203_is_r104_entry(&live.enemy202->entry.request,s->b.work->room);
 const int counter=p[0x2f4]==0&&selector==0x39&&phase==0;
 const int em27_counter=counter&&original_em27_grab_attacker(s,live.enemy202)&&s->b.work->room==0x101;
 const int r102_counter=counter&&(r102_em00_grab_attacker(s,live.enemy202)||
     r102_east_grab_attacker(s,live.enemy202)||r102_area11_short_grab_attacker(s,live.enemy202));
 if(s->b.work->room!=0x100&&!long_grab&&!short_grab&&!em0a_grab&&!r104_grab&&
    !r102_counter&&!em27_counter&&!(arena&&counter))return 0;
 const int em0a_receiver=selector==8&&(s->b.work->room==0x100||em0a_grab);
 if(p[0x2f4]==1&&(phase==0||phase==2||phase==4)&&(em0a_receiver||r104_grab)){
  const size_t row=(r104_grab?0x1a7cu:0xc80u)+4u*phase;
  const gh_title_player_view *attacker=&live.enemy202->entry.object.view;
  const gh_object_view *resource=&live.enemy202->entry.resource;
  return attacker->bytes&&attacker->size>0x1650&&attacker->original_vtable214==0x4479c8&&
     u32(attacker->bytes+0x564)==(r104_grab?0x250u:0x20au)&&
     (r104_grab||em0a_grab||(!attacker->bytes[0x640]&&!attacker->bytes[0x641]&&attacker->bytes[0x1650]==3))&&
     resource->bytes&&resource->file_extent_known&&
     resource->file_extent>=row+8&&resource->file_extent<=resource->size&&
     r->resource==resource->bytes&&r->resource_bytes==resource->size&&
     r->motion_offset==u32(resource->bytes+row)&&
     r->sequence_candidate_offset==u32(resource->bytes+row+4)&&!r->start_frame_bits&&
     r->blend_frames==(phase?3u:0u)&&!r->flags&&r->release_previous_sequence==1;
 }
 const int arena_death=selector==0xb&&phase==4&&r103_grab_attacker(s,live.enemy202,0x214);
 if(!counter&&(p[0x2f4]!=1||
     (selector!=0xa&&selector!=0xb)||
     (selector==0xa?phase!=0:(phase!=0&&phase!=2&&phase!=6&&!arena_death&&!west_death))))return 0;
 const gh_object_view *resource=&live.enemy202->entry.resource;
 const size_t row=counter?0x538:selector==0xa?0x520:
     (arena_death||west_death)&&(int16_t)(u32(p+0x548)>>16)<1?0x810:phase>=4?0x800:phase==2?0x7f8:0x808;
 return resource->bytes&&resource->file_extent_known&&resource->file_extent>=row+8&&
  resource->file_extent<=resource->size&&
  r->resource==resource->bytes&&r->resource_bytes==resource->size&&
  r->motion_offset==u32(resource->bytes+row)&&
  r->sequence_candidate_offset==u32(resource->bytes+row+4)&&
  !r->start_frame_bits&&r->blend_frames==(phase>=4?3u:0u)&&!r->flags&&
  r->release_previous_sequence==(s->b.work->room==0x100||long_grab||r102_counter||em27_counter||(arena&&counter));
}
static gh_player_init_status actor_sound_prepare(gh_game_services*s,gh_actor_attached_sound*sound,uint32_t*frontier){
 if(!s->b.voices)return GH_PLAYER_INIT_OK; /*unbound legacy fixture stays strict on real sound*/
 gh_actor_attached_sound_bindings b={s->b.voices,s->b.sound_control,s->b.sound_scene,s->b.work,&s->b.init->random};
 gh_voice_status e=gh_actor_attached_sound_bind(sound,&b);
 if(!e)e=gh_actor_attached_sound_bind_scene_tables(sound,&s->scene_tables);
 if(e){if(frontier)*frontier=sound->frontier?sound->frontier:0x2cba48;return GH_PLAYER_INIT_CALLEE;}return GH_PLAYER_INIT_OK;
}

static gh_actor_motion_status enemy_ema2_near_sound(void *context,const gh_game_title_player *target,
 uint32_t *frontier) {
 gh_game_services *s=context;
 if(!s||!target||!frontier)return GH_ACTOR_MOTION_ARGUMENT;
 *frontier=0x2cba48;
 if(!s->enemy_sound.bound)return GH_ACTOR_MOTION_UNSUPPORTED;
 gh_motion_sound_request *request=&s->ema2_near_sound;
 if(request->actor&&request->actor!=&target->view)return GH_ACTOR_MOTION_BUSY;
 if(!request->actor){
  uint32_t handle;
  gh_enemy_ems_live live;
  if(gh_enemy_ems_handle_for_actor(s->enemies,target,&handle)||!handle||
     gh_enemy_ems_live_at(s->enemies,handle,&live)||live.actor!=target||!live.model)
   return GH_ACTOR_MOTION_UNSUPPORTED;
  *request=(gh_motion_sound_request){.model=live.model,.actor=&target->view,.bank=0,.program=0x96};
 }
 uint32_t handle;
 gh_actor_motion_status status=gh_actor_attached_sound_motion(&s->enemy_sound,request,&handle,frontier);
 if(!status)*request=(gh_motion_sound_request){0};
 return status;
}

static gh_player_init_status player_motion_initialize(void*p,const gh_player_motion_request*r,uint32_t*frontier){
 gh_game_services*s=p;if(!player_motion_request_matches(s,r))return GH_PLAYER_INIT_ARGUMENT;
 gh_player_init_status sound_ready=actor_sound_prepare(s,&s->player_sound,frontier);if(sound_ready)return sound_ready;
 gh_actor_motion_bindings b={.model=r->model,.resource=r->resource,.resource_bytes=r->resource_bytes,.work=r->work,
   .effect=player_motion_effect,.context=s,.ik_globals=s->b.motion_ik_globals,.save=s->b.save,.save_binding_known=1,
   .sound=s->player_sound.bound?gh_actor_attached_sound_motion:NULL,.sound_context=s->player_sound.bound?&s->player_sound:NULL,
   .sequence_random=s->b.frame_random};
  gh_actor_motion_status e;gh_actor_motion_call *call=&s->player_motion_call;
  if(call->active&&!call->complete){
   /* Retry the same original invocation after a late native frontier. */
   if(call->current!=&s->player_motion||call->motion_offset!=r->motion_offset||call->candidate_offset!=r->sequence_candidate_offset||
      call->start_frame_bits!=r->start_frame_bits||call->blend_frames!=r->blend_frames||call->flags!=r->flags||call->release_previous!=r->release_previous_sequence)
    return GH_PLAYER_INIT_ARGUMENT;
   e=gh_actor_motion_call_step_002a8578(call);if(frontier)*frontier=call->frontier;
  }else if(s->player_motion.bound&&s->player_motion.complete){
   e=gh_actor_motion_call_open_parameters_002a8578(call,&s->player_motion,&b,r->motion_offset,r->sequence_candidate_offset,
    r->start_frame_bits,r->blend_frames,r->flags,r->release_previous_sequence);
   if(!e)e=gh_actor_motion_call_step_002a8578(call);if(frontier)*frontier=call->frontier;
  }else{
   /* Constructor's first Begin has no old native sequence. */
   if(!r->release_previous_sequence){if(frontier)*frontier=0x2a8578;return GH_PLAYER_INIT_UNSUPPORTED;}
   e=gh_actor_motion_begin_parameters_002a8578(&s->player_motion,&b,r->motion_offset,r->sequence_candidate_offset,r->start_frame_bits,r->blend_frames,r->flags);
   if(frontier)*frontier=s->player_motion.frontier;
  }
 return !e?GH_PLAYER_INIT_OK:e==GH_ACTOR_MOTION_UNSUPPORTED?GH_PLAYER_INIT_UNSUPPORTED:e==GH_ACTOR_MOTION_BOUNDS?GH_PLAYER_INIT_BOUNDS:GH_PLAYER_INIT_CALLEE;
}
static gh_player_init_status player_motion_advance(void*p,const gh_player_motion_request*r,uint32_t*frontier){
 gh_game_services*s=p;if(!player_motion_request_matches(s,r)||!r->rate)return GH_PLAYER_INIT_ARGUMENT;
 int ended=0;gh_actor_motion_status e=gh_actor_motion_step_00157c18(&s->player_motion,*r->rate,GH_STATIC_BINARY32_REFERENCE,&ended,frontier);
 return !e?GH_PLAYER_INIT_OK:e==GH_ACTOR_MOTION_UNSUPPORTED?GH_PLAYER_INIT_UNSUPPORTED:e==GH_ACTOR_MOTION_BOUNDS?GH_PLAYER_INIT_BOUNDS:GH_PLAYER_INIT_CALLEE;
}
static gh_player_frame_status player_frame_motion_advance(void*p,const gh_player_motion_request*r,int*ended,uint32_t*frontier){
 gh_game_services*s=p;if(!ended||!player_motion_request_matches(s,r)||!r->rate)return GH_PLAYER_FRAME_ARGUMENT;
 gh_actor_motion_status e=gh_actor_motion_step_00157c18(&s->player_motion,*r->rate,GH_STATIC_BINARY32_REFERENCE,ended,frontier);
 return !e?GH_PLAYER_FRAME_OK:e==GH_ACTOR_MOTION_UNSUPPORTED?GH_PLAYER_FRAME_UNSUPPORTED:GH_PLAYER_FRAME_CALLEE;
}
static gh_player_init_status player_motion_solve(void*p,const gh_player_motion_request*r,uint32_t*frontier){
 gh_game_services*s=p;if(!player_motion_request_matches(s,r)||!s->player_motion.complete)return GH_PLAYER_INIT_ARGUMENT;
 gh_motion_solve_bindings b={.model=&s->player_model,.work=s->b.work,.actors=&s->scene_tables,
  .ik_globals=s->b.motion_ik_globals,.atan_coeff=s->b.camera_envelope?s->b.camera_envelope->atan_coeff:NULL};
 gh_actor_motion_status e=gh_actor_motion_solve_00152a20(&s->player_motion_solve,&b,GH_STATIC_BINARY32_REFERENCE,frontier);
 return !e?GH_PLAYER_INIT_OK:e==GH_ACTOR_MOTION_UNSUPPORTED?GH_PLAYER_INIT_UNSUPPORTED:e==GH_ACTOR_MOTION_BOUNDS?GH_PLAYER_INIT_BOUNDS:GH_PLAYER_INIT_CALLEE;
}
static gh_r100_init_status room_r100_task_exec(void*,uint32_t,int32_t,uint32_t*);
static gh_player_abort_status stop_player_abort_voice(void *context,uint32_t handle,uint32_t *frontier)
{
 gh_game_services *s=context;
 if(!s->b.voices||!frontier)return GH_PLAYER_ABORT_ARGUMENT;
 const gh_voice_status status=gh_se_voice_stop_actor_002cbba0(s->b.voices,handle);
 *frontier=status?gh_se_voice_frontier(s->b.voices):0;
 return !status?GH_PLAYER_ABORT_OK:status==GH_VOICE_UNSUPPORTED?GH_PLAYER_ABORT_UNSUPPORTED:GH_PLAYER_ABORT_CALLEE;
}

static gh_player_abort_status exit_player_roulette(void *context,uint32_t handle,uint32_t *frontier)
{
 gh_game_services *s=context;
 if(!handle||handle>8||!frontier)return GH_PLAYER_ABORT_ARGUMENT;
 const gh_action_prompt *prompt;
 if(gh_action_button_prompt(&s->action_buttons,(gh_action_link){GH_ACTION_LINK_PROMPT,handle-1},&prompt)||
    prompt->kind!=3||prompt->life!=0x32||(prompt->flags&0x201u)!=1)
  return GH_PLAYER_ABORT_UNSUPPORTED;
 if(prompt->unknown35==1)gh_game_cockpit_roulette_flag(s->cockpit,0x20000000u);
 const gh_action_set_status status=gh_action_button_reset_entry_001f7228(&s->action_buttons,handle-1,frontier);
 return !status?GH_PLAYER_ABORT_OK:GH_PLAYER_ABORT_CALLEE;
}

static gh_soft_event_status room_soft_player_abort(void*p,uint32_t*frontier){
 gh_game_services*s=p;gh_player_abort_bindings b={.player=&s->title_player,.sound=s->b.sound_control,
  .action_exit_001f6f10=exit_player_roulette,.se_stop_002cbba0=stop_player_abort_voice,.context=s};
 gh_player_abort_status e=gh_game_player_abort_00126770(&s->soft_player_abort,&b);
 if(frontier)*frontier=s->soft_player_abort.frontier;
 return !e?GH_SOFT_EVENT_OK:e==GH_PLAYER_ABORT_UNSUPPORTED?GH_SOFT_EVENT_UNSUPPORTED:GH_SOFT_EVENT_CALLEE;
}
static gh_r100_script_status room_r100_register_other(void*p,uint32_t entry,int32_t argument,uint32_t*frontier){
 gh_r100_init_status e=room_r100_task_exec(p,entry,argument,frontier);
 return !e?GH_R100_SCRIPT_OK:e==GH_R100_INIT_UNSUPPORTED?GH_R100_SCRIPT_UNSUPPORTED:GH_R100_SCRIPT_CALLEE;
}
static gh_game_event_status room_soft_end_camera_pose(void *context,uint32_t *frontier)
{
 gh_game_services *s=context;
 if(!s->b.camera_follow){*frontier=0x13b9b8;return GH_EVENT_UNSUPPORTED;}
 gh_camera_status status=gh_game_camera_reset_pose_0013b9b8(s->b.camera_follow,s->b.init,frontier);
 return !status?GH_EVENT_OK:status==GH_CAMERA_UNSUPPORTED?GH_EVENT_UNSUPPORTED:GH_EVENT_CALLEE;
}
static gh_game_event_status room_soft_end_camera_switch(void *context,uint32_t *frontier)
{
 gh_game_services *s=context;
 *frontier=0x138d40;
 gh_camera_status status=gh_game_camera_restore_00138d40(s->b.init);
 if(!status)*frontier=0;
 return status?GH_EVENT_CALLEE:GH_EVENT_OK;
}
static gh_game_event_status room_soft_end_camera_both(void *context,uint32_t *frontier)
{
 gh_game_event_status status=room_soft_end_camera_pose(context,frontier);
 return status?status:room_soft_end_camera_switch(context,frontier);
}
static gh_game_event_status room_soft_end_clear(void *context,const gh_title_player_view *player,uint32_t *frontier)
{
 gh_game_services *s=context;
 if(player!=&s->title_player.view)return GH_EVENT_ARGUMENT;
 const gh_player_abort_bindings abort={.player=&s->title_player,.sound=s->b.sound_control};
 const gh_event_clear_bindings b={.event=s->b.event,.config=s->b.event_config,.init=s->b.init,
  .pools=&s->pools,.room_objects=s->room_objects,.player_abort=&abort,
  .camera_reset_0013b9b8_then_00138d40=room_soft_end_camera_both,.context=s};
 return gh_game_services_event_player_clear(s,&b,frontier);
}
static gh_game_event_status room_soft_end_enemies(void *context,uint32_t *frontier)
{
 gh_game_services *s=context;
 gh_actor_motion_status status=gh_enemy_ems_unsuspend_002948e8(s->enemies,s->b.init,frontier);
 return !status?GH_EVENT_OK:status==GH_ACTOR_MOTION_UNSUPPORTED?GH_EVENT_UNSUPPORTED:GH_EVENT_CALLEE;
}
static gh_game_event_status room_soft_end_object(void *context,uint32_t handle,gh_title_player_view *out,uint32_t *frontier)
{
 gh_game_services *s=context;
 gh_game_services_status status=gh_game_services_actor_view(s,handle,out);
 *frontier=status?0x14fd98:0;
 return !status?GH_EVENT_OK:status==GH_SERVICES_FRONTIER?GH_EVENT_UNSUPPORTED:GH_EVENT_CALLEE;
}
static gh_event_start_bindings room_soft_objects(gh_game_services *s)
{
 return (gh_event_start_bindings){.event=s->b.event,.config=s->b.event_config,.work=s->b.work,
  .tables=&s->scene_tables,.pools=&s->pools,.meshes=&s->meshes,.object_view=room_soft_end_object,.context=s};
}

static gh_soft_event_status room_soft_start_enemies(void *context,uint32_t *frontier)
{
 gh_game_services *s=context;
 gh_actor_motion_status status=gh_enemy_ems_set_suspend_002948e8(s->enemies,s->b.init,1,frontier);
 return !status?GH_SOFT_EVENT_OK:status==GH_ACTOR_MOTION_UNSUPPORTED?GH_SOFT_EVENT_UNSUPPORTED:GH_SOFT_EVENT_CALLEE;
}

static gh_soft_event_status room_soft_start_effects(void *context,uint32_t *frontier)
{
 const gh_event_start_bindings objects=room_soft_objects(context);
 return gh_soft_event_suspend_effects_002fa470(&objects,frontier);
}

static const gh_title_player_view *room_soft_player_weapon(gh_game_services *s)
{
 if(gh_om376_paper_room_supported(s->b.work->room)&&s->r105_paper.stage==6){
  const gh_om_object *paper=s->r105_paper.create.object;
  const gh_title_player_view *player=&s->title_player.view;
  if(paper&&paper->registered&&paper->runtime_created&&paper->runtime_kind==0x376&&
     player->bytes&&player->size>=0x6ac&&u32(player->bytes+0x6a8)==paper->handle)
   return &paper->view;
 }
 return s->player_weapon.current?s->player_weapon.current->weapon:NULL;
}

/* EventEnv update and release use the same hand-held OM as the soft-event
 * player path, including R002's room-local recreation from the paper flag. */
static const gh_event_weapon_slot *room_event_weapon_slot(gh_game_services *s)
{
 if(!s->player_weapon.known||s->player_weapon.current)return &s->player_weapon;
 gh_om_object *paper=s->r105_paper.create.object;
 if(!paper||room_soft_player_weapon(s)!=&paper->view||!paper->initialization||
    s->r105_paper.player!=&s->player_model)return &s->player_weapon;
 gh_actor_model *model=&paper->initialization->model;
 if(!model->ready||model->root.bytes!=paper->view.bytes||model->external_parent!=&s->player_model||
    model->external_owner!=paper||model->external_parent_index!=2)return &s->player_weapon;
 s->event_paper_attachment=(gh_event_weapon_attachment){.weapon=&paper->view,.weapon_model=model,
   .parent=&s->title_player.view,.parent_model=&s->player_model,.parent_part=2};
 s->event_paper_slot=(gh_event_weapon_slot){.known=1,.current=&s->event_paper_attachment};
 return &s->event_paper_slot;
}


static gh_soft_event_status room_soft_start_player(void *context,uint32_t *frontier)
{
 gh_game_services *s=context;
 const gh_title_player_view *const links[4]={
  s->work5f8?&s->work5f8->object.view:NULL,NULL,NULL,
  room_soft_player_weapon(s)};
 const uint8_t known_mask=s->player_weapon.known?15:7;
 return gh_soft_event_suspend_player_0012c7d0(&s->title_player.view,links,known_mask,frontier);
}

static gh_r100_script_status room_r100_start_soft(void *context,gh_soft_event_start *state,
 unsigned kind,uint32_t *frontier)
{
 gh_game_services *s=context;
 if(!s||!state||!frontier)return GH_R100_SCRIPT_ARGUMENT;
 *frontier=0x2c39b0;
 if(kind!=0)return GH_R100_SCRIPT_UNSUPPORTED;
 const gh_event_start_bindings objects=room_soft_objects(s);
 const gh_soft_event_kind0_bindings kind0={.room_objects=&objects,.suspend_enemies=room_soft_start_enemies,
  .suspend_effects=room_soft_start_effects,.suspend_player=room_soft_start_player};
 const gh_soft_event_bindings bindings={
  .scenario={s->scenario_scalar,sizeof(s->scenario_scalar),&s->scenario},.work=s->b.work,
  .player_abort_00126770=room_soft_player_abort,.context=s,.kind0=&kind0};
 gh_soft_event_status status=gh_scenario_start_soft_event_002c39b0(state,&bindings,(uint8_t)kind);
 *frontier=status?(state->frontier?state->frontier:0x2c39b0):0;
 return !status?GH_R100_SCRIPT_OK:status==GH_SOFT_EVENT_UNSUPPORTED?GH_R100_SCRIPT_UNSUPPORTED:GH_R100_SCRIPT_CALLEE;
}

static gh_r100_script_status room_r100_set_camera(void *context,gh_room_camera_start *state,
 const char *name,uint32_t *frontier)
{
 gh_game_services *s=context;
 *frontier=0x2c3ed8;
 if(!s->b.motion_camera||!s->b.display)return GH_R100_SCRIPT_UNSUPPORTED;
 gh_room_camera_bindings b={
  .scenario={s->scenario_scalar,sizeof(s->scenario_scalar),&s->scenario},.init=s->b.init,
  .motion=s->b.motion_camera,.work=s->b.work,
  .display={&s->b.display->screen_width,&s->b.display->screen_height_bits,
   &s->b.display->pixel_aspect_bits,&s->b.display->initialized,GH_CAMERA_NATIVE_BINARY32_REFERENCE}};
 if(gh_game_room_pack_bundle(&s->room_pack,&b.room)){*frontier=0x31ce38;return GH_R100_SCRIPT_CALLEE;}
 if(!b.motion->camera.constructed){
  gh_camera_status status=gh_game_motion_camera_construct_00138be8(b.motion,GH_CAMERA_NATIVE_BINARY32_REFERENCE);
  if(status){*frontier=0x138be8;return GH_R100_SCRIPT_CALLEE;}
 }
 int started;
 gh_camera_status status=gh_game_room_camera_start_002c3ed8(state,&b,name,&started,frontier);
 /* Original REL2B18 ignores the returned started flag. */
 return !status?GH_R100_SCRIPT_OK:status==GH_CAMERA_UNSUPPORTED?GH_R100_SCRIPT_UNSUPPORTED:GH_R100_SCRIPT_CALLEE;
}

static gh_r100_script_status room_r100_program_se(void *context,int16_t program,uint32_t *frontier)
{
 gh_game_services *s=context;
 *frontier=0x2cb8a0;
 const gh_sound_scene *scene=s->b.sound_scene;
 if(!s->b.voices||!scene||!scene->ready||scene->control!=s->b.sound_control||
    scene->registry!=s->b.audio_registry||!gh_audio_token_complete(scene->registry,scene->bank2))
  return GH_R100_SCRIPT_UNSUPPORTED;
 gh_voice_status status=gh_se_voice_bind_external_adx(s->b.voices,s->b.assets->dvd->disc,
     scene->control,s->b.boot_elf,s->b.boot_elf_bytes);
 if(!status)status=gh_se_voice_bind_origin_seh(s->b.voices,scene->seh.bytes,scene->seh.size);
 uint32_t handle=0;
 if(!status)status=gh_se_voice_request_origin(s->b.voices,2,program,-1,-1,0,0,&handle);
#ifdef GH_NATIVE_STARTUP_QA
 if(s->qa_r105_exit_fixture&&s->b.work->room==0x105)
  fprintf(stderr,"CPU_R105_NATIVE_SE program=%04x handle=%u status=%d\n",(unsigned)program,handle,status);
#endif
 if(!status){*frontier=0;return GH_R100_SCRIPT_OK;}
 const uint32_t original=gh_se_voice_frontier(s->b.voices);
 if(original)*frontier=original;
 return status==GH_VOICE_UNSUPPORTED?GH_R100_SCRIPT_UNSUPPORTED:GH_R100_SCRIPT_CALLEE;
}

static gh_game_event_status room_soft_end_player(void *context,const gh_title_player_view *player,uint32_t *frontier)
{
 gh_game_services *s=context;
 if(player!=&s->title_player.view||!s->player_weapon.known)return GH_EVENT_ARGUMENT;
 const gh_soft_player_suspend_links links={.child_5f8=s->work5f8?&s->work5f8->object.view:NULL,
  .weapon_6a8=room_soft_player_weapon(s),.known_mask=15};
 return gh_soft_player_unsuspend_0012c7d0(player,&links,frontier);
}
static gh_game_event_status room_soft_end_effects(void *context,uint32_t *frontier)
{
 const gh_event_start_bindings objects=room_soft_objects(context);
 gh_soft_event_status status=gh_soft_event_set_effect_suspend_002fa470(&objects,0,frontier);
 return !status?GH_EVENT_OK:status==GH_SOFT_EVENT_UNSUPPORTED?GH_EVENT_UNSUPPORTED:GH_EVENT_CALLEE;
}

static gh_r100_script_status room_r100_end_live_soft(void *context,gh_soft_event_end *state,uint32_t *frontier)
{
 gh_game_services *s=context;
 const gh_event_start_bindings objects=room_soft_objects(s);
 const gh_soft_end_bindings bindings={
  .scenario={s->scenario_scalar,sizeof(s->scenario_scalar),&s->scenario},.work=s->b.work,
  .current_player=&s->title_player.view,.room_objects=&objects,.player_clear_0012c418=room_soft_end_clear,
  .camera_pose_0013b9b8=room_soft_end_camera_pose,.camera_switch_00138d40=room_soft_end_camera_switch,
  .effect_unsuspend_002fa470=room_soft_end_effects,.enemies_unsuspend_002948e8=room_soft_end_enemies,
  .player_unsuspend_0012c7d0=room_soft_end_player,.context=s};
 gh_game_event_status status=gh_scenario_end_soft_event_002c3c10(state,&bindings,frontier);
 return !status?GH_R100_SCRIPT_OK:status==GH_EVENT_UNSUPPORTED?GH_R100_SCRIPT_UNSUPPORTED:GH_R100_SCRIPT_CALLEE;
}

static gh_r100_script_status room_r100_end_soft(void *context,uint32_t *frontier)
{
 gh_game_services *s=context;
 return room_r100_end_live_soft(s,&s->r100_soft_end,frontier);
}

static gh_game_event_status room_reserved_event_end_soft(void *context,uint32_t *frontier)
{
 gh_game_services *s=context;
 gh_r100_script_status status=room_r100_end_live_soft(s,&s->r003_soft_end,frontier);
 return status==GH_R100_SCRIPT_OK?GH_EVENT_OK:
  status==GH_R100_SCRIPT_UNSUPPORTED?GH_EVENT_UNSUPPORTED:GH_EVENT_CALLEE;
}

static gh_door_status room_event_begin_jump(void *context,uint16_t room,const float position[3],
 float yaw,uint8_t kind,uint32_t flags,uint8_t force,uint32_t *frontier)
{
 gh_game_services *s=context;
 if(!s->b.begin_event_room_jump){*frontier=0x2c4548;return GH_DOOR_UNSUPPORTED;}
 const gh_door_status status=s->b.begin_event_room_jump(s->b.event_room_context,room,position,yaw,kind,flags,force,frontier);
#ifdef GH_NATIVE_STARTUP_QA
 if(s->qa_r105_exit_fixture&&s->b.work->room==0x105&&room==2&&s->b.event_reserve){
  const gh_event_reserve_work *reserve=s->b.event_reserve;
  fprintf(stderr,"CPU_R105_JUMP_ROOM room=%u status=%d reserve=%04x/%u/%08x\n",
   room,status,reserve->destination_room,reserve->mode,reserve->flags);
 }
#endif
 return status;
}

static gh_scene_status room_reserved_event_init(void *context,uint32_t *frontier)
{
 gh_game_services *s=context;
 const gh_scene_scenario_view scenario={s->scenario_scalar,sizeof(s->scenario_scalar),&s->scenario};
 const gh_r003_bindings bindings={
  .module=&s->module,.heap=s->b.assets->heap,.disc=s->b.assets->dvd->disc,.scenario=scenario,
  .threads=s->scenario_threads,.reserve=s->b.event_reserve,
  .soft={.scenario=scenario,.work=s->b.work,.player_abort_00126770=room_soft_player_abort,.context=s},
  .play={.event=s->b.event,.config=s->b.event_config,.work=s->b.work,.player=&s->title_player.view,
   .fade=s->b.fade,.display=s->b.display,.elf=s->b.boot_elf,.elf_bytes=s->b.boot_elf_bytes},
  .end_soft=room_reserved_event_end_soft,.begin_jump=room_event_begin_jump,.context=s};
 const int r002=s->b.work->room==2;
 gh_r003_status status=r002?gh_r002_event_init_000001a0(&s->r003,&bindings):
  gh_r003_event_init_000001a0(&s->r003,&bindings);
 *frontier=status?(s->r003.frontier?s->r003.frontier:0x1a0):0;
 if(status){
  s->frontier=*frontier;
  s->frontier_module_code=s->r003.frontier?0:r002?6:3;
 }
 return !status?GH_SCENE_OK:status==GH_R003_UNSUPPORTED?GH_SCENE_UNSUPPORTED:GH_SCENE_CALLEE;
}

static gh_scene_status room_r002_move(void *context,uint32_t *frontier)
{
 const gh_game_services *s=context;
 if(!s||!s->r003.initialized||s->module.room!=2||s->module.move_offset!=0x208)
  return GH_SCENE_ARGUMENT;
 if(frontier)*frontier=0;
 /* The authenticated R002Move is JR RA / NOP. */
 return GH_SCENE_OK;
}

static gh_scene_status room_r003_move(void *context,uint32_t *frontier)
{
 gh_game_services *s=context;
 const gh_r003_status status=gh_r003_move_flags_000003d8(&s->r003,&s->om_factory,
  &s->b.init->random,s->b.frame_random,room_r100_om_view,s,frontier);
 if(!status)return GH_SCENE_OK;
 s->frontier=*frontier?*frontier:s->module.move_offset;
 s->frontier_module_code=s->frontier<0x100000?3:0;
 return status==GH_R003_UNSUPPORTED?GH_SCENE_UNSUPPORTED:GH_SCENE_CALLEE;
}

static gh_scene_status room_r105_status(gh_game_services *s,gh_scene_status status,uint32_t *frontier)
{
 if(!status)return GH_SCENE_OK;
 s->frontier=*frontier;
 s->frontier_module_code=*frontier&&*frontier<0x100000?4:0;
 return status;
}

static gh_player_frame_status release_player_paper(void *context,const gh_title_player_view *player,
 gh_core_save_view save,uint32_t *frontier)
{
 gh_game_services *s=context;
 if(!s||player!=&s->title_player.view||!gh_om376_paper_room_supported(s->b.work->room))
  return GH_PLAYER_FRAME_ARGUMENT;
 const gh_scene_placement_status status=gh_om376_release_paper_001cf6e0(&s->r105_paper,player,save,frontier);
 return !status?GH_PLAYER_FRAME_OK:status==GH_PLACE_UNSUPPORTED?GH_PLAYER_FRAME_UNSUPPORTED:GH_PLAYER_FRAME_CALLEE;
}

static gh_game_services_status bind_scene_om_factory(gh_game_services *s)
{
 if(s->om_factory.bound)return GH_SERVICES_OK;
 gh_scene_dynamic_status dynamic=gh_scene_dynamic_bind(&s->dynamic_objects,&s->scene_tables,s->b.assets->heap);
 if(dynamic)return fail(s,GH_SERVICES_ASSET,0x12ead8,dynamic);
 s->om_shapes=(gh_hitarea_shape_bindings){.pairs=&s->collision_pairs,.math=s->b.math_globals,.policy=GH_STATIC_BINARY32_REFERENCE};
 s->om_initializer=(gh_om_initializer_bindings){.work=s->b.work,.packet_heap=s->b.assets->heap,
  .fog_rgb=s->world_light.scalar+4,.init=s->b.init,.elf=s->b.boot_elf,.elf_bytes=s->b.boot_elf_bytes,
  .shapes=&s->om_shapes,.dynamic=&s->dynamic_objects,.door_shared=s->b.door_shared,
  .player=&s->title_player.view,.solids=&s->collision_solids,.save=s->b.save};
 gh_om_factory_bindings bindings={.pools=&s->pools,.work=s->b.work,.room_state=&s->room_state,
  .objects=s->objects,.room_objects=s->room_objects,.factory_serial=s->b.factory_serial,
  .resources=&s->object_resources,.initializer=&s->om_initializer};
 const gh_room_vector *registry=NULL;
 if(gh_room_registry_view(&s->pools,0x754c58,&registry))
  return fail(s,GH_SERVICES_ASSET,0x30fb70,GH_POOL_ARGUMENT);
 gh_scene_placement_status status=gh_om_factory_bind(&s->om_factory,&bindings,registry->capacity);
 return status?fail(s,GH_SERVICES_ASSET,0x30fb70,status):GH_SERVICES_OK;
}

static gh_scene_status room_r105_paper(void *context,uint32_t *frontier)
{
 gh_game_services *s=context;
 const gh_scene_placement_status status=gh_om376_set_paper_0012c8d8(&s->r105_paper,
  &s->om_factory,&s->player_model,s->b.save,&s->scene_tables,&s->r100.collision,frontier);
 if(!status)return GH_SCENE_OK;
 return status==GH_PLACE_UNSUPPORTED?GH_SCENE_UNSUPPORTED:GH_SCENE_CALLEE;
}

static gh_player_init_status restore_player_paper(void *context,const gh_title_player_view *player,
 uint32_t *frontier)
{
 gh_game_services *s=context;
 if(!s||!frontier||!gh_om376_paper_room_supported(s->b.work->room)||player!=&s->title_player.view||
    s->player_model.root.bytes!=player->bytes)return GH_PLAYER_INIT_UNSUPPORTED;
 /* Original 107F58 can create paper before the OMS placement loop. The
  * typed factory has to own that same object through the later loop. */
 gh_game_services_status status=bind_scene_om_factory(s);
 if(status){*frontier=s->frontier;return GH_PLAYER_INIT_CALLEE;}
 const gh_scene_status paper=room_r105_paper(s,frontier);
 return !paper?GH_PLAYER_INIT_OK:paper==GH_SCENE_UNSUPPORTED?GH_PLAYER_INIT_UNSUPPORTED:GH_PLAYER_INIT_CALLEE;
}

static gh_r100_init_status room_r100_collision(void *,const char *,int,uint32_t *);

static gh_scene_status room_r105_omsub(void *context,uint8_t *room_work,uint32_t *frontier)
{
 gh_game_services *s=context;
 if(room_work!=s->r105.work.pointer){*frontier=0x5f8;return GH_SCENE_ARGUMENT;}
 return gh_r105_gate_000005f8(&s->r105,&s->om_factory,room_r100_om_view,room_r100_collision,s,frontier);
}

static gh_r100_script_status room_r100_camera_ended(void *,int *,uint32_t *);
static gh_r100_script_status room_r100_checkpoint(void *,uint32_t *);
static gh_scene_status room_r105_tutorial(void *context,uint32_t *frontier);

static gh_scene_status room_r105_goto_player(void *context,uint8_t stamp,const float position[3],
 uint32_t *frontier)
{
 gh_game_services *s=context;
 const gh_actor_motion_status status=gh_enemy_ems_set_stamp_goto_00295f80(s->enemies,
  s->b.init,stamp,position,2,0,frontier);
 return !status?GH_SCENE_OK:status==GH_ACTOR_MOTION_UNSUPPORTED?GH_SCENE_UNSUPPORTED:GH_SCENE_CALLEE;
}

static gh_scene_status room_r105_jump_unsuspend(void *context,uint32_t *frontier)
{
 gh_game_services *s=context;
 const gh_game_event_status status=room_soft_end_player(s,&s->title_player.view,frontier);
 return status==GH_EVENT_OK?GH_SCENE_OK:
  status==GH_EVENT_UNSUPPORTED?GH_SCENE_UNSUPPORTED:GH_SCENE_CALLEE;
}

static gh_scene_status room_r105_mount_player(void *context,const uint32_t position[3],
 uint32_t *frontier)
{
 gh_game_services *s=context;
 *frontier=0x12ce60;
 const gh_title_player_view *aux=s->player_tail.active_child&&s->player_tail.child_original_return?
  &s->player_child.object.view:NULL;
 if(gh_title_player_set_script_position_0012ce60(&s->title_player.view,aux,position))
  return GH_SCENE_UNSUPPORTED;
 *frontier=0;
 return GH_SCENE_OK;
}

static gh_scene_status room_r105_refresh_mounted_player(void *context,uint32_t *frontier)
{
 gh_game_services *s=context;
 *frontier=0x149eb8;
 if(gh_actor_model_update_hierarchy_00149eb8(&s->player_model,GH_STATIC_BINARY32_REFERENCE)){
  if(s->player_model.frontier)*frontier=s->player_model.frontier;
  return GH_SCENE_CALLEE;
 }
 *frontier=0;
 return GH_SCENE_OK;
}

static gh_scene_status room_r105_init(void *context,uint32_t *frontier)
{
 gh_game_services *s=context;
#ifdef GH_NATIVE_STARTUP_QA
 /* The controlled exit starts from the original saved-open gate branch. */
 if(s->qa_r105_exit_fixture&&!s->qa_r105_exit_pose_applied&&!s->r105.stage&&
    s->room_state.active&&s->room_state.size>=8)
  s->room_state.active[7]|=0x10;
#endif
 const gh_r105_room_bindings bindings={
  .module=&s->module,.heap=s->b.assets->heap,.work=s->b.work,.room=&s->room_state,
  .scenario={s->scenario_scalar,sizeof(s->scenario_scalar),&s->scenario},
  .threads=s->scenario_threads,.at=&s->scene_at,.init=s->b.init,
  .player=&s->title_player.view,.save=s->b.save,.paper_0012c8d8=room_r105_paper,
  .omsub_000005f8=room_r105_omsub,.tutorial_000003b8=room_r105_tutorial,
  .goto_player_00295f80=room_r105_goto_player,.context=s,.enemies=&s->enemies,
  .start_soft_event=room_r100_start_soft,.set_room_camera=room_r100_set_camera,
  .gate_factory=&s->om_factory,.object_view=room_r100_om_view,.program_se=room_r100_program_se,
  .random=&s->b.init->random,.camera_ended=room_r100_camera_ended,
  .end_soft_event=room_r100_end_live_soft,.collision=room_r100_collision,
  .buttons=&s->action_buttons,.jump_unsuspend_player=room_r105_jump_unsuspend,
  .place_mounted_player=room_r105_mount_player,.refresh_mounted_player=room_r105_refresh_mounted_player,
  .fade=s->b.fade,.display=s->b.display,.reserve=s->b.event_reserve,.request_room=room_event_begin_jump};
 const gh_scene_status result=gh_r105_initialize_000001a8(&s->r105,&bindings);
 *frontier=result?(s->r105.frontier?s->r105.frontier:0x1a8):0;
 return room_r105_status(s,result,frontier);
}

static gh_scene_status room_r102_status(gh_game_services *s,uint32_t *frontier,
 uint32_t entry,gh_scene_status status)
{
 *frontier=entry;s->frontier=entry;
 s->frontier_module_code=entry&&entry<0x100000?7:0;
 return status;
}

static gh_scene_status room_r102_register_followups(gh_game_services *s,uint32_t *frontier);
static gh_scene_status room_r102_prepare_om094(gh_game_services *s,uint32_t *frontier);
static gh_scene_status room_r102_finish_init(gh_game_services *s,uint32_t *frontier);
static gh_scene_status room_r102_start_task(gh_game_services *,gh_native_task_entry,int32_t,
 uint32_t callsite,int32_t *output_slot,uint32_t *frontier);
static void room_r102_area11_followup_23b8(void *,int32_t);

static gh_scene_status room_r102_init(void *context,uint32_t *frontier)
{
 gh_game_services *s=context;
 gh_scene_status status=gh_r102_room_init_prefix_000001a8(&s->r102,&s->module,
  s->b.assets->heap,&s->om_factory,frontier);
 if(status)return room_r102_status(s,frontier,*frontier,status);
 status=gh_r102_room_prepare_om046_000002b8(&s->r102,
  room_r100_om_view,room_r100_collision,s,frontier);
 if(status==GH_SCENE_UNSUPPORTED&&*frontier==0x2f8){
  const gh_r102_om046_bindings task={
   .scenario={s->scenario_scalar,sizeof(s->scenario_scalar),&s->scenario},
   .threads=s->scenario_threads,.room=&s->room_state,.object_view=room_r100_om_view,
   .set_collision=room_r100_collision,.context=s
  };
  status=gh_r102_room_register_om046_00000898(&s->r102,&task,frontier);
  if(!status){*frontier=0x348;status=GH_SCENE_UNSUPPORTED;}
 }
 if(status==GH_SCENE_UNSUPPORTED&&*frontier==0x348){
  status=gh_r102_room_hide_om095_00000348(&s->r102,room_r100_om_view,s,frontier);
  if(!status){*frontier=0x370;status=GH_SCENE_UNSUPPORTED;}
 }
 if(status==GH_SCENE_UNSUPPORTED&&*frontier==0x370){
  const gh_r102_area11_bindings task={
   .scenario={s->scenario_scalar,sizeof(s->scenario_scalar),&s->scenario},
   .threads=s->scenario_threads,.room=&s->room_state,.init=s->b.init,.areas=&s->scene_at,
   .enemies=&s->enemies,.player=&s->title_player.view,
   .saved_route=room_r102_area11_followup_23b8,.saved_context=s
  };
  status=gh_r102_room_register_area11_00002508(&s->r102,&task,frontier);
  if(!status){*frontier=0x384;status=GH_SCENE_UNSUPPORTED;}
 }
 if(status==GH_SCENE_UNSUPPORTED&&*frontier==0x384){
  status=room_r102_register_followups(s,frontier);
  if(!status){*frontier=0x428;status=GH_SCENE_UNSUPPORTED;}
 }
 if(status==GH_SCENE_UNSUPPORTED&&*frontier==0x428){
  status=room_r102_prepare_om094(s,frontier);
  if(status)return room_r102_status(s,frontier,*frontier,status);
  status=gh_r102_room_create_mount_00000488(&s->r102,frontier);
  if(status)return room_r102_status(s,frontier,*frontier,status);
  status=gh_r102_room_preload_00000520(&s->r102,frontier);
  if(status)return room_r102_status(s,frontier,*frontier,status);
  status=room_r102_finish_init(s,frontier);
 }
 return room_r102_status(s,frontier,*frontier,status);
}

static gh_scene_status room_r102_move(void *context,uint32_t *frontier)
{
 gh_game_services *s=context;
 gh_room_state *room=&s->room_state;
 if(!room->active||room->size<0x1610)
  return room_r102_status(s,frontier,0x670,GH_SCENE_ARGUMENT);
 if(!(u32(room->active+4)&0x00400000u)){
  const gh_r100_roof_bindings query={.pools=&s->pools,.room=room,
   .object_view=room_r100_om_view,.context=s};
  int broken;
  uint32_t pc=0;
  const gh_r100_script_status status=gh_r100_roof_is_broken_002c5168(&query,"om057",&broken,&pc);
  if(status)return room_r102_status(s,frontier,pc?pc:0x2c5168,
     status==GH_R100_SCRIPT_UNSUPPORTED?GH_SCENE_UNSUPPORTED:GH_SCENE_CALLEE);
  if(broken==1){
   if(gh_enemy_set_data_appearance_00295458(room,0x0b,0))
    return room_r102_status(s,frontier,0x295458,GH_SCENE_CALLEE);
   w32(room->active+4,u32(room->active+4)|0x00400000u);
  }
 }
 return room_r102_status(s,frontier,0,GH_SCENE_OK);
}

static gh_scene_status r101_stop(gh_game_services *s,uint32_t *frontier,uint32_t pc)
{
 *frontier=pc;s->frontier=pc;s->frontier_module_code=pc<0x100000?5:0;
 return GH_SCENE_UNSUPPORTED;
}

/* Original displayOmLayer, following the canonical 754C58 identity. */
static gh_scene_status room_r101_om_layer(gh_game_services *s,unsigned layer,int show)
{
 const gh_room_vector *objects;
 if(gh_room_registry_view(&s->pools,0x754c58,&objects))return GH_SCENE_CALLEE;
 for(uint32_t i=0;i<objects->count;++i){
  uint32_t handle=u32((const uint8_t *)objects->allocation.pointer+4*i);
  gh_r100_om_view actor;
  if(room_r100_om_view(s,handle,&actor,NULL)||actor.size<0x300)
   return GH_SCENE_CALLEE;
  const unsigned type=actor.bytes[0x2fe]|(unsigned)actor.bytes[0x2ff]<<8;
  if(type>=0x300&&type<0x500&&actor.bytes[0x2fd]==layer){
   const uint32_t flags=u32(actor.bytes+0x250);
   w32((uint8_t *)actor.bytes+0x250,show?flags&~2u:flags|2u);
  }
 }
 return GH_SCENE_OK;
}

static int room_live_enemy_count(gh_game_services *s,uint32_t *count)
{
 const gh_game_init_state *init=s->b.init;
 *count=0;
 if(!init->enemy_top)return 0;
 unsigned index=0;
 while(index<64&&init->enemy_top!=init->enemies+index*20)++index;
 uint64_t visited=0;
 while(index!=UINT8_MAX){
  if(index>=64||(visited&(UINT64_C(1)<<index)))return 1;
  visited|=UINT64_C(1)<<index;
  const gh_game_title_player *actor=init->enemy_actors[index];
  index=init->enemy_next[index];
  if(!actor)continue;
  if(!actor->view.bytes||actor->view.size<0x642)return 1;
  const uint8_t *raw=actor->view.bytes;
  const unsigned kind=raw[0x2fe]|(unsigned)raw[0x2ff]<<8;
  if(kind<0x200||kind>=0x2a0)continue;
  int dead;
  if(gh_r100_enemy_dead_0028fb38(&s->room_state,actor,&dead))return 1;
  *count+=!dead;
 }
 return 0;
}

static void room_count_defeated_opponents(gh_game_services *s,void *work,uint32_t *fault)
{
 if(gh_task_threads_sleep(s->scenario_threads,1)!=GH_THREAD_OK)return;
 uint32_t count;
 if(room_live_enemy_count(s,&count)){
  *fault=0x28fb38;return;
 }
 uint8_t *saved_count=s->room_state.active+0x1610;
 const uint32_t initial=count+u32(saved_count);
 w32(work,initial);
 for(;;){
  if(room_live_enemy_count(s,&count)){
   *fault=0x28fb38;return;
  }
  w32(saved_count,u32(work)-count);
  if(gh_task_threads_sleep(s->scenario_threads,1)!=GH_THREAD_OK)return;
 }
}

static void room_r101_counter(void *context,int32_t argument)
{
 gh_game_services *s=context;
 (void)argument;
 room_count_defeated_opponents(s,s->r101_work.pointer,&s->r101_task_frontier);
}

static void room_r102_counter(void *context,int32_t argument)
{
 gh_game_services *s=context;
 (void)argument;
 /* REL2A50/2B00 use the same list, exclusion range and RoomSave free[0]. */
 room_count_defeated_opponents(s,s->r102.work.pointer,&s->r102.task_frontier);
}

static int room_r102_area_inside(gh_game_services *s,uint32_t area,int *inside)
{
 gh_at_native_unit unit;
 if(gh_scene_at_find_id_002c3118(&s->scene_at,area,&unit)){
  s->r102.task_frontier=0x2c3118;return 1;
 }
 if(unit.bytes&&unit.size<0x54){s->r102.task_frontier=0x2c0ed0;return 1;}
 *inside=unit.bytes&&(u32(unit.bytes+0x50)&0x80000000u);
 return 0;
}

static int room_r102_sleep(gh_game_services *s)
{
 if(gh_task_threads_sleep(s->scenario_threads,1)==GH_THREAD_OK)return 0;
 s->r102.task_frontier=0x2d5aa0;
 return 1;
}

static void room_r102_reinforce_2720(void *context,int32_t argument)
{
 gh_game_services *s=context;
 (void)argument;
 for(;;){
  int inside;
  if(room_r102_area_inside(s,0x14,&inside))return;
  if(inside&&(u32(s->room_state.active+4)&0x80000000u))break;
  if(room_r102_sleep(s))return;
 }
 const uint8_t stamps[]={0x15,0x16};
 for(unsigned i=0;i<2;++i){
  gh_enemy_ems_set_call call={0};
  gh_game_title_player *actor;
  uint32_t frontier=0;
  if(gh_enemy_ems_set_002952d0(s->enemies,&call,stamps[i],&actor,&frontier)){
   s->r102.task_frontier=frontier?frontier:0x2952d0;return;
  }
  w32(s->r102.work.pointer,u32(s->r102.work.pointer)+1);
 }
 if(!s->title_player.view.alias_f0){s->r102.task_frontier=0x27f0;return;}
 float position[3];
 memcpy(position,s->title_player.view.alias_f0,sizeof(position));
 for(unsigned i=0;i<2;++i){
  uint32_t frontier=0;
  if(gh_enemy_ems_set_stamp_goto_00295f80(s->enemies,s->b.init,stamps[i],position,2,0,&frontier)){
   s->r102.task_frontier=frontier?frontier:0x295f80;return;
  }
 }
}

static void room_r102_mount_2868(void *context,int32_t argument)
{
 gh_game_services *s=context;
 (void)argument;
 uint8_t *flags=s->room_state.active+4;
 for(;;){
  if(u32(flags)&0x10000000u){
   int inside;
   if(room_r102_area_inside(s,0x15,&inside))return;
   if(inside)break;
  }
  if(room_r102_sleep(s))return;
 }
 w32(flags,u32(flags)|0x00800000u);
 (void)gh_r102_room_mount_reinforcements_00002900(&s->r102,&s->r102.task_frontier);
}

static gh_object_step_status room_r102_mount_spawn(gh_game_services *s,uint8_t stamp,
 const float position[3],const float ground[3],uint32_t *frontier)
{
 gh_r102_room *room=&s->r102;
 if(!room->mount_spawn_step){
  gh_game_title_player *spawned;
  if(gh_enemy_ems_set_002952d0(s->enemies,&room->mount_spawn,stamp,&spawned,frontier)){
   if(!*frontier)*frontier=0x2952d0;
   return GH_OBJSTEP_UNSUPPORTED;
  }
  if(gh_enemy_manager_get_00290868(s->b.init,stamp,&room->mount_enemy)){
   *frontier=0x290868;return GH_OBJSTEP_UNSUPPORTED;
  }
  room->mount_spawn_step=1;
 }
 if(room->mount_spawn.stamp!=stamp){*frontier=0x1a7f58;return GH_OBJSTEP_ARGUMENT;}
 gh_game_title_player *actor=room->mount_enemy;
 if(room->mount_spawn_step==1){
  if(actor){
   uint32_t handle=0;
   if(gh_enemy_ems_handle_for_actor(s->enemies,actor,&handle)||!handle){
    *frontier=0x2959b0;return GH_OBJSTEP_UNSUPPORTED;
   }
   if(gh_enemy_ems_set_position_00276228(s->enemies,handle,position,frontier))return GH_OBJSTEP_UNSUPPORTED;
   gh_enemy_ems_live live;
   if(gh_enemy_ems_live_at(s->enemies,handle,&live)||live.actor!=actor||!live.model||
      gh_actor_model_update_hierarchy_00149eb8(live.model,GH_STATIC_BINARY32_REFERENCE)){
    *frontier=0x149eb8;return GH_OBJSTEP_UNSUPPORTED;
   }
  }
  room->mount_spawn_step=2;
 }
 if(room->mount_spawn_step==2){
  if(gh_enemy_ems_start_stamp_action_00295ce8(s->enemies,s->b.init,stamp,frontier))
   return GH_OBJSTEP_UNSUPPORTED;
  room->mount_spawn_step=3;
 }
 if(room->mount_spawn_step==3){
  if(gh_enemy_ems_set_stamp_keep_00296070(s->enemies,s->b.init,stamp,ground,frontier))
   return GH_OBJSTEP_UNSUPPORTED;
  room->mount_spawn_step=4;
 }
 if(actor){
  if(!actor->view.bytes||actor->view.size<0x550){*frontier=0x295b80;return GH_OBJSTEP_ARGUMENT;}
  w32(actor->view.bytes+0x54c,0x41f00000u);
 }
 if(gh_enemy_set_data_position_00295380(&s->room_state,stamp,ground)){
  *frontier=0x295380;return GH_OBJSTEP_UNSUPPORTED;
 }
 if(gh_enemy_set_data_appearance_00295458(&s->room_state,stamp,0)){
  *frontier=0x295458;return GH_OBJSTEP_UNSUPPORTED;
 }
 room->mount_spawn=(gh_enemy_ems_set_call){0};
 room->mount_enemy=NULL;
 room->mount_spawn_step=0;
 return GH_OBJSTEP_OK;
}

static gh_object_step_status room_r102_mount_release_001a7f58(void *context,gh_om_object *mount,
 uint8_t *stage,uint32_t *frontier)
{
 gh_game_services *s=context;
 *frontier=0;
 if(s->b.work->room!=0x102||s->r102.mount.object!=mount||!mount->view.alias_f0){
  *frontier=0x1a7f58;return GH_OBJSTEP_ARGUMENT;
 }
 uint8_t *bytes=mount->view.bytes;
 const unsigned first=bytes[0x611];
 if(first>=16){*frontier=0x1a7f58;return GH_OBJSTEP_ARGUMENT;}
 if(!bytes[0x612]||!bytes[0x615])return GH_OBJSTEP_OK;
 if(!*stage){
  if(room_r100_program_se(s,0x33,frontier))return GH_OBJSTEP_UNSUPPORTED;
  *stage=1;
 }
 float position[3],ground[3];
 memcpy(position,mount->view.alias_f0,sizeof(position));
 memcpy(ground,position,sizeof(ground));
 memcpy(ground+1,bytes+0x61c,4);
 for(unsigned i=0;i<16&&bytes[0x612]&&bytes[0x615];++i){
  const unsigned index=(first+i)%16;
  uint8_t *stamp=bytes+0x600+index;
  if(*stamp==UINT8_MAX)continue;
  if(*stamp<0x20||*stamp>0x22){*frontier=0x2952d0;return GH_OBJSTEP_UNSUPPORTED;}
  const gh_object_step_status status=room_r102_mount_spawn(s,*stamp,position,ground,frontier);
  if(status)return status;
  *stamp=UINT8_MAX;
  bytes[0x611]=(uint8_t)index;
  --bytes[0x612];
  --bytes[0x615];
 }
 return GH_OBJSTEP_OK;
}

static void room_r102_start_action_0798(void *context,int32_t argument)
{
 gh_game_services *s=context;
 (void)argument;
 do {
  if(room_r102_sleep(s))return;
  int inside;
  if(room_r102_area_inside(s,0x1b,&inside))return;
  if(inside)break;
 }while(1);
 gh_game_title_player *actor;
 if(gh_enemy_manager_get_00290868(s->b.init,10,&actor)){
  s->r102.task_frontier=0x295978;return;
 }
 if(!actor)return;
 int dead;
 if(gh_r100_enemy_dead_0028fb38(&s->room_state,actor,&dead)){
  s->r102.task_frontier=0x28fb38;return;
 }
 if(dead)return;
 if(actor->view.original_kind4ac==0x42c7f0){
  if(!actor->view.bytes||actor->view.size<0x16d4){s->r102.task_frontier=0x295f28;return;}
  if(u32(actor->view.bytes+0x16d0)&2u)return;
 }
 uint32_t frontier=0;
 if(gh_enemy_ems_start_stamp_action_00295ce8(s->enemies,s->b.init,10,&frontier))
  s->r102.task_frontier=frontier?frontier:0x295ce8;
}

static int room_r102_dead_stamp(gh_game_services *s,uint8_t stamp,int *dead)
{
 gh_game_title_player *actor;
 if(gh_enemy_manager_get_00290868(s->b.init,stamp,&actor)){
  s->r102.task_frontier=0x295978;return 1;
 }
 *dead=1;
 if(actor&&gh_r100_enemy_dead_0028fb38(&s->room_state,actor,dead)){
  s->r102.task_frontier=0x28fb38;return 1;
 }
 return 0;
}

/* SearchOM returns the canonical raw actor. Recover its registered handle
 * for the native model and ReleaseObj providers. */
static const gh_om_object *room_r102_om_object(const gh_game_services *s,const gh_r100_om_view *actor)
{
 for(size_t i=0;i<s->om_factory.count;++i){
  const gh_om_object *object=s->om_factory.objects+i;
  if(object->registered&&object->view.bytes==actor->bytes)return object;
 }
 return NULL;
}

static gh_actor_model *room_r102_om_model(gh_game_services *s,const gh_r100_om_view *actor)
{
 const gh_om_object *object=room_r102_om_object(s,actor);
 gh_actor_model *model=NULL;
 return object&&!gh_om_factory_model_at(&s->om_factory,object->handle,&model)?model:NULL;
}

static gh_scene_status room_r102_release_om094(gh_game_services *s,const gh_r100_om_view *actor,
 uint32_t call_address,uint32_t *frontier)
{
 const gh_om_object *object=room_r102_om_object(s,actor);
 if(!object||object->runtime_created||object->source.type!=0x356){
  *frontier=call_address;
  return GH_SCENE_UNSUPPORTED;
 }
 const gh_room_pool_status status=gh_om_retire_mark_00312708(&s->om_factory,object->handle,s->b.voices,frontier);
 if(status&&!*frontier)*frontier=call_address;
 return status?(status==GH_POOL_UNSUPPORTED?GH_SCENE_UNSUPPORTED:GH_SCENE_CALLEE):GH_SCENE_OK;
}

static void room_r102_split_om094_0938(void *context,int32_t argument)
{
 gh_game_services *s=context;
 (void)argument;
 gh_r100_om_view found;
 uint32_t pc=0;
 if(gh_r100_find_om_0014fd98(&s->pools,room_r100_om_view,s,"om094",&found,&pc)){
  s->r102.task_frontier=pc?pc:0x14fd98;return;
 }
 if(room_r100_program_se(s,0x0e,&pc)){
  s->r102.task_frontier=pc?pc:0x2cb8a0;return;
 }
 if(!found.bytes)return;
 if(found.size<0x5bc){s->r102.task_frontier=0x1b7680;return;}
 uint8_t *actor=(uint8_t *)found.bytes;
 uint64_t key;
 memcpy(&key,actor+0x540,sizeof(key));
 uint32_t changed;
 if(gh_collision_scroll_set_key_enabled_001331b8(&s->scene_tables,&s->r100.collision,key,0,&changed)){
  s->r102.task_frontier=0x1331b8;return;
 }
 if(actor[0x2b4]>=2){
  gh_actor_model *model=room_r102_om_model(s,&found);
  const gh_actor_part *parts[2];
  if(!model||gh_actor_model_part_at(model,0,parts)||gh_actor_model_part_at(model,1,parts+1)||
     !parts[0]->bytes||!parts[1]->bytes||!parts[0]->alias_f0||!parts[1]->alias_f0){
   s->r102.task_frontier=0x14bfe0;return;
  }
  for(size_t i=0;i<2;++i)w32(parts[i]->bytes+0x154,u32(parts[i]->bytes+0x154)|0x10u);
  w32(actor+0x5b8,u32(actor+0x5b8)|8u);
  float initial;
  memcpy(&initial,parts[1]->alias_f0,sizeof(initial));
  for(;;){
   float left,right;
   memcpy(&left,parts[0]->alias_f0,sizeof(left));
   memcpy(&right,parts[1]->alias_f0,sizeof(right));
   if(!isfinite(left)||!isfinite(right)){s->r102.task_frontier=0xb10;return;}
   volatile const float distance=right-initial;
   if(distance>5.0f)break;
   volatile const float next_left=left-0.1f,next_right=right+0.1f;
   left=next_left;right=next_right;
   memcpy(parts[0]->alias_f0,&left,sizeof(left));
   memcpy(parts[1]->alias_f0,&right,sizeof(right));
   if(room_r102_sleep(s))return;
  }
 }
 w32(actor+0x5a0,u32(actor+0x5a0)|1u);
 (void)room_r102_release_om094(s,&found,0xbb8,&s->r102.task_frontier);
}

static void room_r102_gate_bf8(void *context,int32_t argument)
{
 gh_game_services *s=context;
 (void)argument;
 for(;;){
  int dead2,dead16;
  if(room_r102_dead_stamp(s,2,&dead2)||room_r102_dead_stamp(s,0x10,&dead16))return;
  const uint32_t defeated=u32(s->room_state.active+0x1610);
  if(defeated>5&&defeated<=INT32_MAX&&(dead2||dead16)&&
     !(u32((const uint8_t*)s->b.work+0x5e4)&0x40000000u)){
   const gh_title_player_view *player=&s->title_player.view;
   if(!player->bytes||player->size<0x15f8){s->r102.task_frontier=0x12baf0;return;}
   const uint8_t *actor=player->bytes;
   if((u32(actor+0x15f4)&0x400u)||(!actor[0x2f4]&&actor[0x2f5]<4)){
    int inside;
    if(room_r102_area_inside(s,0x0f,&inside))return;
    if(inside)break;
   }
  }
  if(room_r102_sleep(s))return;
 }
 uint32_t pc=0;
 if(room_r100_checkpoint(s,&pc)){s->r102.task_frontier=pc?pc:0x2c05b0;return;}
 uint8_t *flags=s->room_state.active+4;
 w32(flags,u32(flags)|0x20000000u);
 if(gh_r100_scroll_layer_002db2b0(&s->pools,&s->meshes,10,1,&pc)){
  s->r102.task_frontier=pc?pc:0x2db2b0;return;
 }
 gh_soft_event_start start={0};
 if(room_r100_start_soft(s,&start,0,&pc)){s->r102.task_frontier=pc?pc:0x2c39b0;return;}
 gh_room_camera_start camera={0};
 if(room_r100_set_camera(s,&camera,"A00",&pc)){s->r102.task_frontier=pc?pc:0x2c3ed8;return;}
 if(room_r102_start_task(s,room_r102_split_om094_0938,0,0xd54,NULL,&s->r102.task_frontier))return;
 if(room_r100_program_se(s,0x32,&pc)){s->r102.task_frontier=pc?pc:0x2cb8a0;return;}
 const uint8_t stamps[]={0x19,0x1a,0x0a,0x13,0x3f};
 for(size_t i=0;i<sizeof(stamps);++i){
  gh_enemy_ems_set_call call={0};
  gh_game_title_player *actor;
  if(gh_enemy_ems_set_002952d0(s->enemies,&call,stamps[i],&actor,&pc)){
   s->r102.task_frontier=pc?pc:0x2952d0;return;
  }
  if(i<4)w32(s->r102.work.pointer,u32(s->r102.work.pointer)+1);
 }
 if(!s->title_player.view.alias_f0){s->r102.task_frontier=0x2db6b0;return;}
 for(size_t i=0;i<2;++i){
  float destination[3];
  memcpy(destination,s->title_player.view.alias_f0,sizeof(destination));
  if(gh_enemy_ems_set_stamp_goto_00295f80(s->enemies,s->b.init,stamps[i],destination,2,0,&pc)){
   s->r102.task_frontier=pc?pc:0x295f80;return;
  }
 }
 for(;;){
  int ended;
  if(room_r100_camera_ended(s,&ended,&pc)){s->r102.task_frontier=pc?pc:0x2c4040;return;}
  if(ended)break;
  if(room_r102_sleep(s))return;
 }
 gh_soft_event_end end={0};
 if(room_r100_end_live_soft(s,&end,&pc))s->r102.task_frontier=pc?pc:0x2c3c10;
}

static gh_scene_status room_r102_start_task(gh_game_services *s,gh_native_task_entry entry,
 int32_t argument,uint32_t callsite,int32_t *output_slot,uint32_t *frontier)
{
 const gh_scene_scenario_view scenario={s->scenario_scalar,sizeof(s->scenario_scalar),&s->scenario};
 int32_t slot;
 uint32_t pc=0;
 const gh_scenario_exec_status status=gh_scenario_exec_002c3890(scenario,s->scenario_threads,
  -1,entry,s,argument,&slot,&pc);
 if(!status){
  if(output_slot)*output_slot=slot;
  *frontier=0;return GH_SCENE_OK;
 }
 *frontier=pc?pc:callsite;
 return status==GH_SCENARIO_EXEC_MISSING?GH_SCENE_UNSUPPORTED:GH_SCENE_CALLEE;
}

/* REL1E00 refreshes cEmWrap::setMutekiTimer(5.0) once per Scenario visit. */
static void room_r102_invincible_1e00(void *context,int32_t stamp)
{
 gh_game_services *s=context;
 for(;;){
  gh_game_title_player *actor;
  if(gh_enemy_manager_get_00290868(s->b.init,(uint8_t)stamp,&actor)){
   s->r102.task_frontier=0x295978;return;
  }
  if(actor){
   if(!actor->view.bytes||actor->view.size<0x550){s->r102.task_frontier=0x295b80;return;}
   w32(actor->view.bytes+0x54c,0x40a00000);
  }
  if(room_r102_sleep(s))return;
 }
}

static int room_r102_intro_om099(gh_game_services *s,int beginning)
{
 gh_r100_om_view placed;
 uint32_t frontier=0;
 const gh_r100_script_status status=gh_r100_find_om_0014fd98(&s->pools,room_r100_om_view,s,
  "om099",&placed,&frontier);
 if(status){s->r102.task_frontier=frontier?frontier:0x14fd98;return 1;}
 if(!placed.bytes)return 0;
 const uint32_t offset=beginning?0x5b8:0x250;
 if(placed.size<offset+4){s->r102.task_frontier=beginning?0x1f00:0x21cc;return 1;}
 uint8_t *flags=(uint8_t *)placed.bytes+offset;
 w32(flags,u32(flags)|2u);
 return 0;
}

static int room_r102_set_lock_off(gh_game_services *s,uint8_t stamp,int enabled)
{
 uint32_t frontier;
 if(!gh_enemy_ems_set_stamp_lock_off_00295c10(s->b.init,stamp,enabled,&frontier))return 0;
 s->r102.task_frontier=frontier;
 return 1;
}

static int room_r102_intro_near(gh_game_services *s,int *near)
{
 gh_game_title_player *actor;
 if(gh_enemy_manager_get_00290868(s->b.init,2,&actor)){
  s->r102.task_frontier=0x295978;return 1;
 }
 const gh_title_player_view *player=&s->title_player.view;
 if(!player->bytes||player->size<0x108||!player->alias_f0||(actor&&!actor->view.alias_f0)){
  s->r102.task_frontier=0x1fb0;return 1;
 }
 gh_scene_ray_request ray={.mask=7};
 if(actor)memcpy(ray.from,actor->view.alias_f0,16); /* Otherwise original zero vector at 747B00. */
 memcpy(ray.to,player->alias_f0,16);
 const float x=ray.from[0]-ray.to[0],y=ray.from[1]-ray.to[1],z=ray.from[2]-ray.to[2];
 const float square=(x*x+y*y)+z*z;
 *near=0;
 if(square<900.0f){
  float yaw;
  memcpy(&yaw,player->bytes+0x104,sizeof(yaw));
  float threshold;
  const uint32_t threshold_bits=0x3dfa35dd;
  memcpy(&threshold,&threshold_bits,sizeof(threshold));
  int inside;
  uint32_t arc_frontier=0;
  if(gh_player_target_inside_arc_002da2d0(s->b.boot_elf,s->b.boot_elf_bytes,ray.to,ray.from,
    yaw,threshold,&inside,&arc_frontier)){
   s->r102.task_frontier=arc_frontier?arc_frontier:0x2da2d0;return 1;
  }
  if(inside){
   int hit;
   uint32_t frontier=0;
   if(gh_scene_ray_query_00131e10(&s->scene_tables,&ray,&hit,&frontier)){
    s->r102.task_frontier=frontier?frontier:0x131e10;return 1;
   }
   if(!hit){*near=1;return 0;}
  }
 }
 if(square<196.0f){
  ray.flags=0x14;
  int hit;
  uint32_t frontier=0;
  if(gh_scene_ray_query_00131e10(&s->scene_tables,&ray,&hit,&frontier)){
   s->r102.task_frontier=frontier?frontier:0x131e10;return 1;
  }
  *near=!hit;
 }
 return 0;
}

/* 296370 finds the authentic event, enters soft event kind4, then lets the
 * scene scheduler drive cEvent until the original bit0 clears. */
static int room_play_event_wait(gh_game_services *s,int32_t number,uint32_t *frontier)
{
 char path[32];
 if(gh_game_event_resource_path_00297540(number,path)){*frontier=0x296370;return 1;}
 size_t index;
 const gh_disc_error found=gh_disc_find(s->b.assets->dvd->disc,path,&index);
 if(found==GH_DISC_NOT_FOUND)return 0;
 if(found){*frontier=0x2011f8;return 1;}
 while(u32((const uint8_t *)s->b.work+0x5e4)&0x10u)if(gh_task_threads_sleep(s->scenario_threads,1)!=GH_THREAD_OK){*frontier=0x2d5aa0;return 1;}
 const gh_soft_event_bindings soft={
  .scenario={s->scenario_scalar,sizeof(s->scenario_scalar),&s->scenario},.work=s->b.work,
  .player_abort_00126770=room_soft_player_abort,.context=s};
 gh_soft_event_start start={0};
 if(gh_scenario_start_soft_event_002c39b0(&start,&soft,4)){
  *frontier=start.frontier?start.frontier:0x2c39b0;return 1;
 }
 const gh_event_play_start_bindings play={.event=s->b.event,.config=s->b.event_config,
  .work=s->b.work,.player=&s->title_player.view,.fade=s->b.fade,.display=s->b.display,
  .elf=s->b.boot_elf,.elf_bytes=s->b.boot_elf_bytes};
 uint32_t pc=0;
 if(gh_game_event_play_start_00296458(&play,number,&pc)){
  *frontier=pc?pc:0x296458;return 1;
 }
 while(s->b.event->fields.flags&1u)if(gh_task_threads_sleep(s->scenario_threads,1)!=GH_THREAD_OK){*frontier=0x2d5aa0;return 1;}
 gh_soft_event_end end={0};
 if(room_r100_end_live_soft(s,&end,&pc)){
  *frontier=pc?pc:0x2c3c10;return 1;
 }
 return 0;
}

static int room_r102_play_event_wait(gh_game_services *s,int32_t number)
{
 return room_play_event_wait(s,number,&s->r102.task_frontier);
}

static void room_r102_area11_followup_23b8(void *context,int32_t argument)
{
 gh_game_services *s=context;
 (void)argument;
 uint8_t *flags=s->room_state.active+4;
 if(!(u32(flags)&0x01000000u))return;
 for(;;){
  int inside;
  if(room_r102_area_inside(s,0x0b,&inside))return;
  if(inside)break;
  if(room_r102_sleep(s))return;
 }
 w32(flags,u32(flags)|0x01000000u);
 gh_at_native_unit unit;
 float destination[3]={0};
 if(gh_scene_at_find_id_002c3118(&s->scene_at,0x19,&unit)||
    gh_scene_at_center_002c0e78(unit,destination)){
  s->r102.task_frontier=0x2c2a48;return;
 }
 uint32_t pc=0;
 if(gh_enemy_ems_set_stamp_goto_00295f80(s->enemies,s->b.init,0x17,destination,2,0,&pc)){
  s->r102.task_frontier=pc?pc:0x295f80;return;
 }
 if(gh_task_threads_sleep(s->scenario_threads,30)){
  s->r102.task_frontier=0x2d5aa0;return;
 }
 if(!s->title_player.view.alias_f0){s->r102.task_frontier=0x2db6b0;return;}
 memcpy(destination,s->title_player.view.alias_f0,sizeof(destination));
 if(gh_enemy_ems_set_stamp_goto_00295f80(s->enemies,s->b.init,7,destination,2,0,&pc))
  s->r102.task_frontier=pc?pc:0x295f80;
}

static void room_r102_poison_0ff8(void *context,int32_t argument)
{
 gh_game_services *s=context;
 (void)argument;
 int32_t defense_slot;
 uint32_t *frontier=&s->r102.task_frontier;
 if(room_r102_start_task(s,room_r102_invincible_1e00,0x11,0x1044,&defense_slot,frontier))return;
 gh_game_title_player *actor;
 if(gh_enemy_manager_get_00290868(s->b.init,0x11,&actor)){
  *frontier=0x290868;return;
 }
 if(actor){
  if(!actor->view.bytes||actor->view.size<0x54c||actor->view.original_vtable214!=0x448ca0){
   *frontier=0x289068;return;
  }
  memcpy(actor->view.bytes+0x2f4,(const uint8_t[4]){0,5,0,0},4);
 }
 if(s->room_state.size<0x1618){*frontier=0x2bf108;return;}
 uint8_t *counter=s->room_state.active+0x1614;
 uint8_t *flags=s->room_state.active+4;
 w32(counter,0);
 for(;;){
  const uint32_t elapsed=u32(counter)+1;
  w32(counter,elapsed);
  if(elapsed>=12600)break;
  gh_game_title_player *live;
  if(gh_enemy_manager_get_00290868(s->b.init,0x11,&live)){
   *frontier=0x295978;return;
  }
  if(live){
   if(!live->view.bytes||live->view.size<0x54c){*frontier=0x295e00;return;}
   int16_t maximum;
   memcpy(&maximum,live->view.bytes+0x548,sizeof(maximum));
   volatile const float factor=(float)(12600u-elapsed)/12600.0f;
   int16_t remaining=(int16_t)((float)maximum*factor);
   if(remaining>=maximum)remaining=maximum;
   memcpy(live->view.bytes+0x54a,&remaining,sizeof(remaining));
  }
  if(u32(flags)&0x04000000u)break;
  if(room_r102_sleep(s))return;
 }
 if(!(u32(flags)&0x04000000u)){
  if(gh_enemy_manager_get_00290868(s->b.init,0x11,&actor)){
   *frontier=0x290868;return;
  }
  if(actor){
   if(!actor->view.bytes||actor->view.size<0x54c||actor->view.original_vtable214!=0x448ca0){
    *frontier=0x289080;return;
   }
   memcpy(actor->view.bytes+0x54a,(const uint8_t[2]){0,0},2);
   memcpy(actor->view.bytes+0x2f4,(const uint8_t[4]){0,5,2,0},4);
  }
  w32(flags,u32(flags)|0x08000000u);
 }
 if(defense_slot>=0&&gh_task_threads_cancel(s->scenario_threads,defense_slot))
  *frontier=0x2d59f0;
}

static int room_r102_area_center(gh_game_services *s,uint32_t id,float center[3])
{
 gh_at_native_unit unit;
 if(gh_scene_at_find_id_002c3118(&s->scene_at,id,&unit)||gh_scene_at_center_002c0e78(unit,center)){
  s->r102.task_frontier=0x2c2a48;return 1;
 }
 return 0;
}

static int room_r102_set_scroll_collision(gh_game_services *s,uint8_t stamp,int enabled)
{
 gh_game_title_player *actor;
 if(gh_enemy_manager_get_00290868(s->b.init,stamp,&actor)){
  s->r102.task_frontier=0x295978;return 1;
 }
 if(actor){
  if(!actor->view.bytes||actor->view.size<0x5a4){s->r102.task_frontier=0x295e78;return 1;}
  uint8_t *flags=actor->view.bytes+0x5a0;
  w32(flags,enabled?u32(flags)&~4u:u32(flags)|4u);
 }
 return 0;
}

static int room_r102_goto_area(gh_game_services *s,uint32_t area,uint32_t mode,uint32_t timer)
{
 float center[3]={0};
 if(room_r102_area_center(s,area,center))return 1;
 uint32_t pc=0;
 if(gh_enemy_ems_set_stamp_goto_timed_00295f80(s->enemies,s->b.init,0x12,center,mode,1,timer,&pc)){
  s->r102.task_frontier=pc?pc:0x295f80;return 1;
 }
 return 0;
}

static int room_r102_wait_goto(gh_game_services *s,int check_om046)
{
 for(;;){
  int finished;
  if(gh_enemy_ems_check_stamp_goto_00296018(s->b.init,0x12,&finished)){
   s->r102.task_frontier=0x296018;return 1;
  }
  if(finished)return 0;
  if(check_om046){
   uint32_t pc=0;
   const gh_r100_roof_bindings query={.pools=&s->pools,.room=&s->room_state,
    .object_view=room_r100_om_view,.context=s};
   if(gh_r100_roof_is_broken_002c5168(&query,"om046",&finished,&pc)){
    s->r102.task_frontier=pc?pc:0x2c5168;return 1;
   }
   if(finished)return 0;
  }
  if(room_r102_sleep(s))return 1;
 }
}

static int room_r102_wait_route_area(gh_game_services *s,uint32_t area,uint32_t flag)
{
 for(;;){
  int inside;
  if(room_r102_area_inside(s,area,&inside))return 1;
  if(inside||(u32((const uint8_t *)s->b.work+0x5ec)&flag))return 0;
  if(room_r102_sleep(s))return 1;
 }
}

/* Original OM094 part2 angular step in REL1220; the normal OM dispatcher
 * consumes the same part transform after each Scenario visit. */
static void room_r102_turn_om094_1220(void *context,int32_t opening)
{
 gh_game_services *s=context;
 gh_r100_om_view found;
 uint32_t pc=0;
 if(gh_r100_find_om_0014fd98(&s->pools,room_r100_om_view,s,"om094",&found,&pc)){
  s->r102.task_frontier=pc?pc:0x14fd98;return;
 }
 if(!found.bytes)return;
 if(found.size<0x2b5){s->r102.task_frontier=0x12b0;return;}
 if(found.bytes[0x2b4]<3)return;
 const uint32_t step_bits=opening==1?0xbeb2b8c2u:0x3eb2b8c2u;
 const uint32_t limit_bits=opening==1?0xbfc90fdbu:0;
 float delta,limit;
 memcpy(&delta,&step_bits,sizeof(delta));memcpy(&limit,&limit_bits,sizeof(limit));
 for(;;){
  gh_actor_model *model=room_r102_om_model(s,&found);
  const gh_actor_part *part;
  if(!model||gh_actor_model_part_at(model,2,&part)||!part->bytes){
   s->r102.task_frontier=0x14bfe0;return;
  }
  uint8_t *rotation=part->bytes+0x100;
  float angle;
  memcpy(&angle,rotation,sizeof(angle));
  if(!isfinite(angle)){s->r102.task_frontier=0x1300;return;}
  volatile float next=angle+delta;
  if(opening==1?next<limit:next>limit){w32(rotation,limit_bits);return;}
  angle=next;
  memcpy(rotation,&angle,sizeof(angle));
  if(room_r102_sleep(s))return;
 }
}

/* REL1BDC checks the original chair's break state before positioning the
 * runner. The retail caller measures horizontal distance only. */
static int room_r102_finish_route_chair(gh_game_services *s)
{
 uint32_t pc=0;
 int broken;
 const gh_r100_roof_bindings query={.pools=&s->pools,.room=&s->room_state,
  .object_view=room_r100_om_view,.context=s};
 if(gh_r100_roof_is_broken_002c5168(&query,"om058",&broken,&pc)){
  s->r102.task_frontier=pc?pc:0x2c5168;return 1;
 }
 if(broken)return 0;
 gh_game_title_player *runner;
 if(gh_enemy_manager_get_00290868(s->b.init,0x12,&runner)){
  s->r102.task_frontier=0x290868;return 1;
 }
 if(!runner)return 0;
 gh_r100_om_view chair;
 if(gh_r100_find_om_0014fd98(&s->pools,room_r100_om_view,s,"om058",&chair,&pc)){
  s->r102.task_frontier=pc?pc:0x14fd98;return 1;
 }
 const gh_om_object *object=room_r102_om_object(s,&chair);
 const gh_title_player_view *actor=&runner->view;
 if(!object||!object->view.alias_f0||!actor->alias_f0){s->r102.task_frontier=0x1c5c;return 1;}
 float origin[3],position[3];
 memcpy(origin,actor->alias_f0,sizeof(origin));
 memcpy(position,object->view.alias_f0,sizeof(position));
 const volatile float x=position[0]-origin[0],z=position[2]-origin[2];
 const volatile float xx=x*x,zz=z*z,distance=xx+zz;
 if(!isfinite(distance)){s->r102.task_frontier=0x1f7e28;return 1;}
 if(distance>2.25f)return 0;
 if(!actor->bytes||actor->size<0x16f0||actor->original_kind4ac!=0x42c7f0||
    u32(actor->bytes+0x564)!=0x20f||actor->bytes[0x640]!=0x12){
  s->r102.task_frontier=0x273678;return 1;
 }
 w32(actor->bytes+0x16ec,0);
 w32(actor->bytes+0x16d0,u32(actor->bytes+0x16d0)&~0x20000000u);
 memcpy(actor->bytes+0x490,position,sizeof(position));
 memcpy(actor->alias_f0,position,sizeof(position));
 w32(actor->bytes+0x104,u32(object->view.bytes+0x104));
 actor->bytes[0x1650]=5;
 w32(actor->bytes+0x2f4,0x00000100u);
 return 0;
}

static void room_r102_run_route_1710(gh_game_services *s)
{
 uint32_t pc=0;
 if(gh_enemy_ems_set_stamp_alpha_00296230(s->enemies,s->b.init,0x12,0,&pc)){
  s->r102.task_frontier=pc?pc:0x296230;return;
 }
 gh_game_title_player *actor;
 if(gh_enemy_manager_get_00290868(s->b.init,0x12,&actor)){
  s->r102.task_frontier=0x295978;return;
 }
 if(actor){
  if(!actor->view.bytes||actor->view.size<0x564){s->r102.task_frontier=0x295e38;return;}
  w32(actor->view.bytes+0x560,0x3bc);
 }
 if(gh_enemy_set_data_flags_00295498(&s->room_state,0x12,0)){
  s->r102.task_frontier=0x295498;return;
 }
 const uint32_t full_timer=0x497423feu;
 if(room_r102_goto_area(s,4,11,full_timer)||room_r102_wait_route_area(s,0x0c,0x80000000u)||
    room_r102_set_scroll_collision(s,0x12,1)||room_r102_goto_area(s,5,11,full_timer)||
    room_r102_wait_route_area(s,0x0e,0x40000000u)||room_r102_goto_area(s,0x0d,0,full_timer)||
    room_r102_wait_goto(s,1)||room_r102_goto_area(s,6,10,full_timer)||room_r102_wait_goto(s,0))return;
 int left;
 for(;;){
  int right;
  if(room_r102_area_inside(s,0x12,&right))return;
  if(right){left=0;break;}
  if(room_r102_area_inside(s,0x11,&left))return;
  if(left)break;
  if(room_r102_sleep(s))return;
 }
 const uint8_t first_stamp=left?0x1c:0x1e;
 for(uint8_t i=0;i<2;++i){
  gh_enemy_ems_set_call call={0};
  gh_game_title_player *spawned;
  if(gh_enemy_ems_set_002952d0(s->enemies,&call,(uint8_t)(first_stamp+i),&spawned,&pc)){
   s->r102.task_frontier=pc?pc:0x2952d0;return;
  }
  w32(s->r102.work.pointer,u32(s->r102.work.pointer)+1);
 }
 if(!left&&(room_r102_goto_area(s,8,0,full_timer)||room_r102_wait_goto(s,0)||
    room_r102_goto_area(s,9,0,full_timer)||room_r102_wait_goto(s,0)))return;
 if(room_r102_goto_area(s,7,11,full_timer)||room_r102_wait_goto(s,0)||
    room_r102_wait_route_area(s,0x13,0)||room_r102_goto_area(s,10,11,full_timer)||
    room_r102_wait_goto(s,0)||room_r102_finish_route_chair(s))return;
 uint8_t *flags=s->room_state.active+4;
 w32(flags,u32(flags)|0x10000000u);
 for(;;){
  int dead;
  if(room_r102_dead_stamp(s,0x12,&dead)||dead||room_r102_sleep(s))return;
 }
}

static void room_r102_saved_followup_1690(void *context,int32_t argument)
{
 gh_game_services *s=context;
 (void)argument;
 if(room_r102_sleep(s))return;
 uint32_t pc=0;
 if(gh_enemy_ems_set_stamp_alpha_00296230(s->enemies,s->b.init,0x12,0,&pc)){
  s->r102.task_frontier=pc?pc:0x296230;return;
 }
 float center[3]={0};
 if(room_r102_area_center(s,4,center))return;
 gh_game_title_player *actor;
 if(gh_enemy_manager_get_00290868(s->b.init,0x12,&actor)){
  s->r102.task_frontier=0x295978;return;
 }
 if(actor){
  uint32_t handle;
  if(gh_enemy_ems_handle_for_actor(s->enemies,actor,&handle)||!handle||
     gh_enemy_ems_set_position_00276228(s->enemies,handle,center,&pc)){
   s->r102.task_frontier=pc?pc:0x2959b0;return;
  }
  gh_enemy_ems_live live;
  if(gh_enemy_ems_live_at(s->enemies,handle,&live)||live.actor!=actor||!live.model||
     gh_actor_model_update_hierarchy_00149eb8(live.model,GH_STATIC_BINARY32_REFERENCE)){
   s->r102.task_frontier=0x149eb8;return;
  }
  if(!actor->view.bytes||actor->view.size<0x532){s->r102.task_frontier=0x295ee0;return;}
  actor->view.bytes[0x531]=4;
 }
 room_r102_run_route_1710(s);
}

static void room_r102_opening_followup_14c8(void *context,int32_t argument)
{
 gh_game_services *s=context;
 (void)argument;
 gh_game_title_player *actor;
 if(gh_enemy_manager_get_00290868(s->b.init,0x12,&actor)){
  s->r102.task_frontier=0x295978;return;
 }
 if(actor){
  if(!actor->view.bytes||actor->view.size<0x532){s->r102.task_frontier=0x295ee0;return;}
  actor->view.bytes[0x531]=4;
 }
 uint32_t pc=0;
 if(gh_enemy_ems_set_stamp_alpha_00296230(s->enemies,s->b.init,0x12,0,&pc)){
  s->r102.task_frontier=pc?pc:0x296230;return;
 }
 if(actor){
  if(actor->view.size<0x564){s->r102.task_frontier=0x295e38;return;}
  w32(actor->view.bytes+0x560,0x3bc);
 }
 if(room_r102_set_lock_off(s,0x12,1))return;
 if(gh_enemy_set_data_flags_00295498(&s->room_state,0x12,0)){
  s->r102.task_frontier=0x295498;return;
 }
 int32_t defense_slot;
 if(room_r102_start_task(s,room_r102_invincible_1e00,0x12,0x1544,&defense_slot,&s->r102.task_frontier))return;
 if(room_r102_goto_area(s,3,0,0x497423feu)||room_r102_set_scroll_collision(s,0x12,0)||
    room_r102_wait_goto(s,0))return;
 if(room_r102_start_task(s,room_r102_turn_om094_1220,1,0x15d0,NULL,&s->r102.task_frontier)||
    room_r102_goto_area(s,0x17,12,0x41f00000u)||room_r102_wait_goto(s,0))return;
 if(room_r102_start_task(s,room_r102_turn_om094_1220,0,0x1638,NULL,&s->r102.task_frontier))return;
 if(defense_slot>=0&&gh_task_threads_cancel(s->scenario_threads,defense_slot)){
  s->r102.task_frontier=0x2d59f0;return;
 }
 if(room_r102_set_lock_off(s,0x12,0))return;
 room_r102_run_route_1710(s);
}

static void room_r102_intro_1e40(void *context,int32_t argument)
{
 gh_game_services *s=context;
 (void)argument;
 if(room_r102_sleep(s)||room_r102_intro_om099(s,1))return;
 const uint8_t locked[]={2,0x10,0x12};
 for(size_t i=0;i<sizeof(locked);++i)if(room_r102_set_lock_off(s,locked[i],1))return;
 const uint8_t stamps[]={2,0x10,0x11,0x12};
 const uint32_t registrations[]={0x1f50,0x1f68,0x1f80,0x1f98};
 int32_t defense_slots[sizeof(stamps)];
 for(size_t i=0;i<sizeof(stamps);++i)
  if(room_r102_start_task(s,room_r102_invincible_1e00,stamps[i],registrations[i],&defense_slots[i],
     &s->r102.task_frontier))return;
 for(;;){
  int inside,near;
  if(room_r102_area_inside(s,0,&inside))return;
  if(inside)break;
  if(room_r102_intro_near(s,&near))return;
  if(near)break;
  if(room_r102_sleep(s))return;
 }
 if(gh_task_threads_sleep(s->scenario_threads,13)){
  s->r102.task_frontier=0x2d5aa0;return;
 }
 for(;;){
  const gh_title_player_view *player=&s->title_player.view;
  if(!player->bytes||player->size<0x15f8){s->r102.task_frontier=0x12baf0;return;}
  const uint8_t *actor=player->bytes;
  if(!(u32((const uint8_t *)s->b.work+0x5e4)&0x40000000u)&&
     ((u32(actor+0x15f4)&0x400u)||(!actor[0x2f4]&&actor[0x2f5]<4)))break;
  if(room_r102_sleep(s))return;
 }
 if(room_r102_intro_om099(s,0))return;
 if(!(u32((const uint8_t *)s->b.work+0x590)&0x200000u)&&room_r102_play_event_wait(s,0x1b))return;
 gh_r100_om_view placed;
 uint32_t frontier=0;
 if(gh_r100_find_om_0014fd98(&s->pools,room_r100_om_view,s,"om099",&placed,&frontier)){
  s->r102.task_frontier=frontier?frontier:0x2250;return;
 }
 if(placed.bytes){
  if(placed.size<0x5bc){s->r102.task_frontier=0x2260;return;}
  uint8_t *flags=(uint8_t *)placed.bytes;
  w32(flags+0x250,u32(flags+0x250)&~2u);
  w32(flags+0x5b8,u32(flags+0x5b8)&~2u);
 }
 if(gh_enemy_set_data_appearance_00295458(&s->room_state,2,0)){
  s->r102.task_frontier=0x295458;return;
 }
 if(room_r102_start_task(s,room_r102_area11_followup_23b8,0,0x22ac,NULL,&s->r102.task_frontier))return;
 uint8_t *flags=s->room_state.active+4;
 w32(flags,u32(flags)|0x80000000u);
 for(size_t i=0;i<sizeof(stamps);++i){
  if(defense_slots[i]>=0&&gh_task_threads_cancel(s->scenario_threads,defense_slots[i])){
   s->r102.task_frontier=0x2d59f0;return;
  }
 }
 for(size_t i=0;i<2;++i)if(room_r102_set_lock_off(s,stamps[i],0))return;
 for(size_t i=0;i<2;++i){
  gh_game_title_player *actor;
  if(gh_enemy_manager_get_00290868(s->b.init,stamps[i],&actor)){
   s->r102.task_frontier=0x295978;return;
  }
  if(actor&&actor->view.original_kind4ac==0x42c7f0&&
     gh_enemy_clear_set_flags_002736f8(&actor->view,&s->room_state,&s->r102.task_frontier))return;
 }
 if(room_r102_start_task(s,room_r102_opening_followup_14c8,0,0x2360,NULL,&s->r102.task_frontier))return;
 (void)room_r102_start_task(s,room_r102_poison_0ff8,0,0x2374,NULL,&s->r102.task_frontier);
}

static void room_r102_ticket_wait_1d48(void *context,int32_t argument)
{
 gh_game_services *s=context;
 (void)argument;
 while(u32((const uint8_t *)s->b.work+0x610)!=0x3bc)if(room_r102_sleep(s))return;
 uint8_t *flags=s->room_state.active+4;
 w32(flags,u32(flags)|0x12000000u);
 if(gh_game_cockpit_set_ticket_visible(s->cockpit,1))s->r102.task_frontier=0x1da8;
}

static int room_message_inline_002c41c8(gh_game_services *s,uint16_t message,uint32_t *frontier)
{
 const gh_scene_scenario_view scene={s->scenario_scalar,sizeof(s->scenario_scalar),&s->scenario};
 /* 2C40C8 with a NULL camera and inline mode0. 2C41C8 opens the room
  * message, enters mode2, then waits for the normal menu task to close it. */
 memcpy(scene.bytes+0xd8,&message,sizeof(message));
 w32(scene.bytes+0xdc,0);scene.bytes[0xe4]=scene.bytes[0xe5]=0;
 uint32_t pc=0;
 if(gh_scenario_message_open_002c42e0(scene,&s->scene_ui,s->b.work,message,&pc)){
  *frontier=pc?pc:0x2c42e0;return 1;
 }
 const gh_soft_event_bindings soft={.scenario=scene,.work=s->b.work,
  .player_abort_00126770=room_soft_player_abort,.context=s};
 gh_soft_event_start start={0};
 if(gh_scenario_start_soft_event_002c39b0(&start,&soft,2)){
  *frontier=start.frontier?start.frontier:0x2c39b0;return 1;
 }
 for(;;){
  int active;
  if(gh_scenario_message_active_002c4350(scene,&s->scene_ui,&active,&pc)){
   *frontier=pc?pc:0x2c4350;return 1;
  }
  if(!active)break;
  if(gh_task_threads_sleep(s->scenario_threads,1)!=GH_THREAD_OK){
   *frontier=0x2d5aa0;return 1;
  }
 }
 if(gh_scenario_message_close_002c4390(scene,&s->scene_ui,&pc)){
  *frontier=pc?pc:0x2c4390;return 1;
 }
 gh_soft_event_end end={0};
 if(room_r100_end_live_soft(s,&end,&pc)){
  *frontier=pc?pc:0x2c3c10;return 1;
 }
 return 0;
}

static int room_r102_create_ticket_0f7c(gh_game_services *s,const uint8_t *position)
{
 gh_om_object *ticket;
 uint32_t pc=0;
 if(gh_om_factory_create_0030fa98(&s->om_factory,&s->r102.ticket,0x3da,0xffff,&ticket)){
  s->r102.task_frontier=s->om_factory.frontier?s->om_factory.frontier:0x30fa98;return 1;
 }
 if(!ticket){s->r102.task_frontier=0x292650;return 1;}
 if(gh_om_factory_initialize_runtime_ticket_001c0128(&s->om_factory,ticket)){
  s->r102.task_frontier=s->om_factory.frontier?s->om_factory.frontier:0x1c0128;return 1;
 }
 float spawn[4];
 memcpy(spawn,position,sizeof(spawn));
 const volatile float raised=spawn[1]+0.5f;
 spawn[1]=raised;
 if(gh_om_r101_item_position_001c2630(ticket->initialization,(const uint8_t *)spawn)){
  s->r102.task_frontier=ticket->initialization->frontier?ticket->initialization->frontier:0x1c2630;return 1;
 }
 const gh_title_player_view *player=&s->title_player.view;
 if(!player->bytes||player->size<0x10c){s->r102.task_frontier=0x2db6b0;return 1;}
 memcpy(ticket->view.bytes+0x100,player->bytes+0x100,12);
 gh_actor_model *model;
 if(gh_om_factory_model_at(&s->om_factory,ticket->handle,&model)||
    gh_actor_model_update_hierarchy_00149eb8(model,GH_STATIC_BINARY32_REFERENCE)){
  s->r102.task_frontier=0x149eb8;return 1;
 }
 if(actor_sound_prepare(s,&s->om_sound,&pc)||!s->om_sound.bound){
  s->r102.task_frontier=pc?pc:0x2cba48;return 1;
 }
 const gh_motion_sound_request sound={.model=model,.actor=&ticket->view,.bank=0,.program=0xfa};
 uint32_t handle;
 if(gh_actor_attached_sound_motion(&s->om_sound,&sound,&handle,&pc)){
  s->r102.task_frontier=pc?pc:0x2cba48;return 1;
 }
 return 0;
}

static int room_r102_retire_ticket_source(gh_game_services *s,gh_game_title_player *source)
{
 uint8_t *room=s->room_state.active;
 if(!room||s->room_state.size<12||room[8]>64||12+(size_t)room[8]*24>s->room_state.size){
  s->r102.task_frontier=0x2befe8;return 1;
 }
 for(unsigned i=0;i<room[8];++i){
  uint8_t *row=room+12+(size_t)i*24;
  if(row[0x13]==0x11){row[0x14]=1;break;}
 }
 uint32_t handle,pc=0;
 if(gh_enemy_ems_handle_for_actor(s->enemies,source,&handle)||!handle){s->r102.task_frontier=0x295b38;return 1;}
 int removed;
 if(gh_enemy_ems_remove_00290770(s->enemies,&s->enemy_removal,handle,s->b.voices,&removed,&pc)){
  s->r102.task_frontier=pc?pc:0x290770;return 1;
 }
 s->enemy_removal=(gh_enemy_manager_remove){0};
 return 0;
}

static void room_r102_ticket_area_0eb8(void *context,int32_t argument)
{
 gh_game_services *s=context;
 (void)argument;
 uint8_t *flags=s->room_state.active+4;
 if(u32(flags)&0x08000000u){(void)room_message_inline_002c41c8(s,0x1001,&s->r102.task_frontier);return;}
 if(!(u32(flags)&0x02000000u)){(void)room_message_inline_002c41c8(s,0x1000,&s->r102.task_frontier);return;}
 const gh_scene_scenario_view scene={s->scenario_scalar,sizeof(s->scenario_scalar),&s->scenario};
 uint32_t pc=0;
 if(gh_scenario_message_close_002c4390(scene,&s->scene_ui,&pc)){
  s->r102.task_frontier=pc?pc:0x2c4390;return;
 }
 if(gh_game_cockpit_set_ticket_visible(s->cockpit,0)){
  s->r102.task_frontier=0xf28;return;
 }
 w32(flags,u32(flags)|0x04000000u);
 if(room_r102_play_event_wait(s,0x1c))return;
 gh_game_title_player *source;
 if(gh_enemy_manager_get_00290868(s->b.init,0x11,&source)||!source||!source->view.alias_f0){
  s->r102.task_frontier=0x295ab8;return;
 }
 if(room_r102_create_ticket_0f7c(s,source->view.alias_f0)||room_r102_retire_ticket_source(s,source))return;
 gh_at_native_unit unit;
 if(gh_scene_at_find_id_002c3118(&s->scene_at,0x1a,&unit)||
    (unit.bytes&&unit.size<0x4f)){s->r102.task_frontier=0x2c2838;return;}
 if(unit.bytes){
  if(unit.bytes[0x4e])unit.bytes[0x3b]=unit.bytes[0x4e];
  unit.bytes[0x4e]=0;
  w32(unit.bytes+0x3c,0);
  unit.bytes[0x44]=0;
 }
}

static gh_scene_at_status room_r102_ticket_unit(gh_game_services *s,gh_at_native_unit *unit,uint32_t *frontier)
{
 if(s->b.work->room!=0x102){*frontier=0x2c2138;return GH_AT_UNSUPPORTED;}
 const gh_scene_at_status status=gh_scene_at_find_id_002c3118(&s->scene_at,0x1a,unit);
 if(status){*frontier=0x2c3118;return status;}
 if(!unit->bytes||unit->size!=0x9c||unit->bytes[0x35]!=2||unit->bytes[0x39]!=4||
    unit->bytes[0x3a]!=2||unit->bytes[0x3b]!=8||unit->bytes[0x44]!=0xff||
    u32(unit->bytes+0x3c)!=0xeb8||u32(unit->bytes+0x40)){
  *frontier=0x2c2138;return GH_AT_UNSUPPORTED;
 }
 return GH_AT_OK;
}

static gh_scene_at_status room_action_jump_unit(gh_game_services *services,
 gh_at_native_unit *unit,uint32_t *frontier)
{
 *frontier=0x2c2078;
 const uint16_t room=services->b.work->room;
 if(room!=0x101&&room!=0x102&&room!=0x103)return GH_AT_UNSUPPORTED;
 const uint8_t index=room==0x102?2:1;
 const uint8_t kind=room==0x103?1:0;
 const gh_scene_at_status status=gh_scene_at_get_jump_002c2ae0(&services->scene_at,index,unit);
 if(status){*frontier=0x2c2ae0;return status;}
 const uint8_t *bytes=unit->bytes;
 if(!bytes||unit->size!=0x9c||bytes[0x35]!=1||bytes[0x39]!=4||
    bytes[0x3a]!=0x20||bytes[0x3b]!=8||u32(bytes+0x3c)||u32(bytes+0x40)||
    bytes[0x5e]!=kind||u32(bytes+0x70)||u32(bytes+0x74)||bytes[0x79]!=index||bytes[0x7a])
  return GH_AT_UNSUPPORTED;
 *frontier=0;
 return GH_AT_OK;
}

/* R101 Init attaches REL 06E8 to the closed JMP1. The same authored
 * volume uses the default 2C2078 door when the original save bit is set. */
static gh_scene_at_status r101_gate_unit(gh_game_services *s,gh_at_native_unit *unit,
 uint32_t *frontier)
{
 if(s->b.work->room!=0x101)return GH_AT_UNSUPPORTED;
 gh_scene_at_status status=gh_scene_at_get_jump_002c2ae0(&s->scene_at,1,unit);
 if(status){*frontier=0x2c2ae0;return status;}
 const uint8_t *row=unit->bytes;
 if(!row||unit->size!=0x9c||row[0x35]!=1||row[0x39]!=4||row[0x3a]!=0x20||
    row[0x3b]!=8||u32(row+0x3c)!=0x6e8||u32(row+0x40)||row[0x44]!=0xff||
    row[0x79]!=1||row[0x7a])return GH_AT_UNSUPPORTED;
 return GH_AT_OK;
}

static gh_scene_at_status r101_side_door_unit(gh_game_services *s,const void *identity,
 gh_at_native_unit *unit,uint32_t *frontier)
{
 *frontier=0x2c2078;
 if(s->b.work->room!=0x101||!identity)return GH_AT_UNSUPPORTED;
 for(unsigned index=2;index<=3;++index){
  gh_at_native_unit candidate;
  if(gh_scene_at_get_jump_002c2ae0(&s->scene_at,index,&candidate)){
   *frontier=0x2c2ae0;
   return GH_AT_UNSUPPORTED;
  }
  if(!candidate.bytes||candidate.bytes!=identity||candidate.size!=0x9c)continue;
  const uint8_t *row=candidate.bytes;
  uint32_t callback,argument;
  if(row[0x35]!=1||row[0x39]!=4||row[0x3a]!=0x20||row[0x3b]!=8||
     u32(row+0x3c)||u32(row+0x40)||row[0x5e]||row[0x79]!=index||row[0x7a]||
     gh_scene_at_door_func(&s->scene_at,candidate,&callback,&argument)||
     callback!=0x1490||argument!=index)return GH_AT_UNSUPPORTED;
  *unit=candidate;
  *frontier=0;
  return GH_AT_OK;
 }
 return GH_AT_UNSUPPORTED;
}

gh_scene_at_status gh_game_services_r101_action_offer(void *context,
 const gh_scene_at_task_request *request,uint32_t *frontier)
{
 gh_game_services *s=context;
 if(!s||!request||!frontier)return GH_AT_ARGUMENT;
 *frontier=0x1f7858;
 const int closed=request->entry_address==0x2c2138;
 if(s->b.work->room!=0x101||(!closed&&request->entry_address!=0x2c2078)||
    request->argument_mode!=2||request->raw_argument||request->group!=4||request->mode!=0x20)
  return GH_AT_UNSUPPORTED;
 gh_at_native_unit unit;
 gh_scene_at_status status=closed?r101_gate_unit(s,&unit,frontier):room_action_jump_unit(s,&unit,frontier);
 if(!closed&&(status||unit.bytes!=request->unit.bytes))
  status=r101_side_door_unit(s,request->unit.bytes,&unit,frontier);
 if(status)return status;
 if(request->unit.bytes!=unit.bytes||request->unit.size!=unit.size||request->unit.handle!=unit.handle){
  *frontier=0x1f7858;
  return GH_AT_UNSUPPORTED;
 }
 const gh_action_set_request action={.priority=request->group,.life=request->mode,
    .callback_id=request->entry_address,.argument=unit.bytes,.kind=2};
 int emitted;
 unsigned index;
 if(gh_action_button_set_001f7858(&s->action_buttons,&action,&emitted,&index,frontier)){
  if(!*frontier)*frontier=0x1f7858;
  return GH_AT_UNSUPPORTED;
 }
 return GH_AT_OK;
}

/* REL0528 activates five authored EMS stamps, then reapplies native escape
 * and drop fields because these enemies may have just been instantiated. */
static void room_r101_reinforce_00000528(void *context,int32_t argument)
{
 gh_game_services *s=context;
 gh_at_native_unit area;
 if(argument||gh_scene_at_find_id_002c3118(&s->scene_at,0,&area)||!area.bytes||area.size<0x4f){
  s->r101_task_frontier=0x2c2838;
  return;
 }
 uint8_t *flags=s->room_state.active+4;
 w32(flags,u32(flags)|0x01000000u);
 if(area.bytes[0x4e])area.bytes[0x3b]=area.bytes[0x4e];
 area.bytes[0x4e]=area.bytes[0x44]=0;
 w32(area.bytes+0x3c,0);
 static const uint8_t stamps[]={7,10,11,16,17};
 for(unsigned i=0;i<sizeof(stamps);++i){
  gh_enemy_ems_set_call call={0};
  gh_game_title_player *actor;
  uint32_t pc=0;
  if(gh_enemy_ems_set_002952d0(s->enemies,&call,stamps[i],&actor,&pc)){
   s->r101_task_frontier=pc?pc:0x2952d0;
   return;
  }
 }
 static const float escape[4]={-.53f,0,-49.22f,1};
 for(unsigned stamp=16;stamp<=17;++stamp){
  gh_game_title_player *actor=NULL;
  if(gh_enemy_manager_get_00290868(s->b.init,(uint8_t)stamp,&actor)||
     (actor&&gh_enemy2a2_set_escape_position_00289038(actor,escape))){
   s->r101_task_frontier=0x289038;
   return;
  }
 }
 static const uint8_t card_stamps[]={9,7};
 for(unsigned i=0;i<sizeof(card_stamps);++i){
  gh_game_title_player *actor=NULL;
  if(gh_enemy_manager_get_00290868(s->b.init,card_stamps[i],&actor)){
   s->r101_task_frontier=0x295e38;
   return;
  }
  if(actor)w32(actor->view.bytes+0x560,0x3e3);
 }
}

gh_scene_at_status gh_game_services_r101_area_dispatch(void *context,uint32_t entry,
 gh_at_native_unit area,uint8_t trigger,int *original_return,uint32_t *frontier)
{
 gh_game_services *s=context;
 if(!s||!original_return||!frontier)return GH_AT_ARGUMENT;
 *frontier=0x2c2138;
 if(s->b.work->room!=0x101||entry!=0x2c2138||!area.bytes||area.size!=0x9c||
    area.bytes[0x35]!=2||area.bytes[0x36]||area.bytes[0x3b]!=1||
    u32(area.bytes+0x3c)!=0x528||u32(area.bytes+0x40)||area.bytes[0x44]!=0xff)
  return GH_AT_UNSUPPORTED;
 gh_at_native_unit canonical;
 if(gh_scene_at_find_id_002c3118(&s->scene_at,0,&canonical)||
    canonical.handle!=area.handle||canonical.bytes!=area.bytes)
  return GH_AT_UNSUPPORTED;
 const gh_scene_scenario_view scene={s->scenario_scalar,sizeof(s->scenario_scalar),&s->scenario};
 int32_t slot;
 if(gh_scenario_exec_002c3890(scene,s->scenario_threads,-1,room_r101_reinforce_00000528,s,0,&slot,frontier))
  return GH_AT_UNSUPPORTED;
 if(slot>=0)w32(scene.bytes+0x58+(size_t)slot*8,trigger);
 *original_return=1;
 *frontier=0;
 return GH_AT_OK;
}

/* 2DB3E8 uses bit index 1 for the original gate seal scroll flag.
 * 2DB370 writes both RGB copies when the seals are made visible. */
static uint32_t room_r101_gate_seal_style(gh_game_services *s,unsigned layer,int enabled)
{
 const gh_room_vector *meshes;
 if(gh_room_registry_view(&s->pools,0x754c38,&meshes))return 0x2db3e8;
 for(uint32_t i=0;i<meshes->count;++i){
  const uint32_t handle=u32((const uint8_t *)meshes->allocation.pointer+4u*i);
  void *object=NULL;
  if(gh_room_object_lookup(&s->pools,handle,GH_ROOM_OBJECT_SCROLL_MESH,&s->meshes,&object))
   return 0x2db3e8;
  const gh_scene_mesh_object *mesh=object;
  if(!mesh->allocation.pointer||!mesh->constructor_complete)return 0x2db3e8;
  uint8_t *record=mesh->allocation.pointer;
  if(record[0x2fd]!=layer)continue;
  const uint32_t flags=u32(record+0x350);
  w32(record+0x350,enabled?flags|0x40000000u:flags&~0x40000000u);
  if(enabled)for(unsigned i=0;i<3;++i){
   w32(record+0x240+4*i,0x3f800000);
   w32(record+0x310+4*i,0x3f800000);
  }
 }
 return 0;
}

static int room_r101_gate_sleep(gh_game_services *s,int frames)
{
 if(gh_task_threads_sleep(s->scenario_threads,frames)==GH_THREAD_OK)return 0;
 s->r101_task_frontier=0x2d5aa0;
 return 1;
}

static int room_r101_gate_fade_seal(gh_game_services *s,unsigned layer,float amount)
{
 float alpha=1.0f;
 do{
  alpha-=amount;
  uint32_t pc;
  if(gh_r100_scroll_layer_alpha_002db318(&s->pools,&s->meshes,layer,alpha,&pc)){
   s->r101_task_frontier=pc?pc:0x2db318;return 1;
  }
  if(room_r101_gate_sleep(s,1))return 1;
 }while(alpha>0.1f);
 return 0;
}

/* REL06E8 card consumption, from the original camera A01 branch. */
static int room_r101_apply_gate_cards(gh_game_services *s,uint32_t cards)
{
 static const uint32_t seal_bits[]={0x08000000u,0x04000000u,0x02000000u};
 const uint32_t seals=0x0e000000u;
 uint8_t *flags=s->room_state.active+4;
 uint8_t *deposited=s->room_state.active+0x1614; /* 2BF108 free[1] */
 uint32_t pc=0;
 for(unsigned i=0;i<3;++i){
  const unsigned layer=0x1f+i;
  if(u32(flags)&seal_bits[i]){
   if(gh_r100_scroll_layer_002db2b0(&s->pools,&s->meshes,layer,0,&pc)){
    s->r101_task_frontier=pc?pc:0x2db2b0;return 1;
   }
   continue;
  }
  pc=room_r101_gate_seal_style(s,layer,1);
  if(pc){s->r101_task_frontier=pc;return 1;}
  if(gh_r100_scroll_layer_alpha_002db318(&s->pools,&s->meshes,layer,1.0f,&pc)){
   s->r101_task_frontier=pc?pc:0x2db318;return 1;
  }
 }
 if(room_message_inline_002c41c8(s,0x3002,&s->r101_task_frontier))return 1;
 const uint32_t total=u32(deposited)+cards;
 w32(deposited,total);
 w32(s->b.save.bytes+0x68,0); /* 1FA908 argument 0 is within the clamp. */
 if(room_r101_gate_sleep(s,15))return 1;
 if(total>=1&&total<=3)for(unsigned i=3-total;i<3;++i){
  const unsigned layer=0x1f+i;
  if(!(u32(flags)&seal_bits[i])){
   w32(flags,u32(flags)|seal_bits[i]);
   if(room_r100_program_se(s,(u32(flags)&seals)==seals?0x29:0x28,&pc)){
    s->r101_task_frontier=pc?pc:0x2cb8a0;return 1;
   }
   if(room_r101_gate_fade_seal(s,layer,0.03f))return 1;
  }
  if(gh_r100_scroll_layer_002db2b0(&s->pools,&s->meshes,layer,0,&pc)){
   s->r101_task_frontier=pc?pc:0x2db2b0;return 1;
  }
 }
 if(total>2){
  if(room_r101_gate_fade_seal(s,0x1e,0.02f))return 1;
  if(gh_r100_scroll_layer_002db2b0(&s->pools,&s->meshes,0x1e,0,&pc)){
   s->r101_task_frontier=pc?pc:0x2db2b0;return 1;
  }
 }
 if(room_r101_gate_sleep(s,15))return 1;
 for(;;){
  int ended;
  if(room_r100_camera_ended(s,&ended,&pc)){
   s->r101_task_frontier=pc?pc:0x2c4040;return 1;
  }
  if(ended)break;
  if(room_r101_gate_sleep(s,1))return 1;
 }
 for(unsigned layer=0x1f;layer<=0x21;++layer){
  pc=room_r101_gate_seal_style(s,layer,0);
  if(pc){s->r101_task_frontier=pc;return 1;}
 }
 if(total>2){
  gh_at_native_unit jump;
  if(r101_gate_unit(s,&jump,&pc)){
   s->r101_task_frontier=pc?pc:0x2c2ae0;return 1;
  }
  w32(flags,u32(flags)|0x10000000u);
  if(jump.bytes[0x4e])jump.bytes[0x3b]=jump.bytes[0x4e];
  jump.bytes[0x4e]=jump.bytes[0x44]=0;
  w32(jump.bytes+0x3c,0); /* Original AtDataReset 2C2808. */
 }
 return 0;
}

/* REL 06E8 applies the carried cards or plays camera A01 with message 3003. */
static void room_r101_gate_message_000006e8(void *context,int32_t argument)
{
 gh_game_services *s=context;
 uint32_t pc=0;
 gh_soft_event_start outer={0};
 gh_room_camera_start camera={0};
 gh_soft_event_end end={0};
 const uint16_t message=0x3003;
 const gh_scene_scenario_view scene={s->scenario_scalar,sizeof(s->scenario_scalar),&s->scenario};
 const gh_soft_event_bindings soft={.scenario=scene,.work=s->b.work,
  .player_abort_00126770=room_soft_player_abort,.context=s};
 gh_soft_event_start nested={0};
 if(argument){s->r101_task_frontier=0x6e8;return;}
 if(room_r100_start_soft(s,&outer,0,&pc)){s->r101_task_frontier=pc?pc:0x2c39b0;return;}
 if(room_r100_set_camera(s,&camera,"A01",&pc)){s->r101_task_frontier=pc?pc:0x2c3f10;return;}
 const uint32_t cards=u32(s->b.save.bytes+0x68);
 if(cards){
  if(room_r101_apply_gate_cards(s,cards))return;
  if(room_r100_end_live_soft(s,&end,&pc)){
   s->r101_task_frontier=pc?pc:0x2c3c10;return;
  }
  s->r101_gate_task_active=0;
  return;
 }
 memcpy(scene.bytes+0xd8,&message,sizeof(message));
 scene.bytes[0xe4]=scene.bytes[0xe5]=0;
 if(gh_scenario_message_open_002c42e0(scene,&s->scene_ui,s->b.work,message,&pc)){
  s->r101_task_frontier=pc?pc:0x2c42e0;return;
 }
 if(gh_scenario_start_soft_event_002c39b0(&nested,&soft,1)){
  s->r101_task_frontier=nested.frontier?nested.frontier:0x2c39b0;return;
 }
 for(;;){
  int active;
  if(gh_scenario_message_active_002c4350(scene,&s->scene_ui,&active,&pc)){
   s->r101_task_frontier=pc?pc:0x2c4350;return;
  }
  if(!active)break;
  if(gh_task_threads_sleep(s->scenario_threads,1)!=GH_THREAD_OK){
   s->r101_task_frontier=0x2d5aa0;return;
  }
 }
 if(gh_scenario_message_close_002c4390(scene,&s->scene_ui,&pc)){
  s->r101_task_frontier=pc?pc:0x2c4390;return;
 }
 if(room_r100_end_live_soft(s,&end,&pc)){
  s->r101_task_frontier=pc?pc:0x2c3c10;return;
 }
 end=(gh_soft_event_end){0};
 if(room_r100_end_live_soft(s,&end,&pc)){
  s->r101_task_frontier=pc?pc:0x2c3c10;return;
 }
 s->r101_gate_task_active=0;
}

gh_action_step_status gh_game_services_r101_gate_action(gh_game_services *s,void *argument,
 uint32_t *frontier)
{
 if(!s||!frontier)return GH_ACTION_STEP_ARGUMENT;
 *frontier=0x2c2138;
 gh_at_native_unit unit;
 if(r101_gate_unit(s,&unit,frontier)||unit.bytes!=argument)return GH_ACTION_STEP_UNSUPPORTED;
 *frontier=0x6e8;
 if(!s->b.save.bytes||s->b.save.size<0x6c||u32(s->b.save.bytes+0x68)>9||
    !s->room_state.active||s->room_state.size<0x1618)
  return GH_ACTION_STEP_UNSUPPORTED;
 if(!s->r101_gate_task_active){
  const gh_scene_scenario_view scene={s->scenario_scalar,sizeof(s->scenario_scalar),&s->scenario};
  int32_t slot;
  /* 2C215C sign-extends the JMP hook's signed FF task selector. */
  if(gh_scenario_exec_002c3890(scene,s->scenario_threads,-1,
     room_r101_gate_message_000006e8,s,0,&slot,frontier))
   return GH_ACTION_STEP_CALLEE;
  s->r101_gate_task_active=slot>=0;
 }
 *frontier=0;
 return GH_ACTION_STEP_OK;
}

gh_scene_at_status gh_game_services_r102_action_offer(void *context,
 const gh_scene_at_task_request *request,uint32_t *frontier)
{
 gh_game_services *s=context;
 if(!s||!request||!frontier)return GH_AT_ARGUMENT;
 *frontier=0;
 const int jump=request->entry_address==0x2c2078;
 if((!jump&&request->entry_address!=0x2c2138)||request->argument_mode!=2||request->raw_argument||
    request->group!=4||request->mode!=(jump?0x20:2)){*frontier=0x1f7858;return GH_AT_UNSUPPORTED;}
 gh_at_native_unit unit;
 const gh_scene_at_status resolved=jump?room_action_jump_unit(s,&unit,frontier):
    room_r102_ticket_unit(s,&unit,frontier);
 if(resolved)return resolved;
 if(request->unit.bytes!=unit.bytes||request->unit.size!=unit.size||request->unit.handle!=unit.handle){
  *frontier=0x1f7858;return GH_AT_UNSUPPORTED;
 }
 const gh_action_set_request action={.priority=request->group,.life=request->mode,
    .callback_id=request->entry_address,.argument=unit.bytes,.kind=2};
 int emitted;
 unsigned index;
 if(gh_action_button_set_001f7858(&s->action_buttons,&action,&emitted,&index,frontier)){
  if(!*frontier)*frontier=0x1f7858;
  return GH_AT_UNSUPPORTED;
 }
 return GH_AT_OK;
}

gh_action_step_status gh_game_services_r102_ticket_action(void *context,uint32_t entry,
 void *argument,uint32_t *frontier)
{
 gh_game_services *s=context;
 if(!s||!frontier)return GH_ACTION_STEP_ARGUMENT;
 *frontier=entry;
 if(entry!=0x2c2138)return GH_ACTION_STEP_UNSUPPORTED;
 gh_at_native_unit unit;
 if(room_r102_ticket_unit(s,&unit,frontier)||unit.bytes!=argument)return GH_ACTION_STEP_UNSUPPORTED;
 if(gh_scenario_clear_pad_0031b410(s->b.work)){
  *frontier=0x31b410;return GH_ACTION_STEP_CALLEE;
 }
 w32((uint8_t *)s->b.work+8,0);
 w32(unit.bytes+0x50,u32(unit.bytes+0x50)|0x02000000u);
 int32_t slot;
 if(room_r102_start_task(s,room_r102_ticket_area_0eb8,0,0x2c38d8,&slot,frontier))
  return GH_ACTION_STEP_CALLEE;
 const size_t caller=slot<0?0x50:0x58+(size_t)slot*8;
 if(caller>sizeof(s->scenario_scalar)-4){*frontier=0x2c2180;return GH_ACTION_STEP_BOUNDS;}
 w32(s->scenario_scalar+caller,0x747618);
 *frontier=0;
 return GH_ACTION_STEP_OK;
}

gh_action_step_status gh_game_services_room_jump_action(gh_game_services *services,
 const gh_game_door_bindings *bindings,void *argument,uint32_t *frontier)
{
 if(!services||!bindings||bindings->work!=services->b.work||!frontier)
  return GH_ACTION_STEP_ARGUMENT;
 gh_at_native_unit unit;
 gh_scene_at_status resolved=room_action_jump_unit(services,&unit,frontier);
 int side_door=0;
 if((resolved||unit.bytes!=argument)&&bindings->work->room==0x101){
  resolved=r101_side_door_unit(services,argument,&unit,frontier);
  side_door=resolved==GH_AT_OK;
 }
 if(resolved||unit.bytes!=argument){*frontier=0x2c2078;return GH_ACTION_STEP_UNSUPPORTED;}
 gh_boot_work *work=bindings->work;
 if(gh_scenario_clear_pad_0031b410(work)){*frontier=0x31b410;return GH_ACTION_STEP_CALLEE;}
 w32((uint8_t *)work+8,0);
 w32(unit.bytes+0x50,u32(unit.bytes+0x50)|0x02000000u);
 const uint32_t work_lock=work->room==0x102?1u:8u;
 if(u32((const uint8_t *)work+0x584)&work_lock)return GH_ACTION_STEP_OK;

 /* The default door units resolve the current authored JMP bundle. */
 gh_bundle room;
 gh_bundle_entry jump;
 if(gh_game_room_pack_bundle(&services->room_pack,&room)||gh_bundle_find(&room,"JMP",0,&jump)){
  *frontier=0x31ccf0;
  return GH_ACTION_STEP_CALLEE;
 }
 const gh_door_status status=gh_game_door_request_jump_002004c8(&services->door,bindings,
    &jump,unit.bytes[0x79],unit.bytes[0x5e],2,0);
 if(status){
  *frontier=services->door.frontier?services->door.frontier:0x2004c8;
  return status==GH_DOOR_UNSUPPORTED?GH_ACTION_STEP_UNSUPPORTED:GH_ACTION_STEP_CALLEE;
 }
 if(side_door){
  /* Original 2C4DD0 retains the REL callback on a same-room request. */
  w32(services->scenario_scalar+0xf4,0x1490);
  w32(services->scenario_scalar+0xf8,unit.bytes[0x79]);
 }
 return GH_ACTION_STEP_OK;
}

static gh_at_native_unit room_r103_message_area(gh_game_services *s,const void *argument)
{
 for(unsigned i=0;i<2;++i){
  const gh_at_native_unit unit=s->r103_message_areas[i];
  if(!unit.bytes||unit.bytes!=argument||unit.size!=0x9c)continue;
  const uint8_t *row=unit.bytes;
  if((row[0x34]&1)&&row[0x35]==4&&row[0x39]==1&&row[0x3a]==2&&row[0x3b]==8&&
     !u32(row+0x3c)&&row[0x5e]==i&&row[0x5f]==0x10&&!u32(row+0x68))return unit;
 }
 return (gh_at_native_unit){0};
}

gh_scene_at_status gh_game_services_r103_action_offer(void *context,
 const gh_scene_at_task_request *request,uint32_t *frontier)
{
 gh_game_services *s=context;
 if(!s||!request||!frontier)return GH_AT_ARGUMENT;
 *frontier=0x1f7858;
 if(s->b.work->room!=0x103||request->argument_mode!=2||request->raw_argument)
  return GH_AT_UNSUPPORTED;
 gh_at_native_unit unit;
 if(request->entry_address==0x2c2078&&request->group==4&&request->mode==0x20){
  const gh_scene_at_status status=room_action_jump_unit(s,&unit,frontier);
  if(status)return status;
 }else if(request->entry_address==0x2c2330&&request->group==1&&request->mode==2){
  unit=room_r103_message_area(s,request->unit.bytes);
 }else return GH_AT_UNSUPPORTED;
 if(!unit.bytes||unit.bytes!=request->unit.bytes||unit.size!=request->unit.size||
    unit.handle!=request->unit.handle){
  *frontier=0x1f7858;return GH_AT_UNSUPPORTED;
 }
 const gh_action_set_request action={.priority=request->group,.life=request->mode,
  .callback_id=request->entry_address,.argument=unit.bytes,.kind=2};
 int emitted;
 unsigned index;
 *frontier=0;
 if(gh_action_button_set_001f7858(&s->action_buttons,&action,&emitted,&index,frontier)){
  if(!*frontier)*frontier=0x1f7858;
  return GH_AT_UNSUPPORTED;
 }
 return GH_AT_OK;
}

static void room_r103_message_task_002c2368(void *context,int32_t message)
{
 gh_game_services *s=context;
 (void)room_message_inline_002c41c8(s,(uint16_t)message,&s->r103_task_frontier);
}

gh_action_step_status gh_game_services_r103_message_action(void *context,void *argument,
 uint32_t *frontier)
{
 gh_game_services *s=context;
 if(!s||!frontier)return GH_ACTION_STEP_ARGUMENT;
 *frontier=0x2c2330;
 if(s->b.work->room!=0x103)return GH_ACTION_STEP_UNSUPPORTED;
 const gh_at_native_unit unit=room_r103_message_area(s,argument);
 if(!unit.bytes)return GH_ACTION_STEP_UNSUPPORTED;
 if(gh_scenario_clear_pad_0031b410(s->b.work)){
  *frontier=0x31b410;return GH_ACTION_STEP_CALLEE;
 }
 w32((uint8_t *)s->b.work+8,0);
 w32(unit.bytes+0x50,u32(unit.bytes+0x50)|0x02000000u);
 /* 2C2330 schedules slot 1 via 2D5580, then clears its Scenario metadata. */
 const gh_scene_scenario_view scene={s->scenario_scalar,sizeof(s->scenario_scalar),&s->scenario};
 const uint16_t message=(uint16_t)(unit.bytes[0x5e]|(unsigned)unit.bytes[0x5f]<<8);
 int32_t slot;
 uint32_t pc=0;
 if(gh_scenario_exec_002c3890(scene,s->scenario_threads,1,room_r103_message_task_002c2368,
    s,message,&slot,&pc)){
  *frontier=pc?pc:0x2c3920;return GH_ACTION_STEP_CALLEE;
 }
 *frontier=0;
 return GH_ACTION_STEP_OK;
}

static gh_scene_status room_r102_set_escape_position(gh_game_services *s,int start,uint32_t *frontier)
{
 gh_game_title_player *actor;
 if(gh_enemy_manager_get_00290868(s->b.init,0x0e,&actor)){
  *frontier=0x295978;return GH_SCENE_CALLEE;
 }
 if(actor&&actor->view.original_kind4ac==0x448a00){ /* Original 44A878 has the same cEma2 name. */
  if(!actor->view.alias_f0){*frontier=0x295ab8;return GH_SCENE_ARGUMENT;}
  float pos[4];
  memcpy(pos,actor->view.alias_f0,sizeof(pos));
  if(gh_enemy2a2_set_escape_position_00289038(actor,pos)){
   *frontier=0x289038;return GH_SCENE_CALLEE;
  }
  if(start){
   uint8_t *flags=actor->view.bytes+0x15b0;
   w32(flags,u32(flags)|8u); /* 296180 -> 289028. */
  }
 }
 *frontier=0;
 return GH_SCENE_OK;
}

static void room_r102_escape_wait_29a0(void *context,int32_t argument)
{
 gh_game_services *s=context;
 (void)argument;
 for(;;){
  int dead;
  if(room_r102_dead_stamp(s,0x0f,&dead))return;
  if(dead)break;
  if(room_r102_dead_stamp(s,0x0e,&dead))return;
  if(dead)break;
  if(room_r102_sleep(s))return;
 }
 int dead;
 if(room_r102_dead_stamp(s,0x0e,&dead)||dead)return;
 (void)room_r102_set_escape_position(s,1,&s->r102.task_frontier);
}

static gh_scene_status room_r102_exit_0700(void *context,uint32_t *frontier)
{
 gh_game_services *s=context;
 *frontier=0;
 uint8_t *page=s->room_state.active;
 if(!page||s->room_state.size<0x60c)return GH_SCENE_ARGUMENT;
 const uint32_t flags=u32(page+4);
 if((s->b.work->flags&0x04000000u)||!(flags&0x80000000u))return GH_SCENE_OK;
 if(page[8]>64){*frontier=0x2bef08;return GH_SCENE_ARGUMENT;}
 if(!s->b.save.bytes||s->b.save.size<GH_CORE_SAVE_BYTES)return GH_SCENE_ARGUMENT;
 w32(page+4,flags|0x40000000u);
 for(unsigned i=0;i<page[8];++i){
  uint8_t *enemy=page+0xc+(size_t)i*24;
  if(enemy[0x13]!=0x11)continue;
  enemy[0x14]=1;
  break;
 }
 if(!(flags&0x0c000000u)){
  w32(page+4,flags|0x48000000u);
  uint16_t killed;
  static const unsigned counters[]={0x96,0xbb6};
  for(size_t i=0;i<sizeof(counters)/sizeof(*counters);++i){
   uint8_t *value=s->b.save.bytes+counters[i];
   memcpy(&killed,value,sizeof(killed));
   ++killed;
   memcpy(value,&killed,sizeof(killed));
  }
 }
 return GH_SCENE_OK;
}

static gh_scene_status room_r102_finish_init(gh_game_services *s,uint32_t *frontier)
{
 if(!s->r102.item3bc_complete||!s->room_state.active||s->room_state.size<8)return GH_SCENE_ARGUMENT;
 const uint32_t flags=u32(s->room_state.active+4);
 if(s->r102.tail_stage==0){
  if(!(flags&0x80000000u)){
   const gh_scene_status status=room_r102_start_task(s,room_r102_intro_1e40,0,0x540,NULL,frontier);
   if(status)return status;
  }else if(!(flags&0x40000000u)){
   if(!s->r102.saved_intro_registered){
    const gh_scene_status status=room_r102_start_task(s,room_r102_saved_followup_1690,0,0x568,NULL,frontier);
    if(status)return status;
    s->r102.saved_intro_registered=1;
   }
   const gh_scene_status status=room_r102_start_task(s,room_r102_poison_0ff8,0,0x57c,NULL,frontier);
   if(status)return status;
  }
  s->r102.tail_stage=1;
 }
 if(s->r102.tail_stage==1){
  s->scenario_exit=(gh_scene_hook){GH_SCENE_HOOK_NATIVE,room_r102_exit_0700,s};
  w32(s->scenario_scalar+0xec,0x700);w32(s->scenario_scalar+0xf0,0);
  s->r102.tail_stage=2;
 }
 if(s->r102.tail_stage==2){
  const gh_scene_status status=room_r102_start_task(s,room_r102_ticket_wait_1d48,0,0x5a4,NULL,frontier);
  if(status)return status;
  s->r102.tail_stage=3;
 }
 if(s->r102.tail_stage==3){
  if(!(flags&0x40000000u)){
   gh_at_native_unit unit;
   if(gh_scene_at_find_id_002c3118(&s->scene_at,0x1a,&unit)||
      gh_scene_at_set_jump_info_002c2750(&s->scene_at,unit,0xff,0xeb8,0,0)){
    *frontier=0x2c2790;return GH_SCENE_CALLEE;
   }
  }
  s->r102.tail_stage=4;
 }
 if(s->r102.tail_stage==4){
  const gh_id_error status=gh_game_cockpit_load_ticket_0013e418(s->cockpit,s->b.id,
   s->b.assets->dvd,s->b.assets->heap,s->b.work->language);
  if(status){
   *frontier=gh_game_cockpit_frontier(s->cockpit);
   if(!*frontier)*frontier=0x13e418;
   return status==GH_ID_UNSUPPORTED?GH_SCENE_UNSUPPORTED:GH_SCENE_CALLEE;
  }
  s->r102.tail_stage=5;
 }
 if(s->r102.tail_stage==5){
  const gh_scene_status status=room_r102_set_escape_position(s,0,frontier);
  if(status)return status;
  s->r102.tail_stage=6;
 }
 if(s->r102.tail_stage==6){
  int dead;
  if(room_r102_dead_stamp(s,0x0f,&dead)){
   *frontier=s->r102.task_frontier;return GH_SCENE_CALLEE;
  }
  if(!dead){
   const gh_scene_status status=room_r102_start_task(s,room_r102_escape_wait_29a0,0,0x634,NULL,frontier);
   if(status)return status;
  }
  s->r102.tail_stage=7;
 }
 if(s->r102.tail_stage==7){
  if(gh_r100_scroll_layer_002db2b0(&s->pools,&s->meshes,10,(flags&0x20000000u)!=0,frontier))
   return GH_SCENE_CALLEE;
  s->r102.tail_stage=8;
 }
 *frontier=0;
 return GH_SCENE_OK;
}

static gh_scene_status room_r102_prepare_om094(gh_game_services *s,uint32_t *frontier)
{
 if(s->r102.om094_complete){*frontier=0;return GH_SCENE_OK;}
 *frontier=0x428;
 if(!s->room_state.active||s->room_state.size<0x1614)return GH_SCENE_ARGUMENT;
 gh_r100_om_view placed;
 const gh_r100_script_status lookup=gh_r100_find_om_0014fd98(&s->pools,room_r100_om_view,s,
  "om094",&placed,frontier);
 if(lookup)return lookup==GH_R100_SCRIPT_UNSUPPORTED?GH_SCENE_UNSUPPORTED:GH_SCENE_CALLEE;
 if(u32(s->room_state.active+4)&0x20000000u){
  if(placed.bytes){
   const gh_scene_status status=room_r102_release_om094(s,&placed,0x47c,frontier);
   if(status)return status;
  }
 }else{
  if(placed.bytes&&(placed.size<0x5bc||placed.bytes[0x2fe]!=0x56||placed.bytes[0x2ff]!=3)){
   *frontier=0x460;return GH_SCENE_UNSUPPORTED;
  }
  const gh_scene_scenario_view scenario={s->scenario_scalar,sizeof(s->scenario_scalar),&s->scenario};
  int32_t slot;
  uint32_t pc=0;
  const gh_scenario_exec_status status=gh_scenario_exec_002c3890(scenario,s->scenario_threads,
   -1,room_r102_gate_bf8,s,0,&slot,&pc);
  if(status){
   *frontier=pc?pc:0x454;
   return status==GH_SCENARIO_EXEC_MISSING?GH_SCENE_UNSUPPORTED:GH_SCENE_CALLEE;
  }
  if(placed.bytes){
   uint8_t *flags=(uint8_t*)placed.bytes+0x5b8;
   w32(flags,u32(flags)|8u);
  }
 }
 s->r102.om094_complete=1;
 *frontier=0;
 return GH_SCENE_OK;
}

static gh_scene_status room_r102_register_followups(gh_game_services *s,uint32_t *frontier)
{
 if(!s->r102.work.pointer||!s->room_state.active||s->room_state.size<0x1614)
  return GH_SCENE_ARGUMENT;
 const gh_scene_scenario_view scenario={s->scenario_scalar,sizeof(s->scenario_scalar),&s->scenario};
 static const gh_native_task_entry entries[]={
  room_r102_reinforce_2720,room_r102_mount_2868,room_r102_counter,room_r102_start_action_0798
 };
 static const uint32_t callsites[]={0x384,0x3ac,0x3c0,0x3d4};
 for(;s->r102.followup_stage<4;++s->r102.followup_stage){
  const unsigned stage=s->r102.followup_stage;
  if(stage==1&&(u32(s->room_state.active+4)&0x00800000u))continue;
  int32_t slot;
  uint32_t pc=0;
  gh_scenario_exec_status status=gh_scenario_exec_002c3890(scenario,s->scenario_threads,
   -1,entries[stage],s,0,&slot,&pc);
  if(status){
   *frontier=pc?pc:callsites[stage];
   return status==GH_SCENARIO_EXEC_MISSING?GH_SCENE_UNSUPPORTED:GH_SCENE_CALLEE;
  }
 }
 *frontier=0;
 return GH_SCENE_OK;
}

static void room_r101_wait_area(void *context,int32_t argument)
{
 gh_game_services *s=context;
 (void)argument;
 do {
  if(gh_task_threads_sleep(s->scenario_threads,1)!=GH_THREAD_OK)return;
  gh_at_native_unit unit;
  if(gh_scene_at_find_id_002c3118(&s->scene_at,2,&unit)){
   s->r101_task_frontier=0x2c3118;return;
  }
  if(unit.bytes&&(u32(unit.bytes+0x50)&0x80000000u)){
   uint8_t *flags=s->room_state.active+4;
   w32(flags,u32(flags)|0x80000000u);
   if(u32(s->room_state.active+0x1610)){
    s->r101_task_frontier=0x17b8;return; /* Positive deferred SetEm. */
   }
   return;
  }
 } while(1);
}

static gh_r100_script_status room_r100_checkpoint(void *context,uint32_t *frontier);

static gh_scene_status room_r103_status(gh_game_services *s,uint32_t *frontier,
 uint32_t address,gh_scene_status status)
{
 *frontier=address;
 s->frontier=address;s->frontier_module_code=address&&address<0x100000?8:0;
 return status;
}

static int room_r103_sleep(gh_game_services *s,int32_t frames)
{
 if(gh_task_threads_sleep(s->scenario_threads,frames)==GH_THREAD_OK)return 0;
 s->r103_task_frontier=0x2d5aa0;
 return 1;
}

static int room_r103_set_lock(gh_game_services *s,uint8_t stamp,int enabled)
{
 uint32_t pc=0;
 if(!gh_enemy_ems_set_stamp_lock_off_00295c10(s->b.init,stamp,enabled,&pc))return 0;
 s->r103_task_frontier=pc?pc:0x295c10;
 return 1;
}

static int room_r103_set_muteki(gh_game_services *s,uint8_t stamp,uint32_t timer)
{
 gh_game_title_player *actor;
 if(gh_enemy_manager_get_00290868(s->b.init,stamp,&actor)){
  s->r103_task_frontier=0x295978;return 1;
 }
 if(actor){
  if(!actor->view.bytes||actor->view.size<0x550){
   s->r103_task_frontier=0x295b80;return 1;
  }
  w32(actor->view.bytes+0x54c,timer);
 }
 return 0;
}

/* REL0258 refreshes protection until each wave's bit opens. */
static const uint8_t r103_waves[][2]={{2,3},{6,7},{4,5},{0,1}};

static void room_r103_protection_0258(void *context,int32_t argument)
{
 gh_game_services *s=context;
 (void)argument;
 for(;;){
  const uint32_t flags=u32((const uint8_t *)s->b.work+0x5ec);
  for(unsigned wave=0;wave<4;++wave){
   if(flags&(0x80000000u>>wave))continue;
   const uint8_t *stamps=r103_waves[wave];
   if(room_r103_set_muteki(s,stamps[0],0x41a00000u)||room_r103_set_muteki(s,stamps[1],0x41a00000u)||
      room_r103_set_lock(s,stamps[0],1)||room_r103_set_lock(s,stamps[1],1))return;
  }
  if((flags&0x10000000u)||room_r103_sleep(s,1))return;
 }
}

static int room_r103_wave_centers(gh_game_services *s,unsigned first,float centers[2][3])
{
 for(unsigned i=0;i<2;++i){
  gh_at_native_unit unit;
  if(gh_scene_at_find_id_002c3118(&s->scene_at,first+i,&unit)||
     gh_scene_at_center_002c0e78(unit,centers[i])){
   s->r103_task_frontier=0x2c2a48;return 1;
  }
 }
 return 0;
}

static int room_r103_wave_begin(gh_game_services *s,unsigned wave,float centers[2][3])
{
 uint8_t *flags=(uint8_t *)s->b.work+0x5ec;
 w32(flags,u32(flags)|(0x80000000u>>wave));
 const uint8_t *stamps=r103_waves[wave];
 for(unsigned i=0;i<2;++i){
  uint32_t pc=0;
  if(gh_enemy_ems_set_stamp_goto_timed_00295f80(s->enemies,s->b.init,
     stamps[i],centers[i],2,0,0x43160000u,&pc)){
   s->r103_task_frontier=pc?pc:0x295f80;return 1;
  }
 }
 if(room_r103_set_lock(s,stamps[0],0)||room_r103_set_lock(s,stamps[1],0))return 1;
 uint32_t pc=0;
 if(room_r100_program_se(s,(int16_t)(0x32+wave),&pc)){
  s->r103_task_frontier=pc?pc:0x2cb8a0;return 1;
 }
 return 0;
}

static int room_r103_dead(gh_game_services *s,uint8_t stamp,int *dead)
{
 gh_game_title_player *actor;
 if(gh_enemy_manager_get_00290868(s->b.init,stamp,&actor)){
  s->r103_task_frontier=0x295978;return 1;
 }
 *dead=1;
 if(actor&&gh_r100_enemy_dead_0028fb38(&s->room_state,actor,dead)){
  s->r103_task_frontier=0x28fb38;return 1;
 }
 return 0;
}

/* REL0448 starts the two positional room-bank lines 30 and 120 ticks in. */
static void room_r103_dialogue_0448(void *context,int32_t argument)
{
 gh_game_services *s=context;
 (void)argument;
 for(unsigned stamp=0;stamp<2;++stamp){
  if(room_r103_sleep(s,stamp?0x5a:0x1e))return;
  gh_game_title_player *actor=NULL;
  if(gh_enemy_manager_get_00290868(s->b.init,(uint8_t)stamp,&actor)){
   s->r103_task_frontier=0x295978;return;
  }
  if(!actor)continue;
  uint32_t handle=0,pc=0;
  gh_enemy_ems_live live;
  if(gh_enemy_ems_handle_for_actor(s->enemies,actor,&handle)||!handle||
     gh_enemy_ems_live_at(s->enemies,handle,&live)||live.actor!=actor||!live.model){
   s->r103_task_frontier=0x295978;return;
  }
  const gh_motion_sound_request request={.model=live.model,.actor=&actor->view,
      .bank=2,.program=(int16_t)(0x39+stamp)};
  if(gh_actor_attached_sound_motion(&s->enemy_sound,&request,&handle,&pc)){
   s->r103_task_frontier=pc?pc:0x2cba48;return;
  }
 }
}

/* REL0E54 releases the two protected middle bosses after the six reinforcements. */
static int room_r103_boss_wave(gh_game_services *s)
{
 uint8_t *flags=(uint8_t *)s->b.work+0x5ec;
 s->b.sound_control->bgm_state_13c=1;
 w32(flags,u32(flags)|0x10000000u);

 uint32_t pc=0;
 gh_soft_event_start start={0};
 if(room_r100_start_soft(s,&start,0,&pc)){
  s->r103_task_frontier=pc?pc:0x2c39b0;return 1;
 }
 gh_room_camera_start camera={0};
 if(room_r100_set_camera(s,&camera,"A03",&pc)){
  s->r103_task_frontier=pc?pc:0x2c3ed8;return 1;
 }

 const uint8_t *stamps=r103_waves[3];
 const uint32_t drops[]={0x3d1,0x3d7};
 for(unsigned i=0;i<2;++i){
  gh_game_title_player *actor;
  if(gh_enemy_manager_get_00290868(s->b.init,stamps[i],&actor)){
   s->r103_task_frontier=0x295978;return 1;
  }
  if(!actor)continue;
  if(!actor->view.bytes||actor->view.size<0x564){
   s->r103_task_frontier=0x295e38;return 1;
  }
  w32(actor->view.bytes+0x560,drops[i]);
 }
 for(unsigned i=0;i<2;++i){
  if(gh_enemy_ems_set_stamp_suspend_00295bc0(s->enemies,s->b.init,stamps[i],0,&pc)){
   s->r103_task_frontier=pc?pc:0x295bc0;return 1;
  }
 }
 for(unsigned i=0;i<2;++i){
  if(room_r103_set_muteki(s,stamps[i],0x42700000u))return 1;
 }
 float centers[2][3];
 if(room_r103_wave_centers(s,2,centers))return 1;
 for(unsigned i=0;i<2;++i){
  if(gh_enemy_ems_set_stamp_goto_timed_00295f80(s->enemies,s->b.init,
     stamps[i],centers[i],3,0,0x43160000u,&pc)){
   s->r103_task_frontier=pc?pc:0x295f80;return 1;
  }
 }
 if(room_r103_set_lock(s,stamps[0],0)||room_r103_set_lock(s,stamps[1],0))return 1;
 if(room_r100_program_se(s,0x38,&pc)){
  s->r103_task_frontier=pc?pc:0x2cb8a0;return 1;
 }
 for(;;){
  int ended;
  if(room_r100_camera_ended(s,&ended,&pc)){
   s->r103_task_frontier=pc?pc:0x2c4040;return 1;
  }
  if(ended)break;
  if(room_r103_sleep(s,1))return 1;
 }
 gh_soft_event_end end={0};
 if(room_r100_end_live_soft(s,&end,&pc)){
  s->r103_task_frontier=pc?pc:0x2c3c10;return 1;
 }
 if(gh_game_cockpit_show_middle_boss(s->cockpit)){
  s->r103_task_frontier=0xf88;return 1;
 }
 return 0;
}

static int room_r103_outro_1000(gh_game_services *s)
{
 float positions[2][3];
 for(uint8_t stamp=0;stamp<2;++stamp){
  gh_game_title_player *actor=NULL;
  if(gh_enemy_manager_get_00290868(s->b.init,stamp,&actor)){
   s->r103_task_frontier=0x295978;return 1;
  }
  if(actor&&actor->view.alias_f0)memcpy(positions[stamp],actor->view.alias_f0,12);
  else if(!actor&&s->b.math_globals&&s->b.math_globals->loaded)
   memcpy(positions[stamp],s->b.math_globals->zero_vector,12);
  else{
   s->r103_task_frontier=0x295ab8;return 1;
  }
 }
 if(gh_scene_at_add_r103_messages_002c2f90(&s->scene_at,s->r103_work,positions,s->r103_message_areas)){
  s->r103_task_frontier=s->scene_at.frontier?s->scene_at.frontier:0x2c2f90;return 1;
 }
 s->r103_work=(gh_r207_allocation){0};

 float alpha=1.0f;
 uint32_t pc=0;
 do {
  fenv_t saved;
  if(feholdexcept(&saved)){
   s->r103_task_frontier=0x1124;return 1;
  }
  if(fesetround(FE_TONEAREST)){
   (void)fesetenv(&saved);
   s->r103_task_frontier=0x1124;return 1;
  }
  volatile float next=alpha-0.02f;
  alpha=next;
  if(fesetenv(&saved)){
   s->r103_task_frontier=0x1124;return 1;
  }
  if(gh_r100_scroll_layer_alpha_002db318(&s->pools,&s->meshes,1,alpha,&pc)){
   s->r103_task_frontier=pc?pc:0x2db318;return 1;
  }
  if(room_r103_sleep(s,1))return 1;
 } while(alpha>0.1f);
 if(gh_r100_scroll_layer_002db2b0(&s->pools,&s->meshes,1,0,&pc)){
  s->r103_task_frontier=pc?pc:0x2db2b0;return 1;
 }
 gh_at_native_unit jump;
 if(gh_scene_at_get_jump_002c2ae0(&s->scene_at,1,&jump)){
  s->r103_task_frontier=0x2c2ae0;return 1;
 }
 int enabled;
 if(!jump.bytes||gh_scene_at_set_enabled(&s->scene_at,jump,1,&enabled)||!enabled){
  s->r103_task_frontier=s->scene_at.frontier?s->scene_at.frontier:0x2c28f8;return 1;
 }
 return 0;
}

/* REL0670 starts the arena BGM and closes the return jump. */
static void room_r103_arena_0670(void *context,int32_t argument)
{
 gh_game_services *s=context;
 (void)argument;
 if(!s->b.sound_control||!s->b.sound_control->initialized){
  s->r103_task_frontier=0x2d0ab8;return;
 }
 s->b.sound_control->bgm_state_13c=2;
 gh_at_native_unit jump;
 if(gh_scene_at_get_jump_002c2ae0(&s->scene_at,1,&jump)){
  s->r103_task_frontier=0x2c2ae0;return;
 }
 int original;
 if(!jump.bytes||gh_scene_at_set_enabled(&s->scene_at,jump,0,&original)||!original){
  s->r103_task_frontier=s->scene_at.frontier?s->scene_at.frontier:0x2c29a0;return;
 }
 const gh_scene_scenario_view scene={s->scenario_scalar,sizeof(s->scenario_scalar),&s->scenario};
 int32_t slot;
 uint32_t pc=0;
 if(gh_scenario_exec_002c3890(scene,s->scenario_threads,-1,room_r103_protection_0258,s,-1,&slot,&pc)){
  s->r103_task_frontier=pc?pc:0x6ec;return;
 }
 if(room_r103_sleep(s,30))return;

 float centers[2][3];
 if(room_r103_wave_centers(s,0,centers))return;
 if(room_r103_wave_begin(s,0,centers))return;
 uint32_t elapsed=0;
 for(;elapsed<0x709;++elapsed){
  int dead;
  if(room_r103_dead(s,2,&dead))return;
  if(dead)break;
  if(room_r103_dead(s,3,&dead))return;
  if(dead)break;
  if(room_r103_sleep(s,1))return;
 }
 if(room_r103_wave_begin(s,1,centers))return;
 for(;;){
  unsigned dead_count=0;
  for(unsigned wave=0;wave<2;++wave){
   for(unsigned i=0;i<2;++i){
    int dead;
    if(room_r103_dead(s,r103_waves[wave][i],&dead))return;
    dead_count+=(unsigned)dead;
   }
  }
  uint8_t *flags=(uint8_t *)s->b.work+0x5ec;
  if(dead_count>=2&&!(u32(flags)&0x04000000u)){
   w32(flags,u32(flags)|0x04000000u);
   if(gh_scenario_exec_002c3890(scene,s->scenario_threads,-1,room_r103_dialogue_0448,s,0,&slot,&pc)){
    s->r103_task_frontier=pc?pc:0x974;return;
   }
  }
  if(dead_count==4)break;
  ++elapsed;
  if(room_r103_sleep(s,1))return;
 }
 if(room_r103_wave_begin(s,2,centers))return;
 for(;;){
  int all_dead=1;
  for(unsigned wave=0;wave<3&&all_dead;++wave){
   for(unsigned i=0;i<2&&all_dead;++i)
    if(room_r103_dead(s,r103_waves[wave][i],&all_dead))return;
  }
  if(all_dead)break;
  ++elapsed;
  if(room_r103_sleep(s,1))return;
 }
 unsigned level;
 if(gh_core_save_game_level_001fa010(s->b.save,s->b.boot_elf,s->b.boot_elf_bytes,&level)){
  s->r103_task_frontier=0x1fa010;return;
 }
 if((elapsed<0xb23&&level>2)||level>4){
  s->r103_task_frontier=0xb20;return; /* Optional two-enemy soft event. */
 }
 if(room_r103_boss_wave(s))return;
 for(;;){
  int dead;
  if(room_r103_dead(s,0,&dead))return;
  if(dead){
   if(room_r103_dead(s,1,&dead))return;
   if(dead)break;
  }
  if(room_r103_sleep(s,1))return;
 }
 uint8_t *saved=s->room_state.active+4;
 w32(saved,u32(saved)|0x40000000u);
 if(room_r103_sleep(s,30))return;
 s->b.sound_control->bgm_state_13c=0;
 (void)room_r103_outro_1000(s);
}

static void room_r103_entry_0538(void *context,int32_t argument)
{
 gh_game_services *s=context;
 (void)argument;
 const uint32_t position[]={0x41a00000,0x3cf5c28f,0xc2b8bd71};
 const uint32_t rotation[]={0,0xbfcccccd,0};
 uint8_t *flags=s->room_state.active+4;
 w32(flags,u32(flags)|0x80000000u);
 const gh_title_player_view *child=s->player_tail.active_child&&s->player_tail.child_original_return?
  &s->player_child.object.view:NULL;
 if(gh_title_player_set_script_pose_0012ce60(&s->title_player.view,child,position,rotation)){
  s->r103_task_frontier=0x12ce60;return;
 }
 if(!(u32((const uint8_t *)s->b.work+0x590)&0x200000u)&&
    room_play_event_wait(s,0x1a,&s->r103_task_frontier))return;
 uint32_t pc=0;
 if(room_r100_checkpoint(s,&pc)){
  s->r103_task_frontier=pc?pc:0x2c05b0;return;
 }
 const gh_scene_scenario_view scene={s->scenario_scalar,sizeof(s->scenario_scalar),&s->scenario};
 int32_t slot;
 if(gh_scenario_exec_002c3890(scene,s->scenario_threads,-1,room_r103_arena_0670,s,0,&slot,&pc))
  s->r103_task_frontier=pc?pc:0x644;
}

static gh_scene_status room_r103_init(void *context,uint32_t *frontier)
{
 gh_game_services *s=context;
 if(!frontier||!s->module.bound||s->module.room!=0x103||s->b.work->room!=0x103||
    !s->room_state.active||s->room_state.size<8)return GH_SCENE_ARGUMENT;
 if(s->r103_initialized)return room_r103_status(s,frontier,0,GH_SCENE_OK);
 gh_heap *heap=s->b.assets->heap;
 if(!s->r103_work.pointer&&gh_heap_allocate(heap,0x138,16,&s->r103_work)!=GH_MEM_OK)
  return room_r103_status(s,frontier,0x30ee08,GH_SCENE_CALLEE);
 gh_id_error hud=gh_game_cockpit_load_middle_boss_0013f330(s->cockpit,s->b.id,s->b.assets->dvd,heap);
 if(!hud)hud=gh_game_cockpit_hide_middle_boss(s->cockpit);
 if(hud){
  const uint32_t pc=gh_game_cockpit_frontier(s->cockpit);
  return room_r103_status(s,frontier,pc?pc:0x13f330,
    hud==GH_ID_UNSUPPORTED?GH_SCENE_UNSUPPORTED:GH_SCENE_CALLEE);
 }
 const uint32_t flags=u32(s->room_state.active+4);
 gh_native_task_entry task=NULL;
 if(!(flags&0x80000000u))task=room_r103_entry_0538;
 else if(!(flags&0x40000000u))task=room_r103_arena_0670;
 if(task){
  const gh_scene_scenario_view scene={s->scenario_scalar,sizeof(s->scenario_scalar),&s->scenario};
  int32_t slot;
  uint32_t pc=0;
  if(gh_scenario_exec_002c3890(scene,s->scenario_threads,-1,task,s,0,&slot,&pc))
   return room_r103_status(s,frontier,pc?pc:task==room_r103_entry_0538?0x204:0x234,GH_SCENE_CALLEE);
 }
 s->r103_initialized=1;
 return room_r103_status(s,frontier,0,GH_SCENE_OK);
}

/* R103Move is the pinned module's 0250 JR RA / NOP. */
static gh_scene_status room_r103_move(void *context,uint32_t *frontier)
{
 gh_game_services *s=context;
 if(!frontier||!s->module.bound||s->module.room!=0x103||!s->r103_initialized)
  return GH_SCENE_ARGUMENT;
 return room_r103_status(s,frontier,0,GH_SCENE_OK);
}

/* R104 REL04B8 sets the persistent win flag before EV06, then REL0518
 * requests the original results room independently of the local JMP table. */
static void room_r104_victory_04b8(void *context,int32_t argument)
{
 gh_game_services *s=context;
 (void)argument;
 uint8_t *saved=s->room_state.active+4;
 const uint32_t flags=u32(saved);
 if(!(flags&0x40000000u)){
  w32(saved,flags|0x40000000u);
  if(room_play_event_wait(s,6,&s->r104_task_frontier))return;
 }
 const float position[3]={0.0f,0.0f,0.0f};
 uint32_t pc=0;
 if(room_event_begin_jump(s,0x21,position,0.0f,0,0,1,&pc))
  s->r104_task_frontier=pc?pc:0x2c4548;
}

static void room_r104_loop_0540(void *context,int32_t argument)
{
 gh_game_services *s=context;
 (void)argument;
 gh_om_object *reward;
 if(gh_om_factory_create_0030fa98(&s->om_factory,&s->r104_reward,0x3da,0xffff,&reward)||
    !reward){
  s->r104_task_frontier=s->om_factory.frontier?s->om_factory.frontier:0x30fa98;
  return;
 }
 if(gh_om_factory_initialize_runtime_ticket_001c0128(&s->om_factory,reward)){
  s->r104_task_frontier=s->om_factory.frontier?s->om_factory.frontier:0x1c0128;
  return;
 }
 const uint32_t reward_position[]={0xc17b3333,0x3e4ccccd,0xbe19999a};
 const uint32_t reward_rotation[]={0,0xbfc90fdb,0};
 if(gh_om_r101_item_position_001c2630(reward->initialization,(const uint8_t *)reward_position)){
  s->r104_task_frontier=reward->initialization->frontier;
  return;
 }
 memcpy(reward->view.bytes+0x100,reward_rotation,sizeof(reward_rotation));
 w32(reward->view.bytes+0x5b8,u32(reward->view.bytes+0x5b8)|0x10u);
 w32(reward->view.bytes+0x560,0xa5d);
 reward->view.bytes[0x676]|=2;

 const uint32_t player_position[]={0xbfb851ec,0,0x3e99999a};
 const uint32_t player_rotation[]={0,0xbfbc28f6,0};
 const gh_title_player_view *child=s->player_tail.active_child&&s->player_tail.child_original_return?
  &s->player_child.object.view:NULL;
 if(gh_title_player_set_script_pose_0012ce60(&s->title_player.view,child,player_position,player_rotation)){
  s->r104_task_frontier=0x12ce60;
  return;
 }
 if(gh_task_threads_sleep(s->scenario_threads,1)!=GH_THREAD_OK){
  s->r104_task_frontier=0x2d5aa0;
  return;
 }
 uint32_t pc=0;
 if(gh_r100_scroll_layer_002db2b0(&s->pools,&s->meshes,3,1,&pc)){
  s->r104_task_frontier=pc?pc:0x2db2b0;
  return;
 }
 float alpha=0.1f;
 do{
  const volatile float step=alpha+0.04f;
  alpha=step;
  if(gh_r100_scroll_layer_alpha_002db318(&s->pools,&s->meshes,3,alpha,&pc)){
   s->r104_task_frontier=pc?pc:0x2db318;
   return;
  }
  if(gh_task_threads_sleep(s->scenario_threads,1)!=GH_THREAD_OK){
   s->r104_task_frontier=0x2d5aa0;
   return;
  }
 }while(alpha<1.0f);
 if(gh_r100_scroll_layer_alpha_002db318(&s->pools,&s->meshes,3,1.0f,&pc)){
  s->r104_task_frontier=pc?pc:0x2db318;
  return;
 }
 for(;;){
  gh_game_title_player *boss;
  if(gh_enemy_manager_get_00290868(s->b.init,0,&boss)){
   s->r104_task_frontier=0x295978;
   return;
  }
  int dead=1;
  if(boss&&gh_r100_enemy_dead_0028fb38(&s->room_state,boss,&dead)){
   s->r104_task_frontier=0x28fb38;
   return;
  }
  if(dead)break;
  if(gh_task_threads_sleep(s->scenario_threads,1)!=GH_THREAD_OK){
   s->r104_task_frontier=0x2d5aa0;
   return;
  }
 }
 const gh_scene_scenario_view scene={s->scenario_scalar,sizeof(s->scenario_scalar),&s->scenario};
 int32_t slot;
 if(gh_scenario_exec_002c3890(scene,s->scenario_threads,-1,room_r104_victory_04b8,s,0,&slot,&pc))
  s->r104_task_frontier=pc?pc:0x760;
}

static void room_r104_intro_0380(void *context,int32_t argument)
{
 gh_game_services *s=context;
 (void)argument;
 if(room_play_event_wait(s,5,&s->r104_task_frontier))return;
 if(u32((const uint8_t *)s->b.work+0x594)&0x800u){
  s->r104_task_frontier=0x3c4; /* setEnding__3cE3b(0) */
  return;
 }
 const uint32_t position[]={0xbfb851ec,0,0x3e99999a};
 const uint32_t rotation[]={0,0xbfbc28f6,0};
 const gh_title_player_view *child=s->player_tail.active_child&&s->player_tail.child_original_return?
  &s->player_child.object.view:NULL;
 if(gh_title_player_set_script_pose_0012ce60(&s->title_player.view,child,position,rotation)){
  s->r104_task_frontier=0x12ce60;
  return;
 }
 uint32_t pc=0;
 if(room_r100_checkpoint(s,&pc)){
  s->r104_task_frontier=pc?pc:0x2c05b0;
  return;
 }
 const gh_scene_scenario_view scene={s->scenario_scalar,sizeof(s->scenario_scalar),&s->scenario};
 int32_t slot;
 if(gh_scenario_exec_002c3890(scene,s->scenario_threads,-1,room_r104_loop_0540,s,0,&slot,&pc))
  s->r104_task_frontier=pc?pc:0x488;
}

static gh_scene_status room_r104_init(void *context,uint32_t *frontier)
{
 gh_game_services *s=context;
 if(!s||!frontier||!s->module.bound||s->module.room!=0x104||s->b.work->room!=0x104||
    !s->room_state.active||s->room_state.size<8)return GH_SCENE_ARGUMENT;
 *frontier=0;
 if(s->r104_initialized)return GH_SCENE_OK;
 gh_heap *heap=s->b.assets->heap;
 if(!s->r104_work.pointer&&gh_heap_allocate(heap,4,16,&s->r104_work)!=GH_MEM_OK){
  *frontier=0x30ee08;
  return GH_SCENE_CALLEE;
 }
 const unsigned jump_index=(u32((const uint8_t *)s->b.work+0x594)&0x800u)!=0;
 gh_at_native_unit jump;
 int original;
 for(unsigned i=0;i<2;++i){
  if(gh_scene_at_get_jump_002c2ae0(&s->scene_at,i,&jump)){
   *frontier=0x2c2ae0;
   return GH_SCENE_CALLEE;
  }
  if(gh_scene_at_set_enabled(&s->scene_at,jump,i==jump_index,&original)){
   *frontier=s->scene_at.frontier;
   return GH_SCENE_CALLEE;
  }
 }
 w32(s->r104_work.pointer,jump_index);
 if(!jump_index){
  if(gh_r100_scroll_layer_002db2b0(&s->pools,&s->meshes,2,0,frontier))
   return GH_SCENE_CALLEE;
  const gh_id_error hud=gh_game_cockpit_load_boss_0013c818(s->cockpit,s->b.id,s->b.assets->dvd,heap);
  if(hud){
   *frontier=gh_game_cockpit_frontier(s->cockpit);
   if(!*frontier)*frontier=0x13c818;
   return hud==GH_ID_UNSUPPORTED?GH_SCENE_UNSUPPORTED:GH_SCENE_CALLEE;
  }
 }
 if(gh_scene_at_get_jump_002c2ae0(&s->scene_at,jump_index,&jump)){
  *frontier=0x2c2ae0;
  return GH_SCENE_CALLEE;
 }
 if(gh_scene_at_set_enabled(&s->scene_at,jump,0,&original)){
  *frontier=s->scene_at.frontier;
  return GH_SCENE_CALLEE;
 }
 if(gh_r100_scroll_layer_002db2b0(&s->pools,&s->meshes,3,0,frontier)||
    gh_r100_scroll_layer_002db2b0(&s->pools,&s->meshes,11,0,frontier))
  return GH_SCENE_CALLEE;
 s->b.sound_control->bgm_state_13c=1;
 uint8_t *room_flags=s->room_state.active+4;
 const uint32_t flags=u32(room_flags);
 const int reentry=(flags&0x80000000u)!=0;
 if(!reentry)w32(room_flags,flags|0x80000000u);
 const gh_scene_scenario_view scene={s->scenario_scalar,sizeof(s->scenario_scalar),&s->scenario};
 int32_t slot;
 if(gh_scenario_exec_002c3890(scene,s->scenario_threads,-1,
    reentry?room_r104_loop_0540:room_r104_intro_0380,s,0,&slot,frontier)){
  w32(room_flags,flags);
  if(!*frontier)*frontier=0x2c3890;
  return GH_SCENE_CALLEE;
 }
 s->r104_initialized=1;
 return GH_SCENE_OK;
}

/* R104Move at REL0378 is JR RA / NOP. */
static gh_scene_status room_r200_init(void *context,uint32_t *frontier)
{
 gh_game_services *s=context;
 if(!s||!frontier||s->module.room!=0x200||s->module.init_offset!=0x1a0||
    !s->module.bound||s->b.work->room!=0x200)return GH_SCENE_ARGUMENT;
 *frontier=0;
 if(!s->r200_work.pointer){
  if(gh_heap_allocate(s->b.assets->heap,0x24,16,&s->r200_work)!=GH_MEM_OK){
   *frontier=0x30f550;
   return GH_SCENE_CALLEE;
  }
  w32(s->r200_work.pointer,0);
  w32((uint8_t *)s->r200_work.pointer+0x10,0);
 }
 if(!s->r200_init_stage){
  gh_object_view view={0};
  const gh_object_status status=gh_game_objects_load_wait_001fe8a0(&s->r200_load,
   s->objects,0x3bd,&s->object_resources,1,&view,frontier);
  if(status)return status==GH_OBJECT_UNSUPPORTED?GH_SCENE_UNSUPPORTED:GH_SCENE_CALLEE;
  s->r200_init_stage=1;
 }
 /* Original 022C through 0308 replace each existing area callback. The
  * paired ID rows share a callback but pass its second endpoint as arg1. */
 static const struct {uint16_t id,entry;uint8_t argument;} rows[]={
  {0,0x510,0},{2,0x510,1},{1,0x830,0},{3,0x830,1},
  {4,0xbe0,0},{6,0xc90,0},{7,0xf00,0},{8,0xe88,0}
 };
 while(s->r200_init_stage<=sizeof(rows)/sizeof(rows[0])){
  const size_t row=s->r200_init_stage-1;
  gh_at_native_unit unit;
  if(gh_scene_at_find_id_002c3118(&s->scene_at,rows[row].id,&unit)||
     gh_scene_at_set_jump_info_002c2750(&s->scene_at,unit,0xff,
      rows[row].entry,rows[row].argument,0)){
   *frontier=0x2c2790;
   return GH_SCENE_CALLEE;
  }
  ++s->r200_init_stage;
 }
 gh_at_native_unit unit;
 int original;
 if(gh_scene_at_find_id_002c3118(&s->scene_at,4,&unit)||
    gh_scene_at_set_enabled(&s->scene_at,unit,0,&original)){
  *frontier=0x2c29f8;
  return GH_SCENE_CALLEE;
 }
 return GH_SCENE_OK;
}

static gh_scene_status room_r200_move(void *context,uint32_t *frontier)
{
 gh_game_services *s=context;
 if(!s||!frontier||s->module.room!=0x200||s->module.move_offset!=0x338||
    !s->r200_work.pointer)return GH_SCENE_ARGUMENT;
 uint8_t *rows=s->r200_work.pointer;
 for(size_t i=0;i<2;++i){
  uint8_t *row=rows+i*0x10;
  const uint32_t handle=u32(row);
  if(!handle)continue;
  gh_title_player_view view;
  if(gh_om_factory_object_view(&s->om_factory,handle,&view)||
     view.size!=0x640||view.original_vtable214!=0x428c20){
   *frontier=0x1878;
   return GH_SCENE_UNSUPPORTED;
  }
  uint8_t *actor=view.bytes;
  const uint32_t flags=u32(actor+0x5b8);
  const int active=(flags&1u)!=0;
  const uint32_t bit=u32(row+(active?12:8));
  if(bit>=64)continue;
  uint8_t *sys=(uint8_t *)s->b.work+0x5ec+(bit/32)*4;
  const uint32_t mask=0x80000000u>>(bit%32);
  if(!(u32(sys)&mask)||(active&&actor[0x2f5]>=2))continue;
  w32(sys,u32(sys)&~mask);
  w32(actor+0x5b8,active?flags&~1u:flags|1u);
  if(active){
   actor[0x2f4]=3;
   actor[0x2f5]=actor[0x2f6]=actor[0x2f7]=0;
   w32(row,0);
  }
 }
 *frontier=0;
 return GH_SCENE_OK;
}

static gh_scene_status room_r104_move(void *context,uint32_t *frontier)
{
 gh_game_services *s=context;
 if(!s||!frontier||!s->r104_initialized)return GH_SCENE_ARGUMENT;
 *frontier=0;
 return GH_SCENE_OK;
}

static gh_scene_status room_r101_init(void *context,uint32_t *frontier)
{
 gh_game_services *s=context;
 if(!frontier||s->b.work->room!=0x101||!s->scene_at.ready||!s->room_state.active||s->room_state.size<0x1614)
  return GH_SCENE_ARGUMENT;
 *frontier=0;
 if(!s->r101_work.pointer&&gh_heap_allocate(s->b.assets->heap,4,16,&s->r101_work)!=GH_MEM_OK)
  return r101_stop(s,frontier,0x30ee08);
 while(s->r101_init_stage<2){
  const uint32_t index=2+s->r101_init_stage;
  gh_at_native_unit jump;
  if(gh_scene_at_get_jump_002c2ae0(&s->scene_at,index,&jump))return r101_stop(s,frontier,0x2c2ae0);
  if(gh_scene_at_set_door_func_002c2728(&s->scene_at,jump,0x1490,index))
   return r101_stop(s,frontier,0x2c2728);
  ++s->r101_init_stage;
 }
 const uint32_t flags=u32(s->room_state.active+4);
 if(s->r101_init_stage==2){
  if(flags&0x40000000u)return r101_stop(s,frontier,0x1580);
  for(unsigned layer=0;layer<4;++layer){
   gh_r100_init_status status=gh_r100_scroll_layer_002db2b0(&s->pools,&s->meshes,layer,(layer&1)==0,frontier);
   if(status)return r101_stop(s,frontier,*frontier?*frontier:0x2db2b0);
  }
  if(room_r101_om_layer(s,2,1)||room_r101_om_layer(s,1,0))
   return r101_stop(s,frontier,0x2db478);
  if(gh_r100_scroll_layer_002db2b0(&s->pools,&s->meshes,4,0,frontier))
   return r101_stop(s,frontier,*frontier?*frontier:0x2db2b0);
  s->r101_init_stage=3;
 }
 const gh_scene_scenario_view scene={s->scenario_scalar,sizeof(s->scenario_scalar),&s->scenario};
 if(s->r101_init_stage==3){
  int32_t slot;
  if(!(flags&0x80000000u)&&gh_scenario_exec_002c3890(scene,s->scenario_threads,-1,room_r101_wait_area,s,0,&slot,frontier))
   return r101_stop(s,frontier,*frontier?*frontier:0x1698);
  s->r101_init_stage=4;
 }
 if(s->r101_init_stage==4){
  int32_t slot;
  if(gh_scenario_exec_002c3890(scene,s->scenario_threads,-1,room_r101_counter,s,0,&slot,frontier))
   return r101_stop(s,frontier,*frontier?*frontier:0x18b0);
  s->r101_init_stage=5;
 }
 if(s->r101_init_stage==5){
  static const uint8_t stamps[]={9,7};
  for(unsigned i=0;i<2;++i){
   gh_game_title_player *enemy=NULL;
   if(gh_enemy_manager_get_00290868(s->b.init,stamps[i],&enemy))return r101_stop(s,frontier,0x295e38);
   if(enemy)w32(enemy->view.bytes+0x560,0x3e3);
  }
  if(!(flags&0x01000000u)){
   gh_at_native_unit unit;
   if(gh_scene_at_find_id_002c3118(&s->scene_at,0,&unit)||
      gh_scene_at_set_jump_info_002c2750(&s->scene_at,unit,0xff,0x528,0,0))
    return r101_stop(s,frontier,0x2c2790);
  }
  if(!(flags&0x10000000u)){
   gh_at_native_unit jump;
   if(gh_scene_at_get_jump_002c2ae0(&s->scene_at,1,&jump)||
      gh_scene_at_set_jump_info_002c2750(&s->scene_at,jump,0xff,0x6e8,0,0))
    return r101_stop(s,frontier,0x2c2750);
  }
  static const uint32_t layer_flags[]={0x08000000u,0x04000000u,0x02000000u};
  for(unsigned i=0;i<3;++i)
   if((flags&layer_flags[i])&&gh_r100_scroll_layer_002db2b0(&s->pools,&s->meshes,0x1f+i,0,frontier))
    return r101_stop(s,frontier,*frontier?*frontier:0x2db2b0);
  if((flags&0x0e000000u)==0x0e000000u&&gh_r100_scroll_layer_002db2b0(&s->pools,&s->meshes,0x1e,0,frontier))
   return r101_stop(s,frontier,*frontier?*frontier:0x2db2b0);
  static const float escape[4]={-.53f,0,-49.22f,1};
  for(unsigned stamp=0x10;stamp<=0x11;++stamp){
   gh_game_title_player *enemy=NULL;
   if(gh_enemy_manager_get_00290868(s->b.init,(uint8_t)stamp,&enemy)||
      (enemy&&gh_enemy2a2_set_escape_position_00289038(enemy,escape)))
    return r101_stop(s,frontier,0x289038);
  }
  static const uint64_t pickup_keys[]={0x656e,0x7262,0x656d};
  static const uint64_t block_keys[]={0x3334306d6f,0x3434306d6f,0x3234306d6f};
  for(unsigned i=0;i<3;++i){
   int used;
   if(gh_om_room_slot_used_002bed08(&s->room_state,pickup_keys[i],&used))
    return r101_stop(s,frontier,0x2c5168);
   if(used)continue;
   for(size_t k=0;k<s->om_factory.count;++k){
    gh_om_object *object=s->om_factory.objects+k;
    if(object->source.type!=0x3d3+i)continue;
    uint64_t key;
    memcpy(&key,object->view.bytes+0x538,8);
    if(key==pickup_keys[i]){memcpy(object->view.bytes+0x668,&block_keys[i],8);break;}
   }
  }
  s->r101_init_stage=6;
 }
 *frontier=0;return GH_SCENE_OK;
}

static gh_scene_status room_r101_move(void *context,uint32_t *frontier)
{
 const gh_game_services *s=context;
 if(!s||!frontier||!s->module.bound||s->module.room!=0x101||s->module.move_offset!=0x520)
  return GH_SCENE_ARGUMENT;
 *frontier=0; /* Version-pinned REL0520 is JR RA / NOP. */
 return GH_SCENE_OK;
}

static gh_scene_status room_r105_move(void *context,uint32_t *frontier)
{
 gh_game_services *s=context;
 const gh_scene_status result=gh_r105_move_00000398(&s->r105,&s->om_factory,
  &s->b.init->random,s->b.frame_random,room_r100_om_view,s,frontier);
#ifdef GH_NATIVE_STARTUP_QA
 if(s->qa_r105_exit_fixture&&!s->qa_r105_exit_pose_applied&&!result&&s->r105.watcher_slot[1]<0){
  gh_at_native_unit jump;
  *frontier=0x2c28f8;
  if(gh_scene_at_get_jump_002c2ae0(&s->scene_at,1,&jump)||!jump.bytes||
     !(jump.bytes[0x34]&1))return GH_SCENE_CALLEE;
  static const uint32_t pos[]={0x414c28f6u,0xbfa147aeu,0xc1e6e148u},angles[]={0,0x3fd1ff7eu,0};
  const gh_title_player_view *aux=s->player_tail.active_child&&s->player_tail.child_original_return?
   &s->player_child.object.view:NULL;
  if(gh_title_player_set_script_pose_0012ce60(&s->title_player.view,aux,pos,angles)||
     gh_actor_model_update_hierarchy_00149eb8(&s->player_model,GH_STATIC_BINARY32_REFERENCE))
   return GH_SCENE_CALLEE;
  fprintf(stderr,"CPU_R105_JUMP_FIXTURE at_authored_JMP1_center=1 facing=94 saved_flag=1\n");
  s->qa_r105_exit_pose_applied=1;
  *frontier=0;
 }
#endif
 return room_r105_status(s,result,frontier);
}

static gh_r100_script_status room_r100_camera_ended(void *context,int *ended,uint32_t *frontier)
{
 gh_game_services *s=context;
 *frontier=0x2c4040;
 const gh_scene_scenario_view scenario={s->scenario_scalar,sizeof(s->scenario_scalar),&s->scenario};
 gh_camera_status status=gh_game_room_camera_ended_002c4040(scenario,s->b.motion_camera,ended);
 if(!status)*frontier=0;
 return status?GH_R100_SCRIPT_CALLEE:GH_R100_SCRIPT_OK;
}

static gh_r100_script_status room_r100_enemy(void*p,uint8_t entry,const void**identity,uint32_t*f){
 gh_game_services*s=p;gh_game_title_player *actor=NULL;
 gh_actor_motion_status e=gh_enemy_manager_get_00290868(s->b.init,entry,&actor);
 if(e){*f=0x290868;return GH_R100_SCRIPT_CALLEE;}
 *identity=actor;return GH_R100_SCRIPT_OK;
}
static gh_r100_script_status room_r100_enemy_escape(void*p,const void*identity,const float position[4],uint32_t*f){
 gh_game_services*s=p;gh_game_title_player *actor=NULL;
 for(unsigned i=0;i<64;i++)if(s->b.init->enemy_actors[i]==identity){actor=s->b.init->enemy_actors[i];break;}
 if(!actor||gh_enemy2a2_set_escape_position_00289038(actor,position)){
  *f=0x289038;return GH_R100_SCRIPT_UNSUPPORTED;
 }
 return GH_R100_SCRIPT_OK;
}
static gh_r100_init_status room_r100_collision(void *context,const char *name,int enabled,uint32_t *frontier)
{
 gh_game_services *s=context;
 uint32_t changed;
 gh_scene_table_status status=gh_collision_scroll_set_enabled_001332d0(
     &s->scene_tables,&s->r100.collision,name,enabled,&changed);
 *frontier=status?0x1331b8:0;
 return status?GH_R100_INIT_CALLEE:GH_R100_INIT_OK;
}
static gh_r100_script_status room_r100_checkpoint(void *context,uint32_t *frontier)
{
 gh_game_services *s=context;
 const gh_checkpoint_bindings b={s->b.work,s->b.save,&s->prepare,s->b.init,
  &s->title_player.view,s->b.stage->work_area,sizeof(s->b.stage->work_area)};
 gh_checkpoint_status status=gh_game_checkpoint_002c05b0(&s->checkpoint,&b);
 *frontier=s->checkpoint.frontier;
 return !status?GH_R100_SCRIPT_OK:
  status==GH_CHECKPOINT_UNSUPPORTED?GH_R100_SCRIPT_UNSUPPORTED:GH_R100_SCRIPT_CALLEE;
}
static gh_r100_script_status room_r100_door(void *context,const void *identity,
 unsigned operation,int arg0,int arg1,uint32_t *frontier)
{
 gh_game_services *s=context;
 if(!frontier)return GH_R100_SCRIPT_ARGUMENT;
 *frontier=operation==0?0x1bd178:operation==1?0x1bd3f0:operation==3?0x1bd228:0x1bcfb0;
 if(!identity)return GH_R100_SCRIPT_ARGUMENT;
 for(size_t i=0;i<s->om_factory.count;i++){
  gh_om_object *o=&s->om_factory.objects[i];
  if(o->view.bytes!=identity)continue;
  if((o->source.type!=0x300&&o->source.type!=0x354)||!o->registered||
     !o->initialization||o->initialization->geometry_state.object!=o)
   return GH_R100_SCRIPT_UNSUPPORTED;
  gh_title_player_view view;
  if(gh_om_factory_object_view(&s->om_factory,o->handle,&view)||view.bytes!=identity)
   return GH_R100_SCRIPT_CALLEE;
  gh_scene_placement_status e;
  if(operation==0&&!arg1)e=gh_om_door_set_lock_001bd178(&o->initialization->geometry_state,arg0);
  else if(operation==1&&!arg0&&!arg1)e=gh_om_door_release_collision_001bd3f0(&o->initialization->geometry_state);
  else if(operation==2)e=gh_om_door_set_open_001bcfb0(&o->initialization->geometry_state,arg0,arg1);
  else if(operation==3&&!arg0&&!arg1)e=gh_om_door_rebuild_collision_001bd228(
   &o->initialization->geometry_state,s->b.door_shared,GH_STATIC_BINARY32_REFERENCE);
  else return GH_R100_SCRIPT_UNSUPPORTED;
  if(!e){*frontier=0;return GH_R100_SCRIPT_OK;}
  return e==GH_PLACE_UNSUPPORTED?GH_R100_SCRIPT_UNSUPPORTED:GH_R100_SCRIPT_CALLEE;
 }
 return GH_R100_SCRIPT_UNSUPPORTED;
}
static gh_r100_script_status room_r100_successor_player(void *context,const uint32_t xyz[3],
 const uint32_t euler[3],uint32_t *frontier)
{
 gh_game_services *s=context;
 if(!frontier)return GH_R100_SCRIPT_ARGUMENT;
 *frontier=0x12ce60;
 if(!s||!s->title_player.ready)return GH_R100_SCRIPT_UNSUPPORTED;
 const gh_title_player_view *aux=s->player_tail.active_child&&s->player_tail.child_original_return?
  &s->player_child.object.view:NULL;
 gh_title_player_status result=gh_title_player_set_script_pose_0012ce60(&s->title_player.view,aux,xyz,euler);
 if(result)return result==GH_PLAYER_UNSUPPORTED?GH_R100_SCRIPT_UNSUPPORTED:GH_R100_SCRIPT_CALLEE;
 *frontier=0;
 return GH_R100_SCRIPT_OK;
}
static gh_door_status room_r101_side_release_002908c0(gh_game_services *s,uint32_t *frontier)
{
 gh_game_init_state *manager=s->b.init;
 while(manager->enemy_top){
  unsigned slot=0;
  while(slot<64&&manager->enemy_top!=manager->enemies+slot*20)++slot;
  if(slot==64||!manager->enemy_actors[slot]){
   *frontier=0x2908c0;
   return GH_DOOR_UNSUPPORTED;
  }
  uint32_t handle;
  if(gh_enemy_ems_handle_for_actor(s->enemies,manager->enemy_actors[slot],&handle)||!handle){
   *frontier=0x290770;
   return GH_DOOR_UNSUPPORTED;
  }
  int removed;
  if(gh_enemy_ems_remove_00290770(s->enemies,&s->enemy_removal,handle,s->b.voices,&removed,frontier))
   return GH_DOOR_UNSUPPORTED;
  if(!removed){*frontier=0x290770;return GH_DOOR_UNSUPPORTED;}
  s->enemy_removal=(gh_enemy_manager_remove){0};
 }
 const gh_room_vector *objects;
 if(gh_room_registry_view(&s->pools,0x754c58,&objects)){
  *frontier=0x2908c0;
  return GH_DOOR_UNSUPPORTED;
 }
 for(uint32_t i=0;i<objects->count;++i){
  const uint32_t handle=u32((const uint8_t *)objects->allocation.pointer+4*i);
  gh_r100_om_view actor;
  if(room_r100_om_view(s,handle,&actor,frontier)||actor.size<0x300)
   return GH_DOOR_UNSUPPORTED;
  const unsigned kind=actor.bytes[0x2fe]|(unsigned)actor.bytes[0x2ff]<<8;
  if(kind==0x364&&gh_om_retire_mark_00312708(&s->om_factory,handle,s->b.voices,frontier))
   return GH_DOOR_UNSUPPORTED;
 }
 return GH_DOOR_OK;
}

static int room_r200_spawn_catapult(gh_game_services *s,uint32_t *frontier)
{
 gh_om_object *catapult=NULL;
 if(gh_om_factory_create_0030fa98(&s->om_factory,&s->r200_catapult,0x3bd,0xffff,&catapult)||
    (catapult&&gh_om_factory_initialize_runtime_3bd(&s->om_factory,catapult))){
  *frontier=s->om_factory.frontier?s->om_factory.frontier:0x3165b0;
  return 1;
 }
 uint8_t *row=s->r200_work.pointer;
 /* REL1660 builds the relay row at +1A00. REL1100 copies its scalar fields. */
 w32(row,catapult?catapult->handle:0);
 w32(row+4,10);
 w32(row+8,1);
 w32(row+12,3);
 if(catapult){
  const uint32_t position[]={0x40966666,0xc174cccd,0x42dacccd};
  memcpy(catapult->view.alias_f0,position,sizeof(position));
  memset(catapult->view.bytes+0x100,0,12);
 }
 return 0;
}

static void room_r200_catapult_00001020(void *context,int32_t argument)
{
 gh_game_services *s=context;
 uint32_t *frontier=&s->r200_task_frontier;
 if(argument||!s->title_player.ready){*frontier=0x1020;return;}
 const uint8_t *player=s->title_player.view.bytes;
 for(;;){
  /* 2C3E40 gates soft events through system flags and PL00 12BAF0. */
  if(!(u32((const uint8_t *)s->b.work+0x5e4)&0x40000000u)&&
     ((u32(player+0x15f4)&0x400u)||(!player[0x2f4]&&player[0x2f5]<4)))
   break;
  if(gh_task_threads_sleep(s->scenario_threads,1)!=GH_THREAD_OK){
   *frontier=0x2d5aa0;
   return;
  }
 }
 gh_soft_event_start start={0};
 if(room_r100_start_soft(s,&start,0,frontier)){
  if(!*frontier)*frontier=0x2c39b0;
  return;
 }
 if(room_r200_spawn_catapult(s,frontier))return;
 gh_room_camera_start camera={0};
 if(room_r100_set_camera(s,&camera,"A00",frontier))return;
 if(room_r100_program_se(s,0x32,frontier))return;

 static const uint8_t stamps[]={0xc,0xd};
 for(size_t i=0;i<sizeof(stamps);++i)
  if(gh_enemy_ems_set_stamp_alpha_00296230(s->enemies,s->b.init,stamps[i],0,frontier))return;
 for(size_t i=0;i<sizeof(stamps);++i)
  if(gh_enemy_ems_set_stamp_suspend_00295bc0(s->enemies,s->b.init,stamps[i],0,frontier))return;
 for(size_t i=0;i<sizeof(stamps);++i)
  if(gh_enemy_ems_start_stamp_action_00295ce8(s->enemies,s->b.init,stamps[i],frontier))return;
 for(;;){
  int ended;
  if(room_r100_camera_ended(s,&ended,frontier))return;
  if(ended)break;
  if(gh_task_threads_sleep(s->scenario_threads,1)!=GH_THREAD_OK){
   *frontier=0x2d5aa0;
   return;
  }
 }
 for(size_t i=0;i<sizeof(stamps);++i)
  if(gh_enemy_ems_set_stamp_alpha_00296230(s->enemies,s->b.init,stamps[i],1,frontier))return;
 gh_soft_event_end end={0};
 if(room_r100_end_live_soft(s,&end,frontier))return;
 w32(s->room_state.active+4,u32(s->room_state.active+4)|0x08000000u);
}

static void room_r200_entry_00000510(void *context,int32_t argument)
{
 gh_game_services *s=context;
 uint32_t *frontier=&s->r200_task_frontier;
 if(argument!=0||s->b.work->room!=0x200){*frontier=0x510;return;}
 if(room_r100_checkpoint(s,frontier)){if(!*frontier)*frontier=0x2c05b0;return;}
 w32((uint8_t *)s->r200_work.pointer+0x20,0);
 if(room_r101_side_release_002908c0(s,frontier))return;
 static const uint8_t stamps[]={0,1,2,4,0xb,6,7,8,9,0xc,0xd};
 for(size_t i=0;i<sizeof(stamps)/sizeof(stamps[0]);++i){
  gh_enemy_ems_set_call call={0};
  gh_game_title_player *actor=NULL;
  if(gh_enemy_ems_set_002952d0(s->enemies,&call,stamps[i],&actor,frontier)){
   if(!*frontier)*frontier=0x2952d0;
   return;
  }
 }
 if(!s->room_state.active||s->room_state.size<8){*frontier=0x688;return;}
 const uint32_t room_flags=u32(s->room_state.active+4);
 static const uint8_t ids[]={0,2,1,3,6,7,8};
 for(size_t i=0;i<sizeof(ids)/sizeof(ids[0]);++i){
  const uint8_t id=ids[i];
  if(id==6&&!(room_flags&0x10000000u))continue;
  const int enabled=id==1||id==3;
  gh_at_native_unit unit;
  int applied;
  if(gh_scene_at_find_id_002c3118(&s->scene_at,id,&unit)||
     gh_scene_at_set_enabled(&s->scene_at,unit,enabled,&applied)){
   *frontier=enabled?0x2c28f8:0x2c29a0;
   return;
  }
 }
 uint8_t *system_flags=(uint8_t *)s->b.work+0x5ec;
 w32(system_flags,(u32(system_flags)&~0x10000000u)|0x48000000u);


 const gh_scene_scenario_view scene={s->scenario_scalar,sizeof(s->scenario_scalar),&s->scenario};
 int32_t slot;
 if(room_flags&0x08000000u){
  if(room_r200_spawn_catapult(s,frontier))return;
 }else if(gh_scenario_exec_002c3890(scene,s->scenario_threads,-1,
     room_r200_catapult_00001020,s,0,&slot,frontier)){
  if(!*frontier)*frontier=0x2c3890;
  return;
 }
 if(!(room_flags&0x04000000u)){
  gh_at_native_unit jump;
  if(gh_scene_at_get_jump_002c2ae0(&s->scene_at,3,&jump)||
     gh_scene_at_set_jump_info_002c2750(&s->scene_at,jump,0xff,0x1208,0,0)){
   *frontier=0x2c2750;
   return;
  }
 }
 /* REL1578 selects the first bank because the work sentinel was cleared above. */
 static const uint16_t rewards[][2]={{0,0x3d9},{0xb,0x3d9},{0xc,0xa2e}};
 for(size_t i=0;i<sizeof(rewards)/sizeof(rewards[0]);++i){
  gh_game_title_player *actor;
  if(gh_enemy_manager_get_00290868(s->b.init,rewards[i][0],&actor)){
   *frontier=0x295e38;
   return;
  }
  if(actor)w32(actor->view.bytes+0x560,rewards[i][1]);
 }
}


/* REL 0CF8 checks the cEmWrap stamps on each original sleep boundary. */
static int room_r200_start_stamp(gh_game_services *s,uint8_t stamp,int if_inactive)
{
 gh_game_title_player *actor;
 if(gh_enemy_manager_get_00290868(s->b.init,stamp,&actor)){
  s->r200_task_frontier=0x295978;
  return 1;
 }
 if(!actor)return 0;
 if(actor->view.original_vtable214!=0x4479c8||actor->view.size<0x16d4){
  s->r200_task_frontier=0x295ce8;
  return 1;
 }
 if(!if_inactive||!(u32(actor->view.bytes+0x16d0)&2u))
  gh_enemy_start_action_00272570(actor->view.bytes);
 return 0;
}

static int room_r200_dead_pair(gh_game_services *s,uint8_t first,int *count)
{
 *count=0;
 const unsigned contestants=(u32(s->room_state.active+4)&0x40000000u)?3u:2u;
 for(unsigned index=0;index<contestants;++index){
  const uint8_t stamp=(uint8_t)(index==2&&first==6?5:first+index);
  gh_game_title_player *actor;
  if(gh_enemy_manager_get_00290868(s->b.init,stamp,&actor)){
   s->r200_task_frontier=0x295978;
   return 1;
  }
  int dead=1;
  if(actor&&gh_r100_enemy_dead_0028fb38(&s->room_state,actor,&dead)){
   s->r200_task_frontier=0x28fb38;
   return 1;
  }
  *count+=dead!=0;
 }
 return 0;
}

static int room_r200_sleep(gh_game_services *s,uint16_t frames)
{
 if(gh_task_threads_sleep(s->scenario_threads,frames)==GH_THREAD_OK)return 0;
 s->r200_task_frontier=0x2d5aa0;
 return 1;
}

static void room_r200_at06_wave_00000cf8(void *context,int32_t argument)
{
 gh_game_services *s=context;
 if(argument||!s->room_state.active||s->room_state.size<8){
  s->r200_task_frontier=0xcf8;
  return;
 }
 for(uint8_t stamp=5;stamp<=7;++stamp)
  if(room_r200_start_stamp(s,stamp,0))return;
 if(room_r200_sleep(s,5))return;
 for(int dead=0;dead<2;){
  if(room_r200_sleep(s,1)||room_r200_dead_pair(s,6,&dead))return;
 }
 for(uint8_t stamp=8;stamp<=10;++stamp)
  if(room_r200_start_stamp(s,stamp,1))return;
 if(room_r200_sleep(s,5))return;
 for(int dead=0;dead<2;){
  if(room_r200_sleep(s,1)||room_r200_dead_pair(s,8,&dead))return;
 }
 if(room_r200_sleep(s,1))return;
 (void)room_r200_start_stamp(s,11,1);
}

static void room_r200_at06_00000c90(void *context,int32_t argument)
{
 gh_game_services *s=context;
 uint32_t *frontier=&s->r200_task_frontier;
 if(argument||!s->room_state.active||s->room_state.size<8){*frontier=0xc90;return;}
 w32(s->room_state.active+4,u32(s->room_state.active+4)|0x10000000u);
 gh_at_native_unit unit;
 int applied;
 if(gh_scene_at_find_id_002c3118(&s->scene_at,6,&unit)||
    gh_scene_at_set_enabled(&s->scene_at,unit,0,&applied)||!applied){
  *frontier=0x2c29a0;
  return;
 }
 gh_scene_scenario_view scene={s->scenario_scalar,sizeof(s->scenario_scalar),&s->scenario};
 int32_t slot;
 if(gh_scenario_exec_002c3890(scene,s->scenario_threads,-1,room_r200_at06_wave_00000cf8,s,0,&slot,frontier))
  if(!*frontier)*frontier=0x2c3890;
}

static gh_scene_at_status r200_jmp3_gate_unit(gh_game_services *s,gh_at_native_unit *unit)
{
 if(s->b.work->room!=0x200||gh_scene_at_get_jump_002c2ae0(&s->scene_at,3,unit)||
    !unit->bytes||unit->size!=0x9c)return GH_AT_UNSUPPORTED;
 const uint8_t *row=unit->bytes;
 if(row[0x35]!=1||row[0x79]!=3||row[0x7a]||!(row[0x3b]&8)||
    u32(row+0x3c)!=0x1208||u32(row+0x40)||row[0x44]!=0xff)
  return GH_AT_UNSUPPORTED;
 return GH_AT_OK;
}

static void room_r200_jmp3_key_notice_00001208(void *context,int32_t argument)
{
 gh_game_services *s=context;
 (void)argument;
 /* REL1208 calls getKeyNum, then invokes the null-camera up-cut inline. */
 if((int32_t)u32(s->b.save.bytes+0x6c)>=2){
  s->r200_task_frontier=0x126c;
  return;
 }
 (void)room_message_inline_002c41c8(s,0x1000,&s->r200_task_frontier);
}

gh_action_step_status gh_game_services_r200_gate_action(void *context,uint32_t entry,
 void *argument,uint32_t *frontier)
{
 gh_game_services *s=context;
 if(!s||!frontier)return GH_ACTION_STEP_ARGUMENT;
 *frontier=entry;
 gh_at_native_unit unit;
 if(entry!=0x2c2138||r200_jmp3_gate_unit(s,&unit)||unit.bytes!=argument)
  return GH_ACTION_STEP_UNSUPPORTED;
 if(!s->b.save.bytes||s->b.save.size<0x70){
  *frontier=0x1faa00;return GH_ACTION_STEP_BOUNDS;
 }
 if(gh_scenario_clear_pad_0031b410(s->b.work)){
  *frontier=0x31b410;return GH_ACTION_STEP_CALLEE;
 }
 w32((uint8_t *)s->b.work+8,0);
 w32(unit.bytes+0x50,u32(unit.bytes+0x50)|0x02000000u);
 const gh_scene_scenario_view scene={s->scenario_scalar,sizeof(s->scenario_scalar),&s->scenario};
 int32_t slot;
 if(gh_scenario_exec_002c3890(scene,s->scenario_threads,-1,
    room_r200_jmp3_key_notice_00001208,s,0,&slot,frontier)){
  if(!*frontier)*frontier=0x2c38d8;
  return GH_ACTION_STEP_CALLEE;
 }
 if(slot>=0)w32(scene.bytes+0x58+(size_t)slot*8,unit.handle);
 *frontier=0;
 return GH_ACTION_STEP_OK;
}

gh_scene_at_status gh_game_services_r200_action_offer(void *context,
 const gh_scene_at_task_request *request,uint32_t *frontier)
{
 gh_game_services *s=context;
 if(!s||!request||!frontier)return GH_AT_ARGUMENT;
 *frontier=0x1f7858;
 if(request->argument_mode!=2||request->raw_argument||request->entry_address!=0x2c2138)
  return GH_AT_UNSUPPORTED;
 gh_at_native_unit unit;
 if(r200_jmp3_gate_unit(s,&unit)||request->unit.bytes!=unit.bytes||
    request->unit.size!=unit.size||request->unit.handle!=unit.handle||
    request->group!=unit.bytes[0x39]||request->mode!=unit.bytes[0x3a])
  return GH_AT_UNSUPPORTED;
 const gh_action_set_request action={.priority=request->group,.life=request->mode,
    .callback_id=request->entry_address,.argument=unit.bytes,.kind=2};
 int emitted;
 unsigned index;
 if(gh_action_button_set_001f7858(&s->action_buttons,&action,&emitted,&index,frontier)){
  if(!*frontier)*frontier=0x1f7858;
  return GH_AT_UNSUPPORTED;
 }
 return GH_AT_OK;
}
gh_scene_at_status gh_game_services_r200_area_dispatch(void *context,uint32_t entry,
 gh_at_native_unit area,uint8_t trigger,int *original_return,uint32_t *frontier)
{
 gh_game_services *s=context;
 if(!s||!original_return||!frontier)return GH_AT_ARGUMENT;
 *frontier=0x2c2138;
 if(s->b.work->room!=0x200||entry!=0x2c2138||!area.bytes||area.size!=0x9c||
    area.bytes[0x35]!=2||area.bytes[0x37]||u32(area.bytes+0x40)||area.bytes[0x44]!=0xff)
  return GH_AT_UNSUPPORTED;
 const uint8_t id=area.bytes[0x36];
 if((id!=0&&id!=6)||u32(area.bytes+0x3c)!=(id==6?0xc90u:0x510u))
  return GH_AT_UNSUPPORTED;
 gh_at_native_unit canonical;
 if(gh_scene_at_find_id_002c3118(&s->scene_at,id,&canonical)||
    canonical.handle!=area.handle||canonical.bytes!=area.bytes)
  return GH_AT_UNSUPPORTED;
 *original_return=1;
 if(id==6){
  gh_scene_scenario_view scene={s->scenario_scalar,sizeof(s->scenario_scalar),&s->scenario};
  int32_t slot;
  if(gh_scenario_exec_002c3890(scene,s->scenario_threads,-1,room_r200_at06_00000c90,s,0,&slot,frontier))
   return GH_AT_UNSUPPORTED;
  if(slot>=0)w32(scene.bytes+0x58+(size_t)slot*8,trigger);
  *frontier=0;
  return GH_AT_OK;
 }
 if(!s->r200_entry_started){
  gh_scene_scenario_view scene={s->scenario_scalar,sizeof(s->scenario_scalar),&s->scenario};
  int32_t slot;
  if(gh_scenario_exec_002c3890(scene,s->scenario_threads,-1,room_r200_entry_00000510,s,0,&slot,frontier))
   return GH_AT_UNSUPPORTED;
  if(slot>=0)w32(scene.bytes+0x58+(size_t)slot*8,trigger);
  s->r200_entry_started=1;
 }
 *frontier=0;
 return GH_AT_OK;
}

static int room_r101_arena_jumps(gh_game_services *s,int enabled,uint32_t *frontier)
{
 for(unsigned index=2;index<=3;++index){
  gh_at_native_unit unit;
  int applied;
  if(gh_scene_at_get_jump_002c2ae0(&s->scene_at,index,&unit)){
   *frontier=0x2c2ae0;
   return 1;
  }
  if(gh_scene_at_set_enabled(&s->scene_at,unit,enabled,&applied)||!applied){
   *frontier=enabled?0x2c28f8:0x2c29a0;
   return 1;
  }
 }
 return 0;
}

static int room_r101_arena_wave(gh_game_services *s,const uint8_t *stamps,size_t count)
{
 uint32_t *frontier=&s->r101_task_frontier;
 for(size_t i=0;i<count;++i){
  gh_enemy_ems_set_call call={0};
  gh_game_title_player *actor;
  if(gh_enemy_ems_set_002952d0(s->enemies,&call,stamps[i],&actor,frontier)){
   if(!*frontier)*frontier=0x2952d0;
   return 1;
  }
  w32(s->r101_work.pointer,u32(s->r101_work.pointer)+1u);
 }
 for(size_t i=0;i<count;++i){
  if(gh_enemy_ems_start_stamp_action_00295ce8(s->enemies,s->b.init,stamps[i],frontier)){
   if(!*frontier)*frontier=0x295ce8;
   return 1;
  }
 }
 return 0;
}

static int room_r101_arena_wait_clear(gh_game_services *s)
{
 for(;;){
  uint32_t count;
  if(room_live_enemy_count(s,&count)){
   s->r101_task_frontier=0x1808;
   return 1;
  }
  if(!count)return 0;
  if(room_r101_gate_sleep(s,1))return 1;
 }
}

static int room_r101_arena_create_card(gh_game_services *s)
{
 gh_om_runtime_create create={0};
 gh_om_object *card=NULL;
 if(gh_om_factory_create_0030fa98(&s->om_factory,&create,0x3e3,0xffff,&card)){
  s->r101_task_frontier=s->om_factory.frontier?s->om_factory.frontier:0x30fa98;
  return 1;
 }
 if(!card)return 0;
 int initialized=0;
 if(gh_om_factory_initialize_runtime_item_001c0128(&s->om_factory,card,&initialized)||!initialized){
  s->r101_task_frontier=s->om_factory.frontier?s->om_factory.frontier:0x1c0128;
  return 1;
 }
 const uint32_t position[4]={0xc119999a,0,0xc2008f5c,0x3f800000};
 if(gh_om_r101_item_position_001c2630(card->initialization,(const uint8_t *)position)){
  s->r101_task_frontier=card->initialization->frontier?card->initialization->frontier:0x1c2630;
  return 1;
 }
 return 0;
}

static void room_r101_arena_00000d68(void *context,int32_t argument)
{
 gh_game_services *s=context;
 uint32_t *frontier=&s->r101_task_frontier;
 if(argument||!s->r101_work.pointer){*frontier=0xd68;return;}
 if(room_r101_gate_sleep(s,1))return;
 uint8_t *flags=s->room_state.active+4;
 w32(flags,u32(flags)|0x20000000u);
 if(room_r101_arena_jumps(s,0,frontier))return;
 if(gh_r100_scroll_layer_002db2b0(&s->pools,&s->meshes,4,1,frontier)){
  if(!*frontier)*frontier=0x2db2b0;
  return;
 }

 static const uint8_t first[]={0x16,0x20},first_high[]={0x17};
 static const uint8_t second[]={0x18},second_normal[]={0x27,0x28},second_high[]={0x19,0x21};
 static const uint8_t third[]={0x1a,0x1b,0x22,0x23},third_high[]={0x1c,0x1d,0x24,0x25};
 static const uint8_t fourth[]={0x1e,0x26},fourth_high[]={0x1f};
 static const struct {
  const uint8_t *base,*normal,*high;
  size_t base_count,normal_count,high_count;
 } waves[]={
  {first,NULL,first_high,sizeof(first),0,sizeof(first_high)},
  {second,second_normal,second_high,sizeof(second),sizeof(second_normal),sizeof(second_high)},
  {third,NULL,third_high,sizeof(third),0,sizeof(third_high)},
  {fourth,NULL,fourth_high,sizeof(fourth),0,sizeof(fourth_high)},
 };
 for(size_t i=0;i<sizeof(waves)/sizeof(waves[0]);++i){
  if(room_r101_arena_wave(s,waves[i].base,waves[i].base_count))return;
  unsigned level;
  if(gh_core_save_game_level_001fa010(s->b.save,s->b.boot_elf,s->b.boot_elf_bytes,&level)){
   *frontier=0x1fa010;
   return;
  }
  if(room_r101_arena_wave(s,level>4?waves[i].high:waves[i].normal,
     level>4?waves[i].high_count:waves[i].normal_count)||
     room_r101_arena_wait_clear(s)||room_r101_gate_sleep(s,60)||room_r101_arena_wait_clear(s))
   return;
 }
 if(room_r101_arena_create_card(s)||room_r101_gate_sleep(s,30)||room_r101_gate_fade_seal(s,4,0.02f))
  return;
 if(gh_r100_scroll_layer_002db2b0(&s->pools,&s->meshes,4,0,frontier)){
  if(!*frontier)*frontier=0x2db2b0;
  return;
 }
 (void)room_r101_arena_jumps(s,1,frontier);
}

/* 294CA0 revisits the current non-dead, activated RoomSave rows and
 * reapplies saved vitality after their normal native creation. */
static gh_door_status room_r101_restore_ems_00294ca0(gh_game_services *s,uint32_t *frontier)
{
 const uint8_t *page=s->room_state.active;
 if(!page||s->room_state.size<12||page[8]>64||
    s->room_state.size<12+24u*page[8]){
  *frontier=0x294ca0;
  return GH_DOOR_UNSUPPORTED;
 }
 for(unsigned index=0;index<page[8];++index){
  const uint8_t *row=page+12+24u*index;
  if(row[0x14]==1||(u32(row+8)&0x80000000u))continue;
  gh_enemy_ems_set_call call={0};
  gh_game_title_player *actor=NULL;
  if(gh_enemy_ems_set_002952d0(s->enemies,&call,row[0x13],&actor,frontier)){
   if(!*frontier)*frontier=0x294ca0;
   return GH_DOOR_UNSUPPORTED;
  }
  if(actor){
   int16_t hp;
   memcpy(&hp,row+0x16,sizeof(hp));
   if(gh_enemy_apply_saved_vitality_00294f34(actor,hp)){
    *frontier=0x294f34;
    return GH_DOOR_UNSUPPORTED;
   }
  }
 }
 return GH_DOOR_OK;
}

static gh_door_status room_r101_side_entry_00001490(gh_game_services *s,uint32_t argument,
 uint32_t *frontier)
{
 const int entering=argument==2;
 if(!entering&&argument!=3){*frontier=0x1490;return GH_DOOR_UNSUPPORTED;}
 if(s->r101_side_door_entry_stage==0){
  if(room_r100_program_se(s,3,frontier))return GH_DOOR_UNSUPPORTED;
  uint8_t *flags=s->room_state.active+4;
  w32(flags,entering?u32(flags)|0x40000000u:u32(flags)&~0x40000000u);
  s->r101_side_door_entry_stage=1;
 }
 if(!entering&&s->r101_side_door_entry_stage==1){
  const gh_door_status restored=room_r101_restore_ems_00294ca0(s,frontier);
  if(restored)return restored;
  static const uint8_t carriers[]={9,7};
  for(unsigned i=0;i<sizeof(carriers);++i){
   gh_game_title_player *actor=NULL;
   if(gh_enemy_manager_get_00290868(s->b.init,carriers[i],&actor)){
    *frontier=0x295e38;
    return GH_DOOR_UNSUPPORTED;
   }
   if(actor)w32(actor->view.bytes+0x560,0x3e3);
  }
  static const float escape[4]={-.53f,0,-49.22f,1};
  for(unsigned stamp=16;stamp<=17;++stamp){
   gh_game_title_player *actor=NULL;
   if(gh_enemy_manager_get_00290868(s->b.init,(uint8_t)stamp,&actor)||
      (actor&&gh_enemy2a2_set_escape_position_00289038(actor,escape))){
    *frontier=0x289038;
    return GH_DOOR_UNSUPPORTED;
   }
  }
  s->r101_side_door_entry_stage=2;
 }
 if(!entering&&s->r101_side_door_entry_stage==2){
  if(room_r100_checkpoint(s,frontier))return GH_DOOR_UNSUPPORTED;
  s->r101_side_door_entry_stage=4;
 }
 if(s->r101_side_door_entry_stage==1){
  if(room_r100_checkpoint(s,frontier))return GH_DOOR_UNSUPPORTED;
  s->r101_side_door_entry_stage=2;
 }
 if(s->r101_side_door_entry_stage==2){
  const gh_door_status released=room_r101_side_release_002908c0(s,frontier);
  if(released)return released;
  s->r101_side_door_entry_stage=3;
 }
 if(s->r101_side_door_entry_stage==3){
  if(!(u32(s->room_state.active+4)&0x20000000u)){
   const gh_scene_scenario_view scene={s->scenario_scalar,sizeof(s->scenario_scalar),&s->scenario};
   int32_t slot;
   if(gh_scenario_exec_002c3890(scene,s->scenario_threads,-1,room_r101_arena_00000d68,s,0,&slot,frontier))
    return GH_DOOR_UNSUPPORTED;
  }
  s->r101_side_door_entry_stage=4;
 }
 for(unsigned layer=0;layer<4;++layer){
  const int shown=(layer&1u)==(unsigned)entering;
  if(gh_r100_scroll_layer_002db2b0(&s->pools,&s->meshes,layer,shown,frontier))
   return GH_DOOR_UNSUPPORTED;
 }
 if(room_r101_om_layer(s,2,!entering)||room_r101_om_layer(s,1,entering)){
  *frontier=0x2db478;
  return GH_DOOR_UNSUPPORTED;
 }
 if(gh_r100_scroll_layer_002db2b0(&s->pools,&s->meshes,4,0,frontier))
  return GH_DOOR_UNSUPPORTED;
 w32(s->scenario_scalar+0xf4,0);
 s->r101_side_door_entry_stage=0;
 *frontier=0;
 return GH_DOOR_OK;
}

gh_door_status gh_game_services_r101_side_door_step(gh_game_services *s,const gh_game_door *door,
 uint32_t address,uint32_t *frontier)
{
 if(!s||!door||!frontier)return GH_DOOR_ARGUMENT;
 *frontier=address;
 const uint32_t entry=u32(s->scenario_scalar+0xf4);
 const uint32_t argument=u32(s->scenario_scalar+0xf8);
 if(door!=&s->door||s->b.work->room!=0x101||door->target_room!=-1||
    entry!=0x1490||(argument!=2&&argument!=3)||door->point!=5-argument)
  return GH_DOOR_UNSUPPORTED;
 if(address==0x2c4de0)
  return room_r101_side_entry_00001490(s,argument,frontier);
 if(address!=0x2c4440)return GH_DOOR_UNSUPPORTED;
 gh_title_player_view *player=&s->title_player.view;
 if(!s->title_player.ready||!player->bytes||!player->alias_f0)return GH_DOOR_UNSUPPORTED;
 uint8_t *actor=player->bytes;
 if(s->r101_side_door_pose_stage==0){
  const uint32_t angles[3]={0,door->yaw_bits,0};
  if(room_r100_successor_player(s,door->position_bits,angles,frontier))
   return GH_DOOR_UNSUPPORTED;
  memcpy(actor+0x30,door->position_bits,12);
  float translation[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
  float rotation[16],matrix[16],euler[3];
  memcpy(translation+12,player->alias_f0,12);
  memcpy(euler,angles,sizeof(euler));
  if(gh_actor_matrix_rotation_00147728(euler,actor[0x140],rotation,GH_STATIC_BINARY32_REFERENCE,frontier)||
     gh_actor_matrix_product_00147548(translation,rotation,matrix,GH_STATIC_BINARY32_REFERENCE,frontier))
   return GH_DOOR_UNSUPPORTED;
  memcpy(actor+0x80,matrix,sizeof(matrix));
  s->r101_side_door_pose_stage=1;
 }
 if(s->r101_side_door_pose_stage==1){
  if(room_soft_end_clear(s,player,frontier))return GH_DOOR_UNSUPPORTED;
  uint8_t *work=(uint8_t *)s->b.work;
  w32(work+0x5d8,u32(work+0x5d8)&0xffdfffffu);
  s->r101_side_door_pose_stage=2;
 }
 if(s->r101_side_door_pose_stage==2){
  if(room_soft_end_camera_both(s,frontier))return GH_DOOR_UNSUPPORTED;
  s->r101_side_door_pose_stage=3;
 }
 if(room_soft_end_camera_both(s,frontier))return GH_DOOR_UNSUPPORTED;
 s->scenario_scalar[0x111]=0;
 s->r101_side_door_pose_stage=0;
 *frontier=0;
 return GH_DOOR_OK;
}

static gh_r100_script_status room_r100_scripts_bind(gh_game_services*s){
 if(s->r100_scripts.bound)return GH_R100_SCRIPT_OK;
 gh_scene_scenario_view scenario={s->scenario_scalar,sizeof(s->scenario_scalar),&s->scenario};
 gh_r100_script_bindings b={.scenario=scenario,.threads=s->scenario_threads,.room=&s->room_state,
  .work=s->b.work,.disc=s->b.assets->dvd->disc,.init=s->b.init,.pools=&s->pools,
  .soft={.scenario=scenario,.work=s->b.work,.player_abort_00126770=room_soft_player_abort,.context=s},
  .play={.event=s->b.event,.config=s->b.event_config,.work=s->b.work,.player=&s->title_player.view,
   .fade=s->b.fade,.display=s->b.display,.elf=s->b.boot_elf,.elf_bytes=s->b.boot_elf_bytes},
  .register_other=room_r100_register_other,.object_view=room_r100_om_view,
  .get_enemy=room_r100_enemy,.set_escape=room_r100_enemy_escape,
  .collision=room_r100_collision,.door=room_r100_door,
  .end_soft_002c3c10=room_r100_end_soft,.checkpoint_002c05b0=room_r100_checkpoint,.context=s};
 return gh_r100_scripts_bind(&s->r100_scripts,&b);
}
static gh_r100_init_status room_r100_setup(void*p,uint32_t*frontier){
 gh_game_services*s=p;gh_r100_script_status e=room_r100_scripts_bind(s);
 if(!e)e=gh_r100_script_setup_00000bc0(&s->r100_scripts);
 if(frontier)*frontier=s->r100_scripts.frontier_relative?0:s->r100_scripts.frontier;
 /* A direct BC0 failure in another REL body is returned by its owning script;
  * the Init callback has only a main-ELF frontier output. */
 if(e&&s->r100_scripts.frontier_relative){s->r100.frontier=s->r100_scripts.frontier;s->r100.frontier_relative=1;}
 return !e?GH_R100_INIT_OK:e==GH_R100_SCRIPT_UNSUPPORTED?GH_R100_INIT_UNSUPPORTED:GH_R100_INIT_CALLEE;
}
static gh_r100_init_status room_r100_task_exec(void*p,uint32_t entry,int32_t argument,uint32_t*frontier){
 gh_game_services*s=p;if(frontier)*frontier=0;gh_r100_script_status e=GH_R100_SCRIPT_OK;
 uint32_t f=0;int32_t slot=-1;
 if(entry==0x2f30){
  if(!s->r100_roof.bound){gh_r100_roof_bindings b={.room_init=&s->r100,.room=&s->room_state,.pools=&s->pools,
    .actors=&s->scene_tables,.scenario={s->scenario_scalar,sizeof(s->scenario_scalar),&s->scenario},
    .threads=s->scenario_threads,.object_view=room_r100_om_view,.context=s};e=gh_r100_roof_bind(&s->r100_roof,&b);}
  if(!e)e=gh_r100_roof_register_00002f30(&s->r100_roof,argument,&slot);f=s->r100_roof.frontier;
 }else if(entry==0x9b0){
  e=room_r100_scripts_bind(s);if(!e)e=gh_r100_scripts_register_000009b0(&s->r100_scripts,argument,&slot);
  if(!s->r100_scripts.frontier_relative)f=s->r100_scripts.frontier;
 }else if(entry==0x10f8){
  if(!s->r100_counter.bound){gh_r100_counter_bindings b={.room_init=&s->r100,.room=&s->room_state,.init=s->b.init,
    .scenario={s->scenario_scalar,sizeof(s->scenario_scalar),&s->scenario},.threads=s->scenario_threads};
   e=gh_r100_counter_bind(&s->r100_counter,&b);}
  if(!e)e=gh_r100_counter_register_000010f8(&s->r100_counter,argument,&slot);
  if(!s->r100_counter.frontier_relative)f=s->r100_counter.frontier;
 }else if(gh_r100_live_entry(entry)){
  const gh_r100_live_bindings bindings={.scenario={s->scenario_scalar,sizeof(s->scenario_scalar),&s->scenario},
   .threads=s->scenario_threads,.room_init=&s->r100,.room=&s->room_state,.at=&s->scene_at,
   .init=s->b.init,.work=s->b.work,.player=&s->title_player.view,
   .enemies=&s->enemies,.checkpoint=room_r100_checkpoint,.context=s,
   .math=s->b.math_globals,.elf=s->b.boot_elf,.elf_size=s->b.boot_elf_bytes,.scene_tables=&s->scene_tables,
   .start_soft_event=room_r100_start_soft,.set_room_camera=room_r100_set_camera,
   .program_se=room_r100_program_se,.pools=&s->pools,.object_view=room_r100_om_view,
   .camera_ended=room_r100_camera_ended,.end_soft_event=room_r100_end_live_soft,
   .battle_door=room_r100_door,.battle_collision=room_r100_collision,
   .battle_register=room_r100_register_other,.battle_player_pose=room_r100_successor_player};
  e=gh_r100_live_register(&s->r100_live,&bindings,entry,argument,&f);
 }else return GH_R100_INIT_UNSUPPORTED;
 if(e){if(frontier)*frontier=f;return e==GH_R100_SCRIPT_UNSUPPORTED?GH_R100_INIT_UNSUPPORTED:GH_R100_INIT_CALLEE;}
 return GH_R100_INIT_OK;
}
static gh_r100_init_status room_tutorial_init(void*p,uint32_t*frontier){
 gh_game_services*s=p;gh_tutorial_bindings b={.cockpit=s->cockpit,.dvd=s->b.assets->dvd,.room_heap=s->b.assets->heap,
  .id=s->b.id,.fonts=s->b.font,.packed=s->b.packed,.language=s->b.language,.work=s->b.work};
 gh_tutorial_status e=gh_cockpit_tutorial_room_init_00147028(&s->tutorial,&b);
 if(frontier)*frontier=s->tutorial.frontier;
 return !e?GH_R100_INIT_OK:GH_R100_INIT_CALLEE;
}
static gh_r100_init_status room_tutorial_load(void*p,uint16_t kind,int reference,uint32_t*frontier){
 gh_game_services*s=p;gh_object_view resource={0};
 gh_object_status e=gh_game_objects_load_wait_001fe8a0(&s->tutorial_load,s->objects,kind,&s->object_resources,reference,&resource,frontier);
 return !e?GH_R100_INIT_OK:e==GH_OBJECT_UNSUPPORTED?GH_R100_INIT_UNSUPPORTED:GH_R100_INIT_CALLEE;
}
static gh_r100_init_status room_tutorial_create_at(gh_game_services*s,unsigned index,
 uint16_t kind,uint16_t id,gh_r100_om_view*out,uint32_t*frontier){
 *out=(gh_r100_om_view){0};
 gh_scene_placement_status e=gh_om_factory_create_0030fa98(&s->om_factory,&s->tutorial_create[index],kind,id,&s->tutorial_objects[index]);
 if(frontier)*frontier=s->om_factory.frontier;
 if(e)return e==GH_PLACE_UNSUPPORTED?GH_R100_INIT_UNSUPPORTED:GH_R100_INIT_CALLEE;
 gh_om_object*o=s->tutorial_objects[index];if(o)*out=(gh_r100_om_view){o->view.bytes,o->view.size,o->view.bytes};
 return GH_R100_INIT_OK;
}
static gh_r100_init_status room_tutorial_create(void*p,uint16_t kind,uint16_t id,gh_r100_om_view*out,uint32_t*frontier){
 gh_game_services*s=p;
 if(s->r100_tail.stage<4||s->r100_tail.stage>=16||(s->r100_tail.stage-4)%3)return GH_R100_INIT_ARGUMENT;
 return room_tutorial_create_at(s,(s->r100_tail.stage-4)/3,kind,id,out,frontier);
}
static gh_om_object *room_tutorial_object(gh_game_services*s,const gh_r100_om_view*v){
 if(!v||!v->bytes||v->identity!=v->bytes)return NULL;
 for(unsigned i=0;i<4;i++){gh_om_object*o=s->tutorial_objects[i];if(o&&o->view.bytes==v->bytes&&o->view.size==v->size)return o;}
 return NULL;
}
static gh_r100_init_status room_tutorial_object_init(void*p,const gh_r100_om_view*v,uint32_t*frontier){
 gh_game_services*s=p;gh_om_object*o=room_tutorial_object(s,v);if(!o)return GH_R100_INIT_ARGUMENT;
 int original=0;gh_scene_placement_status e=gh_om_factory_initialize_runtime_3eb(&s->om_factory,o,&original);
 if(frontier)*frontier=s->om_factory.frontier;
 return !e?GH_R100_INIT_OK:e==GH_PLACE_UNSUPPORTED?GH_R100_INIT_UNSUPPORTED:GH_R100_INIT_CALLEE;
}
static gh_r100_init_status room_tutorial_position(void*p,const gh_r100_om_view*v,const float position[4],uint32_t*frontier){
 gh_game_services*s=p;gh_om_object*o=room_tutorial_object(s,v);if(!o)return GH_R100_INIT_ARGUMENT;
 gh_scene_placement_status e=gh_om3eb_set_position_001b7e50(o,position);if(e&&frontier)*frontier=0x1b7e50;
 return !e?GH_R100_INIT_OK:e==GH_PLACE_UNSUPPORTED?GH_R100_INIT_UNSUPPORTED:GH_R100_INIT_CALLEE;
}
/* R105 REL 03B8 issues one runtime tutorial with the original selector 4. */
static gh_scene_status room_r105_tutorial(void *context,uint32_t *frontier)
{
 gh_game_services *s=context;
 gh_r100_om_view object;
 gh_r100_init_status status=room_tutorial_init(s,frontier);
 if(status)goto done;
 status=room_tutorial_load(s,0x3eb,1,frontier);
 if(status)goto done;
 status=room_tutorial_create_at(s,0,0x3eb,0xffff,&object,frontier);
 if(status||!object.bytes)goto done;
 status=room_tutorial_object_init(s,&object,frontier);
 if(status)goto done;
 static const uint32_t position_bits[4]={0xc1180000,0xbfa7ae15,0x426d6666,0x3f800000};
 float position[4];
 memcpy(position,position_bits,sizeof(position));
 status=room_tutorial_position(s,&object,position,frontier);
 if(!status)object.bytes[0x600]=4;
done:
 return !status?GH_SCENE_OK:status==GH_R100_INIT_UNSUPPORTED?GH_SCENE_UNSUPPORTED:GH_SCENE_CALLEE;
}
static gh_scene_status room_r100_init(void*p,uint32_t*frontier){
 gh_game_services*s=p;
 gh_r100_init_bindings b={.work=s->b.work,.room_state=&s->room_state,.at=&s->scene_at,
  .pools=&s->pools,.meshes=&s->meshes,.default_heap=s->b.assets->heap,.actor_tables=&s->scene_tables,
  .object_view=room_r100_om_view,.task_exec=room_r100_task_exec,.context=s,.setup_00000bc0=room_r100_setup,
  .tail=&s->r100_tail,.tail_bindings=&s->r100_tail_bindings};
 s->r100_tail_bindings=(gh_r100_init_tail_bindings){.save=&s->b.save,.work=s->b.work,
  .tutorial_room_init_00147028=room_tutorial_init,.load_wait_001fe8a0=room_tutorial_load,
  .create_object_0030fa98=room_tutorial_create,.virtual_init44=room_tutorial_object_init,
  .virtual_set_position74=room_tutorial_position,.context=s};
 gh_r100_init_status e=gh_r100_initialize_00000208(&s->r100,&b);
 if(e&&s->r100_scripts.status!=GH_R100_SCRIPT_OK&&s->r100_scripts.frontier_relative){
  s->r100.frontier=s->r100_scripts.frontier;s->r100.frontier_relative=1;
 }
 if(frontier)*frontier=s->r100.frontier;
 s->frontier=s->r100.frontier;s->frontier_module_code=s->r100.frontier_relative?2:0;
 return e?(e==GH_R100_INIT_UNSUPPORTED?GH_SCENE_UNSUPPORTED:GH_SCENE_CALLEE):GH_SCENE_OK;
}
static int initialize_children(void*context,gh_stage_case1*stage){
 gh_game_services*s=context;gh_child_status e;(void)stage;
 if(!*s->b.children){e=gh_game_children_create(NULL,s->b.children);if(e){fail(s,GH_SERVICES_MEMORY,0x1612c8,e);return e;}s->owns_children=1;s->created_children=*s->b.children;}
 e=gh_game_children_initialize_0015fd98(*s->b.children);if(e)fail(s,GH_SERVICES_ASSET,0x15fd98,e);return e;
}
static int initialize_cockpit(void*context,gh_stage_case1*stage){
 gh_game_services*s=context;gh_id_error e;(void)stage;
 if(!s->cockpit){e=gh_game_cockpit_create(*s->b.runtime,s->b.id,&s->b.init->random,&s->cockpit);if(e){fail(s,GH_SERVICES_MEMORY,0x141678,e);return e;}}
 e=gh_game_cockpit_initialize_00140c88(s->cockpit,s->b.save);
 s->cockpit_pool_end=s->b.id->pool_used;s->cockpit_jobs_end=s->b.id->job_count;s->cockpit_snapshot=1;
 if(e)fail(s,e==GH_ID_UNSUPPORTED?GH_SERVICES_FRONTIER:(e==GH_ID_MEMORY?GH_SERVICES_MEMORY:GH_SERVICES_ASSET),gh_game_cockpit_frontier(s->cockpit)?gh_game_cockpit_frontier(s->cockpit):0x140c88,e);return e;
}
static int valid_bindings(const gh_game_services_bindings*b){
 return b&&b->assets&&b->loop&&b->init&&b->stage&&b->children&&b->save.bytes&&b->save.size>=GH_CORE_SAVE_BYTES&&
  b->rate&&b->work&&b->threads&&b->id&&b->id_global&&b->runtime&&b->font&&b->menu&&b->core_bin&&b->core_bin_size&&b->language&&
  b->assets->work==b->work&&b->loop->work==b->work&&b->assets->threads==b->threads&&b->loop->threads==b->threads&&b->assets->heap&&b->assets->dvd;
}
#ifdef GH_NATIVE_STARTUP_QA
void gh_game_services_qa_enable_r105_exit_fixture(gh_game_services *s)
{
 if(s)s->qa_r105_exit_fixture=1;
}
#endif

static gh_object_status event_effect_source_borrowed(void *context,const void *source,
 size_t bytes,int *borrowed)
{
 return gh_effect_create_source_has_live_refs(context,source,bytes,borrowed)?
     GH_OBJECT_UNSUPPORTED:GH_OBJECT_OK;
}

gh_game_services_status gh_game_services_create(const gh_game_services_bindings*b,gh_game_services**out){
 gh_game_services*s;gh_object_status oe;gh_camera_status ce;gh_thread_status te;gh_game_services_status result=GH_SERVICES_MEMORY;
 if(!out)return GH_SERVICES_ARGUMENT;*out=NULL;if(!valid_bindings(b))return GH_SERVICES_ARGUMENT;
 if(b->camera_envelope&&!b->camera_envelope->loaded)return GH_SERVICES_ARGUMENT;
 if(b->camera_envelope&&b->camera_envelope->player_model)return GH_SERVICES_BUSY;
 if(b->stage->global_slots.configured||b->stage->room_slots.configured||b->stage->skills.image)return GH_SERVICES_BUSY;
 s=calloc(1,sizeof(*s));if(!s)return GH_SERVICES_MEMORY;s->b=*b;
 s->model_effect=(gh_esp14_provider){
  .context=s,.initialize=initialize_model_effect,.move=move_model_effect,.trans=trans_model_effect,
  .destroy_hidden=destroy_model_effect,.collect_retired=collect_model_effects
 };
 if(b->camera_envelope)(void)gh_game_camera_envelope_bind_model(b->camera_envelope,&s->player_model);
 s->object_resources=(gh_object_resource_bindings){.dvd=b->assets->dvd,.heap=b->assets->heap,.threads=b->threads,
  .sound_check=b->enemy_banks?gh_enemy_se_object_sound_check:NULL,.sound_context=b->enemy_banks};
 s->old_context=b->stage->context;s->old_children=b->stage->init_children_15fd98;s->old_cockpit=b->stage->init_cockpit_140c88;
 oe=gh_game_objects_create(&b->stage->global_slots,0,&s->objects);if(oe)goto create_failure;
 oe=gh_game_objects_create(&b->stage->room_slots,1,&s->room_objects);if(oe)goto create_failure;
 gh_game_objects_bind_source_borrower(s->room_objects,event_effect_source_borrowed,&s->effects);
 ce=gh_game_camera_construct_pl_00139bc8(&s->camera,&b->init->camera,GH_CAMERA_NATIVE_BINARY32_REFERENCE);if(ce){result=GH_SERVICES_ASSET;goto create_failure;}
 if(gh_game_door_from_disc(&s->door,b->assets->dvd->disc)!=GH_DOOR_OK){result=GH_SERVICES_ASSET;goto create_failure;}
 s->scenario=(gh_task_manager){.records=s->scenario_records,.capacity=16,.count=16,.current_index=-1};
 for(unsigned i=0;i<16;i++)s->scenario_records[i].thread_id=-1;
 /* ActualELF5FEB00..114 isPT_LOADBSS;2C5338 storesVF0at0,constructs
  * manager1C..4Fandsetsderivedvptr4C. Do notduplicate its nativepointer
  * fields; bytes1C..4Farediagnostic/reserved, nativeScenarioisabove. */
 w32(s->scenario_scalar+12,0x3f800000);w32(s->scenario_scalar+0x4c,0x44d1a8);
 /* Lastpotentialfailingconstructor: newbackendinheritsRootcaller even when
  * thiscreateexecutesinsideMainfiber; noforcedforeignfiber tick. */
 te=gh_task_threads_create(&s->scenario,&s->scenario_threads);if(te){result=GH_SERVICES_THREAD;goto create_failure;}
 s->room_pack=(gh_room_pack){.dvd=b->assets->dvd,.heap=b->assets->heap,.threads=b->threads};
 s->module.input=(gh_room_pack){.dvd=b->assets->dvd,.heap=b->assets->heap,.threads=b->threads};
 /* Actual2B8AE0 is a scalar constructor; allocation occurs only at2B8B50. */
 (void)gh_collision_pairs_construct_002b8ae0(&s->collision_pairs,b->assets->heap);
 (void)gh_game_world_light_construct_002d6d80(&s->world_light);
 s->em_placements.stop_flag=b->em_set_first_byte;gh_om_scene_construct_002a7308(&s->om_placements);
 gh_game_camera_path_construct_00137a48(&s->acd);
 /* NoPFD path neverreads unprovenMain1A8; nativecompanionseed isexplicit0.
  * Actualcase3tail setscooldown15 beforeitsfirstsourceconsumer. */
 gh_game_scene_step_construct(&s->step,0,0);
 *out=s;return GH_SERVICES_OK;
create_failure:
 if(b->camera_envelope)(void)gh_game_camera_envelope_bind_model(b->camera_envelope,NULL);
 gh_game_objects_destroy(s->room_objects);gh_game_objects_destroy(s->objects);free(s);return result;
}
gh_game_services_status gh_game_services_state1(gh_game_services*s){
 gh_stage_status e;gh_id_error ie;gh_stage_case1*stage;
 if(!s||s->b.loop->game_state[0]!=1||!*s->b.runtime)return GH_SERVICES_ARGUMENT;
 if(s->stage_attempted)return fail(s,GH_SERVICES_BUSY,0x2a5bb8,0);
 /* A realMC/otherIDowner mustreleaseitsruns first. Never resetthecanonical
  * manager beneath livehostobjects or silently destroytheiridentity. */
 if(s->b.id->job_count||s->b.id->pool_used)return fail(s,GH_SERVICES_BUSY,0x2acc70,GH_ID_BUSY);
 ie=gh_id_runtime_set_path_policy(*s->b.runtime,GH_ID_PATH_REFERENCE_NEAREST);if(ie)return fail(s,GH_SERVICES_BUSY,0x2acc70,ie);
 s->stage_attempted=1;s->frontier=0;s->detail=0;stage=s->b.stage;
 if(!stage->textures){e=gh_stage_textures_create(&stage->textures);if(e)return fail(s,GH_SERVICES_MEMORY,0x1508f8,e);s->owns_textures=1;s->created_textures=stage->textures;}
 stage->assets=s->b.assets;stage->core_bin=s->b.core_bin;stage->core_bin_bytes=s->b.core_bin_size;stage->language=s->b.language;
 stage->font=s->b.font;stage->menu=s->b.menu;stage->id=s->b.id;stage->id_global=s->b.id_global;
 stage->game_state=&s->b.loop->game_state[0];stage->save=s->b.save;stage->room_heap=s->b.assets->heap;
 stage->context=s;stage->init_children_15fd98=initialize_children;stage->init_cockpit_140c88=initialize_cockpit;
 e=gh_game_stage_002a5bb8(stage);if(stage->skills.image)s->owns_skills=1;
 if(e){if(!s->frontier)fail(s,e==GH_STAGE_UNAVAILABLE||e==GH_STAGE_UNSUPPORTED?GH_SERVICES_FRONTIER:GH_SERVICES_ASSET,stage->failed_entry,stage->detail);return s->status;}
 s->frontier=0;s->detail=0;s->status=GH_SERVICES_OK;return GH_SERVICES_OK;
}
gh_game_services_status gh_game_services_state2(gh_game_services*s){
 if(!s||s->b.loop->game_state[0]!=2)return GH_SERVICES_ARGUMENT;
 gh_init_status e=gh_game_prepare_002a5d40(&s->prepare,s->b.save,s->b.work->room,&s->b.loop->game_state[0]);
 if(e)return fail(s,GH_SERVICES_ASSET,0x2a5d40,e);
 /* Native continuation cursor belongs to one original case3 invocation.
  * The canonical services/owners persist when state2 prepares another room. */
 s->case3_stage=0;s->case3_reloaded_player=0;s->frontier_module_code=0;
 s->frontier=0;s->detail=0;s->status=GH_SERVICES_OK;return GH_SERVICES_OK;
}
gh_game_services_status gh_game_services_state3(gh_game_services*s){
 gh_boot_work*work;uint8_t*raw;uint32_t flags,frontier;gh_room_status re;gh_camera_status ce;
 if(!s||s->b.loop->game_state[0]!=3)return GH_SERVICES_ARGUMENT;
 work=s->b.work;raw=(uint8_t*)work;s->frontier=0;s->detail=0;
 if(!s->case3_stage){
 memset(raw+0x5d8,0,8);memset(raw+0x5e0,0,4);memset(raw+0x5ec,0,8);
 if((u32(raw+0x594)&0x800)&&(work->flags&0x200)){
  size_t bytes;const uint8_t*p=gh_game_cockpit_fields(s->cockpit,&bytes);if(!p||bytes<0x460)return fail(s,GH_SERVICES_ASSET,0x2a5ec0,GH_ID_BOUNDS);
  /* Boundedscalarbyteview only; no offsets into nativeIDBase/entry structs. */
  uint8_t*mutable_fields=(uint8_t*)(void*)p;mutable_fields[0x45e]=1;mutable_fields[0x45f]=0x31;
 }
 if(!(work->flags&0x4000000))s->main_word4=0;
 if(work->room!=0x20&&!(work->flags&0x80)){
  while(!(work->flags&0x400))if(gh_task_threads_sleep(s->b.threads,1)!=GH_THREAD_OK)return fail(s,GH_SERVICES_THREAD,0x2d5aa0,0);
  gh_object_status oe=gh_game_objects_register_startup(s->objects,s->b.assets);if(oe)return fail(s,GH_SERVICES_ASSET,0x1fece0,oe);
 }
 if(!(work->flags&0x10)){if(!s->b.start_loading_menu)return fail(s,GH_SERVICES_FRONTIER,0x2b65b0,0);
  uint32_t f=0;gh_game_services_status loading=s->b.start_loading_menu(s->b.loading_context,&f);
  if(loading)return fail(s,loading,f?f:0x2b65b0,0);}
 flags=work->flags;
 if(!s->case3_reloaded_player&&
    (s->b.save.bytes[0xae]!=s->b.save.bytes[0xaf]||(u32(raw+0x588)&0x800000)||(flags&0x40))){
  const uint8_t costume=gh_core_save_costume(s->b.save);
  gh_game_assets *assets=s->b.assets;
  gh_game_status loaded=gh_game_assets_reload_player_0030d8e0(assets,costume);
  if(loaded)return fail(s,loaded==GH_GAME_THREAD?GH_SERVICES_THREAD:GH_SERVICES_ASSET,
    assets->failed_entry?assets->failed_entry:0x30d8e0,loaded);
  /* 001FC100 records the selected costume only after both reads. */
  s->b.save.bytes[0xae]=s->b.save.bytes[0xaf]=costume;
  s->case3_reloaded_player=1;
 }
 w32(raw+0x5f8,0);w32(raw+0x5f4,0);
 /*2D9BC8 is actualjrra;nop. No emptyreplacementcallback. */
 re=gh_scenario_reset_002c32f0(&s->prepare,work,&s->scenario,s->scenario_threads,&s->room_state,&frontier);
 if(re)return fail(s,re==GH_ROOM_UNSUPPORTED?GH_SERVICES_FRONTIER:(re==GH_ROOM_THREAD?GH_SERVICES_THREAD:GH_SERVICES_ASSET),frontier?frontier:0x2c32f0,re);
 ce=gh_game_camera_reset_00138210(&s->camera,s->b.init->active_camera);if(ce)return fail(s,ce==GH_CAMERA_UNSUPPORTED?GH_SERVICES_FRONTIER:GH_SERVICES_ASSET,s->camera.failed_entry?s->camera.failed_entry:0x138210,ce);
 if(!s->b.sound_control)return fail(s,GH_SERVICES_FRONTIER,0x2ca8c8,0);
 gh_voice_status ve=gh_sound_control_prepare_scene_002ca8c8(s->b.sound_control);if(ve)return fail(s,GH_SERVICES_ASSET,0x2ca8c8,ve);
 /*2BF150->2BF170, same canonicalauxbytes originallyinitializedincase1. */
 static const unsigned aux_offsets[]={0,1,2,3,4,5,6,7,9,10,12,13};
 for(unsigned i=0;i<sizeof(aux_offsets)/sizeof(*aux_offsets);i++)s->b.stage->aux_state[aux_offsets[i]]=0;
 if(!s->b.movie_pool)return fail(s,GH_SERVICES_FRONTIER,0x2b4e48,0);
 int idle=0;do{
  gh_movie_status me=gh_movie_pool_is_idle_002b4e48(s->b.movie_pool,work,&idle);
  if(me){gh_movie_pool_view view;gh_movie_pool_inspect(s->b.movie_pool,&view);return fail(s,me==GH_MOVIE_UNSUPPORTED?GH_SERVICES_FRONTIER:GH_SERVICES_ASSET,view.frontier?view.frontier:0x2b4e48,me);}
  if(!idle&&gh_task_threads_sleep(s->b.threads,1)!=GH_THREAD_OK)return fail(s,GH_SERVICES_THREAD,0x2d5aa0,0);
 }while(!idle);
 /* Actual2FAA20 and3003F0 onlyclearthesecontiguousCPUbuffers/tables.
  * Theyarenotdecoded/executedasGScommandsoraninstructionruntime. */
 memset(s->scene_buffer_a,0,sizeof(s->scene_buffer_a));memset(s->scene_buffer_b,0,sizeof(s->scene_buffer_b));
 for(unsigned i=0;i<32;i++){s->main_records[i].kind=0xffff;s->main_records[i].handle=-1;s->main_records[i].field=0;} /*2A6C78*/
 gh_game_services_status lifetime=finalize_room_lifetime(s);if(lifetime)return lifetime;
 gh_room_pack_status pe=gh_game_room_pack_load_0031cc48(&s->room_pack,(uint16_t)work->room);
 if(pe)return fail(s,GH_SERVICES_ASSET,s->room_pack.failed_entry?s->room_pack.failed_entry:0x31cc48,s->room_pack.detail);
 gh_bundle bundle;gh_bundle_error be=gh_game_room_pack_bundle(&s->room_pack,&bundle);if(be)return fail(s,GH_SERVICES_ASSET,0x31ccf0,be);
 gh_room_pool_status rp=gh_game_room_pools_initialize_002a5d80(&s->pools,&bundle,s->b.assets->heap);
 if(rp)return fail(s,GH_SERVICES_ASSET,s->pools.failed_entry?s->pools.failed_entry:0x2a5d80,rp);
 s->case3_stage=1;
 }
 /* Cursor survives a returned frontier. Earlier file waits and destructive
  * resets must never be replayed when a missing later callee is connected. */
 if(s->case3_stage==1){
  if(s->room_state.record&&s->room_state.record->flags==1){
   gh_room_module_status me=gh_room_module_load_002be708(&s->module,(uint16_t)work->room);
   if(me)return fail(s,me==GH_ROOM_MODULE_UNSUPPORTED?GH_SERVICES_FRONTIER:GH_SERVICES_ASSET,s->module.frontier?s->module.frontier:0x2be708,me);
   if(work->room==0x104)
    s->module.hook4=(gh_scene_hook){.kind=GH_SCENE_HOOK_NATIVE,.call=room_r104_preinit,.context=s};
   if(work->room==0x100){s->module.hook4=(gh_scene_hook){.kind=GH_SCENE_HOOK_NATIVE,.call=room_r100_preinit,.context=s};
    s->module.hook12=(gh_scene_hook){.kind=GH_SCENE_HOOK_NATIVE,.call=room_r100_move,.context=s};}
  }
  s->case3_stage=2;
 }
 if(s->case3_stage==2){
  if(!s->b.em_set_first_byte)return fail(s,GH_SERVICES_FRONTIER,0x2c33d8,0);
  gh_scene_scenario_view sv={s->scenario_scalar,sizeof(s->scenario_scalar),&s->scenario};
  gh_scene_hook hook=s->module.bound?s->module.hook4:(gh_scene_hook){.kind=GH_SCENE_HOOK_ABSENT};
  gh_scene_status se=gh_scene_before_objects_002c33d8(sv,work,s->b.em_set_first_byte,hook,&frontier);
  if(se)return fail(s,GH_SERVICES_FRONTIER,frontier?frontier:0x2c33d8,se);
  s->case3_stage=3;
 }
 if(s->case3_stage==3){
  gh_sound_scene_status se;gh_bundle bundle;
  if(!s->b.audio_registry||!s->b.sound_scene)return fail(s,GH_SERVICES_FRONTIER,0x2cabe8,0);
  if(!s->b.sound_scene->startup_bound||s->b.sound_scene->control!=s->b.sound_control||s->b.sound_scene->registry!=s->b.audio_registry)return fail(s,GH_SERVICES_ASSET,0x2cabe8,GH_SOUND_SCENE_ARGUMENT);
  if(gh_game_room_pack_bundle(&s->room_pack,&bundle))return fail(s,GH_SERVICES_ASSET,0x31ccf0,0);
  s->sound_load_started=1;se=gh_sound_scene_initialize_002cabe8(s->b.sound_scene,&bundle,work,s->b.assets->dvd,s->b.assets->heap,s->b.threads);
  if(se)return fail(s,se==GH_SOUND_SCENE_UNSUPPORTED?GH_SERVICES_FRONTIER:GH_SERVICES_ASSET,s->b.sound_scene->frontier?s->b.sound_scene->frontier:0x2cabe8,se);
  s->case3_stage=4;
 }
 if(s->case3_stage==4){
  gh_collision_status pe=gh_collision_pairs_initialize_002b8b50(&s->collision_pairs);
  if(pe)return fail(s,GH_SERVICES_ASSET,0x2b8b50,pe);s->case3_stage=5;
 }
 if(s->case3_stage==5){
  gh_bundle bundle;if(gh_game_room_pack_bundle(&s->room_pack,&bundle))return fail(s,GH_SERVICES_ASSET,0x31ccf0,0);
  gh_effect_status ee=gh_game_effects_initialize_00305a68(&s->effects,s->scene_buffer_a,sizeof(s->scene_buffer_a),s->scene_buffer_b,sizeof(s->scene_buffer_b),s->b.assets->core.pointer,s->b.assets->core_size,&bundle);
  if(ee)return fail(s,ee==GH_EFFECT_UNSUPPORTED?GH_SERVICES_FRONTIER:GH_SERVICES_ASSET,s->effects.frontier?s->effects.frontier:0x305a68,ee);
  s->case3_stage=6;
 }
 if(s->case3_stage==6){
  gh_bundle bundle;if(gh_game_room_pack_bundle(&s->room_pack,&bundle))return fail(s,GH_SERVICES_ASSET,0x31ccf0,0);
  gh_filter_status fe=gh_scene_filters_initialize_002993a0(&s->filters,&bundle);
  if(fe)return fail(s,fe==GH_FILTER_UNSUPPORTED?GH_SERVICES_FRONTIER:GH_SERVICES_ASSET,s->filters.frontier?s->filters.frontier:0x2993a0,fe);
  s->case3_stage=7;
 }
 if(s->case3_stage==7){
  gh_bundle bundle;if(gh_game_room_pack_bundle(&s->room_pack,&bundle))return fail(s,GH_SERVICES_ASSET,0x31ccf0,0);
  gh_light_status le=gh_game_world_light_initialize_with_heap_002d6f40(&s->world_light,&bundle,s->b.assets->heap);
  if(le)return fail(s,le==GH_LIGHT_UNSUPPORTED?GH_SERVICES_FRONTIER:GH_SERVICES_ASSET,s->world_light.frontier?s->world_light.frontier:0x2d6f40,le);s->case3_stage=8;
 }
 if(s->case3_stage==8){gh_game_model_packets_reset_001509a0(&s->model_packets);s->case3_stage=9;}
 if(s->case3_stage==9){
  gh_id_error ie=gh_game_cockpit_room_reset_00140de0(s->cockpit,s->b.save,work,s->b.init->heat);
  if(ie)return fail(s,ie==GH_ID_UNSUPPORTED?GH_SERVICES_FRONTIER:GH_SERVICES_ASSET,gh_game_cockpit_frontier(s->cockpit)?gh_game_cockpit_frontier(s->cockpit):0x140de0,ie);s->case3_stage=10;
 }
 if(s->case3_stage==10){gh_init_status ie=gh_game_object_pool_reset_001fdea8(s->b.init);if(ie)return fail(s,GH_SERVICES_ASSET,0x1fdf88,ie);s->case3_stage=11;}
 if(s->case3_stage==11){
  if(!gh_collision_solid_units_empty(&s->collision_solids))return fail(s,GH_SERVICES_FRONTIER,0x1347e8,0);
  gh_game_collision_solids_reset_00134690(&s->collision_solids);s->case3_stage=12;
 }
 if(s->case3_stage>=12&&s->case3_stage<=15){
  gh_bundle bundle;if(gh_game_room_pack_bundle(&s->room_pack,&bundle))return fail(s,GH_SERVICES_ASSET,0x31ccf0,0);gh_scene_table_status te;
  if(s->case3_stage==12){te=gh_scene_environment_002a71b0(&s->scene_tables,work,&bundle);if(te)return fail(s,te==GH_TABLE_UNSUPPORTED?GH_SERVICES_FRONTIER:GH_SERVICES_ASSET,s->scene_tables.frontier,te);s->case3_stage=13;}
  if(s->case3_stage==13){
   /* The previous room retired its mesh companion. The original factory
    * serial is still the same global byte for every new room. */
   s->meshes.factory_serial=s->b.factory_serial;
   if(s->b.stage->textures&&s->b.boot_elf&&s->b.boot_elf_bytes){gh_scene_mesh_construction_bindings mb={s->b.stage->textures,&s->world_light,s->b.boot_elf,s->b.boot_elf_bytes};
    gh_scene_mesh_status bound=gh_scene_meshes_bind_construction(&s->meshes,&mb);if(bound)return fail(s,GH_SERVICES_ASSET,0x31d1b0,bound);}
   gh_scene_mesh_status me=gh_scene_meshes_construct_0031d1b0(&s->meshes,&s->scene_tables,work,&bundle,&s->pools,s->b.assets->heap);
   if(me)return fail(s,me==GH_MESH_UNSUPPORTED?GH_SERVICES_FRONTIER:me==GH_MESH_MEMORY?GH_SERVICES_MEMORY:GH_SERVICES_ASSET,s->meshes.frontier?s->meshes.frontier:0x31d1b0,me);s->case3_stage=14;}
  if(s->case3_stage==14){te=gh_scene_actors_0012e670(&s->scene_tables,&bundle);if(te)return fail(s,te==GH_TABLE_UNSUPPORTED?GH_SERVICES_FRONTIER:GH_SERVICES_ASSET,s->scene_tables.frontier,te);s->case3_stage=15;}
  if(s->case3_stage==15){te=gh_scene_actor_scroll_links_native_00307a60(&s->scene_tables,&s->pools,&s->meshes);if(te)return fail(s,te==GH_TABLE_UNSUPPORTED?GH_SERVICES_FRONTIER:GH_SERVICES_ASSET,s->scene_tables.frontier,te);s->case3_stage=16;}
 }
 if(s->case3_stage==16){
  if(work->room==0x20){
   const gh_title_memory_profile memory={GH_TITLE_MEMORY_ZERO_REFERENCE,NULL,0}; /*explicitRootapprovedhostinitial-memoryadaptation*/
   gh_title_player_status pe=gh_game_title_player_create_reference_0031c4e0(&s->title_player,s->b.assets->heap,work,&memory);
   if(pe)return fail(s,pe==GH_PLAYER_UNSUPPORTED?GH_SERVICES_FRONTIER:GH_SERVICES_ASSET,s->title_player.frontier?s->title_player.frontier:0x31c4e0,pe);
  }else{
   while(!(work->flags&0x10000))if(gh_task_threads_sleep(s->b.threads,1)!=GH_THREAD_OK)return fail(s,GH_SERVICES_THREAD,0x2d5aa0,0);
   if(s->title_player.allocation.pointer&&s->title_player.allocation_domain==GH_PLAYER_ALLOCATION_DEFAULT_HEAP){
    if(u32(raw+0x5f4))return fail(s,GH_SERVICES_BUSY,0x31c4e0,0);
    /* Main3 already overwrote the old R020 global with NULL. Explicit native
     * unreachable-owner cleanup, not a recovered original player destructor. */
    gh_title_player_status pe=gh_game_title_player_release(&s->title_player);if(pe)return fail(s,GH_SERVICES_ASSET,0x31c4e0,pe);
   }
   gh_player_factory_bindings pb={&s->title_player,&s->pools,s->objects,s->room_objects,work,s->b.assets->dvd->disc,s->b.save,s->b.factory_serial};int original_nonnull=0;
   gh_title_player_status pe=gh_game_player_factory_create_0030fa98(&s->player_factory,&pb,&original_nonnull);
   if(pe)return fail(s,pe==GH_PLAYER_UNSUPPORTED?GH_SERVICES_FRONTIER:GH_SERVICES_ASSET,s->player_factory.frontier?s->player_factory.frontier:0x30fa98,pe);
   if(!original_nonnull)return fail(s,GH_SERVICES_FRONTIER,0x31c4e0,0);
    /* Actual107E40 constructor explicitly writes6A8=NULL. Retain that typed
     * slot provenance; future weapon writers must update the same companion. */
    if(!s->player_weapon.known)s->player_weapon=(gh_event_weapon_slot){.known=1,.current=NULL};
   /* Canonical typed Work.player is the SAME factory wrapper, never a
    * truncated guest pointer. Publication does not complete virtualInit44. */
   gh_player_init_bindings ib={.factory=&s->player_factory,.work=work,.current_player=&s->title_player,
    .model_initialize_00149738=player_model_initialize,.model_context=s};
   gh_player_init_status ie=gh_game_player_initialize_00107f58(&s->player_init,&ib);
   if(ie==GH_PLAYER_INIT_UNSUPPORTED&&s->player_init.frontier==0x30fa98&&s->player_init.stage==4&&s->player_init.model_call_complete){
    gh_player_init_tail_bindings tb={.prefix=&s->player_init,.parent_model=&s->player_model,.child=&s->player_child,
     .attachment=&s->player_attachment,.solids=&s->collision_solids,.default_heap=s->b.assets->heap,.save=s->b.save,.resources=s->object_resources,
     .initialize_child=player_child_initialize,.child_context=s,.init=s->b.init,.pairs=&s->collision_pairs,
     .math_globals=s->b.math_globals,.shape_policy=GH_STATIC_BINARY32_REFERENCE,.node_names=s->b.player_node_names,
     .initialize_motion=player_motion_initialize,.advance_motion=player_motion_advance,
     .solve_motion=player_motion_solve,.motion_context=s,.rate=s->b.rate,
     .restore_paper=restore_player_paper,.paper_context=s};
    ie=gh_game_player_initialize_tail_00107f58(&s->player_tail,&tb);
    if(ie)return fail(s,ie==GH_PLAYER_INIT_UNSUPPORTED?GH_SERVICES_FRONTIER:GH_SERVICES_ASSET,s->player_tail.frontier?s->player_tail.frontier:0x107f58,ie);
   }
   if(ie)return fail(s,ie==GH_PLAYER_INIT_UNSUPPORTED?GH_SERVICES_FRONTIER:GH_SERVICES_ASSET,s->player_init.frontier?s->player_init.frontier:0x107f58,ie);
   gh_player_spawn_bindings sb={.player=&s->title_player,.model=&s->player_model,.actors=&s->scene_tables,
    .work=work,.init=s->b.init,.camera=&s->camera,.follow=s->b.camera_follow,
    .elf=s->b.boot_elf,.elf_bytes=s->b.boot_elf_bytes};
   gh_player_spawn_status se=gh_game_player_spawn_0031c5c0(&s->player_spawn,&sb);
   if(se)return fail(s,se==GH_PLAYER_SPAWN_UNSUPPORTED?GH_SERVICES_FRONTIER:GH_SERVICES_ASSET,
    s->player_spawn.frontier?s->player_spawn.frontier:0x31c5c0,se);
  }s->case3_stage=17;
 }
 gh_bundle room;if(gh_game_room_pack_bundle(&s->room_pack,&room))return fail(s,GH_SERVICES_ASSET,0x31ccf0,0);
 if(s->case3_stage==17){
  work->flags598&=~0x40u;w32(raw+0x5e4,u32(raw+0x5e4)&~0x8000u);
  gh_checkpoint_bindings cb={work,s->b.save,&s->prepare,s->b.init,&s->title_player.view,s->b.stage->work_area,sizeof(s->b.stage->work_area)};
  gh_checkpoint_status cp=gh_game_checkpoint_002bf8f8(&s->checkpoint,&cb);
  if(cp!=GH_CHECKPOINT_OK&&cp!=GH_CHECKPOINT_SKIPPED)return fail(s,cp==GH_CHECKPOINT_UNSUPPORTED?GH_SERVICES_FRONTIER:GH_SERVICES_ASSET,s->checkpoint.frontier?s->checkpoint.frontier:0x2bf8f8,cp);s->case3_stage=18;
 }
 if(s->case3_stage==18){
  raw[0x5d4]=20;raw[0x5d5]=0; /*actualSH, preserve5D6/7*/
  const uint8_t*core=s->b.assets->core.pointer;size_t size=s->b.assets->core_size;
  if(!core||size<20)return fail(s,GH_SERVICES_ASSET,0x150c10,GH_STAGE_BOUNDS);uint32_t count=u32(core),off=u32(core+16);size_t end=size;
  if(count<4||count>(size-4)/4||off<4+(size_t)count*4||off>=size)return fail(s,GH_SERVICES_ASSET,0x150c10,GH_STAGE_BOUNDS);
  for(unsigned i=1;i<=count;i++){uint32_t next=u32(core+i*4);if(next>off&&next<end)end=next;}
  gh_stage_status ts=gh_stage_texture_load(s->b.stage->textures,core+off,end-off,&s->room_texture_channel);
  if(ts)return fail(s,ts==GH_STAGE_UNSUPPORTED?GH_SERVICES_FRONTIER:GH_SERVICES_ASSET,0x150c10,ts);s->room_texture_ready=1;s->case3_stage=19;
 }
 if(s->case3_stage==19){gh_child_status ce0=gh_game_children_room_reset_0015fee0(*s->b.children);if(ce0)return fail(s,GH_SERVICES_ASSET,0x15fee0,ce0);s->case3_stage=20;}
 /*299860 isactualjrra/nop, notanemptyrendererreplacement. */
 if(s->case3_stage==20){gh_scene_ui_status ue=gh_scene_font_bind_002a4c38(s->b.font,&room,work->language);if(ue)return fail(s,ue==GH_SCENE_UI_UNSUPPORTED?GH_SERVICES_FRONTIER:GH_SERVICES_ASSET,0x2a4c38,ue);s->case3_stage=21;}
 if(s->case3_stage==21){gh_scene_ui_status ue=gh_scene_message_initialize_002aea18(&s->scene_ui,s->b.menu,&room,work->language);if(ue)return fail(s,GH_SERVICES_ASSET,s->scene_ui.frontier?s->scene_ui.frontier:0x2aea18,ue);s->case3_stage=22;}
 if(s->case3_stage==22){gh_scene_at_status ae=gh_scene_at_initialize_units_002c0f90(&s->scene_at,&room,s->b.assets->heap,(uint16_t)work->room,1);if(ae)return fail(s,ae==GH_AT_UNSUPPORTED?GH_SERVICES_FRONTIER:GH_SERVICES_ASSET,s->scene_at.frontier?s->scene_at.frontier:0x2c0f90,ae);s->case3_stage=23;}
 if(s->case3_stage==23){gh_game_action_buttons_reset_001f72c0(&s->action_buttons);s->case3_stage=24;}
 if(s->case3_stage==24){gh_door_status de=gh_game_door_reset_001ffe08(&s->door);if(de)return fail(s,GH_SERVICES_ASSET,0x1ffe08,de);s->case3_stage=25;}
 if(s->case3_stage==25){if(work->room!=0x20){gh_init_status ie=gh_game_enemy_reset_00290230(s->b.init);if(ie)return fail(s,GH_SERVICES_ASSET,0x290230,ie);}s->case3_stage=26;}
 if(s->case3_stage==26){gh_saved_item_spawn pending;gh_saved_item_spawn_status se=gh_core_save_spawn_items_001fb330(s->b.save,(uint16_t)work->room,&pending,&frontier);if(se)return fail(s,se==GH_SPAWN_UNSUPPORTED?GH_SERVICES_FRONTIER:GH_SERVICES_ASSET,frontier?frontier:0x1fb330,se);s->case3_stage=27;}
 if(s->case3_stage==27){
  if(!(u32(raw+0x590)&0x10000000)){
   gh_scene_placement_status pe=gh_em_scene_room_init_00294b38(&s->em_placements,work,s->scenario_scalar[0x14]);if(pe)return fail(s,GH_SERVICES_ASSET,s->em_placements.frontier?s->em_placements.frontier:0x294b38,pe);
   if((u32(s->b.save.bytes+0x14)&0x4000000)&&((uint16_t)work->room==0x4e||(uint16_t)work->room==0x4f))return fail(s,GH_SERVICES_FRONTIER,0x294c68,GH_PLACE_UNSUPPORTED);
   gh_bundle_entry ems={0};gh_bundle_error be=gh_bundle_find(&room,"EMS",0,&ems);if(be&&be!=GH_BUNDLE_NOT_FOUND)return fail(s,GH_SERVICES_ASSET,0x31ccf0,be);int result;
    pe=gh_em_scene_bind_native_00294c68(&s->em_placements,&s->room_state,&ems,work,s->scenario_scalar[0x14],GH_EM_NUMERIC_PCSX2_CHOPZERO_REFERENCE,&result);
    if(pe==GH_PLACE_UNSUPPORTED&&s->em_placements.frontier==0x294ca0){
     int detail;gh_game_services_status status=gh_game_services_enemy_set_all_00294ca0(s,&frontier,&detail);
     if(status)return fail(s,status,frontier,detail);
     pe=GH_PLACE_OK;
    }
    if(pe)return fail(s,pe==GH_PLACE_UNSUPPORTED?GH_SERVICES_FRONTIER:GH_SERVICES_ASSET,s->em_placements.frontier?s->em_placements.frontier:0x294c68,pe);
  }s->case3_stage=28;
 }
 if(s->case3_stage==28){
  if(!(u32(raw+0x590)&0x8000000)){
   gh_bundle_entry oms={0};gh_bundle_error be=gh_bundle_find(&room,"OMS",0,&oms);if(be&&be!=GH_BUNDLE_NOT_FOUND)return fail(s,GH_SERVICES_ASSET,0x31ccf0,be);
   int result;gh_scene_placement_status pe=GH_PLACE_OK;
   if(!s->om_placements.native_bound)pe=gh_om_scene_bind_001c3cb8(&s->om_placements,&oms,&result);
   if(pe==GH_PLACE_UNSUPPORTED||s->om_placements.native_bound){
    gh_game_services_status bind=bind_scene_om_factory(s);
    if(bind)return bind;
    pe=gh_om_scene_bind_native_with_collision_001c3cb8(&s->om_placements,&oms,&s->om_factory,
       &s->scene_tables,&s->r100.collision,&result);
   }
   if(pe)return fail(s,pe==GH_PLACE_UNSUPPORTED?GH_SERVICES_FRONTIER:GH_SERVICES_ASSET,s->om_placements.frontier?s->om_placements.frontier:0x1c3cb8,pe);
  }s->case3_stage=29;
 }
 if(s->case3_stage==29){gh_bundle_entry acd={0};gh_bundle_error be=gh_bundle_find(&room,"ACD",0,&acd);if(be&&be!=GH_BUNDLE_NOT_FOUND)return fail(s,GH_SERVICES_ASSET,0x31ccf0,be);int result;gh_scene_placement_status pe=gh_acd_scene_bind_00137ae8(&s->acd,&acd,&result);if(pe)return fail(s,pe==GH_PLACE_UNSUPPORTED?GH_SERVICES_FRONTIER:GH_SERVICES_ASSET,s->acd.frontier?s->acd.frontier:0x137ae8,pe);s->case3_stage=30;}
 if(s->case3_stage==30){
  if(!s->module.bound)return fail(s,GH_SERVICES_FRONTIER,0x2c3440,GH_SCENE_UNSUPPORTED);
  gh_scene_room_hook init,move;
  switch(work->room){
   case 0x20:init=room_init_native;move=room_move_native;break;
   case 0x21:init=room_results_init;move=room_results_move;break;
   case 0x100:init=room_r100_init;move=room_r100_move;break;
   case 2:init=room_reserved_event_init;move=room_r002_move;break;
   case 3:init=room_reserved_event_init;move=room_r003_move;break;
   case 0x105:init=room_r105_init;move=room_r105_move;break;
   case 0x101:init=room_r101_init;move=room_r101_move;break;
   case 0x102:init=room_r102_init;move=room_r102_move;break;
   case 0x103:init=room_r103_init;move=room_r103_move;break;
   case 0x104:init=room_r104_init;move=room_r104_move;break;
   case 0x200:init=room_r200_init;move=room_r200_move;break;
   default:return fail(s,GH_SERVICES_FRONTIER,0x2c3440,GH_SCENE_UNSUPPORTED);
  }
  s->module.hook8=(gh_scene_hook){GH_SCENE_HOOK_NATIVE,init,s};
  s->module.hook12=(gh_scene_hook){GH_SCENE_HOOK_NATIVE,move,s};
  s->scenario_exit=(gh_scene_hook){.kind=GH_SCENE_HOOK_ABSENT};s->scenario_callback104=(gh_scene_hook){.kind=GH_SCENE_HOOK_ABSENT};
  gh_scene_scenario_view sv={s->scenario_scalar,sizeof(s->scenario_scalar),&s->scenario};gh_scene_status se=gh_scene_initialize_room_002c3440(sv,work,s->module.hook8,&frontier);
  if(se)return fail(s,se==GH_SCENE_UNSUPPORTED?GH_SERVICES_FRONTIER:GH_SERVICES_ASSET,frontier?frontier:0x2c3440,se);s->case3_stage=31;
 }
 if(s->case3_stage==31){
  gh_scene_nearcheck_reset_0031be78(&s->step.nearcheck);
  if(!(work->flags598&0x80000)){gh_bundle_entry pfd={0};gh_bundle_error be=gh_bundle_find(&room,"PFD",0,&pfd);if(be&&be!=GH_BUNDLE_NOT_FOUND)return fail(s,GH_SERVICES_ASSET,0x31ccf0,be);gh_scene_step_status se=gh_scene_nearcheck_bind_0031c1d0(&s->step.nearcheck,pfd.bytes?&pfd:NULL);if(se)return fail(s,se==GH_SCENE_STEP_UNSUPPORTED?GH_SERVICES_FRONTIER:GH_SERVICES_ASSET,0x31c1d0,se);}s->case3_stage=32;
 }
 if(s->case3_stage==32){*s->b.rate=.5f;raw[0x569]=2;s->b.loop->game_state[0]=4;
  if(!(work->flags&0x110))gh_scene_system_menu_enable_002b65f0(&s->system_menu_enabled);work->flags&=~0x110u;s->case3_stage=33;
 }
 if(s->case3_stage==33){
  /* Static light records can select now. Any real player attachment still
   * requires the actual initialized model-parts owner, never invented parts0. */
  const void *parts[255];gh_light_player_parts player={0};
  if(s->player_model.parts_ready){
   for(unsigned i=0;i<s->player_model.part_count;i++)parts[i]=s->player_model.parts[i].bytes;
   player=(gh_light_player_parts){.owner=s->title_player.view.bytes,.parts=parts,.count=s->player_model.part_count};
  }
  gh_light_status le=gh_game_world_light_select_002d8408(&s->world_light,0,player.owner?&player:NULL);if(le)return fail(s,le==GH_LIGHT_UNSUPPORTED?GH_SERVICES_FRONTIER:GH_SERVICES_ASSET,s->world_light.frontier?s->world_light.frontier:0x2d8408,le);s->case3_stage=34;
 }
 if(s->case3_stage==34){if(!s->b.bgm_requests)return fail(s,GH_SERVICES_FRONTIER,0x2d0850,0);gh_bgm_status be=GH_BGM_OK;
  if(s->b.event_reserve)be=gh_bgm_requests_bind_event_reserve(s->b.bgm_requests,s->b.event_reserve);
  if(!be)be=gh_bgm_requests_scene_begin_with_room_002d0850(s->b.bgm_requests,work,*s->b.children,s->b.init,&s->room_state);if(be)return fail(s,be==GH_BGM_UNSUPPORTED?GH_SERVICES_FRONTIER:GH_SERVICES_ASSET,gh_bgm_requests_frontier(s->b.bgm_requests),be);s->case3_stage=35;}
 if(s->case3_stage==35){gh_game_scene_step_case3_cooldown(&s->step);
  if(work->room!=0x20){
   /* Actual2DB6B0 only loads747A94 (Work5F4), whose native publication is
    * SAMEfactory player. Original2A65D0 then writes float30 at player54C. */
   const gh_title_player_view *player=NULL;
   if(gh_game_player_factory_scalar_view(&s->player_factory,s->player_factory.handle,&player)||
      player!=&s->title_player.view||player->size<0x550)return fail(s,GH_SERVICES_FRONTIER,0x2db6b0,0);
   w32(player->bytes+0x54c,0x41f00000);
  }
  if((u32(raw+0x594)&0x800)&&(work->flags&0x200)){size_t n;uint8_t*p=(uint8_t*)(void*)gh_game_cockpit_fields(s->cockpit,&n);if(n<0x460)return fail(s,GH_SERVICES_ASSET,0x2a65ec,0);p[0x45e]=p[0x45f]=255;}
  work->flags&=0xfafffdbd;w32(raw+0x5e0,u32(raw+0x5e0)&0xfdffffff);work->flags59c&=0xff7fffff;s->case3_stage=36;
 }
 s->frontier=0;s->detail=0;s->status=GH_SERVICES_OK;return GH_SERVICES_OK;
}
void gh_game_services_inspect(const gh_game_services*s,gh_game_services_snapshot*out){
 if(!out)return;memset(out,0,sizeof(*out));if(!s)return;
 out->frontier=s->frontier;out->detail=s->detail;out->state=s->b.loop->game_state[0];out->cockpit_ready=gh_game_cockpit_ready(s->cockpit);
 out->id_entries=s->b.id->pool_used;out->id_runs=s->b.id->job_count;out->room_loaded=s->room_pack.loaded;out->room_pools_ready=s->pools.initialized;
 out->frontier_is_r020=s->frontier_module_code==1;
 for(unsigned i=0;i<16;i++)out->scenario_active+=s->scenario_records[i].running!=0;
}
uint32_t gh_game_services_frontier(const gh_game_services*s){return s?s->frontier:0;}
int gh_game_services_detail(const gh_game_services*s){return s?s->detail:0;}
const char *gh_game_services_frontier_module(const gh_game_services*s)
{
 if(!s)return NULL;
 switch(s->frontier_module_code){
 case 1:return "r020.rel";
 case 2:return "r100.rel";
 case 3:return "r003.rel";
 case 4:return "r105.rel";
 case 5:return "r101.rel";
 case 6:return "r002.rel";
 case 7:return "r102.rel";
 case 8:return "r103.rel";
 case 9:return "r104.rel";
 case 10:return "r200.rel";
 default:return NULL;
 }
}
gh_game_services_status gh_game_services_destroy(gh_game_services*s){
 if(s&&release_r101_room_init(s))return s->status;
 if(!s)return GH_SERVICES_ARGUMENT;
 if((s->owns_children&&*s->b.children!=s->created_children)||(s->owns_textures&&s->b.stage->textures!=s->created_textures))return fail(s,GH_SERVICES_BUSY,0x2a5bb8,0);
 if(s->results_idd.pointer||s->results_pair_bound){
  uint32_t pc=0;
  if(room_results_release(s,&pc))return fail(s,GH_SERVICES_BUSY,pc?pc:0x318,0);
 }
 if(s->title){
  gh_r020_title_view v;if(gh_r020_title_inspect(s->title,&v))return fail(s,GH_SERVICES_ASSET,0x248,GH_R020_ARGUMENT);
  uint32_t extra=v.id_base->owns_run?v.id_base->entry_count:0,jobs=v.id_base->owns_run?1:0;
  if(s->b.id->pool_used!=s->cockpit_pool_end+extra||s->b.id->job_count!=s->cockpit_jobs_end+jobs)return fail(s,GH_SERVICES_BUSY,0x2aa078,GH_ID_BUSY);
  if(s->b.title_callbacks.begin_frame){uint32_t f=0;gh_r020_status ce=s->b.title_callbacks.begin_frame(s->b.title_callbacks.context,v.id_base,&f);if(ce)return fail(s,GH_SERVICES_ASSET,f?f:0x2aac28,ce);}
  gh_r020_status te=gh_r020_title_destroy(s->title);if(te)return fail(s,GH_SERVICES_BUSY,0x248,te);s->title=NULL;
 }
 /* Native Scenario entries borrow room/model state; cancel before freeing it. */
 if(s->scenario_threads){for(unsigned i=0;i<16;i++){gh_thread_status e=gh_task_threads_cancel(s->scenario_threads,(int32_t)i);if(e)return fail(s,GH_SERVICES_THREAD,0x2d59f0,e);}
  gh_thread_status e=gh_task_threads_destroy(s->scenario_threads);if(e)return fail(s,GH_SERVICES_THREAD,0x2d56a8,e);s->scenario_threads=NULL;}
 if(release_r103_room_init(s)||release_r104_room_init(s)||release_r200_room_init(s))return s->status;
 if(s->r003.work.pointer){
  gh_r003_status status=gh_r003_event_release(&s->r003);
  if(status)return fail(s,GH_SERVICES_BUSY,0x2a94f0,status);
 }
 if(s->r105.work.pointer){
  const gh_scene_status status=gh_r105_room_release(&s->r105);
  if(status)return fail(s,GH_SERVICES_BUSY,0x2a94f0,status);
 }
 {gh_tutorial_status e=gh_cockpit_tutorial_release(&s->tutorial);if(e)return fail(s,GH_SERVICES_ASSET,s->tutorial.frontier?s->tutorial.frontier:0x147188,e);}
 if(s->cockpit){
  const gh_id_error ticket=gh_game_cockpit_abort_ticket(s->cockpit);
  if(ticket)return fail(s,GH_SERVICES_BUSY,gh_game_cockpit_frontier(s->cockpit),ticket);
  gh_id_error boss=gh_game_cockpit_abort_middle_boss(s->cockpit);
  if(!boss)boss=gh_game_cockpit_abort_boss(s->cockpit);
  if(boss)return fail(s,GH_SERVICES_BUSY,gh_game_cockpit_frontier(s->cockpit),boss);
 }
 if(s->cockpit_snapshot&&(s->b.id->pool_used!=s->cockpit_pool_end||s->b.id->job_count!=s->cockpit_jobs_end))return fail(s,GH_SERVICES_BUSY,0x2aa078,GH_ID_BUSY);
 /* Native shutdown owns this unfinished construction prefix; this is not the
  * original scene-exit destructor for initialized world actors. */
 gh_scene_nearcheck_release(&s->step.nearcheck);
  /* Native destruction detaches effect companions before borrowed actors and
   * models. It neither acknowledges original Kill nor clears live masks. */
  {gh_voice_status e=gh_actor_attached_sound_abort(&s->player_sound);if(e)return fail(s,GH_SERVICES_ASSET,s->player_sound.frontier?s->player_sound.frontier:0x2cba48,e);}
  {gh_voice_status e=gh_actor_attached_sound_abort(&s->enemy_sound);if(e)return fail(s,GH_SERVICES_ASSET,s->enemy_sound.frontier?s->enemy_sound.frontier:0x2cba48,e);}
  {gh_voice_status e=gh_actor_attached_sound_abort(&s->om_sound);if(e)return fail(s,GH_SERVICES_ASSET,s->om_sound.frontier?s->om_sound.frontier:0x2cba48,e);}
  if(s->player_prop.give_created){
   uint32_t f=0;
   if(gh_hitarea_shapes_abort_object(&s->player_prop.give,&f)||
      gh_hitarea_object_release_empty_001fe278(&s->player_prop.give,&f))
    return fail(s,GH_SERVICES_ASSET,f?f:0x1fe278,0);
   s->player_prop.give_created=0;
  }
  gh_effect_create_forget_on_owner_destroy(&s->effects);
  for(size_t i=0;i<s->esp14_limit;i++){
   gh_services_esp14_slot *slot=s->esp14_slots[i];
   if(!slot)continue;
   gh_effect_status status=shutdown_model_effect(s,slot);
   if(status)return fail(s,GH_SERVICES_ASSET,0x2f27e8,status);
   free(slot);
   s->esp14_slots[i]=NULL;
  }
  s->esp14_limit=0;
  while(s->esp14_retired){
   gh_services_esp14_slot *slot=s->esp14_retired;
   gh_effect_status status=shutdown_model_effect(s,slot);
   if(status)return fail(s,GH_SERVICES_ASSET,0x2b8908,status);
   s->esp14_retired=slot->retired_next;
   free(slot);
  }
  if(s->enemies){
   const gh_room_vector *registered=NULL;
   if(gh_room_registry_view(&s->pools,0x754c30,&registered))return fail(s,GH_SERVICES_ASSET,0x30f16c,0);
   for(uint32_t i=0;i<registered->count;i++){
    uint32_t handle=u32((const uint8_t*)registered->allocation.pointer+(size_t)i*4);
    gh_enemy_ems_live actor;
    gh_scene_placement_status lookup=gh_enemy_ems_live_at(s->enemies,handle,&actor);
    if(lookup)return fail(s,enemy_place_error(lookup),0x30f16c,lookup);
    if(actor.actor)gh_scene_dynamic_forget_sphere_actor(&s->dynamic_objects,&actor.actor->view);
   }
   gh_game_devil_camera_detach(&s->devil_camera,s->b.init);
   uint32_t f=0;gh_actor_motion_status e=gh_enemy_ems_release_native(&s->enemies,&f);
   if(e)return fail(s,GH_SERVICES_ASSET,f?f:0x290770,e);
  }
  free(s->enemy_renders);s->enemy_renders=NULL;s->devil_render=(gh_actor_render_call){0};
  if(gh_r100_init_release(&s->r100))return fail(s,GH_SERVICES_ASSET,0x208,0);
 if(gh_scene_at_units_release(&s->scene_at))return fail(s,GH_SERVICES_ASSET,0x2c1270,0);
 gh_scene_dynamic_cancel_update(&s->dynamic_objects);
 gh_scene_dynamic_forget_sphere_actor(&s->dynamic_objects,&s->title_player.view);
 gh_scene_dynamic_forget_sphere_actor(&s->dynamic_objects,&s->player_child.object.view);
 free(s->om_renders);s->om_renders=NULL;s->om_renders_capacity=0;
 s->player_render=(gh_actor_render_call){0};s->child_render=(gh_actor_render_call){0};
 free(s->om_frames);s->om_frames=NULL;s->om_frames_capacity=0;
 if(gh_om_factory_release(&s->om_factory))return fail(s,GH_SERVICES_ASSET,0x30f8b0,0);
 if(s->r102.work.pointer){
  const gh_scene_status status=gh_r102_room_release_prefix(&s->r102);
  if(status)return fail(s,GH_SERVICES_BUSY,0x2a94f0,status);
 }
 {uint32_t f=0;gh_solid_unit_status se=gh_collision_solid_units_shutdown(&s->collision_solids,&f);if(se)return fail(s,GH_SERVICES_ASSET,f?f:0x1347e8,se);}
 if(gh_scene_dynamic_release(&s->dynamic_objects))return fail(s,GH_SERVICES_ASSET,0x12ecf0,0);
 {uint32_t f=0;gh_hitarea_status he=gh_hitarea_shapes_abort(s->b.init,&f);
  if(he)return fail(s,GH_SERVICES_ASSET,f?f:0x1fd580,he);
  gh_init_status ie=gh_game_object_pool_release_001fdfa8(s->b.init,&f);
  if(ie)return fail(s,GH_SERVICES_ASSET,f?f:0x1fdfa8,ie);}
 gh_game_player_child_attachment_clear(&s->player_attachment);
 s->work5f8=NULL;
 if(gh_game_player_child_release_prefix(&s->player_child))return fail(s,GH_SERVICES_ASSET,0x100378,0);
 if(s->b.camera_envelope)(void)gh_game_camera_envelope_bind_model(s->b.camera_envelope,NULL);
 if(gh_actor_model_release(&s->player_model))return fail(s,GH_SERVICES_ASSET,0x149738,0);
 if(gh_game_player_factory_release_prefix(&s->player_factory))return fail(s,GH_SERVICES_ASSET,0x30f8b0,0);
 gh_scene_actor_scroll_links_release(&s->scene_tables);
 gh_scene_ray_triangle_state_release(&s->scene_tables);
 if(s->meshes.descriptors||s->meshes.first.allocation.pointer){gh_scene_mesh_status me=gh_scene_meshes_release_prefix(&s->meshes);
  if(me)return fail(s,GH_SERVICES_BUSY,0x30f8b0,me);}
 if(gh_game_title_player_release(&s->title_player))return fail(s,GH_SERVICES_ASSET,0x31c4e0,0);
 if(gh_game_world_light_release_presets(&s->world_light))return fail(s,GH_SERVICES_ASSET,0x2d73e0,0);
 if(gh_collision_pairs_release(&s->collision_pairs))return fail(s,GH_SERVICES_ASSET,0x2b8b50,0);
 if(s->sound_load_started){
  if(s->b.sound_scene->job&&gh_dvd_check(s->b.assets->dvd,s->b.sound_scene->job))gh_dvd_cancel(s->b.assets->dvd,s->b.sound_scene->job);
  if(gh_sound_scene_release_loading(s->b.sound_scene,s->b.assets->dvd,s->b.assets->heap))return fail(s,GH_SERVICES_ASSET,0x2cabe8,0);
  /* Endborrowedroomviews beforetheirpackallocation isfreed. Rootglobal
   * ratios/controller/registry remainalive; ready isonlyhostmetadata. */
  s->b.sound_scene->seh=(gh_bundle_entry){0};s->b.sound_scene->bmh=(gh_bundle_entry){0};s->b.sound_scene->ts=(gh_bundle_entry){0};s->b.sound_scene->ready=0;s->sound_load_started=0;
 }
 if(s->module.input.job&&gh_dvd_check(s->b.assets->dvd,s->module.input.job))gh_dvd_cancel(s->b.assets->dvd,s->module.input.job);
 if(gh_room_module_release(&s->module))return fail(s,GH_SERVICES_ASSET,0x2be708,0);
 if(s->pools.heap){gh_room_pool_status e=gh_game_room_pools_release(&s->pools);if(e)return fail(s,GH_SERVICES_FRONTIER,s->pools.failed_entry,e);}
 if(s->room_pack.job&&gh_dvd_check(s->room_pack.dvd,s->room_pack.job))gh_dvd_cancel(s->room_pack.dvd,s->room_pack.job);
 if(gh_game_room_pack_release(&s->room_pack)!=GH_PACK_OK)return fail(s,GH_SERVICES_ASSET,0x31cc48,0);
 if(s->owns_skills){gh_stage_skills_clear(&s->b.stage->skills);s->owns_skills=0;}
 if(s->cockpit){gh_id_error e=gh_game_cockpit_destroy(s->cockpit);s->cockpit=NULL;s->cockpit_snapshot=0;if(e)return fail(s,GH_SERVICES_BUSY,0x2aa078,e);}
 if(s->owns_children&&*s->b.children){gh_game_children_destroy(*s->b.children);*s->b.children=NULL;s->owns_children=0;}
 if(s->owns_textures){gh_stage_textures_destroy(s->b.stage->textures);s->b.stage->textures=NULL;s->owns_textures=0;}
 if(s->b.stage->context==s){s->b.stage->context=s->old_context;s->b.stage->init_children_15fd98=s->old_children;s->b.stage->init_cockpit_140c88=s->old_cockpit;}
 gh_game_objects_destroy(s->room_objects);gh_game_objects_destroy(s->objects);free(s);return GH_SERVICES_OK;
}

gh_game_event_status gh_game_services_bind_event_actor_roster(gh_game_services *s,
    gh_event_actor_roster *roster, gh_event_actor_factory *rows, size_t capacity)
{
    if (!s || !roster || (s->event_actor_roster && s->event_actor_roster != roster)) return GH_EVENT_ARGUMENT;
    if (s->event_actor_roster == roster && roster->rows)
        return roster->rows == rows && roster->capacity == capacity ? GH_EVENT_OK : GH_EVENT_ARGUMENT;
    const gh_event_actor_factory_bindings bindings = {
        .pools = &s->pools,
        .global_objects = s->objects,
        .room_objects = s->room_objects,
        .work = s->b.work,
        .factory_serial = s->b.factory_serial,
        .resources = s->object_resources,
        .packet_heap = s->b.assets->heap,
        .fog_rgb = s->world_light.scalar + 4
    };
    gh_game_event_status status = gh_event_actor_roster_bind(roster, &bindings, rows, capacity);
    if (!status) {
        s->event_actor_roster = roster;
        memset(s->event_actor_move, 0, sizeof(s->event_actor_move));
        memset(s->event_actor_render, 0, sizeof(s->event_actor_render));
        memset(s->event_accessory_cloth, 0, sizeof(s->event_accessory_cloth));
        memset(s->event_accessory_render, 0, sizeof(s->event_accessory_render));
    }
    return status;
}

gh_game_event_status gh_game_services_bind_event_actor_extra(gh_game_services *s,
    gh_event_actor_extra *extra, const gh_event_actor_create *creation)
{
    if (!s || !extra || !creation || !s->event_actor_roster || !s->event_actor_roster->rows ||
        (s->event_actor_extra && s->event_actor_extra != extra)) return GH_EVENT_ARGUMENT;
    if (s->event_actor_extra == extra && extra->bound)
        return extra->bindings.creation == creation ? GH_EVENT_OK : GH_EVENT_ARGUMENT;
    const gh_event_actor_extra_bindings bindings = {
        .factory = s->event_actor_roster->rows,
        .player = &s->title_player.view,
        .child = s->work5f8,
        .attachment = &s->player_attachment,
        .weapon_slot = &s->player_weapon,
        .save = s->b.save,
        .roster = s->event_actor_roster,
        .creation = creation,
        .math = s->b.math_globals,
        .boot_elf = s->b.boot_elf,
        .boot_elf_size = s->b.boot_elf_bytes
    };
    gh_game_event_status status = gh_event_actor_extra_bind(extra, &bindings);
    if (!status) s->event_actor_extra = extra;
    return status;
}

static gh_event_actor_factory *event_actor_for_handle(gh_game_services *s, uint32_t handle)
{
    if (!handle || !s->event_actor_roster) return NULL;
    gh_event_actor_roster *roster = s->event_actor_roster;
    for (size_t i = 0; i < roster->capacity; ++i)
        if (roster->rows[i].handle == handle) return &roster->rows[i];
    if (s->event_actor_extra) {
        gh_event_actor_extra *extra = s->event_actor_extra;
        if (extra->olivia_accessory.handle == handle) return &extra->olivia_accessory;
        for (size_t i = 0; i < sizeof(extra->ev04_accessory) / sizeof(*extra->ev04_accessory); ++i)
            if (extra->ev04_accessory[i].handle == handle) return &extra->ev04_accessory[i];
    }
    return NULL;
}

gh_game_services_status gh_game_services_actor_view(gh_game_services*s,uint32_t handle,gh_title_player_view*out){
 if(!s||!out)return GH_SERVICES_ARGUMENT;
 gh_services_esp14_slot *effect_actor=model_effect_actor(s,handle);
 if(effect_actor){*out=effect_actor->instance.child.object.view;return GH_SERVICES_OK;}
 gh_event_actor_factory*event_actor=event_actor_for_handle(s,handle);
 if(event_actor)return gh_event_actor_factory_view(event_actor,handle,out)==GH_EVENT_OK?GH_SERVICES_OK:GH_SERVICES_ASSET;
 gh_enemy_ems_live enemy;
 gh_scene_placement_status ee=gh_enemy_ems_live_at(s->enemies,handle,&enemy);
 if(ee)return enemy_place_error(ee);
 if(enemy.actor){*out=enemy.actor->view;return GH_SERVICES_OK;}
 const gh_title_player_view *player=NULL;
 if(gh_game_player_factory_scalar_view(&s->player_factory,handle,&player)==GH_PLAYER_OK){*out=*player;return GH_SERVICES_OK;}
 if(s->player_child.handle==handle&&s->player_child.complete){void*registered=NULL;
  if(gh_room_object_lookup(&s->pools,handle,GH_ROOM_OBJECT_PLAYER_CHILD,&s->player_child.object,&registered)||registered!=&s->player_child.object)return GH_SERVICES_ASSET;
  *out=s->player_child.object.view;return GH_SERVICES_OK;}
 void*scroll=NULL;if(!gh_room_object_lookup(&s->pools,handle,GH_ROOM_OBJECT_SCROLL_MESH,&s->meshes,&scroll)){
  for(size_t i=0;i<s->meshes.count;i++){const gh_scene_mesh_object*o;
   if(!gh_scene_meshes_object_at(&s->meshes,i,&o)&&o==scroll){
    if(!o->constructor_complete||!o->allocation.pointer||o->handle!=handle||o->local_matrix_offset>0x350||o->rotation_matrix_offset>0x350)return GH_SERVICES_ASSET;
    uint8_t*p=o->allocation.pointer;*out=(gh_title_player_view){.bytes=p,.size=0x360,.alias_d0=p+o->local_matrix_offset,.alias_f0=p+o->rotation_matrix_offset};return GH_SERVICES_OK;
   }
  }return GH_SERVICES_ASSET;
 }
 gh_scene_placement_status e=gh_om_factory_object_view(&s->om_factory,handle,out);
 return !e?GH_SERVICES_OK:e==GH_PLACE_UNSUPPORTED?GH_SERVICES_FRONTIER:GH_SERVICES_ASSET;
}
gh_game_services_status gh_game_services_mask_actor_nodes(gh_game_services*s,uint32_t handle,uint32_t and_mask,uint32_t or_mask,uint32_t*f){
 if(f)*f=0;if(!s||!f)return GH_SERVICES_ARGUMENT;
 void*scroll=NULL;if(!gh_room_object_lookup(&s->pools,handle,GH_ROOM_OBJECT_SCROLL_MESH,&s->meshes,&scroll)){
  for(size_t i=0;i<s->meshes.count;i++){const gh_scene_mesh_object*o;
   if(!gh_scene_meshes_object_at(&s->meshes,i,&o)&&o==scroll){
    if(!o->constructor_complete||!o->node){*f=0x14b6f8;return GH_SERVICES_ASSET;}
    uint8_t*p=(uint8_t*)o->node+0x380;w32(p,(u32(p)&and_mask)|or_mask);return GH_SERVICES_OK;
   }
  }*f=0x14b6f8;return GH_SERVICES_ASSET;
 }
 gh_actor_model*m=NULL;const gh_title_player_view*player=NULL;
 gh_services_esp14_slot *effect_actor=model_effect_actor(s,handle);
 gh_event_actor_factory*event_actor=event_actor_for_handle(s,handle);
 gh_enemy_ems_live enemy;
 gh_scene_placement_status ee=gh_enemy_ems_live_at(s->enemies,handle,&enemy);
 if(ee){*f=0x14b6f8;return enemy_place_error(ee);}
 if(enemy.actor)m=enemy.model;
 else if(effect_actor)m=&effect_actor->instance.child.model;
 else if(!gh_game_player_factory_scalar_view(&s->player_factory,handle,&player))m=&s->player_model;
 else if(handle==s->player_child.handle&&s->player_child.complete){gh_title_player_view v;gh_game_services_status e=gh_game_services_actor_view(s,handle,&v);if(e){*f=0x14b6f8;return e;}m=&s->player_child.model;}
 else if(event_actor){gh_title_player_view v;gh_game_services_status e=gh_game_services_actor_view(s,handle,&v);if(e){*f=0x14b6f8;return e;}m=&event_actor->model;}
 else if(gh_om_factory_model_at(&s->om_factory,handle,&m)){*f=0x14b6f8;return GH_SERVICES_FRONTIER;}
 if(!m||!m->ready||m->native_identity!=m||!m->root.bytes||m->root.size<0x2b2){*f=0x14b6f8;return GH_SERVICES_ASSET;}
 if(m->script.mesh_count>255||m->root.bytes[0x2b1]!=m->script.mesh_count){*f=0x14b6f8;return GH_SERVICES_FRONTIER;}
 unsigned count=(unsigned)m->script.mesh_count;
 for(unsigned i=0;i<count;i++){const uint8_t*node;size_t mesh;if(gh_actor_model_node_at(m,i,&node,&mesh)){*f=0x14b6f8;return GH_SERVICES_ASSET;}}
 for(unsigned i=0;i<count;i++)if(gh_actor_model_node_mask_flags(m,i,0x380,and_mask,or_mask)){*f=0x14b6f8;return GH_SERVICES_ASSET;}
 return GH_SERVICES_OK;
}
static gh_object_step_status om_canopy_actor(void *context,uint32_t handle,
 gh_title_player_view *actor,uint32_t *frontier)
{
 gh_game_services_status status=gh_game_services_actor_view(context,handle,actor);
 if(status&&frontier)*frontier=0x1a8bc0;
 return status?GH_OBJSTEP_CALLEE:GH_OBJSTEP_OK;
}
static gh_player_target_status player_target_view(void*p,uint32_t handle,gh_title_player_view*v,uint32_t*f){
 gh_game_services*s=p;gh_game_services_status e=gh_game_services_actor_view(s,handle,v);if(e&&f)*f=0x30f16c;
 return !e?GH_PLAYER_TARGET_OK:e==GH_SERVICES_FRONTIER?GH_PLAYER_TARGET_UNSUPPORTED:GH_PLAYER_TARGET_CALLEE;
}
gh_player_target_bindings gh_game_services_scene_targets(gh_game_services *s){
 if(!s)return (gh_player_target_bindings){0};
 return (gh_player_target_bindings){.pools=&s->pools,.actors=&s->scene_tables,.player=&s->title_player.view,
  .resolve=player_target_view,.context=s,.elf=s->b.boot_elf,.elf_bytes=s->b.boot_elf_bytes,
  .init=s->b.init,.actions=&s->action_buttons};
}
static gh_player_frame_status player_square_hit_area(void *context,const gh_title_player_view *player,
 gh_hitarea_object give[5],uint32_t *frontier)
{
 gh_game_services *s=context;
 if(player!=&s->title_player.view||give!=s->player_tail.give_objects)return GH_PLAYER_FRAME_ARGUMENT;
 gh_hitarea_status status=gh_player_attack_hitarea_0010a438(player,give,frontier);
 return !status?GH_PLAYER_FRAME_OK:status==GH_HITAREA_UNSUPPORTED?
        GH_PLAYER_FRAME_UNSUPPORTED:GH_PLAYER_FRAME_CALLEE;
}
static gh_player_frame_status player_square_camera_exit(void *context,
 const gh_player_frame_bindings *b,uint32_t *frontier)
{
 gh_game_services *s=context;
 if(b->player!=&s->title_player||b->init!=s->b.init)return GH_PLAYER_FRAME_ARGUMENT;
 gh_game_event_status status;
 if(!s->player_square_camera_pose_done){
  status=room_soft_end_camera_pose(s,frontier);
  if(status)return status==GH_EVENT_UNSUPPORTED?GH_PLAYER_FRAME_UNSUPPORTED:GH_PLAYER_FRAME_CALLEE;
  s->player_square_camera_pose_done=1;
 }
 status=room_soft_end_camera_switch(s,frontier);
 if(status)return status==GH_EVENT_UNSUPPORTED?GH_PLAYER_FRAME_UNSUPPORTED:GH_PLAYER_FRAME_CALLEE;
 s->player_square_camera_pose_done=0;
 return GH_PLAYER_FRAME_OK;
}
static gh_player_frame_status player_square_attack(void *context,
 const gh_player_frame_bindings *b,uint32_t *frontier)
{
 gh_game_services *s=context;
 if(!s||!b||!frontier||b!=&s->player_frame.b||b->player!=&s->title_player||
    b->model!=&s->player_model||b->idle_input_bindings!=&s->player_idle_bindings)
  return GH_PLAYER_FRAME_ARGUMENT;
 *frontier=0;
 if(!s->player_square.bindings){
  const gh_player_target_bindings *targets=&s->player_idle_bindings.targets;
  const uint32_t handle=u32(b->player->view.bytes+0x640);
  gh_player_target target={.handle=handle};
  if(handle){
   gh_player_target_status status=player_target_view(s,handle,&target.view,frontier);
   if(status)return status==GH_PLAYER_TARGET_UNSUPPORTED?GH_PLAYER_FRAME_UNSUPPORTED:GH_PLAYER_FRAME_CALLEE;
  }
  s->player_square_target=target;
  s->player_square_bindings=(gh_player_square_frame_bindings){.frame=b,.target_bindings=targets,
   .target_640=&s->player_square_target,.action_buttons=&s->action_buttons,
   .hit_area_0010a438=player_square_hit_area,
   .camera_exit_00124f60=player_square_camera_exit,.context=s};
 }
 gh_player_frame_status status=gh_game_player_square_frame_0010d3a8(&s->player_square,&s->player_square_bindings);
 *frontier=s->player_square.frontier;
 if(!status){
  status=gh_player_square_frame_reset(&s->player_square);
  s->player_square_target=(gh_player_target){0};
 }
 return status;
}
/* The authored 125F38 choice 14 ends the Roulette sound and enters
 * original PL00 00/23. Its switch branch needs no positional alignment. */
gh_scene_step_status gh_game_services_roulette_command_00125f38(gh_game_services *s,
 unsigned command,uint32_t *frontier)
{
 if(!s||!frontier)return GH_SCENE_STEP_ARGUMENT;
 *frontier=0x125f38;
 gh_title_player_view *player=&s->title_player.view;
 uint8_t *p=player->bytes;
 if(command!=14||(s->b.work->room!=0x103&&s->b.work->room!=0x104)||!p||player->size<0x1660||
    p[0x2f4]||p[0x2f5]!=0x2b||p[0x2f6]!=1||p[0x2f7]||!s->b.sound_control)
  return GH_SCENE_STEP_UNSUPPORTED;
 if(u32(p+0x698)||u32(p+0x6a0)){
  *frontier=u32(p+0x698)?0x12ac28:0x12b928;
  return GH_SCENE_STEP_UNSUPPORTED;
 }
 const uint32_t voice=u32(p+0x161c);
 if(voice){
  const gh_player_abort_status status=stop_player_abort_voice(s,voice,frontier);
  if(status)return status==GH_PLAYER_ABORT_UNSUPPORTED?GH_SCENE_STEP_UNSUPPORTED:GH_SCENE_STEP_CALLEE;
 }
 s->b.sound_control->flags_b0&=~0x400000u;
 s->b.sound_control->flags_ac&=~0x400000u;
 p[0x2f4]=0;p[0x2f5]=0x23;p[0x2f6]=0;p[0x2f7]=0;
 memset(p+0x161c,0,4);
 memset(p+0x165c,0,4);
 const uint32_t cooldown=0x41f00000u;
 memcpy(p+0x54c,&cooldown,4);
 if(u32(p+0x640))p[0x648]=0x5a;
 *frontier=0;
 return GH_SCENE_STEP_OK;
}

/* Original placed-item callback 2DB978 enters the ordinary 1C24E8 pickup state. */
gh_action_step_status gh_game_services_r101_placed_item_action(gh_game_services *s,
 void *argument,uint32_t *frontier)
{
 if(!s||!argument||!frontier||!s->b.init||!s->b.work||s->b.work->room!=0x101)
  return GH_ACTION_STEP_ARGUMENT;
 *frontier=0;
 const uint8_t *player=s->title_player.view.bytes;
 if(!player)return GH_ACTION_STEP_ARGUMENT;
 if((int16_t)(uint16_t)u32(player+0x54a)<=0||(u32(player+0x15f4)&0x400000u)||
    (int32_t)u32(s->b.init->enemies+0x514)>0)return GH_ACTION_STEP_OK;
 for(size_t i=0;i<s->om_factory.count;++i){
  gh_om_object *item=s->om_factory.objects+i;
  if(item!=argument)continue;
  if(item->stage!=7||item->runtime_created||(item->source.type!=0x3d3&&item->source.type!=0x3d4)||
     !item->view.bytes||item->view.size<0x690||item->view.original_vtable214!=0x429b88||
     item->view.original_kind4ac!=0x4298b0)return GH_ACTION_STEP_OK;
  const uint8_t *actor=item->view.bytes;
  if((int32_t)u32(actor+0x250)<0||actor[0x2f4]||actor[0x2f5]||actor[0x2f6]!=3)
   return GH_ACTION_STEP_OK;
  gh_om_init_state *init=item->initialization;
  if(gh_om_r101_item_collect_001c24e8(init)){
   *frontier=init->frontier?init->frontier:0x1c24e8;
   return GH_ACTION_STEP_CALLEE;
  }
  return GH_ACTION_STEP_OK;
 }
 return GH_ACTION_STEP_OK;
}

gh_action_step_status gh_game_services_player_grenade_action_002dba88(
 gh_game_services *s,void *argument,uint32_t *frontier)
{
 if(!s||argument||!frontier||!s->b.init||!s->b.work||s->b.work->room!=0x101||
    !s->title_player.view.bytes||s->title_player.view.size<0x15f8)
  return GH_ACTION_STEP_ARGUMENT;
 *frontier=0;
 uint8_t *p=s->title_player.view.bytes;
 if((int16_t)(uint16_t)u32(p+0x54a)>0&&!(u32(p+0x15f4)&0x400000u)&&
    (int32_t)u32(s->b.init->enemies+0x514)<1&&!s->b.init->enemies[0x53c]){
  p[0x2f4]=0;
  p[0x2f5]=0x12;
  p[0x2f6]=p[0x2f7]=0;
 }
 return GH_ACTION_STEP_OK;
}

gh_action_step_status gh_game_services_player_interact_action_002dc1d0(
 gh_game_services *s,void *argument,uint32_t *frontier)
{
 if(!s||argument||!frontier||!s->b.init||!s->title_player.view.bytes)
  return GH_ACTION_STEP_ARGUMENT;
 *frontier=0;
 uint8_t *p=s->title_player.view.bytes;
 if(!s->player_interact_camera_pending){
  if((int16_t)(uint16_t)u32(p+0x54a)<=0||(u32(p+0x15f4)&0x400000u)||
     (int32_t)u32(s->b.init->enemies+0x514)>0||s->b.init->enemies[0x53c])
   return GH_ACTION_STEP_OK;
  p[0x2f4]=0;p[0x2f5]=0x30;p[0x2f6]=0;p[0x2f7]=0;
  /* The actual 124F60 call with a2=0 enters only on the camera bit. */
  const uint32_t flags=u32(p+0x15f4);
  if(!(flags&2u))return GH_ACTION_STEP_OK;
  w32(p+0x15f4,flags&~2u);
  s->player_interact_camera_pending=1;
 }
 const gh_player_frame_status status=player_square_camera_exit(s,&s->player_idle_bindings.frame,frontier);
 if(status){
  if(!*frontier)*frontier=0x124f60;
  return status==GH_PLAYER_FRAME_UNSUPPORTED?GH_ACTION_STEP_UNSUPPORTED:GH_ACTION_STEP_CALLEE;
 }
 s->player_interact_camera_pending=0;
 return GH_ACTION_STEP_OK;
}

static gh_player_frame_status player_damage_positioned_effect(void *context,const uint32_t position[3],uint32_t *frontier)
{
 gh_game_services *s=context;
 gh_effect_create_call *call=&s->player_effect_create;
 gh_effect_status status;
 if(!call->active||call->complete){
  status=gh_effect_create_player_impact_begin_00306838(call,position);
  if(status)return GH_PLAYER_FRAME_ARGUMENT;
 }else if(call->request.bank||call->request.index!=3||!call->configuration_positioned||
    memcmp(call->configuration_positioned_position,position,12))return GH_PLAYER_FRAME_BUSY;
 const gh_effect_create_bindings bindings={.storage=player_effect_storage(s),.work=s->b.work,
    .random=&s->b.init->random,.atan_coeff=s->b.camera_envelope?s->b.camera_envelope->atan_coeff:NULL};
 status=gh_game_effect_create_00306528(call,&bindings);
 *frontier=call->frontier;
 if(status)return status==GH_EFFECT_UNSUPPORTED?GH_PLAYER_FRAME_UNSUPPORTED:GH_PLAYER_FRAME_CALLEE;
 return call->complete?GH_PLAYER_FRAME_OK:GH_PLAYER_FRAME_ARGUMENT;
}

static gh_player_frame_status player_death_bgm(void *context,uint32_t *frontier)
{
 gh_game_services *s=context;
 if(!s->b.bgm_requests)return GH_PLAYER_FRAME_UNSUPPORTED;
 const gh_bgm_status status=gh_bgm_requests_death_fade_002ca718(s->b.bgm_requests);
 if(status)*frontier=gh_bgm_requests_frontier(s->b.bgm_requests);
 return status==GH_BGM_OK?GH_PLAYER_FRAME_OK:status==GH_BGM_UNSUPPORTED?
   GH_PLAYER_FRAME_UNSUPPORTED:GH_PLAYER_FRAME_CALLEE;
}

static gh_player_frame_status player_grab_camera(void *context,const gh_actor_model *model,
    uint32_t *frontier)
{
 gh_game_services*s=context;
 if(model!=&s->player_model)return GH_PLAYER_FRAME_ARGUMENT;
 const gh_camera_status status=gh_game_devil_camera_bind_00138e38(&s->devil_camera,s->b.init,model,frontier);
 return status==GH_CAMERA_OK?GH_PLAYER_FRAME_OK:status==GH_CAMERA_UNSUPPORTED?
   GH_PLAYER_FRAME_UNSUPPORTED:GH_PLAYER_FRAME_CALLEE;
}

/* Original 129718's paired attacks share the same cleanup and borrow. */
static gh_actor_motion_status player_grab_00129718(void *context,const gh_enemy_actor *enemy,unsigned argument,uint32_t *frontier)
{
 gh_game_services*s=context;
 uint8_t*p=s->title_player.view.bytes;
 const int em27=original_em27_grab_attacker(s,enemy);
 const int r102_em00=r102_em00_grab_attacker(s,enemy);
 const int east_em00=r102_east_grab_attacker(s,enemy);
 const int west_em00=r102_area11_short_grab_attacker(s,enemy);
 const int arena=r103_grab_attacker(s,enemy,0x200);
 const int arena_em03=r103_grab_attacker(s,enemy,0x203);
 const int boss_em14=r103_grab_attacker(s,enemy,0x214);
 const int boss_em15=r103_grab_attacker(s,enemy,0x215);
 const int long_grab=r102_area11_grab_attacker(s,enemy)||r101_side_long_grab_attacker(s,enemy);
 const int em0a_grab=r102_gate_em0a_grab_attacker(s,enemy)||r103_grab_attacker(s,enemy,0x20a);
 const int r104=enemy&&gh_enemy203_is_r104_entry(&enemy->entry.request,s->b.work->room);
 if(!p||s->title_player.view.size<0x1660||
    (s->b.work->room!=0x100&&!em27&&!r102_em00&&!east_em00&&!west_em00&&!long_grab&&!arena&&!arena_em03&&
     !em0a_grab&&!boss_em14&&!boss_em15&&!r104)||
    !enemy||enemy->entry.b.init!=s->b.init||!enemy->entry.published){
  *frontier=0x129718;
  return GH_ACTOR_MOTION_UNSUPPORTED;
 }
 const uint8_t*attacker=enemy->entry.object.view.bytes;
 if(!attacker||enemy->entry.object.view.size<0x642||attacker[0x641]){
  *frontier=0x129718;
  return GH_ACTOR_MOTION_UNSUPPORTED;
 }
 unsigned selector;
 const uint32_t dependency=u32(attacker+0x564);
 if(argument==0&&dependency==0x20a&&enemy->entry.object.view.size>0x1650&&
    (em0a_grab||(!attacker[0x640]&&attacker[0x1650]==3))&&
    !attacker[0x2f4]&&attacker[0x2f5]==0x4c)selector=8;
 /* EM03's original 235968 passes argument 1 and selects player state 01/09. */
 else if(argument==1&&r104&&dependency==0x250&&!attacker[0x2f4]&&attacker[0x2f5]==0x4c&&
         !attacker[0x2f6]&&!attacker[0x2f7])selector=9;
 else if(argument==7&&boss_em15&&!attacker[0x2f4]&&attacker[0x2f5]==0x52&&
         !attacker[0x2f6]&&!attacker[0x2f7])selector=0xf;
 else if(argument==2&&((dependency==0x200&&attacker[0x640]==20)||r102_em00||em27||arena||
         ((east_em00||west_em00)&&!attacker[0x2f4]))&&attacker[0x2f5]==0x4d)selector=0xa;
 else if(argument==3&&((dependency==0x203&&attacker[0x640]==19)||
         ((long_grab||arena_em03||r100_wave_em02_grab_attacker(s,enemy))&&!attacker[0x2f4])||
         (boss_em14&&!attacker[0x2f4]&&!attacker[0x2f6]&&!attacker[0x2f7]))&&
         attacker[0x2f5]==0x4e)selector=0xb;
 else if(argument==0x1d&&
         ((dependency==0x200&&(arena||attacker[0x640]==20||
           ((r102_em00||east_em00||west_em00)&&attacker[0x2f7]==1)))||
          (em27&&s->b.work->room==0x101&&attacker[0x2f7]==1))&&
         attacker[0x2f4]==1&&attacker[0x2f5]==0x1d&&
         ((p[0x2f4]==1&&p[0x2f5]==0xa)||
          (s->soft_player_abort.active&&p[0x2f4]==0&&p[0x2f5]==0x39)))selector=0x39;
 else{
  *frontier=0x129718;
  return GH_ACTOR_MOTION_UNSUPPORTED;
 }
 if(em27&&s->b.voices){
  const gh_voice_status voice=gh_se_voice_bind_external_adx(s->b.voices,s->b.assets->dvd->disc,
      s->b.sound_control,s->b.boot_elf,s->b.boot_elf_bytes);
  if(voice){
   *frontier=gh_se_voice_frontier(s->b.voices);
   if(!*frontier)*frontier=0x2ca34c;
   return voice==GH_VOICE_UNSUPPORTED?GH_ACTOR_MOTION_UNSUPPORTED:GH_ACTOR_MOTION_CALLEE;
  }
 }
 p[0x2f4]=argument==0x1d?0:1;p[0x2f5]=(uint8_t)selector;p[0x2f6]=p[0x2f7]=0;
 gh_soft_event_status status=room_soft_player_abort(s,frontier);
 if(status)return status==GH_SOFT_EVENT_UNSUPPORTED?GH_ACTOR_MOTION_UNSUPPORTED:GH_ACTOR_MOTION_CALLEE;
 const uint32_t cooldown=0x40a00000;
 memcpy(p+0x54c,&cooldown,4);
 /* 126770 already cleared +1620, so the second SeStop sees handle zero. */
 uint8_t*manager=s->b.init->enemies;
 manager[0x53c]=2;
 const int32_t minimum=2;
 int32_t count;
 memcpy(&count,manager+0x514,4);
 if(count<minimum)memcpy(manager+0x514,&minimum,4);
 s->player_grab_attacker=enemy->entry.handle;
 s->player_grab_escape_stamp=selector==0x39?attacker[0x640]:0xff;
 return GH_ACTOR_MOTION_OK;
}

static gh_player_frame_status player_damage_abort(void *context,const gh_player_frame_bindings *bindings,
    uint32_t *frontier)
{
 gh_game_services *s=context;
 if(bindings->player!=&s->title_player)return GH_PLAYER_FRAME_ARGUMENT;
 gh_soft_event_status status=room_soft_player_abort(s,frontier);
 return !status?GH_PLAYER_FRAME_OK:status==GH_SOFT_EVENT_UNSUPPORTED?GH_PLAYER_FRAME_UNSUPPORTED:GH_PLAYER_FRAME_CALLEE;
}

static gh_player_frame_status player_roulette_projectile_0012bc00(void *context,
 const gh_title_player_view *player,uint32_t *frontier)
{
 gh_game_services *s=context;
 *frontier=0x12bc00;
 if(player!=&s->title_player.view||!player->bytes)return GH_PLAYER_FRAME_ARGUMENT;
 unsigned rank;
 if(gh_core_save_game_level_001fa010(s->b.save,s->b.boot_elf,s->b.boot_elf_bytes,&rank)){
  *frontier=0x1fa010;
  return GH_PLAYER_FRAME_UNSUPPORTED;
 }
 float matrix[16],origin[4],yaw,sine,cosine;
 const float local[3]={0,1.3f,0};
 memcpy(matrix,player->bytes+0x80,sizeof matrix);
 memcpy(&yaw,player->bytes+0x104,4);
 if(gh_scene_at_apply_world_001508d8(matrix,local,origin)){
  *frontier=0x30a2b0;
  return GH_PLAYER_FRAME_UNSUPPORTED;
 }
 if(gh_sdk_rotation_sincos_reference_0030a5d0(yaw,&sine,&cosine,
      GH_STATIC_BINARY32_REFERENCE,frontier))return GH_PLAYER_FRAME_UNSUPPORTED;
 const float velocity[3]={sine*1.5f,0,cosine*1.5f};
 gh_om_runtime_create *create=&s->roulette_projectile_create;
 gh_om_object *object=NULL;
 gh_scene_placement_status status=gh_om_factory_create_0030fa98(&s->om_factory,create,0x3be,0xffff,&object);
 if(!status&&object)status=gh_om_factory_initialize_runtime_3be(&s->om_factory,object);
 if(status){
  *frontier=s->om_factory.frontier?s->om_factory.frontier:0x30fa98;
  return GH_PLAYER_FRAME_UNSUPPORTED;
 }
 if(object){
  gh_om3be_state *state=(gh_om3be_state *)object->initialization;
  status=gh_om3be_launch_roulette_001bb548(state,player,origin,velocity,rank);
  if(status){
   *frontier=state->common.frontier?state->common.frontier:0x1bb548;
   return GH_PLAYER_FRAME_UNSUPPORTED;
  }
 }
 memset(create,0,sizeof(*create));
 *frontier=0;
 return GH_PLAYER_FRAME_OK;
}

static gh_player_frame_status player_roulette_time_scale(void *context,uint32_t factor,uint32_t *frontier)
{
 gh_game_services *s=context;
 *frontier=0x292f38;
 gh_effects_frame_globals *globals=s->b.effects_globals;
 gh_sound_control *sound=s->b.sound_control;
 if(!globals||!s->b.init||!sound||!sound->initialized)return GH_PLAYER_FRAME_ARGUMENT;
 if(factor!=0x3cf5c28fu&&factor!=0x3dcccccdu)return GH_PLAYER_FRAME_UNSUPPORTED;
 if(gh_effects_frame_set_speed_00292f38(globals,s->b.init,
     &globals->canonical_ratio_754c48,factor))
  return GH_PLAYER_FRAME_UNSUPPORTED;
 if(factor==0x3cf5c28fu){
  sound->flags_b0|=0x400000u;
  sound->flags_ac|=0x400000u;
 }
 *frontier=0;
 return GH_PLAYER_FRAME_OK;
}

static const gh_title_player_view *player_roulette_target_owner(void *context,uint32_t handle)
{
 gh_game_services *s=context;
 gh_enemy_ems_live live;
 if(s->b.work->room!=0x104||gh_enemy_ems_live_at(s->enemies,handle,&live)||
    !live.enemy202||!live.actor||
    !gh_enemy203_is_r104_entry(&live.enemy202->entry.request,s->b.work->room))
  return NULL;
 return &live.actor->view;
}

static gh_player_frame_status player_frame_effect_create(void*p,const gh_player_frame_effect_request*r,uint32_t*f){
 gh_game_services*s=p;if(!r||!r->actor||!f)return GH_PLAYER_FRAME_ARGUMENT;*f=0;
 const gh_actor_model *model=&s->player_model;
 if(r->actor->bytes!=s->title_player.view.bytes){
  const gh_player_tail_target_slot *slot=&s->player_target_644;
  gh_enemy_ems_live live;
  const uint8_t *player=s->title_player.view.bytes;
  const int r104_target=s->b.work->room==0x104&&
      player_roulette_target_owner(s,slot->expected_guest_token)==r->actor;
  if((s->b.work->room!=0x103&&!r104_target)||player[0x2f4]||player[0x2f5]!=0x2b||
     slot->view!=r->actor||!slot->expected_guest_token||r->parameter_bits||r->kind!=0x61||
     r->mode!=6||r->selector!=-1||r->sequence_scale||
     gh_enemy_ems_live_at(s->enemies,slot->expected_guest_token,&live)||
     !live.actor||!live.model||live.model->root.bytes!=r->actor->bytes){
   *f=0x306528;return GH_PLAYER_FRAME_UNSUPPORTED;
  }
  model=live.model;
 }
 if(r->argument3){*f=0x306528;return GH_PLAYER_FRAME_UNSUPPORTED;}
 int32_t mode;memcpy(&mode,&r->mode,4);
 gh_effect_create_request request={.bank=r->parameter_bits,.index=(uint32_t)r->kind,.actor=r->actor,.mode=mode,.selector=r->selector};
 gh_effect_create_call*c=&s->player_effect_create;gh_effect_status e;
 if(!c->active||c->complete){e=r->sequence_scale?gh_effect_create_actor_sequence_0014fb48(c,&request,r->sequence_scale):
   gh_effect_create_begin(c,&request);
  if(e){*f=0x306528;return e==GH_EFFECT_UNSUPPORTED?GH_PLAYER_FRAME_UNSUPPORTED:GH_PLAYER_FRAME_ARGUMENT;}}
 else if(c->request.bank!=request.bank||c->request.index!=request.index||c->request.actor!=request.actor||c->request.mode!=request.mode||c->request.selector!=request.selector||
   c->configuration_nested_scale!=r->sequence_scale)return GH_PLAYER_FRAME_BUSY;
 gh_effect_create_bindings b={.storage=player_effect_storage(s),.work=s->b.work,
   .random=&s->b.init->random,.model=model};
 e=gh_game_effect_create_00306528(c,&b);*f=c->frontier;
 if(e)return e==GH_EFFECT_UNSUPPORTED?GH_PLAYER_FRAME_UNSUPPORTED:GH_PLAYER_FRAME_CALLEE;
 /* The original caller ignores the bool, including genuine pool-full return0. */
 return c->complete?GH_PLAYER_FRAME_OK:GH_PLAYER_FRAME_ARGUMENT;
}
static gh_player_door_status player_door_publish(void*p,const gh_player_door_action*r,uint32_t*f){
 gh_game_services*s=p;if(!r||!f||!r->player||r->player->bytes!=s->title_player.view.bytes)return GH_PLAYER_DOOR_ARGUMENT;
 if(r->argument){*f=0x1f7858;return GH_PLAYER_DOOR_UNSUPPORTED;}
 gh_action_set_request request={.priority=r->priority,.life=r->life,.unknown20=r->unknown20,.callback_id=r->callback_entry,.argument=NULL,.kind=r->kind};
 int emitted;unsigned index;gh_action_set_status e=gh_action_button_set_001f7858(&s->action_buttons,&request,&emitted,&index,f);
 /* Original voidSet genuinely does nothing when all8 entries are occupied. */
 return !e?GH_PLAYER_DOOR_OK:e==GH_ACTION_SET_UNSUPPORTED?GH_PLAYER_DOOR_UNSUPPORTED:GH_PLAYER_DOOR_CALLEE;
}
static gh_player_frame_status player_frame_door_probe(void*p,const gh_title_player_view*v,uint32_t*f){
 gh_game_services*s=p;if(!v||!f||v->bytes!=s->title_player.view.bytes||v->size!=s->title_player.view.size)return GH_PLAYER_FRAME_ARGUMENT;
 if(!s->player_door.active||s->player_door.complete){
  if(gh_game_room_pack_bundle(&s->room_pack,&s->player_door_room)){*f=0x31ccf0;return GH_PLAYER_FRAME_CALLEE;}
  s->player_door_bindings=(gh_player_door_bindings){.player=&s->title_player.view,.current_player=&s->title_player.view,.init=s->b.init,
   .scene_at=&s->scene_at,.room=&s->player_door_room,.work=s->b.work,.actors=&s->scene_tables,.numeric_policy=GH_CAMERA_NATIVE_BINARY32_REFERENCE,
   .publish=player_door_publish,.context=s};
  if(gh_player_door_probe_begin(&s->player_door)){*f=0x2dc920;return GH_PLAYER_FRAME_ARGUMENT;}
 }
 gh_player_door_status e=gh_player_door_probe_002dc920_002dcad8(&s->player_door,&s->player_door_bindings);*f=s->player_door.frontier;
 if(e)return e==GH_PLAYER_DOOR_UNSUPPORTED?GH_PLAYER_FRAME_UNSUPPORTED:e==GH_PLAYER_DOOR_NUMERIC?GH_PLAYER_FRAME_NUMERIC:GH_PLAYER_FRAME_CALLEE;
 return s->player_door.complete?GH_PLAYER_FRAME_OK:GH_PLAYER_FRAME_ARGUMENT;
}
typedef struct {
 gh_game_services *services;
 const gh_game_actor_render_provider *provider;
 gh_shadow_call *call;
} gh_services_event_move_shadow;

static gh_game_event_status event_move_shadow(void *context,gh_event_actor_factory *actor,uint32_t *frontier){
 gh_services_event_move_shadow *move=context;
 gh_game_services *s=move->services;
 const gh_game_actor_render_provider *provider=move->provider;
 gh_shadow_bindings b={.model=&actor->model,.shadow=&actor->shadow,.work=s->b.work,
  .camera=provider->camera?provider->camera:&s->camera,.world_light=&s->world_light,.actors=&s->scene_tables,
  .up_owner=s->b.camera_follow,.pivot_owner=s->b.player_shared_vector,.templates=provider->shadow_templates,
  .emit=provider->shadow_emit,.emit_context=provider->shadow_context,.policy=GH_STATIC_BINARY32_REFERENCE};
 gh_actor_model_status status=gh_actor_shadow_draw_0014efa8(move->call,&b,frontier);
 if(status&&!*frontier)*frontier=0x14efa8;
 return !status?GH_EVENT_OK:status==GH_ACTOR_MODEL_UNSUPPORTED?GH_EVENT_UNSUPPORTED:GH_EVENT_CALLEE;
}

static gh_game_event_status event_move_tail(void *context,gh_event_actor_factory *actor,
 uint32_t entry,uint32_t *frontier){
 gh_game_services *s=context;
 *frontier=entry;
 if(entry==0x2df860){
  size_t index=actor->actor_index;
  if(index>=sizeof(s->event_actor_move)/sizeof(*s->event_actor_move))return GH_EVENT_ARGUMENT;
  gh_cloth_step_bindings bindings={.templates=s->b.cloth_templates,.globals=s->b.cloth_globals,
   .random=&s->b.init->random,.actors=&s->scene_tables,
   .matrix_policy=GH_STATIC_BINARY32_REFERENCE,.random_policy=GH_FRAME_CHOP_REFERENCE};
  return gh_game_event_pendulum_002df860(&s->event_actor_move[index].pendulum,actor,
   s->event_actor_extra,&bindings,s->b.player_shared_vector,frontier);
 }
 if(entry!=0x2b6a40&&entry!=0x2b79e0)return GH_EVENT_UNSUPPORTED;
 if(!s->event_actor_extra||!s->event_actor_extra->bindings.creation)return GH_EVENT_ARGUMENT;
 gh_event_components *components=s->event_actor_extra->bindings.creation->bindings.components;
 if(entry==0x2b79e0)return gh_event_actor_ring_002b79e0(actor,components,frontier);
 return gh_event_actor_bust_002b6a40(actor,components,frontier);
}

static gh_object_step_status om_break_effect(void *context,const gh_actor_model *model,
 const uint32_t position[3],uint32_t yaw_bits,uint32_t *frontier) {
 gh_game_services *s=context;
 gh_effect_create_call *call=&s->om_break_effect;
 *frontier=0x306528;
 if(!s->b.camera_envelope)return GH_OBJSTEP_UNSUPPORTED;
 if(!call->active||call->complete) {
  uint16_t kind;
  memcpy(&kind,model->root.bytes+0x2fe,sizeof(kind));
  gh_effect_status opened;
  if(kind==0x313||(kind==0x300&&s->b.work->room==0x100)||
     (kind==0x330&&s->b.work->room==0x101&&model->root.original_vtable214==0x425b90)){
   const gh_effect_create_request door={.bank=0,.index=0x1a,.actor=&model->root,.mode=-1,.selector=-1};
   opened=gh_effect_create_begin(call,&door);
  }else if(kind==0x3bd&&s->b.work->room==0x200&&model->root.original_vtable214==0x428c20){
   if(model->root.bytes[0x2f5]==2)
    opened=gh_effect_create_catapult_impact_00306838(call,position);
   else{
    const gh_effect_create_request catapult={.bank=0,.index=0x13,
        .actor=&model->root,.mode=0,.selector=-1};
    opened=gh_effect_create_begin(call,&catapult);
   }
  }else if(kind==0x30a||kind==0x304||kind==0x33f||
      (kind==0x309&&s->b.work->room==0x102&&model->root.original_vtable214==0x4237e8))opened=gh_effect_create_om30a_break_begin_0018c580(call,position);
  else if(kind==0x363||(s->b.work->room==0x101&&(kind==0x3c3||kind==0x3e0))||kind==0x3c4||kind==0x3c8||kind==0x3d0||kind==0x3d1||kind==0x3df||
      ((s->b.work->room==0x100||s->b.work->room==0x104)&&(kind==0x3e0||kind==0x3e1))||(kind>=0x3d3&&kind<=0x3d7))
   opened=gh_effect_create_om_sparkle_begin_00306838(call,position);
  else if(kind==0x305)opened=gh_effect_create_om305_break_begin_001c2ce0(call,position);
  else if(kind==0x346&&s->b.work->room==0x100&&model->root.original_vtable214==0x426e78&&
          model->root.size>=0xda0){
   if(u32(model->root.bytes+0x5b0)&2u)
    opened=gh_effect_create_om307_break_begin_00306838(call,u32(model->root.bytes+0x5ec));
   else opened=gh_effect_create_om346_break_begin_00306838(call,position);
  }
  else if(((kind==0x331&&s->b.work->room==0x102)||(kind==0x332&&s->b.work->room==0x104))&&
          model->root.original_vtable214==0x425c78)
   opened=gh_effect_create_om331_break_begin_0019ca58(call,position);
  else if(kind==0x307&&s->b.work->room==0x100&&model->root.original_vtable214==0x423608&&
          model->root.size>=0xce0&&u32(model->root.bytes+0x5f0)==1)
   opened=gh_effect_create_om307_break_begin_00306838(call,u32(model->root.bytes+0x5ec));
  else if(kind==0x3be&&model->root.original_vtable214==0x4290c8&&s->b.work->room==0x100)
   opened=gh_effect_create_devil_projectile_impact_00306838(call,position);
  else if(kind==0x3be&&model->root.original_vtable214==0x4290c8&&s->b.work->room==0x104&&
          model->root.size==0x630&&!u32(model->root.bytes+0x610))
   opened=gh_effect_create_em03_projectile_impact_00306838(call,position);
  else opened=gh_effect_create_om324_break_begin_001bfd28(call,position,yaw_bits);
  if(opened)
   return GH_OBJSTEP_CALLEE;
  s->om_break_effect_owner=model;
 } else if(s->om_break_effect_owner!=model) return GH_OBJSTEP_ARGUMENT;
 const gh_effect_create_bindings bindings={.storage=player_effect_storage(s),
  .work=s->b.work,.random=&s->b.init->random,.atan_coeff=s->b.camera_envelope->atan_coeff,.model=model};
 gh_effect_status status=gh_game_effect_create_00306528(call,&bindings);
 if(call->frontier)*frontier=call->frontier;
 if(status)return status==GH_EFFECT_UNSUPPORTED?GH_OBJSTEP_UNSUPPORTED:GH_OBJSTEP_CALLEE;
 if(!call->complete)return GH_OBJSTEP_CALLEE;
 s->om_break_effect_owner=NULL;
 *frontier=0;
 return GH_OBJSTEP_OK;
}

static gh_object_step_status om364_fuse_effect(void *context,gh_om364_state *state,uint32_t *frontier)
{
 gh_game_services *s=context;
 if(!s||!state||!state->common.object||!s->b.camera_envelope)return GH_OBJSTEP_ARGUMENT;
 gh_effect_create_call *call=&state->effect;
 if(!call->active){
  const gh_effect_create_request request={.bank=0x58,.index=5,
      .actor=&state->common.object->view,.mode=7,.selector=-1};
  if(gh_effect_create_begin(call,&request))return GH_OBJSTEP_ARGUMENT;
 }
 const gh_effect_create_bindings bindings={.storage=player_effect_storage(s),.work=s->b.work,
     .random=&s->b.init->random,.model=&state->common.model,
     .atan_coeff=s->b.camera_envelope->atan_coeff};
 const gh_effect_status status=gh_game_effect_create_00306528(call,&bindings);
 *frontier=call->frontier;
 if(status)return status==GH_EFFECT_UNSUPPORTED?GH_OBJSTEP_UNSUPPORTED:GH_OBJSTEP_CALLEE;
 return call->complete?GH_OBJSTEP_OK:GH_OBJSTEP_CALLEE;
}

static gh_object_step_status om364_blast_effect(void *context,gh_om364_state *state,uint32_t *frontier)
{
 gh_game_services *s=context;
 if(!s||!state||!state->common.object||!s->b.camera_envelope||
    !state->common.object->view.alias_f0)return GH_OBJSTEP_ARGUMENT;
 gh_effect_create_call *call=&state->blast;
 if(!call->active){
  uint32_t position[3];
  memcpy(position,state->common.object->view.alias_f0,sizeof(position));
  if(gh_effect_create_om364_blast_begin_00306838(call,position))return GH_OBJSTEP_ARGUMENT;
 }
 const gh_effect_create_bindings bindings={.storage=player_effect_storage(s),.work=s->b.work,
     .random=&s->b.init->random,.model=&state->common.model,
     .atan_coeff=s->b.camera_envelope->atan_coeff};
 const gh_effect_status status=gh_game_effect_create_00306528(call,&bindings);
 *frontier=call->frontier;
 if(status)return status==GH_EFFECT_UNSUPPORTED?GH_OBJSTEP_UNSUPPORTED:GH_OBJSTEP_CALLEE;
 return call->complete?GH_OBJSTEP_OK:GH_OBJSTEP_CALLEE;
}

static gh_object_step_status om364_stop_voice(void *context,uint32_t handle,uint32_t *frontier)
{
 gh_game_services *s=context;
 const gh_voice_status status=gh_se_voice_stop_actor_002cbba0(s->b.voices,handle);
 *frontier=status?0x2cbba0:0;
 return status?GH_OBJSTEP_CALLEE:GH_OBJSTEP_OK;
}

static gh_actor_motion_status om_break_sound(void *context,const gh_motion_sound_request *request,
 uint32_t *handle,uint32_t *frontier) {
 gh_game_services *s=context;
 if(actor_sound_prepare(s,&s->om_sound,frontier))return GH_ACTOR_MOTION_CALLEE;
 if(!s->om_sound.bound){*frontier=0x2cba48;return GH_ACTOR_MOTION_UNSUPPORTED;}
 return gh_actor_attached_sound_motion(&s->om_sound,request,handle,frontier);
}

static gh_object_step_status om_stop_sound(void *context,uint32_t handle,uint32_t *frontier)
{
 gh_game_services *s=context;
 if(!s->b.voices){*frontier=0x2cbba0;return GH_OBJSTEP_UNSUPPORTED;}
 const gh_voice_status status=gh_se_voice_stop_actor_002cbba0(s->b.voices,handle);
 *frontier=status?gh_se_voice_frontier(s->b.voices):0;
 return status?GH_OBJSTEP_CALLEE:GH_OBJSTEP_OK;
}

static gh_object_step_status om_catapult_impact_sound(void *context,const gh_title_player_view *actor,
 int16_t bank,int16_t program,uint32_t *frontier)
{
 gh_game_services *s=context;
 *frontier=0x2cb968;
 if(!s->b.voices||!s->b.sound_scene||s->b.work->room!=0x200||!actor||!actor->alias_f0||
    actor->size!=0x640||actor->original_vtable214!=0x428c20)
  return GH_OBJSTEP_UNSUPPORTED;
 float position[4]={0,0,0,1};
 memcpy(position,actor->alias_f0,12);
 const gh_se_attached_environment *environment=&s->b.sound_scene->ts_environment;
 uint32_t handle;
 const gh_voice_status status=gh_se_voice_request_r200_impact_002cb968(s->b.voices,
      bank,program,position,environment,&handle);
 if(status){
  *frontier=gh_se_voice_frontier(s->b.voices);
  return status==GH_VOICE_UNSUPPORTED?GH_OBJSTEP_UNSUPPORTED:GH_OBJSTEP_CALLEE;
 }
 *frontier=0;
 return GH_OBJSTEP_OK;
}

static gh_object_step_status om_break_collision(void *context,uint64_t key,uint32_t *frontier) {
 gh_game_services *s=context;
 uint32_t changed;
 gh_scene_table_status status=gh_collision_scroll_set_key_enabled_001331b8(
     &s->scene_tables,&s->r100.collision,key,0,&changed);
 *frontier=status?0x1331b8:0;
 return status?GH_OBJSTEP_CALLEE:GH_OBJSTEP_OK;
}

/* 31BD88 clears each original 16-byte row's active byte. */
void gh_game_services_clear_player_vibration_0031bd88(gh_game_services *services)
{
 if(!services)return;
 for(unsigned row=0;row<32;row++)services->player_damage_vibration[row][0]=0;
}

/* 1C8AB0 owns its Give sphere and the unique Work5F4-held object borrow. */
static gh_player_frame_status player_interact_prop_001c8ab0(void *context,uint32_t handle,
 const gh_title_player_view *view,const gh_player_frame_bindings *b,size_t motion_row,
 size_t sequence_row,uint32_t *frontier)
{
 gh_game_services *s=context;
 if(!s||!frontier)return GH_PLAYER_FRAME_ARGUMENT;
 *frontier=0x1c8ab0;
 if(!view||!view->bytes||view->size<0x9c0||view->original_vtable214!=0x4238d8||
    b->work!=s->b.work||b->work->room!=0x101||b->player!=&s->title_player||
    b->init!=s->b.init||!b->resource||b->resource_bytes<sequence_row+4||
    b->resource_bytes<motion_row+4)
  return GH_PLAYER_FRAME_UNSUPPORTED;
 if(!handle){
  const int idle=motion_row==0x36c&&sequence_row==0x388;
  const int throwing=motion_row==0x37c&&sequence_row==0x398;
  if(s->player_prop.stage!=7||!s->player_prop.object||view!=&s->player_prop.object->view||
     (!idle&&!throwing)||s->player_prop.move_stage)
   return GH_PLAYER_FRAME_ARGUMENT;
  gh_actor_motion *motion=&s->player_prop.motion;
  gh_actor_motion_call *call=&s->player_prop.idle_motion;
  gh_actor_motion_status status=GH_ACTOR_MOTION_OK;
  if(!call->active||call->complete)
   status=gh_actor_motion_call_open_parameters_002a8578(call,motion,&motion->b,
      u32(b->resource+motion_row),u32(b->resource+sequence_row),0,idle?10:3,0,0);
  if(!status)status=gh_actor_motion_call_step_002a8578(call);
  if(status){*frontier=call->frontier?call->frontier:0x2a8578;
   return status==GH_ACTOR_MOTION_UNSUPPORTED?GH_PLAYER_FRAME_UNSUPPORTED:GH_PLAYER_FRAME_CALLEE;}
  *frontier=0;
  return GH_PLAYER_FRAME_OK;
 }
 void *registered=NULL;
 if(gh_room_object_lookup(&s->pools,handle,GH_ROOM_OBJECT_OM,&s->om_factory,&registered))
  return GH_PLAYER_FRAME_ARGUMENT;
 gh_om_object *object=registered;
 gh_actor_model *model=NULL;
 if(!object||object->source.type!=0x30a||object->view.bytes!=view->bytes||
    gh_om_factory_model_at(&s->om_factory,handle,&model)||!object->initialization)
  return GH_PLAYER_FRAME_ARGUMENT;
 if(s->player_prop.stage>0&&(s->player_prop.object!=object||s->player_prop.handle!=handle))
  return GH_PLAYER_FRAME_BUSY;
 gh_om_init_state *owner=object->initialization;
 uint8_t *p=object->view.bytes;
 const gh_actor_part *part;
 if(!owner->bindings||!owner->bindings->shapes||
    gh_actor_model_part_at(model,0,&part)||!part->alias_d0||u32(p+0x600)||u32(p+0x644))
  return GH_PLAYER_FRAME_UNSUPPORTED;
 const uint64_t collision_key=(uint64_t)u32(p+0x544)<<32|u32(p+0x540);
 if(!s->player_prop.stage){
  if(!(u32(p+0x5b0)&8u)||owner->hitarea_created==0)return GH_PLAYER_FRAME_UNSUPPORTED;
  s->player_prop.object=object;s->player_prop.model=model;s->player_prop.handle=handle;
  w32(p+0x5b0,(u32(p+0x5b0)&~8u)|4u);
  s->player_prop.stage=1;
 }
 if(s->player_prop.stage==1){
  if(gh_hitarea_object_set_active_001fd668(&owner->hitarea,0,frontier))return GH_PLAYER_FRAME_CALLEE;
  s->player_prop.stage=2;
 }
 if(s->player_prop.stage==2){
  if(om_break_collision(s,collision_key,frontier))return GH_PLAYER_FRAME_CALLEE;
  w32(p+0x5b8,u32(p+0x5b8)&~0x40u);memset(p+0x2f4,0,4);
  s->player_prop.stage=3;
 }
 if(s->player_prop.stage==3){
  if(om_break_collision(s,collision_key,frontier))return GH_PLAYER_FRAME_CALLEE;
  int original;
  if(gh_hitarea_object_create_001fe0e8(s->b.init,&object->view,0,&s->player_prop.give,
      &original,frontier))return GH_PLAYER_FRAME_CALLEE;
  if(!original){*frontier=0x1fe0e8;return GH_PLAYER_FRAME_UNSUPPORTED;}
  s->player_prop.give_created=1;s->player_prop.stage=4;
 }
 if(s->player_prop.stage==4){
  const uint32_t zero[3]={0};
  if(gh_hitarea_add_sphere_001fdaf0(&s->player_prop.give,owner->bindings->shapes,
      part->bytes+0x80,zero,0x3f000000,&s->player_prop.shape_index,frontier))
   return GH_PLAYER_FRAME_CALLEE;
  if(s->player_prop.shape_index<0){*frontier=0x1fdaf0;return GH_PLAYER_FRAME_UNSUPPORTED;}
  s->player_prop.stage=5;
 }
 if(s->player_prop.stage==5){
  if(gh_hitarea_object_set_active_001fd668(&s->player_prop.give,0,frontier))return GH_PLAYER_FRAME_CALLEE;
  memset(p+0x2f4,0,4);memcpy(p+0x630,part->alias_d0,12);p[0x640]=0;w32(p+0x624,0);
  s->player_prop.stage=6;
 }
 if(s->player_prop.stage==6){
  if(actor_sound_prepare(s,&s->om_sound,frontier))return GH_PLAYER_FRAME_CALLEE;
  const gh_actor_motion_bindings motion={.model=model,.work=b->work,
   .resource=b->resource,.resource_bytes=b->resource_bytes,.ik_globals=b->ik_globals,
   .save=b->save,.save_binding_known=1,.sound=s->om_sound.bound?gh_actor_attached_sound_motion:NULL,
   .sound_context=&s->om_sound,.sequence_random=s->b.frame_random};
  const gh_actor_motion_status status=gh_actor_motion_begin_parameters_002a8578(
     &s->player_prop.motion,&motion,u32(b->resource+motion_row),u32(b->resource+sequence_row),0,0,0);
  if(status){*frontier=s->player_prop.motion.frontier?s->player_prop.motion.frontier:0x2a8578;
   return status==GH_ACTOR_MOTION_UNSUPPORTED?GH_PLAYER_FRAME_UNSUPPORTED:GH_PLAYER_FRAME_CALLEE;}
  s->player_prop.stage=7;
 }
 *frontier=0;
 return GH_PLAYER_FRAME_OK;
}

static float prop_number(const void *p)
{
 float value;
 memcpy(&value,p,sizeof value);
 return value;
}

static float prop_product(float a,float b)
{
 volatile float value=a*b;
 return value;
}

static float prop_sum(float a,float b)
{
 volatile float value=a+b;
 return value;
}

static gh_player_frame_status player_throw_prop_0012b618(void *context,
 const gh_player_frame_bindings *b,uint32_t target_handle,uint32_t *frontier)
{
 gh_game_services *s=context;
 *frontier=0x12b618;
 if(!s||b->player!=&s->title_player||b->init!=s->b.init||
    s->player_prop.stage!=7||!s->player_prop.object||s->player_prop.move_stage||
    b->aux_6a0!=&s->player_prop.object->view||!b->player->view.alias_f0||
    !b->aux_6a0->alias_f0||b->aux_6a0->size<0x648)
  return GH_PLAYER_FRAME_ARGUMENT;
 uint8_t *p=b->aux_6a0->bytes;
 if(u32(p+0x644))return GH_PLAYER_FRAME_UNSUPPORTED;
 const gh_actor_part *part=NULL;
 if(p[0x2b4]&&(gh_actor_model_part_at(s->player_prop.model,0,&part)||!part->alias_d0)){
  *frontier=0x1c8be0;
  return GH_PLAYER_FRAME_UNSUPPORTED;
 }
 float origin[3],velocity[3];
 memcpy(origin,b->player->view.alias_f0,sizeof origin);
 origin[1]=prop_sum(origin[1],1.6f);
 if(target_handle){
  gh_player_target target={.handle=target_handle};
  const gh_player_target_bindings targets=gh_game_services_scene_targets(s);
  if(targets.resolve(targets.context,target_handle,&target.view,frontier))
   return GH_PLAYER_FRAME_UNSUPPORTED;
  const unsigned kind=target.view.bytes[0x2fe]|(unsigned)target.view.bytes[0x2ff]<<8;
  float destination[3],height=1;
  if(kind>=0x200&&kind<0x300&&target.view.bytes[0x2b4]){
   gh_enemy_ems_live live;
   const gh_actor_part *anchor;
   if(gh_enemy_ems_live_at(s->enemies,target_handle,&live)||!live.actor||
      live.actor->view.bytes!=target.view.bytes||!live.model||
      gh_actor_model_part_at(live.model,0,&anchor)||!anchor->alias_f0)
    return GH_PLAYER_FRAME_UNSUPPORTED;
   memcpy(destination,anchor->alias_f0,sizeof destination);
   height=.5f;
  }else{
   if(gh_player_throw_target_position(&targets,&target,destination,frontier))
    return GH_PLAYER_FRAME_UNSUPPORTED;
   if(kind>=0x200&&kind<0x300)height=1.6f;
  }
  destination[1]=prop_sum(destination[1],height);
  for(unsigned i=0;i<3;i++)velocity[i]=prop_sum(destination[i],-origin[i]);
  const float length=sqrtf(prop_sum(prop_sum(prop_product(velocity[0],velocity[0]),
   prop_product(velocity[1],velocity[1])),prop_product(velocity[2],velocity[2])));
  if(!isfinite(length))return GH_PLAYER_FRAME_NUMERIC;
  volatile float reciprocal=length>0?1/length:0;
  for(unsigned i=0;i<3;i++)velocity[i]=prop_product(prop_product(velocity[i],reciprocal),.7f);
 }else{
  const uint8_t *matrix=b->player->view.bytes+0x80;
  for(unsigned i=0;i<3;i++)
   velocity[i]=prop_sum(prop_product(prop_number(matrix+16+i*4),.01f),
                        prop_product(prop_number(matrix+32+i*4),.7f));
 }
 for(unsigned i=0;i<3;i++)
  if(!isfinite(origin[i])||!isfinite(velocity[i]))return GH_PLAYER_FRAME_NUMERIC;
 float roll=0;
 if(part){
  uint32_t random;
  if(gh_game_mt_next_0030f348(&s->b.init->random,&random)){
   *frontier=0x31d020;
   return GH_PLAYER_FRAME_UNSUPPORTED;
  }
  uint32_t jitter=gh_frame_jitter_bits_0031d020_word(random);
  roll=prop_sum(prop_product(prop_number(&jitter),.5235988f),.5235988f);
 }
 /* 1C8BE0 changes the box's original Move PMF, then 12B618 publishes its
  * launch position and releases Gene's typed +6A0 borrow. */
 w32(p+0x2f4,0x00000100);
 memcpy(p+0x610,velocity,sizeof velocity);
 if(part){
  memcpy(part->alias_d0,p+0x630,12);
  w32(part->bytes+0x100,0);
  w32(part->bytes+0x104,0);
  memcpy(part->bytes+0x108,&roll,4);
  p[0x640]=0;
 }
 memcpy(b->aux_6a0->alias_f0,origin,sizeof origin);
 memcpy(p+0x490,origin,sizeof origin);
 s->player_prop.stage=8;
 s->player_idle_bindings.frame.aux_6a0=NULL;
 *frontier=0;
 return GH_PLAYER_FRAME_OK;
}

/* 1C8DD8 removes the thrown object's Give before virtual +A4, then
 * 18CDA8 hands the same OM30A back to its regular break dispatch. */
static gh_game_services_status player_prop_break_001c8dd8(gh_game_services *s,
 const gh_title_player_view *view,uint32_t *frontier)
{
 uint8_t *p=view->bytes;
 if(s->player_prop.give_created){
  int released;
  if(gh_hitarea_object_clear_shapes_001fd580(&s->player_prop.give,&released,frontier)||
     !released||gh_hitarea_object_release_empty_001fe278(&s->player_prop.give,frontier))
   return GH_SERVICES_ASSET;
  s->player_prop.give_created=0;
  w32(p+0x600,0);w32(p+0x604,UINT32_MAX);
 }
 if(p[0x640]){
  if(!(u32(p+0x250)&0x80000000u)){
   gh_effect_status status=detach_model_effect_sound(s,s->player_prop.model,frontier);
   if(status)return status==GH_EFFECT_UNSUPPORTED?GH_SERVICES_FRONTIER:GH_SERVICES_ASSET;
   w32(p+0x250,u32(p+0x250)|0x80000002u);
  }
 }else if(!(u32(p+0x5b0)&2u)){
  for(unsigned i=0;i<3;i++){
   const float delta=prop_sum(prop_number(view->alias_f0+i*4),-prop_number(p+0x490+i*4));
   memcpy(p+0x5c0+i*4,&delta,4);
  }
  w32(p+0x2f4,2);
  w32(p+0x5b0,(u32(p+0x5b0)&~4u)|1u);
 }
 *frontier=0;
 return GH_SERVICES_OK;
}

static gh_game_services_status player_prop_flight_body(gh_game_services *s,
 const gh_title_player_view *view,uint32_t *frontier)
{
 *frontier=0x1c8288;
 if(!s->player_prop.object||view->bytes!=s->player_prop.object->view.bytes||
    !view->alias_f0||view->size<0x648||!s->title_player.view.bytes)
  return GH_SERVICES_ASSET;
 uint8_t *p=view->bytes,*give=s->player_prop.give.bytes;
 if(s->player_prop.move_stage<2&&
    (p[0x2f4]||p[0x2f5]!=1||p[0x2f6]>1||!s->player_prop.give_created||!give))
  return GH_SERVICES_FRONTIER;
 if(s->player_prop.move_stage==6){*frontier=0x306838;return GH_SERVICES_FRONTIER;}
 const float rate=prop_product(prop_number(s->b.init->enemies+0x548),
                            prop_number((const uint8_t*)s->b.work+0x574));
 if(!isfinite(rate)||rate<0)return GH_SERVICES_ASSET;
 const gh_actor_part *part=NULL;
 if(p[0x2b4]&&gh_actor_model_part_at(s->player_prop.model,0,&part))return GH_SERVICES_ASSET;

 if(!s->player_prop.move_stage){
  float v[3], position[3];
  memcpy(v,p+0x610,sizeof v);
  memcpy(position,view->alias_f0,sizeof position);
  for(unsigned i=0;i<3;i++)if(!isfinite(v[i])||!isfinite(position[i]))return GH_SERVICES_ASSET;
  memcpy(s->player_prop.flight_previous,position,sizeof position);
  /* 1C7E30 resets Give before dispatching this movement. */
  memset(give+0x46,0,2);w32(give+0x4c,0);
  if(gh_hitarea_object_set_active_001fd668(&s->player_prop.give,0,frontier))return GH_SERVICES_ASSET;
  uint32_t pc;
  if(!p[0x2f6]){
   float pitch,yaw;
   const float horizontal=sqrtf(prop_sum(prop_product(v[0],v[0]),prop_product(v[2],v[2])));
   if(gh_game_angle_001f8180(v[1],horizontal,&pitch,GH_CAMERA_NATIVE_BINARY32_REFERENCE,&pc)||
      gh_game_angle_001f8180(v[0],v[2],&yaw,GH_CAMERA_NATIVE_BINARY32_REFERENCE,&pc)){
    *frontier=pc?pc:0x1f8180;return GH_SERVICES_ASSET;
   }
   pitch=-pitch;
   memcpy(p+0x100,&pitch,4);memcpy(p+0x104,&yaw,4);w32(p+0x108,0);
   w32(p+0x620,0x42b40000);w32(p+0x628,0x41200000);
   p[0x2f6]=1;
  }
  if(part){
   const float angle=prop_sum(prop_number(part->bytes+0x100),prop_product(rate,.87266463f));
   memcpy(part->bytes+0x100,&angle,4);
  }
  for(unsigned i=0;i<3;i++)position[i]=prop_sum(position[i],prop_product(v[i],rate));
  memcpy(view->alias_f0,position,sizeof position);
  float gravity_delay=prop_number(p+0x628);
  if(gravity_delay>0){
   gravity_delay=prop_sum(gravity_delay,-rate);
   memcpy(p+0x628,&gravity_delay,4);
  }else{
   const float vertical=prop_sum(v[1],-prop_product(rate,.013f));
   memcpy(p+0x614,&vertical,4);
  }
  s->player_prop.move_stage=1;
 }
 if(s->player_prop.move_stage==1){
  uint32_t pc=0;
  gh_scene_ray_request ray={.mask=7,.flags=0x534,.segmented=1};
  float impact[4]={0,0,0,1};
  memcpy(ray.from,s->player_prop.flight_previous,12);ray.from[3]=1;
  memcpy(ray.to,view->alias_f0,12);ray.to[3]=1;
  ray.hit_position=impact;
  int hit;
  if(gh_scene_ray_query_00131e10(&s->scene_tables,&ray,&hit,&pc)){
   *frontier=pc?pc:0x131e10;return GH_SERVICES_FRONTIER;
  }
  if(hit){
   float v[3];
   memcpy(v,p+0x610,sizeof v);
   const float length=sqrtf(prop_sum(prop_sum(prop_product(v[0],v[0]),
    prop_product(v[1],v[1])),prop_product(v[2],v[2])));
   if(!isfinite(length))return GH_SERVICES_ASSET;
   volatile float scale=length>0?1/length:0;
   for(unsigned i=0;i<3;i++)impact[i]=prop_sum(impact[i],prop_product(prop_product(-v[i],scale),.3f));
   memcpy(view->alias_f0,impact,12);
   s->player_prop.move_stage=7;
  }else{
   const gh_title_player_view *touched_actor=NULL;
   if(gh_hitarea_object_last_hit_actor(&s->player_prop.give,&touched_actor))
    return GH_SERVICES_ASSET;
   const float power=prop_product(prop_number(s->title_player.view.bytes+0x600),30);
   if(!isfinite(power)||power<-2147483648.0f||power>=2147483648.0f)return GH_SERVICES_ASSET;
   if(gh_hitarea_object_set_node_scalar_001fd7b0(&s->player_prop.give,
      (int16_t)s->player_prop.shape_index,0x3f800000,frontier)||
      gh_hitarea_object_set_active_001fd668(&s->player_prop.give,1,frontier))return GH_SERVICES_ASSET;
   give[0x46]=0x36;give[0x47]=0;
   w32(give+0x4c,(uint32_t)(int32_t)power);
   w32(give+0x40,UINT32_MAX);memset(give+0x44,0,2);
   memcpy(give+0x10,p+0x610,12);
   float life=prop_number(p+0x620);
   if(touched_actor&&life>2)life=2;
   life=prop_sum(life,-rate);
   memcpy(p+0x620,&life,4);
   s->player_prop.move_stage=life<=0?5:2;
  }
 }
 if(s->player_prop.move_stage==5||s->player_prop.move_stage==7){
  const int wall=s->player_prop.move_stage==7;
  const gh_game_services_status status=player_prop_break_001c8dd8(s,view,frontier);
  if(status)return status;
  /* 1C8288 still calls 1C8E50 after lifetime expiry. The wall-hit branch
   * calls 1C8DD8 and returns to the shared 1C7E30 model tail directly. */
  s->player_prop.move_stage=wall?3:2;
 }
 if(s->player_prop.move_stage==2){
  if(!p[0x640]){
   uint32_t pc=0;
   gh_scene_ray_report report={0};
   float point[4]={0,0,0,1};
   gh_scene_ray_request ray={.mask=7,.flags=0,.segmented=1,
    .hit_position=point,.report_limit=1,.native_reports=&report};
   memcpy(ray.from,p+0x490,12);ray.from[3]=1;
   memcpy(ray.to,view->alias_f0,12);ray.to[3]=1;
   int hit;
   if(gh_scene_ray_query_00131e10(&s->scene_tables,&ray,&hit,&pc)){
    *frontier=pc?pc:0x131e10;return GH_SERVICES_FRONTIER;
   }
   if(hit){
    if(!report.attributes||report.attribute_bytes<8)return GH_SERVICES_ASSET;
    if(u32(report.attributes+4)&0x400){
     s->player_prop.move_stage=6;
     *frontier=0x306838;
     return GH_SERVICES_FRONTIER;
    }
   }
  }
  s->player_prop.move_stage=3;
 }
 if(gh_actor_model_update_hierarchy_00149eb8(s->player_prop.model,GH_STATIC_BINARY32_REFERENCE)){
  *frontier=s->player_prop.model->frontier;return GH_SERVICES_ASSET;
 }
 memcpy(p+0x490,view->alias_f0,12);memcpy(p+0x5d0,p+0x100,12);
 if(!s->player_prop.give_created)memset(&s->player_prop,0,sizeof(s->player_prop));
 else s->player_prop.move_stage=0;
 s->update_diagnostic.om_updates_completed++;
 *frontier=0;
 return GH_SERVICES_OK;
}

static gh_object_step_status om_spawn_dropped_item_002923e8(gh_game_services *s,gh_om_box_item_drop *drop,
 const gh_title_player_view *source,uint16_t kind,int reset_pity,uint32_t *frontier)
{
 *frontier=0;
 if(drop->stage==4){*frontier=0x31d000;return GH_OBJSTEP_UNSUPPORTED;}
 gh_om_object *item=drop->create.object;
 if(!drop->stage){
  const gh_scene_placement_status status=gh_om_factory_create_0030fa98(
      &s->om_factory,&drop->create,kind,0xffff,&item);
  if(status){
   *frontier=s->om_factory.frontier?s->om_factory.frontier:0x30fa98;
   return status==GH_PLACE_UNSUPPORTED?GH_OBJSTEP_UNSUPPORTED:GH_OBJSTEP_CALLEE;
  }
  if(reset_pity)memset(s->b.init->enemies+0x534,0,4);
  if(drop->selected_kind)w32(s->b.init->enemies+0x5a8,kind);
  if(!item){drop->stage=4;*frontier=0x31d000;return GH_OBJSTEP_UNSUPPORTED;}
  drop->stage=1;
 }
 if(drop->stage==1){
  int initialized=0;
  const gh_scene_placement_status status=gh_om_factory_initialize_runtime_item_001c0128(
      &s->om_factory,item,&initialized);
  if(status||!initialized){
   *frontier=s->om_factory.frontier?s->om_factory.frontier:0x1c0128;
   return status&&status!=GH_PLACE_UNSUPPORTED?GH_OBJSTEP_CALLEE:GH_OBJSTEP_UNSUPPORTED;
  }
  drop->stage=2;
 }
 gh_om_init_state *init=item->initialization;
 if(drop->stage==2){
  const gh_title_player_view *player=&s->title_player.view;
  if(!player->bytes||player->size<0x10c){*frontier=0x2db6b0;return GH_OBJSTEP_UNSUPPORTED;}
  float position[4];
  memcpy(position,source->alias_f0,sizeof(position));
  fenv_t environment;
  if(fegetenv(&environment)||fesetround(FE_TOWARDZERO)){
   *frontier=0x2929cc;return GH_OBJSTEP_CALLEE;
  }
  position[1]=prop_sum(position[1],.5f);
  if(fesetenv(&environment)){*frontier=0x2929cc;return GH_OBJSTEP_CALLEE;}
  if(gh_om_r101_item_position_001c2630(init,(const uint8_t *)position)){
   *frontier=init->frontier?init->frontier:0x1c2630;return GH_OBJSTEP_UNSUPPORTED;
  }
  memcpy(item->view.bytes+0x100,player->bytes+0x100,12);
  drop->stage=3;
 }
 if(drop->stage==3){
  if(gh_actor_model_update_hierarchy_00149eb8(&init->model,GH_STATIC_BINARY32_REFERENCE)){
   *frontier=init->model.frontier?init->model.frontier:0x149eb8;return GH_OBJSTEP_UNSUPPORTED;
  }
  drop->sound=(gh_motion_sound_request){.model=&init->model,.actor=&item->view,.bank=0,.program=0xfa};
  drop->stage=5;
 }
 if(drop->stage==5){
  uint32_t handle=0;
  gh_actor_motion_status status=om_break_sound(s,&drop->sound,&handle,frontier);
  if(status)return status==GH_ACTOR_MOTION_UNSUPPORTED?GH_OBJSTEP_UNSUPPORTED:GH_OBJSTEP_CALLEE;
  drop->stage=6;
 }
 return GH_OBJSTEP_OK;
}

/* 28645C uses the explicit 03C8 class from cEmA2's original Init draw.
 * 1C2280 then attaches it to the live owner, followed by the placement mark. */
static gh_actor_motion_status heavy_offer_item_0028645c(void *context,gh_om_box_item_drop *drop,
 const gh_actor_model *owner,uint32_t handle,uint32_t *frontier)
{
 gh_game_services *s=context;
 const gh_title_player_view *actor=&owner->root;
 const gh_room_state *room=&s->room_state;
 const uint8_t *p=actor->bytes;
 if(s->b.work->room!=0x100||!p||actor->size<0x642||!actor->alias_f0||!handle||
    actor->original_vtable214!=0x448ca0||(u32(p+0x640)&0xffffu)!=6||u32(p+0x560)!=0x3c8||
    !room->active||room->size<12u||12u+24u*room->active[8]>room->size){
  *frontier=0x2923e8;return GH_ACTOR_MOTION_UNSUPPORTED;
 }
 const gh_object_step_status status=om_spawn_dropped_item_002923e8(s,drop,actor,0x3c8,0,frontier);
 gh_om_object *item=drop->create.object;
 if(status&&(drop->stage!=4||item))
  return status==GH_OBJSTEP_UNSUPPORTED?GH_ACTOR_MOTION_UNSUPPORTED:GH_ACTOR_MOTION_CALLEE;
 if(item){
  gh_om_init_state *init=item->initialization;
  uint8_t *data=item->view.bytes;
  w32(data+0x630,handle);w32(data+0x634,10);
  memset(data+0x640,0,12);memset(data+0x650,0,12);
  data[0x2f6]=2;
  init->item_attachment_owner=owner;
  if(gh_om_r101_item_follow_001c2378(init)||
     gh_actor_model_update_hierarchy_00149eb8(&init->model,GH_STATIC_BINARY32_REFERENCE)){
   *frontier=init->frontier?init->frontier:0x149eb8;return GH_ACTOR_MOTION_UNSUPPORTED;
  }
 }
 for(unsigned row=0;row<room->active[8];++row){
  uint8_t *placement=room->active+12u+24u*row;
  if(placement[0x13]==6){placement[0x14]=1;break;}
 }
 *frontier=0;
 return GH_ACTOR_MOTION_OK;
}

static gh_object_step_status om_box_drop_item_002923e8(void *context,gh_om_box_item_drop *drop,
 const gh_title_player_view *source,uint32_t *frontier)
{
 gh_game_services *s=context;
 if(!s||!drop||!source||!source->alias_f0||!s->b.work||!s->b.init||!frontier)
  return GH_OBJSTEP_ARGUMENT;
 const unsigned room=s->b.work->room;
 const int box=source->original_vtable214==0x4238d8&&(room==0x100||room==0x101||room==0x103);
 const int arena_fixture=source->original_vtable214==0x4267f8&&room==0x103;
 const int west_prop=room==0x102&&
     (source->original_vtable214==0x425c78||source->original_vtable214==0x4237e8);
 const int gong=room==0x104&&source->original_vtable214==0x425c78&&
     source->bytes&&source->size>=0x300&&(u32(source->bytes+0x2fc)>>16)==0x332;
 if(!box&&!arena_fixture&&!west_prop&&!gong)return GH_OBJSTEP_ARGUMENT;
 const uint16_t kind=drop->selected_kind?drop->selected_kind:0x3d6;
 if(drop->selected_kind&&!gong)return GH_OBJSTEP_ARGUMENT;
 return om_spawn_dropped_item_002923e8(s,drop,source,kind,!drop->selected_kind,frontier);
}

static int r100_west_death_item_enemy(const gh_game_services *s,const gh_enemy_actor *enemy,
 uint16_t kind)
{
 if(s->b.work->room!=0x100||!enemy)return 0;
 const gh_title_player_view *view=&enemy->entry.object.view;
 const uint8_t *p=view->bytes;
 if(!p||view->size<0x16c8||view->original_vtable214!=0x4479c8)return 0;
 const uint32_t dependency=u32(p+0x564),home=u32(p+0x16c4);
 const unsigned stamp=u32(p+0x640)&0xffffu,appearance=p[0x1650];
 if(!appearance&&((stamp==19&&dependency==0x203)||(stamp==20&&dependency==0x200)))
  return kind==0x3e0||kind==0x3e1||kind==0x3c4;
 if(kind!=0x3c4)return 0;
 if(appearance==2&&home==6)
  return (stamp==3&&dependency==0x200)||(stamp==4&&dependency==0x202)||(stamp==5&&dependency==0x203);
 if(appearance)return 0;
 return (stamp==29&&dependency==0x202)||(stamp==30&&dependency==0x203)||
    (stamp==31&&dependency==0x200)||(stamp==32&&dependency==0x202);
}

static gh_actor_motion_status enemy_drop_item_002923e8(void *context,gh_om_box_item_drop *drop,
 const gh_enemy_actor *enemy,uint16_t kind,uint32_t *frontier)
{
 gh_game_services *s=context;
 if(!s||!drop||!s->b.work||!s->b.init||!frontier)return GH_ACTOR_MOTION_ARGUMENT;
 const int arena=enemy&&(r103_grab_attacker(s,enemy,0x200)||r103_grab_attacker(s,enemy,0x203)||
     r103_grab_attacker(s,enemy,0x20a));
 const int east=(kind==0x3c4||kind==0x3df)&&r102_east_grab_attacker(s,enemy);
 const int boss_drop=(kind==0x3d1&&r103_grab_attacker(s,enemy,0x214))||
     (kind==0x3d7&&r103_grab_attacker(s,enemy,0x215));
 const uint8_t *actor=enemy?enemy->entry.object.view.bytes:NULL;
 const int key_drop=kind==0x3e3&&s->b.work->room==0x101&&actor&&enemy->entry.object.view.size>=0x1651&&
     ((actor[0x640]==9&&actor[0x641]==0&&u32(actor+0x564)==0x227&&actor[0x1650]==4)||
      (actor[0x640]==7&&actor[0x641]==0&&u32(actor+0x564)==0x20a&&actor[0x1650]==1));
 const int side_currency=kind==0x3c4&&s->b.work->room==0x101&&actor&&enemy->entry.object.view.size>=0x1651&&
     actor[0x1650]==6&&actor[0x641]==0&&
     ((actor[0x640]==0x16&&u32(actor+0x564)==0x200)||
      (actor[0x640]==0x20&&u32(actor+0x564)==0x203));
 const int side_grenade_loot=(kind==0x3c4||kind==0x3e0)&&s->b.work->room==0x101&&actor&&
     enemy->entry.object.view.size>0x17b7&&actor[0x1650]==6&&actor[0x641]==0&&
     u32(actor+0x564)==0x20f&&u32(actor+0x17b4)==50&&
     (actor[0x640]==0x1a||actor[0x640]==0x1b||actor[0x640]==0x22||actor[0x640]==0x23);
 const int wave4_loot=(kind==0x3c3||kind==0x3df)&&s->b.work->room==0x101&&actor&&
     enemy->entry.object.view.size>=0x1651&&actor[0x1650]==6&&(u32(actor+0x2fc)>>16)==0x202&&
     u32(actor+0x564)==0x227&&((u32(actor+0x640)&0xffffu)==30||(u32(actor+0x640)&0xffffu)==38);
 if(!boss_drop&&!key_drop&&!east&&!side_currency&&!side_grenade_loot&&!wave4_loot&&!r100_west_death_item_enemy(s,enemy,kind)&&!((kind==0x3c4&&(arena||r102_em00_grab_attacker(s,enemy)))||
      (kind==0x3d0&&arena)||(kind==0x3c8&&r103_grab_attacker(s,enemy,0x20a)))){
  *frontier=0x30fa98;return GH_ACTOR_MOTION_UNSUPPORTED;
 }
 const gh_title_player_view *source=&enemy->entry.object.view;
 if(!source->alias_f0||source->bytes[0x2f4]!=2||source->bytes[0x2f5]!=2){
  *frontier=0x2929a8;return GH_ACTOR_MOTION_UNSUPPORTED;
 }
 const gh_object_step_status status=om_spawn_dropped_item_002923e8(s,drop,source,kind,!boss_drop&&!key_drop&&!wave4_loot,frontier);
 return !status?GH_ACTOR_MOTION_OK:status==GH_OBJSTEP_UNSUPPORTED?GH_ACTOR_MOTION_UNSUPPORTED:GH_ACTOR_MOTION_CALLEE;
}

static gh_object_step_status om_attack_boost_0012c708(gh_game_services *s,gh_om_item_pickup *pickup,
 const gh_title_player_view *item,uint32_t *frontier)
{
 const gh_title_player_view *player=&s->title_player.view;
 if(!player->bytes||player->size<0x15f8||!s->player_model.ready)return GH_OBJSTEP_ARGUMENT;
 if(!pickup->stage){
  w32(player->bytes+0x5fc,600);
  pickup->stage=1;
 }
 if(pickup->stage==1){
  const gh_player_frame_effect_request effect={.actor=player,.kind=0x4e,.mode=5,.selector=-1};
  const gh_player_frame_status status=player_frame_effect_create(s,&effect,frontier);
  if(status)return status==GH_PLAYER_FRAME_UNSUPPORTED?GH_OBJSTEP_UNSUPPORTED:GH_OBJSTEP_CALLEE;
  const int devil=(u32(player->bytes+0x15f4)&0x1000u)!=0;
  pickup->sound=(gh_motion_sound_request){.model=&s->player_model,.actor=player,
      .bank=devil?2:0,.program=devil?200:0xdf};
  pickup->stage=2;
 }
 if(pickup->stage==2){
  uint32_t handle;
  const gh_actor_motion_status status=om_break_sound(s,&pickup->sound,&handle,frontier);
  if(status)return status==GH_ACTOR_MOTION_UNSUPPORTED?GH_OBJSTEP_UNSUPPORTED:GH_OBJSTEP_CALLEE;
  const gh_r100_om_view item_identity={.bytes=item->bytes,.size=item->size,.identity=item};
  gh_actor_model *model=room_r102_om_model(s,&item_identity);
  if(!model)return GH_OBJSTEP_ARGUMENT;
  pickup->sound=(gh_motion_sound_request){.model=model,.actor=item,.bank=0,.program=0xfc};
  pickup->stage=3;
 }
 if(pickup->stage==3){
  uint32_t handle;
  const gh_actor_motion_status status=om_break_sound(s,&pickup->sound,&handle,frontier);
  if(status)return status==GH_ACTOR_MOTION_UNSUPPORTED?GH_OBJSTEP_UNSUPPORTED:GH_OBJSTEP_CALLEE;
  w32((uint8_t *)s->b.work+0x610,0x3df);
  pickup->stage=4;
 }
 return GH_OBJSTEP_OK;
}

static gh_object_step_status om_item_award_001c2408(void *context,gh_om_item_pickup *pickup,
 const gh_title_player_view *item,uint32_t *frontier)
{
 gh_game_services *s=context;
 if(!s||!pickup||!item||!item->bytes||item->size<0x690||!frontier||
    !s->b.work||!s->om_factory.bindings.initializer||item->original_vtable214!=0x429b88)
  return GH_OBJSTEP_ARGUMENT;
 const int16_t room=s->b.work->room;
 const uint32_t kind=u32(item->bytes+0x2fc)>>16;
 const int full_heal=room==0x103&&kind==0x3d0;
 const int west_held_heal=room==0x100&&item->bytes[0x660]==1&&u32(item->bytes+0x634)==10;
 const int partial_heal=kind==0x3c8&&(room==0x103||west_held_heal);
 const int half_heal=room==0x103&&kind==0x3d1;
 const int boss_currency=room==0x103&&kind==0x3d7;
 const int heal=full_heal||partial_heal||half_heal;
 const int roulette_orb=room==0x101&&kind==0x3e0;
 const int key_card=room==0x101&&kind==0x3e3;
 const int placed_currency=room==0x101&&(kind==0x3d3||kind==0x3d4);
 const int large_currency=room==0x101&&kind==0x3c3;
 if((room==0x101||room==0x102)&&kind==0x3df)return om_attack_boost_0012c708(s,pickup,item,frontier);
 if(!(((room>=0x100&&room<=0x103)&&kind==0x3d6)||
      (room>=0x100&&room<=0x103&&kind==0x3c4)||heal||boss_currency||key_card||placed_currency||large_currency||roulette_orb))return GH_OBJSTEP_ARGUMENT;
 *frontier=0;
 const gh_title_player_view *player=&s->title_player.view;
 uint8_t *work=(uint8_t *)s->b.work;
 const gh_om_initializer_bindings *initializer=s->om_factory.bindings.initializer;
 uint8_t *save=initializer->save.bytes;
 if(!player->bytes||player->size<0x15f8||!s->player_model.ready||
    !save||initializer->save.size<GH_CORE_SAVE_BYTES){
  *frontier=0x1c2408;return GH_OBJSTEP_UNSUPPORTED;
 }
 if(!pickup->stage){
  uint16_t bonus;
  memcpy(&bonus,save+0x10,2);
  const int premium=bonus||(u32(work+0x59c)&0x40000u);
  const uint32_t hp_pair=u32(player->bytes+0x548);
  const uint16_t maximum=(uint16_t)hp_pair, hp=(uint16_t)(hp_pair>>16);
  int roulette_full=0;
  unsigned roulette_slots=0;
  if(roulette_orb){
   /* 1C0678 checks 1FABB0 before 1FAEE0, then calls 1FAB50 with 1. */
   unsigned stock=0;
   for(unsigned i=0;i<6&&i<save[0x156];++i)
    stock+=save[0xa8+i]!=0;
   if(u32(work+0x598)&0x08000000u)save[0x156]=6;
   roulette_slots=save[0x156]>6?6:save[0x156];
   roulette_full=save[0x156]<=stock;
  }
  if(key_card){
   /* Original 1FA968 adds one carried key card and clamps the signed count to 0..9. */
   int32_t count;
   memcpy(&count,save+0x68,sizeof(count));
   count=count>=8?9:count<-1?0:count+1;
   memcpy(save+0x68,&count,sizeof(count));
  }else if(roulette_orb?roulette_full:!heal||hp==maximum){
   const int32_t amount=(premium?2:1)*(roulette_orb?300:full_heal||boss_currency?1000:(half_heal||large_currency)?500:200);
   int32_t count,money;
   memcpy(&count,save+0x24,4);memcpy(&money,save+0x20,4);
   if(count<0){*frontier=0x1fa750;return GH_OBJSTEP_UNSUPPORTED;}
   if(count<16){
    memcpy(save+0x28+4*(size_t)count,&amount,4);
    ++count;
    memcpy(save+0x24,&count,4);
   }
   const int64_t total=(int64_t)money+amount;
   const int32_t changed=(u32(work+0x594)&0x2000000u)||total>999999?999999:total<0?0:(int32_t)total;
   memcpy(save+0x20,&changed,4);
  }
  for(unsigned i=0;i<roulette_slots;++i){
   if(save[0xa8+i])continue;
   save[0xa8+i]=1;
   break;
  }
  /* 1C0E50 replenishes current HP and clears stagger. */
  if(heal){
   /* 1C0678 truncates the selected fraction before adding it. */
   const volatile float partial=(float)maximum*(half_heal?0.5f:0.3f);
   const unsigned healed=full_heal?maximum:(unsigned)hp+(unsigned)partial;
   const uint16_t health=(uint16_t)(healed>maximum?maximum:healed);
   memcpy(player->bytes+0x54a,&health,2);
   w32(player->bytes+0x5f4,0);
  }
  /* Original 13E2F8 returns without drawing. The live HUD consumes +24/+28. */
  const int devil=(u32(player->bytes+0x15f4)&0x1000u)!=0;
  const int16_t program=full_heal||placed_currency?0xdc:partial_heal?0xd2:half_heal?0xdf:(boss_currency||large_currency)?0xde:key_card?0xd3:0xd4;
  pickup->sound=(gh_motion_sound_request){.model=&s->player_model,.actor=player,
      .bank=devil?2:0,.program=devil?200:program};
  pickup->stage=1;
 }
 if(pickup->stage==1){
  uint32_t voice=0;
  gh_actor_motion_status status=om_break_sound(s,&pickup->sound,&voice,frontier);
  if(status)return status==GH_ACTOR_MOTION_UNSUPPORTED?GH_OBJSTEP_UNSUPPORTED:GH_OBJSTEP_CALLEE;
  memcpy(work+0x610,&kind,4);
  pickup->stage=2;
 }
 return GH_OBJSTEP_OK;
}

static gh_game_services_status player_prop_flight_001c8288(gh_game_services *s,
 const gh_title_player_view *view,uint32_t *frontier)
{
 fenv_t environment;
 *frontier=0x1c8288;
 if(feholdexcept(&environment))return GH_SERVICES_ASSET;
 if(fesetround(FE_TONEAREST)){
  (void)fesetenv(&environment);
  return GH_SERVICES_ASSET;
 }
 const gh_game_services_status status=player_prop_flight_body(s,view,frontier);
 if(fesetenv(&environment))return GH_SERVICES_ASSET;
 return status;
}

static gh_game_services_status player_prop_move_001c8020(gh_game_services *s,uint32_t handle,
 uint32_t entry,const gh_title_player_view *view,uint32_t *frontier)
{
 *frontier=0x1c8020;
 if(entry!=0x18c370||!view||handle!=s->player_prop.handle||!s->player_prop.object||
    s->player_prop.stage!=7||view->bytes!=s->player_prop.object->view.bytes)
  return GH_SERVICES_FRONTIER;
 uint8_t *p=view->bytes;
 const uint8_t *player=s->title_player.view.bytes;
 if((u32((const uint8_t *)s->b.work+0x5d8)&0x20000000u)||(u32(p+0x250)&0x80000000u)){
  *frontier=0;return GH_SERVICES_OK;
 }
 if(p[0x2f4]||p[0x2f5]){*frontier=0x1c7f80;return GH_SERVICES_FRONTIER;}
 const gh_actor_part *part;
 if(!player||!s->title_player.view.alias_f0||!view->alias_f0||
    gh_actor_model_part_at(s->player_prop.model,0,&part)||!part->alias_d0)
  return GH_SERVICES_ASSET;
 if(!s->player_prop.move_stage){
  w32(p+0x5b4,0);
  memset(s->player_prop.give.bytes+0x46,0,2);w32(s->player_prop.give.bytes+0x4c,0);
  if(gh_hitarea_object_set_active_001fd668(&s->player_prop.give,0,frontier))return GH_SERVICES_ASSET;
  memcpy(view->alias_f0,s->title_player.view.alias_f0,12);memcpy(p+0x100,player+0x100,12);
  w32(p+0x5a8,u32(player+0x5a8));w32(p+0x444,u32(player+0x5a8)^((p[0x434]&4u)?0x80000000u:0));
  s->player_prop.move_stage=1;
 }
 if(s->player_prop.move_stage==1){
  int ended;
  const gh_actor_motion_status status=gh_actor_motion_step_00157c18(&s->player_prop.motion,
     *s->b.rate,GH_STATIC_BINARY32_REFERENCE,&ended,frontier);
  if(status)return status==GH_ACTOR_MOTION_UNSUPPORTED?GH_SERVICES_FRONTIER:GH_SERVICES_ASSET;
  s->player_prop.move_stage=2;
 }
 if(s->player_prop.move_stage==2){
  float fraction;
  memcpy(&fraction,p+0x624,4);
  if(!isfinite(fraction)||fraction<0||fraction>1)return GH_SERVICES_ASSET;
  const float factors[3]={-.1f,.1f,.2f}; /* 3BDE70, 74, 78 */
  for(unsigned i=0;i<3;i++){
   float value;memcpy(&value,part->alias_d0+i*4,4);
   const volatile float delta=factors[i]*fraction;
   value+=delta;memcpy(part->alias_d0+i*4,&value,4);
  }
  const volatile float increase=fraction+.05f;
  fraction=increase>1?1:increase;memcpy(p+0x624,&fraction,4);
  s->player_prop.move_stage=3;
 }
 if(gh_actor_model_update_hierarchy_00149eb8(s->player_prop.model,GH_STATIC_BINARY32_REFERENCE)){
  *frontier=s->player_prop.model->frontier;return GH_SERVICES_ASSET;
 }
 memcpy(p+0x490,view->alias_f0,12);memcpy(p+0x5d0,p+0x100,12);
 s->player_prop.move_stage=0;s->update_diagnostic.om_updates_completed++;
 *frontier=0;
 return GH_SERVICES_OK;
}

static gh_object_step_status om_remove_sound(void *context,gh_actor_model *model,uint32_t *frontier) {
 gh_effect_status status=detach_model_effect_sound(context,model,frontier);
 return status?GH_OBJSTEP_UNSUPPORTED:GH_OBJSTEP_OK;
}

static gh_actor_motion_status release_stamp19_enemy(void *context,const gh_enemy_actor *enemy,uint32_t *frontier)
{
 gh_game_services *s=context;
 uint32_t handle=0;
 if(!enemy||!frontier||gh_enemy_ems_handle_for_actor(s->enemies,&enemy->entry.object,&handle)||!handle){
  if(frontier)*frontier=0x290770;
  return GH_ACTOR_MOTION_UNSUPPORTED;
 }
 int removed=0;
 gh_actor_motion_status status=gh_enemy_ems_remove_00290770(s->enemies,&s->enemy_removal,
  handle,s->b.voices,&removed,frontier);
 if(!status)s->enemy_removal=(gh_enemy_manager_remove){0};
 return status;
}

gh_game_devil_camera *gh_game_services_active_devil_camera(gh_game_services *s)
{
 return s&&s->b.init->active_camera==&s->devil_camera.identity?&s->devil_camera:NULL;
}

static gh_game_pl_camera *enemy_scene_camera(gh_game_services *s)
{
 gh_game_motion_camera *motion=s->b.motion_camera;
 if(motion&&s->b.init->active_camera==&motion->identity)return &motion->camera;
 gh_game_devil_camera *devil=gh_game_services_active_devil_camera(s);
 return devil?&devil->camera:&s->camera;
}

static gh_actor_motion_status r105_scenery_sound(void *context,const gh_motion_sound_request *request,
 uint32_t *handle,uint32_t *frontier)
{
 gh_game_services *s=context;
 if(actor_sound_prepare(s,&s->enemy_sound,frontier))return GH_ACTOR_MOTION_CALLEE;
 if(!s->enemy_sound.bound){*frontier=0x2cba48;return GH_ACTOR_MOTION_UNSUPPORTED;}
 return gh_actor_attached_sound_motion(&s->enemy_sound,request,handle,frontier);
}

static void r105_scenery_task_0028e858(void *context,int32_t argument)
{
 gh_game_services *s=context;
 uint32_t pc=0;
 gh_enemy_ems_live live;
 s->r105_scenery_task_status=GH_SCENE_STEP_CALLEE;
 s->r105_scenery_task_frontier=0x28e858;
 if(argument||gh_enemy_ems_live_at(s->enemies,s->r105_scenery_handle,&live)||
    !live.actor||live.actor!=s->r105_scenery_action.actor||!live.model)return;
 uint8_t *actor=live.actor->view.bytes;
 /* The original +64 method for this concrete class is 2A9350(0). */
 w32(actor+0x250,u32(actor+0x250)&~0x8000u);
 w32(actor+0x5a0,u32(actor+0x5a0)&~8u);
 gh_game_event_status player=room_soft_end_player(s,&s->title_player.view,&pc);
 if(player){
  s->r105_scenery_task_frontier=pc?pc:0x12c7d0;
  if(player==GH_EVENT_UNSUPPORTED)s->r105_scenery_task_status=GH_SCENE_STEP_UNSUPPORTED;
  return;
 }
 s->r105_scenery_task_sound=(gh_motion_sound_request){.model=live.model,.actor=&live.actor->view,.bank=1,.program=0};
 uint32_t voice;
 gh_actor_motion_status sound=r105_scenery_sound(s,&s->r105_scenery_task_sound,&voice,&pc);
 if(sound){
  s->r105_scenery_task_frontier=pc?pc:0x2cba48;
  if(sound==GH_ACTOR_MOTION_UNSUPPORTED)s->r105_scenery_task_status=GH_SCENE_STEP_UNSUPPORTED;
  return;
 }
 /* 2C40C8(3004,NULL,0,0,1) selects the synchronous, text-only path. */
 const gh_scene_scenario_view scenario={s->scenario_scalar,sizeof(s->scenario_scalar),&s->scenario};
 scenario.bytes[0xd8]=0x04;
 scenario.bytes[0xd9]=0x30;
 w32(scenario.bytes+0xdc,0);
 scenario.bytes[0xe4]=0;
 scenario.bytes[0xe5]=0;
 gh_scene_ui_status message=gh_scenario_message_open_002c42e0(scenario,&s->scene_ui,s->b.work,0x3004,&pc);
 if(message){
  s->r105_scenery_task_frontier=pc?pc:0x2c42e0;
  if(message==GH_SCENE_UI_UNSUPPORTED)s->r105_scenery_task_status=GH_SCENE_STEP_UNSUPPORTED;
  return;
 }
 gh_soft_event_start nested={0};
 const gh_soft_event_bindings soft={.scenario=scenario,.work=s->b.work,
    .player_abort_00126770=room_soft_player_abort,.context=s};
 if(gh_scenario_start_soft_event_002c39b0(&nested,&soft,2)){
  s->r105_scenery_task_frontier=nested.frontier?nested.frontier:0x2c39b0;
  return;
 }
 int active;
 for(;;){
  message=gh_scenario_message_active_002c4350(scenario,&s->scene_ui,&active,&pc);
  if(message){s->r105_scenery_task_frontier=pc?pc:0x2c4350;return;}
  if(!active)break;
  s->r105_scenery_task_status=GH_SCENE_STEP_OK;
  if(gh_task_threads_sleep(s->scenario_threads,1)){
   s->r105_scenery_task_frontier=0x2d5aa0;
   s->r105_scenery_task_status=GH_SCENE_STEP_CALLEE;
   return;
  }
  s->r105_scenery_task_status=GH_SCENE_STEP_CALLEE;
 }
 message=gh_scenario_message_close_002c4390(scenario,&s->scene_ui,&pc);
 if(message){s->r105_scenery_task_frontier=pc?pc:0x2c4390;return;}
 gh_soft_event_end end_nested={0};
 gh_r100_script_status end=room_r100_end_live_soft(s,&end_nested,&pc);
 if(end){
  s->r105_scenery_task_frontier=pc?pc:0x2c3c10;
  if(end==GH_R100_SCRIPT_UNSUPPORTED)s->r105_scenery_task_status=GH_SCENE_STEP_UNSUPPORTED;
  return;
 }
 if(voice){
  int accepted;
  if(gh_se_voice_request_stop_00375050(s->b.voices,voice,15,&accepted)){
   s->r105_scenery_task_frontier=0x375050;
   return;
  }
 }
 /* The subsequent +BEC load is a different menu owner field. */
 s->r105_scenery_task_frontier=0x28e914;
 s->r105_scenery_task_status=GH_SCENE_STEP_UNSUPPORTED;
}

gh_scene_at_status gh_game_services_r105_jump_offer(void *context,
 const gh_scene_at_task_request *request,uint32_t *frontier)
{
 gh_game_services *s=context;
 return s?gh_r105_jump_offer_001f7858(&s->r105,request,frontier):GH_AT_ARGUMENT;
}

gh_action_step_status gh_game_services_r105_scenery_action(void *context,uint32_t entry,
 void *argument,uint32_t *frontier)
{
 gh_game_services *s=context;
 if(!s||!frontier)return GH_ACTION_STEP_ARGUMENT;
 *frontier=entry;
 if(s->b.work->room!=0x105||!argument)return GH_ACTION_STEP_UNSUPPORTED;
 if(entry==0x2c2138)return gh_r105_jump_action_002c2138(&s->r105,argument,frontier);
 if(entry!=0x28e7c8)return GH_ACTION_STEP_UNSUPPORTED;
 uint32_t handle;
 gh_enemy_ems_live live;
 if(gh_enemy_ems_handle_for_actor(s->enemies,argument,&handle)||!handle||
    gh_enemy_ems_live_at(s->enemies,handle,&live)||live.actor!=argument||
    live.physical_row!=8||live.actor->view.original_vtable214!=0x44a528)
  return GH_ACTION_STEP_ARGUMENT;
 if(s->r105_scenery_handle&&s->r105_scenery_handle!=handle)return GH_ACTION_STEP_ARGUMENT;
 s->r105_scenery_handle=handle;
 const gh_soft_event_bindings bindings={
  .scenario={s->scenario_scalar,sizeof(s->scenario_scalar),&s->scenario},.work=s->b.work,
  .player_abort_00126770=room_soft_player_abort,.context=s};
 gh_soft_event_status status=gh_enemy2ad_action_0028e7c8(&s->r105_scenery_action,live.actor,
  &bindings,s->scenario_threads,r105_scenery_task_0028e858,s,frontier);
 return !status?GH_ACTION_STEP_OK:status==GH_SOFT_EVENT_UNSUPPORTED?GH_ACTION_STEP_UNSUPPORTED:GH_ACTION_STEP_CALLEE;
}

static gh_actor_motion_status spawn_stamp8_devil(void *context,const gh_enemy_actor *parent,
 const gh_enemy_request *request,uint32_t *frontier)
{
 gh_game_services *s=context;
 const gh_effect_create_bindings effects={.storage=player_effect_storage(s),
     .work=s->b.work,.random=&s->b.init->random,
     .atan_coeff=s->b.camera_envelope?s->b.camera_envelope->atan_coeff:NULL};
 return gh_enemy_ems_spawn_devil_002905d0(s->enemies,parent,request,&effects,
     s->enemy_sound.bound?gh_actor_attached_sound_motion:NULL,
     s->enemy_sound.bound?&s->enemy_sound:NULL,frontier);
}

gh_game_services_status gh_game_services_move_actor_with_render(gh_game_services*s,uint32_t handle,
 uint32_t entry,int16_t delta,const gh_title_player_view*v,const gh_boot_work*work,
 const gh_game_actor_render_provider*provider,uint32_t*f){
 if(f)*f=0;if(!s||!v||!f||work!=s->b.work)return GH_SERVICES_ARGUMENT;
 s->update_diagnostic.last_move_handle=handle;s->update_diagnostic.last_move_entry=entry;
 gh_services_esp14_slot *effect_actor=model_effect_actor(s,handle);
 if(effect_actor){
  gh_esp14_model *child=&effect_actor->instance.child;
  if(entry!=0x2b8850||delta||v->bytes!=child->object.view.bytes||v->size!=child->object.view.size){
   *f=entry;return GH_SERVICES_FRONTIER;
  }
  gh_effect_status status=gh_esp14_model_move_002b8850(child,work,GH_STATIC_BINARY32_REFERENCE,f);
  return !status?GH_SERVICES_OK:status==GH_EFFECT_UNSUPPORTED?GH_SERVICES_FRONTIER:GH_SERVICES_ASSET;
 }
 gh_event_actor_factory *event_actor=event_actor_for_handle(s,handle);
 if(event_actor){
  gh_title_player_view registered;
  if(delta){*f=entry;return GH_SERVICES_FRONTIER;}
  if(gh_event_actor_factory_view(event_actor,handle,&registered)||v->bytes!=registered.bytes||
     v->size!=registered.size||v->alias_f0!=registered.alias_f0){*f=entry;return GH_SERVICES_ASSET;}
  if(entry==0x27a320||entry==0x27ae10){
   gh_event_actor_extra *extra=s->event_actor_extra;
   *f=entry;
   if(!extra)return GH_SERVICES_ASSET;
   const unsigned accessory=entry==0x27ae10?1:event_actor==&extra->ev04_accessory[2]?2:0;
   if(event_actor!=&extra->ev04_accessory[accessory])return GH_SERVICES_ASSET;
   gh_cloth_step_bindings cloth={.templates=s->b.cloth_templates,.globals=s->b.cloth_globals,
    .random=&s->b.init->random,.actors=&s->scene_tables,
    .matrix_policy=GH_STATIC_BINARY32_REFERENCE,.random_policy=GH_FRAME_CHOP_REFERENCE};
   gh_event_pendulum_call *call=&s->event_accessory_cloth[accessory];
   gh_game_event_status status=gh_game_event_accessory_move_0027a320(call,event_actor,
    &extra->ev04_accessory_springs[accessory],work,&cloth,s->b.player_shared_vector,f);
   if(!status)memset(call,0,sizeof(*call));
   return !status?GH_SERVICES_OK:status==GH_EVENT_UNSUPPORTED?GH_SERVICES_FRONTIER:GH_SERVICES_ASSET;
  }
  if(entry!=0x2b6930){*f=entry;return GH_SERVICES_FRONTIER;}
  const size_t capacity=sizeof(s->event_actor_move)/sizeof(*s->event_actor_move);
  const int accessory=s->event_actor_extra&&event_actor==&s->event_actor_extra->olivia_accessory;
  const size_t index=accessory?capacity-1:event_actor->actor_index;
  if(!accessory&&index>=capacity-1){*f=entry;return GH_SERVICES_ASSET;}
  gh_services_event_move_shadow shadow={s,provider,&s->event_actor_move[index].shadow};
  gh_game_event_status status=gh_game_event_simple_move_with_tails_002b6930(&s->event_actor_move[index].move,
    event_actor,s->b.player_shared_vector,provider?event_move_shadow:NULL,&shadow,event_move_tail,s,f);
  if(!status){
   memset(&s->event_actor_move[index].shadow,0,sizeof(s->event_actor_move[index].shadow));
   memset(&s->event_actor_move[index].pendulum,0,sizeof(s->event_actor_move[index].pendulum));
  }
  return !status?GH_SERVICES_OK:status==GH_EVENT_UNSUPPORTED?GH_SERVICES_FRONTIER:GH_SERVICES_ASSET;
 }
 gh_enemy_ems_live enemy;
 gh_scene_placement_status ee=gh_enemy_ems_live_at(s->enemies,handle,&enemy);
 if(ee){*f=entry;return enemy_place_error(ee);}
 if(enemy.actor){
  if(v->bytes!=enemy.actor->view.bytes||v->size!=enemy.actor->view.size||v->alias_f0!=enemy.actor->view.alias_f0){
   *f=entry;return GH_SERVICES_ASSET;
  }
  if(delta){*f=entry;return GH_SERVICES_FRONTIER;}
  if(enemy.devil){
   if(entry!=0x208be0){*f=entry;return GH_SERVICES_FRONTIER;}
   s->enemy_opacity=(gh_object_opacity_environment){s->b.init,enemy_scene_camera(s),&s->title_player.view,1};
   gh_actor_motion_status status=gh_enemy_ems_devil_step_00208be0(s->enemies,handle,&s->devil_camera,
       s->b.camera_follow,&s->enemy_opacity,&s->step.nearcheck,
       &(gh_effect_create_bindings){.storage=player_effect_storage(s),.work=work,
           .random=&s->b.init->random,.model=enemy.model,.atan_coeff=s->b.camera_envelope?s->b.camera_envelope->atan_coeff:NULL},
       &s->title_player.view,s->b.boot_elf,s->b.boot_elf_bytes,f);
   return !status?GH_SERVICES_OK:status==GH_ACTOR_MOTION_UNSUPPORTED?GH_SERVICES_FRONTIER:GH_SERVICES_ASSET;
  }
  if(entry==0x28e420){
   const gh_object_opacity_environment opacity={s->b.init,enemy_scene_camera(s),&s->title_player.view,1};
   const gh_actor_motion_status status=gh_enemy_ems_move_2ad_0028e420(s->enemies,handle,
       &opacity,s->b.boot_elf,s->b.boot_elf_bytes,s->b.voices,r105_scenery_sound,s,f);
   return !status?GH_SERVICES_OK:status==GH_ACTOR_MOTION_UNSUPPORTED?GH_SERVICES_FRONTIER:GH_SERVICES_ASSET;
  }
  if(enemy.other_move){
   gh_enemy_other_move *move=enemy.other_move;
   if(move->original_returned)*move=(gh_enemy_other_move){0};
   gh_actor_motion_status status;
   if(entry==0x2856c0&&enemy.enemy2a2){
    if(actor_sound_prepare(s,&s->enemy_sound,f))return GH_SERVICES_ASSET;
    s->enemy_opacity=(gh_object_opacity_environment){s->b.init,enemy_scene_camera(s),&s->title_player.view,1};
    if(!s->player_idle_bindings.targets.resolve)s->player_idle_bindings.targets=gh_game_services_scene_targets(s);
    status=gh_enemy_other_move_bind_targets(move,&s->player_idle_bindings.targets);
    if(!status)status=gh_enemy_other_move_bind_2a2_services(move,&s->enemy_opacity,enemy_motion_effect,s,
       s->enemy_sound.bound?gh_actor_attached_sound_motion:NULL,s->enemy_sound.bound?&s->enemy_sound:NULL);
    if(!status)status=gh_enemy_other_move_bind_2a2_offer(move,heavy_offer_item_0028645c,&s->om_factory,handle);
    if(!status)status=gh_enemy_other_move_bind_navigation(move,&s->step.nearcheck);
    if(!status)status=gh_enemy2a2_move_002856c0(move,enemy.enemy2a2,&s->title_player.view);
   }else if(entry==0x1c9860&&enemy.weapon){
    gh_held_weapon_effect_context effects={s,&enemy.actor->view};
    status=gh_enemy363_move_001c9860(move,enemy.weapon,held_weapon_motion_effect,&effects);
   }else{*f=entry;return GH_SERVICES_FRONTIER;}
   if(status||!move->original_returned){
    *f=move->frontier?move->frontier:entry;
    return status==GH_ACTOR_MOTION_UNSUPPORTED?GH_SERVICES_FRONTIER:GH_SERVICES_ASSET;
   }
   return GH_SERVICES_OK;
  }
  if(entry!=0x208be0||!enemy.move202||!enemy.enemy202){*f=entry;return GH_SERVICES_FRONTIER;}
  gh_enemy202_move *move=enemy.move202;
  if(move->original_returned)*move=(gh_enemy202_move){0};
  if(actor_sound_prepare(s,&s->enemy_sound,f))return GH_SERVICES_ASSET;
  const gh_title_player_view *enemy_view=&enemy.enemy202->entry.object.view;
  if(s->enemy_sound.bound&&s->b.work->room==0x100&&enemy_view->size>=0x642&&
     enemy_view->bytes[0x2f4]==1&&enemy_view->bytes[0x2f5]==0x1d&&
     u32(enemy_view->bytes+0x564)==0x200&&(u32(enemy_view->bytes+0x640)&0xffffu)==20){
   gh_voice_status voice=gh_se_voice_bind_external_adx(s->b.voices,s->b.assets->dvd->disc,
       s->b.sound_control,s->b.boot_elf,s->b.boot_elf_bytes);
   if(voice){
    *f=gh_se_voice_frontier(s->b.voices);
    if(!*f)*f=0x2ca34c;
    return voice==GH_VOICE_UNSUPPORTED?GH_SERVICES_FRONTIER:GH_SERVICES_ASSET;
   }
  }
  if(s->enemy_sound.bound){
   enemy.enemy202->motion.motion.b.sound=gh_actor_attached_sound_motion;
   enemy.enemy202->motion.motion.b.sound_context=&s->enemy_sound;
  }
  s->enemy_opacity=(gh_object_opacity_environment){s->b.init,enemy_scene_camera(s),&s->title_player.view,1};
  move->projectile_factory=&s->om_factory;
  move->death_item_spawn=enemy_drop_item_002923e8;
  move->death_item_context=s;
  gh_actor_motion_status e=gh_enemy202_move_bind_navigation(move,&s->step.nearcheck);
  if(!e&&((s->b.work->room==0x103&&enemy.enemy202->entry.request.object_kind==0x204&&
     enemy.enemy202->entry.request.entry_number<2)||
     gh_enemy203_is_r104_entry(&enemy.enemy202->entry.request,s->b.work->room))){
   const gh_cloth_step_bindings cloth={.templates=s->b.cloth_templates,.globals=s->b.cloth_globals,
    .random=&s->b.init->random,.actors=&s->scene_tables,.matrix_policy=GH_STATIC_BINARY32_REFERENCE,
    .random_policy=GH_FRAME_CHOP_REFERENCE};
   e=gh_enemy204_move_bind_springs(move,&cloth,s->b.player_shared_vector);
  }
  const gh_player_target_bindings targets=gh_game_services_scene_targets(s);
  if(!e)e=gh_enemy202_move_bind_targets(move,&targets);
  /* PL00 construction publishes +698 NULL; positive writers require a typed owner. */
  if(!e&&!move->stage&&s->title_player.view.size>=0x69c&&!u32(s->title_player.view.bytes+0x698))
   e=gh_enemy202_move_bind_player_aux_698(move,NULL);
  if(!e)e=gh_enemy202_move_bind_opacity(move,&s->enemy_opacity);
  if(!e)e=gh_enemy202_move_bind_near_sound(move,enemy_ema2_near_sound,s);
  if(!e)e=gh_enemy202_move_bind_room_state(move,&s->room_state);
  if(!e&&((s->b.work->room>=0x101&&s->b.work->room<=0x105)||s->b.work->room==0x200)){
   gh_bundle room;
   if(gh_game_room_pack_bundle(&s->room_pack,&room)){*f=0x31ccf0;return GH_SERVICES_ASSET;}
   e=gh_enemy202_move_bind_scene_jump(move,&s->scene_at,&room);
  }
  if(!e)e=gh_enemy202_move_bind_player_grab(move,player_grab_00129718,s,&s->action_buttons);
  if(!e)e=gh_enemy202_move_bind_death_release(move,release_stamp19_enemy,s);
  if(!e)e=gh_enemy202_move_bind_devil_spawn(move,spawn_stamp8_devil,s);
  if(!e&&s->b.camera_envelope){
   const gh_effect_create_bindings impact={.storage=player_effect_storage(s),
    .work=s->b.work,.random=&s->b.init->random,.atan_coeff=s->b.camera_envelope->atan_coeff};
   e=gh_enemy202_move_bind_impact(move,&impact);
  }
  if(!e)e=gh_enemy202_move_00208be0(move,enemy.enemy202,&s->title_player.view,s->b.boot_elf,s->b.boot_elf_bytes);
  if(e){*f=move->frontier?move->frontier:entry;return e==GH_ACTOR_MOTION_UNSUPPORTED?GH_SERVICES_FRONTIER:GH_SERVICES_ASSET;}
  return move->original_returned?GH_SERVICES_OK:GH_SERVICES_ASSET;
 }
  if(handle==s->player_child.handle&&s->player_child.complete&&v->bytes==s->player_child.object.view.bytes&&v->size==s->player_child.object.view.size){
   if(entry!=0x100530||delta){*f=entry;return GH_SERVICES_FRONTIER;}
   if(s->child_update.original.original_returned)s->child_update=(gh_child_wind_update){0};
   gh_child_move_bindings child={.cloth={.springs=&s->child_tail.springs,.attachment=&s->player_attachment,
     .templates=s->b.cloth_templates,.globals=s->b.cloth_globals,.random=&s->b.init->random,.actors=&s->scene_tables,
     .matrix_policy=GH_STATIC_BINARY32_REFERENCE,.random_policy=GH_FRAME_CHOP_REFERENCE},.work=s->b.work,.work5f4=&s->title_player.view};
   gh_title_player_status e=gh_game_player_child_update_wind_00100530(&s->child_update,&child,s->b.player_shared_vector);*f=s->child_update.original.frontier;
   if(e)return e==GH_PLAYER_UNSUPPORTED?GH_SERVICES_FRONTIER:GH_SERVICES_ASSET;
   if(s->child_update.original.original_returned){s->update_diagnostic.child_updates_completed++;return GH_SERVICES_OK;}return GH_SERVICES_ASSET;
  }
 /* Resolve the existing factory registry; metadata below is a continuation
  * per canonical object, not a second object/model or list. */
 void*om_object=NULL;
 if(!gh_room_object_lookup(&s->pools,handle,GH_ROOM_OBJECT_OM,&s->om_factory,&om_object)){
  size_t index;for(index=0;index<s->om_factory.count;index++)if(om_object==s->om_factory.objects+index)break;
  if(index>=s->om_factory.count||index>=s->om_factory.capacity){*f=entry;return GH_SERVICES_ASSET;}
  if(!s->om_frames){s->om_frames=calloc(s->om_factory.capacity,sizeof(*s->om_frames));if(!s->om_frames){*f=entry;return GH_SERVICES_MEMORY;}s->om_frames_capacity=s->om_factory.capacity;}
  if(s->om_frames_capacity!=s->om_factory.capacity){*f=entry;return GH_SERVICES_BUSY;}
  if(s->player_prop.handle==handle){
   if(s->player_prop.stage==8){
    if(entry!=0x18c370||delta){*f=entry;return GH_SERVICES_FRONTIER;}
    return player_prop_flight_001c8288(s,v,f);
   }
   return delta?GH_SERVICES_FRONTIER:player_prop_move_001c8020(s,handle,entry,v,f);
  }
  gh_om_frame_state*state=s->om_frames+index;
  gh_object_opacity_environment environment={s->b.init,enemy_scene_camera(s),&s->title_player.view,1};
  gh_object_step_status environment_status=gh_om_frame_bind_environment(state,&environment);
  if(environment_status){*f=state->frontier?state->frontier:entry;return GH_SERVICES_ASSET;}
  gh_om_frame_runtime runtime={.init=s->b.init,.canonical_rate=s->b.rate,
   .solids=&s->collision_solids,.context=s,.break_effect=om_break_effect,
   .break_sound=om_break_sound,.break_collision=om_break_collision,.remove_sound=om_remove_sound,.stop_sound=om_stop_sound,.catapult_impact_sound=om_catapult_impact_sound,.ik_globals=s->b.motion_ik_globals,.release_effect=om_motion_effect,
  .throw_fuse_effect=om364_fuse_effect,.throw_blast_effect=om364_blast_effect,.stop_throw_voice=om364_stop_voice,
   .fragment_atan_coeff=s->b.camera_envelope?s->b.camera_envelope->atan_coeff:NULL,
   .action_buttons=&s->action_buttons,.resolve_actor=om_canopy_actor,.scene_tables=&s->scene_tables,
   .drop_item=om_box_drop_item_002923e8,.award_item=om_item_award_001c2408,
   .mount_release=room_r102_mount_release_001a7f58};
  gh_object_step_status runtime_status=gh_om_frame_bind_runtime(state,&runtime);
  if(runtime_status){*f=state->frontier?state->frontier:entry;return GH_SERVICES_ASSET;}
  if(state->complete){gh_object_step_status e=gh_om_frame_next_call(state);if(e){*f=state->frontier?state->frontier:entry;return GH_SERVICES_ASSET;}}
  gh_object_move_request q={.handle=handle,.original_entry=entry,.this_delta=delta,.actor=*v};
  gh_object_step_status e=gh_om_frame_move(state,&s->om_factory,&q,work,GH_STATIC_BINARY32_REFERENCE,f);
  if(e)return e==GH_OBJSTEP_UNSUPPORTED?GH_SERVICES_FRONTIER:GH_SERVICES_ASSET;
  if(!state->complete){*f=state->frontier?state->frontier:entry;return GH_SERVICES_ASSET;}s->update_diagnostic.om_updates_completed++;return GH_SERVICES_OK;
 }
 const gh_title_player_view*canonical=NULL;
 if(gh_game_player_factory_scalar_view(&s->player_factory,handle,&canonical)||canonical->bytes!=v->bytes||canonical->size!=v->size){*f=entry;return GH_SERVICES_FRONTIER;}
 if(entry!=0x108ab0||delta){*f=entry;return GH_SERVICES_FRONTIER;}
 if(!s->player_tail.complete||!s->player_model.ready){*f=0x107f58;return GH_SERVICES_FRONTIER;}
 s->player_frame_effects=player_effect_storage(s);
 /* Original107F58 stage12 published target644=NULL; retain one current slot.
  * Later unimplemented writers are detected by the expected-token check. */
 if(!s->player_target_644.known)s->player_target_644=(gh_player_tail_target_slot){.known=1};
 const gh_title_player_view *grab_attacker=NULL;
 uint8_t grab_attacker_retired=0;
 gh_object_view grab_resource={0};
 if(s->player_frame.bound){
  grab_attacker=s->player_frame.b.grab_attacker;
  grab_attacker_retired=s->player_frame.b.grab_attacker_retired;
  grab_resource=s->player_frame.b.grab_resource;
 }else if(s->player_grab_attacker&&((v->bytes[0x2f4]==1&&
     (v->bytes[0x2f5]==8||v->bytes[0x2f5]==9||v->bytes[0x2f5]==0xa||v->bytes[0x2f5]==0xb||
       v->bytes[0x2f5]==0xf))||
     (v->bytes[0x2f4]==0&&v->bytes[0x2f5]==0x39))){
  gh_enemy_ems_live grab={0};
  const gh_scene_placement_status lookup=gh_enemy_ems_live_at(s->enemies,s->player_grab_attacker,&grab);
  if(!lookup&&grab.enemy202){
   grab_attacker=&grab.enemy202->entry.object.view;
   grab_resource=grab.enemy202->entry.resource;
  }else{
   const gh_room_state *room=&s->room_state;
   const unsigned stamp=s->player_grab_escape_stamp;
   const int room_id=s->b.work->room;
   const int authored=(room_id==0x100&&stamp==20)||(room_id==0x102&&(stamp==11||stamp==3||stamp==7))||
        (room_id==0x103&&(stamp==2||stamp==3));
   if(!lookup&&!grab.actor&&authored&&
      v->bytes[0x2f4]==0&&v->bytes[0x2f5]==0x39&&v->bytes[0x2f6]==1&&
      room->record&&room->record->room==room_id&&room->active&&room->size>=12&&
      room->active[8]<=64&&12+(size_t)room->active[8]*24<=room->size){
    for(unsigned i=0;i<room->active[8];++i){
     const uint8_t *row=room->active+12+i*24;
     if(row[0x13]!=stamp)continue;
     grab_attacker_retired=(row[0x14]==1);
     break;
    }
   }
   if(!grab_attacker_retired){
    *f=v->bytes[0x2f5]==0x39?0x117600:v->bytes[0x2f5]==8?0x11baf8:
       v->bytes[0x2f5]==0xa?0x11c348:v->bytes[0x2f5]==0xb?0x11c608:0x11d4b0;
    return GH_SERVICES_FRONTIER;
   }
  }
 }
 gh_player_frame_bindings b={.player=&s->title_player,.work=s->b.work,.init=s->b.init,.camera=s->player_frame.bound?s->player_frame.b.camera:enemy_scene_camera(s),.save=s->b.save,
  .grab_attacker=grab_attacker,.grab_attacker_retired=grab_attacker_retired,.grab_resource=grab_resource,.grab_camera=player_grab_camera,.opacity_camera=&s->camera,.scenario={s->scenario_scalar,sizeof(s->scenario_scalar),&s->scenario},.effects=&s->player_frame_effects,
  .current_player=&s->title_player.view,.elf=s->b.boot_elf,.elf_bytes=s->b.boot_elf_bytes,.numeric_policy=GH_CAMERA_NATIVE_BINARY32_REFERENCE,
  .scene_at=&s->scene_at,.take_object=&s->player_tail.take_object,.give_objects=s->player_tail.give_objects,.solids=&s->collision_solids,
  .collision_pairs=&s->collision_pairs,
  .take_binding_known=1,.give_binding_known=1,.give_present_mask=31,
  .model=&s->player_model,.resource=s->player_factory.resource.bytes,.resource_bytes=s->player_factory.file_bytes,.rate=s->b.rate,
  .initialize_motion=player_motion_initialize,.advance_motion=player_frame_motion_advance,.motion_context=s,
  /* Original107E40 explicitly wrote698/6A0=NULL; no later recovered writer
     has published either. The frame also rejects any changed raw token. */
   .auxiliary_bindings_known=1,.effect_create=player_frame_effect_create,.effect_context=s,
   .roulette_target_owner=player_roulette_target_owner,
   .roulette_time_scale=player_roulette_time_scale,.roulette_projectile=player_roulette_projectile_0012bc00,
   .aux_6a0=s->player_frame.bound?s->player_frame.b.aux_6a0:
       s->player_prop.stage==7?&s->player_prop.object->view:NULL,
   .interact_prop=player_interact_prop_001c8ab0,
   .throw_prop=player_throw_prop_0012b618,
   .idle_input=&s->player_idle_input,.idle_input_bindings=&s->player_idle_bindings,.node_cache=&s->player_tail.node_cache,
   .door_probe=player_frame_door_probe,.door_context=s,.actors=&s->scene_tables,
   .ground_globals=s->b.player_ground_globals,.ik_globals=s->b.motion_ik_globals,.atan_coeff=s->b.camera_envelope?s->b.camera_envelope->atan_coeff:NULL,
   .opacity_globals=s->b.player_opacity_globals,
   .aux_68c=s->player_tail.active_child&&s->player_tail.child_original_return?&s->player_child.object.view:NULL,.auxiliary_68c_binding_known=1,
   .target_644=&s->player_target_644,.cockpit_flags_1ca0=gh_game_cockpit_auxiliary_flags_1ca0(s->cockpit),
   .square_attack=player_square_attack,.square_context=s,
   .damage_sound=s->player_sound.bound?gh_actor_attached_sound_motion:NULL,.damage_sound_context=&s->player_sound,
   .damage_effect=player_damage_positioned_effect,.damage_abort=player_damage_abort,
   .damage_camera_exit=player_square_camera_exit,.death_fade=player_death_bgm,.damage_context=s,.damage_vibration_rows=s->player_damage_vibration,
   .paper_release=gh_om376_paper_room_supported(s->b.work->room)?release_player_paper:NULL,.paper_context=s};
 if(!s->player_frame.bound)s->player_idle_bindings=(gh_player_idle_input_bindings){.frame=b,
  .targets=gh_game_services_scene_targets(s),.action_buttons=&s->action_buttons};
 gh_player_frame_status e=gh_game_player_frame_00108ab0(&s->player_frame,&b);*f=s->player_frame.frontier;
 if(!e&&s->player_frame.stage==31&&!s->player_frame.bound)s->update_diagnostic.player_updates_completed++;
 return !e?GH_SERVICES_OK:e==GH_PLAYER_FRAME_UNSUPPORTED?GH_SERVICES_FRONTIER:GH_SERVICES_ASSET;
}
gh_game_services_status gh_game_services_move_actor(gh_game_services*s,uint32_t handle,uint32_t entry,
 int16_t delta,const gh_title_player_view*v,const gh_boot_work*work,uint32_t*f){
 return gh_game_services_move_actor_with_render(s,handle,entry,delta,v,work,NULL,f);
}

static gh_actor_model_status render_other(void*context,const gh_actor_trans_request*r,uint32_t*f){
 gh_game_services*s=context;if(!s||!r||!f)return GH_ACTOR_MODEL_ARGUMENT;*f=0;
 if(r->entry==0x14d828){
  gh_actor_light_bindings b={.pivot_owner=s->b.player_shared_vector,.camera=r->camera};
  return gh_actor_model_light_0014d828(r->model,&s->world_light,&b,GH_STATIC_BINARY32_REFERENCE,f);
 }
 if(r->entry!=0x31a600){*f=r->entry;return GH_ACTOR_MODEL_UNSUPPORTED;}
 unsigned packets=UINT32_MAX;gh_actor_model_status e=gh_actor_auxiliary_0014a3e8(r->model,r->parity,&packets,f);
 if(e)return e;if(packets){*f=0x31a600;return GH_ACTOR_MODEL_UNSUPPORTED;}return GH_ACTOR_MODEL_OK;
}
/* Developer frontier diagnostics. Copy bits from the canonical model while
 * alive; do not retain a second model/actor owner or change original state. */
static void trace_render_frontier(const char *model_kind,const gh_object_trans_request*q,const gh_actor_model*m,const gh_actor_render_call*c,uint32_t frontier){
 const uint8_t*n=NULL;size_t mesh=0;uint32_t root_flags[2]={0},node_flags=0,node_tail=0,rt[4]={0},nt[4]={0};
 if(m->root.size>=0x258)memcpy(root_flags,m->root.bytes+0x250,8);
 if(m->root.size>=0x250)memcpy(rt,m->root.bytes+0x240,16);
 unsigned kind=m->root.size>=0x300?(u32(m->root.bytes+0x2fc)>>16):0;
 int known=gh_actor_model_node_at(m,(unsigned)c->draw.node_index,&n,&mesh)==GH_ACTOR_MODEL_OK;
 if(known){memcpy(&node_flags,n+0x380,4);memcpy(&node_tail,n+0x410,4);memcpy(nt,n+0x3a0,16);}
 fprintf(stderr,"GH_ACTOR_FRONTIER {\"model_kind\":\"%s\",\"actor_kind\":\"%04X\",\"part_count\":%u,\"node_count\":%llu,\"handle\":%u,\"entry\":\"%08X\",\"frontier\":\"%08X\",\"trans_phase\":%u,\"draw_phase\":%u,\"node_index\":%llu,\"node_known\":%d,\"mesh_index\":%llu,\"root250\":\"%08X\",\"root254\":\"%08X\",\"node380\":\"%08X\",\"node410\":\"%08X\",\"root_tint_bits\":[\"%08X\",\"%08X\",\"%08X\",\"%08X\"],\"node_tint_bits\":[\"%08X\",\"%08X\",\"%08X\",\"%08X\"]}\n",
 model_kind,kind,(unsigned)m->part_count,(unsigned long long)m->script.mesh_count,q->handle,q->original_entry,frontier,c->trans.phase,c->draw.phase,(unsigned long long)c->draw.node_index,known,(unsigned long long)mesh,root_flags[0],root_flags[1],node_flags,node_tail,rt[0],rt[1],rt[2],rt[3],nt[0],nt[1],nt[2],nt[3]);
}
static gh_game_services_status destroy_om(gh_game_services *s,
 const gh_object_trans_request *request,uint32_t *frontier)
{
 void *registered=NULL;
 if(gh_room_object_lookup(&s->pools,request->handle,GH_ROOM_OBJECT_OM,&s->om_factory,&registered)){
  *frontier=request->original_entry;return GH_SERVICES_ASSET;
 }
 size_t index=0;
 while(index<s->om_factory.count&&registered!=s->om_factory.objects+index)++index;
 if(index>=s->om_factory.count||index>=s->om_factory.capacity){*frontier=request->original_entry;return GH_SERVICES_ASSET;}
 gh_om_object *object=registered;
 if(object->view.bytes!=request->actor.bytes){
  *frontier=request->original_entry;return GH_SERVICES_ASSET;
 }
 const int r101_weapon=s->b.work->room==0x101&&!object->runtime_created&&object->source.type==0x382;
 if(request->original_entry==0x1c91b0&&!r101_weapon&&(!object->runtime_created||
    (object->runtime_kind!=0x376&&object->runtime_kind!=0x363))){
  *frontier=request->original_entry;return GH_SERVICES_FRONTIER;
 }
 if(!s->om_retirement.stage){
  int borrowed;
  if(gh_effect_create_actor_has_live_refs(&s->effects,&object->view,&borrowed)||borrowed){
   *frontier=0x305b90;return GH_SERVICES_FRONTIER;
  }
 }
 s->om_retire_sound=(gh_object_sound_binding){s->b.sound_control,s->b.voices};
 const gh_om300_retire_bindings bindings={.factory=&s->om_factory,.tables=&s->scene_tables,
  .scroll=&s->r100.collision,.solids=&s->collision_solids,
  .release_sound_slot=gh_sound_object_slot_release_002cb340,.sound_context=&s->om_retire_sound,
  .effects=player_effect_storage(s)};
 gh_room_pool_status status=gh_om_destroy_detached_mode3(&s->om_retirement,
  &bindings,request->handle,&request->actor,request->original_entry);
 if(status){
  *frontier=s->om_retirement.frontier?s->om_retirement.frontier:request->original_entry;
  return status==GH_POOL_UNSUPPORTED?GH_SERVICES_FRONTIER:GH_SERVICES_ASSET;
 }
 if(s->om_frames&&index<s->om_frames_capacity)s->om_frames[index]=(gh_om_frame_state){0};
 if(s->om_renders&&index<s->om_renders_capacity)s->om_renders[index]=(gh_actor_render_call){0};
 s->om_retirement=(gh_om300_full_retirement){0};
 return GH_SERVICES_OK;
}

static int enemy_destroy_descriptor(const gh_object_trans_request *q)
{
 return q->kind==GH_OBJECT_TRANS_DESTROY&&q->argument==3&&!q->this_delta&&
  ((q->original_entry==0x202058&&q->original_vtable==0x4479c8)||
   (q->original_entry==0x284a90&&q->original_vtable==0x448ca0));
}
static int r101_held_weapon_destroy_descriptor(const gh_game_services *s,const gh_object_trans_request *q)
{
 return s->b.work->room==0x101&&q->kind==GH_OBJECT_TRANS_DESTROY&&q->argument==3&&!q->this_delta&&
    q->original_entry==0x1c91b0&&q->original_vtable==0x42aa98&&q->actor.bytes&&
    q->actor.size>=0x300&&(u32(q->actor.bytes+0x2fc)>>16)==0x361;
}

static gh_actor_motion_status retiring_enemy_effect(void *context,const gh_title_player_view *actor,
 int32_t kind,uint32_t mode,uint32_t *frontier)
{
 gh_game_services *s=context;
 if(!actor||!actor->bytes||actor->bytes!=s->retiring_enemy_actor)return GH_ACTOR_MOTION_ARGUMENT;
 return release_actor_motion_effect(s,actor,kind,mode,frontier);
}
static gh_game_services_status enemy_retire_status(gh_actor_motion_status status,uint32_t pc,uint32_t *frontier)
{
 if(!status)return GH_SERVICES_OK;
 if(!*frontier)*frontier=pc;
 return status==GH_ACTOR_MOTION_UNSUPPORTED?GH_SERVICES_FRONTIER:GH_SERVICES_ASSET;
}
static gh_game_services_status destroy_ems_enemy(gh_game_services *s,const gh_object_trans_request *q,
 uint32_t *frontier)
{
 gh_enemy_ems_live live;
 if(gh_enemy_ems_live_at(s->enemies,q->handle,&live)||!live.actor||live.held_weapon||
    live.physical_row>=64||live.actor->view.bytes!=q->actor.bytes){
  *frontier=q->original_entry;
  return GH_SERVICES_FRONTIER;
 }
 if(s->retiring_enemy_actor&&s->retiring_enemy_actor!=q->actor.bytes)return GH_SERVICES_ARGUMENT;
 s->retiring_enemy_actor=q->actor.bytes;
 s->retiring_enemy_row=live.physical_row;
 s->enemy_retire_sound=(gh_object_sound_binding){s->b.sound_control,s->b.voices};
 const gh_enemy_retire_bindings b={.effects=&s->effects,.dynamic=&s->dynamic_objects,
  .release_effect=retiring_enemy_effect,.effect_context=s,
  .release_sound_slot=gh_sound_object_slot_release_002cb340,.sound_context=&s->enemy_retire_sound};
 gh_actor_motion_status status=gh_enemy_ems_destroy_detached_mode3(s->enemies,&s->enemy_retirement,
  q->handle,&q->actor,&b,frontier);
 return enemy_retire_status(status,q->original_entry,frontier);
}
static gh_game_services_status destroy_ems_weapon(gh_game_services *s,const gh_object_trans_request *q,
 uint32_t *frontier)
{
 gh_enemy_ems_live live;
 if(gh_enemy_ems_live_at(s->enemies,q->handle,&live)||!live.actor||!live.held_weapon||
    live.physical_row>=64||live.actor->view.bytes!=q->actor.bytes){
  *frontier=q->original_entry;
  return GH_SERVICES_FRONTIER;
 }
 if(s->retiring_enemy_actor&&s->retiring_enemy_actor!=q->actor.bytes)return GH_SERVICES_ARGUMENT;
 s->retiring_enemy_actor=q->actor.bytes;
 s->retiring_enemy_row=live.physical_row;
 s->enemy_retire_sound=(gh_object_sound_binding){s->b.sound_control,s->b.voices};
 const gh_enemy_retire_bindings b={.effects=&s->effects,.dynamic=&s->dynamic_objects,
  .release_effect=retiring_enemy_effect,.effect_context=s,
  .release_sound_slot=gh_sound_object_slot_release_002cb340,.sound_context=&s->enemy_retire_sound};
 gh_actor_motion_status status=gh_enemy_ems_destroy_detached_weapon_001c91b0(
  s->enemies,&s->enemy_retirement,q->handle,&b,frontier);
 return enemy_retire_status(status,q->original_entry,frontier);
}

gh_game_services_status gh_game_services_trans_erase(gh_game_services *s,const gh_object_trans_request *q,
 const void *owner,int *handled,uint32_t *frontier)
{
 if(!s||!q||!handled||!frontier)return GH_SERVICES_ARGUMENT;
 *handled=0;*frontier=0;
 if(s->b.work->room==0x101&&q->original_entry==0x1c91b0&&s->enemy_retirement.stage==7&&
    s->enemy_retirement.handle==q->handle&&s->retiring_enemy_actor==q->actor.bytes){
  if(s->retiring_enemy_row>=64)return GH_SERVICES_ARGUMENT;
  if(s->enemy_renders)s->enemy_renders[2*s->retiring_enemy_row+1]=(gh_actor_render_call){0};
  s->enemy_retirement=(gh_enemy_full_retirement){0};
  s->retiring_enemy_actor=NULL;
  *handled=1;
  return GH_SERVICES_OK;
 }
 if(!enemy_destroy_descriptor(q))return GH_SERVICES_OK;
 if(s->enemy_retirement.handle!=q->handle||s->retiring_enemy_actor!=q->actor.bytes||
    s->retiring_enemy_row>=64)return GH_SERVICES_ARGUMENT;
 gh_actor_motion_status status=gh_enemy_ems_finish_detached_mode3(s->enemies,&s->enemy_retirement,
  q->handle,owner,frontier);
 if(status)return enemy_retire_status(status,0x30f7d0,frontier);
 if(s->enemy_renders)s->enemy_renders[2*s->retiring_enemy_row]=(gh_actor_render_call){0};
 s->enemy_retirement=(gh_enemy_full_retirement){0};
 s->retiring_enemy_actor=NULL;
 *handled=1;
 return GH_SERVICES_OK;
}
static gh_actor_motion_status scene_retiring_enemy_effect(void *context,const gh_title_player_view *actor,
 int32_t kind,uint32_t mode,uint32_t *frontier)
{
 gh_game_services *s=context;
 gh_enemy_ems_live live;
 if(!actor||gh_enemy_ems_live_at(s->enemies,s->scene_death.retiring_handle,&live)||
    !live.actor||live.actor->view.bytes!=actor->bytes)return GH_ACTOR_MOTION_ARGUMENT;
 return release_actor_motion_effect(s,actor,kind,mode,frontier);
}

/* State5 has stopped ESPs via 305B90 and no longer runs the regular
 * 2FB0E8 traversal. Apply its existing pause-to-hidden destructor to the
 * still registered typed source before destroying the borrowing model. */
static gh_effect_status finish_paused_scene_model_effect(gh_game_services *s,
    gh_esp14_instance *instance,uint32_t *frontier)
{
 *frontier=0x2f27e8;
 const unsigned index=instance->slot;
 if(index>=1024)return GH_EFFECT_ARGUMENT;
 const gh_effect_release_bindings storage=player_effect_storage(s);
 if(!storage.esp||storage.esp_bytes<0x100180)return GH_EFFECT_ARGUMENT;
 gh_effect_esp_view view;
 if(gh_effect_create_esp_at(&s->effects,index,&view)||view.serial!=instance->serial||
    !view.initialized||(view.type!=4&&view.type!=0x14)||view.parent_emitter!=-1)
  return GH_EFFECT_UNSUPPORTED;
 const unsigned offset=(index>>5)*4;
 const uint32_t mask=UINT32_C(0x80000000)>>(index&31);
 uint8_t *active=storage.esp+0x100000+offset;
 uint8_t *paused=storage.esp+0x100080+offset;
 uint8_t *hidden=storage.esp+0x100100+offset;
 if(!(u32(active)&mask)||!((u32(paused)|u32(hidden))&mask))return GH_EFFECT_UNSUPPORTED;
 w32(hidden,u32(hidden)|mask);
 const gh_effect_status status=destroy_model_effect(s,&view,index,frontier);
 if(status)return status;
 w32(active,u32(active)&~mask);
 return GH_EFFECT_OK;
}

/* ESP14 requests the kind500 model's deletion before State5 visits its
 * original 44B840 mode3 destructor. Keep the actor lifetime distinct from
 * the source particle and reuse the typed 2B8908 bound resource checks. */
static gh_room_pool_status retire_scene_effect_model(void *context,uint32_t handle,int closing,
 int *handled,int *retired,uint32_t *frontier)
{
 gh_game_services *s=context;
 *handled=*retired=0;
 *frontier=0;
 const gh_boot_work *work=s->b.work;
 const uint8_t *raw=(const uint8_t *)work;
 if(work->room!=0x104||raw[0x5b4]!=0x21||raw[0x5b5])return GH_POOL_OK;
 gh_services_esp14_slot *slot=model_effect_actor(s,handle);
 if(!slot)return GH_POOL_OK;
 *handled=1;
 gh_esp14_instance *instance=&slot->instance;
 gh_esp14_model *child=&instance->child;
 uint8_t *actor=child->object.view.bytes;
 if(!actor||child->object.view.size<0x300||child->object.view.original_vtable214!=0x44b840){
  *frontier=0x312708;
  return GH_POOL_UNSUPPORTED;
 }
 if(!instance->esp_destroyed&&finish_paused_scene_model_effect(s,instance,frontier))
  return GH_POOL_UNSUPPORTED;
 if(!instance->child_delete_requested){
  *frontier=0x2f27e8;
  return GH_POOL_UNSUPPORTED;
 }
 uint32_t flags=u32(actor+0x250);
 if((flags&0x80000002u)!=0x80000002u||
    (closing&&(flags&0xe0000000u)!=0xe0000000u)){
  *frontier=0x312708;
  return GH_POOL_UNSUPPORTED;
 }
 if(!(flags&0x20000000u)){
  flags|=(flags&0x40000000u)?0x20000000u:0x40000000u;
  w32(actor+0x250,flags);
  return GH_POOL_OK;
 }
 gh_effect_status effect=gh_esp14_child_destroy_002b8908(instance,s->room_objects,s->objects,frontier);
 if(effect)return GH_POOL_UNSUPPORTED;
 if(gh_room_object_unregister(&s->pools,handle,child)){
  *frontier=0x30fa08;
  return GH_POOL_UNSUPPORTED;
 }
 if(collect_model_effects(s,frontier))return GH_POOL_UNSUPPORTED;
 *retired=1;
 return GH_POOL_OK;
}

static gh_game_services_status retire_scene_objects(gh_game_services *s,int closing,
 uint32_t *frontier,uint32_t *blocked_handle)
{
 if(!s||!frontier)return GH_SERVICES_ARGUMENT;
 *frontier=0;
 if(blocked_handle)*blocked_handle=0;
 const gh_room_vector *primary;
 if(gh_room_registry_view(&s->pools,0x754c30,&primary))return GH_SERVICES_ARGUMENT;
 if(primary->count){
  if(!s->scene_death.bound){
   s->om_retire_sound=(gh_object_sound_binding){s->b.sound_control,s->b.voices};
   s->enemy_retire_sound=(gh_object_sound_binding){s->b.sound_control,s->b.voices};
   s->scene_death_bindings=(gh_room_death_bindings){
    .scroll_meshes=&s->meshes,
    .player={&s->player_factory,&s->player_model,&s->collision_solids,
     &s->player_attachment,&s->object_resources},
    .child=&s->player_child,.child_tail=&s->child_tail,
    .door={.factory=&s->om_factory,.tables=&s->scene_tables,.scroll=&s->r100.collision,
     .solids=&s->collision_solids,.release_sound_slot=gh_sound_object_slot_release_002cb340,
     .sound_context=&s->om_retire_sound,.effects=player_effect_storage(s)},
    .hitarea=s->b.init,.pairs=&s->collision_pairs,.dynamic=&s->dynamic_objects,
    .effects=&s->effects,.voices=s->b.voices,.enemies=s->enemies,
    .retire_effect_model=retire_scene_effect_model,.effect_context=s,
    .enemy={.effects=&s->effects,.dynamic=&s->dynamic_objects,
     .release_effect=scene_retiring_enemy_effect,.effect_context=s,
     .release_sound_slot=gh_sound_object_slot_release_002cb340,
     .sound_context=&s->enemy_retire_sound,
     .voices=s->b.voices}};
  }
  gh_room_pool_status status=closing?gh_room_death_release_owners(&s->scene_death,&s->scene_death_bindings):
   gh_room_death_dispatch_0030f8b0(&s->scene_death,&s->scene_death_bindings);
  if(status){
   *frontier=s->scene_death.frontier?s->scene_death.frontier:0x30f8b0;
   if(blocked_handle)*blocked_handle=s->scene_death.blocked_handle;
   return status==GH_POOL_UNSUPPORTED?GH_SERVICES_FRONTIER:GH_SERVICES_ASSET;
  }
 }
 if(primary->count)return GH_SERVICES_OK;

 /* Native descriptor arrays are no longer borrowed once the last original
  * actor has retired. The original State5 phase5 still owns pool teardown. */
 if(s->meshes.descriptors||s->meshes.first.allocation.pointer){
  if(gh_scene_meshes_release_prefix(&s->meshes)){
   *frontier=0x30f8b0;
   return GH_SERVICES_ASSET;
  }
 }
 if(s->enemies){
  if(s->b.init->enemy_top){*frontier=0x290770;return GH_SERVICES_FRONTIER;}
  gh_game_devil_camera_detach(&s->devil_camera,s->b.init);
  gh_actor_motion_status status=gh_enemy_ems_release_native(&s->enemies,frontier);
  if(status)return enemy_retire_status(status,0x290770,frontier);
 }
 free(s->enemy_renders);
 s->enemy_renders=NULL;
 s->devil_render=(gh_actor_render_call){0};
 s->scene_death=(gh_room_death_dispatch){0};
 s->scene_death_bindings=(gh_room_death_bindings){0};
 return GH_SERVICES_OK;
}

gh_game_services_status gh_game_services_scene_retire_objects_0030f8b0(gh_game_services *s,
 uint32_t *frontier,uint32_t *blocked_handle)
{
 return retire_scene_objects(s,0,frontier,blocked_handle);
}

gh_game_services_status gh_game_services_scene_release_owners(gh_game_services *s,
 uint32_t *frontier,uint32_t *blocked_handle)
{
 return retire_scene_objects(s,1,frontier,blocked_handle);
}

gh_game_services_status gh_game_services_release_enemies_002908c0(gh_game_services *s,uint32_t *frontier)
{
 if(!s||!frontier)return GH_SERVICES_ARGUMENT;
 *frontier=0;
 gh_game_init_state *manager=s->b.init;
 while(manager->enemy_top){
  unsigned slot=0;
  while(slot<64&&manager->enemy_top!=manager->enemies+20*slot)++slot;
  uint32_t handle=0;
  if(slot==64||!manager->enemy_actors[slot]||
     gh_enemy_ems_handle_for_actor(s->enemies,manager->enemy_actors[slot],&handle)||!handle){
   *frontier=0x290770;
   return GH_SERVICES_FRONTIER;
  }
  int removed=0;
  gh_actor_motion_status status=gh_enemy_ems_remove_00290770(s->enemies,&s->enemy_removal,
   handle,s->b.voices,&removed,frontier);
  if(status)return enemy_retire_status(status,0x290770,frontier);
  if(!removed){*frontier=0x290770;return GH_SERVICES_FRONTIER;}
  s->enemy_removal=(gh_enemy_manager_remove){0};
 }
 /* EV1A retires the original room placements before its packed actors load.
  * The second 294CA0 at 2972B0 must build a fresh pass from the saved rows. */
 if(s->b.work->room==0x103&&s->b.event&&s->b.event->fields.data_number==0x1a&&
    s->em_resources.stage==9)
  s->em_resources=(gh_em_resource_prefix){0};
 return GH_SERVICES_OK;
}

gh_game_services_status gh_game_services_trans_actor(gh_game_services*s,const gh_object_trans_request*q,
 const gh_boot_work*work,const gh_game_actor_render_provider*provider,uint32_t*f){
 if(f)*f=0;if(!s||!q||!provider||!f||work!=s->b.work)return GH_SERVICES_ARGUMENT;
 if(r101_held_weapon_destroy_descriptor(s,q))return destroy_ems_weapon(s,q,f);
 if(q->kind==GH_OBJECT_TRANS_DESTROY&&q->argument==3&&!q->this_delta&&
    ((q->original_entry==0x1867c0&&q->original_vtable==0x422f90)||
     (q->original_entry==0x1c00c8&&q->original_vtable==0x429b88)||
     (q->original_entry==0x187d10&&q->original_vtable==0x423338)||
     (q->original_entry==0x188af0&&q->original_vtable==0x423400)||
     (q->original_entry==0x189008&&q->original_vtable==0x423608)||
     (q->original_entry==0x18c120&&q->original_vtable==0x4238d8)||
     (q->original_entry==0x191dc0&&q->original_vtable==0x424128)||
     (q->original_entry==0x19c3a8&&q->original_vtable==0x425b90&&s->b.work->room==0x101)||
     (q->original_entry==0x198ce0&&q->original_vtable==0x425000)||
     (q->original_entry==0x18b318&&q->original_vtable==0x4237e8)||
     (q->original_entry==0x19c610&&q->original_vtable==0x425c78)||
     (q->original_entry==0x1a8730&&q->original_vtable==0x427880)||
     (q->original_entry==0x199df8&&q->original_vtable==0x4251c0)||
     (q->original_entry==0x19f130&&q->original_vtable==0x4267f8)||
     (q->original_entry==0x1a0df0&&q->original_vtable==0x426e78)||
     (q->original_entry==0x1c91b0&&q->original_vtable==0x42aa98)||
     (s->b.work->room==0x200&&q->original_entry==0x1b87f0&&q->original_vtable==0x428c20)||
    (s->b.work->room==0x101&&q->original_entry==0x1b0d30&&q->original_vtable==0x4282f0)||
     (q->original_entry==0x1b9030&&q->original_vtable==0x4290c8)))
  return destroy_om(s,q,f);
 if(enemy_destroy_descriptor(q))return destroy_ems_enemy(s,q,f);
 gh_services_esp14_slot *effect_actor=model_effect_actor(s,q->handle);
 if(q->kind==GH_OBJECT_TRANS_DESTROY&&q->original_entry==0x2b8908&&
    q->argument==3&&!q->this_delta&&effect_actor){
  gh_esp14_instance *instance=&effect_actor->instance;
  if(q->actor.bytes!=instance->child.object.view.bytes){*f=0x2b8908;return GH_SERVICES_ASSET;}
  gh_effect_status status=gh_esp14_child_destroy_002b8908(instance,s->room_objects,s->objects,f);
  return !status?GH_SERVICES_OK:status==GH_EFFECT_UNSUPPORTED?GH_SERVICES_FRONTIER:GH_SERVICES_ASSET;
 }
 if(q->kind!=GH_OBJECT_TRANS_NORMAL||q->original_entry!=0x14a3e8||q->this_delta){*f=q->original_entry;return GH_SERVICES_FRONTIER;}
 gh_actor_model*model=NULL;const gh_player_shadow*shadow=NULL;gh_actor_render_call*call=NULL;
 const gh_title_player_view*player=NULL;
 gh_event_actor_factory *event_actor=event_actor_for_handle(s,q->handle);
 gh_enemy_ems_live enemy;
 gh_scene_placement_status ee=gh_enemy_ems_live_at(s->enemies,q->handle,&enemy);
 if(ee){*f=0x14a3e8;return enemy_place_error(ee);}
 if(enemy.actor){
  if(!enemy.devil&&!s->enemy_renders){
   s->enemy_renders=calloc(128,sizeof(*s->enemy_renders));
   if(!s->enemy_renders){*f=0x14a3e8;return GH_SERVICES_MEMORY;}
  }
  model=enemy.model;shadow=enemy.shadow;
  call=enemy.devil?&s->devil_render:&s->enemy_renders[2*enemy.physical_row+enemy.held_weapon];
 }else if(effect_actor){
  model=&effect_actor->instance.child.model;call=&effect_actor->render;
 }else if(!gh_game_player_factory_scalar_view(&s->player_factory,q->handle,&player)){
  if(!s->player_tail.complete||s->player_model.root.bytes!=player->bytes){*f=0x14a3e8;return GH_SERVICES_FRONTIER;}
  model=&s->player_model;shadow=&s->player_tail.shadow;call=&s->player_render;
 }else if(q->handle==s->player_child.handle&&s->player_child.complete){
  void*registered=NULL;if(gh_room_object_lookup(&s->pools,q->handle,GH_ROOM_OBJECT_PLAYER_CHILD,&s->player_child.object,&registered)||registered!=&s->player_child.object){*f=0x14a3e8;return GH_SERVICES_ASSET;}
  model=&s->player_child.model;call=&s->child_render;
 }else if(event_actor){
  gh_title_player_view registered;
  if(gh_event_actor_factory_view(event_actor,q->handle,&registered)){*f=0x14a3e8;return GH_SERVICES_ASSET;}
  const size_t capacity=sizeof(s->event_actor_render)/sizeof(*s->event_actor_render);
  int accessory=s->event_actor_extra&&event_actor==&s->event_actor_extra->olivia_accessory;
  if(!accessory&&event_actor->actor_index>=capacity-1){*f=0x14a3e8;return GH_SERVICES_ASSET;}
  size_t index=accessory?capacity-1:event_actor->actor_index;
  model=&event_actor->model;
  shadow=event_actor->shadow.initialized?&event_actor->shadow:NULL;
  call=&s->event_actor_render[index];
  if(s->event_actor_extra){
   for(size_t i=0;i<sizeof(s->event_accessory_render)/sizeof(*s->event_accessory_render);++i)
    if(event_actor==&s->event_actor_extra->ev04_accessory[i])call=&s->event_accessory_render[i];
  }
 }else{
  void*registered=NULL;if(gh_room_object_lookup(&s->pools,q->handle,GH_ROOM_OBJECT_OM,&s->om_factory,&registered)){*f=q->original_entry;return GH_SERVICES_FRONTIER;}
  size_t index;for(index=0;index<s->om_factory.count;index++)if(registered==s->om_factory.objects+index)break;
  if(index>=s->om_factory.count||index>=s->om_factory.capacity){*f=0x14a3e8;return GH_SERVICES_ASSET;}
  if(gh_om_factory_model_at(&s->om_factory,q->handle,&model)){*f=0x14a3e8;return GH_SERVICES_FRONTIER;}
  if(!s->om_renders){s->om_renders=calloc(s->om_factory.capacity,sizeof(*s->om_renders));if(!s->om_renders){*f=0x14a3e8;return GH_SERVICES_MEMORY;}s->om_renders_capacity=s->om_factory.capacity;}
  if(s->om_renders_capacity!=s->om_factory.capacity){*f=0x14a3e8;return GH_SERVICES_BUSY;}call=s->om_renders+index;
 }
 if(!model||!model->ready||model->native_identity!=model||model->root.bytes!=q->actor.bytes||model->root.size!=q->actor.size||model->root.alias_f0!=q->actor.alias_f0){*f=0x14a3e8;return GH_SERVICES_ASSET;}
 /* These factory paths execute the recovered1495A0 base
  * constructor. Its four NULL auxiliary refs are explicit provenance, not
  * a generic inference from removed guest-pointer words. */
 gh_actor_model_status auxiliary=gh_actor_model_aux_bind_constructor_empty_001495a0(model);
 if(auxiliary){*f=0x31a600;return auxiliary==GH_ACTOR_MODEL_UNSUPPORTED?GH_SERVICES_FRONTIER:GH_SERVICES_ASSET;}
 if(call->trans.complete&&gh_actor_render_next_call(call)){*f=0x14a3e8;return GH_SERVICES_BUSY;}
 gh_actor_render_bindings b={.work=work,.camera=provider->camera?provider->camera:&s->camera,.light=&s->world_light,.depth_shift=provider->depth_shift,
  .elf=s->b.boot_elf,.elf_bytes=s->b.boot_elf_bytes,.shadow=shadow,.actors=&s->scene_tables,
  .up_owner=s->b.camera_follow,.pivot_owner=s->b.player_shared_vector,.templates=provider->shadow_templates,
  .textures=s->b.stage->textures,.emit_shadow=provider->shadow_emit,.emit_context=provider->shadow_context,.policy=GH_STATIC_BINARY32_REFERENCE,
  .actor_emit=provider->actor_emit,.actor_emit_context=provider->actor_context,
  .other_callee=render_other,.other_context=s,.light_globals=s->b.actor_light_globals,
  .marker_player=enemy.marker_id?&s->title_player.view:NULL,
  .marker_save=enemy.marker_id?s->b.save:(gh_core_save_view){0},
  .marker_cockpit=enemy.marker_id?s->cockpit:NULL,.marker_id=enemy.marker_id,
  .marker_random=enemy.marker_id?&s->b.init->random:NULL,
  .marker_runtime=enemy.marker_id?*s->b.runtime:NULL,
  .marker_manager=enemy.marker_id?s->b.id:NULL,
  .marker_emit=enemy.marker_id?provider->marker_emit:NULL,
  .marker_context=enemy.marker_id?provider->marker_context:NULL};
 gh_actor_model_status e=gh_actor_render_0014a3e8(call,model,&b,f);
 if(e){
  const char *kind=enemy.actor?(enemy.held_weapon?"enemy_weapon":"enemy"):
   effect_actor?"effect":event_actor?"event_actor":model==&s->player_model?"player":
   model==&s->player_child.model?"child":"room_object";
  trace_render_frontier(kind,q,model,call,*f);
  return e==GH_ACTOR_MODEL_UNSUPPORTED?GH_SERVICES_FRONTIER:GH_SERVICES_ASSET;
 }
 if(!call->trans.complete){*f=0x14a3e8;return GH_SERVICES_ASSET;}return GH_SERVICES_OK;
}

gh_action_step_status gh_game_services_r100_tutorial_action(gh_game_services *s,void *argument,
 uint32_t *frontier)
{
 if(!frontier)return GH_ACTION_STEP_ARGUMENT;
 *frontier=0x1bf7d8;
 if(!s||!argument||s->b.work->room!=0x100)return GH_ACTION_STEP_ARGUMENT;
 for(unsigned i=0;i<4;i++){
  gh_om_object *object=s->tutorial_objects[i];
  if(!object||object!=argument)continue;
  void *registered=NULL;
  if(!object->registered||!object->runtime_created||object->runtime_kind!=0x3eb||
     gh_room_object_lookup(&s->pools,object->handle,GH_ROOM_OBJECT_OM,&s->om_factory,&registered)||
     registered!=object)return GH_ACTION_STEP_ARGUMENT;
  const gh_scene_scenario_view scenario={s->scenario_scalar,sizeof(s->scenario_scalar),&s->scenario};
  return gh_cockpit_tutorial_action_001bf7d8(&s->tutorial_action,scenario,s->scenario_threads,s->b.work,
    s->cockpit,&object->view,frontier);
 }
 return GH_ACTION_STEP_ARGUMENT;
}

gh_scene_step_status gh_game_services_task_status(gh_game_services*s,uint32_t*frontier){
 if(!s||!frontier)return GH_SCENE_STEP_ARGUMENT;*frontier=0;
 if(s->r102.task_frontier){
  *frontier=s->r102.task_frontier;s->frontier=*frontier;
  s->frontier_module_code=*frontier<0x100000?7:0;
  return GH_SCENE_STEP_UNSUPPORTED;
 }
 if(s->r104_task_frontier){
  *frontier=s->r104_task_frontier;s->frontier=*frontier;
  s->frontier_module_code=*frontier<0x100000?9:0;
  return GH_SCENE_STEP_UNSUPPORTED;
 }
 if(s->r103_task_frontier){
  *frontier=s->r103_task_frontier;s->frontier=*frontier;
  s->frontier_module_code=*frontier<0x100000?8:0;
  return GH_SCENE_STEP_UNSUPPORTED;
 }
 if(s->r101_task_frontier){
  *frontier=s->r101_task_frontier;s->frontier=*frontier;s->frontier_module_code=*frontier<0x100000?5:0;
  return GH_SCENE_STEP_UNSUPPORTED;
 }
 if(s->r200_task_frontier){
  *frontier=s->r200_task_frontier;s->frontier=*frontier;
  s->frontier_module_code=*frontier<0x100000?10:0;
  return GH_SCENE_STEP_UNSUPPORTED;
 }
 if(s->r105_scenery_task_status){
  *frontier=s->r105_scenery_task_frontier;s->frontier=*frontier;s->frontier_module_code=0;
  return s->r105_scenery_task_status;
 }
 if(s->tutorial_action.status!=GH_ACTION_STEP_OK){
  *frontier=s->tutorial_action.frontier;s->frontier=*frontier;s->frontier_module_code=0;
  return s->tutorial_action.status==GH_ACTION_STEP_UNSUPPORTED?GH_SCENE_STEP_UNSUPPORTED:GH_SCENE_STEP_CALLEE;
 }
 if(s->r003.initialized&&s->r003.status!=GH_R003_OK){
  const int r002=s->b.work->room==2;
  *frontier=s->r003.frontier?s->r003.frontier:r002?0x210:0x248;
  s->frontier=*frontier;
  s->frontier_module_code=s->r003.frontier?0:r002?6:3;
  return s->r003.status==GH_R003_UNSUPPORTED?GH_SCENE_STEP_UNSUPPORTED:GH_SCENE_STEP_CALLEE;
 }
 if(s->r100_roof.registered&&s->r100_roof.status!=GH_R100_SCRIPT_OK){
  *frontier=s->r100_roof.frontier?s->r100_roof.frontier:0x2c5168;s->frontier=*frontier;s->frontier_module_code=0;
  return s->r100_roof.status==GH_R100_SCRIPT_UNSUPPORTED?GH_SCENE_STEP_UNSUPPORTED:GH_SCENE_STEP_CALLEE;
 }
 if(s->r100_scripts.registered&&s->r100_scripts.status!=GH_R100_SCRIPT_OK){
  *frontier=s->r100_scripts.frontier?s->r100_scripts.frontier:0x2c5168;s->frontier=*frontier;
  s->frontier_module_code=s->r100_scripts.frontier_relative?2:0;
  return s->r100_scripts.status==GH_R100_SCRIPT_UNSUPPORTED?GH_SCENE_STEP_UNSUPPORTED:GH_SCENE_STEP_CALLEE;
 }
 if(s->r100_counter.registered&&s->r100_counter.status!=GH_R100_SCRIPT_OK){
  *frontier=s->r100_counter.frontier?s->r100_counter.frontier:0x2c5168;s->frontier=*frontier;
  s->frontier_module_code=s->r100_counter.frontier_relative?2:0;
  return s->r100_counter.status==GH_R100_SCRIPT_UNSUPPORTED?GH_SCENE_STEP_UNSUPPORTED:GH_SCENE_STEP_CALLEE;
 }
 for(unsigned i=0;i<sizeof(s->r100_live.tasks)/sizeof(s->r100_live.tasks[0]);i++){
  const gh_r100_live_task *task=&s->r100_live.tasks[i];
  if(!task->registered||task->status==GH_R100_SCRIPT_OK)continue;
  *frontier=task->frontier?task->frontier:task->entry;s->frontier=*frontier;
  s->frontier_module_code=task->frontier_relative?2:0;
  return task->status==GH_R100_SCRIPT_UNSUPPORTED?GH_SCENE_STEP_UNSUPPORTED:GH_SCENE_STEP_CALLEE;
 }
 return GH_SCENE_STEP_OK;
}
gh_game_event_status gh_game_services_event_return_0031c7e0(gh_game_services *s,
 const gh_game_event *event,uint32_t *frontier){
 if(!frontier)return GH_EVENT_ARGUMENT;
 *frontier=0x31c7e0;
 if(!s||!event||!s->title_player.ready)return GH_EVENT_ARGUMENT;
 gh_title_player_view *player=&s->title_player.view;
 if(!player->bytes||player->size<0x690||!player->alias_f0||
    player->original_vtable214!=0x41d220||u32(player->bytes+0x214)!=0x41d220||
    player->bytes[0x2fe]!=0||player->bytes[0x2ff]!=1)return GH_EVENT_UNSUPPORTED;
 const uint32_t euler[3]={0,event->fields.angle_bits,0};
 uint32_t pose_frontier;
 if(room_r100_successor_player(s,event->fields.position_bits,euler,&pose_frontier))
  return GH_EVENT_UNSUPPORTED;
 memcpy(player->bytes+0x30,event->fields.position_bits,12);
 *frontier=0;
 return GH_EVENT_OK;
}

gh_game_event_status gh_game_services_event_player_clear(gh_game_services*s,const gh_event_clear_bindings*b,uint32_t*frontier){
 if(frontier)*frontier=0;
 if(!s||!b||!frontier||!b->player_abort||b->player_abort->player!=&s->title_player||b->event!=s->b.event)return GH_EVENT_ARGUMENT;
 if(actor_sound_prepare(s,&s->player_sound,frontier))return GH_EVENT_CALLEE;
 gh_game_event_status e=GH_EVENT_OK;
 if(!s->event_player_reset.active||s->event_player_reset.complete){
  s->event_player_abort=*b->player_abort;s->event_player_prefix=*b;
  s->event_player_prefix.player_abort=&s->event_player_abort;s->event_player_prefix.player_clear_0012c418=NULL;
  /* 12C418 may restore the ordinary camera before 126770 when the
   * authored event starts during Roulette. Both callers use 124F60(0,0). */
  if(!s->event_player_prefix.camera_reset_0013b9b8_then_00138d40){
   s->event_player_prefix.camera_reset_0013b9b8_then_00138d40=room_soft_end_camera_both;
   s->event_player_prefix.context=s;
  }
  gh_event_player_reset_bindings bindings={.prefix=&s->event_player_prefix,.motion=&s->player_motion,
   .motion_bindings={.model=&s->player_model,.resource=s->player_factory.resource.bytes,
    .resource_bytes=s->player_factory.file_bytes,.work=s->b.work,.effect=player_motion_effect,.context=s,.ik_globals=s->b.motion_ik_globals,
    .save=s->b.save,.save_binding_known=1,.sound=s->player_sound.bound?gh_actor_attached_sound_motion:NULL,.sound_context=s->player_sound.bound?&s->player_sound:NULL},
   .solve={.model=&s->player_model,.work=s->b.work,.actors=&s->scene_tables,.ik_globals=s->b.motion_ik_globals,
    .atan_coeff=s->b.camera_envelope?s->b.camera_envelope->atan_coeff:NULL},
   .init=s->b.init,.canonical_rate=s->b.rate,.weapon_slot=&s->player_weapon,.policy=GH_STATIC_BINARY32_REFERENCE};
  e=gh_event_player_reset_open(&s->event_player_reset,&bindings);
 }
 if(!e)e=gh_event_player_reset_step_0012c418(&s->event_player_reset);
 *frontier=s->event_player_reset.frontier;return e;
}
gh_game_services_status gh_game_services_update_diagnostic(const gh_game_services*s,gh_game_update_diagnostic*out){
 if(!s||!out)return GH_SERVICES_ARGUMENT;*out=s->update_diagnostic;
 out->player_stage=s->player_frame.stage;out->player_frontier=s->player_frame.frontier;
 out->player_handle=s->player_factory.complete?s->player_factory.handle:0;
 out->player_state_entry=s->player_frame.state_entry;
 out->child_stage=s->child_update.original.stage;out->child_frontier=s->child_update.original.frontier;
 out->child_forces_complete=s->child_update.original.forces.complete;out->child_angles_complete=s->child_update.original.angles.complete;
 out->child_distances_complete=s->child_update.original.distances.complete;out->child_finish_complete=s->child_update.original.finish.complete;
 if(s->effects.ready){
  for(unsigned i=0;i<1024;i++){uint32_t bits;memcpy(&bits,s->scene_buffer_a+0x100000+(i>>5)*4,4);if(bits&(UINT32_C(0x80000000)>>(i&31)))out->esp_active++;}
  for(unsigned i=0;i<256;i++){uint32_t bits;memcpy(&bits,s->scene_buffer_b+0x26000+(i>>5)*4,4);if(!(bits&(UINT32_C(0x80000000)>>(i&31))))continue;out->emitter_active++;
   if(out->emitter_samples<4){unsigned j=out->emitter_samples++;const uint8_t*p=s->scene_buffer_b+(size_t)i*0x260;out->emitter_slots[j]=i;
    out->emitter_banks[j]=(uint32_t)p[0x18c]|(uint32_t)p[0x18d]<<8;out->emitter_kinds[j]=(uint32_t)p[0x18e]|(uint32_t)p[0x18f]<<8;memcpy(out->emitter_flags+j,p+0x118,4);
    gh_effect_emitter_view view={0};if(!gh_effect_create_emitter_at(&s->effects,i,&view)&&view.initialized&&view.record.bytes&&view.record.size>0x128){out->emitter_known[j]=1;out->emitter_types[j]=view.record.bytes[0x128];}
   }
  }
 }return GH_SERVICES_OK;
}
gh_game_services_status gh_game_services_get_scene_view(gh_game_services*s,gh_game_services_scene_view*out){
 if(!s||!out)return GH_SERVICES_ARGUMENT;
 *out=(gh_game_services_scene_view){.root=&s->b,.prepare=&s->prepare,.scenario=&s->scenario,
  .scenario_threads=s->scenario_threads,.scenario_bytes=s->scenario_scalar,.scenario_bytes_size=sizeof(s->scenario_scalar),.room_state=&s->room_state,.room_pack=&s->room_pack,.pools=&s->pools,
  .pl_camera=&s->camera,.cockpit=s->cockpit,.objects=s->objects,.room_objects=s->room_objects,
  .module=&s->module,.sound_scene=s->b.sound_scene,.collision_pairs=&s->collision_pairs,
  .effects=&s->effects,.filters=&s->filters,.model_effect=&s->model_effect,
  .world_light=&s->world_light,.model_packets=&s->model_packets,
  .work5f8=&s->work5f8,.event_weapon=room_event_weapon_slot(s),.player_model=&s->player_model,
  .collision_solids=&s->collision_solids,.scene_tables=&s->scene_tables,.title_player=&s->title_player,
  .checkpoint=&s->checkpoint,.scene_ui=&s->scene_ui,.scene_at=&s->scene_at,.action_buttons=&s->action_buttons,
  .em_placements=&s->em_placements,.om_placements=&s->om_placements,.acd=&s->acd,.step=&s->step,.event=s->b.event,.display=s->b.display,.title=s->title,
  .scenario_exit=&s->scenario_exit,.scenario_callback104=&s->scenario_callback104,.door=&s->door,.door_wait=&s->door.wait,.system_menu_enabled=&s->system_menu_enabled,
  .meshes=&s->meshes,
  .room_texture_channel=s->room_texture_channel,.room_texture_ready=s->room_texture_ready,
  .main_word4=&s->main_word4,.main_records=s->main_records,.main_record_count=32,
  .scene_buffer_a=s->scene_buffer_a,.scene_buffer_b=s->scene_buffer_b,
  .scene_buffer_a_size=sizeof(s->scene_buffer_a),.scene_buffer_b_size=sizeof(s->scene_buffer_b)};
 return GH_SERVICES_OK;
}
gh_game_services_status gh_game_services_begin_next_room(gh_game_services*s){
 if(!s||(s->b.loop->game_state[0]!=2&&s->b.loop->game_state[0]!=3))return GH_SERVICES_ARGUMENT;
 s->case3_stage=0;s->case3_reloaded_player=0;s->frontier_module_code=0;s->frontier=0;s->detail=0;s->status=GH_SERVICES_OK;return GH_SERVICES_OK;
}
uint64_t gh_game_services_ui_resource_epoch(const gh_game_services*s){return s?s->ui_resource_epoch:0;}
