#include "mc_renderer.h"
#include "ui_text_renderer.h"
#include "ui_sprite_renderer.h"
#include "ui_polygon_renderer.h"
#include <algorithm>
#include <cstring>
#include <new>
#include <vector>

struct CachedTexture {const unsigned char *source;uint32_t handle;};
struct gh_mc_renderer {
    gh_ui_text_renderer *text=nullptr;
    gh_ui_sprite_renderer *sprite=nullptr;
    gh_ui_polygon_renderer *polygon=nullptr;
    const unsigned char *font_mst=nullptr,*font_msa=nullptr;
    std::vector<unsigned char> palette,frame,next;
    std::vector<gh_id_text_glyph> glyphs;
    std::vector<CachedTexture> textures;
    std::vector<CachedTexture> polygon_textures;
    gh_mc_render_status status={};
    int32_t language=1;
    bool begun=false;
    bool reference_polygons=false;
};
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
    r->font_mst=r->font_msa=nullptr;r->textures.clear();r->polygon_textures.clear();
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
    r->status={};r->begun=true;
}
extern "C" gh_id_error gh_mc_renderer_draw_view(gh_mc_renderer *r,const gh_id_render_view *s,const gh_id_draw_job *job)
{
    if(!r||!r->begun||!s||!job||!job->entry||!s->base||!s->manager)return GH_ID_ARGUMENT;
    gh_id_error error=GH_ID_UNSUPPORTED;gh_ui_error gpu=GH_UI_OK;
    r->status.consumer=job->consumer_address;r->status.opcode=0;r->status.frontier=0;
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
                    gpu=gh_ui_sprite_batch(r->sprite,&state,&item,1,r->frame.data(),r->next.data(),r->next.size());
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
                        gpu=gh_ui_polygon_batch(r->polygon,&state,&polygon,1,r->frame.data(),r->next.data(),r->next.size());
                    }
                }
            }
        }
        if(gpu!=GH_UI_OK)error=GH_ID_UNSUPPORTED;
        if(error==GH_ID_OK){r->frame.swap(r->next);++r->status.jobs;}
    }catch(const std::bad_alloc&){error=GH_ID_MEMORY;}
    r->status.id_error=error;r->status.gpu_error=(unsigned)gpu;
    return error;
}
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
