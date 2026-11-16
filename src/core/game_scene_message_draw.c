#include "game_scene_message_draw.h"

#include <fenv.h>
#include <string.h>

static uint16_t half(const uint8_t *p)
{
    return (uint16_t)(p[0] | (unsigned)p[1] << 8);
}

static uint32_t word(const uint8_t *p)
{
    uint32_t value;
    memcpy(&value, p, sizeof(value));
    return value;
}

static int32_t signed_word(const uint8_t *p)
{
    int32_t value;
    memcpy(&value, p, sizeof(value));
    return value;
}

enum { MESSAGE_PAGE_TOKENS=96 };

typedef enum { MESSAGE_SCENERY_CHOICE, MESSAGE_TIMED_CAPTION, MESSAGE_ROOM_TEXT } message_page_kind;

static float glyph_advance(const gh_font_glyph *glyph, float scale)
{
    const volatile float width=(float)glyph->width*scale*0.9f;
    return width*0.9f;
}

static gh_scene_ui_status message_page(const gh_scene_message_block *block,
    const gh_scene_message_sources *source,const gh_font_state *font,const gh_display_state *display,
    gh_id_view palette,gh_scene_message_emit emit,void *context,
    message_page_kind kind,uint32_t *frontier)
{
    const uint8_t *p=block->payload;
    *frontier=0x2af728;
    const uint8_t *text=source->primary.bytes;
    const size_t start=source->cursor_7c;
    const int timed_caption=kind==MESSAGE_TIMED_CAPTION;
    const int room_text=kind==MESSAGE_ROOM_TEXT;
    if(!source->bound||source->ruby.bytes||source->ruby.size||!text||source->cursor_8c<start||
       start>=source->primary.size||source->primary.size-start<2)
        return GH_SCENE_UI_UNSUPPORTED;
    if(timed_caption&&(source->primary.size<14||start!=10||half(text)!=0xe705))
        return GH_SCENE_UI_UNSUPPORTED;
    if(room_text&&start&&(start<2||half(text+start-2)!=0xa101))
        return GH_SCENE_UI_UNSUPPORTED;
    if(kind==MESSAGE_SCENERY_CHOICE&&
       (source->primary.size<46||(start!=4&&start!=46)||
        half(text)!=0xa602||half(text+2)!=0||half(text+44)!=0xa001))
        return GH_SCENE_UI_UNSUPPORTED;
    if(!font||!display||!display->initialized||!palette.bytes||palette.size<84)
        return GH_SCENE_UI_ARGUMENT;
    if(!emit)return GH_SCENE_UI_UNSUPPORTED;
    float original_height;
    memcpy(&original_height,&display->screen_height_bits,4);
    if(original_height!=(float)display->screen_lines)return GH_SCENE_UI_UNSUPPORTED;

    struct {
        uint16_t code;
        gh_font_glyph metrics;
        uint32_t time;
        float scale;
    } tokens[MESSAGE_PAGE_TOKENS];
    size_t token_count=0,visible_count=0,offset=start;
    const int32_t elapsed=signed_word(p+0x68-0xc),start_time=signed_word(p+0x84-0xc);
    uint32_t time=0;
    unsigned choice_count=0;
    float scale=1.0f, peak_scale=0.0f;
    const unsigned selected=(word(p+0x14-0xc)&0x20u)?half(p+0x94-0xc):UINT16_MAX;
    while(offset+2<=source->primary.size){
        const uint16_t code=half(text+offset);
        if(room_text&&code==0x8001)break;
        if(room_text&&code==0xa301){
            if(source->primary.size-offset<4)return GH_SCENE_UI_BOUNDS;
            const uint16_t next=half(text+offset+2);
            if(next!=0x8001&&next!=0xa101)return GH_SCENE_UI_UNSUPPORTED;
            offset+=2;
            break;
        }
        if(!room_text&&code==(start==4?0xa001:0x8001))break;
        if((code==0xa301&&start==46)||(code==0xe801&&timed_caption)){
            offset+=2;
            if(offset+2>source->primary.size||half(text+offset)!=0x8001)
                return GH_SCENE_UI_UNSUPPORTED;
            break;
        }
        if(code==0xb002&&timed_caption){
            if(offset+4>source->primary.size)return GH_SCENE_UI_BOUNDS;
            /* 2B04C0 affects both the measured line and emitted glyphs. */
            const unsigned percent=half(text+offset+2);
            if(percent!=100&&percent!=150)return GH_SCENE_UI_UNSUPPORTED;
            scale=(float)percent/100.0f;
            offset+=4;
            continue;
        }
        if(token_count==MESSAGE_PAGE_TOKENS)return GH_SCENE_UI_UNSUPPORTED;
        const int visible=offset<=source->cursor_8c&&(int64_t)start_time+time<=elapsed;
        tokens[token_count].code=code;
        tokens[token_count].time=time;
        tokens[token_count].scale=scale;
        if(code==0xa804&&start==46){
            if(offset+8>source->primary.size||choice_count>=2||
               half(text+offset+2)!=choice_count||
               half(text+offset+4)!=UINT16_MAX||half(text+offset+6)!=UINT16_MAX)
                return GH_SCENE_UI_UNSUPPORTED;
            if(visible&&selected==choice_count){
                *frontier=0x2b00a0;
                return GH_SCENE_UI_UNSUPPORTED;
            }
            ++choice_count;
            ++token_count;
            offset+=8;
            continue;
        }
        if(code<14||code>=0x1000||gh_font_glyph_at(font,code,&tokens[token_count].metrics)||
           tokens[token_count].metrics.width<=0||tokens[token_count].metrics.baseline_delta<=0){
            *frontier=0x2a4b20;
            return GH_SCENE_UI_UNSUPPORTED;
        }
        if(scale>peak_scale)peak_scale=scale;
        visible_count+=visible;
        ++token_count;
        time+=30;
        offset+=2;
    }
    if(offset+2>source->primary.size||source->cursor_8c>offset||
       (kind==MESSAGE_SCENERY_CHOICE?(start==4?token_count!=20:choice_count!=2):token_count==0))
        return GH_SCENE_UI_UNSUPPORTED;
    if(!visible_count){*frontier=0;return GH_SCENE_UI_OK;}

    fenv_t saved;
    if(feholdexcept(&saved))return GH_SCENE_UI_UNSUPPORTED;
    if(fesetround(FE_TONEAREST)){
        (void)fesetenv(&saved);
        return GH_SCENE_UI_UNSUPPORTED;
    }
    const volatile float spacing=2.0f*0.9f,baseline=12.0f*1.1f;
    volatile float line_width=0;
    const volatile float choice_width=(0.9f*0.9f)*40.0f;
    for(size_t i=0;i<token_count;i++){
        if(tokens[i].code==0xa804){
            line_width=line_width+choice_width;
        }else{
            line_width=line_width+spacing;
            line_width=line_width+glyph_advance(&tokens[i].metrics,tokens[i].scale);
        }
    }
    line_width=line_width-spacing;
    const gh_font_slot *atlas=font->slots;
    const volatile float line_height=(float)atlas->cell_height*peak_scale*1.1f+baseline;
    volatile float x=(float)half(p+0xae - 0x0c)-line_width*0.5f;
    const volatile float y=((float)half(p+0xb0-0x0c)-line_height*0.5f)+baseline;
    gh_font_draw_state draw={.scale_x=0.9f*0.9f,.scale_y=1.1f,.alignment_flags=9,
        .screen_width=display->screen_width,.screen_height=display->screen_lines,
        .gs_y_origin=display->gs_y_origin};
    gh_id_text_glyph normal[MESSAGE_PAGE_TOKENS],shadow[MESSAGE_PAGE_TOKENS];
    int valid=1;
    size_t count=0;
    for(size_t token=0;token<token_count;token++){
        if((int64_t)start_time+tokens[token].time>elapsed)break;
        const uint16_t code=tokens[token].code;
        if(code==0xa804){
            x=x+(choice_width+spacing);
            continue;
        }
        if(count==visible_count)break;
        const size_t i=count++;
        draw.scale_x=tokens[token].scale*0.9f*0.9f;
        draw.scale_y=tokens[token].scale*1.1f;
        const float glyph_y=y+((peak_scale*1.1f)*32.0f-draw.scale_y*32.0f);
        normal[i]=(gh_id_text_glyph){.code=code,.x=x,.y=glyph_y};
        shadow[i]=(gh_id_text_glyph){.code=code,.x=x+2,.y=glyph_y+2};
        draw.rgba=word(palette.bytes+80);
        if(gh_font_draw_quad(font,&draw,code,normal[i].x,normal[i].y,65535,&normal[i].quad)){
            valid=0;break;
        }
        draw.rgba=0xff000000u;
        if(gh_font_draw_quad(font,&draw,code,shadow[i].x,shadow[i].y,65535,&shadow[i].quad)){
            valid=0;break;
        }
        const volatile float advance=glyph_advance(&tokens[token].metrics,tokens[token].scale)+spacing;
        x=x+advance;
    }
    *frontier=0x2a53a8;
    if(fesetenv(&saved)||!valid)return GH_SCENE_UI_UNSUPPORTED;
    const gh_scene_message_glyph_pair batch={.font=atlas,.bank=6,.depth=2,.font_mode=0x21,
        .normal_run=normal,.shadow_run=shadow,.count=count};
    uint32_t child=0;
    gh_scene_ui_status status=emit(context,&batch,&child);
    *frontier=status?(child?child:0x2a5930):0;
    return status;
}

gh_scene_ui_status gh_scene_message_render_002af028(gh_game_scene_ui *ui,
    const gh_boot_work *work, const gh_font_state *font, const gh_display_state *display,
    gh_id_view palette, gh_scene_message_emit emit, void *context, uint32_t *frontier)
{
    if (!frontier) return GH_SCENE_UI_ARGUMENT;
    gh_scene_ui_status status = gh_scene_message_prepare_draw_002af028(ui, work, frontier);
    if (status != GH_SCENE_UI_UNSUPPORTED || *frontier != 0x2b1c80) return status;

    const unsigned slot = (unsigned)ui->active_head;
    const gh_scene_message_block *block = ui->blocks + slot;
    const gh_scene_message_sources *source = ui->sources + slot;
    const uint8_t *p = block->payload;
    if(work->language && work->room==0x105 && block->next==-1 && p[0xa4-0xc]==4 &&
       half(p+0x64-0xc)==0x3004)
        return message_page(block,source,font,display,palette,emit,context,MESSAGE_SCENERY_CHOICE,frontier);
    if(work->language && work->room==0x102 && block->next==-1 && !p[0xa4-0xc] &&
       source->primary.bytes && source->primary.size>=2 && half(source->primary.bytes)==0xe705)
        return message_page(block,source,font,display,palette,emit,context,MESSAGE_TIMED_CAPTION,frontier);
    const uint16_t id=half(p+0x64-0xc);
    if(work->language&&work->room==0x103&&block->next==-1&&!p[0xa4-0xc]&&(id==0x1000||id==0x1001))
        return message_page(block,source,font,display,palette,emit,context,MESSAGE_ROOM_TEXT,frontier);
    if (!work->language || block->next != -1 || p[0xa4 - 0xc] || !source->bound ||
        source->ruby.bytes || source->ruby.size || !source->primary.bytes ||
        source->primary.size < 6 || source->cursor_7c || source->cursor_8c > 4)
        return GH_SCENE_UI_UNSUPPORTED;
    const uint8_t *text = source->primary.bytes;
    const uint16_t code = half(text);
    if (code < 14 || code >= 0x1000 || half(text + 2) != 0xa301 || half(text + 4) != 0x8001)
        return GH_SCENE_UI_UNSUPPORTED;
    if (signed_word(p + 0x84 - 0xc) > signed_word(p + 0x68 - 0xc)) {
        *frontier = 0;
        return GH_SCENE_UI_OK;
    }

    *frontier = 0x2b0480;
    if (!font || !display || !display->initialized || !palette.bytes || palette.size < 84)
        return GH_SCENE_UI_ARGUMENT;
    float original_height;
    memcpy(&original_height, &display->screen_height_bits, sizeof(original_height));
    if (original_height != (float)display->screen_lines) return GH_SCENE_UI_UNSUPPORTED;
    gh_font_glyph glyph;
    *frontier = 0x2a4b20;
    if (gh_font_glyph_at(font, code, &glyph) || glyph.width <= 0 || glyph.baseline_delta <= 0)
        return GH_SCENE_UI_UNSUPPORTED;
    const gh_font_slot *atlas = font->slots;
    /* Original English 2AF688/2AF698, with no scaling command before
     * the first glyph. 2B0700 adds and subtracts its trailing spacing. */
    const volatile float advance = ((float)glyph.width * 0.9f) * 0.9f;
    const volatile float spacing = 2.0f * 0.9f;
    const volatile float line_width = (spacing + advance) - spacing;
    const volatile float baseline = 12.0f * 1.1f;
    const float line_height = (float)atlas->cell_height * 1.1f + baseline;
    const float x = (float)half(p + 0xae - 0xc) - line_width * 0.5f;
    const float y = ((float)half(p + 0xb0 - 0xc) - line_height * 0.5f) + baseline;
    gh_font_draw_state draw = {.scale_x = 0.9f * 0.9f, .scale_y = 1.1f, .alignment_flags = 9,
        .screen_width = display->screen_width, .screen_height = display->screen_lines,
        .gs_y_origin = display->gs_y_origin, .rgba = word(palette.bytes + 80)};
    gh_scene_message_glyph_pair pair = {.font = atlas, .bank = 6, .depth = 2, .font_mode = 0x21,
        .normal = {.code = code, .x = x, .y = y},
        .shadow = {.code = code, .x = x + 2.0f, .y = y + 2.0f}};
    *frontier = 0x2a53a8;
    if (gh_font_draw_quad(font, &draw, code, pair.normal.x, pair.normal.y, 65535.0f, &pair.normal.quad))
        return GH_SCENE_UI_UNSUPPORTED;
    draw.rgba = 0xff000000u;
    if (gh_font_draw_quad(font, &draw, code, pair.shadow.x, pair.shadow.y, 65535.0f, &pair.shadow.quad))
        return GH_SCENE_UI_UNSUPPORTED;
    *frontier = 0x2a5930;
    if (!emit) return GH_SCENE_UI_UNSUPPORTED;
    uint32_t child = 0;
    status = emit(context, &pair, &child);
    *frontier = status ? (child ? child : 0x2a5930) : 0;
    return status;
}

static unsigned channel(const uint8_t *atlas, unsigned x, unsigned y, unsigned component)
{
    const unsigned value = atlas[4u * ((y & 255u) * 512u + (x & 511u)) + component];
    return component == 3 ? (value == 255 ? 128 : value / 2) : value;
}

static unsigned sample(const uint8_t *atlas, uint32_t u, uint32_t v, unsigned component)
{
    /* TEX1=0x60 selects linear. Shift the fixed16 coordinates by half a
     * texel before extracting the repeat index and native 4-bit weights. */
    u -= 32768u;
    v -= 32768u;
    const unsigned x = u >> 16, y = v >> 16;
    const unsigned fx = (u >> 12) & 15u, fy = (v >> 12) & 15u;
    const unsigned a = (channel(atlas, x, y, component) * (16u - fx) +
                        channel(atlas, x + 1, y, component) * fx) >> 4;
    const unsigned b = (channel(atlas, x, y + 1, component) * (16u - fx) +
                        channel(atlas, x + 1, y + 1, component) * fx) >> 4;
    return (a * (16u - fy) + b * fy) >> 4;
}

static gh_scene_ui_status raster_glyph(const gh_id_text_glyph *glyph,
    const uint8_t *atlas, size_t bytes, uint8_t *rgba, uint32_t *depth, int *discarded)
{
    if (!glyph || !atlas || !discarded) return GH_SCENE_UI_ARGUMENT;
    if (bytes < 512u * 256u * 4u) return GH_SCENE_UI_BOUNDS;
    const gh_font_quad *q = &glyph->quad;
    if (q->slot || q->palette_parity != (glyph->code & 1) ||
        q->x0 >= q->x1 || q->y0 >= q->y1 || q->u0 > q->u1 || q->v0 > q->v1 ||
        q->u1 > 512u * 16u + 8u || q->v1 > 256u * 16u + 8u || q->z != 65535)
        return GH_SCENE_UI_UNSUPPORTED;

    const int left = ((int)q->x0 - 28672 + 15) / 16;
    const int right = ((int)q->x1 - 28672 + 15) / 16;
    const int top = ((int)q->y0 - 29184 + 15) / 16;
    const int bottom = ((int)q->y1 - 29184 + 15) / 16;
    const uint32_t du = (uint32_t)(q->u1 - q->u0) << 12;
    const uint32_t dv = (uint32_t)(q->v1 - q->v0) << 12;
    const unsigned tint_alpha = q->rgba >> 24;
    *discarded = 1;
    for (int y = top < 0 ? 0 : top; y < bottom && y < 448; ++y) {
        const unsigned offset_y = (unsigned)(29184 + y * 16 - q->y0);
        const uint32_t v = ((uint32_t)q->v0 << 12) +
            (uint32_t)((uint64_t)dv * offset_y / (q->y1 - q->y0));
        for (int x = left < 0 ? 0 : left; x < right && x < 512; ++x) {
            const unsigned offset_x = (unsigned)(28672 + x * 16 - q->x0);
            const uint32_t u = ((uint32_t)q->u0 << 12) +
                (uint32_t)((uint64_t)du * offset_x / (q->x1 - q->x0));
            unsigned alpha = (sample(atlas, u, v, 3) * tint_alpha) >> 7;
            if (alpha <= 1) continue; /* TEST3001D: GREATER 1, AFAIL KEEP. */
            *discarded = 0;
            if (!rgba) return GH_SCENE_UI_OK;
            if (alpha > 255) alpha = 255;
            const size_t pixel = (size_t)y * 512u + (unsigned)x;
            uint8_t *destination = rgba + pixel * 4u;
            for (unsigned k = 0; k < 3; ++k) {
                const unsigned tint = (q->rgba >> (8u * k)) & 255u;
                unsigned source = sample(atlas, u, v, k) * tint >> 7;
                if (source > 255) source = 255;
                const int numerator = (int)(destination[k] * 128u) +
                    ((int)source - destination[k]) * (int)alpha;
                destination[k] = numerator <= 0 ? 0 : numerator >= 255 * 128 ? 255 : (uint8_t)(numerator / 128);
            }
            destination[3] = (uint8_t)alpha;
            if (depth) depth[pixel] = q->z;
        }
    }
    return GH_SCENE_UI_OK;
}

gh_scene_ui_status gh_scene_message_glyph_discarded(const gh_id_text_glyph *glyph,
    const uint8_t *atlas, size_t bytes, int *discarded)
{
    return raster_glyph(glyph, atlas, bytes, NULL, NULL, discarded);
}

gh_scene_ui_status gh_scene_message_glyph_blit(const gh_id_text_glyph *glyph,
    const uint8_t *atlas, size_t atlas_bytes, uint8_t *rgba, size_t rgba_bytes,
    uint32_t *depth, size_t depth_count)
{
    if (!rgba || (!depth && depth_count)) return GH_SCENE_UI_ARGUMENT;
    if (rgba_bytes < 512u * 448u * 4u || (depth && depth_count < 512u * 448u))
        return GH_SCENE_UI_BOUNDS;
    int discarded;
    return raster_glyph(glyph, atlas, atlas_bytes, rgba, depth, &discarded);
}
