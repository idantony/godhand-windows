#include "game_math_globals.h"
#include <string.h>
static uint32_t u32(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static unsigned u16(const uint8_t*p){return p[0]|(unsigned)p[1]<<8;}
static int mapped(const uint8_t*elf,size_t bytes,uint32_t table,unsigned stride,unsigned count,uint32_t va,uint8_t*out,size_t n){
 for(unsigned i=0;i<count;i++){const uint8_t*p=elf+table+(size_t)i*stride;uint32_t off=u32(p+4),base=u32(p+8),files=u32(p+16),mem=u32(p+20);if(u32(p)!=1||va<base||(uint64_t)va+n>(uint64_t)base+mem)continue;if(files>mem||off>bytes||files>bytes-off)return 0;size_t delta=va-base;memset(out,0,n);if(delta<files){size_t copy=files-delta;if(copy>n)copy=n;memcpy(out,elf+off+delta,copy);}return 1;}return 0;
}
gh_math_globals_status gh_game_math_globals_from_elf(gh_game_math_globals*s,const void*raw,size_t bytes){
 if(!s||!raw)return GH_MATH_GLOBALS_ARGUMENT;const uint8_t*elf=raw;if(bytes<52)return GH_MATH_GLOBALS_BOUNDS;if(memcmp(elf,"\177ELF\1\1\1",7)||u16(elf+18)!=8)return GH_MATH_GLOBALS_FORMAT;
 uint32_t table=u32(elf+28);unsigned stride=u16(elf+42),count=u16(elf+44);if(stride<32||count>128||table>bytes||count>(bytes-table)/stride)return GH_MATH_GLOBALS_BOUNDS;
 static const uint32_t words[]={0x3402ffff,0x14a2001a,0,0x10800018,0x3c020074,0x244274a0,0xf84005c0,0x3c030074,0x3c02003c,0x24637ac0,0x24423c40,0xd8440000,0xd8450010,0xd8460020,0xd8470030,0xf8640000,0xf8650010,0xf8660020,0xf8670030,0x44800000,0x3c030074,0x3c013f80,0x44810800,0x24627b00,0xe4607b00,0xe441000c,0xe4400008,0xe4400004,0x03e00008,0};
 for(unsigned i=0;i<sizeof(words)/sizeof(*words);i++){uint8_t value[4];if(!mapped(elf,bytes,table,stride,count,0x3079c8+i*4,value,4))return GH_MATH_GLOBALS_BOUNDS;if(u32(value)!=words[i])return GH_MATH_GLOBALS_FORMAT;}
 gh_game_math_globals n={0};if(!mapped(elf,bytes,table,stride,count,0x747ac0,(uint8_t*)n.identity,64)||!mapped(elf,bytes,table,stride,count,0x747b00,(uint8_t*)n.zero_vector,16)||!mapped(elf,bytes,table,stride,count,0x3c3c40,(uint8_t*)n.source_identity,64))return GH_MATH_GLOBALS_BOUNDS;n.loaded=1;*s=n;return GH_MATH_GLOBALS_OK;
}
gh_math_globals_status gh_game_math_static_initialize_003079c8(gh_game_math_globals*s,gh_boot_work*w,int initialize,int priority){
 if(!s||!s->loaded)return GH_MATH_GLOBALS_ARGUMENT;if(!initialize||priority!=0xffff)return GH_MATH_GLOBALS_OK;if(!w)return GH_MATH_GLOBALS_ARGUMENT;
 memset((uint8_t*)w+0x5c0,0,16);memcpy(s->identity,s->source_identity,64);const uint32_t zero[]={0,0,0,0x3f800000};memcpy(s->zero_vector,zero,16);s->initialized=1;return GH_MATH_GLOBALS_OK;
}
