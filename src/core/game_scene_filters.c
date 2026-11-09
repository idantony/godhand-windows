/* MIT LICENSE.upstream; actualELF evidence game-scene-filters-evidence.json. */
#include "game_scene_filters.h"
#include <string.h>
#include <stddef.h>
_Static_assert(offsetof(gh_scene_filter_work32,active140c8)==0x30,"originalactiveoffset");
_Static_assert(offsetof(gh_scene_filter_work32,presets143a0)==0x308,"originalFI2offset");
_Static_assert(sizeof(gh_scene_filter_work32)==0x5490,"originalscalarspan");
static void h(uint8_t*p,uint16_t x){memcpy(p,&x,2);}
static void w(uint8_t*p,uint32_t x){memcpy(p,&x,4);}
gh_filter_status gh_scene_filter_select_002a2818(gh_scene_filters*s,unsigned n,const uint8_t**selected){
 static const unsigned src[6]={8,0x6c,0xd0,0x134,0x198,0x1fc};
 static const unsigned dst[6]={0x78,0xdc,0x140,0x1a4,0x274,0x14};
 if(selected)*selected=0;
 if(!s||n>5)return GH_FILTER_ARGUMENT;
 const uint8_t*p=s->work.presets143a0+0x10+(n==5?0:n+1)*GH_FILTER_PRESET_BYTES;
 if(selected)*selected=p;
 uint8_t*d=s->work.active140c8;
 memcpy(d,p,8);
 for(unsigned i=0;i<6;i++)memcpy(d+dst[i],p+src[i],100);
 d[0x26c]=p[0x268];d[0x10]=p[0x26a];d[0x26d]=p[0x269];
 memcpy(d+8,p+0x260,4);memcpy(d+12,p+0x264,4);
 memcpy(d+0x26e,p+0x26c,2);memcpy(d+0x270,p+0x26e,2);memcpy(d+0x272,p+0x270,2);
 return GH_FILTER_OK;
}
gh_filter_status gh_scene_filters_initialize_002993a0(gh_scene_filters*s,const gh_bundle*b){
 if(!s||!b)return GH_FILTER_ARGUMENT;s->ready=0;s->frontier=0;
 s->work.transition140a8=s->work.transition140ac=s->work.transition140b0=s->work.transition140a4=0;
 gh_bundle_entry e;gh_bundle_error be=gh_bundle_find(b,"FI2",0,&e);
 if(be==GH_BUNDLE_NOT_FOUND){s->frontier=0x2993a0;return GH_FILTER_UNSUPPORTED;}
 if(be!=GH_BUNDLE_OK||e.size<GH_FILTER_FILE_BYTES){s->frontier=0x31ccf0;return GH_FILTER_BOUNDS;}
 memcpy(s->work.presets143a0,e.bytes,GH_FILTER_FILE_BYTES);
 (void)gh_scene_filter_select_002a2818(s,0,0);
 /* Exactpresentbranch overrides, addresses relative originalactive140C8.
  * These are packedGScolors/scalars, never callableguestaddresses despite
  * Ghidra sometimes formatting 30303030 as an address literal. */
 uint8_t*d=s->work.active140c8;
 h(d+0x262,0xff);w(d+0x20c,0x10101010);w(d+0x210,0x30303030);
 w(d+0x214,0x40404040);w(d+0x218,0xa0a0a0a0);
 h(d+0x22e,0x50);h(d+0x230,0x6e);h(d+0x20a,4);
 w(d+0x248,0x80808080);h(d+0x25e,0x14);h(d+0x260,200);
 h(d+0x208,4);h(d+0x22c,0);h(d+0x232,0xff);
 w(d+0x23c,0x80808080);w(d+0x240,0x80808080);w(d+0x244,0x80808080);h(d+0x25c,0);
 s->ready=1;return GH_FILTER_OK;
}
