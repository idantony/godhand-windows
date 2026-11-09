#include "game_world_light.h"
#include "disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d: %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t u32(const uint8_t*p){uint32_t x;memcpy(&x,p,4);return x;}
static void put(uint8_t*p,uint32_t n){memcpy(p,&n,4);}
int main(int argc,char**argv){gh_disc*d;CHECK(argc==2&&gh_disc_open(argv[1],&d)==GH_DISC_OK);size_t idx;gh_disc_entry de;CHECK(gh_disc_find(d,"st0/r020.dat",&idx)==GH_DISC_OK&&gh_disc_entry_at(d,idx,&de)==GH_DISC_OK);uint8_t*r=malloc((size_t)de.size);CHECK(r&&gh_disc_read(d,idx,0,r,(size_t)de.size)==GH_DISC_OK);gh_bundle b;gh_bundle_entry li;CHECK(gh_bundle_open(r,(size_t)de.size,&b)==GH_BUNDLE_OK&&gh_bundle_find(&b,"LI3",0,&li)==GH_BUNDLE_OK);
 gh_game_world_light*s=calloc(1,sizeof(*s));CHECK(s&&gh_game_world_light_construct_002d6d80(s)==GH_LIGHT_OK);uint8_t*p=s->scalar;
 CHECK(gh_game_world_light_construct_002d6d80(s)==GH_LIGHT_ARGUMENT);
 CHECK(u32(p+0x3c)==0x3f800000&&u32(p+0x90+255*0x70+12)==0x3f800000&&u32(p+0xe8a0+0x3c)==0x3f800000);
 CHECK(!u32(p)&&!u32(p+0x16128));
 /* LI33loadedbranch leavesactivelightingunchangeduntilsubsequentloadPreset. */
 p[4]=0;p[5]=128;p[6]=255;put(p+0x10,0x7fc12345);put(p+0x14,0x80000000);p[0x16179]=0x5a;
 CHECK(gh_game_world_light_initialize_002d6f40(s,&b)==GH_LIGHT_OK&&s->ready);
 CHECK(!memcmp(p+0x15cf0,"LI33",4)&&!s->area.bytes&&!s->table.bytes);
 for(unsigned i=0;i<8;i++){CHECK(!memcmp(p+0x15d00+i*0x80,li.bytes+4+i*0x178,0x70));CHECK(!u32(p+0x15d70+i*0x80)&&!u32(p+0x15d74+i*0x80));CHECK(s->slots[i].bytes==li.bytes+4+i*0x178+0x70&&s->slots[i].size==0x104);CHECK(p[0x16171+i]==i);}
 CHECK(p[0x16168]==255&&p[0x16179]==0x5a&&!u32(p+0x16284));
 CHECK(u32(p+0x161a0)==0xbf000000&&u32(p+0x161b4)==0x3f000000&&u32(p+0x161c4)==0xbf000000&&u32(p+0x161f0)==0x3f4ccccd);
 CHECK(u32(p+0x16150)==0x7fc12345&&u32(p+0x16154)==0x80000000&&u32(p+0x16158)==0&&u32(p+0x1615c)==0x43000000&&u32(p+0x16160)==0x437f0000);
 CHECK(u32(p+0x16144)==255&&u32(p+0x16148)==255&&u32(p+0x1614c)==255);
 put(r+li.offset+4+0x174,1);CHECK(gh_game_world_light_initialize_002d6f40(s,&b)==GH_LIGHT_UNSUPPORTED&&!s->ready&&s->frontier==0x30ee08&&s->detail==0);
 free(s);free(r);gh_disc_close(d);puts("PASS actualR020 LI33 eightpresets/slots/coldctor/fog/directions and preciseallocationfrontier");return 0;}
