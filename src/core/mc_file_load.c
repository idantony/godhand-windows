/* Actual mcport.rel fileLoad730 / GetAllSystemFlags8FF0 / GetSystemFlags9088.
 * Native slice scope and function hashes: analysis/mc-file-load-semantics.json. */
#include "mc_file_load.h"
#include <string.h>
static uint32_t word(const unsigned char *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static uint16_t half(const unsigned char *p){return (uint16_t)((unsigned)p[0]|(unsigned)p[1]<<8);}
gh_mc_boot_error gh_mc_file_load_00000730(gh_mc_boot_checks *s,gh_boot_work *work,
    uint8_t original_f8,uint32_t *first)
{
    uint32_t flags;unsigned i;const unsigned char *records;
    if(!s || !work || !first || s->phase!=2 || !s->icon_work.pointer)return GH_MC_BOOT_ARGUMENT;
    *first=0;
    if(s->subphase){*first=0x31c8;return GH_MC_BOOT_UNSUPPORTED;}
    if(s->load_index<0){*first=0x730;return GH_MC_BOOT_UNSUPPORTED;}
    while(s->load_index<20){
        if(s->present_mask&(UINT32_C(1)<<(unsigned)s->load_index)){
            *first=0x3350;return GH_MC_BOOT_UNSUPPORTED;
        }
        ++s->load_index;
    }
    records=s->icon_work.pointer;memcpy(&flags,work->pad588,4);
    for(i=0;i<20;++i){
        const unsigned char *p=records+i*0x400;uint32_t f=word(p+8);uint16_t n=half(p+0x30);
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
