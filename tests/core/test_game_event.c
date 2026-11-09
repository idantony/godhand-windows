#include "../../src/core/game_event.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x)do{if(!(x)){fprintf(stderr,"FAIL%d %s\n",__LINE__,#x);exit(1);}}while(0)
typedef struct {unsigned stage,*count,*order;int done,error;} fixture;
static int fixture_stage(void *context,gh_game_event *event,int *done)
{fixture *f=context;f->order[(*f->count)++]=f->stage;++event->fields.phase;event->fields.state=99;*done=f->done;return f->error;}
int main(int argc,char **argv)
{
    CHECK(argc==2);FILE *file=fopen(argv[1],"rb");CHECK(file&&!fseek(file,0,SEEK_END));long length=ftell(file);CHECK(length>0);rewind(file);
    uint8_t *elf=malloc((size_t)length);CHECK(elf&&fread(elf,1,(size_t)length,file)==(size_t)length&&fclose(file)==0);
    gh_game_event event={0};CHECK(gh_game_event_from_elf(&event,elf,20)==GH_EVENT_BOUNDS);
    CHECK(gh_game_event_from_elf(&event,elf,(size_t)length)==GH_EVENT_OK);free(elf);
    CHECK(event.initialized&&!event.fields.flags&&!event.fields.state&&!event.fields.resource_token);
    gh_game_event_prefix before=event.fields;
    CHECK(gh_game_event_update_00296530(&event)==GH_EVENT_OK&&!memcmp(&event.fields,&before,sizeof(before)));
    static const uint32_t targets[]={0x2965f0,0x296818,0x296958,0x296c28,0x297038};
    for(unsigned i=0;i<5;++i){event.fields=before;event.fields.flags=1;event.fields.state=(uint8_t)i;
        CHECK(gh_game_event_update_00296530(&event)==GH_EVENT_UNSUPPORTED&&event.frontier==targets[i]&&!event.fields.phase);}
    event.fields.flags=9;CHECK(gh_game_event_update_00296530(&event)==GH_EVENT_UNSUPPORTED&&event.frontier==0x296dd8);
    unsigned count=0,order[8]={0};fixture stages[6];
    for(unsigned i=0;i<6;++i){stages[i]=(fixture){i,&count,order,1,0};CHECK(gh_game_event_set_stage(&event,i,fixture_stage,&stages[i])==GH_EVENT_OK);}
    event.fields=before;event.fields.flags=1;
    CHECK(gh_game_event_update_00296530(&event)==GH_EVENT_OK&&count==5&&event.fields.phase==5&&event.fields.state==99);
    for(unsigned i=0;i<5;++i)CHECK(order[i]==i);
    count=0;event.fields=before;event.fields.flags=1;stages[2].done=0;
    CHECK(gh_game_event_update_00296530(&event)==GH_EVENT_OK&&count==3&&event.fields.phase==3);
    count=0;event.fields=before;event.fields.flags=1;stages[1].error=73;
    CHECK(gh_game_event_update_00296530(&event)==GH_EVENT_CALLEE&&count==2&&event.callee_error==73&&event.frontier==0x296818);
    count=0;event.fields=before;event.fields.flags=9;
    CHECK(gh_game_event_update_00296530(&event)==GH_EVENT_OK&&count==1&&order[0]==5);
    count=0;event.fields.flags=1;event.fields.state=255;
    CHECK(gh_game_event_update_00296530(&event)==GH_EVENT_OK&&!count);
    puts("PASS actual ELF/BSS event prefix, inactive branch, move-step priority, ordered fallthrough and explicit missing-stage frontiers; fixture callbacks are not cutscene execution");return 0;
}
