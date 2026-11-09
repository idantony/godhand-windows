/* Recovered original mcport.rel cMcBoot slotCheck@0x398/fileCheck@0x5D0,
 * actual ISO module hash/evidence: analysis/memory-card-semantics.json.
 * This is a source slice, not a complete replacement for cMcBoot Main/UI. */
#include "mc_boot.h"
#include <string.h>
static void reset_icons(gh_mc_boot_checks *s)
{
    unsigned i;int32_t none=-1;
    memset(s->icon_work.pointer,0,0x5000);
    for(i=0;i<20;++i)memcpy((unsigned char*)s->icon_work.pointer+i*0x400+4,&none,4);
    s->file_index=0;s->load_index=0;s->present_mask=0;
}
gh_mc_boot_error gh_mc_boot_checks_create(gh_mc_boot_checks *s,gh_heap *heap)
{
    if(!s || !heap)return GH_MC_BOOT_ARGUMENT;
    memset(s,0,sizeof(*s));s->error=-1;
    if(gh_heap_allocate(heap,0x5000,0x40,&s->icon_work)!=GH_MEM_OK) {s->done=1;return GH_MC_BOOT_MEMORY;}
    reset_icons(s);return GH_MC_BOOT_OK;
}
gh_mc_boot_error gh_mc_boot_checks_release(gh_mc_boot_checks *s,gh_heap *heap)
{
    if(!s || !heap)return GH_MC_BOOT_ARGUMENT;
    if(s->icon_work.pointer && gh_heap_release(heap,s->icon_work)!=GH_MEM_OK)return GH_MC_BOOT_MEMORY;
    s->icon_work.pointer=0;s->icon_work.owner=0;return GH_MC_BOOT_OK;
}
static void transition(gh_mc_boot_checks *s,uint32_t phase)
{s->frame=0;s->phase=phase;s->subphase=0;}
static int accepted(gh_mc_boot_checks *s,gh_mc_error e)
{s->error=-1;if(e==GH_MC_OK)return 1;s->error=9;return 0;}
gh_mc_boot_error gh_mc_boot_check_control(gh_mc_boot_checks *s,gh_memory_card *card,uint32_t *flags)
{
    gh_mc_result r;gh_mc_error e;int completed;
    if(!s || !card || !flags || !s->icon_work.pointer)return GH_MC_BOOT_ARGUMENT;
    if(s->delay_frames)return GH_MC_BOOT_NEEDS_SLEEP;
    if(s->done)return GH_MC_BOOT_OK;
    if(s->phase==0) {
        if(s->subphase==0) {
            s->mode=2;reset_icons(s);*flags&=~UINT32_C(4);
            s->has_min_free_space=0;s->retry=0;
        }
        if(s->subphase==0 || s->subphase==1) {
            e=gh_memory_card_request_info(card);
            if(!accepted(s,e)) {transition(s,3);return GH_MC_BOOT_IO;}
            s->subphase=2;
        } else if(s->subphase==2) {
            e=gh_memory_card_poll(card,&completed,&r);
            if(e)return GH_MC_BOOT_IO;
            if(completed) {
                s->error=-1;
                /* Original GetInfoCheck accepts result0 and -1/-2. */
                if(r.result==0 || r.result==-1 || r.result==-2) {
                    if(r.card_type==2) {
                        if(!r.formatted) {
                            ++s->retry;
                            if(s->retry<3)s->subphase=1;
                            else {s->done=1;if(s->frame<30){s->delay_frames=30-s->frame;s->phase_after_delay=0;return GH_MC_BOOT_NEEDS_SLEEP;}}
                        } else {
                            s->has_min_free_space=(uint8_t)(r.free_clusters>=150);
                            if(s->has_min_free_space && s->frame<30) {
                                s->delay_frames=30-s->frame;s->phase_after_delay=1;
                                return GH_MC_BOOT_NEEDS_SLEEP;
                            }
                            transition(s,1);
                        }
                    } else {s->error=1;transition(s,3);}
                } else {s->error=1;transition(s,3);}
            }
        }
    } else if(s->phase==1) {
        if(s->subphase==0) {s->subphase=1;s->file_index=0;}
        if(s->subphase==1) {
            e=gh_memory_card_request_icon(card,(uint32_t)s->file_index);
            if(!accepted(s,e)) {transition(s,3);return GH_MC_BOOT_IO;}
            s->subphase=2;
        }
        while(s->subphase==2) {
            e=gh_memory_card_poll(card,&completed,&r);
            if(e)return GH_MC_BOOT_IO;
            if(!completed) {++s->frame;return GH_MC_BOOT_OK;}
            s->error=-1;
            if(r.result>0)s->present_mask|=UINT32_C(1)<<(unsigned)s->file_index;
            else if(r.result!=0 && r.result!=-4 && r.result!=-6) {
                s->error=r.result==-2?-1:1;transition(s,3);++s->frame;return GH_MC_BOOT_OK;
            }
            ++s->file_index;
            if(s->file_index>=20) {transition(s,2);++s->frame;return GH_MC_BOOT_OK;}
            e=gh_memory_card_request_icon(card,(uint32_t)s->file_index);
            if(!accepted(s,e)) {transition(s,3);return GH_MC_BOOT_IO;}
            s->subphase=2;
        }
    } else return GH_MC_BOOT_UNSUPPORTED;
    ++s->frame;
    return GH_MC_BOOT_OK;
}
gh_mc_boot_error gh_mc_boot_delay_completed(gh_mc_boot_checks *s)
{
    if(!s || !s->delay_frames)return GH_MC_BOOT_ARGUMENT;
    s->delay_frames=0;
    if(!s->done)transition(s,s->phase_after_delay);
    ++s->frame;
    return GH_MC_BOOT_OK;
}
