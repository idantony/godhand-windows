#include "game_services.h"
#include "../core/game_cockpit.h"
#include "../core/game_objects.h"
#include "../core/game_prepare.h"
#include "../core/game_room_state.h"
#include "../core/game_room_pack.h"
#include "../core/game_room_pools.h"
#include "../core/game_camera.h"
#include <stdlib.h>
#include <string.h>
struct gh_game_services {
 gh_game_services_bindings b;gh_game_cockpit *cockpit;
 gh_game_objects *objects,*room_objects;gh_game_prepare prepare;
 gh_task_record32 scenario_records[16];gh_task_manager scenario;gh_task_threads *scenario_threads;
 uint8_t scenario_scalar[0x114]; /*canonical5FEB00scalarspan;managerhasitsowntypedowner*/
 gh_room_state room_state;gh_room_pack room_pack;gh_room_pools pools;gh_game_pl_camera camera;
 gh_room_module module;gh_sound_scene sound_scene;gh_collision_pairs collision_pairs;unsigned case3_stage;
 gh_game_effects effects;gh_scene_filters filters;
 gh_stage_object_init old_children,old_cockpit;void *old_context;
 int owns_children,owns_textures,owns_skills,stage_attempted;
 gh_game_children *created_children;gh_stage_textures *created_textures;
 uint32_t cockpit_pool_end,cockpit_jobs_end;int cockpit_snapshot;
 uint32_t frontier;int detail;gh_game_services_status status;uint32_t main_word4;
 gh_game_main_record main_records[32];
 uint8_t scene_buffer_a[0x100180],scene_buffer_b[0x26060];
};
static uint32_t u32(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static void w32(uint8_t*p,uint32_t n){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(n>>(8*i));}
static gh_game_services_status fail(gh_game_services*s,gh_game_services_status e,uint32_t entry,int detail){s->frontier=entry;s->detail=detail;s->status=e;return e;}
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
 if(b->stage->global_slots.configured||b->stage->room_slots.configured||b->stage->skills.image)return GH_SERVICES_BUSY;
 s=calloc(1,sizeof(*s));if(!s)return GH_SERVICES_MEMORY;s->b=*b;
 s->old_context=b->stage->context;s->old_children=b->stage->init_children_15fd98;s->old_cockpit=b->stage->init_cockpit_140c88;
 oe=gh_game_objects_create(&b->stage->global_slots,0,&s->objects);if(oe)goto create_failure;
 oe=gh_game_objects_create(&b->stage->room_slots,1,&s->room_objects);if(oe)goto create_failure;
 ce=gh_game_camera_construct_pl_00139bc8(&s->camera,&b->init->camera,GH_CAMERA_NATIVE_BINARY32_REFERENCE);if(ce){result=GH_SERVICES_ASSET;goto create_failure;}
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
 *out=s;return GH_SERVICES_OK;
create_failure:
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
 if(e)return fail(s,GH_SERVICES_ASSET,0x2a5d40,e);s->frontier=0;s->detail=0;s->status=GH_SERVICES_OK;return GH_SERVICES_OK;
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
 if(!(work->flags&0x10))return fail(s,GH_SERVICES_FRONTIER,0x2b65b0,0);
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
  if(!s->b.audio_registry)return fail(s,GH_SERVICES_FRONTIER,0x2cabe8,0);
  if(!s->sound_scene.startup_bound){se=gh_sound_scene_from_startup(&s->sound_scene,s->b.sound_control,s->b.audio_registry);if(se)return fail(s,GH_SERVICES_ASSET,0x2cabe8,se);}
  if(gh_game_room_pack_bundle(&s->room_pack,&bundle))return fail(s,GH_SERVICES_ASSET,0x31ccf0,0);
  se=gh_sound_scene_initialize_002cabe8(&s->sound_scene,&bundle,work,s->b.assets->dvd,s->b.assets->heap,s->b.threads);
  if(se)return fail(s,se==GH_SOUND_SCENE_UNSUPPORTED?GH_SERVICES_FRONTIER:GH_SERVICES_ASSET,s->sound_scene.frontier?s->sound_scene.frontier:0x2cabe8,se);
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
 return fail(s,GH_SERVICES_FRONTIER,0x2d6f40,0);
}
void gh_game_services_inspect(const gh_game_services*s,gh_game_services_snapshot*out){
 if(!out)return;memset(out,0,sizeof(*out));if(!s)return;
 out->frontier=s->frontier;out->detail=s->detail;out->state=s->b.loop->game_state[0];out->cockpit_ready=gh_game_cockpit_ready(s->cockpit);
 out->id_entries=s->b.id->pool_used;out->id_runs=s->b.id->job_count;out->room_loaded=s->room_pack.loaded;out->room_pools_ready=s->pools.initialized;
 for(unsigned i=0;i<16;i++)out->scenario_active+=s->scenario_records[i].running!=0;
}
uint32_t gh_game_services_frontier(const gh_game_services*s){return s?s->frontier:0;}
int gh_game_services_detail(const gh_game_services*s){return s?s->detail:0;}
gh_game_services_status gh_game_services_destroy(gh_game_services*s){
 if(!s)return GH_SERVICES_ARGUMENT;
 if((s->owns_children&&*s->b.children!=s->created_children)||(s->owns_textures&&s->b.stage->textures!=s->created_textures))return fail(s,GH_SERVICES_BUSY,0x2a5bb8,0);
 if(s->cockpit_snapshot&&(s->b.id->pool_used!=s->cockpit_pool_end||s->b.id->job_count!=s->cockpit_jobs_end))return fail(s,GH_SERVICES_BUSY,0x2aa078,GH_ID_BUSY);
 if(gh_collision_pairs_release(&s->collision_pairs))return fail(s,GH_SERVICES_ASSET,0x2b8b50,0);
 if(s->sound_scene.job&&gh_dvd_check(s->b.assets->dvd,s->sound_scene.job))gh_dvd_cancel(s->b.assets->dvd,s->sound_scene.job);
 if(gh_sound_scene_release_loading(&s->sound_scene,s->b.assets->dvd,s->b.assets->heap))return fail(s,GH_SERVICES_ASSET,0x2cabe8,0);
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

gh_game_services_status gh_game_services_get_scene_view(gh_game_services*s,gh_game_services_scene_view*out){
 if(!s||!out)return GH_SERVICES_ARGUMENT;
 *out=(gh_game_services_scene_view){.root=&s->b,.prepare=&s->prepare,.scenario=&s->scenario,
  .scenario_threads=s->scenario_threads,.scenario_bytes=s->scenario_scalar,.scenario_bytes_size=sizeof(s->scenario_scalar),.room_state=&s->room_state,.room_pack=&s->room_pack,.pools=&s->pools,
  .pl_camera=&s->camera,.cockpit=s->cockpit,.objects=s->objects,.room_objects=s->room_objects,
  .module=&s->module,.sound_scene=&s->sound_scene,.collision_pairs=&s->collision_pairs,
  .effects=&s->effects,.filters=&s->filters,
  .main_word4=&s->main_word4,.main_records=s->main_records,.main_record_count=32,
  .scene_buffer_a=s->scene_buffer_a,.scene_buffer_b=s->scene_buffer_b,
  .scene_buffer_a_size=sizeof(s->scene_buffer_a),.scene_buffer_b_size=sizeof(s->scene_buffer_b)};
 return GH_SERVICES_OK;
}
