/* Actual message address/layout/normal-glyph consumers. Evidence from local
 * EE disassembly/Ghidra: analysis/id-text*.json. Source names/semantics also
 * MIT upstream001e4920.c/002aa6e0.c; notice LICENSE.upstream. */
#include "id_text.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>
static uint16_t u16(const unsigned char *p){return (uint16_t)(p[0]|p[1]<<8);}
static uint32_t u32(const unsigned char *p){return (uint32_t)u16(p)|(uint32_t)u16(p+2)<<16;}
gh_id_error gh_id_text_palette_from_elf(const void *raw,size_t n,gh_id_view *out){
 const unsigned char *b=raw;uint32_t phoff;unsigned count,stride,i;
 if(!raw||!out)return GH_ID_ARGUMENT;if(n<52||memcmp(b,"\177ELF\1\1",6))return GH_ID_FORMAT;
 phoff=u32(b+28);stride=u16(b+42);count=u16(b+44);
 if(stride<32||phoff>n||count>(n-phoff)/stride)return GH_ID_BOUNDS;
 for(i=0;i<count;i++){
  const unsigned char *p=b+phoff+(size_t)i*stride;uint32_t off=u32(p+4),v=u32(p+8),len=u32(p+16);size_t delta;
  if(u32(p)!=1||v>0x3c23b0)continue;delta=(size_t)0x3c23b0-v;
  if(delta>len||200>len-delta)continue;
  if(off>n||len>n-off)return GH_ID_BOUNDS;
  out->bytes=b+off+delta;out->size=200;out->offset=off+delta;return GH_ID_OK;
 }
 return GH_ID_NOT_FOUND;
}
static gh_id_error fail(gh_id_text_result *r,uint16_t op,size_t off,uint32_t address){r->opcode=op;r->source_offset=off;r->frontier_address=address;r->glyph_count=0;return GH_ID_UNSUPPORTED;}
static gh_id_error message_view(const gh_id_view *table,unsigned index,gh_id_view *body){
 uint32_t count,off;size_t end;unsigned i;
 if(!table->bytes||table->size<4)return GH_ID_NOT_FOUND;
 count=u32(table->bytes);if(count>(table->size-4)/8)return GH_ID_BOUNDS;if(index>=count)return GH_ID_NOT_FOUND;
 off=u32(table->bytes+4+(size_t)index*8);if(off<4+(size_t)count*8||off>=table->size)return GH_ID_BOUNDS;
 end=table->size;
 for(i=0;i<count;i++)for(unsigned part=0;part<2;part++){uint32_t next=u32(table->bytes+4+(size_t)i*8+part*4);if(next>off&&next<end)end=next;}
 body->bytes=table->bytes+off;body->size=end-off;body->offset=table->offset+off;return GH_ID_OK;
}
gh_id_error gh_id_text_prepare(const gh_id_base *object,const gh_id_entry *entry,const gh_menu_state *menu,
 const gh_packed_message_state *packed,const gh_font_state *font,const gh_id_text_context *ctx,
 gh_id_text_glyph *output,size_t capacity,gh_id_text_result *result){
 gh_id_view table,body;gh_id_error error;uint16_t message;unsigned bank,index,color_group,color_index,align;
 float sx,sy,spacing,line_gap,line_width=0,line_height=0,page_width=0,page_height=0,startx,starty,x,y;
 size_t offset,glyphs=0,lines=1,icons=0;uint32_t default_color,color;int ended=0;
 if(!object||!entry||!menu||!packed||!font||!ctx||!result||(!output&&capacity))return GH_ID_ARGUMENT;
 memset(result,0,sizeof(*result));
 if(entry->kind!=2||ctx->language<1||ctx->language>6)return fail(result,0,0,0x2ab0b8);
 message=entry->message?entry->message:0x3000;bank=message>>12;index=message&4095;
 if(bank==0){table.bytes=menu->data.bytes;table.size=menu->data.size;table.offset=menu->data.offset;}
 else if(bank==3){table.bytes=menu->texture_data.bytes;table.size=menu->texture_data.size;table.offset=menu->texture_data.offset;}
 else if(bank==2)table=packed->cursor;else return fail(result,0,0,0x2aead0);
 error=message_view(&table,index,&body);if(error)return error;
 color_group=entry->b8b%10;color_group=color_group>=1&&color_group<=7?color_group+2:2;color_index=entry->b8b/10;
 if(!ctx->palette.bytes||color_index>=5||(size_t)color_group*20+color_index*4+4>ctx->palette.size)return GH_ID_BOUNDS;
 const unsigned char *colors=ctx->palette.bytes+(size_t)color_group*20;
 default_color=color=u32(colors+color_index*4);
 sx=0.9f*entry->draw_scale_x;sy=entry->draw_scale_y;spacing=2.0f*entry->draw_scale_x;line_gap=(-entry->draw_scale_y*26.0f+8.0f)*entry->draw_scale_y;
 if(!isfinite(sx)||!isfinite(sy)||sx<=0||sy<=0||(entry->flags&4))return fail(result,0,0,0x2af728);
 // ActualmeasurePage/measureText: line width excludes trailing spacing;
 // ruby flag1 remains set despite languageRubyAddrNULL and adds12*zoomY.
 for(offset=0;offset+2<=body.size;offset+=2){
  uint16_t code=u16(body.bytes+offset);gh_font_glyph glyph;
  if((code&0xff00)==0x8000){ended=1;break;}
  if(code==0xa001){if(!line_height)return fail(result,code,offset,0x2b0700);if(line_width-spacing>page_width)page_width=line_width-spacing;page_height+=line_height+line_gap;line_width=line_height=0;lines++;continue;}
  if(code==0xa301||code==0xb301)continue;
  /* Original 2B04C0 B602 selects an index in the current palette group.
   * Neither it nor B301 consumes width or emits a primitive. */
  if(code==0xb602){
   if(body.size-offset<4)return fail(result,code,offset,0x2b04c0);
   unsigned index=u16(body.bytes+offset+2);
   if(index>=5||(size_t)color_group*20+index*4+4>ctx->palette.size)
    return fail(result,code,offset,0x2b04c0);
   offset+=2;continue;
  }
  if(code&0x8000)return fail(result,code,offset,0x2b0028);
  if(code>=0x1000)return fail(result,code,offset,0x2a4cb0);
  if(code<14){
   line_width+=32.0f*sx+spacing; /* 2B12D8 with nonzero language */
   const float height=(32.0f+12.0f)*sy;
   if(height>line_height)line_height=height;
   ++icons;++glyphs;continue;
  }
  if(gh_font_glyph_at(font,code,&glyph)!=GH_FONT_OK||glyph.width<=0)return fail(result,code,offset,0x2a4b20);
  line_width+=(float)glyph.width*sx+spacing;
  {float h=((float)font->slots[code>>12].cell_height+12.0f)*sy;if(h>line_height)line_height=h;}
  glyphs++;
 }
 if(!ended)return GH_ID_BOUNDS;if(line_width-spacing>page_width)page_width=line_width-spacing;page_height+=line_height;
 result->width=page_width;result->height=page_height;result->line_count=lines;result->icon_count=icons;
 if(glyphs>capacity)return GH_ID_CAPACITY;
 align=entry->b89==0?3:entry->b89<=4?0:1;startx=entry->draw_x-((align&1)?page_width*0.5f:0);starty=entry->draw_y-((align&2)?page_height*0.5f:0);x=startx;y=starty;
 {size_t out=0;for(offset=0;offset+2<=body.size;offset+=2){
  uint16_t code=u16(body.bytes+offset);gh_font_glyph glyph;gh_font_draw_state draw;
  if((code&0xff00)==0x8000)break;if(code==0xa301)continue;
  if(code==0xb301){color=default_color;continue;}
  if(code==0xb602){color=u32(colors+u16(body.bytes+offset+2)*4);offset+=2;continue;}
  if(code==0xa001){x=startx;y+=((float)font->slots[0].cell_height+12.0f)*sy+line_gap;continue;}
  if(code<14){
   memset(&output[out],0,sizeof(output[out]));
   output[out].code=code;output[out].x=x;output[out].y=y+12.0f*sy+3.0f;
   output[out].icon_extent=26.0f*sy;
   x+=32.0f*sx+spacing;
   ++out;continue;
  }
  if(gh_font_glyph_at(font,code,&glyph)!=GH_FONT_OK)return GH_ID_BOUNDS;
  draw.scale_x=sx;draw.scale_y=sy;draw.alignment_flags=9;draw.screen_width=ctx->screen_width;draw.screen_height=ctx->screen_height;draw.gs_y_origin=ctx->gs_y_origin;draw.rgba=color;
  output[out].code=code;output[out].x=x;output[out].y=y+12.0f*sy;output[out].icon_extent=0;
  if(gh_font_draw_quad(font,&draw,code,x,y+12.0f*sy,65535,&output[out].quad)!=GH_FONT_DRAW_OK)return fail(result,code,offset,0x2a53a8);
  x+=(float)glyph.width*sx+spacing;out++;
 }result->glyph_count=out;}
 return GH_ID_OK;
}


gh_id_error gh_id_text_icon_quad(const gh_id_state *manager,const gh_id_text_glyph *icon,
    gh_id_texture_quad *out)
{
    static const uint8_t columns[]={0,1,2,3,2,0,3,1,1,0,3,2,1,0};
    static const uint8_t rows[]   ={0,0,0,0,1,1,1,1,2,2,2,2,3,3};
    if(!manager||!icon||!out||icon->code>=14||
       !isfinite(icon->x)||!isfinite(icon->y)||!isfinite(icon->icon_extent)||icon->icon_extent<=0)
        return GH_ID_ARGUMENT;
    gh_id_texture_quad quad={0};
    gh_id_error status=gh_id_texture_open(manager,1,24,&quad.texture);
    if(status)return status;
    if(quad.texture.tex0!=GH_ID_PAD_ICON_TEX0)return GH_ID_UNSUPPORTED;
    const unsigned code=icon->code;
    quad.x0=icon->x;quad.x1=icon->x+icon->icon_extent;
    quad.y0=icon->y;quad.y1=icon->y+icon->icon_extent;
    quad.u0=columns[code]*0.25f;quad.u1=quad.u0+0.25f;
    quad.v0=rows[code]*0.25f;quad.v1=quad.v0+0.25f;
    quad.color=UINT32_C(0x80808080);quad.color_mode=6;quad.font_mode=5;
    *out=quad;
    return GH_ID_OK;
}
