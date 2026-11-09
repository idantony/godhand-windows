#include "game_scene_spawns.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static void h(uint8_t*p,uint16_t x){p[0]=(uint8_t)x;p[1]=(uint8_t)(x>>8);}
int main(void){uint32_t frontier=99;gh_saved_item_spawn request;CHECK(gh_core_save_spawn_items_001fb330((gh_core_save_view){0},0x20,&request,&frontier)==GH_SPAWN_OK&&!frontier);CHECK(gh_core_save_spawn_items_001fb330((gh_core_save_view){0},0x120,&request,&frontier)==GH_SPAWN_OK);
 uint8_t*save=calloc(1,GH_CORE_SAVE_BYTES);CHECK(save);for(unsigned i=0;i<128;i++)h(save+0x28a+i*16,0xffff);
 gh_core_save_view v={save,GH_CORE_SAVE_BYTES};CHECK(gh_core_save_spawn_items_001fb330(v,0x120,&request,&frontier)==GH_SPAWN_OK);
 uint8_t*p=save+0x28a+127*16;h(p,0x120);h(p+2,0x777);p[4]=0xa5;p[5]=0xff;h(p+6,0x1234);h(p+8,0xffff);h(p+10,200);h(p+12,0x8000);h(p+14,0x9876);
 /* itemNum remains0: actualfunctionstillscans all128slots. */
 CHECK(gh_core_save_spawn_items_001fb330(v,0x120,&request,&frontier)==GH_SPAWN_UNSUPPORTED&&frontier==0x30fa98&&request.index==127&&request.kind==0x777&&request.serial==0x1234&&request.extra==0x9876&&request.flags==0xa5&&request.yaw_half_degrees==-1);
 CHECK(request.position_tenths[0]==-1&&request.position_tenths[1]==200&&request.position_tenths[2]==-32768);
 h(save+0x28a+5*16,0x120);CHECK(gh_core_save_spawn_items_001fb330(v,0x120,&request,&frontier)==GH_SPAWN_UNSUPPORTED&&request.index==5);
 CHECK(gh_core_save_spawn_items_001fb330((gh_core_save_view){save,1},0x120,&request,&frontier)==GH_SPAWN_BOUNDS);
 CHECK(gh_core_save_spawn_items_001fb330((gh_core_save_view){save,1},0x20,&request,&frontier)==GH_SPAWN_OK);
 free(save);puts("PASS originalroomhighbytegate/128slotfilter/signeditemdata/exactCreateObjfrontier");return 0;}
