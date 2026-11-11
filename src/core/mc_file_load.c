/* Actual mcport.rel fileLoad730 / GetAllSystemFlags8FF0 / GetSystemFlags9088.
 * Native slice scope and function hashes: analysis/mc-file-load-semantics.json. */
#include "mc_file_load.h"
#include <string.h>
static uint32_t word(const unsigned char *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static uint16_t half(const unsigned char *p){return (uint16_t)((unsigned)p[0]|(unsigned)p[1]<<8);}
static gh_mc_boot_error platform(gh_mc_error e,uint32_t pc,uint32_t *first)
{
    if(!e)return GH_MC_BOOT_OK;
    *first=pc;
    return e==GH_MC_UNSUPPORTED?GH_MC_BOOT_UNSUPPORTED:GH_MC_BOOT_IO;
}
static void file_error(gh_mc_boot_checks *s,int32_t error)
{
    s->error=error;s->phase=3;s->frame=0;s->subphase=0;
}
gh_mc_boot_error gh_mc_file_load_00000730(gh_mc_boot_checks *s,gh_memory_card *card,gh_boot_work *work,
    uint8_t original_f8,uint32_t *first)
{
    uint32_t flags;unsigned i;unsigned char *records;
    if(!s || !work || !first || s->phase!=2 || !s->icon_work.pointer)return GH_MC_BOOT_ARGUMENT;
    *first=0;
    if(s->load_index<0){*first=0x730;return GH_MC_BOOT_UNSUPPORTED;}
    records=s->icon_work.pointer;
    if(s->subphase){
        if(!card||s->subphase>3||s->load_index>=20){*first=0x31c8;return GH_MC_BOOT_UNSUPPORTED;}
        gh_mc_result result;
        int completed;
        gh_mc_boot_error e=platform(gh_memory_card_poll(card,&completed,&result),0x31c8,first);
        if(e||!completed)return e;
        unsigned char *record=records+(unsigned)s->load_index*GH_MC_SAVE_HEADER_BYTES;
        if(s->subphase==1){
            /* OpenCheck@F00 reports missing files as boot error 2. */
            if(result.result<0){file_error(s,result.result==GH_MC_SAVE_NOT_FOUND?2:1);return GH_MC_BOOT_OK;}
            e=platform(gh_memory_card_request_save_header(card,record),0x35e0,first);
            if(!e)s->subphase=2;
            return e;
        }
        if(s->subphase==2){
            /* ReadCheck@F68 accepts any nonnegative byte count. */
            if(result.result<0){file_error(s,1);return GH_MC_BOOT_OK;}
            e=platform(gh_memory_card_request_save_close(card),0x3300,first);
            if(!e)s->subphase=3;
            return e;
        }
        if(result.result){file_error(s,result.result==GH_MC_SAVE_NOT_FOUND?2:1);return GH_MC_BOOT_OK;}
        /* After CloseCheck@FC8 the original compares the binary32 version to .1f. */
        if(word(record)!=UINT32_C(0x3dcccccd))s->present_mask&=~(UINT32_C(1)<<(unsigned)s->load_index);
        ++s->load_index;
        s->subphase=0;
    }
    while(s->load_index<20){
        if(s->present_mask&(UINT32_C(1)<<(unsigned)s->load_index)){
            if(!card){*first=0x3350;return GH_MC_BOOT_UNSUPPORTED;}
            gh_mc_boot_error e=platform(gh_memory_card_request_save_open(card,(unsigned)s->load_index),0x3350,first);
            if(!e)s->subphase=1;
            return e;
        }
        ++s->load_index;
    }
    memcpy(&flags,work->pad588,4);
    for(i=0;i<20;++i){
        const unsigned char *p=records+i*GH_MC_SAVE_HEADER_BYTES;uint32_t f=word(p+8);uint16_t n=half(p+0x30);
        if(n || (f&1))flags|=0x80000000;
        if(f&2)flags|=0x40000000;if(f&4)flags|=0x20000000;
        if(f&0x200)flags|=0x200000;
        if(f&8)flags|=0x10000000;if(f&0x10)flags|=0x08000000;
        if(f&0x20)flags|=0x04000000;if(f&0x40)flags|=0x02000000;
        if(f&0x80)flags|=0x01000000;if(f&0x100)flags|=0x00400000;
        if(n>98)flags|=0xbf600000;
    }
    memcpy(work->pad588,&flags,4);
    if(s->present_mask)work->flags|=4;
    if(original_f8){s->done=1;return GH_MC_BOOT_OK;}
    s->error=s->present_mask?10:2;s->phase=3;s->frame=0;s->subphase=0;
    return GH_MC_BOOT_OK;
}
