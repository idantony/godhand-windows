#include "font_texture.h"
#include <stdlib.h>
static uint64_t word64(const unsigned char *p){uint64_t v=0;unsigned i;for(i=0;i<8;i++)v|=(uint64_t)p[i]<<(i*8);return v;}
gh_font_error gh_font_transfer_at(const gh_font_slot *slot,unsigned table,unsigned index,gh_font_transfer *out){
 gh_font_texture_record r;gh_font_error error;const unsigned char *p;uint64_t tag,bitblt,trxreg;size_t bytes;gh_font_transfer t;
 if(!out)return GH_FONT_ARGUMENT;
 error=gh_font_texture_at(slot,table,index,&r);if(error)return error;
 if(r.available<96)return GH_FONT_BOUNDS;p=r.data;
 /* GIF packed A+D: four GS writes, then IMAGE payload. */
 tag=word64(p);
 if((tag&0x7fff)!=4||((tag>>58)&3)!=0||((tag>>60)&15)!=1||word64(p+8)!=14)return GH_FONT_UNSUPPORTED;
 if(word64(p+24)!=0x50||word64(p+40)!=0x51||word64(p+56)!=0x52||word64(p+72)!=0x53)return GH_FONT_UNSUPPORTED;
 if(word64(p+32)||word64(p+64))return GH_FONT_UNSUPPORTED;
 bitblt=word64(p+16);trxreg=word64(p+48);tag=word64(p+80);
 if(((tag>>58)&3)!=2)return GH_FONT_UNSUPPORTED;
 t.width=(uint16_t)(trxreg&0xfff);t.height=(uint16_t)((trxreg>>32)&0xfff);
 t.psm=(uint8_t)((bitblt>>56)&63);t.destination_base=(uint16_t)((bitblt>>32)&0x3fff);t.buffer_width=(uint8_t)((bitblt>>48)&63);
 if(!t.width||!t.height||t.psm!=0)return GH_FONT_UNSUPPORTED;
 bytes=(size_t)t.width*t.height*4;
 if(bytes!=(size_t)(tag&0x7fff)*16||bytes>r.available-96)return GH_FONT_BOUNDS;
 t.pixels=p+96;t.pixel_bytes=bytes;*out=t;return GH_FONT_OK;
}
/* Independently derived coordinate-bit permutation; checked against official
 * PCSX2 GSTables at pinned commit in font-texture-evidence. No table/code copy. */
static unsigned col32(unsigned x,unsigned y){return (x&1)^((x&2)<<1)^((x&4)<<1)^((y&1)<<1)^((y&2)<<3)^((y&4)<<3);}
static unsigned addr32(unsigned x,unsigned y){
 unsigned bx=(x%64)/8,by=(y%32)/8;
 unsigned block=(bx&1)^((bx&2)<<1)^((bx&4)<<2)^((by&1)<<1)^((by&2)<<2);
 return (y/32)*2048+block*64+col32(x%8,y%8);
}
static unsigned addr4(unsigned x,unsigned y){
 unsigned bx=(x%128)/32,by=(y%128)/16;
 unsigned block=((bx&1)<<1)^((bx&2)<<2)^(by&1)^((by&2)<<1)^((by&4)<<2);
 unsigned cx=x%32,cy=y%16;
 unsigned col=((cx&1)<<3)^((cx&2)<<4)^((cx&4)<<4)^((cx&8)>>2)^((cx&16)>>2)^((cy&1)<<4)^((cy&2)?65:0)^((cy&4)?192:0)^((cy&8)<<5);
 return ((x/128)+(y/128)*4)*16384+block*512+col;
}
gh_font_error gh_font_atlas_bank_rgba(const gh_font_slot *s,unsigned parity,void *rgba,size_t capacity){
 gh_font_transfer image,clut;gh_font_error e;unsigned *inverse;unsigned char *out=rgba;unsigned x,y,i;
 if(!s||!rgba)return GH_FONT_ARGUMENT;
 if(parity>1)return GH_FONT_UNSUPPORTED;
 if(capacity<512U*256U*4U)return GH_FONT_BOUNDS;
 e=gh_font_transfer_at(s,0,0,&image);if(e)return e;
 e=gh_font_transfer_at(s,1,0,&clut);if(e)return e;
 if(image.width!=64||image.height!=256||image.buffer_width!=1||clut.width!=16||clut.height!=16)return GH_FONT_UNSUPPORTED;
 if(s->mst_size<40||word64(s->mst+32)!=UINT64_C(0x2000000625420000))return GH_FONT_UNSUPPORTED;
 inverse=malloc(16384*sizeof(*inverse));if(!inverse)return GH_FONT_ARGUMENT;
 for(i=0;i<16384;i++)inverse[i]=0xffffffffU;
 for(y=0;y<256;y++)for(x=0;x<64;x++){
  unsigned a=addr32(x,y);if(a>=16384||inverse[a]!=0xffffffffU){free(inverse);return GH_FONT_UNSUPPORTED;}inverse[a]=y*64+x;
 }
 for(y=0;y<256;y++)for(x=0;x<512;x++){
  unsigned nibble=addr4(x,y),word=nibble/8,shift=(nibble%8)*4,index;const unsigned char *pixel,*color;
  if(word>=16384){free(inverse);return GH_FONT_BOUNDS;}
  pixel=image.pixels+inverse[word]*4;index=(unsigned)((word64((const unsigned char[8]){pixel[0],pixel[1],pixel[2],pixel[3],0,0,0,0})>>shift)&15);
  color=clut.pixels+((index&7)+(index>>3)*16+parity*8)*4;i=(y*512+x)*4;
  out[i]=color[0];out[i+1]=color[1];out[i+2]=color[2];out[i+3]=color[3]>=128?255:(unsigned char)(color[3]*2);
 }
 free(inverse);return GH_FONT_OK;
}
gh_font_error gh_font_atlas_rgba(const gh_font_slot *s,void *rgba,size_t capacity){return gh_font_atlas_bank_rgba(s,0,rgba,capacity);}
gh_font_error gh_font_glyph_rgba(const gh_font_state *font,uint16_t code,void *rgba,size_t capacity,unsigned *width,unsigned *height){
 gh_font_glyph g;const gh_font_slot *s;gh_font_error e;unsigned cell,columns,u,v,y;unsigned char *atlas;
 if(!font||!rgba||!width||!height)return GH_FONT_ARGUMENT;
 e=gh_font_glyph_at(font,code,&g);if(e)return e;s=&font->slots[code>>12];
 if(!s->cell_width||s->width%s->cell_width||g.width<=0)return GH_FONT_UNSUPPORTED;
 columns=s->width/s->cell_width;cell=(code&4095)>>1;u=(cell%columns)*s->cell_width+g.left;v=(cell/columns)*s->cell_height;
 if(u+(unsigned)g.width>512||v+s->cell_height>256||capacity<(size_t)(unsigned)g.width*s->cell_height*4)return GH_FONT_BOUNDS;
 atlas=malloc(512U*256U*4U);if(!atlas)return GH_FONT_ARGUMENT;
 e=gh_font_atlas_bank_rgba(s,code&1,atlas,512U*256U*4U);
 if(!e)for(y=0;y<s->cell_height;y++){
  size_t j;const unsigned char *src=atlas+((v+y)*512+u)*4;unsigned char *dst=(unsigned char*)rgba+(size_t)y*(unsigned)g.width*4;
  for(j=0;j<(size_t)(unsigned)g.width*4;j++)dst[j]=src[j];
 }
 free(atlas);if(e)return e;*width=(unsigned)g.width;*height=s->cell_height;return GH_FONT_OK;
}
