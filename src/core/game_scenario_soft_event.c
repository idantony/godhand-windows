#include "game_scenario_soft_event.h"
#include <string.h>
static uint16_t h(const uint8_t*p){return (uint16_t)(p[0]|(unsigned)p[1]<<8);}
static uint32_t w(const uint8_t*p){return (uint32_t)h(p)|(uint32_t)h(p+2)<<16;}
static uint64_t dw(const uint8_t*p){return w(p)|(uint64_t)w(p+4)<<32;}
static void wh(uint8_t*p,uint16_t v){p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
static void ww(uint8_t*p,uint32_t v){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(v>>(i*8));}
static void wd(uint8_t*p,uint64_t v){for(unsigned i=0;i<8;i++)p[i]=(uint8_t)(v>>(i*8));}
gh_soft_event_status gh_scenario_stop_input_0031afa0(gh_boot_work*s){
 if(!s)return GH_SOFT_EVENT_ARGUMENT;uint8_t*r=(uint8_t*)s,*p=r+0x178;uint64_t mask=UINT64_C(0x000f00807ff00000);ww(r+0x5d8,w(r+0x5d8)|0x2000000);
 wd(p+64,dw(p+64)&mask);wd(p+56,dw(p+56)&mask);wd(p,dw(p+24)&mask);wd(p+40,dw(p+40)&mask);wd(p+48,dw(p+48)&mask);
 p[0x6b]=0x80;p[0x7b]=0;p[0x68]=p[0x69]=p[0x6a]=0x80;p[0x6f]=p[0x6e]=p[0x6c]=p[0x6d]=0;memset(p+0x70,0,12);return GH_SOFT_EVENT_OK;
}
static gh_soft_event_status stop(gh_soft_event_start*s,uint32_t pc,gh_soft_event_status e){s->frontier=pc;return e;}
gh_soft_event_status gh_scenario_start_soft_event_002c39b0(gh_soft_event_start*s,const gh_soft_event_bindings*b,uint8_t kind){
 if(!s||!b||!b->work||!b->scenario.bytes||b->scenario.size<0xea||!b->scenario.manager||!b->scenario.manager->records||b->scenario.manager->count>b->scenario.manager->capacity)return GH_SOFT_EVENT_ARGUMENT;
 if(kind!=4)return stop(s,0x2c39b0,GH_SOFT_EVENT_UNSUPPORTED);
 if(s->stage&&(s->scenario_bytes!=b->scenario.bytes||s->manager!=b->scenario.manager||s->work!=b->work||s->kind!=kind))return GH_SOFT_EVENT_ARGUMENT;
 uint8_t*p=b->scenario.bytes,*raw=(uint8_t*)b->work;gh_task_manager*m=b->scenario.manager;
 if(!s->stage){s->scenario_bytes=p;s->manager=m;s->work=b->work;s->kind=kind;uint16_t depth=(uint16_t)(h(p+0xe8)+1);wh(p+0xe8,depth);int32_t signed_depth=depth<=32767?depth:(int32_t)depth-65536;if(signed_depth>=2){s->frontier=0;return GH_SOFT_EVENT_OK;}ww(p+0xe0,kind);s->stage=1;}
 while(s->stage<6){switch(s->stage){
 case 1:ww(raw+0x5e4,w(raw+0x5e4)|0x40000040);if(gh_scenario_stop_input_0031afa0(b->work))return GH_SOFT_EVENT_ARGUMENT;break;
 case 2:ww(raw+0x5d8,w(raw+0x5d8)|0x500000);ww(raw+0x5e0,w(raw+0x5e0)|0x2100000);break;
 case 3:if(!b->player_abort_00126770)return stop(s,0x126770,GH_SOFT_EVENT_UNSUPPORTED);{uint32_t f=0;gh_soft_event_status e=b->player_abort_00126770(b->context,&f);if(e)return stop(s,f?f:0x126770,e);}break;
 case 4:ww(raw+0x5e4,w(raw+0x5e4)|0x80000);ww(raw+0x5d8,w(raw+0x5d8)|0xf4000000);break;
 case 5:if(m->current_index>=0){if((uint32_t)m->current_index>=m->count||m->current!=&m->records[m->current_index])return stop(s,0x2c39b0,GH_SOFT_EVENT_ARGUMENT);m->current->attributes|=1;}m->flags|=1;break;
 default:break;}s->stage++;}
 s->stage=0;s->frontier=0;return GH_SOFT_EVENT_OK;
}
