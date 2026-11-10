#include "game_services.h"
#include "../core/game_gate.h"
#include "../core/sound_scene_tick.h"
#include "../platform/audio_startup.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
typedef struct {
 gh_game_services_bindings bindings;gh_game_services *services;gh_game_gate gate;
 gh_audio_registry *audio;gh_bgm_requests *bgm;gh_game_event event;gh_display_state display;gh_screen_fade fade;gh_disc *disc;gh_sound_control sound_control;gh_sound_scene sound_scene;int main_returned,assets_returned;gh_game_status main_result,assets_result;
 unsigned calls[4];
} context;
static int cover(void*p,gh_game_loop*l){context*c=p;(void)l;return gh_game_gate_002a6ca8(&c->gate);}
static int modal(void*p,gh_game_loop*l){context*c=p;(void)l;return gh_game_gate_002a6da8(&c->gate);}
static int state0(void*p,gh_game_loop*l){context*c=p;c->calls[0]++;return gh_game_init_002a5b18(c->bindings.init,c->bindings.work,&l->game_state[0],c->bindings.rate,c->bindings.save);}
static int state1(void*p,gh_game_loop*l){context*c=p;(void)l;c->calls[1]++;if(!c->services){gh_game_services_status e=gh_game_services_create(&c->bindings,&c->services);if(e)return e;
 gh_game_services_scene_view scene;CHECK(gh_game_services_get_scene_view(c->services,&scene)==GH_SERVICES_OK);
 CHECK(scene.scenario_bytes_size==0x114&&scene.scenario_bytes[15]==0x3f&&scene.scenario_bytes[0x112]==0);scene.scenario_bytes[0x14]=0xa5;
 memset(scene.scene_buffer_a,0xa5,scene.scene_buffer_a_size);memset(scene.scene_buffer_b,0xa5,scene.scene_buffer_b_size);*scene.main_word4=999;
 for(unsigned i=0;i<scene.main_record_count;i++){scene.main_records[i].kind=123;scene.main_records[i].handle=777;scene.main_records[i].field=99;}
 }return gh_game_services_state1(c->services);}
static int state2(void*p,gh_game_loop*l){context*c=p;(void)l;c->calls[2]++;return gh_game_services_state2(c->services);}
static int state3(void*p,gh_game_loop*l){context*c=p;(void)l;c->calls[3]++;return gh_game_services_state3(c->services);}
static void main_task(void*p,int32_t arg){context*c=p;(void)arg;gh_game_platform platform={.context=c,.dvd_cover=cover,.modal_input=modal,.states={state0,state1,state2,state3,NULL,NULL}};c->main_result=gh_game_main_002a5998(c->bindings.loop,&platform);c->main_returned=1;}
static int sound(void*p){context*c=p;gh_audio_status e=gh_audio_startup_load(c->audio,c->disc);if(e)return e;
 if(!c->sound_control.initialized&&gh_sound_control_from_startup(&c->sound_control,c->disc,c->audio))return 1;
 if(!c->sound_scene.startup_bound&&gh_sound_scene_from_startup(&c->sound_scene,&c->sound_control,c->audio))return 1;
 if(!c->bgm){size_t bytes=(size_t)gh_disc_boot_elf_size(c->disc);uint8_t*elf=malloc(bytes);gh_adx_key key;
  CHECK(elf&&!gh_disc_read_boot_elf(c->disc,0,elf,bytes)&&!gh_adx_key_from_elf(elf,bytes,&key)&&!gh_game_event_from_elf(&c->event,elf,bytes)&&!gh_display_state_from_elf(&c->display,elf,bytes));free(elf);CHECK(!gh_display_static_initialize_002a48a8(&c->display,1,0xffff));
  CHECK(!gh_bgm_requests_create(c->audio,&c->sound_control,c->disc,c->bindings.assets->heap,&c->bgm)&&!gh_bgm_requests_enable_native(c->bgm,&key,1,GH_BGM_GAIN_NATIVE_DB_REFERENCE));c->bindings.bgm_requests=c->bgm;
 }return 0;
}
static void assets_task(void*p,int32_t arg){context*c=p;(void)arg;c->assets_result=gh_game_assets_0030d5c8(c->bindings.assets,gh_core_save_costume(c->bindings.save));c->assets_returned=1;}
static void *payload(gh_disc*d,const char*path,size_t*bytes){size_t index;gh_disc_entry e;CHECK(gh_disc_find(d,path,&index)==GH_DISC_OK&&gh_disc_entry_at(d,index,&e)==GH_DISC_OK);void*p=malloc((size_t)e.size);CHECK(p&&gh_disc_read(d,index,0,p,(size_t)e.size)==GH_DISC_OK);*bytes=(size_t)e.size;return p;}
int main(int argc,char**argv){
 gh_disc*disc;CHECK(argc==2&&gh_disc_open(argv[1],&disc)==GH_DISC_OK);size_t core_bytes,language_bytes;
 void*core=payload(disc,"etc/core.bin",&core_bytes),*language=payload(disc,"etc/core_usa.bin",&language_bytes);gh_bundle bundle;gh_bundle_entry idd;
 CHECK(gh_bundle_open(language,language_bytes,&bundle)==GH_BUNDLE_OK&&gh_id_find_language_data(&bundle,&idd)==GH_MENU_OK);
 for(unsigned borrowed=0;borrowed<2;borrowed++){
  gh_boot_work work={0};work.language=1;work.room=0x20;gh_game_init_state*init=calloc(1,sizeof(*init));gh_stage_case1*stage=calloc(1,sizeof(*stage));gh_id_state*id=calloc(1,sizeof(*id));
  gh_game_children*children=NULL,*original_child=NULL;gh_id_state*id_global=id;gh_id_runtime*runtime;gh_font_state font={0};gh_menu_state menu={0};gh_heap*heap;gh_dvd_jobs dvd;
  gh_task_record32 records[4]={0};gh_task_manager manager={.records=records,.capacity=4,.count=4,.current_index=-1};gh_task_threads*threads;
  gh_game_assets assets={0};gh_game_loop loop={0};uint8_t save_bytes[GH_CORE_SAVE_BYTES];gh_core_save_view save={save_bytes,sizeof(save_bytes)};float rate=0;
  CHECK(init&&stage&&id&&gh_heap_create(32*1024*1024,&heap)==GH_MEM_OK);gh_dvd_jobs_init(&dvd,disc);
  CHECK(gh_task_threads_create(&manager,&threads)==GH_THREAD_OK);gh_game_init_state_initialize(init);CHECK(gh_core_save_system_init(save)==GH_INIT_OK);
  CHECK(gh_id_system_init_002acc70(id,&idd)==GH_MENU_OK&&gh_id_runtime_create(id,1,&runtime)==GH_ID_OK);
  if(borrowed){CHECK(gh_game_children_create(NULL,&children)==GH_CHILD_OK);original_child=children;}
  assets=(gh_game_assets){.work=&work,.dvd=&dvd,.heap=heap,.threads=threads};loop=(gh_game_loop){.work=&work,.threads=threads};
  uint8_t em_set_first_byte=0xa5;context c={.disc=disc};gh_movie_pool*movie_pool;CHECK(gh_movie_pool_create_002b4960(&movie_pool)==GH_MOVIE_OK);CHECK(gh_audio_registry_create(&c.audio)==GH_AUDIO_OK);assets.sound_context=&c;assets.sound_startup=sound;
  c.bindings=(gh_game_services_bindings){.assets=&assets,.loop=&loop,.init=init,.stage=stage,.children=&children,.save=save,.rate=&rate,.work=&work,.threads=threads,.id=id,.id_global=&id_global,.runtime=&runtime,.font=&font,.menu=&menu,.core_bin=core,.core_bin_size=core_bytes,.language=&bundle,.sound_control=&c.sound_control,.movie_pool=movie_pool,.audio_registry=c.audio,.em_set_first_byte=&em_set_first_byte,.sound_scene=&c.sound_scene,.event=&c.event,.display=&c.display,.fade=&c.fade};
  c.gate=(gh_game_gate){.work=&work,.dvd=&dvd,.threads=threads,.game_state=loop.game_state};int32_t slot;
  CHECK(gh_task_threads_start(threads,0,main_task,&c,0,&slot)==GH_THREAD_OK&&slot==0);
  CHECK(gh_task_threads_start(threads,2,assets_task,&c,0,&slot)==GH_THREAD_OK&&slot==2);
  unsigned visits=0;while((!c.main_returned||!c.assets_returned)&&visits<200){CHECK(gh_tasks_run_matching_002d52a0(&manager).status==GH_TASK_OK);CHECK(gh_dvd_pump(&dvd)==GH_DVD_OK);if(c.sound_scene.startup_bound)CHECK(!gh_sound_scene_tick_002caf28(&c.sound_scene,&work,children));if(c.bgm)CHECK(!gh_bgm_requests_update_002d0938(c.bgm,&work));visits++;}
  CHECK(c.main_returned&&c.assets_returned&&c.main_result==GH_GAME_UNAVAILABLE&&c.assets_result==GH_GAME_OK);
  for(unsigned i=0;i<4;i++)CHECK(c.calls[i]==1);CHECK(c.services&&loop.game_state[0]==4&&rate==.5f);
  gh_game_services_snapshot snapshot;gh_game_services_inspect(c.services,&snapshot);
  CHECK(snapshot.frontier==0&&snapshot.cockpit_ready&&snapshot.id_entries==443&&snapshot.id_runs==40&&!snapshot.scenario_active&&snapshot.room_loaded&&snapshot.room_pools_ready);
  gh_game_services_scene_view scene;CHECK(gh_game_services_get_scene_view(c.services,&scene)==GH_SERVICES_OK);
  CHECK(scene.root->assets==&assets&&scene.root->init==init&&scene.root->children==&children&&scene.root->loop==&loop);
  CHECK(scene.scenario->records!=records&&scene.scenario->count==16&&scene.scenario_threads!=threads&&scene.scenario_bytes[0x14]==0xa5);
  CHECK(!em_set_first_byte&&scene.module->bound&&scene.module->hook4.kind==GH_SCENE_HOOK_ABSENT&&scene.module->hook8.kind==GH_SCENE_HOOK_NATIVE&&scene.module->hook12.kind==GH_SCENE_HOOK_NATIVE);
  CHECK(scene.title_player->ready&&scene.title_player->view.alias_f0==scene.title_player->view.bytes+0xb0&&scene.scene_tables->actors_ready&&!scene.collision_solids->unit_head);
  CHECK(scene.world_light->ready&&scene.model_packets->packet_count3c==0&&scene.sound_scene==&c.sound_scene);
  CHECK(scene.effects->ready&&scene.filters->ready&&scene.effects->resources[0].kind==0);
  CHECK(scene.sound_scene->ready&&scene.sound_scene->registry==c.audio&&scene.collision_pairs->records.pointer&&scene.collision_pairs->count==400);
  CHECK(scene.room_pack->loaded&&scene.room_pack->file_bytes==25568&&scene.pools->initialized&&scene.pl_camera->identity==&init->camera);
  CHECK(scene.room_state->active==scene.prepare->fields+8&&!*scene.main_word4&&scene.main_record_count==32);
  for(unsigned i=0;i<32;i++)CHECK(scene.main_records[i].kind==0xffff&&scene.main_records[i].handle==-1&&!scene.main_records[i].field);
  for(size_t i=0;i<scene.scene_buffer_a_size;i++)CHECK(!scene.scene_buffer_a[i]);for(size_t i=0;i<scene.scene_buffer_b_size;i++)CHECK(!scene.scene_buffer_b[i]);
  CHECK(scene.title&&scene.checkpoint->captured&&scene.scene_ui->ready&&scene.scene_at->ready&&scene.step->cooldown_1b0==15&&scene.room_texture_ready);
  gh_r020_title_view title_view;CHECK(!gh_r020_title_inspect(scene.title,&title_view)&&title_view.initialized&&title_view.exit_registered&&title_view.id_base->entry_count==38);
  CHECK(scene.event==&c.event&&scene.display==&c.display&&scene.root->bgm_requests==c.bgm&&scene.scenario_exit->kind==GH_SCENE_HOOK_NATIVE);
  int32_t selected;CHECK(gh_bgm_requests_scene_event(c.bgm,&selected)&&selected==2&&!gh_bgm_requests_active(c.bgm));
  CHECK(loop.failed_entry==0x2a6690); /*case3completed;thisfixturedoesnotinstallstate4*/
  CHECK(id_global==id&&children&&(borrowed?children==original_child:1));CHECK(stage->global_slots.count==80&&stage->room_slots.count==32&&stage->skills.image&&stage->skills.count==114);
  CHECK(stage->assets==&assets&&stage->game_state==loop.game_state&&init->active_camera==&init->camera);
  if(!borrowed){gh_game_children*created=children;children=NULL;CHECK(gh_game_services_destroy(c.services)==GH_SERVICES_BUSY);children=created;}
  gh_stage_textures*created_texture=stage->textures;stage->textures=NULL;CHECK(gh_game_services_destroy(c.services)==GH_SERVICES_BUSY);stage->textures=created_texture;
  /* A laterindependentIDallocationmustblockdestruction beforeownermutation. */
  gh_id_pool_run later;int freed;CHECK(gh_id_pool_allocate_002ace48(runtime,1,777,0,&later)==GH_ID_OK);
  CHECK(gh_game_services_destroy(c.services)==GH_SERVICES_BUSY);gh_game_services_inspect(c.services,&snapshot);CHECK(snapshot.cockpit_ready&&children);
  CHECK(gh_id_pool_release_002acf30(runtime,later,777,&freed)==GH_ID_OK&&freed);
  CHECK(gh_game_services_destroy(c.services)==GH_SERVICES_OK);c.services=NULL;CHECK(c.sound_scene.startup_bound&&!c.sound_scene.ready&&c.sound_scene.registry==c.audio);
  CHECK(id->pool_used==0&&id->job_count==0&&!stage->global_slots.configured&&!stage->room_slots.configured&&!stage->textures&&!stage->skills.image);
  CHECK(borrowed?children==original_child:children==NULL);if(children)gh_game_children_destroy(children);
  CHECK(gh_task_threads_destroy(threads)==GH_THREAD_OK&&gh_id_runtime_destroy(runtime)==GH_ID_OK);
  gh_bgm_requests_destroy(c.bgm);CHECK(gh_game_assets_release(&assets)==GH_GAME_OK&&gh_heap_used(heap)==0&&gh_heap_destroy(heap)==GH_MEM_OK);
  CHECK(gh_movie_pool_destroy(movie_pool)==GH_MOVIE_OK);gh_audio_registry_destroy(c.audio);free(init);free(stage);free(id);
  printf("services %s child identity: true GameMain0/1/2/3,443entries40runs, assets finished %u ticks, realcase3complete->state4boundary2A6690, all owned resources released\n",borrowed?"borrowed":"owned",visits);
 }
 free(core);free(language);gh_disc_close(disc);puts("PASS actual GameMain/DVD/audio/assets integrated services, canonical identity, nested Scenario lifetime and safe cleanup");return 0;
}
