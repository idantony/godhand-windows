#include "game_gate.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static void put(gh_boot_work *w,size_t off,uint32_t v){memcpy((uint8_t *)w+off,&v,4);}
static uint32_t get(gh_boot_work *w,size_t off){uint32_t v;memcpy(&v,(uint8_t *)w+off,4);return v;}
static unsigned trace[20],used;static gh_game_gate gate;static int completed;static gh_game_status result;
static int call(void *ctx,gh_game_gate *g,gh_game_gate_operation op){
 (void)ctx;(void)g;CHECK(used<20);trace[used++]=(unsigned)op;return 0;
}
static void modal(void *ctx,int32_t arg){(void)ctx;(void)arg;result=gh_game_gate_002a6da8(&gate);completed=1;}
int main(int argc,char **argv){
 gh_game_disc_gate ds={0};gh_game_disc_inputs in={1,1,1,1};
 gh_boot_work work={0};uint8_t state=0;gh_disc *disc;gh_dvd_jobs dvd;
 gh_task_record32 records[3]={0};gh_task_manager manager={records,3,0,-1,3,0,0,0};
 gh_task_threads *threads;int32_t slot;
 CHECK(gh_game_disc_gate_step(&ds,in)==1 && ds.timer==90);
 in.changed=0;in.queue_idle=0;CHECK(gh_game_disc_gate_step(&ds,in)==1 && ds.timer==90);
 in.queue_idle=1;in.media_ready=0;CHECK(gh_game_disc_gate_step(&ds,in)==1 && ds.timer==90);
 in.media_ready=1;in.identity_matches=0;CHECK(gh_game_disc_gate_step(&ds,in)==1 && ds.timer==90);
 in.identity_matches=1;for(unsigned i=0;i<90;i++)CHECK(gh_game_disc_gate_step(&ds,in)==1);
 CHECK(ds.timer==0 && gh_game_disc_gate_step(&ds,in)==0);
 CHECK(argc==2 && gh_disc_open(argv[1],&disc)==GH_DISC_OK);gh_dvd_jobs_init(&dvd,disc);
 gate.work=&work;gate.game_state=&state;gate.dvd=&dvd;
 CHECK(gh_game_gate_002a6ca8(&gate)==GH_GAME_OK && used==0);
 /* Queue occupancy alone must not manufacture a disc-change cover. */
 dvd.current=0;CHECK(gh_game_gate_002a6ca8(&gate)==GH_GAME_OK);dvd.current=-1;
 gate.disc_gate.state=1;gate.disc_gate.timer=90;
 CHECK(gh_game_gate_002a6ca8(&gate)==GH_GAME_UNAVAILABLE && gate.failed_entry==0x15b880);
 gate.disc_gate.state=0;dvd.last_error=GH_DVD_IO;
 CHECK(gh_game_gate_002a6ca8(&gate)==GH_GAME_DISC && gate.failed_entry==0x201960);dvd.last_error=GH_DVD_OK;
 CHECK(gh_game_gate_002a6da8(&gate)==GH_GAME_OK && used==0);state=4;
 CHECK(gh_game_gate_002a6da8(&gate)==GH_GAME_UNAVAILABLE && gate.failed_entry==0x13f238);
 work.flags=8;CHECK(gh_game_gate_002a6da8(&gate)==GH_GAME_OK);work.flags=0;
 for(unsigned i=0;i<4;i++){uint32_t masks[]={0x8000000,0x20000000,0x40000000,0x2000000};
  put(&work,0x5e4,masks[i]);CHECK(gh_game_gate_002a6da8(&gate)==GH_GAME_OK);}
 put(&work,0x5e4,0);put(&work,0x5e8,0x40000000);
 CHECK(gh_game_gate_002a6da8(&gate)==GH_GAME_OK);put(&work,0x5e8,0);
 work.room=0x20;CHECK(gh_game_gate_002a6da8(&gate)==GH_GAME_OK);work.room=0;
 ((uint8_t *)&work)[0x14]=1;CHECK(gh_game_gate_002a6da8(&gate)==GH_GAME_OK);((uint8_t *)&work)[0x14]=0;
 CHECK(gh_task_threads_create(&manager,&threads)==GH_THREAD_OK);gate.threads=threads;gate.call=call;
 /* Test-only message trace adapter, no real renderer/ID UI claimed. */
 work.language=1;put(&work,0x5e0,0x1234);put(&work,8,0);used=0;
 CHECK(gh_task_threads_start(threads,2,modal,0,0,&slot)==GH_THREAD_OK);
 CHECK(gh_tasks_run_matching_002d52a0(&manager).status==GH_TASK_OK && !completed);
 CHECK(used==4 && trace[0]==GH_GATE_MESSAGE_BEGIN && trace[1]==GH_GATE_MESSAGE_UPDATE && trace[2]==GH_GATE_MESSAGE_DRAW && trace[3]==GH_GATE_COVER_QUAD);
 CHECK(get(&work,0x5e0)==(0x1234|0x2020000u));
 put(&work,8,0x40);CHECK(gh_tasks_run_matching_002d52a0(&manager).status==GH_TASK_OK && !completed);
 CHECK(used==5 && trace[4]==GH_GATE_MESSAGE_FINISH && get(&work,0x5e0)==0x1234);
 CHECK(gh_tasks_run_matching_002d52a0(&manager).status==GH_TASK_OK && completed && result==GH_GAME_OK);
 /* Japanese confirmation uses20 and can skip the inner drawing loop. */
 completed=0;used=0;work.language=0;put(&work,8,0x20);
 CHECK(gh_task_threads_start(threads,2,modal,0,0,&slot)==GH_THREAD_OK);
 CHECK(gh_tasks_run_matching_002d52a0(&manager).status==GH_TASK_OK && !completed);
 CHECK(used==2 && trace[0]==GH_GATE_MESSAGE_BEGIN && trace[1]==GH_GATE_MESSAGE_FINISH);
 CHECK(gh_tasks_run_matching_002d52a0(&manager).status==GH_TASK_OK && completed);
 CHECK(gh_task_threads_destroy(threads)==GH_THREAD_OK);gh_disc_close(disc);
 puts("PASS original DVD cover countdown, actual ISO no-cover, modal Work flag branches and real fiber input waits");return 0;
}
