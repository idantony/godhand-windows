/* Actual2AC9B8; MIT LICENSE.upstream, seeid-entry-reset-evidence.json. */
#include "id_entry_reset.h"
#include "id_path.h"
#include <string.h>
static int half(const uint8_t*p){unsigned n=p[0]|(unsigned)p[1]<<8;return n<32768?(int)n:(int)n-65536;}
static void flag(gh_id_entry*e,uint32_t bit,int on){if(on)e->flags|=bit;else e->flags&=~bit;}
static gh_id_error reset(gh_id_base*o,gh_id_entry*e,const struct gh_id_animation_profile*profile,gh_id_frontier*f){
 uintptr_t base=(uintptr_t)o->source_records.bytes,p=(uintptr_t)e->source;
 if(!e->source||p<base||p-base>o->source_records.size||o->source_records.size-(size_t)(p-base)<0x2e0)return GH_ID_BOUNDS;
 memset(e->jitter,0,sizeof(e->jitter)); /* noRAND, evenifsourceflag40 */
 e->flags&=~0x4000u;
 gh_id_error err=o->path_policy==GH_ID_PATH_REFERENCE_NEAREST?gh_id_path_reset_reference_nearest(e):gh_id_path_reset_002abc30(e);
 if(err){o->first_unported=0x2abc30;if(f){f->entry_index=(uint32_t)(e-o->entries);f->kind=e->kind;f->address=0x2abc30;}return err;}
 const uint8_t*s=e->source;
 if(!s[0x5e])flag(e,0x1000,half(s+0x1b0)<2);
 if(!s[0x5f])flag(e,0x800,half(s+0x214)<2);
 if(!s[0x64])flag(e,0x400,half(s+0x278)<2);
 return profile?gh_id_jobs_calc_entry_reference(o,e,profile,f):gh_id_jobs_calc_entry(o,e,f);
}
static gh_id_error family(gh_id_base*o,gh_id_entry*e,int16_t hide,const struct gh_id_animation_profile*profile,gh_id_frontier*f){
 if(!o)return GH_ID_ARGUMENT;if(f)*f=(gh_id_frontier){0};o->first_unported=0;
 o->frame=0;o->b1d=0;o->b1e=0;if(!e)return GH_ID_OK;
 uintptr_t a=(uintptr_t)o->entries,p=(uintptr_t)e;size_t n=(size_t)o->entry_count*sizeof(*e);
 if(!o->entries||p<a||p-a>=n||(p-a)%sizeof(*e))return GH_ID_BOUNDS;
 gh_id_error err=reset(o,e,profile,f);if(err)return err;
 if(hide)e->flags|=0x7c00u;else e->flags&=0xffff83ffu;
 for(uint32_t i=0;i<o->entry_count;i++)if(o->entries[i].parent==e){
  err=reset(o,o->entries+i,profile,f);if(err)return err;
  o->entries[i].flags&=0xffff83ffu;
 }
 return GH_ID_OK;
}

gh_id_error gh_id_entry_reset_family_002ac9b8(gh_id_base*o,gh_id_entry*e,int16_t hide,gh_id_frontier*f){return family(o,e,hide,0,f);}
gh_id_error gh_id_entry_reset_family_reference(gh_id_base*o,gh_id_entry*e,int16_t hide,const struct gh_id_animation_profile*p,gh_id_frontier*f){return family(o,e,hide,p,f);}
