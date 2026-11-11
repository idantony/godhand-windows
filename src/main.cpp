/* Native execution driver. Unrecovered original consumers remain explicit
 * frontiers; reaching one never means the game has finished or is playable. */
#include "platform/startup_host.h"
#include "platform/window_host.h"
#include "platform/mc_renderer.h"
#include "platform/ui_fade_renderer.h"
#include "platform/model_renderer.h"
#include "platform/static_world_queue.h"
#include "platform/shadow_world_packet.h"
#include "platform/actor_world_queue.h"
#include "platform/effect_postprocess_reference.h"
#include "core/ui_order.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <string>

struct ShadowEntry {gh_shadow_world_packet packet;const gh_stage_material *material;};
struct NativeApp {
    gh_startup_host *host=nullptr;
    gh_window_host *window=nullptr;
    gh_mc_renderer *renderer=nullptr;
    gh_model_renderer *world_renderer=nullptr;
    gh_static_world_queue *world_queue=nullptr;
    gh_actor_world_queue *actor_queue=nullptr;
    gh_startup_snapshot snapshot={};
    std::vector<unsigned char> elf;
    std::vector<unsigned char> queued_pixels;
    std::vector<unsigned char> world_pixels;
    std::vector<gh_ui_order_group> groups;
    std::vector<ShadowEntry> shadow_queue;
    std::vector<gh_effect_trans_packet> effect_queue;
    bool frame_building=false,display_pending=false,building_scene=false,pending_scene=false;
    uint64_t prepared_mc_serial=0,pending_mc_serial=0,pending_fade_serial=0;
    uint64_t prepared_scene_serial=0,presented_scene=0;
    uint64_t resource_epoch=0,frame_serial=0,presented_mc=0,presented_movie=0;
    uint64_t scene_resource_epoch=0;
    uint64_t presented_fade_serial=0;
    uint32_t display_base0_rgb=0;
    bool display_base0_known=false;
    unsigned surface_kind=0; /*1=actual UI512448,2=Movie640448*/
    gh_window_error window_error=GH_WINDOW_OK;
    gh_model_gpu_error world_gpu_error=GH_MODEL_GPU_OK;
    uint32_t world_frontier=0,world_vu_pc=0;
    uint64_t world_packets=0,effect_post_packets=0,effect_sprite_packets=0;
    gh_effect_post_status effect_post_error=GH_EFFECT_POST_OK;
    gh_fade_draw frame_fade={};bool frame_fade_seen=false;
};
static constexpr uint64_t world_identity=UINT64_C(1)<<63;
static constexpr uint64_t shadow_identity=UINT64_C(1)<<62;
static constexpr uint64_t actor_identity=UINT64_C(1)<<61;
static constexpr uint64_t effect_identity=UINT64_C(1)<<60;
static gh_id_error begin_mc(void *opaque)
{
    auto &app=*(NativeApp*)opaque;gh_startup_snapshot s={};
    if(gh_startup_host_snapshot(app.host,&s)!=GH_HOST_OK)return GH_ID_ARGUMENT;
    if(s.mc_ui_epoch!=app.resource_epoch){gh_mc_renderer_reset_resources(app.renderer);app.resource_epoch=s.mc_ui_epoch;}
    gh_mc_renderer_begin(app.renderer);gh_static_world_queue_reset(app.world_queue);gh_actor_world_queue_reset(app.actor_queue);app.shadow_queue.clear();app.effect_queue.clear();app.groups.clear();app.world_pixels.clear();app.frame_fade_seen=false;app.frame_building=true;app.building_scene=false;app.surface_kind=1;return GH_ID_OK;
}
static gh_id_error begin_scene(void *opaque,uint32_t *)
{
    auto &app=*(NativeApp*)opaque;gh_startup_snapshot s={};
    if(gh_startup_host_snapshot(app.host,&s)!=GH_HOST_OK)return GH_ID_ARGUMENT;
    if(s.mc_ui_epoch!=app.resource_epoch||s.ui_resource_epoch!=app.scene_resource_epoch){
        gh_mc_renderer_reset_resources(app.renderer);gh_model_renderer_reset_resources(app.world_renderer);app.resource_epoch=s.mc_ui_epoch;app.scene_resource_epoch=s.ui_resource_epoch;}
    gh_mc_renderer_begin(app.renderer);gh_static_world_queue_reset(app.world_queue);gh_actor_world_queue_reset(app.actor_queue);app.shadow_queue.clear();app.effect_queue.clear();app.groups.clear();app.world_pixels.clear();app.frame_fade_seen=false;app.frame_building=true;app.building_scene=true;app.surface_kind=1;return GH_ID_OK;
}
#ifdef GH_NATIVE_STARTUP_QA
static void qa_float_bits(const char *name,const float *values,size_t count)
{
    std::fprintf(stderr,",\"%s\":[",name);
    for(size_t i=0;i<count;i++){uint32_t bits;std::memcpy(&bits,values+i,4);std::fprintf(stderr,"%s\"%08X\"",i?",":"",bits);}
    std::fprintf(stderr,"]");
}
static void qa_static_failure(const gh_object_static_command *command)
{
    if(!command->object)return;
    const auto &d=command->object->descriptor;
    // Private runtime constants only; source assets remain on the user's disc.
    std::fprintf(stderr,"QA_STATIC_INPUT {\"object_index\":%llu,\"handle\":%u,\"program\":%u,\"script_index\":%u,\"mesh_index\":%u,\"node_flags\":\"%08X\",\"streams\":%u",
        (unsigned long long)command->object_index,command->handle,command->selected_program,d.script_index,d.mesh_index,d.node_flags,command->material_count);
    qa_float_bits("screen_bits",command->packet.inputs.screen_matrix,16);
    qa_float_bits("clip_bits",command->packet.inputs.clip_matrix,16);
    qa_float_bits("projection_bits",command->packet.projection,16);
    qa_float_bits("uv_offset_bits",command->packet.inputs.uv_offset,2);
    qa_float_bits("color_scale_bits",command->packet.inputs.color_scale,4);
    qa_float_bits("fog_bits",command->packet.inputs.fog,2);
    std::fprintf(stderr,"}\n");
}
static void qa_actor_gpu_failure(const gh_actor_world_group &g,const gh_actor_world_packet &p,const gh_model_draw_state &s)
{
    std::fprintf(stderr,"QA_ACTOR_GPU_INPUT {\"node_index\":%llu,\"program\":%u,\"bank\":%u,\"depth\":%u,\"triangles\":%llu,\"vertex_count\":%llu,\"tex0\":\"%016llX\",\"test\":\"%016llX\",\"alpha\":\"%016llX\",\"tex1\":\"%016llX\",\"clamp\":\"%016llX\",\"primitive\":%u,\"texture_function\":%u,\"offset_x\":%u,\"offset_y\":%u,\"fog_rgb\":%u,\"texture_width\":%u,\"texture_height\":%u,\"texture_psm\":%u,\"indices\":[",
        (unsigned long long)g.command.node_index,g.command.selected_program,g.command.bank,g.command.depth,(unsigned long long)p.triangles,(unsigned long long)p.vertex_count,
        (unsigned long long)p.tex0,(unsigned long long)s.test,(unsigned long long)s.alpha,(unsigned long long)s.tex1,(unsigned long long)s.clamp,s.primitive,s.texture_function,s.xy_offset_x16,s.xy_offset_y16,s.fog_rgb,
        p.material?p.material->width:1,p.material?p.material->height:1,p.material?p.material->psm:0);
    for(size_t i=0;i<p.triangles*3;i++)std::fprintf(stderr,"%s%u",i?",":"",p.indices[i]);
    std::fprintf(stderr,"],\"vertices\":[");
    for(size_t i=0;i<p.vertex_count;i++){
        const auto &v=p.vertices[i];uint32_t q[3];std::memcpy(q,v.stq,12);
        std::fprintf(stderr,"%s{\"stq_bits\":[\"%08X\",\"%08X\",\"%08X\"],\"rgba\":[%u,%u,%u,%u],\"xyz_fixed4\":[%d,%d,%d],\"fog_fixed4\":%u,\"control\":%u,\"clip\":%u,\"adc\":%u}",
            i?",":"",q[0],q[1],q[2],v.rgba[0],v.rgba[1],v.rgba[2],v.rgba[3],v.xyz_fixed4[0],v.xyz_fixed4[1],v.xyz_fixed4[2],v.fog_fixed4,v.raw_control,v.clip_flags,v.adc);
    }
    std::fprintf(stderr,"]}\n");
}
#endif
static gh_object_step_status queue_world(void *opaque,const gh_object_static_command *command,uint32_t *frontier)
{
    auto &app=*(NativeApp*)opaque;
    if(!command||!app.frame_building||!app.building_scene)return GH_OBJSTEP_ARGUMENT;
    try {
        app.groups.reserve(app.groups.size()+1);size_t index=0;gh_static_transform_frontier f={};
        gh_static_world_status e=gh_static_world_queue_append(app.world_queue,command,&index,&f);
        if(e){
#ifdef GH_NATIVE_STARTUP_QA
            qa_static_failure(command);
#endif
            app.world_frontier=f.ee_entry;app.world_vu_pc=f.vu_byte_pc;if(frontier)*frontier=f.ee_entry;return e==GH_WORLD_UNSUPPORTED?GH_OBJSTEP_UNSUPPORTED:GH_OBJSTEP_CALLEE;}
        app.groups.push_back({command->bank,command->depth,world_identity|index});return GH_OBJSTEP_OK;
    }catch(const std::bad_alloc&){return GH_OBJSTEP_CALLEE;}
}
static gh_actor_model_status queue_shadow(void *opaque,const gh_shadow_group *group,uint32_t *frontier)
{
    auto &app=*(NativeApp*)opaque;
    if(!group||!app.frame_building||!app.building_scene)return GH_ACTOR_MODEL_ARGUMENT;
    try {
        app.groups.reserve(app.groups.size()+1);app.shadow_queue.reserve(app.shadow_queue.size()+1);
        ShadowEntry entry={};uint32_t f=0;
        gh_shadow_packet_status e=gh_shadow_world_packet_capture(group,&entry.packet,&f);
        if(e){app.world_frontier=f;if(frontier)*frontier=f;return e==GH_SHADOW_PACKET_UNSUPPORTED?GH_ACTOR_MODEL_UNSUPPORTED:GH_ACTOR_MODEL_ARGUMENT;}
        if(gh_startup_host_resolve_tex0(app.host,group->tex0,&entry.material)!=GH_HOST_OK){app.world_frontier=0x151368;if(frontier)*frontier=0x151368;return GH_ACTOR_MODEL_UNSUPPORTED;}
        size_t index=app.shadow_queue.size();app.shadow_queue.push_back(entry);
        app.groups.push_back({group->bank,group->depth,world_identity|shadow_identity|index});return GH_ACTOR_MODEL_OK;
    }catch(const std::bad_alloc&){return GH_ACTOR_MODEL_MEMORY;}
}
static gh_actor_model_status queue_actor(void *opaque,const gh_actor_draw_command *command,uint32_t *frontier)
{
    auto &app=*(NativeApp*)opaque;
    if(!command||!app.frame_building||!app.building_scene)return GH_ACTOR_MODEL_ARGUMENT;
    try {
        app.groups.reserve(app.groups.size()+1);size_t index=0;gh_static_transform_frontier f={};
        gh_actor_world_status e=gh_actor_world_queue_append(app.actor_queue,command,&index,&f);
        if(e){app.world_frontier=f.ee_entry;app.world_vu_pc=f.vu_byte_pc;if(frontier)*frontier=f.ee_entry;return e==GH_ACTOR_WORLD_UNSUPPORTED?GH_ACTOR_MODEL_UNSUPPORTED:GH_ACTOR_MODEL_BOUNDS;}
        app.groups.push_back({command->bank,command->depth,world_identity|actor_identity|index});return GH_ACTOR_MODEL_OK;
    }catch(const std::bad_alloc&){return GH_ACTOR_MODEL_MEMORY;}
}
static gh_effect_status queue_effect(void *opaque,const gh_effect_trans_packet *packet)
{
    auto &app=*(NativeApp*)opaque;
    if(!packet||!app.frame_building||!app.building_scene)return GH_EFFECT_ARGUMENT;
    try {
        // Reserve both before publishing either; retry owns one packet.
        app.groups.reserve(app.groups.size()+1);
        app.effect_queue.reserve(app.effect_queue.size()+1);
        size_t index=app.effect_queue.size();
        app.effect_queue.push_back(*packet);
        app.groups.push_back({packet->order.bank,packet->order.depth,world_identity|effect_identity|index});
        return GH_EFFECT_OK;
    }catch(const std::bad_alloc&){return GH_EFFECT_BOUNDS;}
}
static gh_id_error draw_mc(void *opaque,const gh_id_draw_job *job)
{
    auto &app=*(NativeApp*)opaque;gh_startup_snapshot s={};
    if(gh_startup_host_snapshot(app.host,&s)!=GH_HOST_OK)return GH_ID_ARGUMENT;
    gh_id_render_view view={s.font,s.menu,s.id,s.mc_ui,s.mc_messages};gh_ui_ot_tag tags[10];size_t count=0;
    gh_id_error error=gh_mc_renderer_queue_view_groups(app.renderer,&view,job,tags,10,&count);
    if(error==GH_ID_OK){try {for(size_t i=0;i<count;++i)app.groups.push_back({tags[i].bank,(uint32_t)tags[i].depth,(uint64_t)tags[i].index});}
        catch(const std::bad_alloc&){return GH_ID_MEMORY;}}
    return error;
}
static gh_id_error draw_scene(void *opaque,const gh_id_base *base,const gh_id_draw_job *job,uint32_t *frontier)
{
    auto &app=*(NativeApp*)opaque;gh_startup_snapshot s={};
    if(gh_startup_host_snapshot(app.host,&s)!=GH_HOST_OK)return GH_ID_ARGUMENT;
    gh_id_render_view view={s.font,s.menu,s.id,base,nullptr};gh_ui_ot_tag tags[10];size_t count=0;
    gh_id_error error=s.id_vertex_z_known?
        gh_mc_renderer_queue_view_groups_at_z(app.renderer,&view,job,tags,10,&count,s.id_vertex_z):
        gh_mc_renderer_queue_view_groups(app.renderer,&view,job,tags,10,&count);
    if(error==GH_ID_OK){try {for(size_t i=0;i<count;++i)app.groups.push_back({tags[i].bank,(uint32_t)tags[i].depth,(uint64_t)tags[i].index});}
        catch(const std::bad_alloc&){return GH_ID_MEMORY;}}
    else if(frontier){gh_mc_render_status status={};gh_mc_renderer_inspect(app.renderer,&status);*frontier=status.frontier?status.frontier:job->consumer_address;}
    return error;
}
static gh_id_error draw_fan(void *opaque,const gh_cockpit_fan *fan,uint32_t *frontier)
{
    auto &app=*(NativeApp*)opaque;gh_ui_ot_tag tag={};
    gh_id_error error=gh_mc_renderer_queue_fan(app.renderer,fan,&tag);
    if(error==GH_ID_OK){try {app.groups.push_back({tag.bank,(uint32_t)tag.depth,(uint64_t)tag.index});}
        catch(const std::bad_alloc&){return GH_ID_MEMORY;}}
    else if(frontier)*frontier=0x15c410;return error;
}
static gh_id_error draw_cover(void *opaque,const gh_child_cover_quad *quad,uint32_t *frontier)
{
    auto &app=*(NativeApp*)opaque;gh_ui_ot_tag tag={};
    gh_id_error error=gh_mc_renderer_queue_cover(app.renderer,quad,&tag);
    if(error==GH_ID_OK){try {app.groups.push_back({tag.bank,(uint32_t)tag.depth,(uint64_t)tag.index});}
        catch(const std::bad_alloc&){return GH_ID_MEMORY;}}
    else if(frontier)*frontier=0x15b880;return error;
}
static gh_scene_ui_status draw_scene_message(void *opaque,const gh_scene_message_glyph_pair *pair,uint32_t *frontier)
{
    auto &app=*(NativeApp*)opaque;
    if(!app.frame_building||!app.building_scene)return GH_SCENE_UI_ARGUMENT;
    try{
        app.groups.reserve(app.groups.size()+2);
        gh_ui_ot_tag tags[2]={};
        gh_id_error status=gh_mc_renderer_queue_message(app.renderer,pair,tags);
        if(status){
            *frontier=0x2a5930;
            return status==GH_ID_MEMORY||status==GH_ID_CAPACITY?GH_SCENE_UI_BOUNDS:GH_SCENE_UI_UNSUPPORTED;
        }
        for(const auto &tag:tags)app.groups.push_back({tag.bank,(uint32_t)tag.depth,(uint64_t)tag.index});
        *frontier=0;
        return GH_SCENE_UI_OK;
    }catch(const std::bad_alloc&){return GH_SCENE_UI_BOUNDS;}
}
static gh_id_error post_mc(void *,uint16_t message)
{
    std::fprintf(stderr,"Unrecovered original ID postMessage %04X\n",message);
    return GH_ID_UNSUPPORTED;
}
static int present_movie(void *opaque,const unsigned char *rgba,size_t bytes,unsigned width,unsigned height,uint64_t)
{
    auto &app=*(NativeApp*)opaque;
    app.window_error=gh_window_present(app.window,rgba,bytes,width,height,4,3);
    if(app.window_error!=GH_WINDOW_OK)return 0;
    app.surface_kind=2;++app.presented_movie;return 1;
}
static gh_id_error draw_fade(void *opaque,const gh_fade_draw *draw)
{
    auto &app=*(NativeApp*)opaque;
    if(!draw||app.surface_kind!=1||!app.frame_building)return GH_ID_UNSUPPORTED;
    gh_startup_snapshot s={};if(gh_startup_host_snapshot(app.host,&s)!=GH_HOST_OK)return GH_ID_ARGUMENT;
    const gh_display_state *display=s.display;float aspect=0;
    if(!display||!display->initialized||display->screen_width!=512||display->screen_lines!=448)return GH_ID_UNSUPPORTED;
    std::memcpy(&aspect,&display->pixel_aspect_bits,4);
    gh_ui_fade_context context={512,448,display->gs_y_origin,aspect,1,1,0,0,0};gh_ui_ot_tag tag={};
    gh_id_error error=gh_mc_renderer_queue_fade(app.renderer,draw,&context,&tag);
    if(error==GH_ID_OK){app.frame_fade=*draw;app.frame_fade_seen=true;}
    if(error==GH_ID_OK){try {app.groups.push_back({tag.bank,(uint32_t)tag.depth,(uint64_t)tag.index});}
        catch(const std::bad_alloc&){return GH_ID_MEMORY;}}
    return error;
}
// One context-1 sampler at this part of the original OT. We don't borrow a
// previous frame across the still separate startup UI command consumer.
struct WorldSampler {
    uint64_t tex1=0,clamp=0;
    bool tex1_known=false,clamp_known=false;
    void write(uint64_t t,uint64_t c){tex1=t;clamp=c;tex1_known=clamp_known=true;}
    bool known()const{return tex1_known&&clamp_known;}
};
struct PostFrame {
    std::vector<uint8_t> pixels[4];
    std::vector<uint8_t> warp_capture;
    uint32_t capture_serial=0,capture_slot=0;
    int capture_first_row_known=0;
    gh_effect_post_context context={};
    unsigned cleared=0;
    void initialize(uint32_t x,uint32_t y){
        if(context.visible_width)return;
        for(unsigned i=0;i<4;i++){
            unsigned w=i?128:512,h=i?128:448;
            pixels[i].resize((size_t)w*h*4);
            context.surfaces[i]={pixels[i].data(),pixels[i].size(),(size_t)w*4,w,h};
        }
        context.xy_offset_x16=x;context.xy_offset_y16=y;
        context.visible_width=512;context.visible_height=448;context.colclamp=1;
    }
};
static gh_id_error consume_scene_warp(NativeApp &app,const gh_effect_trans_packet &packet,
    WorldSampler &sampler,PostFrame &post,uint32_t x,uint32_t y)
{
    if(!sampler.known()||!packet.inherited_sampler||packet.writes_clamp)return GH_ID_UNSUPPORTED;
    if(packet.kind==GH_EFFECT_PASS_SCENE_CAPTURE){
        if(packet.writes_texa||!packet.serial)return GH_ID_UNSUPPORTED;
        post.initialize(x,y);
        post.warp_capture.resize(256*448*4);
        auto &scene=post.pixels[0];
        app.world_gpu_error=gh_model_renderer_readback(app.world_renderer,scene.data(),scene.size(),nullptr,0);
        if(app.world_gpu_error!=GH_MODEL_GPU_OK)return GH_ID_UNSUPPORTED;
        gh_effect_post_surface target={post.warp_capture.data(),post.warp_capture.size(),256*4,256,448};
        const gh_effect_post_sampler state={sampler.tex1,sampler.clamp,1,GH_EFFECT_SAMPLE_NATIVE_SEPARABLE_4BIT};
        app.effect_post_error=gh_effect_postprocess_capture_scene(&packet,&state,&post.context.surfaces[0],&target,
            x,y,&post.capture_first_row_known);
        if(app.effect_post_error!=GH_EFFECT_POST_OK)return GH_ID_UNSUPPORTED;
        post.capture_serial=packet.serial;
        post.capture_slot=packet.slot;
    }else{
        if(packet.original_callee!=0x2f02b8||!packet.serial||post.capture_serial!=packet.serial||
           post.capture_slot!=packet.slot||!packet.writes_tex0||!packet.writes_texa||packet.texa!=0x80||
           packet.tex0!=UINT64_C(0x2000000620113c00)||packet.source!=GH_EFFECT_SURFACE_3C00||
           packet.destination!=GH_EFFECT_SURFACE_SCENE_COLOR||packet.quad_count!=1||
           packet.quads[0].vertex_count!=6||!packet.restores_scene_target||
           packet.restore_frame_mask||packet.restore_zbuf!=0x3a0000f8||
           (packet.zbuf!=0x3a0000f8&&packet.zbuf!=UINT64_C(0x13a0000f8)))
            return GH_ID_UNSUPPORTED;
        gh_static_gs_vertex vertices[6]={};
        for(unsigned i=0;i<6;i++){
            const auto &v=packet.quads[0].vertices[i];
            if(v.q!=1||v.z!=0xfff0||v.u12_4||v.v12_4)return GH_ID_UNSUPPORTED;
            vertices[i].stq[0]=v.s;vertices[i].stq[1]=v.t;vertices[i].stq[2]=v.q;
            vertices[i].xyz_fixed4[0]=v.x12_4;vertices[i].xyz_fixed4[1]=v.y12_4;
            vertices[i].xyz_fixed4[2]=(int32_t)(v.z<<4);
            for(unsigned channel=0;channel<4;channel++)vertices[i].rgba[channel]=(v.rgba>>(channel*8))&255;
        }
        const gh_effect_post_sampler source_sampler={sampler.tex1,sampler.clamp,1,
            GH_EFFECT_SAMPLE_NATIVE_SEPARABLE_4BIT};
        app.effect_post_error=gh_effect_postprocess_warp_source_known(&packet,&source_sampler,
            512,448,x,y,post.capture_first_row_known);
        if(app.effect_post_error!=GH_EFFECT_POST_OK){
#ifdef GH_NATIVE_STARTUP_QA
                std::fprintf(stderr,"QA_WARP_BOUNDS {\"scene_serial\":%llu,\"slot\":%u,\"serial\":%u,\"tex1\":\"%016llX\",\"clamp\":\"%016llX\",\"alpha\":\"%016llX\",\"zbuf\":\"%016llX\",\"vertices\":[",
                    (unsigned long long)app.snapshot.scene_frame_serial,packet.slot,packet.serial,
                    (unsigned long long)sampler.tex1,(unsigned long long)sampler.clamp,
                    (unsigned long long)packet.alpha,(unsigned long long)packet.zbuf);
                for(unsigned j=0;j<6;j++){
                    const auto &point=packet.quads[0].vertices[j];uint32_t bits[3];
                    const float stq[]={point.s,point.t,point.q};
                    std::memcpy(bits,stq,sizeof(bits));
                    std::fprintf(stderr,"%s{\"xy\":[%d,%d],\"stq_bits\":[\"%08X\",\"%08X\",\"%08X\"],\"rgba\":\"%08X\"}",
                        j?",":"",point.x12_4,point.y12_4,bits[0],bits[1],bits[2],point.rgba);
                }
                std::fprintf(stderr,"]}\n");
#endif
            return GH_ID_UNSUPPORTED;
        }
        gh_model_draw_state state={};
        state.test=packet.test;state.alpha=packet.alpha;state.tex1=sampler.tex1;state.clamp=sampler.clamp;
        state.primitive=(uint16_t)packet.prim;state.frame_mask=packet.frame_mask;
        state.xy_offset_x16=x;state.xy_offset_y16=y;state.zbuf_psm=58;
        state.z_write=(uint8_t)((packet.zbuf>>32)==0);state.colclamp=state.native_float_reference=1;
        app.world_gpu_error=gh_model_renderer_draw_scene_warp(app.world_renderer,&state,post.warp_capture.data(),
            post.warp_capture.size(),packet.texa,vertices);
        if(app.world_gpu_error!=GH_MODEL_GPU_OK)return GH_ID_UNSUPPORTED;
        post.capture_serial=0;
    }
    ++app.effect_post_packets;
    app.world_frontier=0;
    return GH_ID_OK;
}
static gh_id_error consume_effect(NativeApp &app,const gh_effect_trans_packet &p,WorldSampler &sampler,
    PostFrame &post,uint32_t x,uint32_t y)
{
    app.world_frontier=p.original_callee;
    if(p.kind==GH_EFFECT_PASS_SCENE_CAPTURE||p.kind==GH_EFFECT_PASS_SCENE_WARP)
        return consume_scene_warp(app,p,sampler,post,x,y);
    if(p.kind==GH_EFFECT_PASS_SPRITE){
        // 15BCF0 writes TEX1_1/CLAMP_1 and inherits the scene target.
        // ESP 2FEC38 instead inherits TEX1_1 and restores FRAME/ZBUF.
        const bool marker=p.original_callee==0x15bcf0;
        if(!p.writes_clamp||p.clamp||!p.writes_tex0||p.prim!=0x54||
            p.source!=GH_EFFECT_SURFACE_MATERIAL||p.destination!=GH_EFFECT_SURFACE_SCENE_COLOR||
            !p.material||p.tex0!=p.material->source_tex0||p.quad_count!=1||p.quads[0].vertex_count!=4)
            return GH_ID_UNSUPPORTED;
        if(marker){
            if(!p.inherits_scene_target||!p.writes_tex1||p.tex1!=0x60||p.inherited_sampler||
                p.order.bank!=6||p.test!=0x5000d||p.alpha!=UINT64_C(0x8000000044)||
                p.frame_mask||p.zbuf||p.restores_scene_target)
                return GH_ID_UNSUPPORTED;
            sampler.write(p.tex1,p.clamp);
        }else{
            if(!p.inherited_sampler||p.inherits_scene_target||p.writes_tex1||
                !p.restores_scene_target||p.restore_frame_mask||p.restore_zbuf!=0x3a0000f8||
                (p.zbuf!=0x3a0000f8&&p.zbuf!=UINT64_C(0x13a0000f8)))
                return GH_ID_UNSUPPORTED;
            sampler.clamp=p.clamp;sampler.clamp_known=true;
        }
        if(!sampler.known())return GH_ID_UNSUPPORTED;
        gh_static_gs_vertex vertices[4]={};
        for(unsigned k=0;k<4;k++){
            const auto &v=p.quads[0].vertices[k];auto &dest=vertices[k];
            if(v.z>65535)return GH_ID_UNSUPPORTED;
            dest.stq[0]=v.s;dest.stq[1]=v.t;dest.stq[2]=v.q;
            dest.xyz_fixed4[0]=v.x12_4;dest.xyz_fixed4[1]=v.y12_4;dest.xyz_fixed4[2]=(int32_t)(v.z<<4);
            for(unsigned c=0;c<4;c++)dest.rgba[c]=(v.rgba>>(8*c))&255;
        }
        gh_model_draw_state state={};state.test=p.test;state.alpha=p.alpha;
        state.tex1=sampler.tex1;state.clamp=sampler.clamp;state.primitive=(uint16_t)p.prim;
        state.xy_offset_x16=x;state.xy_offset_y16=y;state.frame_mask=p.frame_mask;
        state.texture_function=(uint8_t)((p.tex0>>35)&3);
        state.zbuf_psm=58;state.z_write=(uint8_t)((p.zbuf>>32)==0);state.colclamp=state.native_float_reference=1;
        uint32_t texture=0;app.world_gpu_error=gh_model_renderer_upload(app.world_renderer,p.material,&texture);
        if(app.world_gpu_error!=GH_MODEL_GPU_OK)return GH_ID_UNSUPPORTED;
        static const uint32_t indices[]={0,1,2,2,1,3};
        app.world_gpu_error=gh_model_renderer_draw(app.world_renderer,&state,texture,vertices,4,indices,6);
        if(app.world_gpu_error!=GH_MODEL_GPU_OK)return GH_ID_UNSUPPORTED;
        ++app.effect_sprite_packets;app.world_frontier=0;return GH_ID_OK;
    }
    if(p.writes_clamp||(unsigned)p.source>=4||(unsigned)p.destination>=4)return GH_ID_UNSUPPORTED;
    post.initialize(x,y);
    if(p.kind!=GH_EFFECT_PASS_CLEAR){
        if((p.source&&!(post.cleared&(1u<<p.source)))||
           (p.destination&&!(post.cleared&(1u<<p.destination))))return GH_ID_UNSUPPORTED;
        if(p.source==GH_EFFECT_SURFACE_SCENE_COLOR||p.destination==GH_EFFECT_SURFACE_SCENE_COLOR){
            auto &scene=post.pixels[0];
            app.world_gpu_error=gh_model_renderer_readback(app.world_renderer,scene.data(),scene.size(),nullptr,0);
            if(app.world_gpu_error!=GH_MODEL_GPU_OK)return GH_ID_UNSUPPORTED;
        }
    }
    gh_effect_post_sampler current={sampler.tex1,sampler.clamp,(uint8_t)sampler.known(),GH_EFFECT_SAMPLE_NATIVE_SEPARABLE_4BIT};
    gh_effect_post_receipt receipt={};
    app.effect_post_error=gh_effect_postprocess_apply(&p,&current,&post.context,&receipt,&app.world_frontier);
    if(app.effect_post_error!=GH_EFFECT_POST_OK)return GH_ID_UNSUPPORTED;
    app.world_frontier=p.original_callee;
    if(p.kind==GH_EFFECT_PASS_CLEAR){
        if(receipt.covered_fragments!=128*128||receipt.color_writes!=128*128)return GH_ID_UNSUPPORTED;
        post.cleared|=1u<<p.destination;
    }
    if(p.destination==GH_EFFECT_SURFACE_SCENE_COLOR){
        const auto &scene=post.pixels[0];
        app.world_gpu_error=gh_model_renderer_replace_color(app.world_renderer,scene.data(),scene.size());
        if(app.world_gpu_error!=GH_MODEL_GPU_OK)return GH_ID_UNSUPPORTED;
    }
    ++app.effect_post_packets;app.world_frontier=0;return GH_ID_OK;
}
static gh_id_error prepare_ui_display(NativeApp &app,bool scene)
{
    try {
    size_t written=0,omitted=0;uint32_t clear=0;
    if(!app.snapshot.work||!app.frame_building)return GH_ID_ARGUMENT;
    std::vector<size_t> order(app.groups.size());
    if(gh_ui_order_indices(app.groups.data(),app.groups.size(),app.snapshot.flags,(uint32_t)app.snapshot.work->mode,
        order.data(),order.size(),&written,&omitted)!=GH_UI_ORDER_OK||omitted||written!=order.size())return GH_ID_UNSUPPORTED;
    std::memcpy(&clear,(const unsigned char*)app.snapshot.work+0x60c,4);
    std::vector<size_t> ui_order,world_order;
    std::vector<uint32_t> world_depth;
    bool ui_started=false;
    for(size_t i:order){const auto &g=app.groups[i];
        if(g.identity&world_identity){
            size_t encoded=(size_t)(g.identity&~world_identity);
            if(ui_started){
                size_t effect_index=encoded&~effect_identity;
                if((encoded&effect_identity)&&effect_index<app.effect_queue.size())
                    app.world_frontier=app.effect_queue[effect_index].original_callee;
                return GH_ID_UNSUPPORTED;
            }
            world_order.push_back(encoded);
        }
        else {ui_started=true;ui_order.push_back((size_t)g.identity);}}
    if(world_order.size()!=gh_static_world_queue_count(app.world_queue)+app.shadow_queue.size()+
       gh_actor_world_queue_count(app.actor_queue)+app.effect_queue.size())return GH_ID_ARGUMENT;
    // ONEOT determines both domains. This bounded native path requires world
    // and effects to precede UI; mixed UI/sprite depths remain strict.
    std::vector<unsigned char> world_seen(gh_static_world_queue_count(app.world_queue),0),shadow_seen(app.shadow_queue.size(),0),
        actor_seen(gh_actor_world_queue_count(app.actor_queue),0),effect_seen(app.effect_queue.size(),0),ui_seen(ui_order.size(),0);
    for(size_t encoded:world_order){
        bool shadow=(encoded&shadow_identity)!=0,actor=(encoded&actor_identity)!=0,effect=(encoded&effect_identity)!=0;
        if((unsigned)shadow+(unsigned)actor+(unsigned)effect>1)return GH_ID_ARGUMENT;
        size_t i=encoded&~(shadow_identity|actor_identity|effect_identity);
        auto &seen=shadow?shadow_seen:actor?actor_seen:effect?effect_seen:world_seen;
        if(i>=seen.size()||seen[i])return GH_ID_ARGUMENT;seen[i]=1;}
    for(size_t i:ui_order){if(i>=ui_seen.size()||ui_seen[i])return GH_ID_ARGUMENT;ui_seen[i]=1;}
    int message_depth=0;
    gh_id_error depth_status=gh_mc_renderer_queued_message_depth(app.renderer,ui_order.data(),ui_order.size(),&message_depth);
    if(depth_status!=GH_ID_OK)return depth_status;
    const bool world_surface=!world_order.empty()||message_depth;
    if(world_surface){
        const gh_display_state *display=app.snapshot.display;
        if(!scene||!display||!display->initialized||display->screen_width!=512||display->screen_lines!=448||display->gs_y_origin!=1824)return GH_ID_UNSUPPORTED;
        if(!app.world_renderer)app.world_gpu_error=gh_model_renderer_create(512,448,&app.world_renderer);
        if(app.world_gpu_error!=GH_MODEL_GPU_OK)return GH_ID_UNSUPPORTED;
        app.world_gpu_error=gh_model_renderer_begin(app.world_renderer,clear,0);
        if(app.world_gpu_error!=GH_MODEL_GPU_OK)return GH_ID_UNSUPPORTED;
        gh_static_world_group setup={};bool has_setup=false;
        uint32_t fog_rgb=0;bool fog_known=false;
        WorldSampler sampler;PostFrame post;
        const uint32_t x=(2048-display->screen_width/2)*16,y=display->gs_y_origin*16;
        for(size_t encoded:world_order){
            if(encoded&effect_identity){
                auto e=consume_effect(app,app.effect_queue[encoded&~effect_identity],sampler,post,x,y);
                if(e)return e;
                continue;
            }
            if(encoded&shadow_identity){
                const ShadowEntry &entry=app.shadow_queue[encoded&~shadow_identity];const auto &packet=entry.packet;const auto &g=packet.group;
                // A zero-fragment shadow group still executes TEX1_1/CLAMP_1.
                sampler.write(g.tex1,g.clamp);
                if(!packet.triangles)continue;
                uint32_t texture=0;app.world_gpu_error=gh_model_renderer_upload(app.world_renderer,entry.material,&texture);
                if(app.world_gpu_error!=GH_MODEL_GPU_OK){app.world_frontier=0x151368;return GH_ID_UNSUPPORTED;}
                gh_model_draw_state state={};state.test=g.test;state.alpha=g.alpha;state.tex1=g.tex1;state.clamp=g.clamp;state.primitive=g.primitive;
                state.xy_offset_x16=(2048-display->screen_width/2)*16;state.xy_offset_y16=display->gs_y_origin*16;
                // Original shadow ZBUF_1=13A0000F8 disables Z writes; its
                // trailing3A0000F8 restores them for subsequent groups.
                state.zbuf_psm=58;state.z_write=0;state.colclamp=state.native_float_reference=1;
                app.world_gpu_error=gh_model_renderer_draw(app.world_renderer,&state,texture,packet.vertices,packet.vertex_count,packet.indices,packet.triangles*3);
                if(app.world_gpu_error!=GH_MODEL_GPU_OK){app.world_frontier=0x15ec60;return GH_ID_UNSUPPORTED;}
                ++app.world_packets;continue;
            }
            if(encoded&actor_identity){
                size_t i=encoded&~actor_identity;gh_actor_world_group g={};
                if(gh_actor_world_queue_group_at(app.actor_queue,i,&g)!=GH_ACTOR_WORLD_OK)return GH_ID_ARGUMENT;
                // VIF/palette work already produced owned post-VU vertices.
                // The original GS setup changes only the ONE current FOGCOL.
                if(g.command.kind==GH_ACTOR_DRAW_SETUP){fog_rgb=g.command.fog_rgb;fog_known=true;continue;}
                sampler.write(g.command.tex1,g.command.clamp);
                for(size_t j=0;j<g.packets;j++){
                    gh_actor_world_packet packet={};
                    if(gh_actor_world_queue_packet_at(app.actor_queue,i,j,&packet)!=GH_ACTOR_WORLD_OK)return GH_ID_ARGUMENT;
                    if((packet.primitive&32)&&!fog_known){app.world_frontier=0x14c5a0;return GH_ID_UNSUPPORTED;}
                    gh_model_draw_state state={};state.test=g.command.test;state.alpha=g.command.alpha;state.tex1=g.command.tex1;state.clamp=g.command.clamp;state.primitive=packet.primitive;
                    state.texture_function=(uint8_t)((packet.tex0>>35)&3);
                    state.xy_offset_x16=(2048-display->screen_width/2)*16;state.xy_offset_y16=display->gs_y_origin*16;state.fog_rgb=fog_rgb;
                    state.zbuf_psm=58;state.z_write=state.colclamp=state.native_float_reference=1;
                    if(packet.samples_display_base0){
                        if(!app.display_base0_known){app.world_frontier=0x30b870;return GH_ID_UNSUPPORTED;}
                        app.world_gpu_error=gh_model_renderer_draw_display_texel(app.world_renderer,&state,app.display_base0_rgb,
                            packet.vertices,packet.vertex_count,packet.indices,packet.triangles*3);
                    }else{
                        uint32_t texture=0;
                        app.world_gpu_error=gh_model_renderer_upload(app.world_renderer,packet.material,&texture);
                        if(app.world_gpu_error!=GH_MODEL_GPU_OK){app.world_frontier=0x151368;return GH_ID_UNSUPPORTED;}
                        app.world_gpu_error=gh_model_renderer_draw(app.world_renderer,&state,texture,packet.vertices,packet.vertex_count,packet.indices,packet.triangles*3);
                    }
                    if(app.world_gpu_error!=GH_MODEL_GPU_OK){
#ifdef GH_NATIVE_STARTUP_QA
                        qa_actor_gpu_failure(g,packet,state);
#endif
                        app.world_frontier=0x31a650;return GH_ID_UNSUPPORTED;}
                    if(packet.triangles)++app.world_packets;
                }
                continue;
            }
            size_t i=encoded;gh_static_world_group g={};
            if(gh_static_world_queue_group_at(app.world_queue,i,&g)!=GH_WORLD_OK)return GH_ID_ARGUMENT;
            if(g.command.kind==GH_OBJECT_SETUP){setup=g;has_setup=true;fog_rgb=g.captured_fog_rgb;fog_known=true;continue;}
            if(!has_setup||setup.command.handle!=g.command.handle||setup.command.object!=g.command.object){app.world_frontier=0x14c6b8;return GH_ID_UNSUPPORTED;}
            sampler.write(g.command.tex1,g.command.clamp);
            for(size_t j=0;j<g.packets;j++){gh_static_world_packet packet={};
                if(gh_static_world_queue_packet_at(app.world_queue,i,j,&packet)!=GH_WORLD_OK)return GH_ID_ARGUMENT;
                uint32_t texture=0;app.world_gpu_error=gh_model_renderer_upload(app.world_renderer,packet.material,&texture);
                if(app.world_gpu_error!=GH_MODEL_GPU_OK){app.world_frontier=0x151368;return GH_ID_UNSUPPORTED;}
                gh_model_draw_state state={};state.test=setup.command.test;state.alpha=setup.command.alpha;
                state.tex1=sampler.tex1;state.clamp=sampler.clamp;state.primitive=packet.primitive;
                state.xy_offset_x16=(2048-display->screen_width/2)*16;state.xy_offset_y16=display->gs_y_origin*16;
                state.fog_rgb=fog_rgb;
                // Explicit native CT32/Z16S/FBA0 initial surface reference,
                // shared with the bounded startup UI backend, not boot GS proof.
                state.zbuf_psm=58;state.z_write=state.colclamp=state.native_float_reference=1;
                app.world_gpu_error=gh_model_renderer_draw(app.world_renderer,&state,texture,packet.vertices,packet.vertex_count,packet.indices,packet.triangles*3);
                if(app.world_gpu_error!=GH_MODEL_GPU_OK){app.world_frontier=0x14b810;return GH_ID_UNSUPPORTED;}
                if(packet.triangles)++app.world_packets;
            }
        }
        app.world_pixels.resize(512*448*4);
        if(message_depth||(app.frame_fade_seen&&app.frame_fade.test==0x70000))world_depth.resize(512*448);
        app.world_gpu_error=gh_model_renderer_readback(app.world_renderer,app.world_pixels.data(),app.world_pixels.size(),
            world_depth.empty()?nullptr:world_depth.data(),world_depth.size());
        if(app.world_gpu_error!=GH_MODEL_GPU_OK)return GH_ID_UNSUPPORTED;
    }
    gh_id_error error=!world_surface?gh_mc_renderer_flush_queued(app.renderer,ui_order.data(),ui_order.size(),clear):
        world_depth.empty()?gh_mc_renderer_flush_queued_on_background(app.renderer,ui_order.data(),ui_order.size(),
            app.world_pixels.data(),app.world_pixels.size()):
        gh_mc_renderer_flush_queued_on_background_with_depth(app.renderer,ui_order.data(),ui_order.size(),
            app.world_pixels.data(),app.world_pixels.size(),world_depth.data(),world_depth.size());
#ifdef GH_NATIVE_STARTUP_QA
    if(app.frame_fade_seen&&app.frame_fade.test==0x70000)
        std::fprintf(stderr,"QA_FADE_DEPTH scene%llu state%02X test%05llX uiGroups%llu depthPixels%llu result%u\n",
            (unsigned long long)app.snapshot.scene_frame_serial,(unsigned)(app.snapshot.fade?app.snapshot.fade->state:0),
            (unsigned long long)app.frame_fade.test,(unsigned long long)ui_order.size(),(unsigned long long)world_depth.size(),(unsigned)error);
#endif
    if(error!=GH_ID_OK)return error;
    size_t bytes=0;const unsigned char *base=gh_mc_renderer_pixels(app.renderer,&bytes);
    if(!base||bytes!=512*448*4)return GH_ID_UNSUPPORTED;
    app.queued_pixels.assign(base,base+bytes);app.display_pending=true;
    app.pending_scene=scene;app.pending_fade_serial=app.snapshot.fade_draw_serial;
    app.pending_mc_serial=app.snapshot.mc_frame_serial;
    if(scene)app.prepared_scene_serial=app.snapshot.scene_frame_serial;
    else app.prepared_mc_serial=app.snapshot.mc_frame_serial;
    app.frame_building=false;return GH_ID_OK;
    }catch(const std::bad_alloc&){return GH_ID_MEMORY;}
}
static void usage()
{
    std::fprintf(stderr,"usage: godhand-windows --iso ISO --profile DIRECTORY [--ffmpeg EXE] [--show|--hidden] [--muted] [--prefix] [--max-ticks N]\n");
}
#ifdef GH_NATIVE_STARTUP_QA
static constexpr unsigned qa_max_capture_count=120;

static std::string qa_capture_path(const char *base,unsigned captured,uint64_t serial)
{
    std::string path(base);
    if(!captured)return path;
    const std::string suffix=".scene-"+std::to_string(serial)+".rgba";
    constexpr size_t extension_size=5;
    if(path.size()>=extension_size&&path.compare(path.size()-extension_size,extension_size,".rgba")==0)
        path.resize(path.size()-extension_size);
    return path+suffix;
}

static bool qa_write_capture(const std::string &path,const std::vector<unsigned char> &pixels)
{
    FILE *file=std::fopen(path.c_str(),"wbx");
    if(!file)return false;
    bool complete=std::fwrite(pixels.data(),1,pixels.size(),file)==pixels.size();
    return std::fclose(file)==0&&complete;
}
#endif
int main(int argc,char **argv)
{
    const char *iso=nullptr,*profile=nullptr,*ffmpeg=nullptr;
    bool show=false,muted=false,prefix=false;uint64_t max_ticks=0;
#ifdef GH_NATIVE_STARTUP_QA
    bool qa_confirm=false,qa_new_game=false;unsigned qa_choice_step=0,qa_captured=0,qa_capture_count=1;gh_native_input qa_input={};
    const char *qa_capture=nullptr;uint64_t qa_capture_frame=1;uint32_t qa_title_trace=UINT32_MAX;
#endif
    for(int i=1;i<argc;++i){
        if(!std::strcmp(argv[i],"--show"))show=true;
        else if(!std::strcmp(argv[i],"--hidden"))show=false;
#ifdef GH_NATIVE_STARTUP_QA
        else if(!std::strcmp(argv[i],"--qa-new-game")){qa_new_game=true;qa_confirm=true;}
        else if(!std::strcmp(argv[i],"--qa-capture")&&i+1<argc)qa_capture=argv[++i];
        else if(!std::strcmp(argv[i],"--qa-capture-frame")&&i+1<argc)qa_capture_frame=std::strtoull(argv[++i],nullptr,10);
        else if(!std::strcmp(argv[i],"--qa-capture-count")&&i+1<argc){
            char *end=nullptr;
            const auto value=std::strtoull(argv[++i],&end,10);
            if(!value||value>qa_max_capture_count||!end||*end){usage();return 2;}
            qa_capture_count=static_cast<unsigned>(value);
        }
#endif
        else if(!std::strcmp(argv[i],"--muted"))muted=true;
        else if(!std::strcmp(argv[i],"--prefix"))prefix=true;
#ifdef GH_NATIVE_STARTUP_QA
        else if(!std::strcmp(argv[i],"--qa-confirm"))qa_confirm=true;
#endif
        else if((!std::strcmp(argv[i],"--iso")||!std::strcmp(argv[i],"--profile")||!std::strcmp(argv[i],"--ffmpeg")||!std::strcmp(argv[i],"--max-ticks"))&&i+1<argc){
            const char *key=argv[i++];
            if(!std::strcmp(key,"--iso"))iso=argv[i];
            else if(!std::strcmp(key,"--profile"))profile=argv[i];
            else if(!std::strcmp(key,"--ffmpeg"))ffmpeg=argv[i];
            else {char *end=nullptr;max_ticks=std::strtoull(argv[i],&end,10);if(!max_ticks||!end||*end){usage();return 2;}}
        }else {usage();return 2;}
    }
    if(!iso||!profile){usage();return 2;}
#ifdef GH_NATIVE_STARTUP_QA
    if(qa_confirm&&(!muted||show||!max_ticks)){std::fprintf(stderr,"Synthetic input QA requires --muted --hidden and --max-ticks.\n");return 2;}
#endif
    bool synthetic_input=false;
#ifdef GH_NATIVE_STARTUP_QA
    synthetic_input=qa_confirm;
#endif
    NativeApp app;gh_disc *disc=nullptr;int result=1;gh_host_error host_error=GH_HOST_OK;
    gh_frame_clock clock={};
    do {
        if(gh_disc_open(iso,&disc)!=GH_DISC_OK)break;
        app.elf.resize((size_t)gh_disc_boot_elf_size(disc));
        if(app.elf.empty()||gh_disc_read_boot_elf(disc,0,app.elf.data(),app.elf.size())!=GH_DISC_OK)break;
        gh_disc_close(disc);disc=nullptr;
        gh_window_config window={};window.width=640;window.height=480;window.show=show;
        window.title="God Hand native source port";window.xinput_index=0;
        gh_native_input_defaults(&window.input);
        // Actual US boot language1, phase569=2: second original mapping table,
        // repeat multiplier1. Other language/boot modes need their own profile.
        if(!gh_native_input_map_from_elf(app.elf.data(),app.elf.size(),1,&window.map))break;
#ifdef GH_NATIVE_STARTUP_QA
        if(qa_confirm){
            if(!gh_native_input_init(&qa_input,&window.input,window.map))break;
            gh_native_input_focus(&qa_input,1);
            std::fprintf(stderr,"QA fixture: synthetic digital keys through the original ELF mapping; no user input claimed.\n");
        }
#endif
        app.window_error=gh_window_create(&window,&app.window);
        if(app.window_error!=GH_WINDOW_OK)break;
        if(gh_mc_renderer_create(app.elf.data(),app.elf.size(),1,&app.renderer)!=GH_ID_OK)break;
        if(gh_mc_renderer_enable_reference_polygons(app.renderer,1)!=GH_ID_OK)break;
        if(gh_static_world_queue_create(&app.world_queue)!=GH_WORLD_OK)break;
        if(gh_actor_world_queue_create(&app.actor_queue)!=GH_ACTOR_WORLD_OK)break;
        host_error=gh_startup_host_create_with_profile(iso,profile,1,0,64*1024*1024,&app.host);
        if(host_error!=GH_HOST_OK)break;
        host_error=gh_startup_host_set_mc_consumers(app.host,draw_mc,&app,post_mc,&app);
        if(host_error!=GH_HOST_OK)break;
        host_error=gh_startup_host_set_mc_frame_begin(app.host,begin_mc,&app);
        if(host_error!=GH_HOST_OK)break;
        host_error=gh_startup_host_set_fade_consumer(app.host,draw_fade,&app);
        if(host_error!=GH_HOST_OK)break;
        gh_host_scene_ui_callbacks scene_callbacks={&app,begin_scene,draw_scene,draw_fan,draw_cover,draw_scene_message};
        host_error=gh_startup_host_set_scene_ui_consumers(app.host,&scene_callbacks);
        if(host_error!=GH_HOST_OK)break;
        host_error=gh_startup_host_set_world_consumer(app.host,queue_world,&app);
        if(host_error!=GH_HOST_OK)break;
        host_error=gh_startup_host_set_shadow_consumer(app.host,queue_shadow,&app);
        if(host_error!=GH_HOST_OK)break;
        host_error=gh_startup_host_set_actor_consumer(app.host,queue_actor,&app);
        if(host_error!=GH_HOST_OK)break;
        host_error=gh_startup_host_set_effect_consumer(app.host,queue_effect,&app);
        if(host_error!=GH_HOST_OK)break;
        if(!prefix){
            gh_startup_continuation config={};config.enabled=1;config.muted=muted;
            config.audio_dsp_policy=GH_SE_DSP_NATIVE_LINEAR_REFERENCE;
            config.ffmpeg_path=ffmpeg;config.movie_present=present_movie;config.movie_present_context=&app;
            host_error=gh_startup_host_set_continuation(app.host,&config);
            if(host_error!=GH_HOST_OK)break;
        }
        if(!gh_frame_clock_init(&clock,30000,1001))break;
        host_error=gh_startup_host_snapshot(app.host,&app.snapshot);if(host_error!=GH_HOST_OK)break;
        for(;;){
            gh_native_input_snapshot input={};app.window_error=gh_window_poll_device(app.window,app.snapshot.work,&input);
            if(app.window_error!=GH_WINDOW_OK){result=app.window_error==GH_WINDOW_CLOSED?0:1;break;}
            // The original outer loop submits the previous parity's OT before
            // updating tasks. Keep the completed bitmap, not borrowed resources.
            if(app.display_pending){
                app.window_error=gh_window_present(app.window,app.queued_pixels.data(),app.queued_pixels.size(),512,448,4,3);
                if(app.window_error!=GH_WINDOW_OK)break;
                // The original display copy follows the complete frame OT.
                // Its base-zero texel is visible to the following frame.
                app.display_base0_rgb=gh_display_copy_base0_0030b870(app.queued_pixels.data(),app.queued_pixels.data()+512*4);
                app.display_base0_known=true;
                app.frame_serial=app.pending_mc_serial;app.presented_fade_serial=app.pending_fade_serial;
                if(app.pending_scene)++app.presented_scene;else ++app.presented_mc;
                app.display_pending=false;
            }
#ifdef GH_NATIVE_STARTUP_QA
            if(qa_confirm){
                unsigned key=0;
                if(app.snapshot.mc_phase==3&&app.snapshot.mc_subphase==1)key='Z';
                else if(app.snapshot.mc_phase==3&&app.snapshot.mc_subphase==2){
                    if(qa_choice_step==0){key=39;++qa_choice_step;}
                    else if(qa_choice_step==1){key='Z';++qa_choice_step;}
                }else qa_choice_step=0;
                if(qa_new_game&&app.snapshot.title_live){
                    if(app.snapshot.title_mode==1&&app.snapshot.ticks%30==0)key='Z';
                    else if(app.snapshot.title_mode==3&&app.snapshot.title_phase>=1&&app.snapshot.title_phase<=3
                        &&app.snapshot.title_subphase==1&&(app.snapshot.title_phase==1||app.snapshot.title_timer>=5)&&app.snapshot.ticks%6==0)key='Z';
                }
                gh_native_input_key(&qa_input,'Z',key=='Z');gh_native_input_key(&qa_input,39,key==39);
                const gh_native_gamepad_sample neutral={};
                if(!gh_native_input_tick_device(&qa_input,app.snapshot.work,&neutral,&input))break;
            }
#endif
            host_error=gh_startup_host_set_input(app.host,&input);if(host_error!=GH_HOST_OK)break;
            gh_host_phase phase=gh_startup_host_tick(app.host);
            host_error=gh_startup_host_snapshot(app.host,&app.snapshot);if(host_error!=GH_HOST_OK)break;
#ifdef GH_NATIVE_STARTUP_QA
            if(app.snapshot.title_live){uint32_t trace=(uint32_t)app.snapshot.title_mode<<16|(uint32_t)app.snapshot.title_phase<<8|app.snapshot.title_subphase;
                if(trace!=qa_title_trace){std::fprintf(stderr,"QA Title tick%llu move%llu mode%u phase%u sub%u timer%u selection%u level%u\n",
                    (unsigned long long)app.snapshot.ticks,(unsigned long long)app.snapshot.title_move_serial,
                    app.snapshot.title_mode,app.snapshot.title_phase,app.snapshot.title_subphase,
                    app.snapshot.title_timer,app.snapshot.title_game_selection,app.snapshot.title_level);qa_title_trace=trace;}
            }
#endif
            if(app.snapshot.mc_frame_ready&&app.snapshot.mc_frame_serial!=app.prepared_mc_serial){
                gh_id_error error=prepare_ui_display(app,false);
                if(error!=GH_ID_OK){std::fprintf(stderr,"Native MC OT preparation failed: %u\n",(unsigned)error);break;}
            }else if(app.snapshot.scene_frame_ready&&app.snapshot.scene_frame_serial!=app.prepared_scene_serial){
                gh_id_error error=prepare_ui_display(app,true);
                if(error!=GH_ID_OK){std::fprintf(stderr,"Native scene OT preparation failed: %u\n",(unsigned)error);break;}
#ifdef GH_NATIVE_STARTUP_QA
                if(qa_capture&&qa_captured<qa_capture_count&&app.snapshot.scene_frame_serial>=qa_capture_frame){
                    if(qa_capture_count>1&&app.world_pixels.empty()){
                        std::fprintf(stderr,"QA capture range requires the matching pre-UI world frame\n");break;
                    }
                    const std::string path=qa_capture_path(qa_capture,qa_captured,app.snapshot.scene_frame_serial);
                    if(!qa_write_capture(path,app.queued_pixels)){
                        std::fprintf(stderr,"QA scene capture failed\n");break;
                    }
                    if(!app.world_pixels.empty()&&!qa_write_capture(path+".world.rgba",app.world_pixels)){
                        std::fprintf(stderr,"QA pre-UI world capture failed\n");break;
                    }
                    ++qa_captured;
                    std::fprintf(stderr,"QA_WORLD_CAPTURE {\"scene_serial\":%llu,\"world_bytes\":%llu,\"fade_visible\":%u,\"fade_rgba\":\"%08X\",\"fade_alpha\":\"%016llX\"}\n",
                        (unsigned long long)app.snapshot.scene_frame_serial,(unsigned long long)app.world_pixels.size(),app.frame_fade_seen?1u:0u,app.frame_fade.rgba,(unsigned long long)app.frame_fade.alpha);
                    std::fprintf(stderr,"QA scene capture: frame%llu, 512x448 rawRGBA\n",(unsigned long long)app.snapshot.scene_frame_serial);
                }
#endif
            }
            if(phase!=GH_HOST_RUNNING){
                // Prefix mode is a bounded developer check. Full execution
                // must report a frontier as unfinished, not successful play.
                result=prefix&&phase==GH_HOST_FRONTIER&&app.presented_mc>0?0:4;break;
            }
            if(max_ticks&&app.snapshot.ticks>=max_ticks){result=3;break;}
            app.window_error=gh_window_wait_frame(app.window,&clock);
            if(app.window_error!=GH_WINDOW_OK){result=app.window_error==GH_WINDOW_CLOSED?0:1;break;}
        }
    }while(false);
    if(disc)gh_disc_close(disc);
    gh_mc_render_status renderer={};gh_mc_renderer_inspect(app.renderer,&renderer);
    uint32_t system_flags5d8=0,work_flags58c=0,work_flags594=0,work_flags598=0,work_flags5e0=0,work_flags5e4=0;
    if(app.snapshot.work){const unsigned char*w=(const unsigned char*)app.snapshot.work;
        std::memcpy(&system_flags5d8,w+0x5d8,4);std::memcpy(&work_flags58c,w+0x58c,4);std::memcpy(&work_flags594,w+0x594,4);
        std::memcpy(&work_flags598,w+0x598,4);std::memcpy(&work_flags5e0,w+0x5e0,4);std::memcpy(&work_flags5e4,w+0x5e4,4);}

    std::printf("{\"scope\":\"native original startup execution; unfinished source port\","
        "\"playable_game_verified\":false,\"hidden\":%s,\"muted\":%s,\"prefix_mode\":%s,\"synthetic_input\":%s,"
        "\"ticks\":%llu,\"host_phase\":%u,\"host_error\":%u,\"subsystem_error\":%d,"
        "\"frontier_module\":\"%s\",\"frontier\":\"%08X\","
        "\"mc_frames_presented\":%llu,\"mc_frame_serial\":%llu,\"mc_ui_live\":%s,\"mc_ui_epoch\":%llu,"
        "\"movie_frames_presented\":%llu,\"movie_eof\":%s,\"movie_audio_done\":%s,\"movie_complete\":%s,"
        "\"mc_phase\":%u,\"mc_subphase\":%u,\"game_state\":%u,\"assets_loaded\":%s,\"room\":\"%04X\",\"next_room\":\"%04X\","
        "\"work_flags584\":\"%08X\",\"system_flags5d8\":\"%08X\","
        "\"work_flags58c\":\"%08X\",\"work_flags594\":\"%08X\",\"work_flags598\":\"%08X\",\"work_flags5e0\":\"%08X\",\"work_flags5e4\":\"%08X\","
        "\"scene_frames_presented\":%llu,\"scene_frame_serial\":%llu,"
        "\"renderer_error\":%u,\"renderer_frontier\":\"%08X\",\"world_gpu_error\":%u,\"world_frontier\":\"%08X\",\"world_vu_pc\":\"%08X\",\"world_packets_submitted\":%llu,\"effect_post_packets\":%llu,\"effect_sprite_packets\":%llu,\"effect_post_error\":%u,\"window_error\":%u,\"exit_code\":%d,"
        "\"player_updates_completed\":%llu,\"child_updates_completed\":%llu,\"om_updates_completed\":%llu,\"last_move_handle\":%u,\"last_move_entry\":\"%08X\",\"player_update_stage\":%u,\"child_update_stage\":%u,"
        "\"child_update_frontier\":\"%08X\",\"child_forces_complete\":%u,\"child_angles_complete\":%u,\"child_distances_complete\":%u,\"child_finish_complete\":%u,"
        "\"emitter_active\":%u,\"esp_active\":%u,"
        "\"hitarea_diagnostic_ready\":%s,\"scene_stage\":%u,\"hitarea_phase\":%u,\"hitarea_slot\":%u,\"hitarea_node\":%u,"
        "\"hitarea_shape_index\":%u,\"hitarea_shape_flags\":\"%08X\",\"hitarea_shape_kind\":%u,\"hitarea_hit_count\":%d,"
        "\"hitarea_hit_samples\":%u,\"hitarea_hit_indices\":[%u,%u,%u,%u],\"hitarea_hit_kinds\":[%u,%u,%u,%u],\"hitarea_hit_distance_bits\":[%u,%u,%u,%u]}\n",
        show?"false":"true",muted?"true":"false",prefix?"true":"false",synthetic_input?"true":"false",
        (unsigned long long)app.snapshot.ticks,(unsigned)app.snapshot.phase,
        (unsigned)(host_error!=GH_HOST_OK?host_error:app.snapshot.error),app.snapshot.subsystem_error,
        app.snapshot.frontier_module?app.snapshot.frontier_module:"main ELF",app.snapshot.unavailable_entry,
        (unsigned long long)app.presented_mc,(unsigned long long)app.snapshot.mc_frame_serial,
        app.snapshot.mc_ui_live?"true":"false",(unsigned long long)app.snapshot.mc_ui_epoch,
        (unsigned long long)app.presented_movie,app.snapshot.movie_video.video_eof?"true":"false",
        app.snapshot.movie_video.audio_done?"true":"false",app.snapshot.movie_video.complete?"true":"false",
        app.snapshot.mc_phase,app.snapshot.mc_subphase,(unsigned)app.snapshot.game_state,
        app.snapshot.game_assets_loaded?"true":"false",app.snapshot.current_room,app.snapshot.next_room,app.snapshot.flags,system_flags5d8,
        work_flags58c,work_flags594,work_flags598,work_flags5e0,work_flags5e4,
        (unsigned long long)app.presented_scene,(unsigned long long)app.snapshot.scene_frame_serial,(unsigned)renderer.id_error,renderer.frontier,
        (unsigned)app.world_gpu_error,app.world_frontier,app.world_vu_pc,(unsigned long long)app.world_packets,
        (unsigned long long)app.effect_post_packets,(unsigned long long)app.effect_sprite_packets,(unsigned)app.effect_post_error,
        (unsigned)app.window_error,result,
        (unsigned long long)app.snapshot.player_updates_completed,(unsigned long long)app.snapshot.child_updates_completed,(unsigned long long)app.snapshot.om_updates_completed,
        app.snapshot.last_move_handle,app.snapshot.last_move_entry,app.snapshot.player_update_stage,app.snapshot.child_update_stage,
        app.snapshot.child_update_frontier,app.snapshot.child_forces_complete,app.snapshot.child_angles_complete,app.snapshot.child_distances_complete,app.snapshot.child_finish_complete,
        app.snapshot.emitter_active,app.snapshot.esp_active,
        app.snapshot.hitarea_diagnostic_ready?"true":"false",
        app.snapshot.scene_stage,app.snapshot.hitarea_phase,app.snapshot.hitarea_slot,app.snapshot.hitarea_node,
        app.snapshot.hitarea_shape_index,app.snapshot.hitarea_shape_flags,app.snapshot.hitarea_shape_kind,app.snapshot.hitarea_hit_count,
        (unsigned)app.snapshot.hitarea_hit_samples,
        app.snapshot.hitarea_hit_indices[0],app.snapshot.hitarea_hit_indices[1],app.snapshot.hitarea_hit_indices[2],app.snapshot.hitarea_hit_indices[3],
        app.snapshot.hitarea_hit_kinds[0],app.snapshot.hitarea_hit_kinds[1],app.snapshot.hitarea_hit_kinds[2],app.snapshot.hitarea_hit_kinds[3],
        app.snapshot.hitarea_hit_distance_bits[0],app.snapshot.hitarea_hit_distance_bits[1],app.snapshot.hitarea_hit_distance_bits[2],app.snapshot.hitarea_hit_distance_bits[3]);
#ifdef GH_NATIVE_STARTUP_QA
    if(qa_new_game&&app.snapshot.fade){
        const auto &d=app.snapshot;const auto &fade=*d.fade;uint32_t shift=0;
        if(d.work)std::memcpy(&shift,(const uint8_t*)d.work+0x564,4);
        std::fprintf(stderr,"QA_EVENT_BOUNDARY number%d flags%08X state%u phase%u config%08X callee%d clearStage%u clearDetail%d audioFlags%X cameraCuts%u actorRecords%u timingRows%u actorHandle%u actorParts%u actorsInitialized%u accessoryHandle%u jacketAttached%u fadeState%02X count%u time%u from%08X to%08X shift%u\n",
            d.event_number,d.event_flags,(unsigned)d.event_state,(unsigned)d.event_phase,d.event_config_flags,d.event_callee_error,
            d.event_clear_stage,d.event_clear_detail,d.event_audio_flags,d.event_camera_cuts,d.event_actor_records,d.event_timing_rows,
            d.event_actor_handle,d.event_actor_parts,d.event_actors_initialized,d.event_actor_accessory_handle,
            (unsigned)d.event_actor_jacket_attached,(unsigned)fade.state,(unsigned)fade.count,(unsigned)fade.time,
            fade.color_from,fade.color_to,shift);
        std::fprintf(stderr,"QA_EVENT_PLAYBACK cut%d frame%.3f motionCamera%u\n",
            d.event_cut,(double)d.event_cut_frame,(unsigned)d.event_motion_camera_active);
        if(d.effect_frontier)std::fprintf(stderr,
            "QA_EFFECT_FRONTIER {\"pc\":\"%08X\",\"emitter_slot\":%u,\"emitter_serial\":%u,"
            "\"emitter_mode\":%u,\"particle_class\":%u,\"emitter_step\":%u,\"esp_slot\":%u,\"esp_step\":%u}\n",
            d.effect_frontier,d.effect_emitter_slot,d.effect_emitter_serial,d.effect_emitter_mode,
            d.effect_emitter_class,d.effect_emitter_step,d.effect_esp_slot,d.effect_esp_step);
    }
    if(qa_new_game)for(unsigned i=0;i<app.snapshot.emitter_samples;i++)std::fprintf(stderr,"QA Emitter slot%u bank%u kind%u flags%08X known%u type%u\n",app.snapshot.emitter_slots[i],app.snapshot.emitter_banks[i],app.snapshot.emitter_kinds[i],app.snapshot.emitter_flags[i],app.snapshot.emitter_known[i],app.snapshot.emitter_types[i]);
    if(qa_new_game&&app.snapshot.current_actor_ready){
        const auto&d=app.snapshot;std::fprintf(stderr,"QA CurrentActor kind%04X flags250%08X state%04X 5B0%08X 5B4%08X 5B8%08X HP548%04X HP54A%04X cooldown54C%08X 5C0XYZ%08X,%08X,%08X\n",d.current_actor_kind,d.current_actor_flags,d.current_actor_state,d.current_actor_5b0,d.current_actor_5b4,d.current_actor_5b8,d.current_actor_hp548,d.current_actor_hp54a,d.current_actor_54c,d.current_actor_5c0[0],d.current_actor_5c0[1],d.current_actor_5c0[2]);
        for(unsigned i=0;i<d.current_b_samples;i++){std::fprintf(stderr,"QA CurrentB index%u group%u generation%u serial%u source_present%u source_known%u source_kind%04X words00..6C",d.current_b_index[i],d.current_b_group[i],d.current_b_generation[i],d.current_b_serial[i],d.current_b_source_present[i],d.current_b_source_known[i],d.current_b_source_kind[i]);for(unsigned j=0;j<28;j++)std::fprintf(stderr," %08X",d.current_b_words[i][j]);std::fprintf(stderr,"\n");}
    }
    if(qa_new_game&&app.snapshot.hitarea_diagnostic_ready){
        std::fprintf(stderr,"QA HitArea source slot%u actor_present%u actor_kind%04X flags250%08X cooldown54C%08X unitflags%08X excluded38%08X sameplayer%u\n",app.snapshot.hitarea_slot,app.snapshot.hitarea_actor_present,app.snapshot.hitarea_actor_kind,
            app.snapshot.hitarea_actor_flags,app.snapshot.hitarea_actor_cooldown_bits,app.snapshot.hitarea_source_unit_flags,app.snapshot.hitarea_source_excluded_token,app.snapshot.hitarea_source_is_player);
        for(unsigned i=0;i<app.snapshot.hitarea_hit_samples;i++)std::fprintf(stderr,"QA HitArea target%u shape%u unit_known%u slot%u actor_present%u actor_kind%04X raw30%08X flags250%08X cooldown54C%08X same_source_actor%u\n",
            i,app.snapshot.hitarea_hit_indices[i],app.snapshot.hitarea_hit_unit_known[i],app.snapshot.hitarea_hit_unit_slots[i],
            app.snapshot.hitarea_hit_actor_present[i],app.snapshot.hitarea_hit_actor_kinds[i],app.snapshot.hitarea_hit_actor_tokens[i],
            app.snapshot.hitarea_hit_actor_flags[i],app.snapshot.hitarea_hit_actor_cooldown_bits[i],app.snapshot.hitarea_hit_same_actor[i]);
    }
#endif
    if(app.host){gh_host_error cleanup=gh_startup_host_destroy(app.host);if(cleanup!=GH_HOST_OK){std::fprintf(stderr,"Native host cleanup failed: %u\n",(unsigned)cleanup);result=1;}}
    gh_static_world_queue_destroy(app.world_queue);gh_actor_world_queue_destroy(app.actor_queue);gh_model_renderer_destroy(app.world_renderer);
    gh_mc_renderer_destroy(app.renderer);gh_window_destroy(app.window);
    return result;
}
