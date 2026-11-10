#ifndef GH_GAME_COCKPIT_H
#define GH_GAME_COCKPIT_H
#include "id_jobs.h"
#include "game_init.h"
#include "cockpit_fan.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct gh_game_cockpit gh_game_cockpit;
typedef struct {uint32_t offset,kind,vtable,method;int initialized;} gh_cockpit_widget;
/* Cold native141678 constructor. ID runtime, immutable language bundle and RNG
 * outlive this owner. Raw fields expose scalar bytes and diagnostic vtable IDs;
 * pointer fields use typed accessors and are never guest host-pointer casts. */
gh_id_error gh_game_cockpit_create(gh_id_runtime *,const gh_id_state *,gh_game_mt *,gh_game_cockpit **);
/* Actual140C88 initialization, no draw/output success implied. On a callee
 * frontier earlier mutations and owned ID runs remain inspectable. */
gh_id_error gh_game_cockpit_initialize_00140c88(gh_game_cockpit *,gh_core_save_view);
/* Actual140DE0 once-per-room reset, same registered11widgets in17slots.
 * Borrows canonicalHeat; firstboot Work5F4 playergetter mustbeNULL (typed
 * nonNULLplayer binding remainsunsupported), never creates newbases.
 * Currentlyfirstboot gauge-limit profile is actualELF3BD70C=95. */
gh_id_error gh_game_cockpit_room_reset_00140de0(gh_game_cockpit *,gh_core_save_view,const gh_boot_work *,const uint8_t heat[0x18]);
/* Actual141628 roomQuit only, preservingglobal405 IDruns/SystemMenu. Six
 * concrete widget methods; dynamic2220 release still needs its real owner. */
gh_id_error gh_game_cockpit_room_quit_00141628(gh_game_cockpit *,gh_core_save_view,const uint8_t heat[0x18]);
gh_id_error gh_game_cockpit_destroy(gh_game_cockpit *);
gh_id_error gh_game_cockpit_base(gh_game_cockpit *,uint32_t original_offset,gh_id_base **);
gh_id_error gh_game_cockpit_widget_at(const gh_game_cockpit *,unsigned,gh_cockpit_widget *);
gh_id_error gh_game_cockpit_entry_at(const gh_game_cockpit *,uint32_t original_pointer_field,gh_id_entry **);
const uint8_t *gh_game_cockpit_fields(const gh_game_cockpit *,size_t *);
uint32_t gh_game_cockpit_frontier(const gh_game_cockpit *);
int gh_game_cockpit_ready(const gh_game_cockpit *);
/* Canonicalscalarborrow for actual1F75B0, not a mutablewholeclasscast. */
uint8_t *gh_game_cockpit_action_flags_1414(gh_game_cockpit *);
/* Same global3BD6E8 widget1AE0+1C0; actual108AB0 tail writes this byte. */
uint8_t *gh_game_cockpit_auxiliary_flags_1ca0(gh_game_cockpit *);
/* Same SystemMenu widget atCockpit+3A0: typed Base and original+90..BF
 * scalar-only span. No guest/native pointer fields are exposed as bytes. */
typedef struct {gh_id_base *base;uint8_t *scalars90;size_t size;} gh_cockpit_system_menu_view;
gh_id_error gh_game_cockpit_get_system_menu(gh_game_cockpit *,gh_cockpit_system_menu_view *);
/* Original tutorial at SAME Cockpit+1A20. Native presence replaces its B0
 * guest pointer; actual resource storage stays owned by tutorial consumer.
 * begin registers its run in this Cockpit's existing owned[] list, finish
 * writes the original seven typed refs and scalar/entry fields. */
typedef struct {gh_id_base *base;const gh_id_state *manager;uint32_t read_id;uint16_t current_message;
 uint8_t data_present,state_b8;} gh_cockpit_tutorial_view;
gh_id_error gh_game_cockpit_get_tutorial(gh_game_cockpit *,gh_cockpit_tutorial_view *);
gh_id_error gh_game_cockpit_tutorial_mark_resource(gh_game_cockpit *,uint32_t read_id);
gh_id_error gh_game_cockpit_tutorial_begin_base(gh_game_cockpit *);
gh_id_error gh_game_cockpit_tutorial_finish_00147028(gh_game_cockpit *);
/* Native borrowed-view teardown before freeing the resource/ID compaction.
 * Not a claim that original147188 freed its ID run (it only clears B0/AC/B8).
 * Requires Main/consumers cancelled, exactly as other source-view releases. */
gh_id_error gh_game_cockpit_tutorial_detach(gh_game_cockpit *);
/* Actual frame consumers, same explicit nativebinary32/seed0 reference policy
 * as room reset, not EE/VU bit identity. Successful IDTrans callback must queue/execute real
 * entries, never acknowledge unknown consumers. Pointer stays owned here. */
typedef struct {
 void *context;
 gh_id_error (*trans)(void *,gh_id_base *,uint32_t *frontier);
 gh_id_error (*set_draw_order)(void *,uint32_t bank,uint32_t depth,uint32_t *frontier);
 gh_id_post_message post;void *post_context;
 gh_core_save_view save;
 gh_id_error (*fan)(void *,const gh_cockpit_fan *,uint32_t *frontier);
} gh_cockpit_frame_callbacks;
/* Loads original immutable radar curves41E780/41E7A8 and scalar3BD6F4.
 * One canonical Cockpit owner retains its original global pulse companion. */
gh_id_error gh_game_cockpit_bind_frame_elf(gh_game_cockpit *,const void *,size_t);
gh_id_error gh_game_cockpit_move_00140e58(gh_game_cockpit *,const gh_boot_work *,const gh_cockpit_frame_callbacks *);
gh_id_error gh_game_cockpit_trans_001410a8(gh_game_cockpit *,const gh_boot_work *,const gh_cockpit_frame_callbacks *);
/* Borrowed SAME original1414..1467 fields for actual1F75B0/1F7680.
 * Byte pointers avoid inventing pointer-bearing guest layout/alignment. */
typedef struct {uint8_t *flags,*icon,*screen_x,*screen_y,*message;const void *identity;} gh_cockpit_action_view;
gh_id_error gh_game_cockpit_action_view_001f7680(gh_game_cockpit *,gh_cockpit_action_view *);
#ifdef __cplusplus
}
#endif
#endif
