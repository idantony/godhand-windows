/* MIT LICENSE.upstream. ActualELF evidence game-world-light-evidence.json. */
#include "game_world_light.h"
#include <string.h>
static uint32_t u32(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static void w(uint8_t*p,uint32_t x){memcpy(p,&x,4);}
static void vf0(uint8_t*p){memset(p,0,12);w(p+12,0x3f800000);}
static gh_light_status fail(gh_game_world_light*s,uint32_t a,int d,gh_light_status e){s->frontier=a;s->detail=d;return e;}
gh_light_status gh_game_world_light_construct_002d6d80(gh_game_world_light*s){
 if(!s||s->constructed)return GH_LIGHT_ARGUMENT;
 memset(s,0,sizeof(*s));uint8_t*p=s->scalar;
 for(unsigned j=0;j<3;j++)vf0(p+0x30+j*16);
 for(unsigned i=0;i<256;i++)for(unsigned j=0;j<3;j++)vf0(p+0x80+i*0x70+0x10+j*16);
 for(unsigned j=0;j<3;j++)vf0(p+0x7480+j*16);
 for(unsigned i=0;i<256;i++)for(unsigned j=0;j<3;j++)vf0(p+0x74d0+i*0x70+0x10+j*16);
 for(unsigned j=0;j<3;j++)vf0(p+0xe8a0+0x30+j*16);
 for(unsigned i=0;i<256;i++)for(unsigned j=0;j<3;j++)vf0(p+0xe8a0+0x80+i*0x70+0x10+j*16);
 for(unsigned i=0;i<8;i++)for(unsigned j=0;j<3;j++)vf0(p+0x15d00+i*0x80+j*16);
 for(unsigned i=0;i<3;i++){vf0(p+0x161a0+i*16);vf0(p+0x161d0+i*16);}
 for(unsigned i=0;i<4;i++)vf0(p+0x16200+i*16);
 s->constructed=1;return GH_LIGHT_OK;
}
gh_light_status gh_game_world_light_initialize_with_heap_002d6f40(gh_game_world_light*s,const gh_bundle*b,gh_heap*heap){
 if(!s||!s->constructed||!b)return GH_LIGHT_ARGUMENT;
 gh_light_status cleanup=gh_game_world_light_release_presets(s);if(cleanup)return cleanup;
 gh_bundle_entry li,e;gh_bundle_error be;uint8_t*p=s->scalar;s->record_heap=heap;s->ready=0;s->frontier=0;s->detail=0;
 w(p+0x16194,0);p[0x1619c]=0;
 be=gh_bundle_find(b,"FAR",0,&e);if(be==GH_BUNDLE_OK)s->area=e;else if(be!=GH_BUNDLE_NOT_FOUND)return fail(s,0x31ccf0,be,GH_LIGHT_BOUNDS);
 be=gh_bundle_find(b,"LAR",0,&e);if(be==GH_BUNDLE_OK)s->table=e;else if(be!=GH_BUNDLE_NOT_FOUND)return fail(s,0x31ccf0,be,GH_LIGHT_BOUNDS);
 for(unsigned i=0;i<8;i++){s->slots[i]=(gh_bundle_entry){0};w(p+0x16100+i*4,0);w(p+0x15d74+i*0x80,0);}
 be=gh_bundle_find(b,"LI3",0,&li);
 if(be==GH_BUNDLE_NOT_FOUND)return fail(s,0x2d6a60,0,GH_LIGHT_UNSUPPORTED);
 if(be!=GH_BUNDLE_OK||li.size<4)return fail(s,0x2d6a60,be,GH_LIGHT_BOUNDS);
 memcpy(p+0x15cf0,li.bytes,4);
 if(memcmp(li.bytes,"LI33",4))return fail(s,0x2d6ae4,0,GH_LIGHT_UNSUPPORTED);
 size_t off=4;
 for(unsigned i=0;i<8;i++){
  if(off>li.size||0x178>li.size-off)return fail(s,0x2d6a60,(int)i,GH_LIGHT_BOUNDS);
  const uint8_t*q=li.bytes+off;uint8_t*d=p+0x15d00+i*0x80;
  memcpy(d,q,0x70);s->slots[i]=(gh_bundle_entry){.bytes=q+0x70,.offset=li.offset+off+0x70,.size=0x104};
  uint32_t count=u32(q+0x174);w(d+0x70,count);
  if(count){
   if(count>256||off+0x178>li.size||(size_t)count*0x70>li.size-off-0x178)return fail(s,0x2d6a60,(int)i,GH_LIGHT_BOUNDS);
   if(!heap)return fail(s,0x30ee08,(int)i,GH_LIGHT_UNSUPPORTED);
   gh_mem_error me=gh_heap_allocate(heap,(size_t)count*0x70,16,s->preset_records+i);
   if(me)return fail(s,0x30ee08,me,GH_LIGHT_MEMORY);
   s->preset_record_capacity[i]=count;memcpy(s->preset_records[i].pointer,q+0x178,(size_t)count*0x70);
  }
  off+=0x178+(size_t)count*0x70;
 }
 p[0x16168]=0xff;w(p+0x16284,0);
 for(unsigned i=0;i<8;i++)p[0x16171+i]=(uint8_t)i;
 static const uint32_t dir[3][4]={{0xbf000000,0xbf000000,0xbf000000,0},{0x3f000000,0x3f000000,0x3f000000,0},{0x3f000000,0xbf000000,0x3f000000,0}};
 static const uint32_t col[3][4]={{0x3f800000,0x3f800000,0x3f800000,0x3f800000},{0x3f000000,0x3f000000,0x3f000000,0x3f800000},{0x3f4ccccd,0x3f4ccccd,0x3f4ccccd,0x3f800000}};
 memcpy(p+0x161a0,dir,sizeof(dir));memcpy(p+0x161d0,col,sizeof(col));
 memcpy(p+0x16150,p+0x10,4);memcpy(p+0x16154,p+0x14,4);
 /* Bytecolors0..255 convertexactly, independentofhostFEnv. */
 for(unsigned i=0;i<3;i++){float color=(float)p[4+i];memcpy(p+0x16158+4*i,&color,4);}
 w(p+0x16128,0);w(p+0x1614c,0xff);w(p+0x16148,0xff);w(p+0x16144,0xff);
 s->ready=1;return GH_LIGHT_OK;
}

gh_light_status gh_game_world_light_initialize_002d6f40(gh_game_world_light*s,const gh_bundle*b){return gh_game_world_light_initialize_with_heap_002d6f40(s,b,NULL);}
gh_light_status gh_game_world_light_release_presets(gh_game_world_light*s){
 if(!s||!s->constructed)return GH_LIGHT_ARGUMENT;
 for(unsigned i=0;i<8;i++)if(s->preset_records[i].pointer){
  if(!s->record_heap)return fail(s,0x30ee08,0,GH_LIGHT_ARGUMENT);
  gh_mem_error e=gh_heap_release(s->record_heap,s->preset_records[i]);if(e)return fail(s,0x30ee08,e,GH_LIGHT_MEMORY);
  s->preset_records[i]=(gh_r207_allocation){0};s->preset_record_capacity[i]=0;
 }
 s->record_heap=NULL;s->ready=0;return GH_LIGHT_OK;
}
gh_light_status gh_game_world_light_preset_records(const gh_game_world_light*s,unsigned index,const uint8_t**out,size_t*count){
 if(!s||!s->constructed||!out||!count)return GH_LIGHT_ARGUMENT;if(index>=8)return GH_LIGHT_BOUNDS;
 size_t n=u32(s->scalar+0x15d70+index*0x80);
 if(n>256||n>s->preset_record_capacity[index]||(n&&!s->preset_records[index].pointer))return GH_LIGHT_BOUNDS;
 *out=s->preset_records[index].pointer;*count=n;return GH_LIGHT_OK;
}
