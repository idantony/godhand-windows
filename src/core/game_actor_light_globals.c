#include "game_actor_light_globals.h"
#include <string.h>
static uint32_t word(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static unsigned half(const uint8_t*p){return p[0]|(unsigned)p[1]<<8;}
gh_actor_model_status gh_actor_light_globals_from_elf(gh_actor_light_globals*s,const void*raw,size_t n){
 if(!s||!raw)return GH_ACTOR_MODEL_ARGUMENT;if(s->initialized)return GH_ACTOR_MODEL_BUSY;
 const uint8_t*b=raw;if(n<52||memcmp(b,"\177ELF\1\1\1",7)||half(b+18)!=8)return GH_ACTOR_MODEL_BOUNDS;
 size_t ph=word(b+28);unsigned stride=half(b+42),count=half(b+44);
 if(stride<32||!count||count>128||ph>n||(size_t)count*stride>n-ph)return GH_ACTOR_MODEL_BOUNDS;
 uint8_t values[3][16];const uint32_t addresses[3]={0x3bd870,0x3bd890,0x3bd860};unsigned found[3]={0};
 for(unsigned i=0;i<count;i++){
  const uint8_t*p=b+ph+(size_t)i*stride;if(word(p)!=1)continue;
  uint32_t offset=word(p+4),base=word(p+8),files=word(p+16),memory=word(p+20);
  if(files>memory||offset>n||files>n-offset||(uint64_t)base+memory>UINT64_C(0x100000000))return GH_ACTOR_MODEL_BOUNDS;
  for(unsigned k=0;k<3;k++)if(base<=addresses[k]&&(uint64_t)addresses[k]+16<=(uint64_t)base+memory){
   if(found[k]++)return GH_ACTOR_MODEL_BOUNDS;
   size_t delta=addresses[k]-base;
   for(unsigned j=0;j<16;j++)values[k][j]=delta+j<files?b[(size_t)offset+delta+j]:0;
  }
 }
 if(found[0]!=1||found[1]!=1||found[2]!=1)return GH_ACTOR_MODEL_BOUNDS;
 memcpy(s->bytes_3bd870,values[0],16);memcpy(s->bytes_3bd890,values[1],16);
 memcpy(s->bytes_3bd860,values[2],16);s->initialized=1;return GH_ACTOR_MODEL_OK;
}
