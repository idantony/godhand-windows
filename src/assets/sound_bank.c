#include "sound_bank.h"
#include <string.h>
static uint32_t le(const uint8_t *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|
 (uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static uint32_t be(const uint8_t *p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|
 (uint32_t)p[2]<<8|p[3];}
static int range(size_t size,uint32_t off,uint32_t len){return off<=size && len<=size-off;}
gh_sound_status gh_sound_bank_open(const void *bytes,size_t size,gh_sound_bank *out){
 const uint8_t *b=bytes;gh_sound_bank bank={0};uint32_t path;
 if(!bytes || !out)return GH_SOUND_ARGUMENT;
 if(size<0x40)return GH_SOUND_BOUNDS;
 if(memcmp(b,"SES",3) || le(b+4)!=0x3f8ccccd)return GH_SOUND_FORMAT;
 if(b[0xc])return GH_SOUND_UNSUPPORTED;
 if(!le(b+0x10))return GH_SOUND_FORMAT;
 bank.image.data=b;bank.image.size=size;
 for(unsigned i=0;i<4;i++){
  uint32_t off=le(b+0x10+8*i),len=le(b+0x14+8*i);
  if(!range(size,off,len))return GH_SOUND_BOUNDS;
  if(!off && len)return GH_SOUND_FORMAT;
  bank.sections[i].data=off?b+off:0;bank.sections[i].size=len;
 }
 path=le(b+0x30);
 if(path){if(path>=size || !memchr(b+path,0,size-path))return GH_SOUND_BOUNDS;
 bank.external_afs=(const char *)b+path;}
 *out=bank;return GH_SOUND_OK;
}
gh_sound_status gh_sound_afs_entry(const void *bytes,size_t size,uint32_t index,gh_sound_span *out){
 const uint8_t *b=bytes;uint32_t count,off,len;
 if(!b || !out)return GH_SOUND_ARGUMENT;
 if(size<8)return GH_SOUND_BOUNDS;
 if(memcmp(b,"AFS\0",4))return GH_SOUND_FORMAT;
 count=le(b+4);if(count>(size-8)/8)return GH_SOUND_BOUNDS;
 if(index>=count)return GH_SOUND_BOUNDS;
 off=le(b+8+(size_t)index*8);len=le(b+12+(size_t)index*8);
 if(off<8+(size_t)count*8 || !range(size,off,len))return GH_SOUND_BOUNDS;
 out->data=b+off;out->size=len;return GH_SOUND_OK;
}
gh_sound_status gh_sound_adx_header(gh_sound_span s,gh_adx_header *out){
 gh_adx_header h;const uint8_t *b=s.data;
 if(!b || !out)return GH_SOUND_ARGUMENT;
 if(s.size<20)return GH_SOUND_BOUNDS;
 if(b[0]!=0x80 || b[1]!=0)return GH_SOUND_FORMAT;
 h.audio_offset=((size_t)b[2]<<8|b[3])+4;
 if(h.audio_offset<20 || h.audio_offset>s.size)return GH_SOUND_BOUNDS;
 if(h.audio_offset<6 || memcmp(b+h.audio_offset-6,"(c)CRI",6))return GH_SOUND_FORMAT;
 h.encoding=b[4];h.block_bytes=b[5];h.bits=b[6];h.channels=b[7];
 h.rate=be(b+8);h.samples=be(b+12);h.highpass=(uint16_t)((unsigned)b[16]<<8|b[17]);
 h.version=b[18];h.flags=b[19];
 if(!h.channels || !h.rate || !h.block_bytes)return GH_SOUND_FORMAT;
 *out=h;return GH_SOUND_OK;
}
gh_sound_status gh_sound_adx_decode_supported(const gh_adx_header *h){
 if(!h)return GH_SOUND_ARGUMENT;
 /* No ADX predictor/encryption recovery is claimed by this bank index layer. */
 return GH_SOUND_UNSUPPORTED;
}
