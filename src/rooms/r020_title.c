#include "r020_title.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
struct gh_r020_title {gh_r020_title_bindings b;uint8_t fields[0xe0];gh_id_base base;
 gh_r207_allocation idd;gh_bundle_entry image;char path[128];uint32_t job,frontier,pool_end,jobs_end;
 gh_r020_title **owner_slot;uint8_t pair_bound,run_snapshot,initialized,exit_registered;
 uint64_t move_serial;uint32_t draw_jobs;};
static uint32_t u32(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static void w32(uint8_t*p,uint32_t n){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(n>>(8*i));}
static uint64_t u64(const uint8_t*p){return (uint64_t)u32(p)|((uint64_t)u32(p+4)<<32);}
static void w64(uint8_t*p,uint64_t n){w32(p,(uint32_t)n);w32(p+4,(uint32_t)(n>>32));}
static uint32_t random_next(void*p){uint32_t n=0;(void)gh_game_mt_next_0030f348(p,&n);return n;}
static gh_r020_status fail(gh_r020_title*s,uint32_t pc,gh_r020_status e){s->frontier=pc;return e;}
static gh_scene_status native_end(void*p,uint32_t*frontier){
 gh_r020_title**slot=p;if(!slot||!*slot)return GH_SCENE_ARGUMENT;
 gh_r020_status e=gh_r020_title_destroy(*slot);if(e){if(frontier)*frontier=0x2aa078;return GH_SCENE_CALLEE;}
 *slot=NULL;if(frontier)*frontier=0;return GH_SCENE_OK;
}
static gh_r020_status load_id(gh_r020_title*s){
 static const char*const formats[]={"id/%s.idd","id/local/US/%s_US.idd","id/local/EN/%s_EN.idd","id/local/FR/%s_FR.idd","id/local/GE/%s_GE.idd","id/local/IT/%s_IT.idd","id/local/SP/%s_SP.idd"};
 size_t index=0;gh_disc_entry entry;int lang=s->b.work->language;gh_disc_error found=GH_DISC_NOT_FOUND;
 if((unsigned)lang<7){(void)snprintf(s->path,sizeof(s->path),formats[lang],"idtitlecore");found=gh_disc_find(s->b.dvd->disc,s->path,&index);if(found!=GH_DISC_OK&&found!=GH_DISC_NOT_FOUND)return fail(s,0x2acfa8,GH_R020_DVD);}
 if(found!=GH_DISC_OK){(void)snprintf(s->path,sizeof(s->path),formats[0],"idtitlecore");if(gh_disc_find(s->b.dvd->disc,s->path,&index))return fail(s,0x2acfa8,GH_R020_DVD);}
 if(gh_disc_entry_at(s->b.dvd->disc,index,&entry)||entry.size>SIZE_MAX)return fail(s,0x200cb0,GH_R020_DVD);
 s->job=gh_dvd_read_alloc(s->b.dvd,s->path,&s->idd,s->b.room_heap,0,0,0,0);if(!s->job)return fail(s,0x200cb0,GH_R020_DVD);
 /* ImportedCheckWait is201160, NOT201108: actualstep/poll loop has no
  * frameyield or mandatoryfinalsleep. It mayfinishotherrealqueuedjobs too. */
 while(gh_dvd_check(s->b.dvd,s->job))if(gh_dvd_pump(s->b.dvd)!=GH_DVD_OK)return fail(s,0x201160,GH_R020_DVD);
 if(s->b.dvd->failed_id==s->job)return fail(s,0x201160,GH_R020_DVD);
 s->image.bytes=s->idd.pointer;s->image.size=(size_t)entry.size;
 if(gh_id_set_data_002accf8(s->b.id,2,&s->image))return fail(s,0x2accf8,GH_R020_ID);s->pair_bound=1;
 int original;gh_id_error ie=gh_id_base_initialize_002a9ea0(s->b.runtime,&s->base,2,0,random_next,s->b.random,&original);
 s->pool_end=s->b.id->pool_used;s->jobs_end=s->b.id->job_count;s->run_snapshot=1;
 if(ie||original!=1)return fail(s,s->base.first_unported?s->base.first_unported:0x2a9ea0,ie==GH_ID_UNSUPPORTED?GH_R020_UNSUPPORTED:GH_R020_ID);
 static const int family[]={0,1,2,0x30};for(unsigned i=0;i<4;i++)if(gh_id_base_set_family_display(s->b.runtime,&s->base,family[i],0))return fail(s,0x2aa410,GH_R020_ID);
 for(int i=0x10;i<=0x11;i++){gh_id_entry*e;int fallback;if(gh_id_base_get_work_002aa410(s->b.runtime,&s->base,i,&e,&fallback)||fallback)return fail(s,0x2aa410,GH_R020_ID);e->flags|=0x20000000;}
 return GH_R020_OK;
}
gh_r020_status gh_r020_title_init_prefix(const gh_r020_title_bindings*b,gh_r020_title**out){
 if(!b||!out||!b->work||!b->save.bytes||b->save.size<GH_CORE_SAVE_BYTES||!b->random||!b->dvd||!b->dvd->disc||!b->room_heap||!b->id||!b->runtime||!b->room_jump.bytes||!b->room_jump.size)return GH_R020_ARGUMENT;
 if(*out||b->id->pair[2].bound)return GH_R020_BUSY;
 uint8_t*raw=(uint8_t*)b->work;
 /* Actual30E5C0 mode1. These are Work scalar displayconfiguration writes,
  * not a successful fake GPUbackground operation. Preserve64bitupperbits. */
 w32(raw+0x60c,0);w64(raw+0x3e0,(u64(raw+0x3e0)&UINT64_C(0xffffffffffff8000))|0x16);w64(raw+0x270,(u64(raw+0x270)&UINT64_C(0xffffffffffff8000))|0x16);
 gh_r020_title*s=calloc(1,sizeof(*s));if(!s)return GH_R020_MEMORY;s->b=*b;
 /* Unknownsourceclassfields stay diagnosticpoison; never read as state until
  * originalInitialize writes them. NativeIDBase/RoomJump are typedowners. */
 memset(s->fields,0xcd,sizeof(s->fields));gh_id_base_clear_002a9df8(&s->base);s->owner_slot=out;*out=s;
 w32(raw+0x5d8,u32(raw+0x5d8)|0xf00c6000);w32(raw+0x5e0,u32(raw+0x5e0)|0xff800000);w32(raw+0x5e4,u32(raw+0x5e4)|0x4000);w32(raw+0x10,1);
 gh_r020_status e=load_id(s);if(e)return e;
 if(gh_core_save_game_init(b->save,b->work)!=GH_INIT_OK)return fail(s,0x1f9b10,GH_R020_SAVE);
 uint32_t difficulty=gh_core_save_difficulty(b->save);w32(s->fields+0x1c,difficulty);w32(s->fields+0x20,difficulty);
 uint32_t selected=(u32(raw+0x584)&4)?1:0;w32(s->fields+0x14,selected);w32(s->fields+0x18,selected);
 for(unsigned i=0;i<6;i++)w32(s->fields+0x50+4*i,0x20+i);
 unsigned first=(u32(raw+0x588)&0x200000)?0x26:0xb;for(unsigned i=0;i<3;i++)w32(s->fields+0x68+4*i,first+i);
 if(!b->bgm)return fail(s,0x2d15a8,GH_R020_UNSUPPORTED);
 gh_bgm_status be=gh_bgm_event_set_002d15a8(b->bgm,0,0,0);
 if(be)return fail(s,gh_bgm_requests_frontier(b->bgm)?gh_bgm_requests_frontier(b->bgm):0x2d15a8,be==GH_BGM_MEMORY?GH_R020_MEMORY:GH_R020_UNSUPPORTED);
 w32(s->fields+0xc,2);s->fields[0xd8]=0;s->fields[0xd9]=1;w32(s->fields+0xd4,0);
 /* RoomJump_setTbl writes the samecanonicalactualtable nowkept inb.room_jump;
  * no raw32bithostpointer is installed into originalclassDC. */
 s->fields[3]=0;s->fields[0]=(u32(raw+0x584)&0x800)?0:1;s->fields[1]=s->fields[2]=0;
 if(s->fields[0]==1)w32(raw+0x584,u32(raw+0x584)|0x800);s->initialized=1;
 if(!b->scenario_exit)return fail(s,0x2c4dc0,GH_R020_UNSUPPORTED);
 *b->scenario_exit=(gh_scene_hook){GH_SCENE_HOOK_NATIVE,native_end,out};s->exit_registered=1;s->frontier=0;return GH_R020_OK;
}
gh_r020_status gh_r020_title_init(const gh_r020_title_bindings*b,gh_r020_title**out){return gh_r020_title_init_prefix(b,out);}
gh_r020_status gh_r020_title_inspect(const gh_r020_title*s,gh_r020_title_view*v){
 if(!s||!v)return GH_R020_ARGUMENT;*v=(gh_r020_title_view){s->fields,sizeof(s->fields),&s->base,s->image,s->path,s->frontier,s->initialized,s->exit_registered,s->move_serial,s->draw_jobs};return GH_R020_OK;
}
static gh_r020_status fade_start(gh_r020_title*s,uint32_t from,uint32_t to,uint16_t frames){
 if(!s->b.fade)return fail(s,0x307208,GH_R020_UNSUPPORTED);
 const uint8_t*raw=(const uint8_t*)s->b.work;
 gh_screen_fade_context c={512,1824,u32(raw+0x564),u32(raw+0x5e0)};
 if(gh_screen_fade_start_00307208(s->b.fade,&c,0,10,0,from,to,frames))return fail(s,0x307208,GH_R020_UNSUPPORTED);return GH_R020_OK;
}
static int fade_done(gh_r020_title*s){uint8_t n=gh_screen_fade_status_00307718(s->b.fade);return (n&1)&&(n&4);}
static gh_r020_status logo_entry(gh_r020_title*s,int id,int show,int b8b){
 gh_id_entry*e;int fallback;if(gh_id_base_get_work_002aa410(s->b.runtime,&s->base,id,&e,&fallback)||fallback)return fail(s,0x2aa410,GH_R020_ID);
 if(show>0)e->flags&=~0x20000000u;else if(!show)e->flags|=0x20000000;if(b8b>=0)e->b8b=(uint8_t)b8b;return GH_R020_OK;
}
static gh_r020_status draw_title(gh_r020_title*s,const gh_r020_move_callbacks*c){
 gh_id_frontier f={0};gh_id_error ie=gh_id_jobs_move(&s->base,NULL,NULL,&f);
 if(ie)return fail(s,f.address?f.address:0x2aaa40,ie==GH_ID_UNSUPPORTED?GH_R020_UNSUPPORTED:GH_R020_ID);
 gh_id_draw_job jobs[64];size_t count=0;ie=gh_id_jobs_collect_visible(&s->base,jobs,64,&count,&f);
 if(ie)return fail(s,f.address?f.address:0x2aac28,ie==GH_ID_UNSUPPORTED?GH_R020_UNSUPPORTED:GH_R020_ID);
 for(size_t i=0;i<count;i++){
  if(!c||!c->draw_entry)return fail(s,jobs[i].consumer_address,GH_R020_UNSUPPORTED);
  uint32_t frontier=0;gh_r020_status e=c->draw_entry(c->context,&s->base,&jobs[i],&frontier);
  if(e)return fail(s,frontier?frontier:jobs[i].consumer_address,e);s->draw_jobs++;
 }return GH_R020_OK;
}
static gh_r020_status move_logo(gh_r020_title*s,const gh_r020_move_callbacks*c){
 uint8_t*p=s->fields,phase=p[1];gh_r020_status e;uint64_t pressed=u64((const uint8_t*)s->b.work+0x1a0)&UINT64_C(0x8030000000);
 if(!s->b.fade)return fail(s,0x307718,GH_R020_UNSUPPORTED);
 switch(phase){
 case 0:
  e=fade_start(s,0xff000000,0xff000000,1);if(e)return e;p[1]++;p[2]=p[3]=0;
  /*actualfallthrough*/
 case 1:if(fade_done(s)){p[1]=10;p[2]=p[3]=0;}break;
 case 10:
  p[0x24]=0;e=fade_start(s,0xff000000,0,15);if(e)return e;e=logo_entry(s,0x11,1,-1);if(e)return e;p[1]++;p[2]=p[3]=0;break;
 case 11:
  if(pressed)p[0x24]=1;if(fade_done(s)){
   if(!c||!c->logo_se)return fail(s,0x2cb8a0,GH_R020_UNSUPPORTED);
   uint32_t frontier=0;e=c->logo_se(c->context,&frontier);if(e)return fail(s,frontier?frontier:0x2cb8a0,e);
   w32(p+8,0);p[1]++;p[2]=p[3]=0;
  }break;
 case 12:case 32:case 42:{
  uint32_t old=u32(p+8),n=old+1;w32(p+8,n);
  int32_t signed_n;memcpy(&signed_n,&n,4);
  if(signed_n<121){n=old+2;w32(p+8,n);memcpy(&signed_n,&n,4);if(signed_n<2)break;if(!pressed&&!p[0x24])break;}
  e=fade_start(s,0,phase==12?0xff000000:0x80808080,phase==12?25:15);if(e)return e;p[1]++;p[2]=p[3]=0;break;
 }
 case 13:if(fade_done(s)){e=logo_entry(s,0x11,0,-1);if(e)return e;p[1]=30;p[2]=p[3]=0;}break;
 case 30:
  p[0x24]=0;e=fade_start(s,0xff000000,0,15);if(e)return e;e=logo_entry(s,0x10,1,6);if(e)return e;p[1]++;p[2]=p[3]=0;break;
 case 31:case 41:
  if(pressed)p[0x24]=1;if(fade_done(s)){w32(p+8,0);p[1]++;p[2]=p[3]=0;}break;
 case 33:if(fade_done(s)){p[1]=40;p[2]=p[3]=0;}break;
 case 40:
  p[0x24]=0;e=fade_start(s,0x80808080,0,15);if(e)return e;e=logo_entry(s,0x10,-1,7);if(e)return e;p[1]++;p[2]=p[3]=0;break;
 case 43:if(fade_done(s)){e=logo_entry(s,0x10,0,-1);if(e)return e;p[0]=3;w32(p+0xc,1);p[1]=p[2]=p[3]=0;}break;
 default:break; /*actualunused/defaultphases stillMove/Trans*/
 }
 return p[1]==1?GH_R020_OK:draw_title(s,c);
}
gh_r020_status gh_r020_title_move(gh_r020_title*s,const gh_r020_move_callbacks*c){
 if(!s||!s->initialized||!s->exit_registered)return GH_R020_ARGUMENT;
 s->frontier=0;s->move_serial++;s->draw_jobs=0;
 if(c&&c->begin_frame){uint32_t frontier=0;gh_r020_status e=c->begin_frame(c->context,&s->base,&frontier);if(e)return fail(s,frontier?frontier:0x2aac28,e);}
 uint8_t*raw=(uint8_t*)s->b.work;w32(raw+0x5d8,u32(raw+0x5d8)|0xf00c6000);w32(raw+0x5e0,u32(raw+0x5e0)|0xff800000);
 gh_r020_status e=GH_R020_OK;
 switch(s->fields[0]){case 0:return fail(s,0x8c8,GH_R020_UNSUPPORTED);case 1:e=move_logo(s,c);break;case 2:break; /*actual3490 jrra/nop*/
 case 3:return fail(s,0x970,GH_R020_UNSUPPORTED);case 4:return fail(s,0x780,GH_R020_UNSUPPORTED);default:break;}
 if(e)return e;if(u32(raw+0x594)&0x800)return fail(s,0x201ae0,GH_R020_UNSUPPORTED);return GH_R020_OK;
}
gh_r020_status gh_r020_title_destroy(gh_r020_title*s){
 if(!s)return GH_R020_ARGUMENT;if(s->run_snapshot&&(s->b.id->pool_used!=s->pool_end||s->b.id->job_count!=s->jobs_end))return GH_R020_BUSY;
 uint8_t*raw=(uint8_t*)s->b.work;w32(raw+0x5e4,u32(raw+0x5e4)&~0x4000u);w32(raw+0x5e0,u32(raw+0x5e0)&0x027fffff);w32(raw+0x10,0);
 if(s->base.entries){int original,freed;if(gh_id_base_release_002aa078(s->b.runtime,&s->base,&original,&freed)!=GH_ID_OK)return GH_R020_ID;}s->run_snapshot=0;
 if(s->pair_bound){memset(&s->b.id->pair[2],0,sizeof(s->b.id->pair[2]));s->pair_bound=0;}
 if(s->job&&gh_dvd_check(s->b.dvd,s->job))gh_dvd_cancel(s->b.dvd,s->job);
 if(s->idd.pointer){if(gh_heap_release(s->b.room_heap,s->idd)!=GH_MEM_OK)return GH_R020_MEMORY;s->idd=(gh_r207_allocation){0};}
 if(s->exit_registered&&s->b.scenario_exit&&s->b.scenario_exit->context==s->owner_slot)*s->b.scenario_exit=(gh_scene_hook){.kind=GH_SCENE_HOOK_ABSENT};
 free(s);return GH_R020_OK;
}
