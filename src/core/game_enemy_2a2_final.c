#include "game_enemy_2a2_final.h"
#include "game_actor_model_nodes.h"
#include <string.h>

enum {DRAW_TIMER=1, LOOKUP_NODES, HIDE_NODES, SHOW_NODES, DRAW_LOOT, DRAW_BONUS, FINISHED};

static uint32_t word(const uint8_t *bytes)
{
    uint32_t value;
    memcpy(&value,bytes,sizeof(value));
    return value;
}

static unsigned half(const uint8_t *bytes)
{
    return bytes[0] | ((unsigned)bytes[1]<<8);
}

static void put(uint8_t *bytes,uint32_t value)
{
    memcpy(bytes,&value,sizeof(value));
}

gh_player_nodes_status gh_enemy2a2_node_names_from_elf(gh_enemy2a2_node_names *out,
    const void *raw,size_t extent)
{
    if (!out || !raw || out->initialized) return GH_PLAYER_NODES_ARGUMENT;
    if (extent<52) return GH_PLAYER_NODES_BOUNDS;
    const uint8_t *elf=raw;
    if (memcmp(elf,"\177ELF\1\1\1",7) || half(elf+18)!=8) return GH_PLAYER_NODES_UNSUPPORTED;
    size_t table=word(elf+28);
    unsigned stride=half(elf+42),count=half(elf+44);
    if (stride<32 || table>extent || count>(extent-table)/stride) return GH_PLAYER_NODES_BOUNDS;
    const uint8_t *names=NULL;
    for (unsigned i=0;i<count;i++) {
        const uint8_t *ph=elf+table+(size_t)i*stride;
        if (word(ph)!=1) continue;
        uint32_t offset=word(ph+4),base=word(ph+8),size=word(ph+16);
        if (base>0x448a28 || (uint64_t)base+size<0x448a68) continue;
        if (names || offset>extent || size>extent-offset) return GH_PLAYER_NODES_BOUNDS;
        names=elf+offset+(0x448a28-base);
    }
    if (!names) return GH_PLAYER_NODES_BOUNDS;
    gh_enemy2a2_node_names next={0};
    memcpy(next.names,names,sizeof(next.names));
    next.initialized=1;
    *out=next;
    return GH_PLAYER_NODES_OK;
}

static gh_actor_model_status stop(gh_enemy2a2_final *s,uint32_t frontier,
    gh_actor_model_status status)
{
    s->frontier=frontier;
    return status;
}

gh_actor_model_status gh_enemy2a2_finalize_0028540c(gh_enemy2a2_final *s,
    gh_enemy2a2_pose *pose,const gh_enemy2a2_node_names *names,gh_em_numeric_policy policy,
    int *original_return)
{
    if (!s || !pose || !pose->prefix || pose->frontier!=0x31d000 || !pose->sphere ||
        !names || !names->initialized || !original_return) return GH_ACTOR_MODEL_ARGUMENT;
    *original_return=0;
    if (s->stage && (s->pose!=pose || s->names!=names)) return GH_ACTOR_MODEL_BUSY;
    if (policy!=GH_EM_NUMERIC_PCSX2_CHOPZERO_REFERENCE)
        return stop(s,0x31d000,GH_ACTOR_MODEL_UNSUPPORTED);
    gh_enemy2a2_init *prefix=pose->prefix;
    gh_actor_model *model=&prefix->model.model;
    if (!model->ready || prefix->bindings.random!=&pose->bindings.init->random ||
        prefix->root->bytes!=model->root.bytes) return GH_ACTOR_MODEL_ARGUMENT;
    uint8_t *bytes=prefix->root->bytes;
    gh_game_mt *random=prefix->bindings.random;
    s->frontier=0;
    if (!s->stage) {
        s->pose=pose;
        s->names=names;
        s->stage=DRAW_TIMER;
    }
    if (s->stage==DRAW_TIMER) {
        uint32_t timer;
        if (gh_game_mt_next_0030f348(random,&s->timer_word))
            return stop(s,0x31d000,GH_ACTOR_MODEL_ARGUMENT);
        if (gh_enemy2a2_mt_timer_reference(s->timer_word,policy,&timer))
            return stop(s,0x31d000,GH_ACTOR_MODEL_UNSUPPORTED);
        put(bytes+0x15bc,timer);
        s->stage=LOOKUP_NODES;
    }
    if (s->stage==LOOKUP_NODES) {
        for (;s->cursor<8;s->cursor++) {
            const unsigned i=s->cursor;
            int nonnull;
            if (gh_actor_model_find_mesh_0014b730(model,names->names[i],8,&s->nodes[i],&nonnull))
                return stop(s,0x14b730,GH_ACTOR_MODEL_UNSUPPORTED);
            put(bytes+0x15cc+i*4,0); /* Retain original pointer in the typed view. */
        }
        s->cursor=0;
        s->stage=HIDE_NODES;
    }
    if (s->stage==HIDE_NODES) {
        for (;s->cursor<8;s->cursor++) {
            const gh_player_mesh_reference *node=&s->nodes[s->cursor];
            if (node->bytes && gh_actor_model_node_or_flags(model,(unsigned)node->mesh_index,0x380,1))
                return stop(s,0x2854d8,GH_ACTOR_MODEL_UNSUPPORTED);
        }
        s->cursor=0;
        s->stage=SHOW_NODES;
    }
    if (s->stage==SHOW_NODES) {
        const unsigned visible[]={0,4};
        for (;s->cursor<2;s->cursor++) {
            const gh_player_mesh_reference *node=&s->nodes[visible[s->cursor]];
            if (node->bytes && gh_actor_model_node_mask_flags(model,(unsigned)node->mesh_index,0x380,~1u,0))
                return stop(s,0x285588,GH_ACTOR_MODEL_UNSUPPORTED);
        }
        bytes[0x1604]=1;
        s->stage=DRAW_LOOT;
    }
    if (s->stage==DRAW_LOOT) {
        if (gh_game_mt_next_0030f348(random,&s->loot_words[0]))
            return stop(s,0x31cfe0,GH_ACTOR_MODEL_ARGUMENT);
        uint32_t low=s->loot_words[0]&15;
        put(bytes+0x560,low<7?0x3c1:low<13?0x3c8:low<15?0x3d1:0x3d0);
        s->stage=DRAW_BONUS;
    }
    if (s->stage==DRAW_BONUS) {
        if (gh_game_mt_next_0030f348(random,&s->loot_words[1]))
            return stop(s,0x31cfe0,GH_ACTOR_MODEL_ARGUMENT);
        if (!(s->loot_words[1]&7)) put(bytes+0x560,0x3d8);
        if (word(bytes+0x1554)&0x10000000u) put(bytes+0x15b0,word(bytes+0x15b0)|0x40u);
        put(bytes+0x15c0,0x43160000);
        s->stage=FINISHED;
        s->complete=1;
    }
    *original_return=1;
    return GH_ACTOR_MODEL_OK;
}

gh_actor_model_status gh_enemy2a2_initialize_00284af8(gh_enemy2a2_virtual_init *s,
    gh_title_player_view *actor,const gh_enemy_request *request,uint32_t argument,
    const gh_object_view *resource,const gh_enemy2a2_init_bindings *init_bindings,
    const gh_enemy2a2_pose_bindings *pose_bindings,const gh_enemy2a2_node_names *names,
    gh_em_numeric_policy policy,int *original_return)
{
    if (!s || !original_return) return GH_ACTOR_MODEL_ARGUMENT;
    *original_return=0;
    int result;
    gh_actor_model_status status=gh_enemy2a2_initialize_prefix_00284af8(&s->prefix,actor,
        request,argument,resource,init_bindings,&result);
    s->frontier=s->prefix.frontier;
    if (status!=GH_ACTOR_MODEL_UNSUPPORTED || s->frontier!=0x157c18) return status;
    status=gh_enemy2a2_pose_continue_002851b8(&s->pose,&s->prefix,pose_bindings);
    s->frontier=s->pose.frontier;
    if (status!=GH_ACTOR_MODEL_UNSUPPORTED || s->frontier!=0x31d000) return status;
    status=gh_enemy2a2_finalize_0028540c(&s->final,&s->pose,names,policy,original_return);
    s->frontier=s->final.frontier;
    return status;
}

gh_actor_model_status gh_enemy2a2_release_native(gh_enemy2a2_virtual_init *s)
{
    if (!s) return GH_ACTOR_MODEL_ARGUMENT;
    gh_actor_model_status status=gh_enemy2a2_pose_release_prefix(&s->pose);
    if (status) {s->frontier=s->pose.frontier;return status;}
    status=gh_enemy2a2_init_release_prefix(&s->prefix);
    if (status) {s->frontier=s->prefix.model.frontier;return status;}
    *s=(gh_enemy2a2_virtual_init){0};
    return GH_ACTOR_MODEL_OK;
}
