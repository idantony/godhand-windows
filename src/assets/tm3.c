/* ActualTM3/TIM3 source reader:151630/1522C0/151990/151CC0/151168.
 * Address mathematics independently reconstructed for native pixels; no GPL
 * implementation copied. See existing GS-layout/font/T32 evidence. */
#include "tm3.h"
#include "gs_indexed_layout.h"
#include <string.h>
static uint16_t u16(const uint8_t*p){return (uint16_t)(p[0]|(unsigned)p[1]<<8);}
static uint32_t u32(const uint8_t*p){return (uint32_t)u16(p)|(uint32_t)u16(p+2)<<16;}
static uint64_t u64(const uint8_t*p){return (uint64_t)u32(p)|(uint64_t)u32(p+4)<<32;}
gh_tm3_error gh_tm3_open(const void*raw,size_t n,gh_tm3_bundle*out){
 if(!raw||!out)return GH_TM3_ARGUMENT;const uint8_t*b=raw;if(n<16)return GH_TM3_BOUNDS;
 if(memcmp(b,"TM3\0",4)||u32(b+8)!=4||u32(b+12))return GH_TM3_UNSUPPORTED;
 uint32_t count=u32(b+4);if(!count||count>256||count>(n-16)/4)return GH_TM3_BOUNDS;
 for(unsigned i=0;i<count;i++){uint32_t o=u32(b+16+i*4);if(o<16+(size_t)count*4||o>=n)return GH_TM3_BOUNDS;}
 *out=(gh_tm3_bundle){b,n,count};return GH_TM3_OK;
}
gh_tm3_error gh_tm3_member(const gh_tm3_bundle*t,unsigned index,gh_tm3_picture*out){
 if(!t||!out||!t->bytes)return GH_TM3_ARGUMENT;if(index>=t->count||index>255||t->size<16||(size_t)t->count>(t->size-16)/4)return GH_TM3_BOUNDS;
 size_t off=u32(t->bytes+16+index*4),end=t->size;
 if(off<16+(size_t)t->count*4||off>=end)return GH_TM3_BOUNDS;
 for(unsigned i=0;i<t->count;i++){uint32_t next=u32(t->bytes+16+i*4);if(next<16+(size_t)t->count*4||next>=t->size)return GH_TM3_BOUNDS;if(next>off&&next<end)end=next;}
 const uint8_t*b=t->bytes+off;if(end-off<64)return GH_TM3_BOUNDS;
 if(memcmp(b,"TIM3",4)||b[4]!=4||(b[5]!=0&&b[5]!=6)||u16(b+6)!=1)return GH_TM3_UNSUPPORTED;
 const uint8_t*p=b+16;uint32_t total=u32(p),clut=u32(p+4),image=u32(p+8);uint16_t head=u16(p+12),w=u16(p+20),h=u16(p+22);
 if(head<48||total<head||total>end-off-16||image>total-head||clut>total-head-image)return GH_TM3_BOUNDS;
 uint64_t tex0=u64(p+24);unsigned tw=(unsigned)(tex0>>26)&15,th=(unsigned)(tex0>>30)&15;
 unsigned psm=(unsigned)(tex0>>20)&63;
 if((psm!=19&&psm!=20)||((tex0>>51)&63)||((tex0>>56)&31)||(1u<<tw)!=w||(1u<<th)!=h||p[17]!=1||p[18]!=3)return GH_TM3_UNSUPPORTED;
 unsigned indexed8=psm==19;
 if(u16(p+14)!=(indexed8?256:16)||p[19]!=(indexed8?5:4)||
    image!=(size_t)w*h/(indexed8?1:2)||clut!=(indexed8?1024:64))return GH_TM3_UNSUPPORTED;
 if(indexed8){
  if(b[5]==0){if(w!=64||h!=64)return GH_TM3_UNSUPPORTED;}
  else if(w<128||w>512||h<64||h>512)return GH_TM3_UNSUPPORTED;
 }
 else if(b[5]==0){if((w!=32&&w!=64&&w!=128)||(h!=16&&h!=32&&h!=64))return GH_TM3_UNSUPPORTED;}
 else if((w!=128&&w!=256)||(h!=128&&h!=256&&h!=512))return GH_TM3_UNSUPPORTED;
 *out=(gh_tm3_picture){b,p+head,p+head+image,end-off,image,clut,w,h,tex0,b[5]};return GH_TM3_OK;
}
static size_t addr4(unsigned x,unsigned y,unsigned pitch){
 unsigned bx=(x%128)/32,by=(y%128)/16,cx=x%32,cy=y%16;
 unsigned block=((bx&1)<<1)^((bx&2)<<2)^(by&1)^((by&2)<<1)^((by&4)<<2);
 unsigned column=((cx&1)<<3)^((cx&2)<<4)^((cx&4)<<4)^((cx&8)>>2)^((cx&16)>>2)^((cy&1)<<4)^((cy&2)?65:0)^((cy&4)?192:0)^((cy&8)<<5);
 return ((x/128)+(size_t)(y/128)*pitch)*16384+block*512+column;
}
static unsigned bit(size_t a,unsigned n){return (unsigned)(a>>n)&1u;}
static size_t upload_word(size_t word,unsigned pitch){
 size_t page=word/2048;
 unsigned x=bit(word,0)|(bit(word,2)<<1)|(bit(word,3)<<2)|(bit(word,6)<<3)|(bit(word,8)<<4)|(bit(word,10)<<5);
 unsigned y=bit(word,1)|(bit(word,4)<<1)|(bit(word,5)<<2)|(bit(word,7)<<3)|(bit(word,9)<<4);
 return ((page/pitch)*32+y)*(pitch*64)+(page%pitch)*64+x;
}
gh_tm3_error gh_tm3_rgba_gs(const gh_tm3_picture*p,void*raw,size_t capacity){
 if(!p||!raw||!p->image||!p->clut)return GH_TM3_ARGUMENT;size_t pixels=(size_t)p->width*p->height;
 unsigned psm=(unsigned)(p->tex0>>20)&63,indexed8=psm==19;
 if(psm!=19&&psm!=20)return GH_TM3_UNSUPPORTED;
 if(!pixels||p->image_bytes!=pixels/(indexed8?1:2)||p->clut_bytes!=(indexed8?1024:64))return GH_TM3_BOUNDS;if(capacity<pixels*4)return GH_TM3_CAPACITY;
 if(p->transfer_kind!=0&&p->transfer_kind!=6)return GH_TM3_UNSUPPORTED;
 if(indexed8){
  uint8_t *out=raw;
  if(p->transfer_kind==0){
   if(p->width!=64||p->height!=64)return GH_TM3_UNSUPPORTED;
   /* 151990 uses 150F68 with PSMT8 and unscaled width/height.
    * The direct image transfer reads rows in order. The CT32 CLUT
    * still follows the common 16x16 CSM1 index permutation. */
   for(size_t i=0;i<pixels;i++){
    unsigned palette=gh_gs_csm1_palette256(p->image[i]);
    memcpy(out+i*4,p->clut+palette*4,4);
   }
   return GH_TM3_OK;
  }
  if(p->width<128||p->width>512||p->height<64||p->height>512||
     (p->width&(p->width-1))||(p->height&(p->height-1)))return GH_TM3_UNSUPPORTED;
  unsigned pitch=p->width/128;
  /* 151168 uploads PSMT8 through CT32 at half width and half height. */
  for(unsigned y=0;y<p->height;y++)for(unsigned x=0;x<p->width;x++){
   size_t address=(size_t)gh_gs_psmt8_offset(x,y,pitch);
   size_t offset=upload_word(address/4,pitch)*4+address%4;
   if(offset>=p->image_bytes)return GH_TM3_BOUNDS;
   unsigned palette=gh_gs_csm1_palette256(p->image[offset]);
   memcpy(out+((size_t)y*p->width+x)*4,p->clut+palette*4,4);
  }
  return GH_TM3_OK;
 }
 uint8_t*out=raw;
 for(unsigned y=0;y<p->height;y++)for(unsigned x=0;x<p->width;x++){
  size_t nibble=(size_t)y*p->width+x;
  if(p->transfer_kind==6){
   if((p->width!=128&&p->width!=256)||(p->height!=128&&p->height!=256&&p->height!=512))return GH_TM3_UNSUPPORTED;
   unsigned pitch=p->width/128;size_t a=addr4(x,y,pitch);
   nibble=upload_word(a/8,pitch)*8+a%8;
  }
  if(nibble/2>=p->image_bytes)return GH_TM3_BOUNDS;
  unsigned index=p->image[nibble/2]>>((nibble&1)*4)&15;memcpy(out+((size_t)y*p->width+x)*4,p->clut+index*4,4);
 }
 return GH_TM3_OK;
}
