#ifndef GH_GAME_STAGE_H
#define GH_GAME_STAGE_H
#include "game_start.h"
#include "../assets/font_texture.h"
#include "game_init.h"
#include "menu_state.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct gh_stage_textures gh_stage_textures;
typedef enum {GH_STAGE_OK,GH_STAGE_ARGUMENT,GH_STAGE_BOUNDS,GH_STAGE_MEMORY,
 GH_STAGE_UNSUPPORTED,GH_STAGE_UNAVAILABLE,GH_STAGE_THREAD} gh_stage_status;
typedef struct gh_stage_channel {
 uint8_t valid,psm;uint16_t width,height,base,clut_base;
 uint64_t tex0,tex1;const uint8_t *source;
} gh_stage_channel;
typedef struct {uint32_t count,next_channel,image_base,clut_index;} gh_stage_texture_cursor;
/* Concrete platform substitute for150C10(mode1)->309D88->30A200: native GS
 * channel registry plus actual CT32 writes into4MiB swizzled VRAM. Restricted
 * original core.dat bank shape, not a general GIF/GS emulator. Immutable input
 * must outlive registry; channel dedup uses original source identity. */
gh_stage_status gh_stage_textures_create(gh_stage_textures **);
void gh_stage_textures_destroy(gh_stage_textures *);
gh_stage_status gh_stage_texture_load(gh_stage_textures *,const void *bank,size_t bytes,uint32_t *channel);
gh_stage_status gh_stage_core_textures(gh_stage_textures *,const void *core_dat,size_t bytes);
gh_stage_status gh_stage_texture_channel(const gh_stage_textures *,uint32_t,gh_stage_channel *);
gh_stage_status gh_stage_texture_cursor_get(const gh_stage_textures *,gh_stage_texture_cursor *);
/* Reads actual CT32-addressed native VRAM. Used for validating transfers,
 * not an approximation of the later PSMT4/PSMT8 texture sampling pipeline. */
/* Original core.dat PSMT4/PSMT8, CSM1/CSA0, RGBA32 CLUT conversion for
 * native D3D texture resources. GS alpha0..128 maps min(2*a,255), matching
 * existing font/ID atlas APIs; final tint/test/blend remain renderer work. */
gh_stage_status gh_stage_texture_rgba(const gh_stage_textures *,uint32_t channel,
 void *rgba,size_t capacity);
gh_stage_status gh_stage_vram_ct32(const gh_stage_textures *,uint16_t base,uint8_t buffer_width,
 uint32_t x,uint32_t y,uint32_t *raw_pixel);
/* Actual2C8EF8 startup SLD selection +2C90A0(mode0)+2C9950 sorting.
 * Records have36-byte stride; mutating flags uses an owned resource copy.
 * Initialize struct tozero before first use; clear when its owner is destroyed. */
typedef struct gh_stage_skills {uint8_t *image;size_t size;uint32_t count,available;
 uint8_t order[128];} gh_stage_skills;
gh_stage_status gh_stage_skills_initialize(gh_stage_skills *,const void *core_bin,
 size_t bytes,const gh_boot_work *,gh_core_save_view);
void gh_stage_skills_clear(gh_stage_skills *);
gh_stage_status gh_stage_skill_at(const gh_stage_skills *,uint32_t sorted_index,
 uint32_t *original_index,const uint8_t **record36);
typedef struct gh_stage_slots {
 uint8_t *records;size_t bytes;uint32_t count;void *heap_handle;int configured;
} gh_stage_slots;
gh_stage_status gh_stage_slots_reset(gh_stage_slots *,void *heap_handle);
typedef struct gh_stage_case1 gh_stage_case1;
typedef int (*gh_stage_object_init)(void *,gh_stage_case1 *);
struct gh_stage_case1 {
 gh_game_assets *assets;const void *core_bin;size_t core_bin_bytes;
 const gh_bundle *language;gh_stage_textures *textures;
 gh_font_state *font;gh_menu_state *menu;gh_id_state *id,**id_global;
 uint8_t *game_state;gh_core_save_view save;
 uint8_t work_area[0xb0f0],object_state[0x38],aux_state[16];
 const uint8_t *vibration_data,*saved_block;size_t vibration_bytes;
 uint32_t vibration_phase,vibration_flags;
 gh_stage_slots global_slots,room_slots;void *room_heap;
 gh_stage_skills skills;
 void *context;gh_stage_object_init init_children_15fd98,init_cockpit_140c88;
 uint32_t failed_entry;int detail;
};
/* Originalcase1 sequence. Reuses actual ID/font/menu primitives and concrete
 * native texture/manager reset work. Child/cockpit virtualinitializers require
 * true implementations; missing functions stop at exactfrontier,state stays1.
 * Caller zero-initializes this state; owned skills need clear at destruction. */
gh_stage_status gh_game_stage_002a5bb8(gh_stage_case1 *);
#ifdef __cplusplus
}
#endif
#endif
