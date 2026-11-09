#ifndef GODHAND_CORE_ID_BASE_H
#define GODHAND_CORE_ID_BASE_H
#include "menu_state.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct gh_id_view {const unsigned char *bytes;size_t size,offset;} gh_id_view;
typedef enum gh_id_error {
    GH_ID_OK,GH_ID_ARGUMENT,GH_ID_BOUNDS,GH_ID_NOT_FOUND,GH_ID_FORMAT,
    GH_ID_CAPACITY,GH_ID_MEMORY,GH_ID_BUSY,GH_ID_UNSUPPORTED
} gh_id_error;
typedef struct gh_id_entry gh_id_entry;
struct gh_id_entry {
    const unsigned char *source,*parts[5];
    gh_id_entry *parent;
    gh_id_view texture;
    uint32_t source_index,flags,seed,color_base,color;
    float off_x,off_y,pos_x,pos_y,scale_x,scale_y,rotation_speed;
    float draw_x,draw_y,draw_scale_x,draw_scale_y,draw_rotation;
    float f68,f6c,f70[4],f80,rotation_x,rotation_y,path_length;
    int16_t jitter[5],h8e;
    uint16_t message;
    uint8_t b28,kind,b89,b8a,b8b,b8d;
    int8_t parent_ref,id,layer,b8c;
};
typedef struct gh_id_runtime gh_id_runtime;
typedef struct gh_id_base {
    gh_id_view source_records,packed;
    gh_id_entry *entries;
    uint32_t entry_count;
    int32_t id,resource_number;
    uint8_t stop,hide,playing,b1d,b1e,mode;
    uint16_t frame;
    float vector_a[4],vector_b[4],f40;
    int16_t h44;
    uint8_t initialized,owns_run;
    uint32_t first_unported;
} gh_id_base;
typedef uint32_t (*gh_id_random_u32)(void *);
typedef struct gh_id_pool_run {gh_id_entry *entries;uint32_t start,count;} gh_id_pool_run;
typedef struct gh_id_job_view {int32_t tag,arg;uint32_t start;int8_t wire_count;uint8_t active;} gh_id_job_view;
/* Uses immutable TBL0 pair data. n is one-based; zero offsets/idx33 mean NULL
 * in the original. Host bounds are checked; no offset becomes a host pointer
 * without a validated containing extent. */
gh_id_error gh_id_lookup_002acd78(const gh_id_state *,uint32_t slot,uint32_t n,gh_id_view *);
/* Runtime owns typed host entries, distinct from the manager's guest-shaped
 * opaque pool. Requires an initialized empty manager. default_manager controls
 * original global-identity SRC_SKIP filtering. No game pointer truncation. */
gh_id_error gh_id_runtime_create(gh_id_state *,int default_manager,gh_id_runtime **);
gh_id_error gh_id_runtime_destroy(gh_id_runtime *);
gh_id_error gh_id_pool_allocate_002ace48(gh_id_runtime *,uint32_t count,int32_t tag,int32_t arg,gh_id_pool_run *);
/* Original free helper only reclaims a matching LAST run, compares tag but
 * ignores arg, and sign-extends stored8-bit count. Nonmatching returns freed0. */
gh_id_error gh_id_pool_release_002acf30(gh_id_runtime *,gh_id_pool_run,int32_t tag,int *freed);
gh_id_error gh_id_runtime_job(const gh_id_runtime *,uint32_t,gh_id_job_view *);
void gh_id_base_clear_002a9df8(gh_id_base *);
/* Zero-init or previously release object before init. Returns original return
 * separately:0 missing resource,1 resource existed, -1 if unported work blocked.
 * initialized means original entry init/reset completed, NOT render readiness.
 * Actual USA MC67 records need neither RAND nor path-sqrt branches. Missing
 * required RNG or a nontrivial2ABC30 path reports exact first_unported address.
 * Source bundle and runtime must outlive the object. */
gh_id_error gh_id_base_initialize_002a9ea0(gh_id_runtime *,gh_id_base *,int32_t resource,
    int32_t id,gh_id_random_u32,void *random_context,int *original_return);
/* Original clears only src/entries/packed if it had entries, even if pool
 * LIFO reclaim fails. entry_count/scalar fields are retained. */
gh_id_error gh_id_base_release_002aa078(gh_id_runtime *,gh_id_base *,int *original_return,int *pool_reclaimed);
/* Original2AA410 returns shared dummy entryD005CB300 when missing/inactive,
 * not NULL. Host dummy lives in runtime; fallback reports that exact case. */
gh_id_error gh_id_base_get_work_002aa410(gh_id_runtime *,gh_id_base *,int32_t id,gh_id_entry **,int *fallback);
/* Original family call modifies parents/children, not the selected entry. */
gh_id_error gh_id_base_set_family_display(gh_id_runtime *,gh_id_base *,int32_t id,int show);
gh_id_error gh_id_base_restart_anim(gh_id_base *,gh_id_random_u32,void *context);
#ifdef __cplusplus
}
#endif
#endif
