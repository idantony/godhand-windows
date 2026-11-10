/* ADX format/recurrence cross-checked against vgmstream (ISC), pinned in
 * analysis/adx-evidence.json. See adx-vgmstream-LICENSE.txt for attribution.
 * Platform replacement, not recovered readable CRI SDK source. */
#include "adx.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
struct gh_adx_decoder {
 const uint8_t *bytes;size_t size;gh_adx_info info;gh_adx_key key;
 uint32_t position,loaded_block;uint16_t xor_value;
 int32_t history[2][2];int16_t block[32][2];
};
static uint16_t be16(const uint8_t*p){return (uint16_t)((uint16_t)p[0]*256+p[1]);}
static uint32_t be32(const uint8_t*p){return (uint32_t)be16(p)*65536+be16(p+2);}
static uint16_t le16(const uint8_t*p){return (uint16_t)((uint16_t)p[1]*256+p[0]);}
static uint32_t le32(const uint8_t*p){return (uint32_t)le16(p+2)*65536+le16(p);}
static const uint8_t *span(const uint8_t*b,size_t n,uint32_t va,size_t bytes){
 size_t i;uint32_t off;uint16_t count,stride;
 if(n<52||memcmp(b,"\177ELF\1\1",6)||le16(b+18)!=8)return NULL;
 off=le32(b+28);stride=le16(b+42);count=le16(b+44);
 if(stride<32||off>n||(size_t)count>(n-off)/stride)return NULL;
 for(i=0;i<count;i++){const uint8_t*p=b+off+i*stride;uint32_t start=le32(p+8),sz=le32(p+16),file=le32(p+4);uint64_t delta;
  if(le32(p)!=1||va<start)continue;delta=(uint64_t)va-start;
  if(delta>sz||bytes>sz-delta||file>n||delta>n-file||bytes>n-file-(size_t)delta)continue;
  return b+file+(size_t)delta;
 }return NULL;
}
gh_adx_error gh_adx_key_from_elf(const void*data,size_t size,gh_adx_key*out){
 const uint8_t*b=data,*call,*key,*table;uint16_t result[3];size_t i,j;
 if(!b||!out)return GH_ADX_ARGUMENT;
 call=span(b,size,0x2ca34c,12);key=span(b,size,0x44ce20,128);table=span(b,size,0x456080,2048);
 if(!call||!key||!table)return GH_ADX_BOUNDS;
 if(le32(call)!=0x3c040045||le32(call+4)!=0x0c0caebc||le32(call+8)!=0x2484ce20)return GH_ADX_UNSUPPORTED;
 for(i=0;i<1024;i++){uint16_t p=le16(table+i*2);unsigned d;
  if(p<0x4000||(i&&p<=le16(table+(i-1)*2)))return GH_ADX_FORMAT;
  for(d=2;d*d<=p;d++)if(p%d==0)return GH_ADX_FORMAT;
 }
 for(j=0;j<3;j++)result[j]=le16(table+(j+1)*256*2);
 for(i=0;i<128&&key[i];i++){
  if(key[i]<32||key[i]>127)return GH_ADX_UNSUPPORTED;
  for(j=0;j<3;j++){uint32_t product=(uint32_t)result[j]*le16(table+(key[i]+128)*2);result[j]=le16(table+(product%1024)*2);}
 }
 if(!i||i==128)return GH_ADX_FORMAT;
 out->start=result[0];out->mult=result[1];out->add=result[2];return GH_ADX_OK;
}
static uint16_t next_key(uint16_t x,gh_adx_key k){return (uint16_t)(((uint32_t)x*k.mult+k.add)&0x7fff);}
static int32_t shift12(int64_t n){return n>=0?(int32_t)(n/4096):-(int32_t)((-n+4095)/4096);}
static void reset(gh_adx_decoder*d){unsigned c;d->position=0;d->loaded_block=UINT32_MAX;d->xor_value=d->key.start;for(c=0;c<2;c++){d->history[c][0]=d->info.initial_history[c][0];d->history[c][1]=d->info.initial_history[c][1];}}
gh_adx_error gh_adx_open(const void*data,size_t size,const gh_adx_key*key,gh_adx_decoder**out){
 const uint8_t*b=data;gh_adx_decoder*d;uint32_t offset,samples;size_t blocks,i;uint16_t x;unsigned valid=0,c;int compact;
 if(!b||!key||!out)return GH_ADX_ARGUMENT;*out=NULL;
 if(size<40||be16(b)!=0x8000)return GH_ADX_FORMAT;
 offset=(uint32_t)be16(b+2)+4;compact=offset==0x28;
 if(!compact&&size<64)return GH_ADX_FORMAT;
 if(b[4]!=3||b[5]!=18||b[6]!=4||b[7]!=2||be32(b+8)!=48000||be16(b+16)!=500||be16(b+18)!=0x0408)return GH_ADX_UNSUPPORTED;
 samples=be32(b+12);
 if(!samples||samples>INT32_MAX||(!compact&&offset<62)||offset>size||memcmp(b+offset-6,"(c)CRI",6))return GH_ADX_FORMAT;
 blocks=((size_t)samples+31)/32;
 if(blocks>(size-offset)/36)return GH_ADX_BOUNDS;
 if(key->start>0x7fff||key->mult>0x7fff||key->add>0x7fff||!(key->mult&1)||!(key->add&1))return GH_ADX_KEY;
 x=key->start;
 for(i=0;i<blocks*2;i++){const uint8_t*f=b+offset+i*18;unsigned nonzero=0,j;for(j=0;j<18;j++)nonzero|=f[j];
  if(nonzero){if((be16(f)&0xe000)!=(x&0x6000))return GH_ADX_KEY;valid++;}x=next_key(x,*key);
 }
 if(!valid)return GH_ADX_KEY;
 /* Actual 33EA98 history ends at 0x20. 33EBD0 rejects loop metadata
  * for the verified 0x28-byte header; its caller leaves the loop flag zero. */
 if(!compact){
  if(be32(b+36)>1)return GH_ADX_UNSUPPORTED;
  if(be32(b+36)&&(be32(b+40)>=be32(b+48)||be32(b+48)>samples))return GH_ADX_FORMAT;
  if(be32(b+36)&&((uint64_t)offset+((uint64_t)be32(b+40)+31)/32*36!=be32(b+44)||
   (uint64_t)offset+((uint64_t)be32(b+48)+31)/32*36!=be32(b+52)))return GH_ADX_FORMAT;
 }
 d=calloc(1,sizeof(*d));if(!d)return GH_ADX_MEMORY;
 d->bytes=b;d->size=size;d->key=*key;d->info.sample_rate=48000;d->info.channels=2;d->info.sample_count=samples;d->info.data_offset=offset;
 if(!compact){d->info.loop_enabled=(uint8_t)be32(b+36);d->info.loop_start=be32(b+40);d->info.loop_end=be32(b+48);}
 /* Same documented ADX high-pass coefficients; truncate toward zero. */
 {float z=cosf((float)(6.28318530717958647692*500.0/48000.0));float a=(float)(1.4142135623730951-z),v=(float)(1.4142135623730951-1.0);float t=(a-sqrtf((a+v)*(a-v)))/v;
 d->info.coefficient[0]=(int16_t)(t*8192);d->info.coefficient[1]=(int16_t)(t*t*-4096);}
 for(c=0;c<2;c++){d->info.initial_history[c][0]=(int16_t)be16(b+24+c*4);d->info.initial_history[c][1]=(int16_t)be16(b+26+c*4);}
 reset(d);*out=d;return GH_ADX_OK;
}
void gh_adx_close(gh_adx_decoder*d){free(d);}
gh_adx_error gh_adx_inspect(const gh_adx_decoder*d,gh_adx_info*out){if(!d||!out)return GH_ADX_ARGUMENT;*out=d->info;return GH_ADX_OK;}
uint32_t gh_adx_tell(const gh_adx_decoder*d){return d?d->position:0;}
static void load_block(gh_adx_decoder*d,uint32_t block){unsigned c,i;
 for(c=0;c<2;c++){const uint8_t*f=d->bytes+d->info.data_offset+(size_t)block*36+c*18;int32_t scale=((be16(f)^d->xor_value)&8191)+1;
  for(i=0;i<32;i++){int32_t nibble=(i&1)?f[2+i/2]&15:f[2+i/2]>>4;int32_t sample;if(nibble>=8)nibble-=16;
   sample=nibble*scale+shift12((int64_t)d->info.coefficient[0]*d->history[c][0]+(int64_t)d->info.coefficient[1]*d->history[c][1]);
   if(sample>32767)sample=32767;if(sample<-32768)sample=-32768;
   d->block[i][c]=(int16_t)sample;d->history[c][1]=d->history[c][0];d->history[c][0]=sample;
  }d->xor_value=next_key(d->xor_value,d->key);
 }d->loaded_block=block;
}
gh_adx_error gh_adx_decode(gh_adx_decoder*d,int16_t*out,size_t capacity,size_t*frames){size_t done=0;if(!d||!frames||(!out&&capacity)||capacity>SIZE_MAX/(2*sizeof(*out)))return GH_ADX_ARGUMENT;*frames=0;
 while(done<capacity&&d->position<d->info.sample_count){uint32_t block=d->position/32;size_t take=32-d->position%32;if(d->loaded_block!=block)load_block(d,block);
  if(take>capacity-done)take=capacity-done;if(take>d->info.sample_count-d->position)take=d->info.sample_count-d->position;
  memcpy(out+done*2,d->block[d->position%32],take*2*sizeof(*out));done+=take;d->position+=(uint32_t)take;
 }*frames=done;return GH_ADX_OK;}
gh_adx_error gh_adx_seek(gh_adx_decoder*d,uint32_t sample){int16_t discard[2048];size_t n;if(!d)return GH_ADX_ARGUMENT;if(sample>d->info.sample_count)return GH_ADX_BOUNDS;reset(d);while(d->position<sample){size_t take=sample-d->position;if(take>1024)take=1024;gh_adx_decode(d,discard,take,&n);}return GH_ADX_OK;}
const char*gh_adx_error_string(gh_adx_error e){static const char*s[]={"ok","argument","format","bounds","unsupported","key","memory","device","state"};return (unsigned)e<sizeof(s)/sizeof(*s)?s[e]:"unknown";}
