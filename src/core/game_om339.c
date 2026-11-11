#include "game_om339.h"
#include "game_om_base.h"
#include "game_om_fragments.h"

#include <string.h>

static void put(uint8_t *p,uint32_t word){memcpy(p,&word,4);}
static void half(uint8_t *p,uint16_t word){memcpy(p,&word,2);}
static void vf0(uint8_t *p){memset(p,0,12);put(p+12,0x3f800000);}

gh_scene_placement_status gh_om339_construct_0019e1c0(void *data,size_t size,gh_title_player_view *out)
{
    if(!data||!out)return GH_PLACE_ARGUMENT;
    if(size<GH_OM339_BYTES)return GH_PLACE_BOUNDS;
    gh_scene_placement_status status=gh_om_base_construct_001b6e90(data,size,out);
    if(status)return status;
    uint8_t *root=data;
    /* 1C27E8 supplies the fragment base; derived 19E1C0 then writes the
     * class vtable and four fragment records. 16F640 touches only byte54. */
    put(root+0x214,0x426268);
    out->original_vtable214=0x426268;
    for(unsigned i=0;i<4;i++){
        uint8_t *part=root+0x620+i*0x60;
        for(unsigned j=0;j<5;j++)vf0(part+16*j);
        part[0x54]=0;
    }
    return GH_PLACE_OK;
}

static gh_scene_placement_status fail(gh_om303_state *s,uint32_t frontier,gh_scene_placement_status status)
{
    s->frontier=frontier;
    return status;
}

gh_scene_placement_status gh_om339_initialize_0019e2a8(gh_om303_state *s,gh_om_object *object,
    const gh_om_initializer_bindings *b,int *out)
{
    if(!s||!object||object->source.type!=0x339||!b||!out)return GH_PLACE_ARGUMENT;
    *out=0;
    s->frontier=0;
    int common_return=0;
    gh_scene_placement_status status=gh_om_common_initialize_001b6fb8(&s->common,object,b,&common_return);
    if(status||!common_return)return fail(s,s->common.frontier?s->common.frontier:0x1b6fb8,
        status?status:GH_PLACE_UNSUPPORTED);
    uint8_t *root=object->view.bytes;
    uint32_t pc=0;
    if(!s->stage){
        if(!b->init)return fail(s,0x1fe218,GH_PLACE_UNSUPPORTED);
        int created;
        const gh_hitarea_status result=gh_hitarea_object_create_001fe218(b->init,&object->view,2,
            &s->common.hitarea,&created,&pc);
        if(result)return fail(s,pc?pc:0x1fe218,result==GH_HITAREA_MEMORY?GH_PLACE_MEMORY:GH_PLACE_UNSUPPORTED);
        s->common.hitarea_created=(uint8_t)created;
        put(root+0x600,0);
        s->stage=1;
    }
    if(s->stage==1){
        if(s->common.hitarea_created){
            if(!b->shapes)return fail(s,0x1fd9d8,GH_PLACE_UNSUPPORTED);
            const uint32_t zero[3]={0},size[3]={0x4019999a,0x3f8ccccd,0x3e4ccccd};
            int index;
            const gh_hitarea_status result=gh_hitarea_add_box_001fd9d8(&s->common.hitarea,b->shapes,
                root+0x80,zero,zero,size,&index,&pc);
            if(result)return fail(s,pc?pc:0x1fd9d8,result==GH_HITAREA_MEMORY?GH_PLACE_MEMORY:GH_PLACE_UNSUPPORTED);
        }
        s->stage=2;
    }
    if(s->stage==2){
        put(root+0x5e4,0);
        put(root+0x5e8,0);
        put(root+0x5ec,7);
        put(root+0x5f0,0);
        half(root+0x548,1);
        half(root+0x54a,1);
        half(root+0x5f4,0xffff);
        half(root+0x5f6,0);
        put(root+0x608,0);
        s->segments=root+0x620;
        root[0x60c]=4;
        status=gh_om_fragments_initialize_001c28a0(&s->common);
        if(status)return fail(s,s->common.frontier?s->common.frontier:0x1c28a0,status);
        s->stage=5;
    }
    *out=1;
    return GH_PLACE_OK;
}
