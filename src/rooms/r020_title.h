#ifndef GH_R020_TITLE_H
#define GH_R020_TITLE_H
#include "../core/game_init.h"
#include "../core/id_base.h"
#include "../core/dvd_jobs.h"
#include "../core/bgm_requests.h"
#include "../core/screen_fade.h"
#include "../core/game_scene_setup.h"
#include "../core/id_jobs.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct gh_r020_title gh_r020_title;
typedef struct {
 gh_boot_work *work;gh_core_save_view save;gh_game_mt *random;
 gh_dvd_jobs *dvd;gh_heap *room_heap;gh_id_state *id;gh_id_runtime *runtime;
 gh_bgm_requests *bgm;gh_screen_fade *fade;
 gh_id_view room_jump; /*canonicalactualasset, notWork+5A4guestpointer*/
 gh_scene_hook *scenario_exit; /*canonicalScenarioEC/F0typedslot,notroomrecord+4*/
} gh_r020_title_bindings;
typedef enum {GH_R020_OK,GH_R020_ARGUMENT,GH_R020_MEMORY,GH_R020_BUSY,
 GH_R020_DVD,GH_R020_ID,GH_R020_SAVE,GH_R020_UNSUPPORTED} gh_r020_status;
typedef struct {
 const uint8_t *fields;size_t field_bytes;const gh_id_base *id_base;
 gh_bundle_entry idd;const char *path;uint32_t frontier;
 uint8_t initialized,exit_registered;
 uint64_t move_serial;uint32_t draw_jobs;
} gh_r020_title_view;
typedef gh_r020_status (*gh_r020_begin_frame)(void *,const gh_id_base *,uint32_t *frontier);
typedef gh_r020_status (*gh_r020_draw_entry)(void *,const gh_id_base *,const gh_id_draw_job *,uint32_t *frontier);
/* OriginalLogoSeCall(bank0,program2D,-1,-1,0,0); callback mustreallyexecute
 * native request semantics, not merely acknowledge a parseddescriptor. */
typedef gh_r020_status (*gh_r020_logo_se)(void *,uint32_t *frontier);
typedef struct {void *context;gh_r020_begin_frame begin_frame;gh_r020_draw_entry draw_entry;gh_r020_logo_se logo_se;} gh_r020_move_callbacks;
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
/* ActualMove@220->650 andMoveLogo@2E80. SharedFade is queried/startonly;
 * originalframeowner steps it AFTERtasktraversal. No hiddenadvancingstatus.
 * Nativebegin_frame expiresfrontendjobs oncepercall ifsupplied; actualTrans
 * visiblejobs require realdraw_entry. Work1A0 pressedbits arecanonicalinput.
 * Wait/Title/RoomJump othermodes retain their preciseRELentryfrontiers. */
gh_r020_status gh_r020_title_move(gh_r020_title *,const gh_r020_move_callbacks *);
/* Native abort follows actualQuit writes and IDBase/slot2/readallocation
 * release. Later ID allocations must firstrelease; BUSY makesnoownerwrites.
 * No Fade/BGM/sharedasset owner is destroyed. */
gh_r020_status gh_r020_title_destroy(gh_r020_title *);
#ifdef __cplusplus
}
#endif
#endif
