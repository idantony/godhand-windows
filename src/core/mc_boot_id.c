/* Resource binding portion of mcport SetIdData@82F0; full IDBase UI remains pending. */
#include "mc_boot_id.h"
#include <string.h>
#include <stdio.h>
gh_mc_boot_error gh_mc_boot_load_id_resource(gh_mc_id_resource *r,gh_dvd_jobs *dvd,
    gh_heap *heap,gh_task_threads *threads,gh_id_state *manager,int8_t requested,int8_t language)
{
    static const char *const formats[]={"id/%s.idd","id/local/US/%s_US.idd",
        "id/local/EN/%s_EN.idd","id/local/FR/%s_FR.idd","id/local/GE/%s_GE.idd",
        "id/local/IT/%s_IT.idd","id/local/SP/%s_SP.idd"};
    gh_disc_entry entry;size_t index;gh_disc_error found;
    int selected=requested==-1?language:requested;
    if(!r || !dvd || !dvd->disc || !heap || !threads || !manager || r->allocation.pointer)return GH_MC_BOOT_ARGUMENT;
    r->path[0]=0;r->preferred_path_found=0;r->pair_bound=0;
    if((unsigned)selected<7) {
        if(snprintf(r->path,sizeof(r->path),formats[selected],"idsavecore")<0)return GH_MC_BOOT_ARGUMENT;
        found=gh_disc_find(dvd->disc,r->path,&index);
        if(found==GH_DISC_OK)r->preferred_path_found=1;
        else if(found!=GH_DISC_NOT_FOUND && found!=GH_DISC_UNSAFE_PATH)return GH_MC_BOOT_IO;
    }
    if(!r->preferred_path_found) {
        if(snprintf(r->path,sizeof(r->path),formats[0],"idsavecore")<0)return GH_MC_BOOT_ARGUMENT;
        if(gh_disc_find(dvd->disc,r->path,&index)!=GH_DISC_OK)return GH_MC_BOOT_IO;
    }
    if(gh_disc_entry_at(dvd->disc,index,&entry)!=GH_DISC_OK)return GH_MC_BOOT_IO;
    r->job_id=gh_dvd_read_alloc(dvd,r->path,&r->allocation,heap,0,0,0,0);
    if(!r->job_id)return dvd->last_error==GH_DVD_MEMORY?GH_MC_BOOT_MEMORY:GH_MC_BOOT_IO;
    /* SetIdData uses Check + cMcSys_sleep(1), not CheckWaitFrame's extra final
     * sleep. This resource-only boundary supplies native task sleep; the full
     * original UI Move/Trans frame pump remains outside this slice. */
    while(gh_dvd_check(dvd,r->job_id)) {
        if(gh_task_threads_sleep(threads,1)!=GH_THREAD_OK) {
            (void)gh_dvd_cancel(dvd,r->job_id);return GH_MC_BOOT_IO;
        }
    }
    if(dvd->failed_id==r->job_id && dvd->last_error==GH_DVD_IO)return GH_MC_BOOT_IO;
    memset(&r->image,0,sizeof(r->image));r->image.bytes=r->allocation.pointer;r->image.size=(size_t)entry.size;
    if(gh_id_set_data_002accf8(manager,0xb,&r->image)!=GH_MENU_OK)return GH_MC_BOOT_UNSUPPORTED;
    r->pair_bound=1;return GH_MC_BOOT_OK;
}
gh_mc_boot_error gh_mc_boot_release_id_resource(gh_mc_id_resource *r,gh_heap *heap,gh_id_state *manager)
{
    if(!r || !heap || !manager)return GH_MC_BOOT_ARGUMENT;
    if(r->pair_bound || r->allocation.pointer) {memset(&manager->pair[0xb],0,sizeof(manager->pair[0xb]));r->pair_bound=0;}
    if(r->allocation.pointer && gh_heap_release(heap,r->allocation)!=GH_MEM_OK)return GH_MC_BOOT_MEMORY;
    memset(&r->allocation,0,sizeof(r->allocation));memset(&r->image,0,sizeof(r->image));
    return GH_MC_BOOT_OK;
}
