#include "menu_state.h"
#include "game_actor_motion_ik.h"
#include "game_camera_envelope.h"
#include "game_enemy_2ad.h"
#include "game_scene_placements.h"
#include "game_enemy_ems.h"
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
       emitter.selector==&entry.object.view&&emitter.model==&live.model.model);
    printf("D7:1 emitter %u part %u class %u\n",index,emitter.record.bytes[0xbe],emitter.record.bytes[0x133]);
 }
 C(!gh_enemy2ad_init_tail_0028e320(&tail,&live,&entry.object.view,&entry.resource,
    &tail_bindings,GH_STATIC_BINARY32_REFERENCE,&original)&&original&&word(emit+0x26000)==0xe0000000u);
 gh_effect_create_forget_on_owner_destroy(effects);
 C(!gh_game_effects_release_2ad_native(effects,effect));
 C(!gh_game_effects_release_2ad_native(effects,effect));
 C(!gh_game_effects_release_2ad_native(effects,effect)&&!effects->emd_tables[0xd7].bytes);
 C(!memcmp(prior_entries,effects->emitter_entries,sizeof(prior_entries)));
 C(!gh_enemy_model_release_prefix(&live.model));
 C(!gh_enemy_entry_abort_prefix(&entry)&&!move_list->count&&!actor_list->count&&!init->enemy_top);
 /* Start a new isolated effect owner after retiring the earlier fixture. */
 memset(esp,0,0x100180);
 memset(emit,0,0x26060);
 C(!gh_game_effects_initialize_00305a68(effects,esp,0x100180,emit,0x26060,core,core_size,&bundle));
 gh_enemy_ems_bindings ems_bindings={
    .actor={.entry=bindings,.cockpit=cockpit,.ik=&ik,.actors=&tables,.atan_coeff=camera.atan_coeff,
        .canonical_rate=&rate,.numeric_policy=GH_EM_NUMERIC_PCSX2_CHOPZERO_REFERENCE},
    .effects_2ad=tail_bindings.effect
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
