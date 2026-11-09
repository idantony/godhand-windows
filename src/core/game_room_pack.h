#ifndef GH_GAME_ROOM_PACK_H
#define GH_GAME_ROOM_PACK_H
#include "dvd_jobs.h"
#include "../assets/core_bundle.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum {GH_PACK_OK,GH_PACK_ARGUMENT,GH_PACK_BUSY,GH_PACK_DVD,GH_PACK_MEMORY} gh_room_pack_status;
typedef struct {
 gh_dvd_jobs *dvd;gh_heap *heap;gh_task_threads *threads;
 gh_r207_allocation allocation;size_t file_bytes;uint32_t allocated_bytes,job;
 /* Original7558A0/4 aliases Work.pack+allocation, not a second allocation. */
 const uint8_t *saved_pointer;uint32_t saved_bytes;uint16_t room;int loaded;
 uint32_t failed_entry;int detail;
} gh_room_pack;
gh_room_pack_status gh_game_room_path(uint16_t room,char out[20]);
/* Actual31CC48:200DF8 ID0/override0/arg0/priority0, then201108 realfiber
 * wait including mandatoryfinalsleep. Frameowner pumpsDVDaftertasktraversal.
 * Success means original fileload+backup completed, not resource consumers. */
gh_room_pack_status gh_game_room_pack_load_0031cc48(gh_room_pack *,uint16_t room);
gh_room_pack_status gh_game_room_pack_release(gh_room_pack *);
/* BoundedSearchData-compatiblebundle view; doesnotrelocate or execute REL. */
gh_bundle_error gh_game_room_pack_bundle(const gh_room_pack *,gh_bundle *);
#ifdef __cplusplus
}
#endif
#endif
