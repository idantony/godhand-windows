#include "r020_title.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fenv.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static void w32(uint8_t*p,uint32_t n){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(n>>(8*i));}
static uint32_t u32(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static uint8_t*readfile(gh_disc*d,const char*p,size_t*n){size_t i;gh_disc_entry e;CHECK(!gh_disc_find(d,p,&i)&&!gh_disc_entry_at(d,i,&e));*n=(size_t)e.size;uint8_t*b=malloc(*n);CHECK(b&&!gh_disc_read(d,i,0,b,*n));return b;}
typedef struct {gh_id_draw_job jobs[64];unsigned count,begins,total,se_calls,last_program;uint64_t seen;} frontend;
static gh_r020_status begin_frame(void*p,const gh_id_base*b,uint32_t*f){frontend*q=p;(void)f;if(b->resource_number!=2||b->id!=0)return GH_R020_ID;q->count=0;q->begins++;return GH_R020_OK;}
static gh_r020_status enqueue_draw(void*p,const gh_id_base*b,const gh_id_draw_job*j,uint32_t*f){frontend*q=p;(void)f;if(!j||j->entry_index>=b->entry_count||q->count>=64)return GH_R020_ID;q->jobs[q->count++]=*j;q->total++;if(j->entry->id>=0&&j->entry->id<64)q->seen|=UINT64_C(1)<<(unsigned)j->entry->id;return GH_R020_OK;}
/* Test-only semantic consumer records exact requests. Actual native SE device
 * consumption is independently verified by test_se_voice, not this fixture. */
static gh_r020_status program_se(void*p,unsigned bank,unsigned program,uint32_t*f){frontend*q=p;(void)f;CHECK((bank==0&&(program==0x160||program==0x161||program==0x15e))||(bank==2&&(program==0||program==10||program==11||program==12||program==15)));q->se_calls++;q->last_program=program;return GH_R020_OK;}
static void check_consumer_types(const gh_id_base*b){for(uint32_t i=0;i<b->entry_count;i++)CHECK(b->entries[i].source&&b->entries[i].kind==b->entries[i].source[0x66]);}
int main(int argc,char**argv){
 gh_disc*d;gh_audio_registry*r;gh_sound_control control={0};gh_heap*h;gh_dvd_jobs dvd;gh_bgm_requests*bgm;
 CHECK(argc==2&&!gh_disc_open(argv[1],&d)&&!gh_audio_registry_create(&r)&&!gh_heap_create(24*1024*1024,&h));
 CHECK(!gh_audio_startup_load(r,d)&&!gh_sound_control_from_startup(&control,d,r)&&!gh_bgm_requests_create(r,&control,d,h,&bgm));
 size_t lang_n,jump_n;uint8_t*lang=readfile(d,"etc/core_usa.bin",&lang_n),*jump=readfile(d,"etc/debugres/RoomJmpTbl.dat",&jump_n);gh_bundle language;gh_bundle_entry language_idd;
 CHECK(!gh_bundle_open(lang,lang_n,&language)&&!gh_id_find_language_data(&language,&language_idd));
 for(unsigned alternate=0;alternate<2;alternate++){
  gh_boot_work work={0};work.language=alternate?99:1;work.room=0x20;uint8_t*raw=(uint8_t*)&work;
  memset(raw+0x3e0,0xff,8);memset(raw+0x270,0xff,8);raw[0x584]=4;if(alternate)raw[0x58a]=0x20;
  gh_id_state*id=calloc(1,sizeof(*id));gh_id_runtime*runtime;gh_game_mt mt;uint8_t save_bytes[GH_CORE_SAVE_BYTES];gh_core_save_view save={save_bytes,sizeof(save_bytes)};
  CHECK(id&&!gh_id_system_init_002acc70(id,&language_idd)&&!gh_id_runtime_create(id,1,&runtime));gh_game_mt_seed_0030f2b0(&mt,0x1234);CHECK(!gh_core_save_system_init(save));
  gh_dvd_jobs_init(&dvd,d);gh_r020_title*title=NULL;gh_screen_fade fade={0};
  gh_r020_title_bindings b={.work=&work,.save=save,.random=&mt,.dvd=&dvd,.room_heap=h,.id=id,.runtime=runtime,.bgm=bgm,.fade=&fade,.room_jump={jump,jump_n,0}};
  CHECK(gh_r020_title_init_prefix(&b,&title)==GH_R020_UNSUPPORTED&&title);
  gh_r020_title_view v;CHECK(!gh_r020_title_inspect(title,&v)&&v.frontier==0x329438&&!v.initialized&&!v.exit_registered);
  CHECK(v.id_base->entry_count==38&&v.id_base->resource_number==2&&v.id_base->id==0&&id->pool_used==38&&id->job_count==1);
  CHECK(v.idd.size==1289856&&strstr(v.path,alternate?"id/idtitlecore.idd":"id/local/US/idtitlecore_US.idd"));
  CHECK((u32(raw+0x5d8)&0xf00c6000)==0xf00c6000&&(u32(raw+0x5e0)&0xff800000)==0xff800000&&(u32(raw+0x5e4)&0x4000)&&u32(raw+0x10)==1);
  CHECK(u32(raw+0x3e0)==0xffff8016&&u32(raw+0x3e4)==UINT32_MAX&&u32(raw+0x270)==0xffff8016&&u32(raw+0x60c)==0);
  CHECK(u32(v.fields+0x14)==1&&u32(v.fields+0x18)==1&&u32(v.fields+0x1c)==gh_core_save_difficulty(save));
  CHECK(u32(v.fields+0x50)==0x20&&u32(v.fields+0x64)==0x25&&u32(v.fields+0x68)==(alternate?0x26u:0xbu));
  /* TailafterBgmEventSet did NOTexecute: sourceunknownbytes poison remains,
   * Work584bit800 absent and noScenarioexitregistration was fabricated. */
  CHECK(v.fields[0]==0xcd&&u32(v.fields+0xc)==0xcdcdcdcd&&!(u32(raw+0x584)&0x800));
  for(int i=0x10;i<=0x11;i++){gh_id_entry*e;int fallback;CHECK(!gh_id_base_get_work_002aa410(runtime,(gh_id_base*)v.id_base,i,&e,&fallback)&&!fallback&&(e->flags&0x20000000));}
  CHECK(gh_bgm_requests_pending(bgm)==1&&!gh_bgm_requests_active(bgm));
  CHECK(gh_r020_title_init_prefix(&b,&title)==GH_R020_BUSY);
  gh_id_pool_run later;int freed;CHECK(!gh_id_pool_allocate_002ace48(runtime,1,777,0,&later));CHECK(gh_r020_title_destroy(title)==GH_R020_BUSY&&u32(raw+0x10)==1);
  CHECK(!gh_id_pool_release_002acf30(runtime,later,777,&freed)&&freed);CHECK(!gh_r020_title_destroy(title));title=NULL;
  CHECK(!id->pair[2].bound&&!id->pool_used&&!id->job_count&&u32(raw+0x10)==0&&!(u32(raw+0x5e4)&0x4000));
  CHECK(!gh_bgm_requests_abort_pending(bgm)&&!gh_heap_used(h)&&!gh_id_runtime_destroy(runtime));free(id);
 }
 /* With the real nativeBGM backend, originalInitialize tail andregisteredEnd
  * genuinelycomplete; still noMovie/Fade/Logo update isexecuted byInit. */
 size_t elf_size=(size_t)gh_disc_boot_elf_size(d);uint8_t*elf=malloc(elf_size);gh_adx_key key;gh_display_state display={0};
 CHECK(elf&&!gh_disc_read_boot_elf(d,0,elf,elf_size)&&!gh_adx_key_from_elf(elf,elf_size,&key)&&!gh_display_state_from_elf(&display,elf,elf_size));free(elf);CHECK(!gh_display_static_initialize_002a48a8(&display,1,0xffff));
 CHECK(!gh_bgm_requests_enable_native(bgm,&key,1,GH_BGM_GAIN_NATIVE_DB_REFERENCE));
 gh_boot_work work={0};work.language=1;work.room=0x20;gh_screen_fade shared_fade={0};gh_id_state*id=calloc(1,sizeof(*id));gh_id_runtime*runtime;gh_game_mt mt;
 uint8_t save_bytes[GH_CORE_SAVE_BYTES];gh_core_save_view save={save_bytes,sizeof(save_bytes)};gh_r020_title*title=NULL;gh_scene_hook scenario_exit={.kind=GH_SCENE_HOOK_ABSENT};
 CHECK(id&&!gh_id_system_init_002acc70(id,&language_idd)&&!gh_id_runtime_create(id,1,&runtime)&&!gh_core_save_system_init(save));gh_game_mt_seed_0030f2b0(&mt,1);gh_dvd_jobs_init(&dvd,d);
 gh_r020_title_bindings bindings={.work=&work,.save=save,.random=&mt,.dvd=&dvd,.room_heap=h,.id=id,.runtime=runtime,.bgm=bgm,.room_jump={jump,jump_n,0},.scenario_exit=&scenario_exit,.fade=&shared_fade,.display=&display};
 CHECK(!gh_r020_title_init(&bindings,&title));gh_r020_title_view final;CHECK(!gh_r020_title_inspect(title,&final)&&final.initialized&&final.exit_registered&&!final.frontier);
 CHECK(final.fields[0]==1&&!final.fields[1]&&!final.fields[2]&&!final.fields[3]&&u32(final.fields+0xc)==2&&final.fields[0xd8]==0&&final.fields[0xd9]==1);
 CHECK((work.flags&0x800)&&scenario_exit.kind==GH_SCENE_HOOK_NATIVE&&scenario_exit.context==&title&&gh_bgm_requests_active(bgm)==1);
 gh_bgm_request_view request;CHECK(!gh_bgm_requests_inspect(bgm,0,&request)&&request.prepared&&!request.transport.started&&!request.transport.submitted_frames);
 frontend queued={0};gh_r020_move_callbacks move_callbacks={.context=&queued,.begin_frame=begin_frame,.draw_entry=enqueue_draw};
 unsigned moves=0;gh_r020_status step=GH_R020_OK;
 while(!step&&moves<100){
  step=gh_r020_title_move(title,&move_callbacks);moves++;
  CHECK(!gh_r020_title_inspect(title,&final)&&final.move_serial==moves);
  if(step)break;
  gh_screen_fade_context fc={512,1824,0,0};gh_fade_draw fade_draw;CHECK(!gh_screen_fade_step_00307388(&shared_fade,&fc,&fade_draw));
 }
 CHECK(step==GH_R020_UNSUPPORTED&&final.frontier==0x2cb8a0&&final.fields[1]==11&&moves>15&&queued.begins==moves);
 CHECK(queued.total>0&&(queued.seen&(UINT64_C(1)<<0x11))&&u32(final.fields+8)==0xcdcdcdcd);
 CHECK((shared_fade.state&4)&&shared_fade.count==shared_fade.time);
 /* Thisfrontend fixture trulyqueues originaltypedjobs; GPUexecution is the
  * Root native renderer's separate integrationtest, not claimedhere. */
 uint32_t end_frontier=99;CHECK(scenario_exit.call(scenario_exit.context,&end_frontier)==GH_SCENE_OK&&!title&&!end_frontier&&scenario_exit.kind==GH_SCENE_HOOK_ABSENT);
 CHECK(!id->pool_used&&!id->job_count&&!id->pair[2].bound&&gh_bgm_requests_active(bgm)==1&&gh_heap_used(h)>0);
 CHECK(!gh_id_runtime_destroy(runtime));free(id);
 gh_bgm_requests_destroy(bgm);
 /* Original Wait starts40 then decrements on its first call: 40 calls leave0,
  * call41 entersTitle; no drawing or hidden Fade stepping during Wait. */
 for(unsigned reference=0;reference<4;reference++){
  CHECK(!gh_bgm_requests_create(r,&control,d,h,&bgm)&&!gh_bgm_requests_enable_native(bgm,&key,1,GH_BGM_GAIN_NATIVE_DB_REFERENCE));
  memset(&work,0,sizeof(work));work.language=1;work.room=0x20;work.flags=0x800;memset(&shared_fade,0,sizeof(shared_fade));memset(&queued,0,sizeof(queued));
  id=calloc(1,sizeof(*id));CHECK(id&&!gh_id_system_init_002acc70(id,&language_idd)&&!gh_id_runtime_create(id,1,&runtime));title=NULL;
  uint8_t supplied[0xe0]={0};w32(supplied+0x30,0x42b40000);
  const gh_r020_reference_profile profile={reference==2?GH_R020_MEMORY_PROVIDED_BYTES:GH_R020_MEMORY_ZERO_REFERENCE,reference==2?supplied:NULL,reference==2?sizeof(supplied):0,1};
  bindings.work=&work;bindings.id=id;bindings.runtime=runtime;bindings.bgm=bgm;bindings.reference_profile=reference?&profile:NULL;
  CHECK(!gh_r020_title_init(&bindings,&title));move_callbacks.program_se=program_se;
  for(unsigned tick=0;tick<40;tick++)CHECK(!gh_r020_title_move(title,&move_callbacks));
  CHECK(!gh_r020_title_inspect(title,&final)&&final.fields[0]==0&&final.fields[1]==1&&u32(final.fields+8)==0&&!queued.total);
  CHECK(!gh_r020_title_move(title,&move_callbacks)&&!gh_r020_title_inspect(title,&final)&&final.fields[0]==3&&!final.fields[1]&&u32(final.fields+8)==UINT32_MAX);
  if(!reference){CHECK(gh_r020_title_move(title,&move_callbacks)==GH_R020_UNSUPPORTED&&!gh_r020_title_inspect(title,&final)&&final.frontier_relative&&final.frontier==0xa18);}
  else {
   CHECK(!fesetround(FE_DOWNWARD));
   CHECK(!gh_r020_title_move(title,&move_callbacks)&&!gh_r020_title_inspect(title,&final)&&final.fields[1]==1&&queued.total&&fegetround()==FE_DOWNWARD);
   CHECK(!fesetround(FE_TONEAREST));
   CHECK(!gh_bgm_requests_inspect(bgm,0,&request)&&request.transport.started&&request.transport.submitted_frames&&request.transport.muted);
   gh_id_entry*alpha_entry;int fallback;CHECK(!gh_id_base_get_work_002aa410(runtime,(gh_id_base*)final.id_base,0x21,&alpha_entry,&fallback)&&!fallback);
   CHECK((alpha_entry->color_base>>24)==(reference==2?128u:0u));
   /* Reach PressWait, inject original Start/Cross pressedmask, then actual
    * six-tick confirmation and slide transition into three-choice menu. */
   CHECK(!gh_r020_title_move(title,&move_callbacks));uint8_t*raw=(uint8_t*)&work;w32(raw+0x1a4,0x80);
   CHECK(!gh_r020_title_move(title,&move_callbacks));w32(raw+0x1a4,0);
   for(unsigned tick=0;tick<15;tick++){CHECK(!gh_r020_title_move(title,&move_callbacks));CHECK(!gh_r020_title_inspect(title,&final));if(final.fields[1]==2)break;}
   CHECK(final.fields[1]==2&&queued.last_program==0x160);check_consumer_types(final.id_base);
   gh_id_entry*layer_entry;CHECK(!gh_id_base_get_work_002aa410(runtime,(gh_id_base*)final.id_base,6,&layer_entry,&fallback)&&!fallback&&layer_entry->layer==0);
   CHECK(!gh_r020_title_move(title,&move_callbacks)&&!gh_r020_title_inspect(title,&final)&&final.fields[2]==1&&u32(final.fields+0x14)==0);
   /* Down changes canonical selection and executes the original transition. */
   w32(raw+0x1b0,0x4000000);CHECK(!gh_r020_title_move(title,&move_callbacks));w32(raw+0x1b0,0);
   CHECK(!gh_r020_title_inspect(title,&final)&&u32(final.fields+0x14)==1&&final.fields[2]==2&&queued.last_program==0x161);
   for(unsigned tick=0;tick<20;tick++){CHECK(!gh_r020_title_move(title,&move_callbacks));CHECK(!gh_r020_title_inspect(title,&final));if(final.fields[2]==1)break;}
   CHECK(final.fields[2]==1&&u32(final.fields+0x14)==1);
   w32(raw+0x1a0,0x20000000);CHECK(!gh_r020_title_move(title,&move_callbacks));w32(raw+0x1a0,0);
   CHECK(!gh_r020_title_inspect(title,&final)&&final.fields[1]==1&&!final.fields[2]&&queued.last_program==0x15e);
   if(reference>=2){
    CHECK(!gh_r020_title_move(title,&move_callbacks));w32(raw+0x1a4,0x80);CHECK(!gh_r020_title_move(title,&move_callbacks));w32(raw+0x1a4,0);
    for(unsigned tick=0;tick<30;tick++){CHECK(!gh_r020_title_move(title,&move_callbacks));CHECK(!gh_r020_title_inspect(title,&final));if(final.fields[1]==2)break;}
    CHECK(final.fields[1]==2);CHECK(!gh_r020_title_move(title,&move_callbacks));
    if(reference==3){w32(raw+0x1b0,0x4000000);CHECK(!gh_r020_title_move(title,&move_callbacks));w32(raw+0x1b0,0);for(unsigned tick=0;tick<20;tick++){CHECK(!gh_r020_title_move(title,&move_callbacks));CHECK(!gh_r020_title_inspect(title,&final));if(final.fields[2]==1)break;}}
    for(unsigned tick=0;tick<4;tick++)CHECK(!gh_r020_title_move(title,&move_callbacks));w32(raw+0x1a4,0x80);CHECK(!gh_r020_title_move(title,&move_callbacks));w32(raw+0x1a4,0);
    step=GH_R020_OK;for(unsigned tick=0;tick<50&&!step;tick++){step=gh_r020_title_move(title,&move_callbacks);if(step)break;gh_screen_fade_context fc={512,1824,0,0};gh_fade_draw draw;CHECK(!gh_screen_fade_step_00307388(&shared_fade,&fc,&draw));}
    CHECK(step==GH_R020_UNSUPPORTED&&!gh_r020_title_inspect(title,&final)&&!final.frontier_relative&&final.frontier==(reference==2?0x2bf268u:0x160020u));
    CHECK(final.fields[2]==(reference==2?22:31));if(reference==3)CHECK(u32(raw+0x10)==0);
   }else {
   /* Return to game selection, confirm New Game, then choose actual level0. */
   CHECK(!gh_r020_title_move(title,&move_callbacks));w32(raw+0x1a4,0x80);CHECK(!gh_r020_title_move(title,&move_callbacks));w32(raw+0x1a4,0);
   for(unsigned tick=0;tick<30;tick++){CHECK(!gh_r020_title_move(title,&move_callbacks));CHECK(!gh_r020_title_inspect(title,&final));if(final.fields[1]==2)break;}
   CHECK(final.fields[1]==2);CHECK(!gh_r020_title_move(title,&move_callbacks));
   /* Previously selected1 persists across Cancel; move Up to0. */
   w32(raw+0x1b0,0x8000000);CHECK(!gh_r020_title_move(title,&move_callbacks));w32(raw+0x1b0,0);
   for(unsigned tick=0;tick<20;tick++){CHECK(!gh_r020_title_move(title,&move_callbacks));CHECK(!gh_r020_title_inspect(title,&final));if(final.fields[2]==1)break;}
   CHECK(final.fields[2]==1&&u32(final.fields+0x14)==0);
   for(unsigned tick=0;tick<4;tick++)CHECK(!gh_r020_title_move(title,&move_callbacks));w32(raw+0x1a4,0x80);CHECK(!gh_r020_title_move(title,&move_callbacks));w32(raw+0x1a4,0);
   for(unsigned tick=0;tick<40;tick++){CHECK(!gh_r020_title_move(title,&move_callbacks));CHECK(!gh_r020_title_inspect(title,&final));if(final.fields[1]==3)break;}
   CHECK(final.fields[1]==3);CHECK(!gh_r020_title_move(title,&move_callbacks));
   for(unsigned tick=0;tick<4;tick++)CHECK(!gh_r020_title_move(title,&move_callbacks));w32(raw+0x1a4,0x80);CHECK(!gh_r020_title_move(title,&move_callbacks));w32(raw+0x1a4,0);
   for(unsigned tick=0;tick<20;tick++){CHECK(!gh_r020_title_move(title,&move_callbacks));CHECK(!gh_r020_title_inspect(title,&final));if(final.fields[1]==4)break;}
   CHECK(final.fields[1]==4);
   step=GH_R020_OK;
   for(unsigned tick=0;tick<220&&!step;tick++){step=gh_r020_title_move(title,&move_callbacks);if(step)break;gh_screen_fade_context fc={512,1824,0,0};gh_fade_draw draw;CHECK(!gh_screen_fade_step_00307388(&shared_fade,&fc,&draw));CHECK(!gh_bgm_requests_update_002d0938(bgm,&work));}
   CHECK(step==GH_R020_UNSUPPORTED&&!gh_r020_title_inspect(title,&final)&&final.fields[0]==4&&final.frontier==0x2d5aa0&&!final.frontier_relative);
   work.flags|=0x400;CHECK(gh_r020_title_move(title,&move_callbacks)==GH_R020_UNSUPPORTED&&!gh_r020_title_inspect(title,&final)&&final.frontier==0x2c4548&&!final.frontier_relative);
   check_consumer_types(final.id_base);CHECK(!gh_id_base_get_work_002aa410(runtime,(gh_id_base*)final.id_base,6,&layer_entry,&fallback)&&!fallback&&layer_entry->layer==1);
   CHECK((work.flags&0x200)&&save.bytes[0x1f]==u32(final.fields+0x1c)&&raw[0x5b4]==0&&raw[0x5b5]==1&&!raw[0x5ba]&&!raw[0x5bb]);
   const uint8_t*room_record=NULL;for(uint32_t i=0;i<u32(jump);i++){CHECK(16+4*(size_t)i+4<=jump_n);const uint8_t*t=jump+16+4*i;if(((unsigned)t[0]|(unsigned)t[1]<<8)==0x100){room_record=t;break;}}
   CHECK(room_record&&room_record[2]);size_t vector_offset=(size_t)u32(jump+8)+(size_t)room_record[3]*48;CHECK(vector_offset+48<=jump_n&&!memcmp(raw+0x5c0,jump+vector_offset,12));

   }

  }
  CHECK(!gh_r020_title_destroy(title)&&!gh_id_runtime_destroy(runtime));free(id);gh_bgm_requests_destroy(bgm);CHECK(!gh_heap_used(h));
 }
 free(jump);free(lang);CHECK(!gh_heap_destroy(h));gh_audio_registry_destroy(r);gh_disc_close(d);
 puts("PASS realR020 Wait41/TitleStart/menu-navigation/cancel/NewGame/Level/Exit/Jump-frontier/explicit-memory/FEnv/Logo/cleanup");return 0;
}
