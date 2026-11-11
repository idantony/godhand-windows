#include "display_state.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
static uint32_t u32(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static unsigned u16(const uint8_t*p){return p[0]|(unsigned)p[1]<<8;}
static int32_t signed32(uint32_t n){int32_t result;memcpy(&result,&n,4);return result;}
static float number(uint32_t n){float result;memcpy(&result,&n,4);return result;}
static uint32_t bits(float n){uint32_t result;memcpy(&result,&n,4);return result;}
static int mapped(const uint8_t*elf,size_t bytes,uint32_t table,unsigned stride,unsigned count,uint32_t address,uint32_t*out){
 for(unsigned i=0;i<count;++i){const uint8_t*p=elf+table+(size_t)i*stride;
  uint32_t offset=u32(p+4),base=u32(p+8),files=u32(p+16),memory=u32(p+20);
  if(u32(p)!=1||address<base||(uint64_t)address+4>(uint64_t)base+memory)continue;
  if(files>memory||offset>bytes||files>bytes-offset)return 0;
  uint32_t delta=address-base;uint8_t data[4]={0};
  if(delta<files){size_t n=files-delta;if(n>4)n=4;memcpy(data,elf+offset+delta,n);}
  *out=u32(data);return 1;
 }return 0;
}
gh_display_status gh_display_state_from_elf(gh_display_state*s,const void*raw,size_t bytes){
 const uint8_t*elf=raw;gh_display_state n={0};uint32_t words[8],table;unsigned stride,count;
 static const uint32_t addresses[]={0x3c1180,0x3c118c,0x3c1190,0x586b80,0x586b84,0x586b88,0x59ac98,0x59acc8};
 if(!s||!elf)return GH_DISPLAY_ARGUMENT;if(bytes<52)return GH_DISPLAY_BOUNDS;
 if(memcmp(elf,"\177ELF\1\1\1",7)||u16(elf+18)!=8)return GH_DISPLAY_FORMAT;
 table=u32(elf+28);stride=u16(elf+42);count=u16(elf+44);
 if(stride<32||count>128||table>bytes||count>(bytes-table)/stride)return GH_DISPLAY_BOUNDS;
 for(unsigned i=0;i<8;++i)if(!mapped(elf,bytes,table,stride,count,addresses[i],&words[i]))return GH_DISPLAY_BOUNDS;
 n.screen_width=signed32(words[0]);n.screen_lines=signed32(words[1]);n.screen_height_bits=words[2];
 n.gs_y_origin=signed32(words[3]);n.gs_y_origin_bits=words[4];n.pixel_aspect_bits=words[5];
 n.font_batch_count[0]=words[6];n.font_batch_count[1]=words[7];n.loaded=1;*s=n;return GH_DISPLAY_OK;
}
gh_display_status gh_display_static_initialize_002a48a8(gh_display_state*s,int initialize,int priority){
 fenv_t env;if(!s||!s->loaded)return GH_DISPLAY_ARGUMENT;
 if(!initialize||priority!=0xffff)return GH_DISPLAY_OK;
 float height=number(s->screen_height_bits);
 if(s->screen_lines<=0||s->screen_lines>4096||!isfinite(height)||height<=0)return GH_DISPLAY_NUMERIC;
 if(feholdexcept(&env))return GH_DISPLAY_NUMERIC;
 if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return GH_DISPLAY_NUMERIC;}
 volatile float lines=(float)s->screen_lines;
 volatile float aspect=lines/480.0f;
 volatile float half=height*0.5f;
 volatile float origin=2048.0f-half;
 uint32_t a=bits(aspect),o=bits(origin);
 if(fesetenv(&env))return GH_DISPLAY_NUMERIC;
 s->pixel_aspect_bits=a;s->gs_y_origin_bits=o;s->gs_y_origin=2048-s->screen_lines/2;
 s->font_batch_count[0]=s->font_batch_count[1]=0;s->initialized=1;return GH_DISPLAY_OK;
}
uint32_t gh_display_copy_base0_0030b870(const uint8_t top_rgb[3],const uint8_t next_rgb[3]){
 uint32_t rgb=0;
 for(unsigned c=0;c<3;c++){
  const uint32_t channel=(top_rgb[c]>>1)+(next_rgb[c]>>2);
  rgb|=channel<<(8*c);
 }
 return rgb;
}
gh_display_status gh_display_copy_base0_surface_0030b870(const uint8_t *source,size_t source_bytes,
 uint8_t *target,size_t target_bytes)
{
 enum {row_bytes=512*4,rows=448,bytes=row_bytes*rows};
 if(!source||!target||source==target)return GH_DISPLAY_ARGUMENT;
 if(source_bytes!=bytes||target_bytes!=bytes)return GH_DISPLAY_BOUNDS;
 static const uint8_t outside[3]={0};
 for(unsigned y=0;y<rows;++y){
  const uint8_t *line=source+y*row_bytes;
  for(unsigned x=0;x<row_bytes;x+=4){
   const uint8_t *above=y?line+x-row_bytes:outside;
   const uint8_t *below=y+1<rows?line+x+row_bytes:outside;
   const uint32_t rgb=gh_display_copy_base0_0030b870(line+x,below);
   uint8_t *pixel=target+y*row_bytes+x;
   for(unsigned channel=0;channel<3;++channel)
    pixel[channel]=(uint8_t)(((rgb>>(channel*8))&0xff)+(above[channel]>>2));
   pixel[3]=0;
  }
 }
 return GH_DISPLAY_OK;
}
