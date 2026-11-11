#include "game_enemy_2a2_init.h"
#include <string.h>

enum {
    REQUEST_WRITTEN=1, MODEL_READY, BASE_READY, HP_WRITTEN, MOTION_SELECTED, MOTION_READY, ORIGINAL_FALSE
};

static uint32_t word(const uint8_t *bytes)
{
    uint32_t value;
    memcpy(&value,bytes,sizeof(value));
    return value;
}

static void put(uint8_t *bytes, uint32_t value)
{
    memcpy(bytes,&value,sizeof(value));
}

static int same_request(const gh_enemy_request *a,const gh_enemy_request *b)
{
    return a->object_kind==b->object_kind && !memcmp(a->position_xyz,b->position_xyz,12) &&
        a->yaw_bits==b->yaw_bits && a->flags==b->flags && a->dependency_kind==b->dependency_kind &&
        a->dependency_flags==b->dependency_flags && a->appearance==b->appearance &&
        a->entry_number==b->entry_number;
}

static int same_bindings(const gh_enemy2a2_init_bindings *a,const gh_enemy2a2_init_bindings *b)
{
    return a->work==b->work && a->pools==b->pools && a->packet_heap==b->packet_heap &&
        a->fog==b->fog && a->cockpit==b->cockpit && a->random==b->random && a->policy==b->policy && a->ik==b->ik;
}

gh_actor_model_status gh_enemy2a2_initialize_prefix_00284af8(gh_enemy2a2_init *s,
    gh_title_player_view *root,const gh_enemy_request *request,uint32_t argument,
    const gh_object_view *resource,const gh_enemy2a2_init_bindings *bindings,int *original_return)
{
    if (!s || !root || !root->bytes || root->size<GH_EM2A2_BYTES || !request ||
        !resource || !bindings || !bindings->work || !bindings->pools || !bindings->packet_heap ||
        !bindings->fog || !bindings->cockpit || !bindings->random || !original_return)
        return GH_ACTOR_MODEL_ARGUMENT;
    *original_return=0;
    uint8_t *bytes=root->bytes;
    uint16_t kind;
    memcpy(&kind,bytes+0x2fe,sizeof(kind));
    if (kind!=0x2a2 || root->original_vtable214!=0x448ca0 || request->object_kind!=kind ||
        request->dependency_kind<0x2a2 || request->dependency_kind>0x2a4) {
        s->frontier=0x284af8;
        return GH_ACTOR_MODEL_UNSUPPORTED;
    }
    if (resource->kind!=kind || !resource->bytes || !resource->file_extent_known ||
        resource->file_extent>resource->size) return GH_ACTOR_MODEL_ARGUMENT;
    if (s->stage && (s->root!=root || (s->model.model.bound && s->model.model.root.bytes!=bytes) ||
        !same_request(&s->request,request) || s->argument!=argument ||
        s->resource.bytes!=resource->bytes || s->resource.file_extent!=resource->file_extent ||
        !same_bindings(&s->bindings,bindings))) return GH_ACTOR_MODEL_BUSY;

    s->frontier=0;
    if (!s->stage) {
        s->root=root;
        s->request=*request;
        s->argument=argument;
        s->resource=*resource;
        s->bindings=*bindings;
        put(bytes+0x1530,request->object_kind);
        memcpy(bytes+0x1540,request->position_xyz,12); /* W stays as constructed. */
        put(bytes+0x1550,request->yaw_bits);
        put(bytes+0x1554,request->flags);
        put(bytes+0x1558,request->dependency_kind);
        put(bytes+0x155c,request->dependency_flags);
        bytes[0x1560]=request->appearance;
        bytes[0x1561]=request->entry_number;
        put(bytes+0x564,request->dependency_kind);
        s->stage=REQUEST_WRITTEN;
    }
    if (s->stage==REQUEST_WRITTEN) {
        int model_return=0;
        gh_actor_model_status status=gh_enemy2a2_model_00288da8(&s->model,root,&s->resource,
            bindings->work,bindings->pools,bindings->packet_heap,bindings->fog,bindings->policy,&model_return);
        if (status) {s->frontier=s->model.frontier?s->model.frontier:0x288da8;return status;}
        s->stage=model_return?MODEL_READY:ORIGINAL_FALSE;
    }
    if (s->stage==ORIGINAL_FALSE) return GH_ACTOR_MODEL_OK;
    if (s->stage==MODEL_READY) {
        int base_return=0;
        gh_actor_model_status status=gh_enemy_base_initialize_0028ebf0(&s->base,root,&s->request,
            argument,bindings->cockpit,bindings->random,&base_return);
        if (status) {s->frontier=s->base.frontier?s->base.frontier:0x28ebf0;return status;}
        /* Original 284AF8 ignores 28EBF0's returned value. */
        s->stage=BASE_READY;
    }
    if (s->stage==BASE_READY) {
        bytes[0x531]=0;
        put(bytes+0x15ec,0);
        unsigned level=((const uint8_t *)bindings->work)[0x5b1];
        uint16_t hp=(uint16_t)(level>=1 && level<=8?400u+100u*level:500u);
        memcpy(bytes+0x548,&hp,sizeof(hp));
        memcpy(bytes+0x54a,&hp,sizeof(hp));
        s->stage=HP_WRITTEN;
    }
    if (s->stage==HP_WRITTEN) {
        if (bytes[0x1560]!=0) {s->frontier=0x284c28;return GH_ACTOR_MODEL_UNSUPPORTED;}
        if (resource->file_extent<0x30) {s->frontier=0x284c7c;return GH_ACTOR_MODEL_BOUNDS;}
        size_t motion=word(resource->bytes+0x28), sequence=word(resource->bytes+0x2c);
        if (motion>resource->file_extent-4 || sequence>resource->file_extent-4) {
            s->frontier=0x284c7c;
            return GH_ACTOR_MODEL_BOUNDS;
        }
        bytes[0x2f4]=0;
        bytes[0x2f5]=2;
        bytes[0x2f6]=0;
        bytes[0x2f7]=0;
        s->motion_offset=motion;
        s->sequence_offset=sequence;
        s->stage=MOTION_SELECTED;
    }
    if (s->stage==MOTION_SELECTED) {
        gh_actor_motion_bindings motion={.model=&s->model.model,.resource=s->resource.bytes,
            .resource_bytes=s->resource.file_extent,.work=s->bindings.work,.ik_globals=s->bindings.ik};
        gh_actor_motion_status status=gh_actor_motion_begin_initial_00157568(&s->motion,&motion,
            s->motion_offset,s->sequence_offset);
        if (status) {
            s->frontier=s->motion.frontier?s->motion.frontier:0x157568;
            switch (status) {
            case GH_ACTOR_MOTION_ARGUMENT: return GH_ACTOR_MODEL_ARGUMENT;
            case GH_ACTOR_MOTION_BOUNDS: return GH_ACTOR_MODEL_BOUNDS;
            case GH_ACTOR_MOTION_BUSY: return GH_ACTOR_MODEL_BUSY;
            default: return GH_ACTOR_MODEL_UNSUPPORTED;
            }
        }
        s->stage=MOTION_READY;
    }
    s->frontier=0x157c18;
    return GH_ACTOR_MODEL_UNSUPPORTED;
}

gh_actor_model_status gh_enemy2a2_init_release_prefix(gh_enemy2a2_init *s)
{
    if (!s) return GH_ACTOR_MODEL_ARGUMENT;
    gh_actor_model_status status=gh_enemy_model_release_prefix(&s->model);
    if (!status) *s=(gh_enemy2a2_init){0};
    return status;
}
