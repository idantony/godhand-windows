#include "sound_scene.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d: %s\n",__LINE__,#x);exit(1);}}while(0)
static uint8_t*read_file(gh_disc*d,const char*p,size_t*n){size_t i;gh_disc_entry e;CHECK(gh_disc_find(d,p,&i)==GH_DISC_OK&&gh_disc_entry_at(d,i,&e)==GH_DISC_OK);*n=(size_t)e.size;uint8_t*b=malloc(*n);CHECK(b&&gh_disc_read(d,i,0,b,*n)==GH_DISC_OK);return b;}
typedef struct {gh_sound_scene *scene;gh_bundle *bundle;gh_boot_work *work;gh_dvd_jobs*dvd;gh_heap*heap;gh_task_threads*threads;int returned;gh_sound_scene_status status;} context;
static void load(void*p,int32_t a){context*c=p;(void)a;c->status=gh_sound_scene_initialize_002cabe8(c->scene,c->bundle,c->work,c->dvd,c->heap,c->threads);c->returned=1;}
int main(int argc,char**argv){gh_disc*d;gh_audio_registry*r;gh_sound_control control={0};gh_sound_scene scene={0};gh_heap*h;gh_dvd_jobs dvd;gh_task_threads*t;gh_task_record32 records[4]={0};gh_task_manager m={.records=records,.capacity=4,.count=4,.current_index=-1};gh_boot_work w={0};size_t n;gh_bundle bundle;gh_sound_span prog,sys,samples,afd;
 CHECK(argc==2&&gh_disc_open(argv[1],&d)==GH_DISC_OK);CHECK(gh_audio_registry_create(&r)==GH_AUDIO_OK);
 CHECK(gh_sound_scene_from_startup(&scene,&control,r)==GH_SOUND_SCENE_ARGUMENT);
 CHECK(gh_audio_startup_load(r,d)==GH_AUDIO_OK&&gh_sound_control_from_startup(&control,d,r)==GH_VOICE_OK);
 CHECK(gh_sound_scene_from_startup(&scene,&control,r)==GH_SOUND_SCENE_OK&&scene.sound_group130==0);
 CHECK(!scene.ready&&!scene.fade_ratio_bits&&!scene.default_ratio_bits);
 CHECK(!scene.current_bank118&&!scene.current_request11c&&!scene.current_weight120);
 for(unsigned j=0;j<5;j++)CHECK(!scene.ratios60[j]&&!scene.ratios74[j]&&!scene.ratios88[j]);
 CHECK(gh_sound_scene_from_startup(&scene,&control,r)==GH_SOUND_SCENE_BUSY);
 uint8_t*pack=read_file(d,"st0/r020.dat",&n);CHECK(gh_bundle_open(pack,n,&bundle)==GH_BUNDLE_OK);
 CHECK(gh_heap_create(2*1024*1024,&h)==GH_MEM_OK);gh_dvd_jobs_init(&dvd,d);CHECK(gh_task_threads_create(&m,&t)==GH_THREAD_OK);
 w.room=0x20;memset((uint8_t*)&w+0x5e4,0xff,4);context c={&scene,&bundle,&w,&dvd,h,t,0,0};int32_t slot;unsigned visits=0;
 CHECK(gh_task_threads_start(t,2,load,&c,0,&slot)==GH_THREAD_OK);
 while(!c.returned&&visits<12){CHECK(gh_tasks_run_matching_002d52a0(&m).status==GH_TASK_OK);
  if(!visits){CHECK(scene.job&&scene.loading.pointer&&!scene.ready&&gh_heap_used(h)>0);CHECK(gh_audio_bank_views(r,2,&prog,&sys,&samples,&afd)==GH_AUDIO_NOT_FOUND);CHECK(gh_sound_scene_release_loading(&scene,&dvd,h)==GH_SOUND_SCENE_BUSY);}
  CHECK(gh_dvd_pump(&dvd)==GH_DVD_OK);visits++;
 }
 CHECK(c.returned&&c.status==GH_SOUND_SCENE_OK&&visits==3&&scene.ready&&!scene.loading.pointer&&gh_heap_used(h)==0);
 CHECK(scene.control==&control&&scene.registry==r&&gh_audio_token_complete(r,scene.bank2));
 CHECK(scene.sample_budget==0xc8000&&scene.current_bank118==-1&&scene.current_request11c==-2&&scene.current_weight120==-1);
 CHECK(!scene.seh.bytes&&!scene.bmh.bytes&&!scene.ts.bytes);
 for(unsigned i=0;i<5;i++)CHECK(scene.ratios60[i]==0x3f800000&&scene.ratios74[i]==0x3f800000&&scene.ratios88[i]==0x3c9374bc);
 uint32_t flags;memcpy(&flags,(uint8_t*)&w+0x5e4,4);CHECK(flags==0xfffff7ff);
 uint8_t*ses=read_file(d,"se/r020.ses",&n);gh_sound_bank original;CHECK(gh_sound_bank_open(ses,n,&original)==GH_SOUND_OK);
 CHECK(gh_audio_bank_views(r,2,&prog,&sys,&samples,&afd)==GH_AUDIO_OK&&!afd.data&&!afd.size);
 CHECK(prog.size==original.sections[0].size&&!memcmp(prog.data,original.sections[0].data,prog.size));CHECK(samples.size==original.sections[3].size&&!memcmp(samples.data,original.sections[3].data,samples.size));free(ses);
 CHECK(gh_audio_startup_ready(r,0,0,0));CHECK(gh_sound_scene_initialize_002cabe8(&scene,&bundle,&w,&dvd,h,t)==GH_SOUND_SCENE_BUSY);
 /* Actual fallback r100 has externalAFS: real read, exact unsupported boundary,
  * no replacement published. This is not a pretend successful audio map. */
 CHECK(gh_audio_release_bank(r,2)==GH_AUDIO_OK);scene=(gh_sound_scene){0};CHECK(gh_sound_scene_from_startup(&scene,&control,r)==GH_SOUND_SCENE_OK);w.room=-1;c.returned=0;c.status=GH_SOUND_SCENE_OK;visits=0;
 CHECK(gh_task_threads_start(t,2,load,&c,0,&slot)==GH_THREAD_OK);
 while(!c.returned&&visits++<12){CHECK(gh_tasks_run_matching_002d52a0(&m).status==GH_TASK_OK);CHECK(gh_dvd_pump(&dvd)==GH_DVD_OK);}
 CHECK(c.returned&&c.status==GH_SOUND_SCENE_UNSUPPORTED&&scene.frontier==0x2cf258&&!scene.ready&&scene.loading.pointer);
 CHECK(gh_audio_bank_views(r,2,&prog,&sys,&samples,&afd)==GH_AUDIO_NOT_FOUND);CHECK(gh_sound_scene_release_loading(&scene,&dvd,h)==GH_SOUND_SCENE_OK);
 CHECK(gh_task_threads_destroy(t)==GH_THREAD_OK&&gh_heap_destroy(h)==GH_MEM_OK);free(pack);gh_audio_registry_destroy(r);gh_disc_close(d);
 puts("PASS soundscene realR020/DVD/fiber/ownedbank2/defaults and realr100 unsupported fallback");return 0;}
