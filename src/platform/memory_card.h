#ifndef GODHAND_PLATFORM_MEMORY_CARD_H
#define GODHAND_PLATFORM_MEMORY_CARD_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct gh_memory_card gh_memory_card;
typedef enum gh_mc_error {GH_MC_OK,GH_MC_ARGUMENT,GH_MC_BUSY,GH_MC_IO,GH_MC_MEMORY,GH_MC_UNSUPPORTED,GH_MC_ENDED} gh_mc_error;
typedef struct gh_mc_result {
    int32_t result; /* original command result: GetInfo0, SearchFile1/0 */
    int32_t card_type,free_clusters,formatted;
} gh_mc_result;
/* Native replacement for the GetInfo/SearchFile/Sync subset used during
 * mcport boot checks. Explicit UTF-8 profile directory stands for slot0 storage.
 * Opening never creates it; a missing directory is genuinely no card.
 * Existing directory maps to compatible/formatted storage, free space is
 * measured on its real host volume in 1024-byte clusters. No EE RPC/thread
 * initialization, writes, formatting or save-data interpretation is claimed. */
gh_mc_error gh_memory_card_open(const char *profile_directory,gh_memory_card **);
void gh_memory_card_close(gh_memory_card *);
/* Native endpoint shutdown corresponding to McPort.End service lifetime.
 * Pending real filesystemcommand returnsBUSY withoutcanceling/mutating it.
 * Idle End isidempotent; furtherrequests/poll returnENDED. Caller stillowns
 * objectandmustcloseit. No guestRPC/thread/semaphore success isfabricated. */
gh_mc_error gh_memory_card_end(gh_memory_card *);
gh_mc_error gh_memory_card_request_info(gh_memory_card *);
/* USA original folder base, slot0..19, icon.sys. No caller path interpolation. */
gh_mc_error gh_memory_card_request_icon(gh_memory_card *,uint32_t save_slot);
/* pending command executes real filesystem query here; completed=1 reports
 * completed result. With no queued command completed=0. Overlap is BUSY. */
gh_mc_error gh_memory_card_poll(gh_memory_card *,int *completed,gh_mc_result *);
#ifdef __cplusplus
}
#endif
#endif
