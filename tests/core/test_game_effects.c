#include "game_effects.h"
#include "../platform/disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d: %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t u32(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static void put(uint8_t*p,uint32_t n){memcpy(p,&n,4);}
static uint8_t*read_file(gh_disc*d,const char*p,size_t*n){size_t i;gh_disc_entry e;CHECK(gh_disc_find(d,p,&i)==GH_DISC_OK&&gh_disc_entry_at(d,i,&e)==GH_DISC_OK);*n=(size_t)e.size;uint8_t*b=malloc(*n);CHECK(b&&gh_disc_read(d,i,0,b,*n)==GH_DISC_OK);return b;}
int main(int argc,char**argv){gh_disc*d;size_t cz,rz;CHECK(argc==2&&gh_disc_open(argv[1],&d)==GH_DISC_OK);uint8_t*core=read_file(d,"etc/core.dat",&cz),*room=read_file(d,"st0/r020.dat",&rz);gh_bundle r;CHECK(gh_bundle_open(room,rz,&r)==GH_BUNDLE_OK);
 uint8_t*a=calloc(1,0x100180),*b=calloc(1,0x26060);gh_game_effects*s=calloc(1,sizeof(*s));CHECK(a&&b&&s);
 for(unsigned i=0;i<1000;i++){s->vectors[i][0]=99;s->vectors[i][1]=88;s->vectors[i][2]=77;s->vectors[i][3]=0x7fc00000u+i;}
 for(unsigned i=0;i<32;i++)s->nodes[i].unknown14=0xa5a50000u+i;s->head.unknown14=0x12345678;
 CHECK(gh_game_effects_initialize_00305a68(s,a,0x100180,b,0x26060,core,cz,&r)==GH_EFFECT_OK&&s->ready&&!s->active);
 const int ids[4]={0,0x58,4,1};for(unsigned i=0;i<4;i++)CHECK(s->resources[i].kind==ids[i]&&s->resources[i].references==1&&s->resources[i].tbl0.bytes&&s->resources[i].tbl1.bytes);
 for(unsigned i=4;i<64;i++)CHECK(s->resources[i].kind==5&&!s->resources[i].references);
 CHECK(s->resources[0].tbl0.bytes==s->resources[2].tbl0.bytes&&s->resources[0].tbl1.bytes==s->resources[2].tbl1.bytes);
 CHECK(!s->ssd.bytes&&s->ear.bytes&&s->cached408==0xffffffffu&&s->cached40c==0xffffffffu&&!s->field414);
 CHECK(s->emd_tables[0].bytes==core+0x1b4260&&s->emd_tables[4].bytes==s->emd_tables[0].bytes&&s->emd_tables[0x58].bytes==core+0x296870);
 CHECK(s->emitter_entries[0].bytes==core+0x1b42c0&&s->emitter_entries[0].size>0);
 CHECK(u32(s->emd_tables[0].bytes+4)==18&&u32(s->emd_tables[1].bytes+4)==1);
 for(unsigned i=0;i<1000;i++)CHECK(!s->vectors[i][0]&&!s->vectors[i][1]&&!s->vectors[i][2]&&s->vectors[i][3]==0x7fc00000u+i);
 CHECK(s->nodes[0].in_use&&s->nodes[0].units==1000&&s->nodes[0].previous==32&&s->nodes[0].next==-1&&s->head.next==0);
 for(unsigned i=0;i<32;i++)CHECK(s->nodes[i].unknown14==0xa5a50000u+i);CHECK(s->head.unknown14==0x12345678);
 /* OriginalMSBfirst slotgates: live31 mustnotbe silentlykilled; paused/hidden
  * live slots remainuntouched and are genuinelyskipped byoriginalAllKill. */
 put(b+0x26000,1);CHECK(gh_game_effects_initialize_00305a68(s,a,0x100180,b,0x26060,core,cz,&r)==GH_EFFECT_UNSUPPORTED&&s->frontier==0x302918&&s->detail==31&&!s->ready);CHECK(u32(b+0x26000)==1);
 put(b+0x26020,1);put(a+0x100000,0x80000000u);CHECK(gh_game_effects_initialize_00305a68(s,a,0x100180,b,0x26060,core,cz,&r)==GH_EFFECT_UNSUPPORTED&&s->frontier==0x2fbe50&&s->detail==0);
 put(a+0x100100,0x80000000u);CHECK(gh_game_effects_initialize_00305a68(s,a,0x100180,b,0x26060,core,cz,&r)==GH_EFFECT_OK&&u32(a+0x100000)==0x80000000u&&u32(b+0x26000)==1);
 CHECK(gh_game_effects_initialize_00305a68(s,a,0x10017f,b,0x26060,core,cz,&r)==GH_EFFECT_ARGUMENT);
 free(s);free(a);free(b);free(core);free(room);gh_disc_close(d);puts("PASS actualCore/R020 effects bindings/EMD/pool layout and realMSBfirst killgates");return 0;}
