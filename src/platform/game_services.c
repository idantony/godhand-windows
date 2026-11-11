#include "../core/game_player_square_frame.h"
#include "../core/game_player_attack_hitarea.h"
#include "game_services.h"
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
#include "../core/game_player_factory.h"
#include "../core/game_actor_scroll_links.h"
#include "../core/game_ray_static.h"
#include "../core/game_player_init.h"
#include "../core/game_actor_model.h"
#include "../core/game_collision_solid_units.h"
#include "../core/game_player_init_tail.h"
#include "../core/game_player_springs.h"
#include "../core/game_player_spawn.h"
#include "../core/game_om_initialize.h"
#include "../core/game_hitarea_shapes.h"
#include "../core/game_actor_motion_step.h"
#include "../core/game_actor_motion_call.h"
#include "../core/game_actor_motion_solve.h"
#include "../core/game_effect_release.h"
#include "../rooms/r100_roof_task.h"
#include "../rooms/r100_counter_task.h"
#include "../rooms/r100_live_tasks.h"
#include "../rooms/r100_init_tail.h"
#include "../core/game_cockpit_tutorial.h"
#include "../core/game_player_abort.h"
#include "../core/game_om3eb.h"
#include "../core/game_enemy_resources.h"
#include "../core/game_enemy_ems.h"
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
 gh_game_scene_step step;gh_r020_title*title;gh_scene_hook scenario_exit,scenario_callback104;
 gh_scene_meshes meshes;
 gh_r100_init r100;
 gh_r100_roof_task r100_roof;
 gh_r100_scripts r100_scripts;gh_r100_counter_task r100_counter;gh_r100_live_tasks r100_live;
 gh_soft_event_end r100_soft_end;
 gh_r100_init_tail r100_tail;gh_r100_init_tail_bindings r100_tail_bindings;
 gh_cockpit_tutorial tutorial;gh_object_resource_load tutorial_load;
 gh_om_runtime_create tutorial_create[4];gh_om_object *tutorial_objects[4];
 gh_game_player_abort soft_player_abort;
 gh_event_player_reset event_player_reset;gh_event_clear_bindings event_player_prefix;
 gh_player_abort_bindings event_player_abort;gh_event_weapon_slot player_weapon;
 gh_om_factory om_factory;
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
 } event_actor_move[10];
 gh_actor_render_call event_actor_render[10];
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
 uint8_t player_square_camera_pose_done;
 gh_actor_motion player_motion;
 gh_actor_motion_call player_motion_call;
 gh_game_player_frame player_frame;gh_effect_release_bindings player_frame_effects;
 gh_player_idle_input player_idle_input;gh_player_idle_input_bindings player_idle_bindings;
 gh_effect_create_call player_effect_create;
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
 detach_bundle(&s->om_placements.file,base,bytes);detach_bundle(&s->om_placements.entries,base,bytes);
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
 if(index>=sizeof(s->esp14_slots)/sizeof(*s->esp14_slots)||view->type!=0x14||!view->serial)return NULL;
 gh_services_esp14_slot *slot=s->esp14_slots[index];
 return slot&&slot->serial==view->serial?slot:NULL;
}

static gh_effect_status initialize_model_effect(void *context,const gh_effect_esp_view *view,unsigned index,
 uint32_t parameter_40,int *original_return,uint32_t *frontier)
{
 gh_game_services *s=context;
 *frontier=0x2f1d58;
 if(!view||view->type!=0x14||!view->serial||index>=sizeof(s->esp14_slots)/sizeof(*s->esp14_slots))
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
 return gh_esp14_initialize_002f1d58(&slot->instance,&bindings,view,index,parameter_40,original_return,frontier);
}

static gh_effect_status move_model_effect(void *context,const gh_effect_esp_view *view,unsigned index,
 int active,uint32_t *frontier)
{
 gh_services_esp14_slot *slot=model_effect_slot(context,view,index);
 if(!slot)return GH_EFFECT_ARGUMENT;
 const gh_esp14_bindings bindings=model_effect_bindings(slot);
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
    .policy=GH_STATIC_BINARY32_REFERENCE},
   .cockpit=s->cockpit,.ik=s->b.motion_ik_globals,.actors=&s->scene_tables,
   .atan_coeff=s->b.camera_envelope?s->b.camera_envelope->atan_coeff:NULL,.canonical_rate=&s->b.work->canonical_rate574,
   .effect=enemy_motion_effect,.effect_context=s,.solids=&s->collision_solids,.default_heap=s->b.assets->heap,
   .node_names=&s->enemy_nodes,.shapes={&s->collision_pairs,s->b.math_globals,GH_STATIC_BINARY32_REFERENCE},
   .numeric_policy=GH_EM_NUMERIC_PCSX2_CHOPZERO_REFERENCE,.weapon_scroll=&s->r100.collision},
  .names_2a2=&s->enemy2a2_nodes};
 return enemy_place_error(gh_enemy_ems_spawn_00294ca0(&s->enemies,&s->em_resources,&s->room_state,&s->enemy_bindings,f,detail));
}
static int player_motion_request_matches(gh_game_services*s,const gh_player_motion_request*r){
 return r&&r->player==&s->title_player&&r->model==&s->player_model&&r->work==s->b.work&&
  r->resource==s->player_factory.resource.bytes&&r->resource_bytes==s->player_factory.file_bytes&&r->rate==s->b.rate;
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
static gh_soft_event_status room_soft_player_abort(void*p,uint32_t*frontier){
 gh_game_services*s=p;gh_player_abort_bindings b={.player=&s->title_player,.sound=s->b.sound_control};
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
static gh_game_event_status room_soft_end_player(void *context,const gh_title_player_view *player,uint32_t *frontier)
{
 gh_game_services *s=context;
 if(player!=&s->title_player.view||!s->player_weapon.known)return GH_EVENT_ARGUMENT;
 const gh_soft_player_suspend_links links={.child_5f8=s->work5f8?&s->work5f8->object.view:NULL,
  .weapon_6a8=s->player_weapon.current?s->player_weapon.current->weapon:NULL,.known_mask=15};
 return gh_soft_player_unsuspend_0012c7d0(player,&links,frontier);
}
static gh_r100_script_status room_r100_end_soft(void *context,uint32_t *frontier)
{
 gh_game_services *s=context;
 const gh_event_start_bindings objects={.event=s->b.event,.config=s->b.event_config,.work=s->b.work,
  .tables=&s->scene_tables,.pools=&s->pools,.meshes=&s->meshes,.object_view=room_soft_end_object,.context=s};
 const gh_soft_end_bindings bindings={
  .scenario={s->scenario_scalar,sizeof(s->scenario_scalar),&s->scenario},.work=s->b.work,
  .current_player=&s->title_player.view,.room_objects=&objects,.player_clear_0012c418=room_soft_end_clear,
  .camera_pose_0013b9b8=room_soft_end_camera_pose,.camera_switch_00138d40=room_soft_end_camera_switch,
  .enemies_unsuspend_002948e8=room_soft_end_enemies,.player_unsuspend_0012c7d0=room_soft_end_player,.context=s};
 gh_game_event_status status=gh_scenario_end_soft_event_002c3c10(&s->r100_soft_end,&bindings,frontier);
 return !status?GH_R100_SCRIPT_OK:status==GH_EVENT_UNSUPPORTED?GH_R100_SCRIPT_UNSUPPORTED:GH_R100_SCRIPT_CALLEE;
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
 *frontier=operation==0?0x1bd178:operation==1?0x1bd3f0:0x1bcfb0;
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
  else return GH_R100_SCRIPT_UNSUPPORTED;
  if(!e){*frontier=0;return GH_R100_SCRIPT_OK;}
  return e==GH_PLACE_UNSUPPORTED?GH_R100_SCRIPT_UNSUPPORTED:GH_R100_SCRIPT_CALLEE;
 }
 return GH_R100_SCRIPT_UNSUPPORTED;
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
   .enemies=&s->enemies,.checkpoint=room_r100_checkpoint,.context=s};
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
static gh_r100_init_status room_tutorial_create(void*p,uint16_t kind,uint16_t id,gh_r100_om_view*out,uint32_t*frontier){
 gh_game_services*s=p;*out=(gh_r100_om_view){0};
 if(s->r100_tail.stage<4||s->r100_tail.stage>=16||(s->r100_tail.stage-4)%3)return GH_R100_INIT_ARGUMENT;
 unsigned index=(s->r100_tail.stage-4)/3;
 gh_scene_placement_status e=gh_om_factory_create_0030fa98(&s->om_factory,&s->tutorial_create[index],kind,id,&s->tutorial_objects[index]);
 if(frontier)*frontier=s->om_factory.frontier;
 if(e)return e==GH_PLACE_UNSUPPORTED?GH_R100_INIT_UNSUPPORTED:GH_R100_INIT_CALLEE;
 gh_om_object*o=s->tutorial_objects[index];if(o)*out=(gh_r100_om_view){o->view.bytes,o->view.size,o->view.bytes};
 return GH_R100_INIT_OK;
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
 s->meshes.factory_serial=b->factory_serial;
 s->old_context=b->stage->context;s->old_children=b->stage->init_children_15fd98;s->old_cockpit=b->stage->init_cockpit_140c88;
 oe=gh_game_objects_create(&b->stage->global_slots,0,&s->objects);if(oe)goto create_failure;
 oe=gh_game_objects_create(&b->stage->room_slots,1,&s->room_objects);if(oe)goto create_failure;
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
 s->case3_stage=0;s->frontier_module_code=0;
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
 if(s->b.save.bytes[0xae]!=s->b.save.bytes[0xaf]||(u32(raw+0x588)&0x800000)||(flags&0x40))
  return fail(s,GH_SERVICES_FRONTIER,0x30d8e0,0);
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
     .initialize_motion=player_motion_initialize,.advance_motion=player_motion_advance,.solve_motion=player_motion_solve,.motion_context=s,.rate=s->b.rate};
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
    if(!s->om_factory.bound){
     gh_scene_dynamic_status de=gh_scene_dynamic_bind(&s->dynamic_objects,&s->scene_tables,s->b.assets->heap);
     if(de)return fail(s,GH_SERVICES_ASSET,0x12ead8,de);
     s->om_shapes=(gh_hitarea_shape_bindings){.pairs=&s->collision_pairs,.math=s->b.math_globals,.policy=GH_STATIC_BINARY32_REFERENCE};
     s->om_initializer=(gh_om_initializer_bindings){.work=work,.packet_heap=s->b.assets->heap,.fog_rgb=s->world_light.scalar+4,.init=s->b.init,
      .shapes=&s->om_shapes,.dynamic=&s->dynamic_objects,.door_shared=s->b.door_shared,.player=&s->title_player.view,.solids=&s->collision_solids};
     gh_om_factory_bindings ob={.pools=&s->pools,.work=work,.room_state=&s->room_state,.objects=s->objects,.room_objects=s->room_objects,.factory_serial=s->b.factory_serial,.resources=&s->object_resources,.initializer=&s->om_initializer};
     /* Runtime CreateObj follows the OMS loop. These objects share this array
      * and both canonical registries; their addresses must remain stable. */
     const gh_room_vector*om_registry=NULL;
     if(gh_room_registry_view(&s->pools,0x754c58,&om_registry))return fail(s,GH_SERVICES_ASSET,0x30fb70,GH_POOL_ARGUMENT);
     pe=gh_om_factory_bind(&s->om_factory,&ob,om_registry->capacity);if(pe)return fail(s,GH_SERVICES_ASSET,0x30fb70,pe);}
    pe=gh_om_scene_bind_native_001c3cb8(&s->om_placements,&oms,&s->om_factory,&result);
   }
   if(pe)return fail(s,pe==GH_PLACE_UNSUPPORTED?GH_SERVICES_FRONTIER:GH_SERVICES_ASSET,s->om_placements.frontier?s->om_placements.frontier:0x1c3cb8,pe);
  }s->case3_stage=29;
 }
 if(s->case3_stage==29){gh_bundle_entry acd={0};gh_bundle_error be=gh_bundle_find(&room,"ACD",0,&acd);if(be&&be!=GH_BUNDLE_NOT_FOUND)return fail(s,GH_SERVICES_ASSET,0x31ccf0,be);int result;gh_scene_placement_status pe=gh_acd_scene_bind_00137ae8(&s->acd,&acd,&result);if(pe)return fail(s,pe==GH_PLACE_UNSUPPORTED?GH_SERVICES_FRONTIER:GH_SERVICES_ASSET,s->acd.frontier?s->acd.frontier:0x137ae8,pe);s->case3_stage=30;}
 if(s->case3_stage==30){
  if(!s->module.bound||(work->room!=0x20&&work->room!=0x100))return fail(s,GH_SERVICES_FRONTIER,0x2c3440,GH_SCENE_UNSUPPORTED);
  s->module.hook8=(gh_scene_hook){GH_SCENE_HOOK_NATIVE,work->room==0x20?room_init_native:room_r100_init,s};s->module.hook12=(gh_scene_hook){GH_SCENE_HOOK_NATIVE,work->room==0x20?room_move_native:room_r100_move,s};
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
 if(s->case3_stage==34){if(!s->b.bgm_requests)return fail(s,GH_SERVICES_FRONTIER,0x2d0850,0);gh_bgm_status be=gh_bgm_requests_scene_begin_002d0850(s->b.bgm_requests,work,*s->b.children,s->b.init);if(be)return fail(s,be==GH_BGM_UNSUPPORTED?GH_SERVICES_FRONTIER:GH_SERVICES_ASSET,gh_bgm_requests_frontier(s->b.bgm_requests),be);s->case3_stage=35;}
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
const char *gh_game_services_frontier_module(const gh_game_services*s){return s?(s->frontier_module_code==1?"r020.rel":s->frontier_module_code==2?"r100.rel":NULL):NULL;}
gh_game_services_status gh_game_services_destroy(gh_game_services*s){
 if(!s)return GH_SERVICES_ARGUMENT;
 if((s->owns_children&&*s->b.children!=s->created_children)||(s->owns_textures&&s->b.stage->textures!=s->created_textures))return fail(s,GH_SERVICES_BUSY,0x2a5bb8,0);
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
 {gh_tutorial_status e=gh_cockpit_tutorial_release(&s->tutorial);if(e)return fail(s,GH_SERVICES_ASSET,s->tutorial.frontier?s->tutorial.frontier:0x147188,e);}
 if(s->cockpit_snapshot&&(s->b.id->pool_used!=s->cockpit_pool_end||s->b.id->job_count!=s->cockpit_jobs_end))return fail(s,GH_SERVICES_BUSY,0x2aa078,GH_ID_BUSY);
 /* Native shutdown owns this unfinished construction prefix; this is not the
  * original scene-exit destructor for initialized world actors. */
 gh_scene_nearcheck_release(&s->step.nearcheck);
  /* Native destruction detaches effect companions before borrowed actors and
   * models. It neither acknowledges original Kill nor clears live masks. */
  {gh_voice_status e=gh_actor_attached_sound_abort(&s->player_sound);if(e)return fail(s,GH_SERVICES_ASSET,s->player_sound.frontier?s->player_sound.frontier:0x2cba48,e);}
  {gh_voice_status e=gh_actor_attached_sound_abort(&s->enemy_sound);if(e)return fail(s,GH_SERVICES_ASSET,s->enemy_sound.frontier?s->enemy_sound.frontier:0x2cba48,e);}
  {gh_voice_status e=gh_actor_attached_sound_abort(&s->om_sound);if(e)return fail(s,GH_SERVICES_ASSET,s->om_sound.frontier?s->om_sound.frontier:0x2cba48,e);}
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
   uint32_t f=0;gh_actor_motion_status e=gh_enemy_ems_release_native(&s->enemies,&f);
   if(e)return fail(s,GH_SERVICES_ASSET,f?f:0x290770,e);
  }
  free(s->enemy_renders);s->enemy_renders=NULL;
  if(gh_r100_init_release(&s->r100))return fail(s,GH_SERVICES_ASSET,0x208,0);
 if(gh_scene_at_units_release(&s->scene_at))return fail(s,GH_SERVICES_ASSET,0x2c1270,0);
 gh_scene_dynamic_cancel_update(&s->dynamic_objects);
 gh_scene_dynamic_forget_sphere_actor(&s->dynamic_objects,&s->title_player.view);
 gh_scene_dynamic_forget_sphere_actor(&s->dynamic_objects,&s->player_child.object.view);
 free(s->om_renders);s->om_renders=NULL;s->om_renders_capacity=0;
 s->player_render=(gh_actor_render_call){0};s->child_render=(gh_actor_render_call){0};
 free(s->om_frames);s->om_frames=NULL;s->om_frames_capacity=0;
 if(gh_om_factory_release(&s->om_factory))return fail(s,GH_SERVICES_ASSET,0x30f8b0,0);
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
    if (!status) s->event_actor_roster = roster;
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
    if (s->event_actor_extra && s->event_actor_extra->olivia_accessory.handle == handle)
        return &s->event_actor_extra->olivia_accessory;
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
static gh_player_target_status player_target_view(void*p,uint32_t handle,gh_title_player_view*v,uint32_t*f){
 gh_game_services*s=p;gh_game_services_status e=gh_game_services_actor_view(s,handle,v);if(e&&f)*f=0x30f16c;
 return !e?GH_PLAYER_TARGET_OK:e==GH_SERVICES_FRONTIER?GH_PLAYER_TARGET_UNSUPPORTED:GH_PLAYER_TARGET_CALLEE;
}
static gh_player_target_bindings scene_target_bindings(gh_game_services *s){
 return (gh_player_target_bindings){.pools=&s->pools,.actors=&s->scene_tables,.player=&s->title_player.view,
  .resolve=player_target_view,.context=s,.elf=s->b.boot_elf,.elf_bytes=s->b.boot_elf_bytes};
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
   .target_640=&s->player_square_target,.hit_area_0010a438=player_square_hit_area,
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
static gh_player_frame_status player_frame_effect_create(void*p,const gh_player_frame_effect_request*r,uint32_t*f){
 gh_game_services*s=p;if(!r||!r->actor||!f)return GH_PLAYER_FRAME_ARGUMENT;*f=0;
 if(r->actor->bytes!=s->title_player.view.bytes||r->argument3){*f=0x306528;return GH_PLAYER_FRAME_UNSUPPORTED;}
 int32_t mode;memcpy(&mode,&r->mode,4);
 gh_effect_create_request request={.bank=r->parameter_bits,.index=(uint32_t)r->kind,.actor=r->actor,.mode=mode,.selector=r->selector};
 gh_effect_create_call*c=&s->player_effect_create;gh_effect_status e;
 if(!c->active||c->complete){e=gh_effect_create_begin(c,&request);if(e)return GH_PLAYER_FRAME_ARGUMENT;}
 else if(c->request.bank!=request.bank||c->request.index!=request.index||c->request.actor!=request.actor||c->request.mode!=request.mode||c->request.selector!=request.selector)return GH_PLAYER_FRAME_BUSY;
 gh_effect_create_bindings b={.storage=player_effect_storage(s),.work=s->b.work,.random=&s->b.init->random,.model=&s->player_model};
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
 if(entry!=0x2b6a40)return GH_EVENT_UNSUPPORTED;
 if(!s->event_actor_extra||!s->event_actor_extra->bindings.creation)return GH_EVENT_ARGUMENT;
 gh_event_components *components=s->event_actor_extra->bindings.creation->bindings.components;
 return gh_event_actor_bust_002b6a40(actor,components,frontier);
}

static gh_object_step_status om_break_effect(void *context,const gh_actor_model *model,
 const uint32_t position[3],uint32_t yaw_bits,uint32_t *frontier) {
 gh_game_services *s=context;
 gh_effect_create_call *call=&s->om_break_effect;
 *frontier=0x306528;
 if(!s->b.camera_envelope)return GH_OBJSTEP_UNSUPPORTED;
 if(!call->active||call->complete) {
  if(gh_effect_create_om324_break_begin_001bfd28(call,position,yaw_bits))
   return GH_OBJSTEP_CALLEE;
  s->om_break_effect_owner=model;
 } else if(s->om_break_effect_owner!=model) return GH_OBJSTEP_ARGUMENT;
 const gh_effect_create_bindings bindings={.storage=player_effect_storage(s),
  .work=s->b.work,.random=&s->b.init->random,.atan_coeff=s->b.camera_envelope->atan_coeff};
 gh_effect_status status=gh_game_effect_create_00306528(call,&bindings);
 if(call->frontier)*frontier=call->frontier;
 if(status)return status==GH_EFFECT_UNSUPPORTED?GH_OBJSTEP_UNSUPPORTED:GH_OBJSTEP_CALLEE;
 if(!call->complete)return GH_OBJSTEP_CALLEE;
 s->om_break_effect_owner=NULL;
 *frontier=0;
 return GH_OBJSTEP_OK;
}

static gh_actor_motion_status om_break_sound(void *context,const gh_motion_sound_request *request,
 uint32_t *handle,uint32_t *frontier) {
 gh_game_services *s=context;
 if(actor_sound_prepare(s,&s->om_sound,frontier))return GH_ACTOR_MOTION_CALLEE;
 if(!s->om_sound.bound){*frontier=0x2cba48;return GH_ACTOR_MOTION_UNSUPPORTED;}
 return gh_actor_attached_sound_motion(&s->om_sound,request,handle,frontier);
}

static gh_object_step_status om_break_collision(void *context,uint64_t key,uint32_t *frontier) {
 gh_game_services *s=context;
 uint32_t changed;
 gh_scene_table_status status=gh_collision_scroll_set_key_enabled_001331b8(
     &s->scene_tables,&s->r100.collision,key,0,&changed);
 *frontier=status?0x1331b8:0;
 return status?GH_OBJSTEP_CALLEE:GH_OBJSTEP_OK;
}

static gh_object_step_status om_remove_sound(void *context,gh_actor_model *model,uint32_t *frontier) {
 gh_effect_status status=detach_model_effect_sound(context,model,frontier);
 return status?GH_OBJSTEP_UNSUPPORTED:GH_OBJSTEP_OK;
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
  if(entry!=0x2b6930||delta){*f=entry;return GH_SERVICES_FRONTIER;}
  if(gh_event_actor_factory_view(event_actor,handle,&registered)||v->bytes!=registered.bytes||
     v->size!=registered.size||v->alias_f0!=registered.alias_f0){*f=entry;return GH_SERVICES_ASSET;}
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
  if(enemy.other_move){
   gh_enemy_other_move *move=enemy.other_move;
   if(move->original_returned)*move=(gh_enemy_other_move){0};
   gh_actor_motion_status status;
   if(entry==0x2856c0&&enemy.enemy2a2){
    if(actor_sound_prepare(s,&s->enemy_sound,f))return GH_SERVICES_ASSET;
    s->enemy_opacity=(gh_object_opacity_environment){s->b.init,&s->camera,&s->title_player.view,1};
    if(!s->player_idle_bindings.targets.resolve)s->player_idle_bindings.targets=scene_target_bindings(s);
    status=gh_enemy_other_move_bind_targets(move,&s->player_idle_bindings.targets);
    if(!status)status=gh_enemy_other_move_bind_2a2_services(move,&s->enemy_opacity,enemy_motion_effect,s,
       s->enemy_sound.bound?gh_actor_attached_sound_motion:NULL,s->enemy_sound.bound?&s->enemy_sound:NULL);
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
  if(s->enemy_sound.bound){
   enemy.enemy202->motion.motion.b.sound=gh_actor_attached_sound_motion;
   enemy.enemy202->motion.motion.b.sound_context=&s->enemy_sound;
  }
  s->enemy_opacity=(gh_object_opacity_environment){s->b.init,&s->camera,&s->title_player.view,1};
  gh_actor_motion_status e=gh_enemy202_move_bind_navigation(move,&s->step.nearcheck);
  /* PL00 construction publishes +698 NULL; positive writers require a typed owner. */
  if(!e&&!move->stage&&s->title_player.view.size>=0x69c&&!u32(s->title_player.view.bytes+0x698))
   e=gh_enemy202_move_bind_player_aux_698(move,NULL);
  if(!e)e=gh_enemy202_move_bind_opacity(move,&s->enemy_opacity);
  if(!e)e=gh_enemy202_move_bind_near_sound(move,enemy_ema2_near_sound,s);
  if(!e)e=gh_enemy202_move_bind_room_state(move,&s->room_state);
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
  gh_om_frame_state*state=s->om_frames+index;
  gh_object_opacity_environment environment={s->b.init,&s->camera,&s->title_player.view,1};
  gh_object_step_status environment_status=gh_om_frame_bind_environment(state,&environment);
  if(environment_status){*f=state->frontier?state->frontier:entry;return GH_SERVICES_ASSET;}
  gh_om_frame_runtime runtime={.init=s->b.init,.canonical_rate=s->b.rate,
   .solids=&s->collision_solids,.context=s,.break_effect=om_break_effect,
   .break_sound=om_break_sound,.break_collision=om_break_collision,.remove_sound=om_remove_sound};
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
 gh_player_frame_bindings b={.player=&s->title_player,.work=s->b.work,.init=s->b.init,.camera=&s->camera,.save=s->b.save,
  .scenario={s->scenario_scalar,sizeof(s->scenario_scalar),&s->scenario},.effects=&s->player_frame_effects,
  .current_player=&s->title_player.view,.elf=s->b.boot_elf,.elf_bytes=s->b.boot_elf_bytes,.numeric_policy=GH_CAMERA_NATIVE_BINARY32_REFERENCE,
  .scene_at=&s->scene_at,.take_object=&s->player_tail.take_object,.give_objects=s->player_tail.give_objects,.solids=&s->collision_solids,
  .take_binding_known=1,.give_binding_known=1,.give_present_mask=31,
  .model=&s->player_model,.resource=s->player_factory.resource.bytes,.resource_bytes=s->player_factory.file_bytes,.rate=s->b.rate,
  .initialize_motion=player_motion_initialize,.advance_motion=player_frame_motion_advance,.motion_context=s,
  /* Original107E40 explicitly wrote698/6A0=NULL; no later recovered writer
     has published either. The frame also rejects any changed raw token. */
   .auxiliary_bindings_known=1,.effect_create=player_frame_effect_create,.effect_context=s,
   .idle_input=&s->player_idle_input,.idle_input_bindings=&s->player_idle_bindings,.node_cache=&s->player_tail.node_cache,
   .door_probe=player_frame_door_probe,.door_context=s,.actors=&s->scene_tables,
   .ground_globals=s->b.player_ground_globals,.ik_globals=s->b.motion_ik_globals,.atan_coeff=s->b.camera_envelope?s->b.camera_envelope->atan_coeff:NULL,
   .opacity_globals=s->b.player_opacity_globals,
   .aux_68c=s->player_tail.active_child&&s->player_tail.child_original_return?&s->player_child.object.view:NULL,.auxiliary_68c_binding_known=1,
   .target_644=&s->player_target_644,.cockpit_flags_1ca0=gh_game_cockpit_auxiliary_flags_1ca0(s->cockpit),
   .square_attack=player_square_attack,.square_context=s};
 if(!s->player_frame.bound)s->player_idle_bindings=(gh_player_idle_input_bindings){.frame=b,
  .targets=scene_target_bindings(s),.action_buttons=&s->action_buttons};
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
static void trace_render_frontier(const gh_game_services*s,const gh_object_trans_request*q,const gh_actor_model*m,const gh_actor_render_call*c,uint32_t frontier){
 const uint8_t*n=NULL;size_t mesh=0;uint32_t root_flags[2]={0},node_flags=0,node_tail=0,rt[4]={0},nt[4]={0};
 if(m->root.size>=0x258)memcpy(root_flags,m->root.bytes+0x250,8);
 if(m->root.size>=0x250)memcpy(rt,m->root.bytes+0x240,16);
 int known=gh_actor_model_node_at(m,(unsigned)c->draw.node_index,&n,&mesh)==GH_ACTOR_MODEL_OK;
 if(known){memcpy(&node_flags,n+0x380,4);memcpy(&node_tail,n+0x410,4);memcpy(nt,n+0x3a0,16);}
 fprintf(stderr,"GH_ACTOR_FRONTIER {\"model_kind\":\"%s\",\"part_count\":%u,\"node_count\":%llu,\"handle\":%u,\"entry\":\"%08X\",\"frontier\":\"%08X\",\"trans_phase\":%u,\"draw_phase\":%u,\"node_index\":%llu,\"node_known\":%d,\"mesh_index\":%llu,\"root250\":\"%08X\",\"root254\":\"%08X\",\"node380\":\"%08X\",\"node410\":\"%08X\",\"root_tint_bits\":[\"%08X\",\"%08X\",\"%08X\",\"%08X\"],\"node_tint_bits\":[\"%08X\",\"%08X\",\"%08X\",\"%08X\"]}\n",
 m==&s->player_model?"player":m==&s->player_child.model?"child":"om",(unsigned)m->part_count,(unsigned long long)m->script.mesh_count,q->handle,q->original_entry,frontier,c->trans.phase,c->draw.phase,(unsigned long long)c->draw.node_index,known,(unsigned long long)mesh,root_flags[0],root_flags[1],node_flags,node_tail,rt[0],rt[1],rt[2],rt[3],nt[0],nt[1],nt[2],nt[3]);
}
gh_game_services_status gh_game_services_trans_actor(gh_game_services*s,const gh_object_trans_request*q,
 const gh_boot_work*work,const gh_game_actor_render_provider*provider,uint32_t*f){
 if(f)*f=0;if(!s||!q||!provider||!f||work!=s->b.work)return GH_SERVICES_ARGUMENT;
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
  if(!s->enemy_renders){
   s->enemy_renders=calloc(128,sizeof(*s->enemy_renders));
   if(!s->enemy_renders){*f=0x14a3e8;return GH_SERVICES_MEMORY;}
  }
  model=enemy.model;shadow=enemy.shadow;call=&s->enemy_renders[2*enemy.physical_row+enemy.held_weapon];
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
 if(e){trace_render_frontier(s,q,model,call,*f);return e==GH_ACTOR_MODEL_UNSUPPORTED?GH_SERVICES_FRONTIER:GH_SERVICES_ASSET;}
 if(!call->trans.complete){*f=0x14a3e8;return GH_SERVICES_ASSET;}return GH_SERVICES_OK;
}

gh_scene_step_status gh_game_services_task_status(gh_game_services*s,uint32_t*frontier){
 if(!s||!frontier)return GH_SCENE_STEP_ARGUMENT;*frontier=0;
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
 const gh_title_player_view *aux=s->player_tail.active_child&&s->player_tail.child_original_return?&s->player_child.object.view:NULL;
 if((aux&&(!aux->bytes||aux->size<0x2f4))||(!aux&&u32(player->bytes+0x68c)))return GH_EVENT_UNSUPPORTED;
 uint8_t *raw=player->bytes;
 /* VT12CE60 writes position XYZ to all three views; VT2A9420 writes Euler. */
 memcpy(player->alias_f0,event->fields.position_bits,12);
 memcpy(raw+0x490,event->fields.position_bits,12);
 memcpy(raw+0xb0,event->fields.position_bits,12);
 const uint32_t euler[3]={0,event->fields.angle_bits,0};
 memcpy(raw+0x100,euler,12);
 memcpy(raw+0x30,event->fields.position_bits,12);
 if(aux){const uint32_t flags=u32(aux->bytes+0x2f0)|1u;memcpy(aux->bytes+0x2f0,&flags,4);}
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
  .work5f8=&s->work5f8,.event_weapon=&s->player_weapon,.player_model=&s->player_model,
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
 s->case3_stage=0;s->frontier_module_code=0;s->frontier=0;s->detail=0;s->status=GH_SERVICES_OK;return GH_SERVICES_OK;
}
uint64_t gh_game_services_ui_resource_epoch(const gh_game_services*s){return s?s->ui_resource_epoch:0;}
