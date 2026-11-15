#include "game_om311.h"
#include "game_actor_model_nodes.h"
#include "game_om_base.h"

#include <fenv.h>
#include <math.h>
#include <string.h>

enum { ANCHOR_CREATE, ANCHOR_CONFIGURE, BOXES_CREATE, BOXES_CONFIGURE, MODEL_LAYOUT, READY };

static uint32_t word(const uint8_t *p)
{
    uint32_t value;
    memcpy(&value,p,sizeof(value));
    return value;
}

static void put(uint8_t *p,uint32_t value)
{
    memcpy(p,&value,sizeof(value));
}

static void half(uint8_t *p,uint16_t value)
{
    memcpy(p,&value,sizeof(value));
}

static gh_scene_placement_status stop(gh_om311_state *s,uint32_t pc,gh_scene_placement_status status)
{
    s->frontier=pc;
    return status;
}

static gh_scene_placement_status hit_status(gh_om311_state *s,uint32_t pc,gh_hitarea_status status)
{
    return stop(s,pc,status==GH_HITAREA_MEMORY?GH_PLACE_MEMORY:GH_PLACE_UNSUPPORTED);
}

gh_scene_placement_status gh_om311_construct_001901c8(void *storage,size_t size,gh_title_player_view *out)
{
    if(!storage||!out)return GH_PLACE_ARGUMENT;
    if(size<GH_OM311_BYTES)return GH_PLACE_BOUNDS;
    gh_scene_placement_status status=gh_om_base_construct_001b6e90(storage,size,out);
    if(status)return status;
    uint8_t *bytes=storage;
    put(bytes+0x214,0x423f58);
    out->original_vtable214=0x423f58;
    for(unsigned i=0;i<GH_OM311_FRAGMENTS;++i){
        uint8_t *fragment=bytes+0x620+(size_t)i*0x60;
        for(unsigned j=0;j<5;++j){
            memset(fragment+16*j,0,12);
            put(fragment+16*j+12,0x3f800000);
        }
        fragment[0x54]=0;
    }
    put(bytes+0xf80,0);
    return GH_PLACE_OK;
}

/* 424010 assigns ordinary/broken layers 0..4 and 40..44, then five
 * fragment groups. The last pair says parts 12..19 relative to part 24. */
static gh_scene_placement_status model_layout(gh_om311_state *s,const gh_actor_part *anchor)
{
    gh_actor_model *model=&s->common.model;
    if(model->part_count!=GH_OM311_FRAGMENTS)
        return stop(s,0x14bfe0,GH_PLACE_UNSUPPORTED);
    for(unsigned i=0;i<model->script.mesh_count;++i){
        const gh_actor_node_packets *packet;
        if(gh_actor_model_packet_at(model,i,&packet))
            return stop(s,0x14b6f8,GH_PLACE_BOUNDS);
        uint8_t *node=packet->node;
        const unsigned layer=node[0x40c];
        if(layer<5)put(node+0x380,word(node+0x380)&~1u);
        if(layer>=0x40&&layer<0x45)put(node+0x380,word(node+0x380)|1u);
    }
    for(unsigned i=0;i<20;++i)
        put(model->parts[i].bytes+0x154,word(model->parts[i].bytes+0x154)|0x18u);

    float origin[3],offsets[8][3];
    memcpy(origin,anchor->bytes+0x30,sizeof(origin));
    for(unsigned i=0;i<8;++i){
        memcpy(offsets[i],model->parts[12+i].bytes+0x30,sizeof(offsets[i]));
        for(unsigned axis=0;axis<3;++axis)
            if(!isfinite(origin[axis])||!isfinite(offsets[i][axis]))
                return stop(s,0x147c60,GH_PLACE_UNSUPPORTED);
    }
    fenv_t saved;
    if(fegetenv(&saved)||fesetround(FE_TONEAREST))
        return stop(s,0x147c40,GH_PLACE_UNSUPPORTED);
    for(unsigned i=0;i<8;++i){
        for(unsigned axis=0;axis<3;++axis){
            volatile float coordinate=offsets[i][axis]-origin[axis];
            offsets[i][axis]=coordinate;
        }
    }
    if(fesetenv(&saved))return stop(s,0x147c40,GH_PLACE_UNSUPPORTED);
    for(unsigned i=0;i<8;++i)
        memcpy(model->parts[12+i].bytes+0x30,offsets[i],sizeof(offsets[i]));
    return GH_PLACE_OK;
}

gh_scene_placement_status gh_om311_initialize_001902f8(gh_om311_state *s,gh_om_object *object,
    const gh_om_initializer_bindings *bindings,int *out)
{
    if(!s||!object||object->source.type!=0x311||!bindings||!out)return GH_PLACE_ARGUMENT;
    *out=0;
    s->frontier=0;
    int common_return;
    gh_scene_placement_status status=gh_om_common_initialize_001b6fb8(&s->common,object,bindings,&common_return);
    if(status||!common_return)
        return stop(s,s->common.frontier?s->common.frontier:0x1b6fb8,status?status:GH_PLACE_UNSUPPORTED);
    if(s->stage==READY){*out=1;return GH_PLACE_OK;}
    const gh_actor_part *anchor;
    if(gh_actor_model_part_at(&s->common.model,24,&anchor))
        return stop(s,0x14bfe0,GH_PLACE_BOUNDS);
    if(!bindings->init||!bindings->shapes)
        return stop(s,0x1fe0e8,GH_PLACE_UNSUPPORTED);
    uint8_t *bytes=object->view.bytes;
    uint32_t pc=0;
    int nonnull;
    const uint32_t zero[3]={0};
    if(s->stage==ANCHOR_CREATE){
        gh_hitarea_status hit=gh_hitarea_object_create_001fe0e8(bindings->init,&object->view,2,
            &s->anchor_box,&nonnull,&pc);
        if(hit)return hit_status(s,pc?pc:0x1fe0e8,hit);
        s->anchor_created=(uint8_t)nonnull;
        put(bytes+0x610,0);
        s->stage=ANCHOR_CONFIGURE;
    }
    if(s->stage==ANCHOR_CONFIGURE){
        if(s->anchor_created){
            const uint32_t extent[3]={0x40400000,0x3f000000,0x41000000};
            int index;
            gh_hitarea_status hit=gh_hitarea_add_box_001fd9d8(&s->anchor_box,bindings->shapes,
                anchor->bytes+0x80,zero,zero,extent,&index,&pc);
            if(hit)return hit_status(s,pc?pc:0x1fd9d8,hit);
            half(s->anchor_box.bytes+0x46,0x53);
            put(s->anchor_box.bytes+0x4c,10);
        }
        s->stage=BOXES_CREATE;
    }
    if(s->stage==BOXES_CREATE){
        if(!s->cursor&&s->anchor_created){
            gh_hitarea_status hit=gh_hitarea_object_set_active_001fd668(&s->anchor_box,0,&pc);
            if(hit)return hit_status(s,pc?pc:0x1fd668,hit);
        }
        for(;s->cursor<4;++s->cursor){
            gh_hitarea_status hit=gh_hitarea_object_create_001fe218(bindings->init,&object->view,2,
                &s->boxes[s->cursor],&nonnull,&pc);
            if(hit)return hit_status(s,pc?pc:0x1fe218,hit);
            s->created[s->cursor]=(uint8_t)nonnull;
            put(bytes+0x600+4*s->cursor,0);
        }
        s->cursor=0;
        s->stage=BOXES_CONFIGURE;
    }
    if(s->stage==BOXES_CONFIGURE){
        const uint32_t extent[3]={0x3e99999a,0x40000000,0x3e99999a};
        const uint32_t centers[4][3]={
            {0xc02a3d71,0x3eae147b,0x3fe1eb85},
            {0xc02a3d71,0x3eae147b,0xbce56042},
            {0xc02a3d71,0x3eae147b,0xbfee147b},
            {0xc02a3d71,0x3eae147b,0xc06eb852},
        };
        for(;s->cursor<4;++s->cursor){
            if(!s->created[s->cursor])continue;
            gh_hitarea_object *box=&s->boxes[s->cursor];
            int index;
            gh_hitarea_status hit=gh_hitarea_add_box_001fd9d8(box,bindings->shapes,
                bytes+0x80,centers[s->cursor],zero,extent,&index,&pc);
            if(hit)return hit_status(s,pc?pc:0x1fd9d8,hit);
            put(box->bytes,word(box->bytes)|1u);
        }
        for(unsigned i=0;i<4;++i)half(bytes+0xf88+2*i,1);
        s->stage=MODEL_LAYOUT;
    }
    if(s->stage==MODEL_LAYOUT){
        status=model_layout(s,anchor);
        if(status)return status;
        s->stage=READY;
    }
    *out=1;
    return GH_PLACE_OK;
}

static gh_scene_placement_status abort_box(gh_hitarea_object *box,uint8_t *created)
{
    if(!*created)return GH_PLACE_OK;
    uint32_t pc=0;
    if(gh_hitarea_shapes_abort_object(box,&pc)||gh_hitarea_object_release_empty_001fe278(box,&pc))
        return GH_PLACE_UNSUPPORTED;
    *created=0;
    return GH_PLACE_OK;
}

gh_scene_placement_status gh_om311_abort(gh_om311_state *s)
{
    if(!s)return GH_PLACE_ARGUMENT;
    for(unsigned i=0;i<4;++i){
        gh_scene_placement_status status=abort_box(&s->boxes[i],&s->created[i]);
        if(status)return status;
    }
    gh_scene_placement_status status=abort_box(&s->anchor_box,&s->anchor_created);
    return status?status:gh_om_initialize_abort(&s->common);
}
