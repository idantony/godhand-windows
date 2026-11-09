#ifndef GH_GAME_MOVIE_POOL_H
#define GH_GAME_MOVIE_POOL_H
#include "boot.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct gh_movie_pool gh_movie_pool;
typedef enum {GH_MOVIE_OK,GH_MOVIE_ARGUMENT,GH_MOVIE_MEMORY,GH_MOVIE_BUSY,GH_MOVIE_UNSUPPORTED,GH_MOVIE_NATIVE} gh_movie_status;
typedef enum {GH_MOVIE_UPDATE_002D3750,GH_MOVIE_STOP_002D42E0} gh_movie_pool_operation;
/* Required onlyforliveplayer. Mustperformactualnativepoll/updateorstop;
 * close/releaseonEOF+audioDONEmustprecedetrack_closed below. No dummyack.
 * Callback mayclearthistrack throughtrack_closed; no recursivepoolupdate.
 * STOPoperationisissuedbytheLATERupdateafter2D42E0setsstate60=1;
 * stop_002b4d68onlywritesrealrequest/playingflag, not completion. */
typedef gh_movie_status (*gh_movie_provider)(void *,gh_movie_pool *,unsigned slot,void *native_player,gh_movie_pool_operation,uint32_t *frontier);
typedef struct {void *native_player;uint32_t cached_handle,clip_flags;const uint8_t *scalar_bytes;size_t size;} gh_movie_track_view;
typedef struct {uint32_t counter,busy_mask,movie_active_3c3cd4;uint8_t busy;float default_fps;uint32_t frontier;} gh_movie_pool_view;
/* Game manager/3track portion ofactual2B4960. Nativecodec/globalallocator
 * setupbelongsprovider, not emulatedCRIdeviceinit. Coldtracks trulyhave
 * no nativeplayer; no decoderready/outputcompletion isclaimedbycreate. */
gh_movie_status gh_movie_pool_create_002b4960(gh_movie_pool **);
gh_movie_status gh_movie_pool_set_provider(gh_movie_pool *,gh_movie_provider,void *);
gh_movie_status gh_movie_pool_first_free_002b4f38(const gh_movie_pool *,int *slot);
/* CommitonlyAFTERrealbackendopen. OriginalMovieStart game-side state:
 * originalcounterincrementsandavoids0,cacheEEhandle,LOCKPADmask/inputbits,
 * global3C3CD4=1,Work+5E4|2000. Doesnotopen/decodeaplayerbyitself. */
gh_movie_status gh_movie_pool_commit_started(gh_movie_pool *,unsigned slot,void *native_player,uint32_t clip_flags,gh_boot_work *,uint32_t *handle);
/* NotifyonlyAFTERrealnativeclose. Resetsoriginaltrackscalarfields, preserves
 * cachedhandle/busymaskuntilnext2B4A50visit, exactlytheoriginaltiming. */
gh_movie_status gh_movie_pool_track_closed(gh_movie_pool *,unsigned slot,void *native_player);
gh_movie_status gh_movie_pool_update_002b4a50(gh_movie_pool *,gh_boot_work *);
gh_movie_status gh_movie_pool_is_idle_002b4e48(gh_movie_pool *,gh_boot_work *,int *idle);
gh_movie_status gh_movie_pool_stop_002b4d68(gh_movie_pool *,gh_boot_work *,uint32_t handle);
gh_movie_status gh_movie_pool_track_at(const gh_movie_pool *,unsigned,gh_movie_track_view *);
void gh_movie_pool_inspect(const gh_movie_pool *,gh_movie_pool_view *);
/* Activeplayersmustbetrulyclosedbytheirproviderfirst. Rootownsonepoolfrom
 * startupthroughMovie0andMain/services; servicesonlyborrowsit. */
gh_movie_status gh_movie_pool_destroy(gh_movie_pool *);
#ifdef __cplusplus
}
#endif
#endif
