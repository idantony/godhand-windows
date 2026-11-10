#ifndef GH_R020_TITLE_H
#define GH_R020_TITLE_H
#include "../core/game_init.h"
#include "../core/id_base.h"
#include "../core/dvd_jobs.h"
#include "../core/bgm_requests.h"
#include "../core/screen_fade.h"
#include "../core/game_scene_setup.h"
#include "../core/id_jobs.h"
#include "../core/display_state.h"
#include "../platform/task_threads.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct gh_r020_title gh_r020_title;
typedef enum {GH_R020_MEMORY_UNKNOWN,GH_R020_MEMORY_ZERO_REFERENCE,GH_R020_MEMORY_PROVIDED_BYTES} gh_r020_memory_policy;
/* Original allocation leaves alpha phases30..48 unwritten before TitleID.
 * NULL/default preserves unknowns. Supplied bytes are copied only at creation;
 * ZERO is an explicit host reference policy, not observed EE malloc contents.
 * vu_nearest_reference opts into the bounded original polynomial/SQRT model. */
typedef struct {gh_r020_memory_policy memory;const uint8_t *bytes;size_t size;int vu_nearest_reference;} gh_r020_reference_profile;
typedef struct {
 gh_boot_work *work;gh_core_save_view save;gh_game_mt *random;
 gh_dvd_jobs *dvd;gh_heap *room_heap;gh_id_state *id;gh_id_runtime *runtime;
 gh_bgm_requests *bgm;gh_screen_fade *fade;
 gh_task_threads *threads; /*borrow actual executing Main backend for Jump sleep*/
 gh_id_view room_jump; /*canonicalactualasset, notWork+5A4guestpointer*/
 gh_scene_hook *scenario_exit; /*canonicalScenarioEC/F0typedslot,notroomrecord+4*/
 const gh_r020_reference_profile *reference_profile; /*optional explicit unknown-memory/math adaptation*/
 gh_display_state *display; /*borrowONEinitializedglobaldisplay, requiredbyMoveFade*/
} gh_r020_title_bindings;
typedef enum {GH_R020_OK,GH_R020_ARGUMENT,GH_R020_MEMORY,GH_R020_BUSY,
 GH_R020_DVD,GH_R020_ID,GH_R020_SAVE,GH_R020_UNSUPPORTED} gh_r020_status;
typedef struct {
 const uint8_t *fields;size_t field_bytes;const gh_id_base *id_base;
 gh_bundle_entry idd;const char *path;uint32_t frontier;
 uint8_t initialized,exit_registered;
 uint64_t move_serial;uint32_t draw_jobs;
 uint8_t frontier_relative; /*onlyexplicitRELentries;externalcallbacklowIDsarenotinferred*/
} gh_r020_title_view;
typedef gh_r020_status (*gh_r020_begin_frame)(void *,const gh_id_base *,uint32_t *frontier);
typedef gh_r020_status (*gh_r020_draw_entry)(void *,const gh_id_base *,const gh_id_draw_job *,uint32_t *frontier);
/* OriginalLogoSeCall(bank0,program2D,-1,-1,0,0); callback mustreallyexecute
 * native request semantics, not merely acknowledge a parseddescriptor. */
typedef gh_r020_status (*gh_r020_logo_se)(void *,uint32_t *frontier);
typedef gh_r020_status (*gh_r020_program_se)(void *,unsigned bank,unsigned program,uint32_t *frontier);
typedef struct {uint16_t room;float position[3],yaw;uint8_t kind,transition;uint32_t flags;} gh_r020_room_jump_request;
typedef gh_r020_status (*gh_r020_begin_room_jump)(void *,const gh_r020_room_jump_request *,uint32_t *frontier);
/* These providers must execute original openLoad and SubScrOn(5,0,0),
 * respectively, on the canonical owners; an absent provider stops at its entry. */
typedef gh_r020_status (*gh_r020_open_panel)(void *,uint32_t *frontier);
typedef struct {void *context;gh_r020_begin_frame begin_frame;gh_r020_draw_entry draw_entry;gh_r020_logo_se logo_se;gh_r020_program_se program_se;gh_r020_begin_room_jump begin_room_jump;gh_r020_open_panel open_load,open_options;} gh_r020_move_callbacks;
/* Actual R020Init@1A0/Initialize@278 prefix, never a roomflow substitute.
 * Only call from Scenario record+8 at its original2C3440 position, after all
 * originalcase3 predecessors. Real synchronous201160 DVDpump, ID resource2/
 * id0 andCoreSave writes precede BgmEventSet. OnlyrealcompleteBGMbackend
 * permitsInitialize tail andactualScenarioexit registration. Partialobject
 * remains owned for inspection/teardown whenanydependency fails.
 * Objectslot must be unique (originalmoduleglobal3E00); retry returnsBUSY. */
gh_r020_status gh_r020_title_init_prefix(const gh_r020_title_bindings *,gh_r020_title **);
gh_r020_status gh_r020_title_init(const gh_r020_title_bindings *,gh_r020_title **);
gh_r020_status gh_r020_title_inspect(const gh_r020_title *,gh_r020_title_view *);
/* ActualMove@220->650, Wait8C8, Logo2E80, Title970 and Jump780. SharedFade is queried/startonly;
 * originalframeowner steps it AFTERtasktraversal. No hiddenadvancingstatus.
 * Nativebegin_frame expiresfrontendjobs oncepercall ifsupplied; actualTrans
 * visiblejobs require realdraw_entry. Work1A0 pressedbits arecanonicalinput.
 * Load/options/room transition providers are mandatory at actual call sites;
 * failures retain Main/REL frontier identity. Jump wait uses real task sleep. */
gh_r020_status gh_r020_title_move(gh_r020_title *,const gh_r020_move_callbacks *);
/* Native abort follows actualQuit writes and IDBase/slot2/readallocation
 * release. Later ID allocations must firstrelease; BUSY makesnoownerwrites.
 * No Fade/BGM/sharedasset owner is destroyed. */
gh_r020_status gh_r020_title_destroy(gh_r020_title *);
#ifdef __cplusplus
}
#endif
#endif
