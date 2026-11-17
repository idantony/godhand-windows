/*Actual1C28A0,ONE sharedimplementation for303/305/320.*/
#include "game_om_fragments.h"
#include "game_om_base.h"
#include "game_actor_model_nodes.h"
#include <string.h>
static uint32_t w(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}static void put(uint8_t*p,size_t o,uint32_t n){memcpy(p+o,&n,4);}
static const gh_om_fragment_profile profiles[]={
    /* 314E10 -> 19BE28/19BF10. The Move wrapper delegates to 1C29D0. */
    {0x32f,0x920,0x314e10,0x4259d8,0x19beb0,0x19c030,
     0,{0x3fb33333,0x400ccccd,0x3fb33333},8,0},
    /* 193468/193550, 19D9C0/19DAA8, 19DC48/19DD30. */
    {0x31b,0x800,0x3145c0,0x424790,0x1934f0,0x1936a8,
     0x3eab851f,{0x3f970a3d,0x3fce147b,0x3f2b851f},5,1},
    {0x336,0x800,0x315120,0x425fb0,0x19da48,0x19dbd8,
     0,{0x3fa66666,0x3ff33333,0x3f4ccccd},5,0},
    {0x337,0x860,0x315190,0x426098,0x19dcd0,0x19de60,
     0x3ecccccd,{0x3f4ccccd,0x40933333,0x3f4ccccd},6,0},
};

const gh_om_fragment_profile *gh_om_fragment_profile_for(uint32_t kind)
{
    for(size_t i=0;i<sizeof(profiles)/sizeof(profiles[0]);i++)
        if(profiles[i].kind==kind)return &profiles[i];
    return NULL;
}

gh_scene_placement_status gh_om_fragment_construct(const gh_om_fragment_profile *profile,
    void *memory,size_t size,gh_title_player_view *out)
{
    if(!profile||!memory||!out)return GH_PLACE_ARGUMENT;
    if(size<profile->bytes)return GH_PLACE_BOUNDS;
    gh_scene_placement_status status=gh_om_base_construct_001b6e90(memory,size,out);
    if(status)return status;
    uint8_t *actor=memory;
    put(actor,0x214,profile->vtable);
    out->original_vtable214=profile->vtable;
    const uint32_t vf0[4]={0,0,0,0x3f800000};
    for(unsigned i=0;i<profile->fragments;i++){
        uint8_t *fragment=actor+0x620+(size_t)i*0x60;
        for(unsigned j=0;j<5;j++)memcpy(fragment+j*16,vf0,sizeof(vf0));
        fragment[0x54]=0;
    }
    return GH_PLACE_OK;
}

static gh_scene_placement_status fragment_fail(gh_om303_state *s,uint32_t frontier,
    gh_scene_placement_status status)
{
    s->frontier=frontier;
    return status;
}

gh_scene_placement_status gh_om_fragment_initialize(const gh_om_fragment_profile *profile,
    gh_om303_state *s,gh_om_object *object,const gh_om_initializer_bindings *b,int *out)
{
    if(!profile||!s||!object||!b||!out||object->source.type!=profile->kind)
        return GH_PLACE_ARGUMENT;
    *out=0;
    int inherited;
    gh_scene_placement_status status=gh_om_common_initialize_001b6fb8(
        &s->common,object,b,&inherited);
    if(status)return fragment_fail(s,s->common.frontier,status);
    if(!inherited)return fragment_fail(s,0x1b6fb8,GH_PLACE_UNSUPPORTED);
    uint8_t *actor=object->view.bytes;
    if(!s->stage){
        uint32_t frontier;
        int created;
        if(!b->init)return fragment_fail(s,0x1fe218,GH_PLACE_UNSUPPORTED);
        if(gh_hitarea_object_create_001fe218(b->init,&object->view,2,
            &s->common.hitarea,&created,&frontier))
            return fragment_fail(s,frontier?frontier:0x1fe218,GH_PLACE_UNSUPPORTED);
        s->common.hitarea_created=(uint8_t)created;
        put(actor,0x600,0);
        s->stage=1;
    }
    if(s->stage==1){
        if(s->common.hitarea_created){
            const uint32_t center[3]={0,profile->box_y,0}, rotation[3]={0};
            uint32_t frontier;
            int index;
            if(!b->shapes)return fragment_fail(s,0x1fd9d8,GH_PLACE_UNSUPPORTED);
            if(gh_hitarea_add_box_001fd9d8(&s->common.hitarea,b->shapes,
                actor+0x80,center,rotation,profile->box_extent,&index,&frontier))
                return fragment_fail(s,frontier?frontier:0x1fd9d8,GH_PLACE_UNSUPPORTED);
            if(profile->receiver_flag){
                uint8_t *unit=s->common.hitarea.bytes;
                put(unit,0,w(unit)|1u);
            }
        }
        s->stage=2;
    }
    if(s->stage==2){
        put(actor,0x5e4,0);
        put(actor,0x5e8,0);
        put(actor,0x5ec,7);
        put(actor,0x5f0,0);
        const uint16_t no_collision=0xffff, health=1, zero=0;
        memcpy(actor+0x5f4,&no_collision,2);
        memcpy(actor+0x5f6,&zero,2);
        memcpy(actor+0x548,&health,2);
        memcpy(actor+0x54a,&health,2);
        put(actor,0x608,0);
        s->segments=actor+0x620;
        actor[0x60c]=profile->fragments;
        status=gh_om_fragments_initialize_001c28a0(&s->common);
        if(status)return fragment_fail(s,s->common.frontier,status);
        s->stage=5;
    }
    s->frontier=0;
    *out=1;
    return GH_PLACE_OK;
}

gh_scene_placement_status gh_om_fragments_initialize_001c28a0(gh_om_init_state*s){if(!s||!s->object||!s->model.ready||s->model.root.bytes!=s->object->view.bytes||s->object->view.size<0x612)return GH_PLACE_ARGUMENT;uint8_t*p=s->object->view.bytes;s->frontier=0;if(p[0x2b4]!=s->model.part_count){s->frontier=0x14bfe0;return GH_PLACE_BOUNDS;}uint16_t value=30;memcpy(p+0x610,&value,2);for(unsigned i=0;i<s->model.script.mesh_count;i++){const gh_actor_node_packets*packet;if(gh_actor_model_packet_at(&s->model,i,&packet)){s->frontier=0x14b6f8;return GH_PLACE_BOUNDS;}uint8_t*n=packet->node;if(n[0x40c]==0)put(n,0x380,w(n+0x380)&~1u);else if(n[0x40c]==0x40)put(n,0x380,w(n+0x380)|1u);}int limit=p[0x60c]<128?p[0x60c]:(int)p[0x60c]-256;for(unsigned i=1;(int)i<limit&&i<s->model.part_count;i++){const gh_actor_part*part;if(gh_actor_model_part_at(&s->model,i,&part)){s->frontier=0x14bfe0;return GH_PLACE_BOUNDS;}put(part->bytes,0x154,w(part->bytes+0x154)|0x18u);}return GH_PLACE_OK;}
