#include "game_scene_filters.h"
#include "disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d: %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t u32(const uint8_t*p){uint32_t x;memcpy(&x,p,4);return x;}
int main(int argc,char**argv){gh_disc*d;CHECK(argc==2&&gh_disc_open(argv[1],&d)==GH_DISC_OK);size_t idx;gh_disc_entry de;CHECK(gh_disc_find(d,"st0/r020.dat",&idx)==GH_DISC_OK&&gh_disc_entry_at(d,idx,&de)==GH_DISC_OK);uint8_t*p=malloc((size_t)de.size);CHECK(p&&gh_disc_read(d,idx,0,p,(size_t)de.size)==GH_DISC_OK);gh_bundle b;gh_bundle_entry fi;CHECK(gh_bundle_open(p,(size_t)de.size,&b)==GH_BUNDLE_OK&&gh_bundle_find(&b,"FI2",0,&fi)==GH_BUNDLE_OK);
 gh_scene_filters s;memset(&s,0xa5,sizeof(s));CHECK(gh_scene_filters_initialize_002993a0(&s,&b)==GH_FILTER_OK&&s.ready);
 CHECK(!memcmp(s.work.presets143a0,fi.bytes,GH_FILTER_FILE_BYTES));CHECK(s.work.flags14098==UINT64_C(0xa5a5a5a5a5a5a5a5)&&s.work.unknown140a0==0xa5a5a5a5);
 CHECK(!s.work.transition140a4&&!s.work.transition140a8&&!s.work.transition140ac&&!s.work.transition140b0);
 CHECK(u32(s.work.active140c8+0x20c)==0x10101010&&u32(s.work.active140c8+0x210)==0x30303030&&u32(s.work.active140c8+0x218)==0xa0a0a0a0);
 /* Allsixliteralchoice mappings, nonoverlappingblock copies and holes. */
 for(unsigned choice=0;choice<6;choice++){
  memset(s.work.active140c8,0x5a,sizeof(s.work.active140c8));CHECK(gh_scene_filter_select_002a2818(&s,choice,0)==GH_FILTER_OK);
  const uint8_t*q=fi.bytes+0x10+(choice==5?0:choice+1)*0x278;
  CHECK(!memcmp(s.work.active140c8,q,8));CHECK(!memcmp(s.work.active140c8+0x78,q+8,100));CHECK(!memcmp(s.work.active140c8+0x274,q+0x198,100));CHECK(!memcmp(s.work.active140c8+0x14,q+0x1fc,100));
  CHECK(s.work.active140c8[0x11]==0x5a&&s.work.active140c8[0x264]==0x5a&&s.work.active140c8[0x26b]==0x5a);
 }
 CHECK(gh_scene_filter_select_002a2818(&s,6,0)==GH_FILTER_ARGUMENT);
 for(unsigned i=0;i<b.count;i++){uint8_t*tag=p+4+b.count*4+i*4;if(!memcmp(tag,"FI2",4)){memcpy(tag,"ZZZ",4);break;}}
 CHECK(gh_scene_filters_initialize_002993a0(&s,&b)==GH_FILTER_UNSUPPORTED&&!s.ready&&s.frontier==0x2993a0);
 free(p);gh_disc_close(d);puts("PASS actualR020 FI2 exactpresetcopy/selection/overrides and preservedholes");return 0;}
