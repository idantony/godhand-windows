#include "movie_task.h"
int gh_movie_task_0030da08(gh_movie_task*t,const gh_movie_platform*p){
 int32_t status;if(!t||!p||!p->start||!p->status||!p->sleep||!p->stop||!p->register_task)return -1;
 t->error=0;t->body_returned=0;t->sleeps=0;t->operation=GH_MOVIE_START;
 if((t->error=p->start(p->context,0,1,&t->handle))!=0)return t->error;
 for(;;){t->operation=GH_MOVIE_STATUS;if((t->error=p->status(p->context,t->handle,&status))!=0)return t->error;if(!status)break;t->operation=GH_MOVIE_SLEEP;if((t->error=p->sleep(p->context,1))!=0)return t->error;t->sleeps++;}
 t->operation=GH_MOVIE_STOP;if((t->error=p->stop(p->context,t->handle))!=0)return t->error;
 t->operation=GH_MOVIE_REGISTER_MAIN;if((t->error=p->register_task(p->context,0x2a5998,0))!=0)return t->error;
 t->operation=GH_MOVIE_REGISTER_INIT;if((t->error=p->register_task(p->context,0x30d5c8,3))!=0)return t->error;
 t->operation=GH_MOVIE_FINISHED;t->body_returned=1;return 0;
}
