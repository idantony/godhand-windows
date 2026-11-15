#include "menu_state.h"
#include "game_scene_message_create.h"
#include "game_message_step.h"
#include "game_scene_message_draw.h"
#include <math.h>
#include "game_actor_motion_ik.h"
#include "game_camera_envelope.h"
#include "game_enemy_2ad.h"
#include "game_actor_model_trans.h"
#include "game_scene_placements.h"
#include "game_enemy_ems.h"
#include "game_effects_frame.h"
#include "disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static uint8_t *load(gh_disc *disc,const char *name,size_t *n){
 size_t index;gh_disc_entry e;
 if(gh_disc_find(disc,name,&index)||gh_disc_entry_at(disc,index,&e))return NULL;
 *n=(size_t)e.size;uint8_t *p=malloc(*n);
 if(p&&gh_disc_read(disc,index,0,p,*n)){free(p);p=NULL;}
 return p;
}
static uint32_t word(const uint8_t *p){uint32_t x;memcpy(&x,p,4);return x;}
static uint32_t effect_clock(void *context){return ++*(uint32_t *)context;}
static gh_voice_status emad_probe(void *ctx,const float *position,int seh,
 gh_se_attached_environment *out,uint32_t *frontier){
 (void)ctx;(void)position;(void)seh;(void)frontier;
 *out=(gh_se_attached_environment){.output_mode=1,.se_level_q8=0x5a00};
 return GH_VOICE_OK;
}
static uint64_t pcm_hash(const int16_t *samples,size_t frames){
 uint64_t hash=UINT64_C(14695981039346656037);
 for(size_t i=0;i<frames;i++){
  uint16_t value=(uint16_t)samples[i];
  hash=(hash^(value&255u))*UINT64_C(1099511628211);
  hash=(hash^(value>>8))*UINT64_C(1099511628211);
 }
 return hash;
}
typedef struct { unsigned abort_calls,task_calls; } scenery_action_probe;
static gh_soft_event_status scenery_abort_probe(void *context,uint32_t *frontier){
 scenery_action_probe *probe=context;
 probe->abort_calls++;
 *frontier=0;
 return GH_SOFT_EVENT_OK;
}
static void scenery_task_probe(void *context,int32_t argument){
 scenery_action_probe *probe=context;
 if(!argument)probe->task_calls++;
}
typedef struct {
 unsigned calls;
 size_t count;
 gh_id_text_glyph normal[32],shadow[32];
} scenery_glyph_capture;
static gh_scene_ui_status capture_scenery_glyphs(void *context,const gh_scene_message_glyph_pair *draw,
 uint32_t *frontier){
 scenery_glyph_capture *capture=context;
 if(!draw->count||draw->count>32||draw->bank!=6||draw->depth!=2||draw->font_mode!=0x21)
   return GH_SCENE_UI_ARGUMENT;
 capture->calls++;
 capture->count=draw->count;
 memcpy(capture->normal,draw->normal_run,draw->count*sizeof(*capture->normal));
 memcpy(capture->shadow,draw->shadow_run,draw->count*sizeof(*capture->shadow));
 *frontier=0;
 return GH_SCENE_UI_OK;
}
int main(int argc,char **argv){
 C(argc==2);gh_disc *disc=NULL;C(!gh_disc_open(argv[1],&disc));
 size_t room_size,resource_size;
 uint8_t *room_data=load(disc,"st1/r105.dat",&room_size);
 uint8_t *resource_data=load(disc,"em/emad.dat",&resource_size);
 C(room_data&&resource_data);
 gh_bundle bundle;gh_bundle_entry ems;
 C(!gh_bundle_open(room_data,room_size,&bundle)&&!gh_bundle_find(&bundle,"EMS",0,&ems));
 gh_game_prepare prepare={0};uint8_t stage=0,stop=1;
 gh_room_state room;
 C(!gh_game_prepare_002a5d40(&prepare,(gh_core_save_view){0},0x105,&stage));
 C(!gh_room_state_select_002bebc8(&prepare,0x105,0,&room));
 gh_boot_work work={.room=0x105};
 gh_em_scene_placements placements={.stop_flag=&stop};
 int original;
 C(!gh_em_scene_bind_native_00294c68(&placements,&room,&ems,&work,0,GH_EM_NUMERIC_PCSX2_CHOPZERO_REFERENCE,&original)&&original);
 gh_enemy_request request;int spawn;int16_t saved;uint32_t pc;
 C(!gh_enemy_request_from_room_00294ca0(&room,8,GH_EM_NUMERIC_PCSX2_CHOPZERO_REFERENCE,&request,&spawn,&saved,&pc));
 C(spawn&&request.entry_number==63&&request.object_kind==0x2ad&&request.dependency_kind==0x2ad);
 gh_heap *heap=NULL;gh_game_title_player actor={0};C(!gh_heap_create(32*1024*1024,&heap));
 memcpy((uint8_t *)&work+0x594,&(uint32_t){0x100000},4);
 C(!gh_enemy2ad_allocate_00313970(&actor,heap,&work,&original)&&!original&&!actor.ready&&!gh_heap_used(heap));
 memcpy((uint8_t *)&work+0x594,&(uint32_t){0},4);
 C(!gh_enemy2ad_allocate_00313970(&actor,heap,&work,&original)&&original&&actor.ready);
 C(actor.view.size==GH_EM2AD_BYTES&&actor.view.original_vtable214==0x44a528&&
    actor.view.original_kind4ac==0x44a510&&(uintptr_t)actor.view.bytes%16==0);
 C(word(actor.view.bytes+0x1520)==0&&word(actor.view.bytes+0x152c)==0x3f800000&&actor.view.bytes[0x1541]==255);
 C(gh_enemy2ad_allocate_00313970(&actor,heap,&work,&original)==GH_PLAYER_BUSY&&!original);
 gh_object_view resource={.bytes=resource_data,.size=resource_size,.kind=0x2ad,
    .file_extent_known=1,.file_extent=resource_size};
 gh_effect_view effect={0};
 C(!gh_enemy2ad_effect_data(&resource,&effect));
 printf("EMAD extent %zu D7 relative %08x magic %.4s\n",resource_size,word(resource_data+0x10),(const char *)effect.bytes);
 C(resource_size==93952&&word(resource_data+0x10)==0x15540&&effect.bytes==resource_data+0x15540);
 gh_bundle effect_bundle;gh_bundle_entry emd;
 C(!gh_bundle_open(effect.bytes,effect.size,&effect_bundle)&&!gh_bundle_find(&effect_bundle,"EMD",0,&emd));
 printf("D7 EMD: count %u [%08x]\n",word(emd.bytes+4),word(emd.bytes+16));
 gh_room_pools pools={0};gh_enemy_model model={0};uint32_t fog=0;
 C(!gh_game_room_pools_initialize_002a5d80(&pools,&bundle,heap));
 const gh_actor_model_status model_status=gh_enemy2ad_model_0028ea38(&model,&actor.view,&resource,&work,
    &pools,heap,&fog,GH_STATIC_BINARY32_REFERENCE,&original);
 printf("EMAD model status%d original%d frontier%x nodes%u parts%u\n",model_status,original,model.frontier,
    model.model.script.mesh_count,model.model.part_count);
 C(!model_status&&original&&model.model.ready);
 C(model.model.part_count==31&&model.model.script.mesh_count==4&&model.geometry.bytes&&model.texture.bytes);
 const size_t model_heap_live=gh_heap_used(heap);
 C(!gh_enemy2ad_model_0028ea38(&model,&actor.view,&resource,&work,&pools,heap,&fog,GH_STATIC_BINARY32_REFERENCE,&original)&&
    original&&gh_heap_used(heap)==model_heap_live);
 C(!gh_enemy_model_release_prefix(&model));
 size_t core_size;uint8_t *core=load(disc,"etc/core.dat",&core_size);
 uint8_t *esp=calloc(1,0x100180),*emit=calloc(1,0x26060);
 gh_game_effects *effects=calloc(1,sizeof(*effects));
 C(core&&esp&&emit&&effects&&!gh_game_effects_initialize_00305a68(effects,esp,0x100180,emit,0x26060,core,core_size,&bundle));
 gh_effect_view prior_entries[64];memcpy(prior_entries,effects->emitter_entries,sizeof(prior_entries));
 uint8_t save_bytes[GH_CORE_SAVE_BYTES]={0};gh_core_save_view save={save_bytes,sizeof(save_bytes)};
 gh_enemy2ad_init skipped={0},live_init={0};
 memcpy(save_bytes+0x14,&(uint32_t){0x4000},4);
 C(!gh_enemy2ad_init_prefix_0028e0e0(&skipped,&actor.view,&request,&resource,&work,
    save,effects,&pools,heap,&fog,NULL,NULL,GH_STATIC_BINARY32_REFERENCE,&original));
 C(!original&&skipped.stage==5&&!skipped.model.stage&&effects->emd_tables[0xd7].bytes==emd.bytes);
 unsigned bank=0;while(bank<64&&effects->resources[bank].kind!=0xd7)bank++;
 C(bank<64&&effects->resources[bank].references==1);
 C(!gh_enemy2ad_init_prefix_0028e0e0(&skipped,&actor.view,&request,&resource,&work,
    save,effects,&pools,heap,&fog,NULL,NULL,GH_STATIC_BINARY32_REFERENCE,&original)&&!original&&effects->resources[bank].references==1);
 memset(save_bytes+0x14,0,4);
 C(gh_enemy2ad_init_prefix_0028e0e0(&live_init,&actor.view,&request,&resource,&work,
    save,effects,&pools,heap,&fog,NULL,NULL,GH_STATIC_BINARY32_REFERENCE,&original)==GH_ACTOR_MODEL_UNSUPPORTED);
 C(!original&&live_init.frontier==0x2abbe8&&live_init.model.model.ready&&
    live_init.model.model.part_count==31&&effects->resources[bank].references==2);
 C(actor.view.bytes[0x1541]==63&&word(actor.view.bytes+0x564)==0x2ad&&
    !memcmp(prior_entries,effects->emitter_entries,sizeof(prior_entries)));
 C(gh_enemy2ad_init_prefix_0028e0e0(&live_init,&actor.view,&request,&resource,&work,
    save,effects,&pools,heap,&fog,NULL,NULL,GH_STATIC_BINARY32_REFERENCE,&original)==GH_ACTOR_MODEL_UNSUPPORTED);
 C(!original&&effects->resources[bank].references==2);
 C(!gh_enemy_model_release_prefix(&live_init.model));
 gh_stage_slots root_slots={0},room_slots={0};
 gh_game_objects *root_objects,*room_objects;
 C(!gh_game_objects_create(&root_slots,0,&root_objects)&&!gh_game_objects_create(&room_slots,1,&room_objects));
 C(!gh_stage_slots_reset(&root_slots,NULL)&&!gh_stage_slots_reset(&room_slots,NULL));
 C(!gh_game_objects_add_pinned_001fece0(room_objects,0x2ad,resource_data,resource_size,&original)&&original);
 C(!gh_game_objects_set_file_extent(room_objects,0x2ad,resource_data,resource_size));
 gh_game_init_state *init=malloc(sizeof(*init));C(init);
 gh_game_init_state_initialize(init);C(!gh_game_enemy_reset_00290230(init));
 uint8_t scenario=0,serial=7;
 memcpy((uint8_t *)&work+0x5e4,&(uint32_t){0x20000000},4); /* Focused fixture has no SE loader. */
 gh_enemy_entry_bindings bindings={.init=init,.pools=&pools,.room_objects=room_objects,.global_objects=root_objects,
    .work=&work,.room_heap=heap,.packet_heap=heap,.fog_rgb=&fog,.save=save,.scenario14=&scenario,
    .factory_serial=&serial,.policy=GH_STATIC_BINARY32_REFERENCE};
 gh_enemy_entry entry={0};
 C(!gh_enemy_entry_prepare_2ad_002905d0(&entry,&bindings,&request,&original)&&original&&entry.published&&entry.handle);
 C(entry.object.view.bytes[0x2fc]==7&&entry.object.view.bytes[0x640]==63&&serial==8&&
    entry.resource.bytes==resource_data&&init->enemy_actors[entry.slot]==&entry.object);
 const gh_room_vector *move_list,*actor_list;
 C(!gh_room_registry_view(&pools,0x754c30,&move_list)&&move_list->count==1);
 C(!gh_room_registry_view(&pools,0x754c58,&actor_list)&&actor_list->count==1);
 C(!gh_enemy_entry_prepare_2ad_002905d0(&entry,&bindings,&request,&original)&&original&&serial==8&&move_list->count==1);
 size_t language_size;
 uint8_t *language_data=load(disc,"etc/core_usa.bin",&language_size);C(language_data);
 gh_bundle language;gh_bundle_entry idd;
 C(!gh_bundle_open(language_data,language_size,&language)&&!gh_id_find_language_data(&language,&idd));
 gh_id_state *id=calloc(1,sizeof(*id));C(id);
 gh_id_runtime *runtime;gh_game_cockpit *cockpit;
 C(!gh_core_save_system_init(save));
 C(!gh_id_system_init_002acc70(id,&idd)&&!gh_id_runtime_create(id,1,&runtime));
 C(!gh_id_runtime_set_path_policy(runtime,GH_ID_PATH_REFERENCE_NEAREST));
 C(!gh_game_cockpit_create(runtime,id,&init->random,&cockpit)&&!gh_game_cockpit_initialize_00140c88(cockpit,save));
 gh_enemy2ad_init live={0};
 C(gh_enemy2ad_init_prefix_0028e0e0(&live,&entry.object.view,&request,&entry.resource,&work,
    save,effects,&pools,heap,&fog,cockpit,&init->random,GH_STATIC_BINARY32_REFERENCE,&original)==GH_ACTOR_MODEL_UNSUPPORTED);
 C(live.stage==4&&live.frontier==0x2a8578);
 size_t elf_size=(size_t)gh_disc_boot_elf_size(disc);
 uint8_t *elf=malloc(elf_size);C(elf&&!gh_disc_read_boot_elf(disc,0,elf,elf_size));
 gh_motion_ik_globals ik={0};gh_camera_envelope camera={0};gh_game_scene_tables tables={0};
 C(!gh_motion_ik_globals_from_elf(&ik,elf,elf_size)&&!gh_game_camera_envelope_from_elf(&camera,elf,elf_size,&pc));
 C(!gh_scene_actors_0012e670(&tables,&bundle));
 const float rate=1;
 gh_enemy2ad_tail_bindings tail_bindings={.rate=&rate,.solve={.model=&live.model.model,.work=&work,.actors=&tables,
     .ik_globals=&ik,.atan_coeff=camera.atan_coeff},.motion={.model=&live.model.model,.resource=resource_data,
     .resource_bytes=resource_size,.work=&work,.ik_globals=&ik,.save=save,.save_binding_known=1},
     .effect={.storage={.effects=effects,.esp=esp,.esp_bytes=0x100180,.emitters=emit,.emitter_bytes=0x26060},
     .work=&work,.random=&init->random,.model=&live.model.model,.atan_coeff=camera.atan_coeff}};
 gh_enemy2ad_init_tail tail={0};
 const gh_actor_model_status status=gh_enemy2ad_init_tail_0028e320(&tail,&live,&entry.object.view,
     &entry.resource,&tail_bindings,GH_STATIC_BINARY32_REFERENCE,&original);
 printf("R105 02AD Init tail status%d pc%08x stage%u effect step%u\n",status,tail.frontier,tail.stage,tail.effect.stage);
 C(!status&&original&&tail.stage==7&&tail.effect.complete&&tail.shadow.initialized);
 printf("D7:1 rows %u traversed %u original %d emitter-mask %08x\n",tail.effect.count,tail.effect.row,tail.effect.original_return,word(emit+0x26000));
 C(tail.effect.count==3&&tail.effect.row==3&&tail.effect.original_return&&word(emit+0x26000)==0xe0000000u);
 C(word(entry.object.view.bytes+0x110)==0x3f000000&&word(entry.object.view.bytes+0x114)==0x3f000000&&
   word(entry.object.view.bytes+0x118)==0x3f000000&&entry.object.view.bytes[0x140]==4&&
   !entry.object.view.bytes[0x531]&&!word(entry.object.view.bytes+0x1550));
 for(unsigned index=0;index<3;index++){
    gh_effect_emitter_view emitter;
    C(!gh_effect_create_emitter_at(effects,index,&emitter)&&emitter.actor==&entry.object.view&&
       emitter.selector==&entry.object.view&&emitter.model==&live.model.model&&
       gh_effect_emad_emitter_is_bound(effects,&emitter));
    printf("D7:1 emitter %u part %u class %u\n",index,emitter.record.bytes[0xbe],emitter.record.bytes[0x133]);
 }
 C(!gh_enemy2ad_init_tail_0028e320(&tail,&live,&entry.object.view,&entry.resource,
    &tail_bindings,GH_STATIC_BINARY32_REFERENCE,&original)&&original&&word(emit+0x26000)==0xe0000000u);
 gh_effects_frame_globals effect_globals={0};
 C(!gh_effects_frame_globals_from_elf(&effect_globals,elf,elf_size));
 effect_globals.factor_741dc0=0x3f800000u;
 gh_boot_work fx_work=work;
 fx_work.canonical_rate574=1;
 gh_game_scene_aux effect_profile={0};
 uint32_t effect_time=0;
 gh_effects_frame_bindings effect_frame={.storage=tail_bindings.effect.storage,.work=&fx_work,
     .init=init,.current_player=&actor.view,.globals=&effect_globals,.profile=&effect_profile,
     .clock=effect_clock,.clock_context=&effect_time,.atan_coeff=camera.atan_coeff,
     .policy=GH_STATIC_BINARY32_REFERENCE,.actors=&tables};
 gh_game_effects_frame effect_move={0};
 gh_effect_status frame_status=gh_game_effects_frame_update_0030f228(&effect_move,&effect_frame);
 printf("R105 D7 first update status%d pc%08x emitter%u ESP%u\n",
        frame_status,effect_move.frontier,effect_move.emitter_index,effect_move.esp_index);
 C(!frame_status&&!effect_move.stage);
 C(word(emit+0x114)&0x800000u);
 C(word(esp+0x100000)==0xf8000000u);
 for(unsigned index=0;index<5;index++){
    gh_effect_esp_view sprite;
    C(!gh_effect_create_esp_at(effects,index,&sprite)&&sprite.initialized&&sprite.type==0&&
       sprite.record.bytes[0x12e]==(index<2?14:index<4?43:54)&&
       gh_effect_emad_esp_is_bound(effects,&sprite));
 }
 uint8_t *flags250=entry.object.view.bytes+0x250;
 uint32_t retiring=word(flags250)|0xe0000002u;
 memcpy(flags250,&retiring,4);
 gh_effect_emitter_view retained;
 C(gh_effect_create_quiesce_actor_room_exit(effects,&entry.object.view)==GH_EFFECT_UNSUPPORTED);
 C(!gh_effect_create_emitter_at(effects,0,&retained));
 /* These are the actual State5 stop requests, with active masks still set. */
 for(unsigned i=0;i<3;i++){
    uint8_t *stop=emit+0x260*i+0x114;
    uint32_t value=word(stop)|0x40000000u;
    memcpy(stop,&value,4);
 }
 C(gh_effect_create_quiesce_actor_room_exit(effects,&entry.object.view)==GH_EFFECT_UNSUPPORTED);
 C(!gh_effect_create_emitter_at(effects,0,&retained));
 for(unsigned i=0;i<5;i++){
    gh_effect_esp_view sprite;
    C(!gh_effect_create_esp_at(effects,i,&sprite));
    C(!gh_effect_create_esp_pause_002fbe50(effects,i,sprite.serial,&pc));
 }
 C(!gh_effect_create_quiesce_actor_room_exit(effects,&entry.object.view));
 C(!gh_effect_create_quiesce_actor_room_exit(effects,&entry.object.view));
 int borrowed=1;
 C(!gh_effect_create_actor_has_live_refs(effects,&entry.object.view,&borrowed)&&!borrowed);
 C(!gh_effect_create_source_has_live_refs(effects,effect.bytes,effect.size,&borrowed)&&!borrowed);
 C(gh_effect_create_emitter_at(effects,0,&retained)==GH_EFFECT_UNSUPPORTED);
 C(word(emit+0x26000)==0xe0000000u&&word(esp+0x100000)==0xf8000000u);
 C(!gh_game_effects_release_2ad_00305778(effects,effect)&&effects->emd_tables[0xd7].bytes);
 gh_effect_create_forget_on_owner_destroy(effects);
 C(!gh_game_effects_release_2ad_native(effects,effect));
 C(!gh_game_effects_release_2ad_native(effects,effect)&&!effects->emd_tables[0xd7].bytes);
 C(!memcmp(prior_entries,effects->emitter_entries,sizeof(prior_entries)));
 C(!gh_enemy_model_release_prefix(&live.model));
 C(!gh_enemy_entry_abort_prefix(&entry)&&!move_list->count&&!actor_list->count&&!init->enemy_top);
 /* Start a new isolated effect owner after retiring the earlier fixture. */
 memset(esp,0,0x100180);
 memset(emit,0,0x26060);
 C(!gh_game_effects_initialize_00305a68(effects,esp,0x100180,emit,0x26060,core,core_size,&bundle));
 gh_game_action_buttons buttons={0};
 gh_game_action_buttons_reset_001f72c0(&buttons);
 gh_enemy_ems_bindings ems_bindings={
    .actor={.entry=bindings,.cockpit=cockpit,.ik=&ik,.actors=&tables,.atan_coeff=camera.atan_coeff,
        .canonical_rate=&rate,.numeric_policy=GH_EM_NUMERIC_PCSX2_CHOPZERO_REFERENCE},
    .effects_2ad=tail_bindings.effect,.action_buttons=&buttons
 };
 gh_em_resource_prefix prefix={.stage=5,.prefix_complete=1,.spawn_cursor=8,.page=room.active};
 gh_enemy_ems *owner=NULL;
 int detail=0;
 const gh_scene_placement_status admitted=gh_enemy_ems_spawn_00294ca0(&owner,&prefix,&room,
    &ems_bindings,&pc,&detail);
 printf("R105 EMS row8 status%d pc%08x detail%d cursor%u count%u\n",
    admitted,pc,detail,prefix.spawn_cursor,move_list->count);
 C(!admitted&&!pc&&!detail&&prefix.stage==9&&prefix.spawn_cursor==room.active[8]);
 C(move_list->count==1&&actor_list->count==1&&serial==9&&word(emit+0x26000)==0xe0000000u);
 const uint32_t handle=word(move_list->allocation.pointer);
 gh_enemy_ems_live actor_live;
 C(!gh_enemy_ems_live_at(owner,handle,&actor_live)&&actor_live.actor&&actor_live.model->ready&&
     actor_live.physical_row==8&&actor_live.actor->view.bytes[0x640]==63&&actor_live.shadow->initialized);
 C(!gh_enemy_ems_spawn_00294ca0(&owner,&prefix,&room,&ems_bindings,&pc,&detail)&&serial==9);
 uint8_t *const trans_root=actor_live.actor->view.bytes;
 const uint32_t original_flags=word(trans_root+0x250);
 const uint32_t original_hidden=word(trans_root+0x5a0);
 const uint32_t trans_global=word((const uint8_t *)&work+0x5e0);
 const uint32_t trans_debug=word((const uint8_t *)&work+0x58c);
 /* Explicit in-view profile: test each original predicate before the
  * actual next unresolved draw consumer of the neutral fixture. */
 memcpy(trans_root+0x250,&(uint32_t){original_flags|0x10000u},4);
 gh_actor_trans_bindings trans={.work=&work,.elf=elf,.elf_bytes=elf_size,.shadow=actor_live.shadow,
    .numeric_policy=GH_STATIC_BINARY32_REFERENCE};
 gh_game_pl_camera trans_camera={.constructed=1};
 for(unsigned i=0;i<4;i++)memcpy(trans_camera.fields+i*20,&(float){1},4);
 uint8_t depth_shift=0;
 trans.camera=&trans_camera;
 trans.depth_shift=&depth_shift;
 gh_actor_trans_call draw={0};
 memcpy((uint8_t *)&work+0x5e0,&(uint32_t){trans_global|0x40000000u},4);
 C(!gh_actor_model_trans_0014a3e8(&draw,actor_live.model,&trans,&pc)&&!pc&&draw.complete&&draw.phase==1);
 memcpy((uint8_t *)&work+0x5e0,&trans_global,4);
 memcpy(trans_root+0x5a0,&(uint32_t){original_hidden|2u},4);
 draw=(gh_actor_trans_call){0};
 C(!gh_actor_model_trans_0014a3e8(&draw,actor_live.model,&trans,&pc)&&!pc&&draw.complete&&draw.phase==1);
 memcpy(trans_root+0x5a0,&original_hidden,4);
 memcpy((uint8_t *)&work+0x58c,&(uint32_t){trans_debug|0x400000u},4);
 draw=(gh_actor_trans_call){0};
 C(!gh_actor_model_trans_0014a3e8(&draw,actor_live.model,&trans,&pc)&&!pc&&draw.complete&&draw.phase==1);
 memcpy((uint8_t *)&work+0x58c,&trans_debug,4);
 draw=(gh_actor_trans_call){0};
 C(gh_actor_model_trans_0014a3e8(&draw,actor_live.model,&trans,&pc)==GH_ACTOR_MODEL_UNSUPPORTED&&
    pc==0x14d828&&draw.phase==2&&!draw.complete);
 memcpy(trans_root+0x250,&original_flags,4);
 const gh_object_opacity_environment opacity={.init=init,.current_player=&actor.view,.player_binding_known=1};
 uint8_t *const live_bytes=actor_live.actor->view.bytes;
 memcpy((uint8_t *)&work+0x5d8,&(uint32_t){0x40000000},4);
 C(!gh_enemy_ems_move_2ad_0028e420(owner,handle,&opacity,elf,elf_size,NULL,NULL,NULL,&pc)&&!pc&&!live_bytes[0x2f6]);
 memset((uint8_t *)&work+0x5d8,0,4);
 memcpy((uint8_t *)&work+0x55c,&(float){1.0f},4);
 /* Both endpoints are outside actual R105 geometry, at distance1 and
  * facing +Z, so the original positive sight path publishes a prompt. */
 const float player_position[4]={1000,1000,1000,1},scenery_position[4]={1000,1000,1001,1};
 memcpy(actor.view.alias_f0,player_position,16);
 memcpy(actor.view.bytes+0x104,&(float){0},4);
 memcpy(actor_live.actor->view.alias_f0,scenery_position,16);
 gh_actor_motion_status moved=gh_enemy_ems_move_2ad_0028e420(owner,handle,&opacity,elf,elf_size,NULL,NULL,NULL,&pc);
 printf("R105 02AD Move status%d pc%08x phase%u\n",moved,pc,live_bytes[0x2f6]);
 C(moved==GH_ACTOR_MOTION_UNSUPPORTED&&pc==0x2cba48&&live_bytes[0x2f6]==1);
 gh_action_link link=buttons.heads[4];
 const gh_action_prompt *prompt;
 C(!gh_action_button_prompt(&buttons,link,&prompt)&&prompt->life==14&&prompt->priority==4&&
   prompt->unknown20==0&&prompt->kind==0&&prompt->callback_id10==0x28e7c8&&
   prompt->callback_argument==actor_live.actor);
 C(gh_action_button_reset_entry_001f7228(&buttons,link.index,&pc)==GH_ACTION_SET_OK);
 C(gh_enemy_ems_move_2ad_0028e420(owner,handle,&opacity,elf,elf_size,NULL,NULL,NULL,&pc)==GH_ACTOR_MOTION_UNSUPPORTED&&
    pc==0x2cba48&&live_bytes[0x2f6]==1&&buttons.entries[link.index].callback_argument==NULL);
 size_t ses_size;
 uint8_t *ses=load(disc,"se/emad.ses",&ses_size);
 C(ses&&ses_size==14496);
 gh_audio_registry *audio;
 gh_audio_token ses_token;
 C(!gh_audio_registry_create(&audio)&&
   !gh_audio_commit_se_with_external_index(audio,11,ses,ses_size,disc,&ses_token));
 gh_sound_span sample,adpcm;
 C(!gh_audio_sample_at(audio,11,0,&sample,&adpcm)&&adpcm.size==12736);
 gh_adpcm_pcm decoded={0};
 C(gh_audio_decode_ps_adpcm(adpcm.data,adpcm.size,&decoded)==GH_VOICE_UNSUPPORTED&&!decoded.samples);
 C(!gh_audio_decode_ps_adpcm_loop(adpcm.data,adpcm.size,&decoded)&&decoded.loop_first==22288&&
   decoded.loop_frames==22260&&decoded.frames==44548&&decoded.source_bytes==12736);
 C(pcm_hash(decoded.samples,decoded.frames)==UINT64_C(0x2dca0358eae6c5e7));
 gh_se_voice_engine *se;
 gh_se_mix mix;
 C(!gh_se_mix_boot_defaults_002c9e78(&mix,0,GH_SE_DSP_NATIVE_LINEAR_REFERENCE));
 /* Pending admission and explicit pre-transport frontier never call this opaque endpoint. */
 C(!gh_se_voice_create(audio,(gh_audio_output *)&mix,&mix,&se));
 uint32_t voice=0;
 gh_voice_status admission=gh_se_voice_request_attached(se,owner,&prefix,actor_live.model,-1,11,0x12,
   0,0,emad_probe,NULL,NULL,NULL,&voice);
 printf("R105 EMAD 12 admission status%d pc%08x handle%u\n",admission,gh_se_voice_frontier(se),voice);
 C(!admission&&voice);
 const int16_t *pending;
 size_t frames;
 C(!gh_se_voice_pcm_view(se,voice,&pending,&frames)&&frames==decoded.frames&&
    memcmp(pending,decoded.samples,frames*sizeof(*pending)));
 C(gh_se_voice_update(se)==GH_VOICE_UNSUPPORTED&&gh_se_voice_frontier(se)==0x308da8);
 gh_se_voice_info pending_voice;
 C(!gh_se_voice_query(se,voice,&pending_voice)&&pending_voice.pending&&!pending_voice.native_voice);
 gh_se_voice_destroy(se);
 C(!gh_se_voice_create(audio,(gh_audio_output *)&mix,&mix,&se));
 C(!gh_se_voice_request_attached(se,owner,&prefix,actor_live.model,-1,11,0,
    0,0,emad_probe,NULL,NULL,NULL,&voice)&&voice);
 C(!gh_se_voice_query(se,voice,&pending_voice)&&pending_voice.pending&&
    pending_voice.bank==11&&pending_voice.program==0&&pending_voice.position_mode==2&&
    pending_voice.frames==decoded.frames);
 gh_se_voice_destroy(se);
 gh_audio_pcm_release(&decoded);
 gh_audio_registry_destroy(audio);
 free(ses);
 uint8_t scenario_bytes[0x114]={0};
 memset(scenario_bytes+0x5c,0x7b,8);
 gh_task_record32 records[16]={0};
 gh_task_manager manager={.records=records,.capacity=16,.count=16,.current_index=-1};
 gh_task_threads *threads;
 C(!gh_task_threads_create(&manager,&threads));
 scenery_action_probe action_probe={0};
 gh_soft_event_bindings soft={.scenario={scenario_bytes,sizeof(scenario_bytes),&manager},
    .work=&work,.player_abort_00126770=scenery_abort_probe,.context=&action_probe};
 gh_enemy2ad_action action={0};
 const uint32_t initial_flags=0x00010000;
 memcpy((uint8_t *)&work+0x5e4,&initial_flags,4);
 C(!gh_enemy2ad_action_0028e7c8(&action,actor_live.actor,&soft,threads,
    scenery_task_probe,&action_probe,&pc)&&!pc&&action.slot==1);
 C(action_probe.abort_calls==1&&!action_probe.task_calls&&scenario_bytes[0xe8]==1&&
   word(scenario_bytes+0xe0)==2&&(manager.flags&1)&&
   word((uint8_t *)&work+0x5d8)==0xfed40000u&&word((uint8_t *)&work+0x5e4)==0x40000040u);
 C(records[1].running==2&&records[1].attributes==1&&((uint8_t *)&records[1])[0x1e]==1&&
    !word(scenario_bytes+0x5c)&&!word(scenario_bytes+0x60));
 C(!gh_enemy2ad_action_0028e7c8(&action,actor_live.actor,&soft,threads,
    scenery_task_probe,&action_probe,&pc)&&action_probe.abort_calls==1&&scenario_bytes[0xe8]==1);
 const gh_task_run_result selection=gh_tasks_run_matching_002d52a0(&manager);
 C(selection.status==GH_TASK_OK&&action_probe.task_calls==1&&!records[1].running);
 C(!gh_task_threads_destroy(threads));
 work.language=1;
 gh_menu_state menu={0};
 gh_game_scene_ui ui={0};
 C(!gh_menu_load_tables_002ae990(&menu,&language));
 C(!gh_scene_message_initialize_002aea18(&ui,&menu,&bundle,work.language));
 scenario_bytes[0x112]=scenario_bytes[0x113]=0xff;
 gh_scene_scenario_view scenario_view={scenario_bytes,sizeof(scenario_bytes),&manager};
 int visible=0;
 C(!gh_scenario_message_open_002c42e0(scenario_view,&ui,&work,0x3004,&pc)&&!pc);
 C(!gh_scenario_message_active_002c4350(scenario_view,&ui,&visible,&pc)&&visible);
 C(scenario_bytes[0x112]==4&&scenario_bytes[0x113]==0x30&&ui.active_head==0);
 const uint8_t *text=ui.sources[0].primary.bytes;
 C(text&&text>=menu.texture_data.bytes&&ui.sources[0].primary.size>=32);
 printf("MSD 3004 offset %td leading codes:",text-menu.texture_data.bytes);
 for(unsigned k=0;k<16;k++)printf(" %04x",text[k*2]|(unsigned)text[k*2+1]<<8);
 puts("");
 gh_scene_ui_status step=gh_scene_message_update_002aefa8(&ui,&work,&pc);
 printf("MSD 3004 first update status%d pc%08x\n",step,pc);
 C(!step&&!pc&&ui.sources[0].cursor_7c==4&&ui.sources[0].cursor_8c==4);
 uint8_t *window=ui.blocks[0].payload;
 C(window[0xa4-0xc]==4&&window[0x70-0xc]==2&&!word(window+0x94-0xc));
 C(!gh_scene_message_prepare_draw_002af028(&ui,&work,&pc)&&!pc&&
   word(window+0xae - 0xc)==0x01700100&&word(window+0xaa-0xc)==0x007801e0);
 C(!gh_scene_message_update_002aefa8(&ui,&work,&pc)&&!pc);
 gh_font_state font={0};
 gh_display_state display={0};
 gh_id_view palette;
 C(!gh_font_initialize(&font,&language,work.language)&&
   !gh_scene_font_bind_002a4c38(&font,&bundle,work.language));
 C(!gh_display_state_from_elf(&display,elf,elf_size)&&
   !gh_display_static_initialize_002a48a8(&display,1,0xffff)&&
   !gh_id_text_palette_from_elf(elf,elf_size,&palette));
 scenery_glyph_capture capture={0};
 C(!gh_scene_message_render_002af028(&ui,&work,&font,&display,palette,
    capture_scenery_glyphs,&capture,&pc)&&capture.calls==1&&capture.count==1);
 menu.timer=30;
 for(unsigned i=0;i<19;i++)C(!gh_scene_message_update_002aefa8(&ui,&work,&pc));
 C(ui.sources[0].cursor_8c==44);
 C(!gh_scene_message_render_002af028(&ui,&work,&font,&display,palette,
    capture_scenery_glyphs,&capture,&pc)&&capture.calls==2&&capture.count==20);
 double line_width=19*1.8,limits[20];
 for(unsigned i=0;i<20;i++){
   const uint16_t code=(uint16_t)(text[4+i*2]|(uint16_t)text[5+i*2]<<8);
   gh_font_glyph glyph;
   C(!gh_font_glyph_at(&font,code,&glyph));
   limits[i]=glyph.width*.81;
   line_width+=limits[i];
   C(capture.normal[i].code==code&&capture.shadow[i].code==code&&
     capture.normal[i].quad.z==65535&&capture.shadow[i].quad.z==65535&&
     fabs((double)(capture.shadow[i].x-capture.normal[i].x)-2)<.001&&
     fabs((double)(capture.shadow[i].y-capture.normal[i].y)-2)<.001);
 }
 double x=256-line_width*.5;
 const double y=368-((font.slots[0].cell_height+12)*1.1)*.5+12*1.1;
 for(unsigned i=0;i<20;i++){
   C(fabs(capture.normal[i].x-x)<.002&&fabs(capture.normal[i].y-y)<.002);
   x+=limits[i]+1.8;
 }
 printf("MSD 3004 first page: %zu glyphs, start %.4f %.4f\n",
   capture.count,(double)capture.normal[0].x,(double)capture.normal[0].y);
 step=gh_scene_message_update_002aefa8(&ui,&work,&pc);
 printf("page transition status %d pc %x pos %zu/%zu budget %u/%u/%u counter %u choice %x voice %u/%u\n",step,pc,ui.sources[0].cursor_7c,ui.sources[0].cursor_8c,word(window+0x84-0xc),word(window+0x68-0xc),word(window+0x90-0xc),window[0x30-0xc],word(window+0x94-0xc),word(window+0xb4-0xc),word(window+0x40-0xc));
 C(!step&&!pc&&
   ui.sources[0].cursor_7c==46&&ui.sources[0].cursor_8c==56&&
   word(window+0x84-0xc)==600&&word(window+0x68-0xc)==600&&
   word(window+0x90-0xc)==630&&window[0x30-0xc]==1&&word(window+0x94-0xc)==0x10000);
 C(!gh_scene_message_render_002af028(&ui,&work,&font,&display,palette,
    capture_scenery_glyphs,&capture,&pc)&&capture.calls==3&&capture.count==1&&
    capture.normal[0].code==0x4e);
 for(unsigned i=0;i<26;i++)C(!gh_scene_message_update_002aefa8(&ui,&work,&pc)&&!pc);
 C(window[0]==2&&word(window+0x94-0xc)==0x20000&&word(window+0x90-0xc)==1410);
 C(!gh_scene_message_update_002aefa8(&ui,&work,&pc)&&!pc&&window[0]==4);
 C(!gh_scene_message_render_002af028(&ui,&work,&font,&display,palette,
    capture_scenery_glyphs,&capture,&pc)&&capture.calls==4&&capture.count==27&&
    capture.normal[11].code==0x10&&capture.normal[12].code==0x4b);
 gh_font_glyph before_option;
 C(!gh_font_glyph_at(&font,0x10,&before_option)&&
   fabs((double)capture.normal[12].x-capture.normal[11].x-
       (before_option.width*.81+1.8+40*.81+1.8))<.002);
 C(!gh_scene_message_update_002aefa8(&ui,&work,&pc)&&!pc&&
   window[0]==4&&(word(window+0x14-0xc)&0x20));
 C(gh_scene_message_render_002af028(&ui,&work,&font,&display,palette,
    capture_scenery_glyphs,&capture,&pc)==GH_SCENE_UI_UNSUPPORTED&&pc==0x2b00a0&&capture.calls==4);
 memcpy((uint8_t *)&work+0x1a0,&(uint64_t){UINT64_C(0x4000000)},8);
 C(gh_scene_message_update_002aefa8(&ui,&work,&pc)==GH_SCENE_UI_UNSUPPORTED&&pc==0x2b46f0);
 memcpy((uint8_t *)&work+0x1a0,&(uint64_t){UINT64_C(0x10000000)},8);
 C(gh_scene_message_update_002aefa8(&ui,&work,&pc)==GH_SCENE_UI_UNSUPPORTED&&pc==0x2b3fc8);
 memset((uint8_t *)&work+0x1a0,0,8);
 printf("MSD 3004 page two: %zu glyphs, A804 rows 0/1, choice state %u\n",capture.count,window[0]);
 C(!gh_scenario_message_close_002c4390(scenario_view,&ui,&pc)&&ui.active_head==-1);
 C(!gh_scenario_message_active_002c4350(scenario_view,&ui,&visible,&pc)&&!visible&&
    scenario_bytes[0x112]==0xff&&scenario_bytes[0x113]==0xff);
 C(gh_enemy_ems_2ad_dtor_motion_0028e068(owner,handle,&pc)==GH_ACTOR_MOTION_UNSUPPORTED&&pc==0x305778);
 C(gh_enemy_ems_2ad_dtor_motion_0028e068(owner,handle,&pc)==GH_ACTOR_MOTION_UNSUPPORTED&&pc==0x305778);
 C(gh_enemy_ems_release_native(&owner,&pc)==GH_ACTOR_MOTION_UNSUPPORTED&&pc==0x28e068&&owner);
 gh_effect_create_forget_on_owner_destroy(effects);
 C(!gh_enemy_ems_release_native(&owner,&pc)&&!pc&&!owner);
 C(!move_list->count&&!actor_list->count&&!init->enemy_top&&!effects->emd_tables[0xd7].bytes);
 C(!gh_game_cockpit_destroy(cockpit)&&!gh_id_runtime_destroy(runtime));
 C(!gh_scene_actors_reset_0012e800(&tables));
 free(elf);free(id);free(language_data);
 C(!gh_enemy_entry_abort_prefix(&entry)&&!move_list->count&&!actor_list->count&&!init->enemy_top);
 gh_game_objects_destroy(room_objects);gh_game_objects_destroy(root_objects);free(init);
 C(!gh_game_room_pools_release(&pools));
 free(effects);free(emit);free(esp);free(core);
 resource.file_extent=0x10;
 C(gh_enemy2ad_effect_data(&resource,&effect)==GH_EFFECT_BOUNDS);
 C(!gh_game_title_player_release(&actor)&&!gh_heap_used(heap)&&!gh_heap_destroy(heap));
 free(resource_data);free(room_data);gh_disc_close(disc);
 puts("PASS actual R105 row8 stamp63 and 02AD constructor/effect resource");
 return 0;
}
