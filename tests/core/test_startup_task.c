#include "startup_task.h"
#include <stdlib.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);exit(1); } } while(0)
#include <string.h>
#include <stdio.h>
typedef struct { int reads,waits,regs,fail; const char *path; int32_t job;
    uint32_t entries[2];int32_t slots[2]; unsigned calls; } rec;
static int op(rec *r,int n){r->calls++;return r->fail==n?77:0;}
static int sound(void *v){return op(v,GH_START_SOUND);}
static int read_data(void *v,const char *path,void **dst,int32_t *job){
 rec *r=v;r->reads++;r->path=path;*dst=r;*job=123+r->reads;return op(r,GH_START_READ);}
static int wait_job(void *v,int32_t job){rec *r=v;r->waits++;r->job=job;return op(r,GH_START_WAIT);}
static int font(void *v,gh_startup_task *s){CHECK(s->core_data);return op(v,GH_START_FONT);}
static int menu(void *v,gh_startup_task *s){(void)s;return op(v,GH_START_MENU);}
static int bind(void *v,gh_startup_task *s){(void)s;return op(v,GH_START_BIND_ID);}
static int find(void *v,void *data,const char *name,int32_t i,void **out){
 (void)data;CHECK(!strcmp(name,"IDD") && i==0);*out=v;return op(v,GH_START_FIND_ID);}
static int init(void *v,void *data){CHECK(data==v);return op(v,GH_START_INIT_ID);}
static int reg(void *v,uint32_t entry,int32_t slot){rec *r=v;CHECK(r->regs<2);
 r->entries[r->regs]=entry;r->slots[r->regs++]=slot;return op(v,GH_START_REGISTER);}
int main(void){
 const char *names[]={"etc/core_jpn.bin","etc/core_usa.bin","etc/core_eng.bin",
 "etc/core_fra.bin","etc/core_ger.bin","etc/core_ita.bin","etc/core_spa.bin"};
 for(int language=-1;language<=7;language++) for(int sign=0;sign<2;sign++) {
  gh_boot_work work={0};rec r={0};r.fail=-1;
  gh_startup_task s={&work,0,0,0,0};
  gh_startup_platform p={&r,sound,read_data,wait_job,font,menu,bind,find,init,reg};
  work.language=language;work.flags=1;work.flags598=sign?0x80000000u:0;
  CHECK(gh_startup_task_0030d358(&s,&p)==0);
  CHECK(work.flags==0xc4001 && r.waits==2);
  CHECK(r.reads==((unsigned)language<7?2:1));
  CHECK(!strcmp(r.path,(unsigned)language<7?names[language]:"etc/core.bin"));
  CHECK(r.job==123+r.reads && r.regs==(sign?2:1));
  CHECK(r.entries[0]==(sign?0x2a5998u:0x2ae8a8u));
  CHECK(r.slots[0]==(sign?0:2));
  if(sign) CHECK(r.entries[1]==0x30d5c8 && r.slots[1]==2);
 }
 for(int n=0;n<GH_START_OP_COUNT;n++) {
  gh_boot_work work={0};rec r={0};r.fail=n;
  gh_startup_task s={&work,0,0,0,0};
  gh_startup_platform p={&r,sound,read_data,wait_job,font,menu,bind,find,init,reg};
  CHECK(gh_startup_task_0030d358(&s,&p)==77);
  CHECK(s.failed_operation==(unsigned)n && s.error==77);
 }
 {gh_boot_work work={0};gh_startup_task s={&work,0,0,0,0};
 gh_startup_platform p={0};CHECK(gh_startup_task_0030d358(&s,&p)!=0);
 CHECK(work.flags==0);}
 puts("PASS startup-task original language/sign branches, double wait default and host failure boundaries");
 return 0;
}
