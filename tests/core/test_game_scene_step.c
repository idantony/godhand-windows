#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "../../src/core/game_scene_step.h"
#include "../../src/platform/disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL%d %s\n",__LINE__,#x);exit(1);}}while(0)
static void put(uint8_t*p,uint32_t n){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(n>>(i*8));}
static uint32_t get(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static uint32_t counter(void*unused){LARGE_INTEGER n;(void)unused;C(QueryPerformanceCounter(&n));return (uint32_t)n.QuadPart;}
typedef struct {gh_game_scene_step state;gh_game_scene_step_bindings binding;gh_task_record32 parent_records[4],child_records[16];gh_task_manager parent,child;uint8_t scalar[0x114];int entered,returned,mode,blocked,later;} fibers;
static gh_scene_step_status script_status(void*p,uint32_t*frontier){fibers*f=p;if(f->blocked){*frontier=0x2c5168;return GH_SCENE_STEP_UNSUPPORTED;}return GH_SCENE_STEP_OK;}
static void later_child(void*p,int32_t arg){fibers*f=p;(void)arg;f->later++;}
static void child_entry(void*p,int32_t arg){fibers*f=p;C(arg==17);f->entered++;if(f->mode==0){f->scalar[0x50]=2;f->scalar[0x51]=0;}C(gh_task_threads_sleep(f->binding.scenario_threads,1)==GH_THREAD_OK);f->entered++;}
static void parent_entry(void*p,int32_t arg){fibers*f=p;(void)arg;
 if(f->mode==2){C(gh_game_scene_tasks_002d5358(&f->state,&f->binding)==GH_SCENE_STEP_UNSUPPORTED&&f->state.frontier==0x2c5168&&f->entered==1&&!f->later&&f->state.scenario_task_cursor==3&&f->state.scenario_status_pending);C(gh_game_scene_tasks_002d5358(&f->state,&f->binding)==GH_SCENE_STEP_UNSUPPORTED&&f->entered==1&&!f->later);f->blocked=0;C(gh_game_scene_tasks_002d5358(&f->state,&f->binding)==GH_SCENE_STEP_OK&&f->entered==1&&f->later==1&&!f->state.scenario_status_pending);C(gh_task_threads_sleep(f->binding.main_threads,1)==GH_THREAD_OK);}
 if(f->mode==1){C(gh_game_scene_tasks_002d5358(&f->state,&f->binding)==GH_SCENE_STEP_THREAD&&f->child.current_index==4&&f->entered==1);f->child_records[4].running=0;C(gh_game_scene_tasks_002d5358(&f->state,&f->binding)==GH_SCENE_STEP_OK&&f->entered==1);C(gh_task_threads_sleep(f->binding.main_threads,1)==GH_THREAD_OK);}
 C(gh_game_scene_tasks_002d5358(&f->state,&f->binding)==GH_SCENE_STEP_OK&&f->entered==2);f->returned=1;
}
static void test_fibers(int mode){fibers*f=calloc(1,sizeof(*f));C(f);f->mode=mode;f->parent=(gh_task_manager){.records=f->parent_records,.capacity=4,.count=4,.current_index=-1};f->child=(gh_task_manager){.records=f->child_records,.capacity=16,.count=16,.current_index=-1,.flags=1};f->binding.scenario=(gh_scene_scenario_view){f->scalar,sizeof(f->scalar),&f->child};
 C(gh_task_threads_create(&f->parent,&f->binding.main_threads)==GH_THREAD_OK&&gh_task_threads_create(&f->child,&f->binding.scenario_threads)==GH_THREAD_OK);int32_t slot;C(gh_task_threads_start(f->binding.scenario_threads,2,child_entry,f,17,&slot)==GH_THREAD_OK&&slot==2);f->child_records[2].attributes=1;
 if(mode==1){f->child_records[4].running=2;f->child_records[4].attributes=1;}
 if(mode==2){f->blocked=1;f->binding.scenario_status=script_status;f->binding.scenario_status_context=f;C(!gh_task_threads_start(f->binding.scenario_threads,3,later_child,f,0,&slot)&&slot==3);f->child_records[3].attributes=1;}
 C(gh_task_threads_start(f->binding.main_threads,2,parent_entry,f,0,&slot)==GH_THREAD_OK);C(gh_tasks_run_matching_002d52a0(&f->parent).status==GH_TASK_OK&&!f->returned&&f->entered==1);
 if(!mode){C(gh_tasks_run_matching_002d52a0(&f->parent).status==GH_TASK_OK&&!f->returned);}
 C(gh_tasks_run_matching_002d52a0(&f->parent).status==GH_TASK_OK&&f->returned&&f->entered==2);C(f->child.step==gh_task_threads_step&&f->child.step_context==f->binding.scenario_threads&&f->child.current_index==-1&&!f->child.current&&!f->child_records[2].running);
 C(gh_task_threads_destroy(f->binding.scenario_threads)==GH_THREAD_OK&&gh_task_threads_destroy(f->binding.main_threads)==GH_THREAD_OK);free(f);
}
int main(int argc,char**argv){gh_disc*d;size_t index,elfn;gh_disc_entry asset;uint8_t*data,*elf;gh_bundle bundle;gh_bundle_entry pfd;gh_heap*heap;gh_game_scene_step step;gh_game_scene_step_bindings b={0};gh_boot_work work={0};gh_game_prepare prepare={0};gh_collision_pairs collision={0};gh_game_scene_tables actors={0};gh_scene_at_manager at={0};gh_game_action_buttons buttons={0};gh_game_event event;uint8_t scalar[0x114]={0},action_flag=255;gh_task_record32 records[16]={0};gh_task_manager scenario={.records=records,.capacity=16,.count=16,.current_index=-1};gh_game_init_state*init=calloc(1,sizeof(*init));C(argc==2&&init);
 C(gh_disc_open(argv[1],&d)==GH_DISC_OK&&gh_disc_find(d,"st0/r020.dat",&index)==GH_DISC_OK&&gh_disc_entry_at(d,index,&asset)==GH_DISC_OK);data=malloc((size_t)asset.size);C(data&&gh_disc_read(d,index,0,data,(size_t)asset.size)==GH_DISC_OK);elfn=(size_t)gh_disc_boot_elf_size(d);elf=malloc(elfn);C(elf&&gh_disc_read_boot_elf(d,0,elf,elfn)==GH_DISC_OK);gh_disc_close(d);
 C(gh_bundle_open(data,(size_t)asset.size,&bundle)==GH_BUNDLE_OK&&gh_bundle_find(&bundle,"PFD",0,&pfd)==GH_BUNDLE_NOT_FOUND);gh_game_scene_step_construct(&step,137,7);step.nearcheck.pair_count=99;gh_scene_nearcheck_reset_0031be78(&step.nearcheck);C(!step.nearcheck.pair_count&&step.nearcheck.cursor==137);C(gh_scene_nearcheck_bind_0031c1d0(&step.nearcheck,NULL)==GH_SCENE_STEP_OK);gh_game_scene_step_case3_cooldown(&step);C(step.cooldown_1b0==15);
 C(gh_heap_create(1024*1024,&heap)==GH_MEM_OK&&gh_collision_pairs_construct_002b8ae0(&collision,heap)==GH_COLLISION_OK&&gh_collision_pairs_initialize_002b8b50(&collision)==GH_COLLISION_OK);C(gh_scene_actors_0012e670(&actors,&bundle)==GH_TABLE_OK&&actors.actors_ready&&!actors.pending_actor_head);C(gh_scene_at_initialize_002c0f90(&at,&bundle)==GH_AT_OK);gh_game_action_buttons_reset_001f72c0(&buttons);C(gh_game_event_from_elf(&event,elf,elfn)==GH_EVENT_OK);
 work.room=0x20;work.phase569=2;work.flags=0x80000000;gh_game_init_state_initialize(init);init->camera.mode=4;b=(gh_game_scene_step_bindings){.work=&work,.init=init,.prepare=&prepare,.collision=&collision,.actors=&actors,.scenario={scalar,sizeof(scalar),&scenario},.event=&event,.scene_at=&at,.action_buttons=&buttons,.cockpit_action_flags_1414=&action_flag,.profile_counter=counter};
 /* No supplied world/render callbacks: trueprefix reaches the nextcamera
  * frontier; this is not a successful frame or an acknowledgedemptyrenderer. */
 C(gh_game_scene_step_002a6690(&step,&b)==GH_SCENE_STEP_CALLEE&&step.frontier==0x138600);C(step.cooldown_1b0==14&&init->world_time==1&&prepare.global_0061a994==1&&get((uint8_t*)&work+0x610)==0xffff&&action_flag==254&&!step.completed_frames);
 C(gh_game_scene_step_002a6690(&step,&b)==GH_SCENE_STEP_CALLEE&&step.frontier==0x138600&&step.cooldown_1b0==14&&init->world_time==1); /* noreplay */
 /* Actual R020 Init's 5D8=F00C6000 gates world clock, enemy and world light.
  * This domain must not require a fabricated player/save or mutate enemies. */
 init->camera.mode=1;put((uint8_t*)&work+0x5d8,0xf00c6000);init->enemies[0x538]=17;
 C(gh_game_scene_step_002a6690(&step,&b)==GH_SCENE_STEP_UNSUPPORTED&&step.frontier==0x30f090&&init->enemies[0x538]==17);
 C(init->world_time==1&&step.cooldown_1b0==14&&!step.completed_frames);
 gh_game_scene_step_construct(&step,0,15);put(collision.records.pointer,0x200);put((uint8_t*)collision.records.pointer+GH_COLLISION_RECORD_BYTES,0x400);
 C(gh_game_scene_step_002a6690(&step,&b)==GH_SCENE_STEP_UNSUPPORTED&&step.frontier==0x2bac98&&step.collision_cursor==1&&step.detail==1);C(get(collision.records.pointer)==0x600);C(gh_game_scene_step_002a6690(&step,&b)==GH_SCENE_STEP_UNSUPPORTED&&step.frontier==0x2bac98&&step.detail==1&&step.collision_cursor==1&&get(collision.records.pointer)==0x600); /* same pausedframe must not revisit firstrow as a new sweep */
 C(gh_collision_pairs_release(&collision)==GH_COLLISION_OK&&gh_heap_destroy(heap)==GH_MEM_OK);free(init);free(data);free(elf);test_fibers(0);test_fibers(1);test_fibers(2);
 puts("PASS actualR020 state4 orderedprefix to30F090, realELFevent/AT/action emptydomains, camera mode4 frontier, independentMainNearCheck, noreplay failureprefix andScenario16 nativefibers +parentSleep/revisit");return 0;
}
