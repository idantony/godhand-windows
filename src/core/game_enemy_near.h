#ifndef GH_GAME_ENEMY_NEAR_H
#define GH_GAME_ENEMY_NEAR_H
#include "game_actor_motion.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Original queries of the SAME cEmManage active list and typed position aliases. */
gh_actor_motion_status gh_enemy_find_ema2_near_00292dc8(const gh_game_init_state *,
    const gh_title_player_view *origin,float radius,const void *elf,size_t elf_bytes,
    const gh_game_title_player **found,uint32_t *frontier);
gh_actor_motion_status gh_enemy_active_near_002919f8(const gh_game_init_state *,
    const gh_title_player_view *origin,float radius,int *found,uint32_t *frontier);
gh_actor_motion_status gh_enemy_other_near_00291ac8(const gh_game_init_state *,
    const gh_title_player_view *origin,const gh_title_player_view *skip,float radius,
    int *found,uint32_t *frontier);

/* 288F00 requests actor-attached bank0/program96 before changing HP or state. */
typedef gh_actor_motion_status (*gh_enemy_ema2_hit_sound)(void *,
    const gh_game_title_player *target,uint32_t *frontier);
typedef struct {
    const gh_game_init_state *manager;
    const gh_title_player_view *origin,*player;
    const gh_boot_work *work;
    const void *elf;
    size_t elf_bytes;
    gh_enemy_ema2_hit_sound sound;
    void *sound_context;
} gh_enemy_ema2_damage_bindings;
typedef struct {
    gh_enemy_ema2_damage_bindings bindings;
    const void *identity;
    const gh_game_title_player *targets[64];
    uint8_t slots[64],count,cursor;
    uint16_t amount;
    uint8_t active,complete;
} gh_enemy_ema2_damage_call;
/* Keep the original traversal position across a sound provider retry. */
gh_actor_motion_status gh_enemy_damage_ema2_near_00292d18(gh_enemy_ema2_damage_call *,
    const gh_enemy_ema2_damage_bindings *,uint32_t *frontier);

#ifdef __cplusplus
}
#endif
#endif
