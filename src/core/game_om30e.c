/* Original OM30E and OM326 share 1BFAC8 construction and 1BFB28/1BFB80. */
#include "game_om30e.h"
#include "game_om_base.h"
#include "game_actor_model_nodes.h"
#include <string.h>

static uint32_t word(const uint8_t *p)
{
    uint32_t value;
    memcpy(&value,p,4);
    return value;
}

static void put(uint8_t *p,size_t offset,uint32_t value)
{
    memcpy(p+offset,&value,4);
}

static gh_scene_placement_status construct(void *data,size_t size,gh_title_player_view *out,
    uint32_t vtable)
{
    if(!data||!out)return GH_PLACE_ARGUMENT;
    if(size<0x610)return GH_PLACE_BOUNDS;
    gh_scene_placement_status error=gh_om_base_construct_001b6e90(data,size,out);
    if(error)return error;
    put(data,0x214,vtable);
    out->original_vtable214=vtable;
    return GH_PLACE_OK;
}

gh_scene_placement_status gh_om30e_construct_0018ec98(void *data,size_t size,gh_title_player_view *out)
{
    return construct(data,size,out,0x423c70);
}

gh_scene_placement_status gh_om326_construct_00199dc0(void *data,size_t size,gh_title_player_view *out)
{
    return construct(data,size,out,0x4251c0);
}

static gh_scene_placement_status fail(gh_om30e_state *s,uint32_t frontier,gh_scene_placement_status status)
{
    s->frontier=frontier;
    return status;
}

static gh_scene_placement_status initialize(gh_om30e_state *s,gh_om_object *o,
    const gh_om_initializer_bindings *b,int *out,uint32_t width,uint32_t depth,uint32_t flags)
{
    *out=0;
    s->frontier=0;
    int original=0;
    gh_scene_placement_status error=gh_om_common_initialize_001b6fb8(&s->common,o,b,&original);
    if(error)return fail(s,s->common.frontier,error);
    if(!original)return fail(s,0x1b6fb8,GH_PLACE_UNSUPPORTED);
    uint8_t *p=o->view.bytes;
    if(!s->stage){
        if(!b->init)return fail(s,0x1fe218,GH_PLACE_UNSUPPORTED);
        uint32_t frontier=0;
        if(gh_hitarea_object_create_001fe218(b->init,&o->view,2,&s->common.hitarea,&original,&frontier))
            return fail(s,frontier?frontier:0x1fe218,GH_PLACE_UNSUPPORTED);
        s->common.hitarea_created=(uint8_t)original;
        put(p,0x600,0);
        s->stage=1;
    }
    if(s->stage==1){
        if(s->common.hitarea_created){
            if(!b->shapes)return fail(s,0x1fd9d8,GH_PLACE_UNSUPPORTED);
            const uint32_t rotation[3]={0},offset[3]={0,0xbf800000,0},size[3]={width,0x40000000,depth};
            uint32_t frontier=0;
            int index;
            if(gh_hitarea_add_box_001fd9d8(&s->common.hitarea,b->shapes,p+0x80,offset,rotation,size,&index,&frontier))
                return fail(s,frontier?frontier:0x1fd9d8,GH_PLACE_UNSUPPORTED);
        }
        s->stage=2;
    }
    if(s->stage==2){
        const uint16_t health=1;
        memcpy(p+0x548,&health,2);
        memcpy(p+0x54a,&health,2);
        const gh_actor_node_packets *first;
        if(gh_actor_model_packet_at(&s->common.model,0,&first))
            return fail(s,0x14b6f8,GH_PLACE_BOUNDS);
        put(first->node,0x380,word(first->node+0x380)&~1u);
        if(s->common.hitarea_created)put(s->common.hitarea.bytes,0,word(s->common.hitarea.bytes)|1u);
        if(flags)put(p,0x5b8,word(p+0x5b8)|flags);
        s->stage=5;
    }
    *out=1;
    return GH_PLACE_OK;
}

gh_scene_placement_status gh_om30e_initialize_0018ed30(gh_om30e_state *s,gh_om_object *o,
    const gh_om_initializer_bindings *b,int *out)
{
    if(!s||!o||o->source.type!=0x30e||!out)return GH_PLACE_ARGUMENT;
    return initialize(s,o,b,out,0x3f6809d4,0x3e4ccccd,0);
}

gh_scene_placement_status gh_om326_initialize_00199e58(gh_om30e_state *s,gh_om_object *o,
    const gh_om_initializer_bindings *b,int *out)
{
    if(!s||!o||o->source.type!=0x326||!out)return GH_PLACE_ARGUMENT;
    return initialize(s,o,b,out,0x40000000,0x3ecccccd,0x200);
}

gh_scene_placement_status gh_om30e_abort(gh_om30e_state *s)
{
    return gh_om303_abort(s);
}
