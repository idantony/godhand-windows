#ifndef GH_GAME_COCKPIT_H
#define GH_GAME_COCKPIT_H
#include "id_jobs.h"
#include "game_init.h"
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
gh_id_error gh_game_cockpit_destroy(gh_game_cockpit *);
gh_id_error gh_game_cockpit_base(gh_game_cockpit *,uint32_t original_offset,gh_id_base **);
gh_id_error gh_game_cockpit_widget_at(const gh_game_cockpit *,unsigned,gh_cockpit_widget *);
gh_id_error gh_game_cockpit_entry_at(const gh_game_cockpit *,uint32_t original_pointer_field,gh_id_entry **);
const uint8_t *gh_game_cockpit_fields(const gh_game_cockpit *,size_t *);
uint32_t gh_game_cockpit_frontier(const gh_game_cockpit *);
int gh_game_cockpit_ready(const gh_game_cockpit *);
#ifdef __cplusplus
}
#endif
#endif
