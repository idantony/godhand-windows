#include "sound_scene.h"
#include <stdio.h>
#include <string.h>
static gh_sound_scene_status fail(gh_sound_scene*s,uint32_t f,int d,gh_sound_scene_status e){s->frontier=f;s->detail=d;return e;}
gh_sound_scene_status gh_sound_scene_from_startup(gh_sound_scene*s,gh_sound_control*c,gh_audio_registry*r){
 if(!s||!c||!r||!c->initialized)return GH_SOUND_SCENE_ARGUMENT;
 if(s->startup_bound)return GH_SOUND_SCENE_BUSY;
 if(!gh_audio_startup_ready(r,0,0,0))return GH_SOUND_SCENE_ASSET;
 *s=(gh_sound_scene){0};s->control=c;s->registry=r;s->sound_group130=0;s->startup_bound=1;
 return GH_SOUND_SCENE_OK;
}
gh_sound_scene_status gh_sound_scene_release_loading(gh_sound_scene*s,gh_dvd_jobs*d,gh_heap*h){
 if(!s||!d||!h)return GH_SOUND_SCENE_ARGUMENT;
 if(s->job&&gh_dvd_check(d,s->job))return GH_SOUND_SCENE_BUSY;
 if(s->loading.pointer&&gh_heap_release(h,s->loading)!=GH_MEM_OK)return GH_SOUND_SCENE_ASSET;
 s->loading=(gh_r207_allocation){0};s->job=0;s->file_bytes=0;return GH_SOUND_SCENE_OK;
}
gh_sound_scene_status gh_sound_scene_initialize_002cabe8(gh_sound_scene*s,
 const gh_bundle*b,gh_boot_work*w,gh_dvd_jobs*d,gh_heap*h,gh_task_threads*t){
 gh_bundle_entry e;gh_bundle_error be;gh_disc_entry de;size_t index;char path[32];
 gh_sound_span programs,sys,samples,afd;gh_dvd_error dr;gh_audio_status ar;
 if(!s||!s->startup_bound||!b||!w||!d||!d->disc||!h||!t)return GH_SOUND_SCENE_ARGUMENT;
 if(s->ready||s->loading.pointer)return GH_SOUND_SCENE_BUSY;s->frontier=0;s->detail=0;
 /* Original stores BMH before SEH. Keep each verified present view even if the
  * next consumer is not recovered. Never treat malformed lookup as absent. */
 be=gh_bundle_find(b,"BMH",0,&e);if(be==GH_BUNDLE_OK)s->bmh=e;else if(be!=GH_BUNDLE_NOT_FOUND)return fail(s,0x31ccf0,be,GH_SOUND_SCENE_ASSET);
 be=gh_bundle_find(b,"SEH",0,&e);if(be==GH_BUNDLE_OK)s->seh=e;else if(be!=GH_BUNDLE_NOT_FOUND)return fail(s,0x31ccf0,be,GH_SOUND_SCENE_ASSET);
 if(s->seh.bytes||s->bmh.bytes)return fail(s,s->seh.bytes?0x2ca390:0x2d2cf0,0,GH_SOUND_SCENE_UNSUPPORTED);
 if(s->sound_group130!=0)return fail(s,0x2ca390,0,GH_SOUND_SCENE_UNSUPPORTED);
 /* Firstboot only: replacing an already occupied bank requires its original
  * release/update path; preserve registry generation rather than guessing. */
 ar=gh_audio_bank_views(s->registry,2,&programs,&sys,&samples,&afd);
 if(ar!=GH_AUDIO_NOT_FOUND)return fail(s,0x2cf600,ar,GH_SOUND_SCENE_UNSUPPORTED);
 s->sample_budget=0xc8000;
 (void)snprintf(path,sizeof(path),"se/r%03x.ses",(unsigned)(uint16_t)w->room&0xfffu);
 if(gh_disc_find(d->disc,path,&index)!=GH_DISC_OK){
  memcpy(path,"se/r100.ses",11);
  if(gh_disc_find(d->disc,path,&index)!=GH_DISC_OK)return fail(s,0x2cb6a0,GH_AUDIO_NOT_FOUND,GH_SOUND_SCENE_ASSET);
 }
 if(gh_disc_entry_at(d->disc,index,&de)!=GH_DISC_OK||de.size>SIZE_MAX)return fail(s,0x2cf600,0,GH_SOUND_SCENE_ASSET);
 s->file_bytes=(size_t)de.size;
 s->job=gh_dvd_read_alloc(d,path,&s->loading,h,0,0,0,0);
 if(!s->job)return fail(s,0x200cb0,d->last_error,GH_SOUND_SCENE_ASSET);
 dr=gh_dvd_check_wait_frame(d,s->job,t);if(dr)return fail(s,0x2cb778,dr,GH_SOUND_SCENE_ASSET);
 {gh_sound_bank bank;
  if(gh_sound_bank_open(s->loading.pointer,s->file_bytes,&bank)!=GH_SOUND_OK)return fail(s,0x2cf600,0,GH_SOUND_SCENE_ASSET);
  if(bank.external_afs)return fail(s,0x2cf258,0,GH_SOUND_SCENE_UNSUPPORTED);
  if(bank.sections[3].size>s->sample_budget)return fail(s,0x2cd7c8,0,GH_SOUND_SCENE_ASSET);
 }
 ar=gh_audio_commit_se(s->registry,2,s->loading.pointer,s->file_bytes,&s->bank2);
 if(ar)return fail(s,0x374b70,ar,GH_SOUND_SCENE_ASSET);
 if(!gh_audio_token_complete(s->registry,s->bank2))return fail(s,0x2cb778,0,GH_SOUND_SCENE_ASSET);
 be=gh_bundle_find(b,"TS",0,&e);if(be==GH_BUNDLE_OK){s->ts=e;return fail(s,0x2cb080,0,GH_SOUND_SCENE_UNSUPPORTED);}
 if(be!=GH_BUNDLE_NOT_FOUND)return fail(s,0x31ccf0,be,GH_SOUND_SCENE_ASSET);
 /* Empty2CB080 still clears Work+5E4 bit800 and native list is truly empty. */
 {uint32_t v;memcpy(&v,(uint8_t*)w+0x5e4,4);v&=~0x800u;memcpy((uint8_t*)w+0x5e4,&v,4);}
 s->fade_ratio_bits=0x3f800000;s->default_ratio_bits=0x3c9374bc;
 for(unsigned i=0;i<5;i++){s->ratios60[i]=s->fade_ratio_bits;s->ratios74[i]=s->fade_ratio_bits;s->ratios88[i]=s->default_ratio_bits;}
 s->current_weight120=-1;s->current_request11c=-2;s->current_bank118=-1;s->ready=1;
 return gh_sound_scene_release_loading(s,d,h);
}
