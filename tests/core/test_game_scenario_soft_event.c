#include "game_scenario_soft_event.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t w(const uint8_t*p){uint32_t v;memcpy(&v,p,4);return v;}
static uint64_t dw(const uint8_t*p){uint64_t v;memcpy(&v,p,8);return v;}
typedef struct {unsigned calls;uint32_t*word;} action;
/* Fixture for the separate actual playerabort boundary, not a runtimeprovider. */
static gh_soft_event_status abort_fixture(void*p,uint32_t*f){action*a=p;a->calls++;*a->word=0;*f=0;return GH_SOFT_EVENT_OK;}
int main(void){uint8_t scenario[0x114];memset(scenario,0xa5,sizeof(scenario));scenario[0xe8]=scenario[0xe9]=0;gh_boot_work work;memset(&work,0x35,sizeof(work));uint8_t before[sizeof(work)];memcpy(before,&work,sizeof(work));gh_task_record32 records[16]={0};gh_task_manager m={.records=records,.capacity=16,.count=16,.current_index=2,.current=&records[2]};records[2].attributes=2;uint32_t borrowed_action=123;action a={0,&borrowed_action};gh_soft_event_bindings b={.scenario={scenario,sizeof(scenario),&m},.work=&work};gh_soft_event_start s={0};
 C(gh_scenario_start_soft_event_002c39b0(&s,&b,4)==GH_SOFT_EVENT_UNSUPPORTED&&s.frontier==0x126770&&s.stage==3&&scenario[0xe8]==1&&w(scenario+0xe0)==4&&records[2].attributes==2&&!m.flags);
 uint8_t*p=(uint8_t*)&work;uint64_t mask=UINT64_C(0x000f00807ff00000);C(dw(p+0x178)==(dw(before+0x178+24)&mask));for(unsigned i=0;i<sizeof(work);i++){int changed=(i>=0x178&&i<0x180)||(i>=0x1a0&&i<0x1c0)||(i>=0x1e0&&i<0x1f4)||(i>=0x5d8&&i<0x5dc)||(i>=0x5e0&&i<0x5e8);if(!changed)C(p[i]==before[i]);}C(p[0x1e0]==0x80&&p[0x1e3]==0x80&&!p[0x1f3]);C(w(p+0x5d8)==(0x35353535u|0x2500000)&&w(p+0x5e4)==(0x35353535u|0x40000040));
 /* Resume MUSTNOT repeatdepth/inputprefix. */
 p[0x178]=0x7b;b.player_abort_00126770=abort_fixture;b.context=&a;C(!gh_scenario_start_soft_event_002c39b0(&s,&b,4)&&!s.stage&&!s.frontier&&scenario[0xe8]==1&&p[0x178]==0x7b&&a.calls==1&&!borrowed_action&&records[2].attributes==3&&(m.flags&1));C(w(p+0x5d8)==(0x35353535u|0xf6500000)&&w(p+0x5e4)==(0x35353535u|0x40080040));
 b.player_abort_00126770=NULL;C(!gh_scenario_start_soft_event_002c39b0(&s,&b,4)&&scenario[0xe8]==2&&a.calls==1);C(gh_scenario_start_soft_event_002c39b0(&s,&b,5)==GH_SOFT_EVENT_UNSUPPORTED&&scenario[0xe8]==2);puts("PASS actualkind4 nesteddepth/inputwidths/flags/attr gates, honestplayer126770 frontier and samecursorresume withoutreplayedprefix");return 0;}
