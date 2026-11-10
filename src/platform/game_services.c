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
#include "../core/game_actor_motion_solve.h"
#include "../core/game_effect_release.h"
#include <stdlib.h>
#include <string.h>
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
 gh_game_scene_step step;gh_r020_title*title;gh_scene_hook scenario_exit,scenario_callback104;
 gh_scene_meshes meshes;
 gh_r100_init r100;
 gh_om_factory om_factory;
 gh_om_initializer_bindings om_initializer;
 gh_hitarea_shape_bindings om_shapes;gh_scene_dynamic_owner dynamic_objects;
 gh_game_player_factory player_factory;
 gh_object_resource_bindings object_resources;
 gh_game_player_init player_init;gh_actor_model player_model;
 gh_game_player_init_tail player_tail;gh_game_player_child player_child;
 gh_player_child_attachment player_attachment;
 gh_player_child_tail child_tail;gh_game_player_child *work5f8;
 gh_game_player_spawn player_spawn;
 gh_actor_motion player_motion;
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
 /* Actual2A6210 resets parentroomheap754220 immediately BEFORE31CC48.
  * Native allocations are split by owner: retire old bounded views here,
  * never destroy the host heap which also owns persistent/defaultheap data. */
 if(s->module.bound||s->title||s->pools.initialized||s->scenario_exit.kind!=GH_SCENE_HOOK_ABSENT)
  return fail(s,GH_SERVICES_BUSY,0x2a94f0,0);
 for(unsigned i=0;i<16;i++)if(s->scenario_records[i].running)return fail(s,GH_SERVICES_BUSY,0x2d59f0,0);
 for(size_t i=0;i<s->pools.object_slots;i++)if(s->pools.objects[i].object)return fail(s,GH_SERVICES_FRONTIER,0x30f8b0,0);
 if(s->meshes.descriptors)return fail(s,GH_SERVICES_FRONTIER,0x30f8b0,0);
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
 detach_table(&s->step.nearcheck.pairs,base,bytes);detach_table(&s->step.nearcheck.vertices,base,bytes);detach_table(&s->step.nearcheck.indices,base,bytes);
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
static gh_actor_motion_status player_motion_effect(void*p,const gh_title_player_view*actor,int32_t kind,uint32_t mode,uint32_t*frontier){
 gh_game_services*s=p;if(!actor||actor->bytes!=s->title_player.view.bytes)return GH_ACTOR_MOTION_ARGUMENT;
 gh_effect_release_bindings b={.effects=&s->effects,.esp=s->scene_buffer_a,.emitters=s->scene_buffer_b,
  .esp_bytes=sizeof(s->scene_buffer_a),.emitter_bytes=sizeof(s->scene_buffer_b)};unsigned slot=0;
 gh_effect_status e=gh_game_effect_release_00306200(&b,actor,kind,mode,frontier,&slot);
 return !e?GH_ACTOR_MOTION_OK:e==GH_EFFECT_UNSUPPORTED?GH_ACTOR_MOTION_UNSUPPORTED:GH_ACTOR_MOTION_CALLEE;
}
static int player_motion_request_matches(gh_game_services*s,const gh_player_motion_request*r){
 return r&&r->player==&s->title_player&&r->model==&s->player_model&&r->work==s->b.work&&
  r->resource==s->player_factory.resource.bytes&&r->resource_bytes==s->player_factory.file_bytes&&r->rate==s->b.rate;
}
static gh_player_init_status player_motion_initialize(void*p,const gh_player_motion_request*r,uint32_t*frontier){
 gh_game_services*s=p;if(!player_motion_request_matches(s,r))return GH_PLAYER_INIT_ARGUMENT;
 if(r->start_frame_bits||r->blend_frames||!r->release_previous_sequence){if(frontier)*frontier=0x2a8578;return GH_PLAYER_INIT_UNSUPPORTED;}
 gh_actor_motion_bindings b={.model=r->model,.resource=r->resource,.resource_bytes=r->resource_bytes,.work=r->work,
  .effect=player_motion_effect,.context=s,.ik_globals=s->b.motion_ik_globals};
 gh_actor_motion_status e=gh_actor_motion_begin_002a8578(&s->player_motion,&b,r->motion_offset,r->sequence_candidate_offset,r->flags);
 if(frontier)*frontier=s->player_motion.frontier;
 return !e?GH_PLAYER_INIT_OK:e==GH_ACTOR_MOTION_UNSUPPORTED?GH_PLAYER_INIT_UNSUPPORTED:e==GH_ACTOR_MOTION_BOUNDS?GH_PLAYER_INIT_BOUNDS:GH_PLAYER_INIT_CALLEE;
}
static gh_player_init_status player_motion_advance(void*p,const gh_player_motion_request*r,uint32_t*frontier){
 gh_game_services*s=p;if(!player_motion_request_matches(s,r)||!r->rate)return GH_PLAYER_INIT_ARGUMENT;
 int ended=0;gh_actor_motion_status e=gh_actor_motion_step_00157c18(&s->player_motion,*r->rate,GH_STATIC_BINARY32_REFERENCE,&ended,frontier);
 return !e?GH_PLAYER_INIT_OK:e==GH_ACTOR_MOTION_UNSUPPORTED?GH_PLAYER_INIT_UNSUPPORTED:e==GH_ACTOR_MOTION_BOUNDS?GH_PLAYER_INIT_BOUNDS:GH_PLAYER_INIT_CALLEE;
}
static gh_player_init_status player_motion_solve(void*p,const gh_player_motion_request*r,uint32_t*frontier){
 gh_game_services*s=p;if(!player_motion_request_matches(s,r)||!s->player_motion.complete)return GH_PLAYER_INIT_ARGUMENT;
 gh_motion_solve_bindings b={.model=&s->player_model,.work=s->b.work,.actors=&s->scene_tables};
 gh_actor_motion_status e=gh_actor_motion_solve_00152a20(&s->player_motion_solve,&b,GH_STATIC_BINARY32_REFERENCE,frontier);
 return !e?GH_PLAYER_INIT_OK:e==GH_ACTOR_MOTION_UNSUPPORTED?GH_PLAYER_INIT_UNSUPPORTED:e==GH_ACTOR_MOTION_BOUNDS?GH_PLAYER_INIT_BOUNDS:GH_PLAYER_INIT_CALLEE;
}
static gh_scene_status room_r100_init(void*p,uint32_t*frontier){
 gh_game_services*s=p;
 gh_r100_init_bindings b={.work=s->b.work,.room_state=&s->room_state,.at=&s->scene_at,
  .pools=&s->pools,.meshes=&s->meshes,.default_heap=s->b.assets->heap,.actor_tables=&s->scene_tables,
  .object_view=room_r100_om_view,.context=s};
 /* Positive OM lookup and Scenario scripts still require their real typed
  * implementations. Missing providers preserve the original call frontier. */
 gh_r100_init_status e=gh_r100_initialize_00000208(&s->r100,&b);
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
 if(b->camera_envelope)(void)gh_game_camera_envelope_bind_model(b->camera_envelope,&s->player_model);
 s->object_resources=(gh_object_resource_bindings){.dvd=b->assets->dvd,.heap=b->assets->heap,.threads=b->threads};
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
   pe=gh_em_scene_bind_00294c68(&s->em_placements,&s->room_state,&ems,&result);if(pe)return fail(s,pe==GH_PLACE_UNSUPPORTED?GH_SERVICES_FRONTIER:GH_SERVICES_ASSET,s->em_placements.frontier?s->em_placements.frontier:0x294c68,pe);
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
      .shapes=&s->om_shapes,.dynamic=&s->dynamic_objects,.door_shared=s->b.door_shared};
     gh_om_factory_bindings ob={.pools=&s->pools,.work=work,.room_state=&s->room_state,.objects=s->objects,.room_objects=s->room_objects,.factory_serial=s->b.factory_serial,.resources=&s->object_resources,.initializer=&s->om_initializer};
     pe=gh_om_factory_bind(&s->om_factory,&ob,s->om_placements.count);if(pe)return fail(s,GH_SERVICES_ASSET,0x30fb70,pe);}
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
 if(s->case3_stage==35){gh_game_scene_step_case3_cooldown(&s->step);if(work->room!=0x20)return fail(s,GH_SERVICES_FRONTIER,0x2db6b0,0);
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
 if(s->cockpit_snapshot&&(s->b.id->pool_used!=s->cockpit_pool_end||s->b.id->job_count!=s->cockpit_jobs_end))return fail(s,GH_SERVICES_BUSY,0x2aa078,GH_ID_BUSY);
 /* Native shutdown owns this unfinished construction prefix; this is not the
  * original scene-exit destructor for initialized world actors. */
 if(gh_r100_init_release(&s->r100))return fail(s,GH_SERVICES_ASSET,0x208,0);
 if(gh_scene_at_units_release(&s->scene_at))return fail(s,GH_SERVICES_ASSET,0x2c1270,0);
 {uint32_t f=0;gh_solid_unit_status se=gh_collision_solid_units_shutdown(&s->collision_solids,&f);if(se)return fail(s,GH_SERVICES_ASSET,f?f:0x1347e8,se);}
 if(gh_om_factory_release(&s->om_factory))return fail(s,GH_SERVICES_ASSET,0x30f8b0,0);
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
 if(s->scenario_threads){for(unsigned i=0;i<16;i++){gh_thread_status e=gh_task_threads_cancel(s->scenario_threads,(int32_t)i);if(e)return fail(s,GH_SERVICES_THREAD,0x2d59f0,e);}
  gh_thread_status e=gh_task_threads_destroy(s->scenario_threads);if(e)return fail(s,GH_SERVICES_THREAD,0x2d56a8,e);s->scenario_threads=NULL;}
 if(s->b.stage->context==s){s->b.stage->context=s->old_context;s->b.stage->init_children_15fd98=s->old_children;s->b.stage->init_cockpit_140c88=s->old_cockpit;}
 gh_game_objects_destroy(s->room_objects);gh_game_objects_destroy(s->objects);free(s);return GH_SERVICES_OK;
}

gh_game_services_status gh_game_services_actor_view(gh_game_services*s,uint32_t handle,gh_title_player_view*out){
 if(!s||!out)return GH_SERVICES_ARGUMENT;
 const gh_title_player_view *player=NULL;
 if(gh_game_player_factory_scalar_view(&s->player_factory,handle,&player)==GH_PLAYER_OK){*out=*player;return GH_SERVICES_OK;}
 if(s->player_child.handle==handle&&s->player_child.complete){*out=s->player_child.object.view;return GH_SERVICES_OK;}
 gh_scene_placement_status e=gh_om_factory_object_view(&s->om_factory,handle,out);
 return !e?GH_SERVICES_OK:e==GH_PLACE_UNSUPPORTED?GH_SERVICES_FRONTIER:GH_SERVICES_ASSET;
}
gh_game_services_status gh_game_services_get_scene_view(gh_game_services*s,gh_game_services_scene_view*out){
 if(!s||!out)return GH_SERVICES_ARGUMENT;
 *out=(gh_game_services_scene_view){.root=&s->b,.prepare=&s->prepare,.scenario=&s->scenario,
  .scenario_threads=s->scenario_threads,.scenario_bytes=s->scenario_scalar,.scenario_bytes_size=sizeof(s->scenario_scalar),.room_state=&s->room_state,.room_pack=&s->room_pack,.pools=&s->pools,
  .pl_camera=&s->camera,.cockpit=s->cockpit,.objects=s->objects,.room_objects=s->room_objects,
  .module=&s->module,.sound_scene=s->b.sound_scene,.collision_pairs=&s->collision_pairs,
  .effects=&s->effects,.filters=&s->filters,
  .world_light=&s->world_light,.model_packets=&s->model_packets,
  .work5f8=&s->work5f8,
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
