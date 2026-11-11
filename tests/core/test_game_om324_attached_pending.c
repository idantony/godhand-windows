#include "game_actor_attached_sound.h"
#include "game_actor_model_nodes.h"
#include "game_om324.h"
#include "disc.h"
#include <fenv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); exit(1); } } while (0)

static uint8_t *read_disc(gh_disc *disc,const char *path,size_t *size)
{
 size_t i;
 gh_disc_entry entry;
 CHECK(!gh_disc_find(disc,path,&i)&&!gh_disc_entry_at(disc,i,&entry));
 *size=(size_t)entry.size;
 uint8_t *bytes=malloc(*size);
 CHECK(bytes&&!gh_disc_read(disc,i,0,bytes,*size));
 return bytes;
}

typedef struct {
 gh_sound_scene *scene;
 gh_bundle *room;
 gh_boot_work *work;
 gh_dvd_jobs *dvd;
 gh_heap *heap;
 gh_task_threads *threads;
 int done;
} sound_load;

static void load_scene(void *context,int32_t argument)
{
 (void)argument;
 sound_load *load=context;
 CHECK(!gh_sound_scene_initialize_002cabe8(load->scene,load->room,load->work,
        load->dvd,load->heap,load->threads));
 load->done=1;
}

int main(int argc,char **argv)
{
 CHECK(argc==3);
 FILE *file=fopen(argv[2],"rb");
 CHECK(file&&!fseek(file,0,SEEK_END));
 long elf_size=ftell(file);
 CHECK(elf_size>0&&!fseek(file,0,SEEK_SET));
 uint8_t *elf=malloc((size_t)elf_size);
 CHECK(elf&&fread(elf,1,(size_t)elf_size,file)==(size_t)elf_size&&!fclose(file));
 gh_disc *disc;
 CHECK(!gh_disc_open(argv[1],&disc));
 size_t room_size,object_size;
 uint8_t *room_bytes=read_disc(disc,"st1/r100.dat",&room_size);
 uint8_t *object_bytes=read_disc(disc,"om/om24.dat",&object_size);
 gh_bundle room;
 gh_bundle_entry oms;
 CHECK(!gh_bundle_open(room_bytes,room_size,&room)&&!gh_bundle_find(&room,"OMS",0,&oms));
 gh_om_scene_placements placement={0};
 int original;
 CHECK(gh_om_scene_bind_001c3cb8(&placement,&oms,&original)==GH_PLACE_UNSUPPORTED);
 gh_om_placement_record record,second_record;
 CHECK(!gh_om_scene_record_at(&placement,49,&record)&&record.type==0x324);
 CHECK(!gh_om_scene_record_at(&placement,50,&second_record)&&second_record.type==0x324);

 gh_heap *heap;
 gh_boot_work work={.room=0x100};
 gh_room_pools pools={0};
 CHECK(!gh_heap_create(16*1024*1024,&heap));
 CHECK(!gh_game_room_pools_initialize_002a5d80(&pools,&room,heap));
 gh_stage_slots global_slots={0},room_slots={0};
 gh_game_objects *global,*local;
 CHECK(!gh_game_objects_create(&global_slots,0,&global)&&
       !gh_game_objects_create(&room_slots,1,&local));
 CHECK(!gh_stage_slots_reset(&global_slots,NULL)&&!gh_stage_slots_reset(&room_slots,NULL));
 CHECK(!gh_game_objects_add_pinned_001fece0(global,0x324,object_bytes,object_size,&original));
 CHECK(!gh_game_objects_set_file_extent(global,0x324,object_bytes,object_size));
 gh_game_prepare prepare={0};
 uint8_t stage=0,serial=0;
 gh_room_state room_state;
 CHECK(!gh_game_prepare_002a5d40(&prepare,(gh_core_save_view){0},0x100,&stage));
 CHECK(!gh_room_state_select_002bebc8(&prepare,0x100,0,&room_state));
 gh_game_math_globals math={0};
 CHECK(!gh_game_math_globals_from_elf(&math,elf,(size_t)elf_size));
 CHECK(!gh_game_math_static_initialize_003079c8(&math,&work,1,0xffff));
 gh_collision_pairs pairs={0};
 CHECK(!gh_collision_pairs_construct_002b8ae0(&pairs,heap)&&!gh_collision_pairs_initialize_002b8b50(&pairs));
 gh_game_init_state *init=malloc(sizeof(*init));
 CHECK(init);
 gh_game_init_state_initialize(init);
 uint32_t frontier;
 CHECK(!gh_hitarea_objects_construct_001fde00(init,&frontier)&&!gh_game_object_pool_reset_001fdea8(init));
 gh_hitarea_shape_bindings shapes={&pairs,&math,GH_STATIC_BINARY32_REFERENCE};
 uint32_t fog=0;
 gh_om_initializer_bindings om_init={.work=&work,.packet_heap=heap,.fog_rgb=&fog,
     .init=init,.shapes=&shapes};
 gh_object_resource_bindings resources={0};
 gh_om_factory_bindings om_bind={.pools=&pools,.work=&work,.room_state=&room_state,
     .objects=global,.room_objects=local,.factory_serial=&serial,
     .initializer=&om_init,.resources=&resources};
 gh_om_factory factory={0};
 CHECK(!gh_om_factory_bind(&factory,&om_bind,2));
 gh_scene_placement_status made=gh_om_factory_create_record(&factory,&record,&original);
 if(made)fprintf(stderr,"OM324 status %d frontier %08x detail %u\n",made,factory.frontier,factory.detail);
 CHECK(!made&&original&&factory.count==1);
 CHECK(!gh_om_factory_create_record(&factory,&second_record,&original)&&original&&factory.count==2);
 gh_om_object *object=factory.objects;
 gh_om324_state *om=(gh_om324_state *)object->initialization;
 gh_actor_model *model=&om->common.model;
 gh_om_object *second=object+1;
 gh_om324_state *second_om=(gh_om324_state *)second->initialization;
 gh_actor_model *second_model=&second_om->common.model;
 CHECK(model->ready&&model->root.bytes==object->view.bytes&&model->part_count==2);

 gh_audio_registry *registry;
 CHECK(!gh_audio_registry_create(&registry)&&!gh_audio_startup_load(registry,disc));
 gh_sound_control control={0};
 gh_sound_scene scene={0};
 CHECK(!gh_sound_control_from_startup(&control,disc,registry));
 CHECK(!gh_sound_scene_from_startup(&scene,&control,registry));
 gh_dvd_jobs dvd;
 gh_dvd_jobs_init(&dvd,disc);
 gh_task_record32 records[4]={0};
 gh_task_manager manager={.records=records,.capacity=4,.count=4,.current_index=-1};
 gh_task_threads *threads;
 CHECK(!gh_task_threads_create(&manager,&threads));
 sound_load load={&scene,&room,&work,&dvd,heap,threads,0};
 int32_t slot;
 CHECK(!gh_task_threads_start(threads,2,load_scene,&load,0,&slot));
 for(unsigned i=0;!load.done&&i<16;++i)
  CHECK(!gh_tasks_run_matching_002d52a0(&manager).status&&!gh_dvd_pump(&dvd));
 CHECK(load.done&&scene.ready&&!scene.seh.bytes&&!scene.seh.size);

 uint8_t program[48];
 gh_sound_span descriptor,tail;
 gh_adpcm_pcm decoded={0};
 CHECK(!gh_audio_get_program(registry,2,10,0,program));
 CHECK(program[4]==4&&!program[5]&&program[6]==23&&program[7]==2&&program[8]==84&&
       program[9]==2&&!program[0x21]&&program[0x22]==3);
 CHECK(!gh_audio_sample_at(registry,2,4,&descriptor,&tail)&&descriptor.size==16);
 CHECK(!gh_audio_decode_ps_adpcm(tail.data,tail.size,&decoded)&&decoded.frames==38248&&decoded.source_bytes==21856);
 gh_audio_pcm_release(&decoded);

 /* The pending test supplies an identity only. No output API is entered. */
 max_align_t unused_output={0};
 gh_se_mix mix;
 gh_se_voice_engine *voices;
 CHECK(!gh_se_mix_boot_defaults_002c9e78(&mix,0,GH_SE_DSP_NATIVE_LINEAR_REFERENCE));
 CHECK(!gh_se_voice_create(registry,(gh_audio_output *)&unused_output,&mix,&voices));
 gh_game_mt_seed_0030f2b0(&init->random,1);
 gh_game_mt expected_random=init->random;
 gh_actor_attached_sound source={0};
 gh_actor_attached_sound_bindings bindings={voices,&control,&scene,&work,&init->random};
 CHECK(!gh_actor_attached_sound_bind(&source,&bindings));
 gh_motion_sound_request request={.model=model,.actor=&object->view,.bank=2,.program=10};
 uint32_t handle;
 CHECK(!fesetround(FE_UPWARD)&&!feclearexcept(FE_ALL_EXCEPT)&&!feraiseexcept(FE_DIVBYZERO));
 gh_actor_motion_status status=gh_actor_attached_sound_motion(&source,&request,&handle,&frontier);
 if(status)fprintf(stderr,"attached status %d frontier %08x detail %d\n",status,frontier,source.detail);
 CHECK(!status&&handle&&!frontier);
 CHECK(fegetround()==FE_UPWARD&&fetestexcept(FE_ALL_EXCEPT)==FE_DIVBYZERO);
 CHECK(!fesetround(FE_TONEAREST));
 gh_se_voice_info info;
 CHECK(!gh_se_voice_query(voices,handle,&info)&&info.pending&&!info.native_voice&&info.attached);
 CHECK(info.attached_model==model&&info.attached_part==-1&&info.bank==2&&info.program==10&&
       info.sample_slot==4&&info.rate==18000&&info.adsr1==0x80ff&&info.adsr2==0x5fc0);
 CHECK(info.position_mode==2&&!info.pitch_q12&&model->sound_borrowers==1);
 float position[4];
 memcpy(position,object->view.alias_f0,sizeof(position));
 CHECK(info.source_position[0]==position[0]&&info.source_position[1]==position[1]+1.3f&&
       info.source_position[2]==position[2]);
 uint32_t duplicate=999;
 CHECK(!gh_actor_attached_sound_motion(&source,&request,&duplicate,&frontier)&&!duplicate&&!frontier);
 CHECK(gh_se_voice_active_nodes(voices)==1&&model->sound_borrowers==1&&!gh_se_voice_attached_reservations(voices));
 /* Two original placements request this sound before the next audio tick.
  * Pin the canonical listener at each source to exercise both decisions. */
 memcpy(control.listener_position_c0,info.source_position,sizeof(info.source_position));
 gh_motion_sound_request second_request={.model=second_model,.actor=&second->view,.bank=2,.program=10};
 CHECK(!gh_actor_attached_sound_motion(&source,&second_request,&duplicate,&frontier)&&!duplicate&&!frontier);
 CHECK(gh_se_voice_active_nodes(voices)==1&&model->sound_borrowers==1&&!second_model->sound_borrowers);
 CHECK(!gh_se_voice_query(voices,handle,&info)&&info.pending&&!info.native_voice);
 float second_position[4];
 memcpy(second_position,second->view.alias_f0,sizeof(second_position));
 second_position[1]+=1.3f;
 memcpy(control.listener_position_c0,second_position,sizeof(second_position));
 CHECK(!gh_actor_attached_sound_motion(&source,&second_request,&duplicate,&frontier)&&duplicate&&!frontier);
 CHECK(gh_se_voice_query(voices,handle,&info)==GH_VOICE_BOUNDS);
 CHECK(!gh_se_voice_query(voices,duplicate,&info)&&info.pending&&info.attached_model==second_model);
 CHECK(gh_se_voice_active_nodes(voices)==1&&!model->sound_borrowers&&second_model->sound_borrowers==1&&
       !gh_se_voice_attached_reservations(voices)&&!memcmp(&expected_random,&init->random,sizeof(expected_random)));
 /* No libsd pitch reference is bound and no output device may be entered. */
 CHECK(gh_se_voice_update(voices)==GH_VOICE_UNSUPPORTED&&gh_se_voice_frontier(voices)==0x37d760);
 CHECK(!gh_se_voice_query(voices,duplicate,&info)&&info.pending&&!info.native_voice);
 CHECK(!gh_actor_attached_sound_abort(&source)&&!second_model->sound_borrowers&&
       !gh_se_voice_active_nodes(voices)&&!gh_se_voice_attached_reservations(voices));
 gh_se_voice_destroy(voices);
 CHECK(!gh_om_factory_release(&factory)&&!gh_game_object_pool_release_001fdfa8(init,&frontier));
 CHECK(!gh_collision_pairs_release(&pairs)&&!gh_task_threads_destroy(threads));
 gh_game_objects_destroy(local);
 gh_game_objects_destroy(global);
 CHECK(!gh_game_room_pools_release(&pools)&&!gh_heap_destroy(heap));
 gh_audio_registry_destroy(registry);
 gh_disc_close(disc);
 free(init);
 free(elf);
 free(room_bytes);
 free(object_bytes);
 puts("PASS actual R100 OMS49/50 and R100.SES program10, equal/far duplicate suppression and nearer pending replacement (no device)");
 return 0;
}
