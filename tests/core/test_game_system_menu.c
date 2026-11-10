#include "game_system_menu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fenv.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint8_t*read_file(gh_disc*d,const char*p,size_t*n){size_t i;gh_disc_entry e;CHECK(!gh_disc_find(d,p,&i)&&!gh_disc_entry_at(d,i,&e));*n=(size_t)e.size;uint8_t*b=malloc(*n);CHECK(b&&!gh_disc_read(d,i,0,b,*n));return b;}
typedef struct {unsigned trans,jobs;} frontend;
/* Explicit test-only real typed-job collector, not a GPU draw acknowledgement. */
static gh_id_error collect(void*p,gh_id_base*b,uint32_t*f){frontend*q=p;gh_id_draw_job jobs[16];size_t n=0;gh_id_frontier stop={0};gh_id_error e=gh_id_jobs_collect_visible(b,jobs,16,&n,&stop);if(e){*f=stop.address;return e;}CHECK(b->id==2&&b->resource_number==1&&n>0);q->trans++;q->jobs+=(unsigned)n;return GH_ID_OK;}
int main(int argc,char**argv){
 gh_disc*d;CHECK(argc==2&&!gh_disc_open(argv[1],&d));size_t size;uint8_t*bytes=read_file(d,"etc/core_usa.bin",&size);gh_bundle language;gh_bundle_entry idd;CHECK(!gh_bundle_open(bytes,size,&language)&&!gh_id_find_language_data(&language,&idd));
 gh_id_state*id=calloc(1,sizeof(*id));gh_id_runtime*runtime;gh_game_mt random;uint8_t savebytes[GH_CORE_SAVE_BYTES];gh_core_save_view save={savebytes,sizeof(savebytes)};gh_game_cockpit*c;
 CHECK(id&&!gh_id_system_init_002acc70(id,&idd)&&!gh_id_runtime_create(id,1,&runtime)&&!gh_id_runtime_set_path_policy(runtime,GH_ID_PATH_REFERENCE_NEAREST)&&!gh_core_save_system_init(save));gh_game_mt_seed_0030f2b0(&random,1);CHECK(!gh_game_cockpit_create(runtime,id,&random,&c)&&!gh_game_cockpit_initialize_00140c88(c,save));
 gh_cockpit_system_menu_view view;CHECK(!gh_game_cockpit_get_system_menu(c,&view)&&view.size==0x30&&view.base->id==2&&view.base->resource_number==1);gh_id_entry*entry;int fallback;CHECK(!gh_id_base_get_work_002aa410(runtime,view.base,0,&entry,&fallback)&&!fallback);
 gh_game_mt before=random;gh_id_entry foreign={0};CHECK(gh_id_entry_restore_source_002aa2d0(view.base,&foreign)==GH_ID_ARGUMENT);
 view.base->frame=173;entry->jitter[0]=93;entry->color_base=0;entry->pos_x=999;entry->kind=99;entry->layer=99;gh_id_view texture=entry->texture;gh_id_entry*parent=entry->parent;
 CHECK(!gh_id_entry_restore_source_002aa2d0(view.base,entry)&&view.base->frame==173&&entry->jitter[0]==93&&entry->kind==entry->source[0x66]&&entry->layer==(int8_t)entry->source[0x59]&&entry->parent==parent&&entry->texture.bytes==texture.bytes&&!memcmp(&before,&random,sizeof(random)));
 const uint8_t*src=entry->source;entry->source=src+1;CHECK(gh_id_entry_restore_source_002aa2d0(view.base,entry)==GH_ID_BOUNDS);entry->source=src;
 gh_task_record32 records[4]={0};gh_task_manager manager={records,4,NULL,-1,4,0,NULL,NULL};gh_task_threads*threads;CHECK(!gh_task_threads_create(&manager,&threads));
 gh_game_system_menu menu={0};CHECK(!gh_system_menu_from_disc(&menu,d)&&!menu.active_3c2554&&menu.task_slot==-1);uint8_t enabled=1;frontend jobs={0};
 gh_system_menu_bindings b={.cockpit=c,.runtime=runtime,.random=&random,.global_threads=threads,.enabled_3c2555=&enabled,.frame={.context=&jobs,.trans=collect},.animation_profile={1,0},.binary32_reference=1};
 CHECK(!gh_system_menu_start_002b65b0(&menu,&b)&&menu.active_3c2554==1&&menu.task_live&&!menu.iterations&&!jobs.trans);int slot=menu.task_slot;uint32_t pool=id->pool_used,runs=id->job_count;
 CHECK(!gh_system_menu_start_002b65b0(&menu,&b)&&menu.task_slot==slot);
 CHECK(!fesetround(FE_DOWNWARD));gh_task_run_result visit=gh_tasks_run_matching_002d52a0(&manager);CHECK(visit.status==GH_TASK_OK&&fegetround()==FE_DOWNWARD);CHECK(!fesetround(FE_TONEAREST));
 CHECK(!menu.error&&!menu.frontier&&!menu.task_live&&!menu.active_3c2554&&!enabled&&menu.iterations==1&&jobs.trans==1&&jobs.jobs>0&&id->pool_used==pool&&id->job_count==runs);
 /* Disabled acknowledgement keeps original fiber and C loop alive, sleeps1,
  * then the same owner exits on its next actual selected visit after enable. */
 CHECK(!gh_system_menu_start_002b65b0(&menu,&b));visit=gh_tasks_run_matching_002d52a0(&manager);CHECK(visit.status==GH_TASK_OK&&!menu.error&&menu.task_live&&menu.active_3c2554==1&&menu.iterations==2);enabled=1;visit=gh_tasks_run_matching_002d52a0(&manager);CHECK(visit.status==GH_TASK_OK&&!menu.error&&!menu.task_live&&!menu.active_3c2554&&!enabled&&menu.iterations==3);
 CHECK(!gh_system_menu_cancel(&menu)&&!gh_task_threads_destroy(threads));CHECK(!gh_game_cockpit_destroy(c)&&!gh_id_runtime_destroy(runtime));free(id);free(bytes);gh_disc_close(d);puts("PASS actualCockpitID2/source-restore-bounds-noRNG/canonical-scalars/real-systemmenu-fiber/sleep1/typed-trans/FEnv/no-new-ID-runs");return 0;
}
