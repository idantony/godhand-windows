#include "game_enemy_2a2.h"
#include "game_enemy_row2.h"
#include <string.h>

static void put_word(uint8_t *bytes, uint32_t value)
{
    memcpy(bytes,&value,sizeof(value));
}

static uint32_t word(const uint8_t *bytes)
{
    uint32_t value;
    memcpy(&value,bytes,sizeof(value));
    return value;
}

gh_title_player_status gh_enemy2a2_construct_00284a28(void *raw,size_t extent,
    gh_title_player_view *out)
{
    if (!raw || !out) return GH_PLAYER_ARGUMENT;
    if (extent<GH_EM2A2_BYTES) return GH_PLAYER_BOUNDS;
    gh_title_player_view view;
    gh_title_player_status status=gh_enemy_base_construct_0028eb00(raw,extent,&view);
    if (status) return status;

    uint8_t *bytes=raw;
    static const size_t vector_offsets[]={0x1540,0x1570,0x1580,0x1590,0x15a0,0x15f0};
    for (unsigned i=0;i<sizeof(vector_offsets)/sizeof(vector_offsets[0]);i++) {
        uint8_t *vector=bytes+vector_offsets[i];
        memset(vector,0,12);
        put_word(vector+12,0x3f800000);
    }
    bytes[0x1561]=0xff;
    put_word(bytes+0x214,0x448ca0);
    put_word(bytes+0x4ac,0x448a00);
    view.original_vtable214=0x448ca0;
    view.original_kind4ac=0x448a00;
    *out=view;
    return GH_PLAYER_OK;
}

gh_title_player_status gh_enemy2a2_allocate_00313820(gh_game_title_player *actor,
    gh_heap *room_heap,const gh_boot_work *work,int *original_nonnull)
{
    if (!actor || !room_heap || !work || !original_nonnull) return GH_PLAYER_ARGUMENT;
    *original_nonnull=0;
    if (actor->allocation.pointer || actor->arena_allocation.pointer) return GH_PLAYER_BUSY;
    if (word((const uint8_t *)work+0x594)&0x100000) return GH_PLAYER_OK;

    gh_r207_allocation allocation;
    gh_mem_error allocated=gh_heap_allocate(room_heap,GH_EM2A2_BYTES,16,&allocation);
    if (allocated==GH_MEM_CAPACITY) return GH_PLAYER_OK;
    if (allocated) return GH_PLAYER_MEMORY;
    memset(allocation.pointer,0,GH_EM2A2_BYTES);
    gh_title_player_view view;
    gh_title_player_status status=gh_enemy2a2_construct_00284a28(allocation.pointer,
        GH_EM2A2_BYTES,&view);
    if (status) {
        (void)gh_heap_release(room_heap,allocation);
        return status;
    }
    view.initial_memory=GH_TITLE_MEMORY_ORIGINAL_FACTORY_ZERO;
    *actor=(gh_game_title_player){.allocation=allocation,.heap=room_heap,.view=view,.ready=1,
        .allocation_domain=GH_PLAYER_ALLOCATION_DEFAULT_HEAP};
    *original_nonnull=1;
    return GH_PLAYER_OK;
}

gh_actor_model_status gh_enemy2a2_model_00288da8(gh_enemy_model *state,
    gh_title_player_view *root,const gh_object_view *resource,gh_boot_work *work,
    gh_room_pools *pools,gh_heap *heap,const void *fog,gh_static_transform_policy policy,
    int *original_return)
{
    if (!state || !root || !root->bytes || root->size<GH_EM2A2_BYTES ||
        !resource || !work || !pools || !heap || !fog || !original_return)
        return GH_ACTOR_MODEL_ARGUMENT;
    *original_return=0;
    state->frontier=0;
    uint16_t kind;
    memcpy(&kind,root->bytes+0x2fe,sizeof(kind));
    if (kind!=0x2a2 || root->original_vtable214!=0x448ca0) {
        state->frontier=0x288da8;
        return GH_ACTOR_MODEL_UNSUPPORTED;
    }
    unsigned occurrence;
    gh_actor_model_status status=gh_enemy_model_occurrence_002ddea0(kind,
        word(root->bytes+0x564),&occurrence);
    if (status) {state->frontier=0x2ddea0;return status;}
    if (resource->kind!=kind || !resource->bytes || !resource->file_extent_known ||
        resource->file_extent>resource->size) return GH_ACTOR_MODEL_ARGUMENT;
    if (state->stage && (state->resource!=resource->bytes || state->occurrence!=occurrence ||
        (state->model.bound && state->model.root.bytes!=root->bytes))) return GH_ACTOR_MODEL_BUSY;

    if (!state->stage) {
        /* 1595E8 stores the dependency's original offset before SearchData. */
        put_word(root->bytes+0x424,25u*occurrence);
        gh_bundle bundle;
        gh_bundle_error error=gh_bundle_open(resource->bytes,resource->file_extent,&bundle);
        if (error) {state->frontier=0x31ccf0;return GH_ACTOR_MODEL_BOUNDS;}
        error=gh_bundle_find(&bundle,"MD",occurrence,&state->geometry);
        if (error && error!=GH_BUNDLE_NOT_FOUND) {state->frontier=0x31ccf0;return GH_ACTOR_MODEL_BOUNDS;}
        error=gh_bundle_find(&bundle,"TM3",occurrence,&state->texture);
        if (error && error!=GH_BUNDLE_NOT_FOUND) {state->frontier=0x31ccf0;return GH_ACTOR_MODEL_BOUNDS;}
        state->resource=resource->bytes;
        state->occurrence=occurrence;
        state->stage=1;
    }
    const gh_bundle_entry *entries[]={&state->geometry,&state->texture};
    for (unsigned i=0;i<sizeof(entries)/sizeof(entries[0]);i++) {
        const gh_bundle_entry *entry=entries[i];
        if (entry->bytes && (entry->offset>resource->file_extent ||
            entry->size>resource->file_extent-entry->offset)) {
            state->frontier=0x31ccf0;
            return GH_ACTOR_MODEL_BOUNDS;
        }
    }
    if (!state->texture.bytes) {state->stage=2;return GH_ACTOR_MODEL_OK;}
    status=gh_actor_model_bind_packet_inputs(&state->model,heap,fog);
    if (status) return status;
    status=gh_actor_model_initialize_00149738(&state->model,root,work,pools,
        state->geometry.bytes?&state->geometry:NULL,&state->texture,0,0,policy,original_return);
    state->frontier=state->model.frontier;
    if (!status) state->stage=2;
    return status;
}
