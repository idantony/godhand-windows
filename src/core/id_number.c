#include "id_number.h"
#include "../assets/tim2.h"
#include "../assets/font_texture.h"
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <fenv.h>
#include <stdlib.h>
static uint32_t word(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static float add(float a,float b){volatile float n=a+b;return n;}
static float mul(float a,float b){volatile float n=a*b;return n;}
static float divide(float a,float b){volatile float n=a/b;return n;}
static gh_id_error fail(uint32_t*f,uint32_t pc,gh_id_error e){if(f)*f=pc;return e;}
static gh_id_error lookup(const gh_id_state*m,unsigned slot,unsigned n,gh_id_texture*t){
 if(slot==33||!n)return GH_ID_NOT_FOUND;if(slot>=32)return GH_ID_BOUNDS;const gh_bundle_entry*b=&m->pair[slot].texture_data;
 if(!b->bytes)return GH_ID_NOT_FOUND;if(b->size<4)return GH_ID_BOUNDS;uint32_t count=word(b->bytes);if(count>(b->size-4)/4)return GH_ID_BOUNDS;if(n>count)return GH_ID_NOT_FOUND;
 uint32_t off=word(b->bytes+(size_t)n*4);if(!off)return GH_ID_NOT_FOUND;if(off<4+(size_t)count*4||off>=b->size)return GH_ID_BOUNDS;size_t end=b->size;
 for(unsigned i=1;i<=count;i++){uint32_t a=word(b->bytes+(size_t)i*4);if(a&&(a<4+(size_t)count*4||a>b->size))return GH_ID_BOUNDS;if(a>off&&a<end)end=a;}
 *t=(gh_id_texture){.source={b->bytes+off,end-off,off}};return GH_ID_OK;
}
static gh_id_error picture(const gh_id_texture*t,gh_tim2_picture*p){
 gh_tim2_error e=gh_tim2_first(t->source.bytes,t->source.size,p);if(e)return e==GH_TIM2_BOUNDS?GH_ID_BOUNDS:GH_ID_UNSUPPORTED;
 if(p->width!=128||p->height!=128||p->image_type!=4||p->clut_type!=3||p->colors!=16||p->tex0!=UINT64_C(0x1dd408000))return GH_ID_UNSUPPORTED;
 for(unsigned i=0;i<16;i++)if(p->clut[(size_t)i*4+3]>128)return GH_ID_UNSUPPORTED;return GH_ID_OK;
}
static uint64_t wide(const uint8_t*p){return (uint64_t)word(p)|(uint64_t)word(p+4)<<32;}
static gh_id_error transfers(const gh_id_texture*t,gh_font_transfer*image,gh_font_transfer*clut){
 if(t->source.size<48||wide(t->source.bytes+32)!=UINT64_C(0x20000001dd308000))return GH_ID_UNSUPPORTED;
 gh_font_slot slot={0};slot.bound=1;slot.mst=t->source.bytes;slot.mst_size=t->source.size;
 for(unsigned i=0;i<2;i++){slot.table_offset[i]=word(slot.mst+8+i*4);slot.table_count[i]=(uint16_t)(slot.mst[16+i*2]|(unsigned)slot.mst[17+i*2]<<8);if(slot.table_count[i]!=1)return GH_ID_UNSUPPORTED;}
 if(gh_font_transfer_at(&slot,0,0,image)||gh_font_transfer_at(&slot,1,0,clut))return GH_ID_UNSUPPORTED;
 if(image->width!=64||image->height!=64||image->buffer_width!=1||image->destination_base||clut->width!=16||clut->height!=16||clut->destination_base)return GH_ID_UNSUPPORTED;
 for(unsigned i=0;i<256;i++)if(clut->pixels[i*4+3]>128)return GH_ID_UNSUPPORTED;return GH_ID_OK;
}
static unsigned address32(unsigned x,unsigned y){unsigned bx=x%64/8,by=y%32/8;unsigned b=(bx&1)^((bx&2)<<1)^((bx&4)<<2)^((by&1)<<1)^((by&2)<<2);unsigned c=(x&1)^((x&2)<<1)^((x&4)<<1)^((y&1)<<1)^((y&2)<<3)^((y&4)<<3);return y/32*2048+b*64+c;}
static unsigned address8(unsigned x,unsigned y){unsigned bx=x/16,by=y%64/16,cx=x%16,cy=y%16;unsigned b=(bx&1)^((bx&2)<<1)^((bx&4)<<2)^((by&1)<<1)^((by&2)<<2);unsigned c=((cx&1)<<2)^((cx&2)<<3)^((cx&4)<<3)^((cx&8)>>2)^((cy&1)<<3)^((cy&2)?33:0)^((cy&4)?96:0)^((cy&8)<<4);return y/64*8192+b*256+c;}
gh_id_error gh_id_number_texture_open(const gh_id_state*m,uint32_t slot,uint32_t index,gh_id_texture*out){
 if(!m||!out)return GH_ID_ARGUMENT;gh_id_texture t;gh_id_error e=lookup(m,slot,index,&t);if(e)return e;
 if(t.source.size>=4&&!memcmp(t.source.bytes,"TIM2",4)){gh_tim2_picture p;e=picture(&t,&p);if(e)return e;t.tex0=p.tex0;}
 else{gh_font_transfer image,clut;e=transfers(&t,&image,&clut);if(e)return e;t.tex0=wide(t.source.bytes+32);}t.width=t.height=128;*out=t;return GH_ID_OK;
}
gh_id_error gh_id_number_texture_rgba(const gh_id_texture*t,void*out,size_t n){
 if(!t||!out||!t->source.bytes)return GH_ID_ARGUMENT;if(n<128*128*4)return GH_ID_CAPACITY;
 if(t->source.size>=4&&!memcmp(t->source.bytes,"TIM2",4)){gh_tim2_picture p;gh_id_error e=picture(t,&p);if(e)return e;return gh_tim2_rgba4(&p,out,n)==GH_TIM2_OK?GH_ID_OK:GH_ID_UNSUPPORTED;}
 gh_font_transfer image,clut;gh_id_error e=transfers(t,&image,&clut);if(e)return e;unsigned inverse[4096];for(unsigned i=0;i<4096;i++)inverse[i]=UINT32_MAX;
 for(unsigned y=0;y<64;y++)for(unsigned x=0;x<64;x++){unsigned a=address32(x,y);if(a>=4096||inverse[a]!=UINT32_MAX)return GH_ID_FORMAT;inverse[a]=y*64+x;}
 uint8_t*rgba=out;for(unsigned y=0;y<128;y++)for(unsigned x=0;x<128;x++){unsigned a=address8(x,y);if(a>=16384||inverse[a/4]==UINT32_MAX)return GH_ID_BOUNDS;unsigned index=image.pixels[inverse[a/4]*4+a%4];unsigned palette=(index&~24u)|((index&8)<<1)|((index&16)>>1);const uint8_t*c=clut.pixels+palette*4;size_t o=((size_t)y*128+x)*4;rgba[o]=c[0];rgba[o+1]=c[1];rgba[o+2]=c[2];rgba[o+3]=c[3]==128?255:(uint8_t)(c[3]*2);}return GH_ID_OK;
}
gh_id_error gh_id_number_prepare(const gh_id_state*m,const gh_id_base*b,const gh_id_entry*e,gh_id_texture_quad*out,size_t cap,size_t*count,uint32_t*f){
 if(f)*f=0;if(!m||!b||!e||!count||e->kind!=1)return GH_ID_ARGUMENT;
 if(e->b8c<1||e->b8c>9||e->b89>4)return fail(f,0x2aaff0,GH_ID_UNSUPPORTED);
 int32_t value=(int32_t)e->seed;if(value==INT32_MIN)return fail(f,0x2c70a8,GH_ID_UNSUPPORTED);
 unsigned maxdigits=(unsigned)e->b8c;int32_t limit=1;for(unsigned i=0;i<maxdigits;i++)limit*=10;if(value>=limit)value=limit-1;
 int negative=value<0;if(negative)value=-value;if(value>999999999)return fail(f,0x2c70a8,GH_ID_UNSUPPORTED);
 unsigned used=maxdigits;if(!(e->flags&0x100)){used=1;int32_t v=value;while(v>=10){used++;v/=10;}}
 size_t n=used+(unsigned)negative;if(n>cap)return GH_ID_CAPACITY;if(n&&!out)return GH_ID_ARGUMENT;
 gh_id_texture texture;gh_id_error error=lookup(m,e->b8b?e->b8a:1,e->b8b?(unsigned)e->b8b+1:28,&texture);
 if(error==GH_ID_NOT_FOUND)error=lookup(m,1,28,&texture);if(error)return fail(f,0x2acdc8,error);
 if(texture.source.size>=4&&!memcmp(texture.source.bytes,"TIM2",4)){gh_tim2_picture p;error=picture(&texture,&p);if(error)return fail(f,0x2c6d48,error);texture.tex0=p.tex0;}
 else {gh_font_transfer image,clut;error=transfers(&texture,&image,&clut);if(error)return fail(f,0x2c6d48,error);texture.tex0=wide(texture.source.bytes+32);}texture.width=texture.height=128;
 float x=e->draw_x,y=e->draw_y,w=e->f68,h=e->f6c,spacing=e->f70[0];
 if(!isfinite(x)||!isfinite(y)||!isfinite(w)||!isfinite(h)||!isfinite(spacing)||!isfinite(e->draw_scale_x)||!isfinite(e->draw_scale_y))return fail(f,0x2aaff0,GH_ID_UNSUPPORTED);
 gh_id_texture_quad q[10];fenv_t env;if(feholdexcept(&env))return GH_ID_UNSUPPORTED;if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return GH_ID_UNSUPPORTED;}
 w=divide(mul(w,e->draw_scale_x),(float)e->b8c);h=mul(h,e->draw_scale_y);
 if(w<0||h<0||!isfinite(w)||!isfinite(h)){(void)fesetenv(&env);return fail(f,0x2c6550,GH_ID_UNSUPPORTED);}
 float step=add(w,spacing);switch(e->b89){case 0:x=add(x,-mul(w,add(mul((float)used,.5f),-.5f)));break;case 1:y=add(y,mul(h,.5f));x=add(x,mul(w,.5f));break;case 2:y=add(y,mul(h,.5f));x=add(x,-mul(step,add((float)used,-.5f)));break;case 3:y=add(y,-mul(h,.5f));x=add(x,mul(w,.5f));break;case 4:y=add(y,-mul(h,.5f));x=add(x,-mul(step,add((float)used,-.5f)));break;}
 int divisor=1;for(unsigned i=1;i<used;i++)divisor*=10;
 for(size_t i=0;i<n;i++){int digit;float cx;
  if(negative&&!i){digit=-1;cx=add(x,-step);}else{digit=value/divisor%10;divisor/=10;cx=add(x,mul(step,(float)(i-(unsigned)negative)));}
  unsigned col=digit<0?2:(unsigned)digit%4,row=digit<0?3:(unsigned)digit/4;
  q[i]=(gh_id_texture_quad){.texture=texture,.x0=add(add(cx,-mul(w,.5f)),1),.y0=add(add(y,-mul(h,.5f)),1),.color=e->color,.color_mode=b->mode==6?e->b8d:b->mode,.font_mode=e->h8e,.clip=0};
  q[i].x1=add(q[i].x0,w);q[i].y1=add(q[i].y0,h);q[i].u0=(float)col*.25f;q[i].v0=(float)row*.25f;q[i].u1=q[i].u0+.25f;q[i].v1=q[i].v0+.25f;
 }
 if(fesetenv(&env))return GH_ID_UNSUPPORTED;memcpy(out,q,n*sizeof(*q));*count=n;return GH_ID_OK;
}
