/* Independent byte reader; layout evidence from MIT god-hand-decomp
 * include/godhand/tim2.h and src/cod/0015e0a0.c. No bitfield ABI copied. */
#include "tim2.h"
#include "gs_indexed_layout.h"
#include <string.h>
gh_tim2_error gh_tim2_rgba4(const gh_tim2_picture *p,void *rgba,size_t capacity){
 unsigned char *out=rgba;size_t n,i;
 if(!p||!rgba||!p->image||!p->clut)return GH_TIM2_ARGUMENT;
 if(!p->width||!p->height||(p->width&1)||p->image_type!=4||p->clut_type!=3||p->colors!=16||
    ((p->tex0>>20)&63)!=20||((p->tex0>>51)&15)!=0||((p->tex0>>55)&1)||((p->tex0>>56)&31))return GH_TIM2_UNSUPPORTED;
 n=(size_t)p->width*p->height;
 if(p->image_size!=n/2||p->clut_size!=64||n>SIZE_MAX/4||capacity<n*4)return GH_TIM2_BOUNDS;
 /* gsKit PNG4 loader swaps PNG's high-first nibbles before upload and
  * uses sequential RGBA32 CLUT16; only its T8 path rotates CLUT groups. */
 for(i=0;i<16;i++)if(p->clut[i*4+3]>128)return GH_TIM2_UNSUPPORTED;
 for(i=0;i<n;i++){
  unsigned index=(p->image[i/2]>>((i&1)*4))&15;
  const unsigned char *c=p->clut+index*4;
  out[i*4]=c[0];out[i*4+1]=c[1];out[i*4+2]=c[2];
  out[i*4+3]=c[3]>=128?255:(unsigned char)(c[3]*2);
 }
 return GH_TIM2_OK;
}
gh_tim2_error gh_tim2_rgba8(const gh_tim2_picture *picture,void *rgba,size_t capacity)
{
 unsigned char *out=rgba;
 if(!picture||!rgba||!picture->image||!picture->clut)return GH_TIM2_ARGUMENT;
 uint64_t tex0=picture->tex0;
 if(!picture->width||!picture->height||picture->image_type!=5||picture->clut_type!=3||
    picture->colors!=256||((tex0>>20)&63)!=19||((tex0>>51)&0x3ff))return GH_TIM2_UNSUPPORTED;
 size_t count=(size_t)picture->width*picture->height;
 if(picture->image_size!=count||picture->clut_size!=1024||
    count>SIZE_MAX/4||capacity<count*4)return GH_TIM2_BOUNDS;
 for(unsigned i=0;i<256;++i)
  if(picture->clut[i*4+3]>128)return GH_TIM2_UNSUPPORTED;
 for(size_t i=0;i<count;++i){
  unsigned index=gh_gs_csm1_palette256(picture->image[i]);
  const unsigned char *color=picture->clut+index*4;
  memcpy(out+i*4,color,3);
  out[i*4+3]=color[3]==128?255:(unsigned char)(color[3]*2);
 }
 return GH_TIM2_OK;
}
static uint16_t u16(const unsigned char *p){return (uint16_t)(p[0]|p[1]<<8);}
static uint32_t u32(const unsigned char *p){return (uint32_t)u16(p)|(uint32_t)u16(p+2)<<16;}
gh_tim2_error gh_tim2_first(const void *bytes,size_t size,gh_tim2_picture *out){
 const unsigned char *b=bytes,*p;size_t offset,total,header,image,clut,expected;
 gh_tim2_picture v;
 if(!bytes||!out)return GH_TIM2_ARGUMENT;
 if(size<16)return GH_TIM2_BOUNDS;
 if(memcmp(b,"TIM2",4))return GH_TIM2_FORMAT;
 if(b[4]!=4||b[5]>1||u16(b+6)!=1)return GH_TIM2_UNSUPPORTED;
 offset=b[5]?128:16;
 if(size<offset||size-offset<48)return GH_TIM2_BOUNDS;
 p=b+offset;total=u32(p);clut=u32(p+4);image=u32(p+8);header=u16(p+12);
 if(header<48||total<header||total>size-offset||image>total-header||clut>total-header-image)return GH_TIM2_BOUNDS;
 memset(&v,0,sizeof(v));v.width=u16(p+20);v.height=u16(p+22);v.colors=u16(p+14);
 v.image_type=p[19];v.clut_type=p[18];
 if(!v.width||!v.height)return GH_TIM2_FORMAT;
 if(p[17]!=1||(v.image_type!=4&&v.image_type!=5))return GH_TIM2_UNSUPPORTED;
 expected=(size_t)v.width*v.height;
 if(v.image_type==4)expected=(expected+1)/2;
 if(image!=expected||v.colors!=(v.image_type==4?16:256)||clut!=(size_t)v.colors*4)return GH_TIM2_UNSUPPORTED;
 v.tex0=(uint64_t)u32(p+24)|(uint64_t)u32(p+28)<<32;
 v.image=p+header;v.clut=v.image+image;v.image_size=image;v.clut_size=clut;
 *out=v;return GH_TIM2_OK;
}
