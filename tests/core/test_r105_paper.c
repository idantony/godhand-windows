#include "game_om382.h"
#include "game_actor_external_parent.h"
#include "game_player_factory.h"
#include "game_math_globals.h"
#include "game_effect_release.h"
#include "game_object_live_trans.h"
#include "game_om300_retire.h"
#include "game_scene_dynamic_geometry.h"
#include "disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) do{if(!(x)){fprintf(stderr,"fixture line %d: %s\n",__LINE__,#x);exit(1);}}while(0)

static uint32_t word(const void *p)
{
    uint32_t value;
    memcpy(&value,p,4);
    return value;
}

typedef struct {
    gh_effect_release_bindings effects;
    const gh_title_player_view *paper;
    gh_se_voice_engine *voices;
    unsigned calls;
} release_observer;

static gh_actor_motion_status release_effect(void *context,const gh_title_player_view *actor,
    int32_t kind,uint32_t mode,uint32_t *frontier)
{
    release_observer *observer=context;
    CHECK(actor==observer->paper&&kind==8&&mode==2);
    ++observer->calls;
    unsigned slot;
    return gh_game_effect_release_00306200(&observer->effects,actor,kind,mode,frontier,&slot)?
        GH_ACTOR_MOTION_UNSUPPORTED:GH_ACTOR_MOTION_OK;
}

static gh_object_step_status release_sound(void *context,gh_actor_model *model,uint32_t *frontier)
{
    const release_observer *observer=context;
    CHECK(model&&model->root.bytes==observer->paper->bytes);
    *frontier=0;
    return gh_se_voice_detach_actor_002ce488(observer->voices,model)?GH_OBJSTEP_CALLEE:GH_OBJSTEP_OK;
}

typedef struct {
    gh_om376_paper *paper;
    gh_om_factory *factory;
    gh_actor_model *player;
    gh_core_save_view save;
    gh_game_scene_tables *tables;
    gh_collision_scroll_mutation *scroll;
    uint32_t frontier;
    gh_scene_placement_status status;
    int complete;
} paper_load;

static void load_paper(void *context,int32_t argument)
{
    paper_load *task=context;
    (void)argument;
    task->status=gh_om376_set_paper_0012c8d8(task->paper,task->factory,task->player,
        task->save,task->tables,task->scroll,&task->frontier);
    task->complete=1;
}

typedef struct {
    gh_om300_retire_bindings bindings;
    gh_om300_full_retirement retirement;
    unsigned sound_visits,invocations;
} paper_retire;

static gh_object_status drop_sound_slot(void *context,int32_t slot,uint32_t *frontier)
{
    paper_retire *state=context;
    CHECK(slot==-1);
    ++state->sound_visits;
    *frontier=0;
    return GH_OBJECT_OK;
}

static gh_object_step_status lookup_paper(void *context,uint32_t handle,gh_title_player_view *out,uint32_t *frontier)
{
    paper_retire *state=context;
    *frontier=0;
    return gh_om_factory_object_view(state->bindings.factory,handle,out)?GH_OBJSTEP_CALLEE:GH_OBJSTEP_OK;
}

static gh_object_step_status unexpected_move(void *context,const gh_object_move_request *request,
    const gh_boot_work *work,uint32_t *frontier)
{
    (void)context;
    (void)request;
    (void)work;
    *frontier=0x30f090;
    return GH_OBJSTEP_UNSUPPORTED;
}

static gh_object_step_status destroy_paper(void *context,const gh_object_trans_request *request,
    const gh_boot_work *work,uint32_t *frontier)
{
    paper_retire *state=context;
    CHECK(work->room==0x105&&request->kind==GH_OBJECT_TRANS_DESTROY&&request->argument==3&&
        request->original_entry==0x1c91b0&&request->original_vtable==0x42aa98&&!request->this_delta);
    ++state->invocations;
    const gh_room_pool_status status=gh_om_destroy_detached_mode3(&state->retirement,
        &state->bindings,request->handle,&request->actor,request->original_entry);
    *frontier=state->retirement.frontier;
    return status?GH_OBJSTEP_CALLEE:GH_OBJSTEP_OK;
}

static uint8_t *load(gh_disc *disc,const char *name,size_t *bytes)
{
    size_t index;
    gh_disc_entry file;
    CHECK(!gh_disc_find(disc,name,&index)&&!gh_disc_entry_at(disc,index,&file));
    *bytes=(size_t)file.size;
    uint8_t *data=malloc(*bytes);
    CHECK(data&&!gh_disc_read(disc,index,0,data,*bytes));
    return data;
}

int main(int argc,char **argv)
{
    gh_disc *disc;
    CHECK(argc==2&&!gh_disc_open(argv[1],&disc));
    size_t room_bytes, player_bytes, paper_bytes,core_bytes;
    uint8_t *room_data=load(disc,"st1/r105.dat",&room_bytes);
    uint8_t *player_data=load(disc,"pl/pl00.dat",&player_bytes);
    uint8_t *paper_data=load(disc,"om/om76.dat",&paper_bytes);
    uint8_t *core=load(disc,"etc/core.dat",&core_bytes);
    CHECK(room_bytes==1120320&&paper_bytes==5600);
    const size_t elf_bytes=(size_t)gh_disc_boot_elf_size(disc);
    uint8_t *elf=malloc(elf_bytes);
    CHECK(elf&&!gh_disc_read_boot_elf(disc,0,elf,elf_bytes));

    gh_bundle room_pack,player_pack;
    gh_bundle_entry md,tm3;
    CHECK(!gh_bundle_open(room_data,room_bytes,&room_pack)&&
          !gh_bundle_open(player_data,player_bytes,&player_pack)&&
          !gh_bundle_find(&player_pack,"MD",0,&md)&&!gh_bundle_find(&player_pack,"TM3",0,&tm3));
    gh_heap *heap;
    gh_room_pools pools={0};
    CHECK(!gh_heap_create(32*1024*1024,&heap)&&!gh_game_room_pools_initialize_002a5d80(&pools,&room_pack,heap));

    gh_boot_work work={.room=0x105,.phase569=2,.canonical_rate574=1.0f};
    gh_game_math_globals math={0};
    CHECK(!gh_game_math_globals_from_elf(&math,elf,elf_bytes)&&!gh_game_math_static_initialize_003079c8(&math,&work,1,0xffff));
    gh_game_init_state *init=malloc(sizeof(*init));
    CHECK(init);
    gh_game_init_state_initialize(init);
    memcpy(init->enemies+0x548,&work.canonical_rate574,4);
    CHECK(!gh_hitarea_objects_construct_001fde00(init,NULL)&&!gh_game_object_pool_reset_001fdea8(init));
    gh_collision_pairs pairs={0};
    CHECK(!gh_collision_pairs_construct_002b8ae0(&pairs,heap)&&!gh_collision_pairs_initialize_002b8b50(&pairs));
    gh_hitarea_shape_bindings shapes={&pairs,&math,GH_STATIC_BINARY32_REFERENCE};
    gh_stage_slots global_slots={0},local_slots={0};
    gh_game_objects *global,*local;
    CHECK(!gh_game_objects_create(&global_slots,0,&global)&&!gh_game_objects_create(&local_slots,1,&local)&&
          !gh_stage_slots_reset(&global_slots,heap)&&!gh_stage_slots_reset(&local_slots,heap));
    int added;
    CHECK(!gh_game_objects_add_pinned_001fece0(global,0x100,player_data,player_bytes,&added)&&added&&
          !gh_game_objects_set_file_extent(global,0x100,player_data,player_bytes));
    gh_dvd_jobs dvd={0};
    gh_dvd_jobs_init(&dvd,disc);
    gh_task_record32 task_records[8]={0};
    gh_task_manager task_manager={.records=task_records,.count=8,.capacity=8,.current_index=-1};
    gh_task_threads *threads;
    CHECK(!gh_task_threads_create(&task_manager,&threads));
    const gh_object_resource_bindings resources={.dvd=&dvd,.heap=heap,.threads=threads};

    gh_game_title_player player={0};
    gh_game_player_factory player_factory={0};
    uint8_t save_bytes[GH_CORE_SAVE_BYTES];
    gh_core_save_view save={save_bytes,sizeof(save_bytes)};
    CHECK(!gh_core_save_system_init(save)&&!gh_core_save_game_init(save,&work));
    uint8_t serial=52;
    gh_player_factory_bindings player_bindings={&player,&pools,global,local,&work,disc,save,&serial};
    int original;
    CHECK(!gh_game_player_factory_create_0030fa98(&player_factory,&player_bindings,&original)&&original);
    uint32_t fog=0;
    gh_actor_model player_model={0};
    CHECK(!gh_actor_model_bind_packet_inputs(&player_model,heap,&fog)&&
          !gh_actor_model_initialize_00149738(&player_model,&player.view,&work,&pools,&md,&tm3,0,1,
          GH_STATIC_BINARY32_REFERENCE,&original)&&original);

    gh_game_scene_tables tables={0};
    CHECK(!gh_scene_actors_0012e670(&tables,&room_pack));
    gh_scene_dynamic_owner dynamic={0};
    CHECK(!gh_scene_dynamic_bind(&dynamic,&tables,heap));
    gh_collision_scroll_mutation scroll={0};
    uint8_t room_page[0x1650]={0};
    gh_room_state room={.active=room_page,.size=sizeof(room_page)};
    gh_om_initializer_bindings init_om={.work=&work,.packet_heap=heap,.fog_rgb=&fog,.init=init,
        .shapes=&shapes,.dynamic=&dynamic};
    gh_om_factory_bindings om_bindings={.pools=&pools,.work=&work,.room_state=&room,
        .objects=global,.room_objects=local,.factory_serial=&serial,.initializer=&init_om,.resources=&resources};
    gh_om_factory factory={0};
    CHECK(!gh_om_factory_bind(&factory,&om_bindings,4));
    gh_om376_paper call={0};
    uint32_t frontier;
    paper_load task={.paper=&call,.factory=&factory,.player=&player_model,.save=save,.tables=&tables,.scroll=&scroll};
    int32_t task_slot;
    CHECK(!gh_task_threads_start(threads,-1,load_paper,&task,0,&task_slot)&&task_slot>=0);
    for(unsigned visit=0;!task.complete&&visit<16;visit++){
        CHECK(gh_tasks_run_matching_002d52a0(&task_manager).status==GH_TASK_OK);
        CHECK(!gh_dvd_pump(&dvd));
    }
    if(task.status)fprintf(stderr,"paper status%d frontier%08x stage%u\n",task.status,task.frontier,call.stage);
    CHECK(task.complete&&!task.status&&!task.frontier&&call.stage==6&&factory.count==1&&serial==54&&save_bytes[0x157]==1);
    const gh_om_object *paper=call.create.object;
    CHECK(paper&&paper->runtime_kind==0x376&&paper->registered&&paper->allocation.bytes==0x790);
    CHECK(paper->resource_manager==global&&paper->resource.file_extent==paper_bytes&&
        !memcmp(paper->resource.bytes,paper_data,paper_bytes));
    CHECK(paper->view.original_vtable214==0x42aa98&&word(player.view.bytes+0x6a8)==paper->handle);
    const uint8_t *p=paper->view.bytes;
    CHECK(word(p+0x2d0)==0x80000000u&&word(p+0x2f4)==0x100&&word(p+0x604)==2&&word(p+0x668)==100);
    CHECK(!word(p+0x610)&&word(p+0x614)==0x3d408312&&word(p+0x618)==0xbe0f5c29);
    CHECK(player_model.external_borrowers==1&&paper->initialization->model.ready);
    const uint8_t *parent;
    CHECK(!gh_actor_model_external_parent_at(&paper->initialization->model,0,&parent)&&parent==player_model.parts[2].bytes);
    CHECK(!gh_om376_set_paper_0012c8d8(&call,&factory,&player_model,save,&tables,&scroll,&frontier)&&serial==54&&factory.count==1);
    gh_game_effects *effects=calloc(1,sizeof(*effects));
    uint8_t *esp=calloc(1,0x100180),*emitters=calloc(1,0x26060);
    CHECK(effects&&esp&&emitters&&!gh_game_effects_initialize_00305a68(effects,esp,0x100180,
        emitters,0x26060,core,core_bytes,&room_pack));
    release_observer observer={.effects={.effects=effects,.esp=esp,.esp_bytes=0x100180,
        .emitters=emitters,.emitter_bytes=0x26060},.paper=&paper->view};
    gh_audio_registry *audio;
    gh_se_mix mix;
    uint8_t unused_transport;
    CHECK(!gh_audio_registry_create(&audio)&&!gh_se_mix_boot_defaults_002c9e78(&mix,0,GH_SE_DSP_NATIVE_LINEAR_REFERENCE)&&
        !gh_se_voice_create(audio,(gh_audio_output *)&unused_transport,&mix,&observer.voices));
    gh_game_collision_solids solids={0};
    gh_om_frame_state frame={0};
    const gh_om_frame_runtime runtime={.init=init,.solids=&solids,.canonical_rate=&work.canonical_rate574,
        .release_effect=release_effect,.remove_sound=release_sound,.context=&observer};
    const gh_object_opacity_environment environment={.init=init,.current_player=&player.view,.player_binding_known=1};
    CHECK(!gh_om_frame_bind_environment(&frame,&environment)&&!gh_om_frame_bind_runtime(&frame,&runtime));
    const gh_object_move_request move={.handle=paper->handle,.original_entry=0x1c9860,.actor=paper->view};
    gh_actor_model *borrowed=NULL;
    CHECK(!gh_om_factory_model_at(&factory,paper->handle,&borrowed)&&borrowed==&paper->initialization->model);
    CHECK(!gh_om_frame_move(&frame,&factory,&move,&work,GH_STATIC_BINARY32_REFERENCE,&frontier));
    CHECK(!frontier&&frame.complete&&observer.calls==1&&p[0x2f6]==1&&p[0x531]==0xff);
    const uint32_t move_pose[3]={0xc0875c29,0xc03c28f6,0x429a6666};
    const uint32_t yaw=0xc010a3d7;
    memcpy(player.view.alias_f0,move_pose,sizeof(move_pose));
    memcpy(player.view.bytes+0x104,&yaw,4);
    CHECK(!gh_actor_model_update_hierarchy_00149eb8(&player_model,GH_STATIC_BINARY32_REFERENCE));
    CHECK(!gh_om_frame_next_call(&frame)&&!gh_om_frame_move(&frame,&factory,&move,&work,GH_STATIC_BINARY32_REFERENCE,&frontier));
    CHECK(!frontier&&frame.complete&&observer.calls==1&&word(p+0x104)==yaw);
    CHECK(word(p+0x490)!=0&&word(p+0x490)==word(paper->view.alias_f0));
    const uint32_t drop_y=word(borrowed->parts[0].alias_f0+4);
    CHECK(!gh_om376_release_paper_001cf6e0(&call,&player.view,save,&frontier));
    CHECK(!frontier&&call.stage==8&&!word(player.view.bytes+0x6a8)&&!save.bytes[0x157]);
    CHECK(!borrowed->external_owner&&!player_model.external_borrowers&&word(p+0x2f4)==0x1300);
    CHECK(word(p+0x490)==move_pose[0]&&word(p+0x494)==drop_y&&word(p+0x498)==move_pose[2]);
    CHECK(word(p+0x630)==0x3d851eb8&&word(p+0x644)==0x3d4ccccd);
    CHECK(!gh_om376_release_paper_001cf6e0(&call,&player.view,save,&frontier)&&!frontier);
    CHECK(!gh_om_frame_next_call(&frame)&&!gh_om_frame_move(&frame,&factory,&move,&work,GH_STATIC_BINARY32_REFERENCE,&frontier));
    printf("first fall rate=%08x timer=%08x opacity=%08x rotation=%08x\n",
        word(p+0x5a8),word(p+0x660),word(p+0x24c),word(p+0x100));
    CHECK(!frontier&&frame.complete&&word(p+0x660)==0x41e80000&&word(p+0x24c)==0x3f800000);
    CHECK(word(p+0x490)!=move_pose[0]&&word(p+0x494)!=drop_y&&word(p+0x100)==0x3e32b8c2);
    unsigned fall_frames=1;
    while(!p[0x2f4]&&fall_frames<50){
        CHECK(!gh_om_frame_next_call(&frame)&&!gh_om_frame_move(&frame,&factory,&move,&work,GH_STATIC_BINARY32_REFERENCE,&frontier));
        ++fall_frames;
    }
    CHECK(p[0x2f4]==1&&!p[0x2f5]&&!p[0x2f6]&&fall_frames<50&&observer.calls==1);
    CHECK(!gh_om_frame_next_call(&frame)&&!gh_om_frame_move(&frame,&factory,&move,&work,GH_STATIC_BINARY32_REFERENCE,&frontier));
    CHECK(!frontier&&frame.complete&&(word(p+0x250)&0xe0000002u)==0x80000002u);
    paper_retire retirement={.bindings={.factory=&factory,.tables=&tables,.scroll=&scroll,
        .solids=&solids,.effects=observer.effects,.release_sound_slot=drop_sound_slot}};
    retirement.bindings.sound_context=&retirement;
    gh_game_object_step step={0};
    CHECK(!gh_game_object_step_construct_002c53a8(&step,init));
    const gh_object_move_bindings object={.resolve=lookup_paper,.invoke=unexpected_move,
        .context=&retirement,.elf=elf,.elf_bytes=elf_bytes};
    const gh_object_trans_bindings trans={.invoke=destroy_paper,.context=&retirement};
    CHECK(!gh_game_object_step_bind_live(&step,&object)&&!gh_game_object_step_bind_trans(&step,&trans));
    const uint32_t handle=paper->handle;
    int erased;
    for(unsigned visit=0;visit<3;visit++){
        CHECK(!gh_object_live_trans_one_0030f650(&step,&pools,&work,handle,&erased,&frontier));
        CHECK(!frontier&&erased==(visit==2)&&retirement.invocations==(visit==2));
    }
    CHECK(retirement.retirement.stage==5&&!paper->registered&&!paper->initialization&&!paper->allocation.pointer);
    gh_object_view asset;
    CHECK(!gh_game_objects_find(global,0x376,&asset)&&asset.state==5&&!asset.use_count);
    CHECK(!gh_game_objects_update_bound_001fe748(global,&resources,&frontier));
    CHECK(!gh_game_objects_update_bound_001fe748(global,&resources,&frontier));
    CHECK(!gh_game_objects_update_bound_001fe748(global,&resources,&frontier));
    CHECK(gh_game_objects_find(global,0x376,&asset)==GH_OBJECT_MISSING);
    CHECK(retirement.sound_visits==10);
    CHECK(!gh_om_factory_release(&factory)&&!player_model.external_borrowers);
    CHECK(!gh_collision_scroll_mutation_release(&scroll));
    CHECK(!gh_actor_model_release(&player_model)&&!gh_game_player_factory_release_prefix(&player_factory));
    CHECK(!gh_hitarea_shapes_abort(init,&frontier)&&!gh_game_object_pool_release_001fdfa8(init,&frontier));
    CHECK(!gh_collision_pairs_release(&pairs));
    gh_game_objects_destroy(local);
    gh_game_objects_destroy(global);
    gh_se_voice_destroy(observer.voices);
    gh_audio_registry_destroy(audio);
    CHECK(!gh_task_threads_destroy(threads)&&!gh_scene_dynamic_release(&dynamic));
    CHECK(!gh_game_room_pools_release(&pools)&&!gh_heap_used(heap)&&!gh_heap_destroy(heap));
    free(elf);
    free(init);
    free(paper_data);
    free(player_data);
    free(room_data);
    free(emitters);
    free(esp);
    free(effects);
    free(core);
    gh_disc_close(disc);
    printf("PASS actual R105 PL00 and OM76 attach, 1CF6E0 release, %u fall/fade frames, three ordered Trans visits and resource retirement\n",fall_frames);
    return 0;
}
