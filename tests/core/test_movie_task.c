#include "../../src/core/movie_task.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL%d %s\n",__LINE__,#x);exit(1);}}while(0)
typedef struct {unsigned statuses,sleeps,stops,registrations;int fail_status,fail_second;} trace;
static int start(void*c,int32_t id,int32_t arg,int32_t*h){trace*t=c;C(!t->statuses&&!t->sleeps&&!t->registrations);C(id==0&&arg==1);*h=17;return 0;}
static int status(void*c,int32_t h,int32_t*s){trace*t=c;C(h==17);if(t->fail_status)return 73;*s=t->statuses<3?3-(int)t->statuses:0;t->statuses++;return 0;}
static int sleep_one(void*c,int32_t n){trace*t=c;C(n==1&&t->statuses==t->sleeps+1);t->sleeps++;return 0;}
static int stop(void*c,int32_t h){trace*t=c;C(h==17&&t->statuses==4&&t->sleeps==3);t->stops++;return 0;}
static int reg(void*c,uint32_t entry,int32_t slot){trace*t=c;C(t->stops==1);if(!t->registrations)C(entry==0x2a5998&&slot==0);else{C(t->registrations==1&&entry==0x30d5c8&&slot==3);if(t->fail_second)return 81;}t->registrations++;return 0;}
int main(void){trace t={0};gh_movie_task task={0};gh_movie_platform p={&t,start,status,sleep_one,stop,reg};C(gh_movie_task_0030da08(&task,&p)==0);C(task.body_returned&&task.operation==GH_MOVIE_FINISHED&&task.sleeps==3&&t.registrations==2);
 memset(&t,0,sizeof(t));t.fail_status=1;C(gh_movie_task_0030da08(&task,&p)==73);C(task.operation==GH_MOVIE_STATUS&&!task.body_returned&&!t.stops&&!t.registrations);
 memset(&t,0,sizeof(t));t.fail_second=1;C(gh_movie_task_0030da08(&task,&p)==81);C(task.operation==GH_MOVIE_REGISTER_INIT&&!task.body_returned&&t.stops==1&&t.registrations==1);
 p.register_task=NULL;C(gh_movie_task_0030da08(&task,&p)==-1);puts("PASS original30DA08 Movie0(0,1), all nonzero status classes sleep1, Stop, ordered successors; failed status/secondregistration retain concrete prefix");return 0;}
