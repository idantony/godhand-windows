#include "se_pitch.h"
#include <stdlib.h>
#include <string.h>
static unsigned h(const uint8_t*p){return p[0]|(unsigned)p[1]<<8;}
static uint32_t w(const uint8_t*p){return h(p)|h(p+2)<<16;}
static int16_t signed16(unsigned n){uint16_t x=(uint16_t)n;int16_t out;memcpy(&out,&x,2);return out;}
static const uint8_t*mapped(const uint8_t*b,size_t n,uint32_t address,size_t bytes){
 if(n<52||memcmp(b,"\177ELF\1\1\1",7)||h(b+18)!=8)return NULL;
 uint32_t table=w(b+28);unsigned stride=h(b+42),count=h(b+44);if(stride<32||count>128||table>n||count>(n-table)/stride)return NULL;
 for(unsigned i=0;i<count;i++){const uint8_t*p=b+table+(size_t)i*stride;uint32_t off=w(p+4),base=w(p+8),size=w(p+16);if(w(p)!=1||address<base||off>n||size>n-off)continue;size_t delta=address-base;if(delta<=size&&bytes<=size-delta)return b+off+delta;}return NULL;
}
static gh_voice_status boot_word(gh_disc*d,uint32_t address,uint32_t*out){uint8_t header[52],p[32],raw[4];uint64_t size=gh_disc_boot_elf_size(d);if(size<52||gh_disc_read_boot_elf(d,0,header,52))return GH_VOICE_BOUNDS;if(memcmp(header,"\177ELF\1\1\1",7)||h(header+18)!=8||h(header+42)!=32)return GH_VOICE_UNSUPPORTED;uint32_t table=w(header+28);unsigned count=h(header+44);if(count>128||(uint64_t)table+count*32>size)return GH_VOICE_BOUNDS;for(unsigned i=0;i<count;i++){if(gh_disc_read_boot_elf(d,(uint64_t)table+i*32,p,32))return GH_VOICE_BOUNDS;uint32_t off=w(p+4),base=w(p+8),files=w(p+16);if(w(p)!=1||address<base||(uint64_t)address+4>(uint64_t)base+files)continue;if((uint64_t)off+files>size||gh_disc_read_boot_elf(d,(uint64_t)off+address-base,raw,4))return GH_VOICE_BOUNDS;*out=w(raw);return GH_VOICE_OK;}return GH_VOICE_BOUNDS;}
gh_voice_status gh_se_pitch_load_reference(gh_se_pitch_reference*s,gh_disc*d){
 if(!s||!d)return GH_VOICE_ARGUMENT;if(s->loaded)return GH_VOICE_UNSUPPORTED;
 uint32_t seed,store;gh_voice_status e=boot_word(d,0x377bf8,&seed);if(e)return e;e=boot_word(d,0x377c2c,&store);if(e)return e;
 if(seed!=0x24030d37||store!=0xa6030020)return GH_VOICE_UNSUPPORTED;
 uint64_t total;if(gh_disc_iso_file_size(d,"MODULES/IRX.IMS",&total)||total<16)return GH_VOICE_BOUNDS;uint8_t head[16];if(gh_disc_read_iso_file(d,"MODULES/IRX.IMS",0,head,16)||memcmp(head,"IMS\0",4))return GH_VOICE_UNSUPPORTED;unsigned count=w(head+8);if(count>256||(uint64_t)16+count*32>total)return GH_VOICE_BOUNDS;
 uint32_t offset=0,bytes=0;for(unsigned i=0;i<count;i++){uint8_t row[32];if(gh_disc_read_iso_file(d,"MODULES/IRX.IMS",16+(uint64_t)i*32,row,32))return GH_VOICE_BOUNDS;if(!memcmp(row,"libsd.irx\0",10)){if(bytes)return GH_VOICE_UNSUPPORTED;offset=w(row+16);bytes=w(row+20);}}
 if(!bytes||bytes>2*1024*1024||offset>total||bytes>total-offset)return GH_VOICE_BOUNDS;
 uint8_t*module=malloc(bytes);if(!module)return GH_VOICE_MEMORY;if(gh_disc_read_iso_file(d,"MODULES/IRX.IMS",offset,module,bytes)){free(module);return GH_VOICE_BOUNDS;}
 gh_se_pitch_reference candidate={0};const uint8_t*semi=mapped(module,bytes,0x54c0,24),*fine=mapped(module,bytes,0x54d8,256),*code=mapped(module,bytes,0x3bd0,0x114);int exported=0;
 for(size_t p=12;p+8+14*4<=bytes;p++)if(!memcmp(module+p,"libsd\0\0\0",8)&&w(module+p-12)==0x41c00000&&w(module+p+8+13*4)==0x3bd0)exported++;
 /* Bind this recovered implementation only to the actual imported ordinal0D
  * and original lookup instructions; malformed modules never publish tables. */
 if(exported!=1||!semi||!fine||!code||w(code)!=0x00804021||w(code+0xc8)!=0x948454c0||w(code+0xd4)!=0x944254d8){free(module);return GH_VOICE_UNSUPPORTED;}
 for(unsigned i=0;i<12;i++)candidate.semitones[i]=(uint16_t)h(semi+i*2);for(unsigned i=0;i<128;i++)candidate.fine[i]=(uint16_t)h(fine+i*2);
 candidate.random_state=(uint16_t)seed;candidate.loaded=1;free(module);*s=candidate;return GH_VOICE_OK;
}
gh_voice_status gh_se_pitch_choose_0037d760(gh_se_pitch_reference*s,int16_t low,int16_t high,int16_t*out){
 if(!s||!s->loaded||!out)return GH_VOICE_ARGUMENT;int difference=(int)high-low;if(difference<0||difference>32767)return GH_VOICE_UNSUPPORTED;if(!difference){*out=low;return GH_VOICE_OK;}
 /* Original377FA0: signed16 triple, arithmetic high byte, two-byte shuffle;
  * escape modifies stored seed but not returned low byte. */
 uint16_t old=s->random_state;int triple=signed16((unsigned)((int)signed16(old)*3));int delta=triple>=0?triple/256:-((-triple+255)/256);unsigned result=(old+(unsigned)delta)&255;uint16_t next=(uint16_t)(result|((unsigned)delta&255)<<8);if((int)old==(int)signed16(next))next=(uint16_t)(old+1);s->random_state=next;
 int add=difference<=16?(int)(result%(unsigned)(difference+1)):(int)(result%17)*(difference/16);*out=signed16((unsigned)(low+add));return GH_VOICE_OK;
}
gh_voice_status gh_se_pitch_q12_000038e4(const gh_se_pitch_reference*s,int16_t offset,uint32_t rate,uint16_t*out){
 if(!s||!s->loaded||!out||!rate||rate>48000)return GH_VOICE_ARGUMENT;
 int whole=(int)offset/128,fine=(int)offset-whole*128;int note=signed16((unsigned)whole),octave=note/12-2,semitone=note%12;
 if(semitone<0||(semitone==0&&fine<0)){semitone+=12;octave=note/12-3;}
 /* snddrv decomposes offset first: libsd a3 is already a remainder, so its own carry is zero. */if(fine<0){fine+=128;semitone--;}
 if(semitone<0||semitone>=12||fine<0||fine>=128)return GH_VOICE_UNSUPPORTED;
 uint32_t product=(uint32_t)s->semitones[semitone]*s->fine[fine];int32_t signed_product;memcpy(&signed_product,&product,4);int32_t value=signed_product>=0?signed_product/65536:-((-((int64_t)signed_product)+65535)/65536);
 uint32_t pitch=(uint32_t)value;if(octave<0){unsigned shift=(unsigned)-octave;pitch=(pitch+(UINT32_C(1)<<((shift-1)&31)))>>(shift&31);}
 int32_t product_rate=(int32_t)(int16_t)(uint16_t)pitch*(int32_t)rate;uint32_t scaled=(uint32_t)product_rate/48000;uint16_t result=(uint16_t)scaled;if(result>0x3fff)result=0x3fff;if(!result)return GH_VOICE_UNSUPPORTED;*out=result;return GH_VOICE_OK;
}
