#ifndef GH_GAME_SUBSCREEN_SAVE_H
#define GH_GAME_SUBSCREEN_SAVE_H
#include "game_children.h"
#include "dvd_jobs.h"
#include "game_init.h"
#include "game_title_player.h"
#include "packed_message.h"
#include "screen_fade.h"
#include "game_scene_aux.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef gh_child_status (*gh_save_map_sound)(void *,uint16_t program,uint32_t *frontier);
typedef struct {
    gh_boot_work *work;
    gh_game_title_player *player;
    gh_game_scene_aux *cursor;
    gh_core_save_view save;
    gh_dvd_jobs *dvd;
    gh_heap *subscreen_heap;
    gh_id_state *manager;
    gh_id_runtime *id_runtime;
    gh_font_state *font;
    gh_packed_message_state *packed;
    const gh_bundle *language;
    gh_screen_fade *fade;
    gh_screen_fade_context fade_context;
    uint16_t map_room,target_room;
    gh_save_map_sound sound;
    void *sound_context;
} gh_save_map_bindings;
typedef struct {
    gh_id_base *base;
    uint16_t chapter_message,selected,target,exit_kind;
    uint8_t variant,active,visible;
} gh_save_map_view;
/* Child3's original 002AAC28 IDBase Trans. The chapter is cMenu+BEE. */
typedef gh_child_status (*gh_save_map_trans_submit)(void *,gh_id_base *,
    uint16_t chapter_message,uint32_t *frontier);
gh_child_status gh_game_children_draw_with_save_map_00160b90(const gh_game_children *,
    const gh_boot_work *,gh_render_draw_env *,gh_child_cover_submit,
    gh_save_map_trans_submit,void *context,uint32_t *frontier);
/* Actual Door-only SubScrOn(3,2,0x11), sharing the nine-child owner. */
gh_child_status gh_game_children_save_map_open_00160020(gh_game_children *,const gh_save_map_bindings *,int *started,uint32_t *frontier);
/* Actual 1601D8 states for the ordinary Door map and mode0 save handoff.
 * Call on ordinary scene visits. The outer scene pumps DVD and Fade. */
gh_child_status gh_game_children_save_map_move_001601d8(gh_game_children *,const gh_save_map_bindings *,uint32_t *frontier);
/* Original manager Quit and borrowed map release for an active Door screen. */
gh_child_status gh_game_children_save_map_quit_0015ff10(gh_game_children *,const gh_save_map_bindings *,uint32_t *frontier);
gh_child_status gh_game_children_save_map_view(const gh_game_children *,gh_save_map_view *);
#ifdef __cplusplus
}
#endif
#endif
