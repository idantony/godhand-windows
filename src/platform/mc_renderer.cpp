#include "mc_renderer.h"
#include "ui_text_renderer.h"
#include "ui_sprite_renderer.h"
#include "ui_polygon_renderer.h"
#include "../core/id_number.h"
#include "../assets/tim2.h"
#include "../assets/font_texture.h"
#include <algorithm>
#include <cstring>
#include <new>
#include <vector>
#include <cmath>

struct CachedTexture {const unsigned char *source;uint32_t handle;};
struct QueuedID {
    gh_id_render_view view;
    gh_id_base base;
    gh_id_entry entry;
    uint32_t entry_index,consumer;
    uint16_t z=0;bool z_known=false;
};
enum class CommandKind {ID,FAN,FADE,NUMBER,COVER};
struct QueuedCommand {
    CommandKind kind=CommandKind::ID;
    QueuedID id={};gh_cockpit_fan fan={};gh_fade_draw fade={};gh_ui_fade_context fade_context={};gh_id_texture_quad number={};gh_child_cover_quad cover={};
};
struct gh_mc_renderer {
    gh_ui_text_renderer *text=nullptr;
    gh_ui_sprite_renderer *sprite=nullptr;
    gh_ui_polygon_renderer *polygon=nullptr;
    gh_ui_fade_renderer *fade=nullptr;
    const unsigned char *font_mst=nullptr,*font_msa=nullptr;
    std::vector<unsigned char> palette,frame,next;
    std::vector<gh_id_text_glyph> glyphs;
    std::vector<CachedTexture> textures;
    std::vector<CachedTexture> polygon_textures;
    std::vector<CachedTexture> fan_textures;
    std::vector<CachedTexture> number_textures;
    std::vector<QueuedCommand> queued;
    gh_mc_render_status status={};
    int32_t language=1;
    bool begun=false;
    bool reference_polygons=false;
    bool flushing=false,queue_failed=false;
};
static bool null_texture(const gh_id_render_view*s,const gh_id_entry*e)
{return e->kind==0&&!e->texture.bytes&&(e->b8a==33||(e->b8a<32&&!s->manager->pair[e->b8a].texture_data.bytes));}
static gh_id_error render_number(gh_mc_renderer*r,const gh_id_texture_quad&q,gh_ui_error*gpu)
{
    if(!r->polygon)*gpu=gh_ui_polygon_create(512,448,&r->polygon);gh_ui_polygon_item p={};
    for(const auto&t:r->number_textures)if(t.source==q.texture.source.bytes){p.texture=t.handle;break;}
    if(*gpu==GH_UI_OK&&!p.texture){std::vector<unsigned char>rgba(128*128*4);gh_id_error e=gh_id_number_texture_rgba(&q.texture,rgba.data(),rgba.size());if(e)return e;
        *gpu=gh_ui_polygon_upload_number128(r->polygon,&q.texture,rgba.data(),rgba.size(),&p.texture);if(*gpu==GH_UI_OK)r->number_textures.push_back({q.texture.source.bytes,p.texture});}
    if(*gpu==GH_UI_OK&&(q.x0==q.x1||q.y0==q.y1)){r->next=r->frame;return GH_ID_OK;} /* genuine zero-area raster */
    if(*gpu==GH_UI_OK){auto&v=p.polygon;v.texture=q.texture;v.color=q.color;v.color_mode=q.color_mode;v.font_mode=q.font_mode;v.clip=q.clip;v.alignment=0;
        const float x[]={q.x0,q.x1,q.x0,q.x1},y[]={q.y0,q.y0,q.y1,q.y1},u[]={q.u0,q.u1,q.u0,q.u1},t[]={q.v0,q.v0,q.v1,q.v1};
        std::memcpy(v.x,x,sizeof(x));std::memcpy(v.y,y,sizeof(y));std::memcpy(v.u,u,sizeof(u));std::memcpy(v.v,t,sizeof(t));
        gh_ui_sprite_state state={UINT64_C(0x8000000044),0x3001d,0x54,0x60,0,1,1,0,0,0};*gpu=gh_ui_polygon_batch(r->polygon,&state,&p,1,r->frame.data(),r->next.data(),r->next.size());}
    return *gpu==GH_UI_OK?GH_ID_OK:GH_ID_UNSUPPORTED;
}
static gh_id_error zero_texture(gh_mc_renderer*r,const gh_id_render_view*s,const gh_id_entry*e)
{
    /* Current canonical finite geometry only. No inference from guest heap
     * initialization and no substitute visibility/color policy. */
    if(e->kind||e->f80!=1||e->texture.bytes||(e->b89!=0&&e->b89!=3)||e->draw_rotation!=0||
       !std::isfinite(e->f68)||!std::isfinite(e->f6c)||!std::isfinite(e->draw_scale_x)||!std::isfinite(e->draw_scale_y)||!std::isfinite(e->draw_x)||!std::isfinite(e->draw_y))return GH_ID_UNSUPPORTED;
    float w=e->f68*e->draw_scale_x,h=e->f6c*e->draw_scale_y;if(!std::isfinite(w)||!std::isfinite(h)||(w!=0&&h!=0))return GH_ID_UNSUPPORTED;
    if((s->base->mode==6?e->b8d:s->base->mode)!=6||e->h8e!=5)return GH_ID_UNSUPPORTED;
    /* CLAMP0 permits repeat coordinates outside0..1; no sample is taken for
     * zero coverage, but nonfinite serialized ST remains unsupported. */
    for(float uv:e->f70)if(!std::isfinite(uv))return GH_ID_UNSUPPORTED;
    if(e->b8a>=32)return GH_ID_BOUNDS;
    const auto&table=s->manager->pair[e->b8a].texture_data;
    auto word=[](const unsigned char*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;};
    if(table.bytes&&table.size>=4){uint32_t count=word(table.bytes),index=(uint32_t)e->b8b+1;
        if(count>(table.size-4)/4||index>count)return GH_ID_BOUNDS;uint32_t offset=word(table.bytes+index*4);size_t end=table.size;
        if(offset<4+(size_t)count*4||offset>=end)return GH_ID_BOUNDS;
        for(uint32_t i=1;i<=count;i++){uint32_t next=word(table.bytes+i*4);if(next&&(next<4+(size_t)count*4||next>table.size))return GH_ID_BOUNDS;if(next>offset&&next<end)end=next;}
        if(end-offset>=4&&!std::memcmp(table.bytes+offset,"TIM2",4)){gh_tim2_picture p={};gh_tim2_error te=gh_tim2_first(table.bytes+offset,end-offset,&p);
            if(te)return te==GH_TIM2_BOUNDS?GH_ID_BOUNDS:GH_ID_UNSUPPORTED;
            unsigned psm=(unsigned)((p.tex0>>20)&63),tw=(unsigned)((p.tex0>>26)&15),th=(unsigned)((p.tex0>>30)&15);
            if(p.clut_type!=3||((p.tex0>>35)&3)||((p.tex0>>51)&63)||((p.tex0>>56)&31)||tw>9||th>9||p.width!=(1u<<tw)||p.height!=(1u<<th)||
               !((p.image_type==4&&psm==20&&p.colors==16&&p.image_size==(size_t)p.width*p.height/2)||(p.image_type==5&&psm==19&&p.colors==256&&p.image_size==(size_t)p.width*p.height)))return GH_ID_UNSUPPORTED;
            for(unsigned i=0;i<p.colors;i++)if(p.clut[i*4+3]>128)return GH_ID_UNSUPPORTED;
            /* Header/upload validated; zero coverage samples no CLUT entry.
             * This is NOT an implementation of positive-area TIM2/T8 decode. */
            r->next=r->frame;return GH_ID_OK;
        }
        if(end-offset>=48&&!(word(table.bytes+offset)&0x80000000u)){
            const unsigned char*bytes=table.bytes+offset;uint64_t tex0=(uint64_t)word(bytes+32)|(uint64_t)word(bytes+36)<<32;
            unsigned psm=(unsigned)((tex0>>20)&63),tw=(unsigned)((tex0>>26)&15),th=(unsigned)((tex0>>30)&15);
            if((psm!=19&&psm!=20)||tw>9||th>9||((tex0>>35)&3)||((tex0>>51)&63)||((tex0>>56)&31))return GH_ID_UNSUPPORTED;
            gh_font_slot packet={};packet.bound=1;packet.mst=bytes;packet.mst_size=end-offset;
            for(unsigned i=0;i<2;i++){packet.table_offset[i]=word(bytes+8+i*4);packet.table_count[i]=(uint16_t)(bytes[16+i*2]|(unsigned)bytes[17+i*2]<<8);if(packet.table_count[i]!=1)return GH_ID_UNSUPPORTED;}
            gh_font_transfer image={},clut={};if(gh_font_transfer_at(&packet,0,0,&image)||gh_font_transfer_at(&packet,1,0,&clut))return GH_ID_UNSUPPORTED;
            size_t expected=((size_t)1<<tw)*((size_t)1<<th)/(psm==20?2:1);
            if(image.width!=64||image.buffer_width!=1||image.destination_base||image.pixel_bytes!=expected||clut.width!=16||clut.height!=16||clut.destination_base)return GH_ID_UNSUPPORTED;
            for(unsigned i=0;i<256;i++)if(clut.pixels[i*4+3]>128)return GH_ID_UNSUPPORTED;
            r->next=r->frame;return GH_ID_OK; /* validated CT32 upload, no sampler needed */
        }
    }
    gh_id_texture t={};bool number=false;gh_id_error error=gh_id_polygon_texture_open(s->manager,e->b8a,(uint32_t)e->b8b+1,&t);
    if(error==GH_ID_UNSUPPORTED){error=gh_id_number_texture_open(s->manager,e->b8a,(uint32_t)e->b8b+1,&t);number=error==GH_ID_OK;}
    if(error)return error;
    std::vector<unsigned char>pixels((size_t)t.width*t.height*4);error=number?gh_id_number_texture_rgba(&t,pixels.data(),pixels.size()):gh_id_polygon_texture_rgba(&t,pixels.data(),pixels.size());if(error)return error;
    r->next=r->frame;return GH_ID_OK; /* GS zero-area strip has no color/Z samples. */
}
extern "C" gh_id_error gh_mc_renderer_create(const void *elf,size_t bytes,int32_t language,gh_mc_renderer **out)
{
    if(!out||language<0||language>6)return GH_ID_ARGUMENT;
    *out=nullptr;gh_id_view palette={};
    gh_id_error e=gh_id_text_palette_from_elf(elf,bytes,&palette);
    if(e!=GH_ID_OK)return e;
    try {
        auto *r=new gh_mc_renderer;
        try {
            r->palette.assign(palette.bytes,palette.bytes+palette.size);r->language=language;
            r->frame.resize(512*448*4);r->next.resize(r->frame.size());r->glyphs.resize(8192);
        }catch(...){delete r;throw;}
        *out=r;return GH_ID_OK;
    }catch(const std::bad_alloc&){return GH_ID_MEMORY;}
}
extern "C" void gh_mc_renderer_reset_resources(gh_mc_renderer *r)
{
    if(!r)return;
    gh_ui_text_destroy(r->text);r->text=nullptr;
    gh_ui_sprite_destroy(r->sprite);r->sprite=nullptr;
    gh_ui_polygon_destroy(r->polygon);r->polygon=nullptr;
    gh_ui_fade_destroy(r->fade);r->fade=nullptr;
    r->font_mst=r->font_msa=nullptr;r->textures.clear();r->polygon_textures.clear();r->fan_textures.clear();r->number_textures.clear();
}
extern "C" gh_id_error gh_mc_renderer_enable_reference_polygons(gh_mc_renderer *r,int enabled)
{if(!r||(enabled!=0&&enabled!=1))return GH_ID_ARGUMENT;if(r->begun)return GH_ID_BUSY;r->reference_polygons=enabled!=0;return GH_ID_OK;}
extern "C" void gh_mc_renderer_destroy(gh_mc_renderer *r)
{if(r){gh_mc_renderer_reset_resources(r);delete r;}}
extern "C" void gh_mc_renderer_begin(gh_mc_renderer *r)
{
    if(!r)return;
    std::fill(r->frame.begin(),r->frame.end(),0);
    for(size_t i=3;i<r->frame.size();i+=4)r->frame[i]=128;
    r->status={};r->begun=true;r->queued.clear();r->queue_failed=false;
}
static gh_id_error draw_view(gh_mc_renderer *r,const gh_id_render_view *s,const gh_id_draw_job *job,const gh_ui_depth_write *depth)
{
    if(!r||!r->begun||!s||!job||!job->entry||!s->base||!s->manager)return GH_ID_ARGUMENT;
    if(!r->flushing&&!r->queued.empty())return GH_ID_BUSY;
    gh_id_error error=GH_ID_UNSUPPORTED;gh_ui_error gpu=GH_UI_OK;
    r->status.consumer=job->consumer_address;r->status.opcode=0;r->status.frontier=0;
    if(null_texture(s,job->entry)){r->status.id_error=GH_ID_OK;r->status.gpu_error=0;return GH_ID_OK;}
    try {
        if(job->entry->kind==2){
            if(!s->font||!s->menu||!s->messages)return GH_ID_ARGUMENT;
            gh_id_text_result result={};
            gh_id_text_context context={r->language,512,448,1824,{r->palette.data(),r->palette.size(),0}};
            error=gh_id_text_prepare(s->base,job->entry,s->menu,s->messages,s->font,&context,r->glyphs.data(),r->glyphs.size(),&result);
            r->status.opcode=result.opcode;r->status.frontier=result.frontier_address;
            if(error==GH_ID_OK){
                const auto &font=s->font->slots[0];
                if(r->text&&(r->font_mst!=font.mst||r->font_msa!=font.msa))gh_mc_renderer_reset_resources(r);
                if(!r->text){gpu=gh_ui_text_create(&font,512,448,&r->text);if(gpu==GH_UI_OK){r->font_mst=font.mst;r->font_msa=font.msa;}}
                if(gpu==GH_UI_OK){
                    gh_ui_gs_state state={};state.alpha=UINT64_C(0x8000000044);state.test=0x3001d;
                    state.tex0=UINT64_C(0x2000000625420000);state.prim=0x156;state.colclamp=state.prmodecont=1;
                    gpu=gh_ui_text_batch(r->text,&state,28672,29184,r->glyphs.data(),result.glyph_count,r->frame.data(),r->next.data(),r->next.size());
                    if(gpu==GH_UI_OK)r->status.glyphs+=result.glyph_count;
                }
            }
        }else if(job->entry->kind==1){
            gh_id_texture_quad q[10];size_t n=0;error=gh_id_number_prepare(s->manager,s->base,job->entry,q,10,&n,&r->status.frontier);
            if(error==GH_ID_OK&&n!=1){error=GH_ID_UNSUPPORTED;r->status.frontier=0x2c70a8;}
            if(error==GH_ID_OK)error=render_number(r,q[0],&gpu);
        }else if(job->entry->kind==0){
            gh_ui_sprite_item item={};error=gh_id_texture_quad_prepare(s->manager,s->base,job->entry,&item.quad);
            if(error==GH_ID_OK){
                if(!r->sprite)gpu=gh_ui_sprite_create(512,448,&r->sprite);
                bool found=false;
                for(const auto &t:r->textures)if(t.source==item.quad.texture.source.bytes){item.texture=t.handle;found=true;break;}
                if(gpu==GH_UI_OK&&!found){
                    std::vector<unsigned char> rgba((size_t)item.quad.texture.width*item.quad.texture.height*4);
                    error=gh_id_texture_rgba(&item.quad.texture,rgba.data(),rgba.size());
                    if(error==GH_ID_OK){gpu=gh_ui_sprite_upload(r->sprite,&item.quad.texture,rgba.data(),rgba.size(),&item.texture);
                        if(gpu==GH_UI_OK)r->textures.push_back({item.quad.texture.source.bytes,item.texture});}
                }
                if(error==GH_ID_OK&&gpu==GH_UI_OK){
                    gh_ui_sprite_state state={};state.alpha=UINT64_C(0x8000000044);state.test=0x3001d;
                    state.prim=0x54;state.tex1=0x60;state.colclamp=state.prmodecont=1;
                    gpu=gh_ui_sprite_batch_with_depth(r->sprite,&state,&item,1,r->frame.data(),r->next.data(),r->next.size(),depth);
                }
            }else if(error==GH_ID_UNSUPPORTED&&r->reference_polygons){
                gh_ui_polygon_item polygon={};
                error=gh_id_texture_polygon_prepare(s->manager,s->base,job->entry,&polygon.polygon);
                if(error==GH_ID_OK){
                    if(!r->polygon)gpu=gh_ui_polygon_create(512,448,&r->polygon);
                    bool found=false;
                    for(const auto &t:r->polygon_textures)if(t.source==polygon.polygon.texture.source.bytes){polygon.texture=t.handle;found=true;break;}
                    if(gpu==GH_UI_OK&&!found){
                        std::vector<unsigned char> rgba((size_t)polygon.polygon.texture.width*polygon.polygon.texture.height*4);
                        error=gh_id_polygon_texture_rgba(&polygon.polygon.texture,rgba.data(),rgba.size());
                        if(error==GH_ID_OK){gpu=gh_ui_polygon_upload(r->polygon,&polygon.polygon.texture,rgba.data(),rgba.size(),&polygon.texture);
                            if(gpu==GH_UI_OK)r->polygon_textures.push_back({polygon.polygon.texture.source.bytes,polygon.texture});}
                    }
                    if(error==GH_ID_OK&&gpu==GH_UI_OK){
                        gh_ui_sprite_state state={};state.alpha=UINT64_C(0x8000000044);state.test=0x3001d;
                        state.prim=0x54;state.tex1=0x60;state.colclamp=state.prmodecont=1;
                        gpu=gh_ui_polygon_batch_with_depth(r->polygon,&state,&polygon,1,r->frame.data(),r->next.data(),r->next.size(),depth);
                    }
                }
            }
        }
        if(error==GH_ID_UNSUPPORTED&&gpu==GH_UI_OK&&job->entry->kind==0)error=zero_texture(r,s,job->entry);
        if(gpu!=GH_UI_OK)error=GH_ID_UNSUPPORTED;
        if(error==GH_ID_OK){r->frame.swap(r->next);++r->status.jobs;}
    }catch(const std::bad_alloc&){error=GH_ID_MEMORY;}
    r->status.id_error=error;r->status.gpu_error=(unsigned)gpu;if(error!=GH_ID_OK&&!r->status.frontier)r->status.frontier=job->consumer_address;
#if defined(GH_NATIVE_STARTUP_QA)
    if(error!=GH_ID_OK){const auto&e=*job->entry;std::fprintf(stderr,"QA ID render error%d gpu%u base%d resource%d entry%u source%u kind%u consumer%08X frontier%08X slot%u item%u align%u rot%.9g f80%.9g xy%.9g,%.9g scale%.9g,%.9g dim%.9g,%.9g UV%.9g,%.9g,%.9g,%.9g color%08X bank%u depth%d overrideBytes%zu\n",(int)error,(unsigned)gpu,s->base->id,s->base->resource_number,job->entry_index,e.source_index,e.kind,job->consumer_address,r->status.frontier,e.b8a,(unsigned)e.b8b+1,e.b89,(double)e.draw_rotation,(double)e.f80,(double)e.draw_x,(double)e.draw_y,(double)e.draw_scale_x,(double)e.draw_scale_y,(double)e.f68,(double)e.f6c,(double)e.f70[0],(double)e.f70[1],(double)e.f70[2],(double)e.f70[3],e.color,e.b8d,e.h8e,e.texture.size);}
#endif
    return error;
}
extern "C" gh_id_error gh_mc_renderer_draw_view(gh_mc_renderer *r,const gh_id_render_view *s,const gh_id_draw_job *job)
{return draw_view(r,s,job,nullptr);}
extern "C" gh_id_error gh_mc_renderer_queue_view(gh_mc_renderer *r,const gh_id_render_view *view,const gh_id_draw_job *job,gh_ui_ot_tag *tag)
{
    if(!r||!r->begun||!view||!view->base||!view->manager||!job||!job->entry||!tag)return GH_ID_ARGUMENT;
    if(r->flushing||r->queue_failed||r->status.jobs)return GH_ID_BUSY;
    if(null_texture(view,job->entry)){*tag={0,0,SIZE_MAX};return GH_ID_OK;}
    uint32_t bank;int32_t depth;
    if(job->entry->kind==2){bank=6;depth=2;}
    else if(job->entry->kind==0||job->entry->kind==1){bank=view->base->mode==6?job->entry->b8d:view->base->mode;depth=job->entry->h8e;}
    else return GH_ID_UNSUPPORTED;
    if(r->queued.size()>=8192)return GH_ID_CAPACITY;
    try {
        QueuedCommand command;command.id={*view,*view->base,*job->entry,job->entry_index,job->consumer_address};
        if(job->entry->kind==1){gh_id_texture_quad digits[10];size_t n=0;gh_id_error e=gh_id_number_prepare(view->manager,view->base,job->entry,digits,10,&n,&r->status.frontier);if(e)return e;
            if(n!=1){r->status.frontier=0x2c70a8;return GH_ID_UNSUPPORTED;}command.kind=CommandKind::NUMBER;command.number=digits[0];}
        r->queued.push_back(command);*tag={bank,depth,r->queued.size()-1};
        return GH_ID_OK;
    }catch(const std::bad_alloc&){return GH_ID_MEMORY;}
}
extern "C" gh_id_error gh_mc_renderer_queue_fan(gh_mc_renderer*r,const gh_cockpit_fan*fan,gh_ui_ot_tag*tag)
{
    if(!r||!r->begun||!fan||!fan->ready||!tag)return GH_ID_ARGUMENT;
    if(r->flushing||r->queue_failed||r->status.jobs)return GH_ID_BUSY;
    if(fan->bank!=6||fan->depth!=5||fan->texture.width!=16||fan->texture.height!=16||!fan->texture.source.bytes)return GH_ID_UNSUPPORTED;
    if(r->queued.size()>=8192)return GH_ID_CAPACITY;
    try {QueuedCommand c;c.kind=CommandKind::FAN;c.fan=*fan;r->queued.push_back(c);*tag={6,5,r->queued.size()-1};return GH_ID_OK;}
    catch(const std::bad_alloc&){return GH_ID_MEMORY;}
}
extern "C" gh_id_error gh_mc_renderer_queue_view_groups(gh_mc_renderer*r,const gh_id_render_view*view,const gh_id_draw_job*job,gh_ui_ot_tag*tags,size_t capacity,size_t*count)
{
    if(!r||!r->begun||!view||!view->base||!view->manager||!job||!job->entry||!count)return GH_ID_ARGUMENT;
    if(r->flushing||r->queue_failed||r->status.jobs)return GH_ID_BUSY;
    if(null_texture(view,job->entry)){*count=0;return GH_ID_OK;}
    if(job->entry->kind!=1){if(!capacity)return GH_ID_CAPACITY;if(!tags)return GH_ID_ARGUMENT;gh_ui_ot_tag tag;gh_id_error e=gh_mc_renderer_queue_view(r,view,job,&tag);if(e)return e;tags[0]=tag;*count=1;return GH_ID_OK;}
    gh_id_texture_quad digits[10];size_t n=0;gh_id_error e=gh_id_number_prepare(view->manager,view->base,job->entry,digits,10,&n,&r->status.frontier);if(e)return e;
    if(n>capacity||n>8192-r->queued.size())return GH_ID_CAPACITY;if(n&&!tags)return GH_ID_ARGUMENT;
    try {
        size_t first=r->queued.size();r->queued.reserve(first+n);
        for(size_t i=0;i<n;i++){QueuedCommand command;command.kind=CommandKind::NUMBER;command.number=digits[i];r->queued.push_back(command);}
        for(size_t i=0;i<n;i++)tags[i]={digits[i].color_mode,digits[i].font_mode,first+i};*count=n;return GH_ID_OK;
    }catch(const std::bad_alloc&){return GH_ID_MEMORY;}
}
extern "C" gh_id_error gh_mc_renderer_queue_view_groups_at_z(gh_mc_renderer *r,const gh_id_render_view *view,
 const gh_id_draw_job *job,gh_ui_ot_tag *tags,size_t capacity,size_t *count,uint16_t z)
{
    gh_id_error error=gh_mc_renderer_queue_view_groups(r,view,job,tags,capacity,count);
    // SystemMenu's original 2AAEF0 -> 2C5EA8 -> 15BCF0 chain. Accept only
    // the proved vertex Z=FFFF profile, bank6/depth5 and the same Cockpit ID2.
    if(error==GH_ID_OK&&*count==1&&job->entry->kind==0&&view->base->id==2&&view->base->resource_number==1&&
       job->consumer_address==0x2aaef0&&tags[0].bank==6&&tags[0].depth==5&&z==65535){
        auto &id=r->queued[tags[0].index].id;id.z=z;id.z_known=true;
    }
    return error;
}
extern "C" gh_id_error gh_mc_renderer_queue_fade(gh_mc_renderer*r,const gh_fade_draw*draw,const gh_ui_fade_context*context,gh_ui_ot_tag*tag)
{
    if(!r||!r->begun||!draw||!context||!tag||!draw->visible)return GH_ID_ARGUMENT;
    if(r->flushing||r->queue_failed||r->status.jobs)return GH_ID_BUSY;
    if(draw->ot_min>=10||!draw->ot_max||draw->ot_max>INT32_MAX)return GH_ID_UNSUPPORTED;
    uint16_t xy[8];if(gh_ui_fade_geometry(draw,context,xy)!=GH_UI_OK)return GH_ID_UNSUPPORTED;
    if(r->queued.size()>=8192)return GH_ID_CAPACITY;
    try {QueuedCommand c;c.kind=CommandKind::FADE;c.fade=*draw;c.fade_context=*context;r->queued.push_back(c);*tag={draw->ot_min,(int32_t)draw->ot_max,r->queued.size()-1};return GH_ID_OK;}
    catch(const std::bad_alloc&){return GH_ID_MEMORY;}
}
extern "C" gh_id_error gh_mc_renderer_queue_cover(gh_mc_renderer*r,const gh_child_cover_quad*cover,gh_ui_ot_tag*tag)
{
    if(!r||!r->begun||!cover||!tag)return GH_ID_ARGUMENT;
    if(r->flushing||r->queue_failed||r->status.jobs)return GH_ID_BUSY;
    static const float xyzw[4][4]={{-1,-1,0,1},{513,-1,0,1},{-1,449,0,1},{513,449,0,1}};
    bool known=cover->mode==1&&cover->descriptor_flags==1&&cover->bank==6&&cover->depth==5&&cover->alpha==UINT64_C(0x8000000044)&&cover->test_override==0x3001d;
    for(unsigned i=0;i<4;i++)for(unsigned k=0;k<4;k++)if(cover->vertices[i][k]!=xyzw[i][k])known=false;
    if(!known||(cover->rgba>>24)>1){r->status.frontier=0x15b880;return GH_ID_UNSUPPORTED;}
    if(r->queued.size()>=8192)return GH_ID_CAPACITY;
    try {QueuedCommand c;c.kind=CommandKind::COVER;c.cover=*cover;r->queued.push_back(c);*tag={6,5,r->queued.size()-1};return GH_ID_OK;}
    catch(const std::bad_alloc&){return GH_ID_MEMORY;}
}
static gh_id_error draw_fan(gh_mc_renderer*r,const gh_cockpit_fan&fan)
{
    gh_id_error error=GH_ID_OK;gh_ui_error gpu=GH_UI_OK;r->status.consumer=0x15c410;r->status.frontier=0;r->status.opcode=0;
    try {
        if(!r->polygon)gpu=gh_ui_polygon_create(512,448,&r->polygon);uint32_t handle=0;
        for(const auto&t:r->fan_textures)if(t.source==fan.texture.source.bytes){handle=t.handle;break;}
        if(gpu==GH_UI_OK&&!handle){std::vector<unsigned char> rgba(16*16*4);error=gh_cockpit_fan_texture_rgba(&fan.texture,rgba.data(),rgba.size());
            if(error==GH_ID_OK){gpu=gh_ui_polygon_upload_fan16(r->polygon,&fan.texture,rgba.data(),rgba.size(),&handle);if(gpu==GH_UI_OK)r->fan_textures.push_back({fan.texture.source.bytes,handle});}}
        if(error==GH_ID_OK&&gpu==GH_UI_OK){gh_ui_sprite_state state={UINT64_C(0x8000000044),0x3001d,0x55,0x60,0,1,1,0,0,0};
            gpu=gh_ui_polygon_fan(r->polygon,&state,handle,&fan,r->frame.data(),r->next.data(),r->next.size());}
        if(gpu!=GH_UI_OK)error=GH_ID_UNSUPPORTED;if(error==GH_ID_OK){r->frame.swap(r->next);++r->status.jobs;}
    }catch(const std::bad_alloc&){error=GH_ID_MEMORY;}
    r->status.id_error=error;r->status.gpu_error=(unsigned)gpu;if(error!=GH_ID_OK)r->status.frontier=0x15c410;return error;
}
static gh_id_error draw_fade(gh_mc_renderer*r,const gh_fade_draw&draw,const gh_ui_fade_context&context,
 const uint32_t *z,size_t z_count)
{
    gh_ui_error gpu=GH_UI_OK;r->status.consumer=0x307388;r->status.opcode=0;r->status.frontier=0;
    if(!r->fade)gpu=gh_ui_fade_create(&r->fade);if(gpu==GH_UI_OK)gpu=gh_ui_fade_apply_with_depth(r->fade,&draw,&context,r->frame.data(),r->next.data(),r->next.size(),z,z_count);
    gh_id_error e=gpu==GH_UI_OK?GH_ID_OK:GH_ID_UNSUPPORTED;if(e==GH_ID_OK){r->frame.swap(r->next);++r->status.jobs;}else r->status.frontier=0x15c928;
    r->status.id_error=e;r->status.gpu_error=(unsigned)gpu;return e;
}
static gh_id_error flush_queued(gh_mc_renderer *r,const size_t *order,size_t count,uint32_t clear_rgb,const uint8_t *background,size_t background_bytes,const uint32_t *depth,size_t depth_count)
{
    if(!r||!r->begun||(!order&&count)||count!=r->queued.size())return GH_ID_ARGUMENT;
    if((background&&background_bytes!=r->frame.size())||(!background&&background_bytes))return GH_ID_ARGUMENT;
    if(depth?(!background||depth_count!=r->frame.size()/4):depth_count!=0)return GH_ID_ARGUMENT;
    if(r->flushing||r->queue_failed)return GH_ID_BUSY;
    try {
        std::vector<unsigned char> seen(count,0);
        for(size_t i=0;i<count;++i){if(order[i]>=count||seen[order[i]])return GH_ID_ARGUMENT;seen[order[i]]=1;}
#ifdef GH_NATIVE_STARTUP_QA
        if(depth)for(size_t i=0;i<count;i++){
            const auto &c=r->queued[order[i]];
            std::fprintf(stderr,"QA_UI_DEPTH_PREFIX seq%llu type%u idKind%u resource%d entry%u consumer%08X bank%u depth%d test%llX\n",
                (unsigned long long)i,(unsigned)c.kind,(unsigned)c.id.entry.kind,c.id.base.resource_number,c.id.entry_index,c.id.consumer,
                (unsigned)c.id.entry.b8d,c.id.entry.h8e,(unsigned long long)c.fade.test);
        }
#endif
        if(background)std::copy(background,background+background_bytes,r->frame.begin());
        else for(size_t i=0;i<r->frame.size();i+=4){
            r->frame[i]=(unsigned char)clear_rgb;r->frame[i+1]=(unsigned char)(clear_rgb>>8);
            r->frame[i+2]=(unsigned char)(clear_rgb>>16);r->frame[i+3]=0;
        }
        std::vector<uint32_t> depth_samples;
        if(depth)depth_samples.assign(depth,depth+depth_count);
        r->flushing=true;
        bool depth_known=depth!=nullptr;
        for(size_t i=0;i<count;++i){
            auto &command=r->queued[order[i]];gh_id_error error;
            if(command.kind==CommandKind::FAN)error=draw_fan(r,command.fan);
            else if(command.kind==CommandKind::FADE){
                bool known_terminal=depth_known&&i+1==count;
                error=draw_fade(r,command.fade,command.fade_context,known_terminal?depth_samples.data():nullptr,known_terminal?depth_samples.size():0);
            }
            else if(command.kind==CommandKind::NUMBER){gh_ui_error gpu=GH_UI_OK;r->status.consumer=0x2aaff0;r->status.frontier=0;r->status.opcode=0;error=render_number(r,command.number,&gpu);r->status.id_error=error;r->status.gpu_error=(unsigned)gpu;
                if(error==GH_ID_OK){r->frame.swap(r->next);++r->status.jobs;}else r->status.frontier=0x2c6d48;}
            else if(command.kind==CommandKind::COVER){r->status.consumer=0x160d50;r->status.frontier=0;r->status.opcode=0;r->status.id_error=GH_ID_OK;r->status.gpu_error=0;++r->status.jobs;error=GH_ID_OK;} /* alpha test KEEP discards color AND Z */
            else {auto view=command.id.view;view.base=&command.id.base;
                gh_id_draw_job job={command.id.entry_index,&command.id.entry,command.id.consumer};
                gh_ui_depth_write plane={depth_samples.data(),depth_samples.size(),command.id.z};
                error=draw_view(r,&view,&job,depth_known&&command.id.z_known?&plane:nullptr);}
            if(error!=GH_ID_OK){r->flushing=false;r->queue_failed=true;return error;}
            // COVER always discards both writes. Each admitted SystemMenu
            // batch returns the real depth writes from its color shader.
            if(command.kind!=CommandKind::COVER&&!(command.kind==CommandKind::ID&&command.id.z_known))depth_known=false;
        }
        r->flushing=false;r->queued.clear();return GH_ID_OK;
    }catch(const std::bad_alloc&){r->flushing=false;r->queue_failed=true;return GH_ID_MEMORY;}
}
extern "C" gh_id_error gh_mc_renderer_flush_queued(gh_mc_renderer *r,const size_t *order,size_t count,uint32_t clear_rgb)
{return flush_queued(r,order,count,clear_rgb,nullptr,0,nullptr,0);}
extern "C" gh_id_error gh_mc_renderer_flush_queued_on_background(gh_mc_renderer *r,const size_t *order,size_t count,const uint8_t *rgba,size_t bytes)
{if(!rgba)return GH_ID_ARGUMENT;return flush_queued(r,order,count,0,rgba,bytes,nullptr,0);}
extern "C" gh_id_error gh_mc_renderer_flush_queued_on_background_with_depth(gh_mc_renderer*r,const size_t *order,size_t count,
 const uint8_t *rgba,size_t bytes,const uint32_t *z,size_t z_count)
{if(!rgba||!z)return GH_ID_ARGUMENT;return flush_queued(r,order,count,0,rgba,bytes,z,z_count);}
extern "C" gh_id_error gh_mc_renderer_draw(gh_mc_renderer *r,const gh_startup_snapshot *s,const gh_id_draw_job *job)
{
    if(!s)return GH_ID_ARGUMENT;
    gh_id_render_view view={s->font,s->menu,s->id,s->mc_ui,s->mc_messages};
    return gh_mc_renderer_draw_view(r,&view,job);
}
extern "C" const unsigned char *gh_mc_renderer_pixels(const gh_mc_renderer *r,size_t *bytes)
{if(!r||!bytes)return nullptr;*bytes=r->frame.size();return r->frame.data();}
extern "C" void gh_mc_renderer_inspect(const gh_mc_renderer *r,gh_mc_render_status *out)
{if(r&&out)*out=r->status;}
