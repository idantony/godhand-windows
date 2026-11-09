/* Semantic recovery of actual ELF2A53A8..2A589C; no packet/code copied.
 * Actual30A570 loads VU vector, vftoi4, stores: fixedpoint truncation.
 * Source scope and raw instruction evidence: analysis/font-draw-evidence.json. */
#include "font_draw.h"
#include <math.h>
#include <limits.h>
static int fixed(float value,int32_t *out){
 float scaled=value*16.0f;
 if(!isfinite(scaled)||scaled< -2147483648.0f||scaled>=2147483648.0f)return 0;
 *out=(int32_t)scaled;return 1;
}
gh_font_draw_error gh_font_draw_quad(const gh_font_state *font,const gh_font_draw_state *s,
 uint16_t code,float x,float y,float z,gh_font_quad *out){
 gh_font_glyph g;const gh_font_slot *slot;gh_font_quad q;
 unsigned cell,columns,u,v;float width,height,x1,y1,yscale;int32_t fx0,fx1,fy0,fy1,fz;int64_t ox,oy;float advance;
 if(!font||!s||!out)return GH_FONT_DRAW_ARGUMENT;
 if(gh_font_glyph_at(font,code,&g)!=GH_FONT_OK)return GH_FONT_DRAW_UNSUPPORTED;
 slot=&font->slots[code>>12];
 if(!slot->cell_width||slot->width%slot->cell_width||g.width<=0||
    !isfinite(s->scale_x)||!isfinite(s->scale_y)||s->scale_x<0||s->scale_y<0||
    s->screen_width<=0||s->screen_height<=0||z<0)return GH_FONT_DRAW_UNSUPPORTED;
 columns=slot->width/slot->cell_width;cell=(code&4095)>>1;
 u=(cell%columns)*slot->cell_width+g.left;v=(cell/columns)*slot->cell_height;
 if(u+(unsigned)g.width>65535/16||v+slot->cell_height>65535/16)return GH_FONT_DRAW_BOUNDS;
 width=(float)g.width*s->scale_x;height=(float)slot->cell_height*s->scale_y;
 if(!(s->alignment_flags&1)){if(s->alignment_flags&2)x-=width*0.5f;else if(s->alignment_flags&4)x-=width;}
 if(!(s->alignment_flags&8)){if(s->alignment_flags&16)y-=height*0.5f;else if(s->alignment_flags&32)y-=height;}
 x1=x+width;y1=y+height;yscale=(float)s->screen_height/448.0f;
 if(!fixed(x,&fx0)||!fixed(x1,&fx1)||!fixed(y*yscale,&fy0)||!fixed(y1*yscale,&fy1)||!fixed(z,&fz))return GH_FONT_DRAW_UNSUPPORTED;
 ox=((int64_t)2048-s->screen_width/2)*16;oy=(int64_t)s->gs_y_origin*16;
 if((int64_t)fx0+ox<0||(int64_t)fx1+ox>65535||(int64_t)fy0+oy<0||(int64_t)fy1+oy>65535)return GH_FONT_DRAW_BOUNDS;
 advance=width;if(!isfinite(advance)||advance<0||advance>=65536)return GH_FONT_DRAW_UNSUPPORTED;
 q.x0=(uint16_t)(fx0+ox);q.x1=(uint16_t)(fx1+ox);q.y0=(uint16_t)(fy0+oy);q.y1=(uint16_t)(fy1+oy);
 q.u0=(uint16_t)(u*16+8);q.v0=(uint16_t)(v*16+8);q.u1=(uint16_t)((u+(unsigned)g.width)*16+8);q.v1=(uint16_t)((v+slot->cell_height)*16+8);
 q.z=(uint32_t)fz>>4;q.rgba=s->rgba;q.advance=(uint16_t)advance;q.slot=(uint8_t)(code>>12);q.palette_parity=(uint8_t)(code&1);
 *out=q;return GH_FONT_DRAW_OK;
}
