#include "game_enemy_row2.h"
#include <string.h>

int gh_enemy202_is_row2_profile(const gh_enemy_request *request)
{
    return request && request->object_kind==0x202 && request->dependency_kind==0x200 &&
        request->appearance==2 && request->flags==0x04000000u && !request->dependency_flags;
}

int gh_enemy202_is_r100_appearance2_profile(const gh_enemy_request *request)
{
    return request && request->object_kind==0x202 &&
        (request->dependency_kind==0x200 || request->dependency_kind==0x202 ||
         request->dependency_kind==0x203) &&
        request->appearance==2 && request->flags==0x04000000u && !request->dependency_flags;
}

static int is_r100_appearance5_profile(const gh_enemy_request *request)
{
    if(!request||request->object_kind!=0x202||request->appearance!=5||request->dependency_flags)
        return 0;
    if(request->dependency_kind==0x200)
        return (request->entry_number==8 && request->flags==0x48800000u) ||
            (request->entry_number==10 && request->flags==0x04000000u);
    return request->dependency_kind==0x203 && request->entry_number==11 &&
        request->flags==0x04000000u;
}

static int is_r100_appearance9_profile(const gh_enemy_request *request)
{
    return request && request->object_kind==0x202 && request->dependency_kind==0x202 &&
        request->entry_number==18 && request->appearance==9 && request->flags==0x14000004u &&
        !request->dependency_flags;
}

static int is_r100_appearance0_profile(const gh_enemy_request *request)
{
    return request && request->object_kind==0x202 && !request->appearance &&
        request->flags==0x04000000u && !request->dependency_flags &&
        ((request->entry_number==19 && request->dependency_kind==0x203) ||
         ((request->entry_number==20||request->entry_number==31) && request->dependency_kind==0x200) ||
         (request->entry_number==32 && request->dependency_kind==0x202));
}

int gh_enemy202_is_r100_supported_profile(const gh_enemy_request *request)
{
    return gh_enemy202_is_r100_appearance2_profile(request)||is_r100_appearance5_profile(request)||
        is_r100_appearance9_profile(request)||is_r100_appearance0_profile(request);
}

int gh_enemy202_body_radius_002070cc(uint32_t scale,uint32_t nominal,uint32_t *bits)
{
    if (!bits || scale<0x3f800000 || scale>0x3f8ccccd ||
        (nominal!=0x3e99999a && nominal!=0x3ecccccd)) return 0;
    /* Binary32 exponents 0 and -2. Both finite positive operands are exact. */
    uint64_t product=(uint64_t)((scale&0x7fffff)|0x800000)*((nominal&0x7fffff)|0x800000);
    unsigned normalized=(unsigned)(product>>47);
    *bits=((125u+normalized)<<23)|((uint32_t)(product>>(23+normalized))&0x7fffff);
    return 1;
}

gh_actor_model_status gh_enemy_model_occurrence_002ddea0(uint32_t kind,
    uint32_t dependency, unsigned *occurrence)
{
    if (!occurrence) return GH_ACTOR_MODEL_ARGUMENT;
    if (kind==0x202) {
        switch (dependency) {
        case 0x200: *occurrence=0; return GH_ACTOR_MODEL_OK;
        case 0x202: *occurrence=4; return GH_ACTOR_MODEL_OK;
        case 0x203: *occurrence=1; return GH_ACTOR_MODEL_OK;
        case 0x20a:
            *occurrence=2;
            return GH_ACTOR_MODEL_OK;
        }
    } else if (kind==0x2a2 && dependency>=0x2a2 && dependency<=0x2a4) {
        *occurrence=(unsigned)(dependency-0x2a2);
        return GH_ACTOR_MODEL_OK;
    }
    return GH_ACTOR_MODEL_UNSUPPORTED;
}

static gh_actor_model_status select_initial_motion(const gh_object_view *resource,
    size_t table_offset,uint8_t substate,uint8_t detail,gh_enemy_initial_motion_selection *out)
{
    if (!resource->file_extent_known || resource->file_extent>resource->size ||
        resource->file_extent<table_offset+8)
        return GH_ACTOR_MODEL_BOUNDS;

    uint32_t motion, sequence;
    memcpy(&motion,resource->bytes+table_offset,sizeof(motion));
    memcpy(&sequence,resource->bytes+table_offset+4,sizeof(sequence));
    if (motion>resource->file_extent-4 || sequence>resource->file_extent-4)
        return GH_ACTOR_MODEL_BOUNDS;
    if (memcmp(resource->bytes+motion,"mtb3",4) || memcmp(resource->bytes+sequence,"SEQ\0",4))
        return GH_ACTOR_MODEL_UNSUPPORTED;
    *out=(gh_enemy_initial_motion_selection){.motion_offset=motion,.sequence_offset=sequence,
        .substate=substate,.phase_detail=detail};
    return GH_ACTOR_MODEL_OK;
}

gh_actor_model_status gh_enemy202_appearance2_select_00203c38(
    const gh_enemy_request *request, const gh_object_view *resource,
    gh_enemy_initial_motion_selection *out)
{
    if (!request || !resource || !out || !resource->bytes || resource->kind!=0x202)
        return GH_ACTOR_MODEL_ARGUMENT;
    if (request->object_kind!=0x202 || (request->dependency_kind!=0x200 &&
        request->dependency_kind!=0x202 && request->dependency_kind!=0x203) ||
        request->appearance!=2 || (request->dependency_flags&1))
        return GH_ACTOR_MODEL_UNSUPPORTED;
    return select_initial_motion(resource,0x41c,0x71,0,out);
}

gh_actor_model_status gh_enemy202_appearance5_select_00203b3c(
    const gh_enemy_request *request, const gh_object_view *resource,
    gh_enemy_initial_motion_selection *out)
{
    if (!request || !resource || !out || !resource->bytes || resource->kind!=0x202)
        return GH_ACTOR_MODEL_ARGUMENT;
    if (!is_r100_appearance5_profile(request))
        return GH_ACTOR_MODEL_UNSUPPORTED;
    /* 200 uses the underflow default; 203 reaches the same directory via 42DB50[1].
     * Both write {0,1,0,3} then branch to 2042C0, skipping the zero-detail stores. */
    return select_initial_motion(resource,0x3fc,1,3,out);
}

gh_actor_model_status gh_enemy202_appearance9_select_0020393c(
    const gh_enemy_request *request, const gh_object_view *resource,
    gh_enemy_initial_motion_selection *out)
{
    if (!request || !resource || !out || !resource->bytes || resource->kind!=0x202)
        return GH_ACTOR_MODEL_ARGUMENT;
    if (!is_r100_appearance9_profile(request))
        return GH_ACTOR_MODEL_UNSUPPORTED;
    /* 203700 selects the dependency-202 pair. 20393C leaves it intact. */
    return select_initial_motion(resource,0x648,1,0,out);
}

gh_actor_model_status gh_enemy202_appearance0_select_0020393c(
    const gh_enemy_request *request, const gh_object_view *resource,
    gh_enemy_initial_motion_selection *out)
{
    if (!request || !resource || !out || !resource->bytes || resource->kind!=0x202)
        return GH_ACTOR_MODEL_ARGUMENT;
    if (!is_r100_appearance0_profile(request))
        return GH_ACTOR_MODEL_UNSUPPORTED;
    /* Dependency 203 uses 42D4C0[1] -> 203700; dependency 200 takes
     * the underflow branch 2036EC. 20393C keeps those candidate pairs. */
    size_t directory=request->dependency_kind==0x200?0x98:0x648;
    return select_initial_motion(resource,directory,1,0,out);
}
