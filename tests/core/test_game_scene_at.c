#include "game_scene_at.h"
#include "disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
int main(int argc,char**argv){gh_disc*d;CHECK(argc==2&&gh_disc_open(argv[1],&d)==GH_DISC_OK);size_t idx;gh_disc_entry de;CHECK(gh_disc_find(d,"st0/r020.dat",&idx)==GH_DISC_OK&&gh_disc_entry_at(d,idx,&de)==GH_DISC_OK);uint8_t*r=malloc((size_t)de.size);CHECK(r&&gh_disc_read(d,idx,0,r,(size_t)de.size)==GH_DISC_OK);gh_bundle b;CHECK(gh_bundle_open(r,(size_t)de.size,&b)==GH_BUNDLE_OK);gh_scene_at_manager s={0};s.field60=s.field68=s.field6c=s.field70=99;s.field64=7;
 CHECK(gh_scene_at_initialize_002c0f90(&s,&b)==GH_AT_OK&&s.ready&&!s.frontier);
 const size_t offsets[]={0x6260,0x6360,0x60a0,0x60e0};for(unsigned i=0;i<4;i++)CHECK(s.headers[i].bytes==r+offsets[i]);
 CHECK(s.units[0].bytes==r+0x6270&&!s.units[0].count&&!s.units[1].bytes&&!s.units[2].bytes&&!s.units[3].bytes);
 CHECK(s.heads[0].kind==GH_AT_LINK_END);for(unsigned i=1;i<16;i++)CHECK(s.heads[i].kind==GH_AT_LINK_SLOT&&s.heads[i].index==i-1);
 for(unsigned i=0;i<8;i++)CHECK(!s.jump_slots[i].bytes);CHECK(!s.field60&&!s.field64&&!s.field68&&!s.field6c&&!s.field70);
 /* Originalinvalidversionh2doesNOToverwrite anearlieru2 view. */
 uint8_t foreign[16]={0};s.units[2]=(gh_at_unit_view){foreign,sizeof(foreign),0};CHECK(gh_scene_at_initialize_002c0f90(&s,&b)==GH_AT_OK&&s.units[2].bytes==foreign);
 r[0x6260+6]=1;CHECK(gh_scene_at_initialize_002c0f90(&s,&b)==GH_AT_UNSUPPORTED&&!s.ready&&s.frontier==0x2c3040&&s.detail==0);r[0x6260+6]=0;
 gh_bundle_entry jump;CHECK(gh_bundle_find(&b,"JMP",0,&jump)==GH_BUNDLE_OK);((uint8_t*)jump.bytes)[0]=1;CHECK(gh_scene_at_initialize_002c0f90(&s,&b)==GH_AT_UNSUPPORTED&&s.frontier==0x2c12c8&&!s.ready);
 free(r);gh_disc_close(d);puts("PASS actualR020 sceneAt headers/emptyunits/taggedslotchain/versiongates/jumps/frontiers");return 0;}
