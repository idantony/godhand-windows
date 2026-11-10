#include "r020_title.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fenv.h>
#include "../core/id_layout_rotation.h"
struct gh_r020_title {gh_r020_title_bindings b;uint8_t fields[0xe0];gh_id_base base;
 gh_r207_allocation idd;gh_bundle_entry image;char path[128];uint32_t job,frontier,pool_end,jobs_end;
 gh_r020_title **owner_slot;uint8_t pair_bound,run_snapshot,initialized,exit_registered;
 uint8_t reference_memory,reference_math,menu_tables_ready;uint8_t menu_tables[160];
 uint64_t move_serial;uint32_t draw_jobs;uint8_t frontier_relative;};
static uint32_t u32(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static void w32(uint8_t*p,uint32_t n){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(n>>(8*i));}
static uint64_t u64(const uint8_t*p){return (uint64_t)u32(p)|((uint64_t)u32(p+4)<<32);}
static void w64(uint8_t*p,uint64_t n){w32(p,(uint32_t)n);w32(p+4,(uint32_t)(n>>32));}
static uint32_t random_next(void*p){uint32_t n=0;(void)gh_game_mt_next_0030f348(p,&n);return n;}
static gh_r020_status fail(gh_r020_title*s,uint32_t pc,gh_r020_status e){s->frontier=pc;s->frontier_relative=0;return e;}
static gh_r020_status fail_relative(gh_r020_title*s,uint32_t pc){fail(s,pc,GH_R020_UNSUPPORTED);s->frontier_relative=1;return GH_R020_UNSUPPORTED;}
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
 if(b->reference_profile){const gh_r020_reference_profile*p=b->reference_profile;
  if(p->memory>GH_R020_MEMORY_PROVIDED_BYTES||(p->memory==GH_R020_MEMORY_PROVIDED_BYTES&&(!p->bytes||p->size!=0xe0))||(p->vu_nearest_reference!=0&&p->vu_nearest_reference!=1))return GH_R020_ARGUMENT;
 }
 uint8_t*raw=(uint8_t*)b->work;
 /* Actual30E5C0 mode1. These are Work scalar displayconfiguration writes,
  * not a successful fake GPUbackground operation. Preserve64bitupperbits. */
 w32(raw+0x60c,0);w64(raw+0x3e0,(u64(raw+0x3e0)&UINT64_C(0xffffffffffff8000))|0x16);w64(raw+0x270,(u64(raw+0x270)&UINT64_C(0xffffffffffff8000))|0x16);
 gh_r020_title*s=calloc(1,sizeof(*s));if(!s)return GH_R020_MEMORY;s->b=*b;
 /* Unknownsourceclassfields stay diagnosticpoison; never read as state until
  * originalInitialize writes them. NativeIDBase/RoomJump are typedowners. */
 memset(s->fields,0xcd,sizeof(s->fields));
 if(b->reference_profile){const gh_r020_reference_profile*p=b->reference_profile;s->reference_math=(uint8_t)p->vu_nearest_reference;
  if(p->memory==GH_R020_MEMORY_ZERO_REFERENCE){memset(s->fields,0,sizeof(s->fields));s->reference_memory=1;}
  else if(p->memory==GH_R020_MEMORY_PROVIDED_BYTES){memcpy(s->fields,p->bytes,sizeof(s->fields));s->reference_memory=1;}
 }
 gh_id_base_clear_002a9df8(&s->base);s->owner_slot=out;*out=s;
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
 if(!s||!v)return GH_R020_ARGUMENT;*v=(gh_r020_title_view){s->fields,sizeof(s->fields),&s->base,s->image,s->path,s->frontier,s->initialized,s->exit_registered,s->move_serial,s->draw_jobs,s->frontier_relative};return GH_R020_OK;
}
static gh_r020_status fade_start(gh_r020_title*s,uint32_t from,uint32_t to,uint16_t frames){
 if(!s->b.fade)return fail(s,0x307208,GH_R020_UNSUPPORTED);
 if(!s->b.display||!s->b.display->initialized)return fail(s,0x3c1180,GH_R020_UNSUPPORTED);
 const uint8_t*raw=(const uint8_t*)s->b.work;
 gh_screen_fade_context c={s->b.display->screen_width,s->b.display->gs_y_origin,u32(raw+0x564),u32(raw+0x5e0)};
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
static float readf(const uint8_t*p){uint32_t n=u32(p);float f;memcpy(&f,&n,4);return f;}
static void writef(uint8_t*p,float f){uint32_t n;memcpy(&n,&f,4);w32(p,n);}
static gh_r020_status entry_at(gh_r020_title*s,int id,gh_id_entry**out){int fallback=0;if(gh_id_base_get_work_002aa410(s->b.runtime,&s->base,id,out,&fallback)||fallback)return fail(s,0x2aa410,GH_R020_ID);return GH_R020_OK;}
static gh_r020_status family(gh_r020_title*s,int id,int show){if(gh_id_base_set_family_display(s->b.runtime,&s->base,id,show))return fail(s,0x2aa410,GH_R020_ID);return GH_R020_OK;}
static gh_r020_status alpha(gh_r020_title*s,int id,unsigned n){gh_id_entry*e;gh_r020_status r=entry_at(s,id,&e);if(r)return r;e->color_base=(e->color_base&0xffffff)|((n&255)<<24);return GH_R020_OK;}
static gh_r020_status se(gh_r020_title*s,const gh_r020_move_callbacks*c,unsigned bank,unsigned program){if(!c||!c->program_se)return fail(s,0x2cb8a0,GH_R020_UNSUPPORTED);uint32_t f=0;gh_r020_status r=c->program_se(c->context,bank,program,&f);return r?fail(s,f?f:0x2cb8a0,r):GH_R020_OK;}
static void state(uint8_t*p,unsigned mode,unsigned phase,unsigned sub){p[0]=(uint8_t)mode;p[1]=(uint8_t)phase;p[2]=(uint8_t)sub;p[3]=0;}
static gh_r020_status move_wait(gh_r020_title*s){uint8_t*p=s->fields,*w=(uint8_t*)s->b.work;
 if(!p[1]){if(!(u32(w+0x584)&0x800)){state(p,1,0,0);w32(w+0x584,u32(w+0x584)|0x800);return GH_R020_OK;}w32(p+8,40);p[1]++;p[2]=p[3]=0;}
 else if(p[1]!=1)return GH_R020_OK;
 uint32_t n=u32(p+8)-1;w32(p+8,n);if(n&0x80000000)state(p,3,0,0);return GH_R020_OK;
}
static gh_r020_status sine_alpha(gh_r020_title*s,int id,unsigned offset,float scale){
 if(!s->reference_memory)return fail_relative(s,0xa18);if(!s->reference_math)return fail(s,0x1f7ef0,GH_R020_UNSUPPORTED);
 volatile float radians=readf(s->fields+offset)*0.01745329238474369f;float sn,cs;
 if(gh_id_sincos_reference(radians,&sn,&cs))return fail(s,0x1f7ef0,GH_R020_UNSUPPORTED);
 volatile float value=sn*scale;return alpha(s,id,(unsigned)(int)value);
}
static gh_r020_status title_id(gh_r020_title*s,const gh_r020_move_callbacks*c){uint8_t*p=s->fields;gh_r020_status r;uint32_t bits=u32(p+4);
 if(bits&1){float f=readf(p+0x28)+3.5f;writef(p+0x28,f>160?20:f);r=sine_alpha(s,5,0x28,96);}
 else if(bits&2)r=sine_alpha(s,4,0x28,192);else r=alpha(s,4,255);if(r)return r;
 static const int ids[]={8,3,7,0x21,0x24,0x27,0xc,0x31,0x34,0x37};
 static const unsigned offsets[]={0x28,0x28,0x28,0x30,0x34,0x38,0x38,0x40,0x44,0x48};
 for(unsigned i=0;i<10;i++){r=sine_alpha(s,ids[i],offsets[i],i==0?96:i==1?192:i==2?160:128);if(r)return r;}
 if(p[1]!=4){float f=readf(p+0x2c)+2;writef(p+0x2c,f>180?0:f);}r=sine_alpha(s,6,0x2c,255);if(r)return r;return draw_title(s,c);
}
static gh_r020_status title_initialize(gh_r020_title*s){uint8_t*p=s->fields;gh_r020_status r;
 if(!p[2]){w32(p+8,0);w32(p+0x28,0);w32(p+0x2c,0);if((r=family(s,0,1))||(r=family(s,1,1)))return r;
  const int hide[]={3,4,7,8};for(unsigned i=0;i<4;i++)if((r=logo_entry(s,hide[i],0,-1)))return r;
  if((r=alpha(s,5,255))||(r=alpha(s,6,255))||(r=logo_entry(s,10,(u32((uint8_t*)s->b.work+0x594)&0x800)!=0,-1)))return r;
  if(!s->b.bgm)return fail(s,0x2d1610,GH_R020_UNSUPPORTED);gh_bgm_status b=gh_bgm_event_start_002d1610(s->b.bgm,0,0,0);if(b)return fail(s,gh_bgm_requests_frontier(s->b.bgm),GH_R020_UNSUPPORTED);
  if((r=fade_start(s,0x80808080,0,15)))return r;state(p,p[0],p[1]+1,0);w32(p+4,0);
 }else if(p[2]!=1)return GH_R020_OK;
 if(fade_done(s)){p[2]=1;p[3]=0;}return GH_R020_OK;
}
static gh_r020_status title_press_wait(gh_r020_title*s,const gh_r020_move_callbacks*c){uint8_t*p=s->fields;const uint8_t*w=(const uint8_t*)s->b.work;gh_r020_status r;gh_id_entry*e;
 switch(p[2]){
 case 0:{if((r=family(s,0,1))||(r=family(s,1,1))||(r=family(s,2,0)))return r;const int hide[]={3,4,7,8};for(unsigned i=0;i<4;i++)if((r=logo_entry(s,hide[i],0,-1)))return r;
  if((r=logo_entry(s,10,(u32(w+0x594)&0x800)!=0,-1)))return r;const int kinds[]={10,8,6,7};for(unsigned i=0;i<4;i++){if((r=entry_at(s,kinds[i],&e)))return r;e->layer=0;}w32(p+4,u32(p+4)|1);p[2]++;p[3]=0;}
 /*fallthrough*/
 case 1:if(u64(w+0x178))w32(p+8,0);if(u64(w+0x1a0)&UINT64_C(0x8010000000)){p[2]++;p[3]=0;}break;
 case 2:w32(p+8,0);writef(p+0x28,140);if((r=entry_at(s,3,&e)))return r;e->pos_y=150;
  if((r=logo_entry(s,4,1,-1))||(r=logo_entry(s,5,1,-1))||(r=logo_entry(s,3,0,-1))||(r=sine_alpha(s,3,0x28,192)))return r;
  w32(p+4,(u32(p+4)&~1u)|2);if((r=se(s,c,0,0x160)))return r;p[2]++;p[3]=0;
 /*fallthrough*/
 case 3:{uint32_t n=u32(p+8)+1;w32(p+8,n);if((int32_t)n>5){writef(p+0x28,140);if((r=logo_entry(s,3,1,-1))||(r=logo_entry(s,4,0,-1))||(r=logo_entry(s,5,0,-1)))return r;p[2]++;p[3]=0;}}break;
 case 4:if((r=entry_at(s,3,&e)))return r;e->pos_y-=20;{float f=readf(p+0x28)+15;writef(p+0x28,f);if(f>=180){w32(p+0x28,0);w32(p+4,u32(p+4)&~2u);if((r=family(s,1,0)))return r;state(p,p[0],2,0);}}break;
 default:break;
 }return GH_R020_OK;
}

static gh_r020_status menu_layout(gh_r020_title*s,unsigned selected,int level){
 if(selected>2)return fail_relative(s,level?0x36e0:0x3540);
 if(!s->menu_tables_ready){size_t i;gh_disc_entry e;uint8_t header[4];
  if(gh_disc_find(s->b.dvd->disc,"dll/debugoptimize/r020.rel",&i)||gh_disc_entry_at(s->b.dvd->disc,i,&e)||e.size!=24129||gh_disc_read(s->b.dvd->disc,i,0,header,4)||memcmp(header,"SNR2",4)||gh_disc_read(s->b.dvd->disc,i,0x3c70,s->menu_tables,sizeof(s->menu_tables)))return fail(s,0x201160,GH_R020_DVD);s->menu_tables_ready=1;
 }
 unsigned base=level?80:0;gh_r020_status r;gh_id_entry*e;
 for(unsigned row=0;row<3;row++){
  int32_t x;uint32_t n=u32(s->menu_tables+base+4*(row*3+selected));memcpy(&x,&n,4);float scale=readf(s->menu_tables+base+40+4*(row*3+selected));writef(s->fields+(level?0x30:0x40)+4*row,15);
  for(unsigned k=0;k<(level?3u:2u);k++){int id=level?(int)u32(s->fields+0x50+12*row+4*k):0x31+(int)row*3+(k?2:0);if((r=entry_at(s,id,&e)))return r;e->pos_x=(float)x;e->scale_x=scale;}
 }return GH_R020_OK;
}
static gh_r020_status game_select(gh_r020_title*s,const gh_r020_move_callbacks*c){uint8_t*p=s->fields;const uint8_t*w=(uint8_t*)s->b.work;gh_r020_status r;gh_id_entry*e;
 unsigned selected=u32(p+0x14),previous;uint32_t n;
 switch(p[2]){
 case 0:if((r=menu_layout(s,selected,0))||(r=family(s,0x30,1)))return r;
  for(unsigned i=0;i<3;i++){w32(p+0x40+4*i,0);if((r=logo_entry(s,0x32+3*(int)i,0,-1))||(r=logo_entry(s,0x31+3*(int)i,1,-1))||(r=logo_entry(s,0x33+3*(int)i,0,-1)))return r;}
  p[2]++;p[3]=0;w32(p+8,0);
 /*fallthrough*/
 case 1:w32(p+0x18,selected);
  for(unsigned i=0;i<3;i++){float v=readf(p+0x40+4*i)+(i==selected?3:1);if(i==selected){if(v>160)v=15;}else if(v>15)v=15;writef(p+0x40+4*i,v);}
  if(u64(w+0x1b0)&0x8000000){selected=selected?selected-1:2;w32(p+0x14,selected);if((r=se(s,c,0,0x161)))return r;}
  else if(u64(w+0x1b0)&0x4000000){selected=selected<2?selected+1:0;w32(p+0x14,selected);if((r=se(s,c,0,0x161)))return r;}
  else {n=u32(p+8)+1;w32(p+8,n);if((int32_t)n>3&&(u64(w+0x1a0)&UINT64_C(0x8010000000))){p[2]=selected==0?10:selected==1?20:30;p[3]=0;return se(s,c,0,0x160);}
   if(u64(w+0x1a0)&0x20000000){if((r=family(s,0x30,0)))return r;state(p,p[0],1,0);return se(s,c,0,0x15e);}}
  previous=u32(p+0x18);if(previous==selected)return GH_R020_OK;
  for(unsigned i=0;i<3;i++){if((r=logo_entry(s,0x32+3*(int)i,0,-1))||(r=logo_entry(s,0x31+3*(int)i,0,-1))||(r=logo_entry(s,0x33+3*(int)i,1,-1))||(r=alpha(s,0x33+3*(int)i,i==previous?0x50:0x30))||(r=entry_at(s,0x33+3*(int)i,&e)))return r;e->scale_x=i==previous?1:.72f;}
  {gh_id_entry*a,*b;if((r=entry_at(s,0x31+3*(int)selected,&a))||(r=entry_at(s,0x31+3*(int)previous,&b)))return r;writef(p+0x3c,a->pos_x<b->pos_x?40:-40);}p[2]++;break;
 case 2:previous=u32(p+0x18);for(unsigned i=0;i<3;i++)if(i!=previous){if((r=entry_at(s,0x33+3*(int)i,&e)))return r;e->pos_x+=readf(p+0x3c);}
  if((r=entry_at(s,0x33+3*(int)previous,&e)))return r;e->scale_x+=.15f;if(e->scale_x<=1.2f)return GH_R020_OK;
  if((r=menu_layout(s,selected,0)))return r;for(unsigned i=0;i<3;i++){if((r=logo_entry(s,0x31+3*(int)i,i!=selected,-1))||(r=logo_entry(s,0x33+3*(int)i,i==selected,-1)))return r;if(i==selected){if((r=entry_at(s,0x33+3*(int)i,&e)))return r;e->scale_x=1.2f;}}p[2]++;break;
 case 3:if((r=entry_at(s,0x33+3*(int)selected,&e)))return r;e->scale_x-=.15f;if(e->scale_x>=1)return GH_R020_OK;
  if((r=logo_entry(s,0x31+3*(int)selected,1,-1))||(r=logo_entry(s,0x33+3*(int)selected,0,-1)))return r;for(unsigned i=0;i<3;i++){if((r=entry_at(s,0x33+3*(int)i,&e)))return r;e->scale_x=1;}p[2]=1;break;
 case 10:{unsigned done=0;for(unsigned i=0;i<3;i++){float f=readf(p+0x40+4*i);if(f>90){f+=6;if(f>=180){f=180;done++;}}else{f-=6;if(f<=0){f=0;done++;}}writef(p+0x40+4*i,f);}if(done==3){p[2]++;p[3]=0;}w32(p+8,0);}break;
 case 11:n=u32(p+8)+1;w32(p+8,n);if((int32_t)n<3)return GH_R020_OK;if((r=family(s,0x30,0)))return r;for(unsigned i=0;i<3;i++)for(unsigned k=0;k<3;k++)if((r=logo_entry(s,0x31+3*(int)i+(int)k,0,-1)))return r;state(p,p[0],3,0);break;

 case 20:if((r=se(s,c,2,15))||(r=fade_start(s,0,0xff000000,30)))return r;p[2]++;p[3]=0;
 /*fallthrough*/
 case 21:if(s->b.fade->state&&!(s->b.fade->state&4))return GH_R020_OK;if((r=family(s,0,0))||(r=family(s,0x30,0))||(r=family(s,0x30,0)))return r;p[2]++;p[3]=0;
 /*fallthrough*/
 case 22:{if(!c||!c->open_load)return fail(s,0x2bf268,GH_R020_UNSUPPORTED);uint32_t f=0;r=c->open_load(c->context,&f);if(r)return fail(s,f?f:0x2bf268,r);p[2]++;p[3]=0;}
 /*fallthrough*/
 case 23:if(u32(w+0x5e8)&0x40000000)return GH_R020_OK;if(u32(w+0x584)&0x40){state(p,5,0,0);w32((uint8_t*)w+0x10,0);return GH_R020_OK;}
  if((r=family(s,0,1))||(r=family(s,0x30,1))||(r=family(s,0x30,1))||(r=logo_entry(s,10,0,-1)))return r;for(unsigned i=0;i<3;i++){if((r=logo_entry(s,0x32+3*(int)i,0,-1))||(r=logo_entry(s,0x33+3*(int)i,0,-1)))return r;}if((r=fade_start(s,0xff000000,0,20)))return r;p[2]++;p[3]=0;
 /*fallthrough*/
 case 24:if(s->b.fade->state&&!(s->b.fade->state&4))return GH_R020_OK;p[2]=1;p[3]=0;break;
 case 30:if((r=fade_start(s,0,0xff000000,15)))return r;p[2]++;p[3]=0;
 /*fallthrough*/
 case 31:if(s->b.fade->state&&!(s->b.fade->state&4))return GH_R020_OK;w32((uint8_t*)w+0x10,0);if(!c||!c->open_options)return fail(s,0x160020,GH_R020_UNSUPPORTED);
  {uint32_t f=0;r=c->open_options(c->context,&f);if(r)return fail(s,f?f:0x160020,r);}p[2]++;p[3]=0;
 /*fallthrough*/
 case 32:if(u32(w+0x5e4)&0x8000000)return GH_R020_OK;if(s->b.fade->state&&!(s->b.fade->state&4))return GH_R020_OK;w32((uint8_t*)w+0x10,1);p[2]=1;p[3]=0;break;
 default:break;

 }return GH_R020_OK;
}

static int level_id(const gh_r020_title*s,unsigned row,unsigned part){return (int)u32(s->fields+0x50+12*row+4*part);}
static gh_r020_status level_select(gh_r020_title*s,const gh_r020_move_callbacks*c){uint8_t*p=s->fields;const uint8_t*w=(uint8_t*)s->b.work;unsigned selected=u32(p+0x1c),previous;gh_r020_status r;gh_id_entry*e;uint32_t n;
 if(selected>2)return fail_relative(s,0x2060);
 switch(p[2]){
 case 0:if((r=menu_layout(s,selected,1))||(r=family(s,2,1)))return r;for(unsigned i=0;i<3;i++){w32(p+0x30+4*i,0);for(unsigned k=0;k<3;k++)if((r=logo_entry(s,level_id(s,i,k),k!=2,-1)))return r;}p[2]++;p[3]=0;w32(p+8,0);
 /*fallthrough*/
 case 1:w32(p+0x20,selected);for(unsigned i=0;i<3;i++){int active=i==selected&&((u32(w+0x588)&0x200000)||selected!=2);float v=readf(p+0x30+4*i)+(active?3.5f:1);if(active){if(v>160)v=15;}else if(v>15)v=15;writef(p+0x30+4*i,v);}
  if(u64(w+0x1b0)&0x8000000){selected=selected?selected-1:2;w32(p+0x1c,selected);if((r=se(s,c,0,0x161)))return r;}
  else if(u64(w+0x1b0)&0x4000000){selected=selected<2?selected+1:0;w32(p+0x1c,selected);if((r=se(s,c,0,0x161)))return r;}
  else {n=u32(p+8)+1;w32(p+8,n);if((int32_t)n>=4&&(u64(w+0x1a0)&UINT64_C(0x8010000000))){if(!(u32(w+0x588)&0x200000)&&selected==2){if((r=se(s,c,0,0x15e)))return r;goto hide_locked;}writef(p+0x30+4*selected,90);p[2]=5;p[3]=0;goto hide_locked;}
   if(u64(w+0x1a0)&0x20000000){if((r=family(s,2,0)))return r;state(p,p[0],2,0);if((r=se(s,c,0,0x15e)))return r;goto hide_locked;}}
  previous=u32(p+0x20);if(previous==selected)break;
  for(unsigned i=0;i<3;i++){for(unsigned k=0;k<3;k++)if((r=logo_entry(s,level_id(s,i,k),k==2,-1)))return r;if((r=alpha(s,level_id(s,i,2),i==previous?0x50:0x30))||(r=entry_at(s,level_id(s,i,2),&e)))return r;e->scale_x=i==previous?1:.72f;}
  {gh_id_entry*a,*b;if((r=entry_at(s,level_id(s,selected,0),&a))||(r=entry_at(s,level_id(s,previous,0),&b)))return r;writef(p+0x3c,a->pos_x<b->pos_x?40:-40);}p[2]++;p[3]=0;break;
 case 2:previous=u32(p+0x20);for(unsigned i=0;i<3;i++)if(i!=previous){if((r=entry_at(s,level_id(s,i,2),&e)))return r;e->pos_x+=readf(p+0x3c);}if((r=entry_at(s,level_id(s,previous,2),&e)))return r;e->scale_x+=.15f;if(e->scale_x<=1.2f)break;
  if((r=menu_layout(s,selected,1)))return r;for(unsigned i=0;i<3;i++)for(unsigned k=0;k<3;k++){if((r=logo_entry(s,level_id(s,i,k),i==selected?k==2:k!=2,-1)))return r;if(i==selected&&k==2){if((r=entry_at(s,level_id(s,i,k),&e)))return r;e->scale_x=1.2f;}}p[2]++;p[3]=0;break;
 case 3:if((r=entry_at(s,level_id(s,selected,2),&e)))return r;e->scale_x-=.15f;if(e->scale_x<1){for(unsigned k=0;k<3;k++)if((r=logo_entry(s,level_id(s,selected,k),k!=2,-1)))return r;for(unsigned i=0;i<3;i++){if((r=entry_at(s,level_id(s,i,2),&e)))return r;e->scale_x=1;}p[2]=1;p[3]=0;}break;
 case 5:n=u32(p+8)+1;w32(p+8,n);if((int32_t)n>5)state(p,p[0],4,0);break;
 default:break;
 }
 hide_locked:{int first=(u32(w+0x588)&0x200000)?0xb:0x26;for(int i=0;i<3;i++)if((r=logo_entry(s,first+i,0,-1)))return r;}return GH_R020_OK;
}

static gh_r020_status bgm_fade(gh_r020_title*s,float frames){if(!s->b.bgm)return fail(s,0x2d1768,GH_R020_UNSUPPORTED);gh_bgm_status r=gh_bgm_event_fade_002d1768(s->b.bgm,0,frames,0,-1);return r?fail(s,gh_bgm_requests_frontier(s->b.bgm),GH_R020_UNSUPPORTED):GH_R020_OK;}
static gh_r020_status title_exit(gh_r020_title*s,const gh_r020_move_callbacks*c){uint8_t*p=s->fields,*w=(uint8_t*)s->b.work;gh_r020_status r;gh_id_entry*e;
 switch(p[2]){
 case 0:w32(p+0x28,0);writef(p+0x2c,90);if((r=logo_entry(s,3,0,-1))||(r=logo_entry(s,8,1,-1))||(r=alpha(s,8,0)))return r;
  {const int kinds[]={8,6,7,10};for(unsigned i=0;i<4;i++){if((r=entry_at(s,kinds[i],&e)))return r;e->layer=i==3?(u32(w+0x594)&0x800)!=0:1;}}
  if((r=se(s,c,2,0))||(r=bgm_fade(s,60)))return r;p[2]++;p[3]=0;
 /*fallthrough*/
 case 1:{float v=readf(p+0x28)+15;writef(p+0x28,v);if(v>=180){p[0x25]=0;w32(p+0x28,0);if((r=logo_entry(s,8,0,-1))||(r=logo_entry(s,7,1,-1))||(r=alpha(s,7,0))||(r=se(s,c,2,(u32(p+0x1c)+10)&65535)))return r;p[2]++;p[3]=0;}}break;
 case 2:{float v=readf(p+0x28)+.5f;writef(p+0x28,v);if(v>=40&&!p[0x25]){if((r=fade_start(s,0,0xff000000,37)))return r;p[0x25]=1;}if(v>=70){writef(p+0x28,70);p[2]++;p[3]=0;}}break;
 case 3:if(fade_done(s)){w32(w+0x584,u32(w+0x584)|0x200);for(int i=0;i<3;i++)if((r=family(s,i,0)))return r;if((r=logo_entry(s,0x10,0,-1))||(r=logo_entry(s,0x11,0,-1)))return r;state(p,4,0,0);}break;
 default:break;
 }return GH_R020_OK;
}
static gh_r020_status title_loop(gh_r020_title*s){uint8_t*p=s->fields;gh_r020_status r;
 if(!p[2]){if((r=fade_start(s,0,0xff000000,15))||(r=bgm_fade(s,15)))return r;p[2]++;p[3]=0;}else if(p[2]!=1)return GH_R020_OK;
 if(fade_done(s)){if((r=family(s,0,0))||(r=family(s,1,0)))return r;state(p,(u32((uint8_t*)s->b.work+0x594)&0x800)&&u32(p+0xc)==2?1:2,0,0);}return GH_R020_OK;
}
static gh_r020_status move_jump(gh_r020_title*s,const gh_r020_move_callbacks*c){uint8_t*w=(uint8_t*)s->b.work,*p=s->fields;
 while(!(u32(w+0x584)&0x400)){if(!s->b.threads||gh_task_threads_sleep(s->b.threads,1)!=GH_THREAD_OK)return fail(s,0x2d5aa0,GH_R020_UNSUPPORTED);}
 uint32_t difficulty=u32(p+0x1c);if(difficulty<3){s->b.save.bytes[0x1f]=(uint8_t)difficulty;if(gh_core_save_game_init(s->b.save,s->b.work))return fail(s,0x1f9b10,GH_R020_SAVE);}
 uint16_t room=(uint16_t)((unsigned)p[0xd8]|(unsigned)p[0xd9]<<8);unsigned spawn=p[0xd4];gh_id_view table=s->b.room_jump;
 if(!table.bytes||table.size<16)return fail(s,0x2beaa8,GH_R020_ID);uint32_t count=u32(table.bytes);if(count>(table.size-16)/4)return fail(s,0x2beaa8,GH_R020_ID);
 float vector[4]={0,0,0,0};for(uint32_t i=0;i<count;i++){const uint8_t*row=table.bytes+16+4*i;if(((unsigned)row[0]|(unsigned)row[1]<<8)!=room)continue;
  if(spawn<row[2]){uint64_t off=(uint64_t)u32(table.bytes+8)+(row[3]+spawn)*48u;if(off>table.size||48>table.size-off)return fail(s,0x2beb00,GH_R020_ID);for(unsigned k=0;k<4;k++)vector[k]=readf(table.bytes+(size_t)off+4*k);
   if(vector[3]<-3.1415927410125732f||vector[3]>3.1415927410125732f){if(!s->reference_math)return fail(s,0x2be998,GH_R020_UNSUPPORTED);fenv_t saved;if(fegetenv(&saved)||fesetround(FE_TONEAREST))return fail(s,0x2be998,GH_R020_UNSUPPORTED);volatile float yaw=vector[3]*0.01745329238474369f;vector[3]=yaw;if(fesetenv(&saved))return fail(s,0x2be998,GH_R020_UNSUPPORTED);}
  }break;
 }
 for(unsigned k=0;k<3;k++)writef(w+0x5c0+4*k,vector[k]);writef(w+0x5d0,vector[3]);w[0x5b4]=(uint8_t)room;w[0x5b5]=(uint8_t)(room>>8);w[0x5ba]=w[0x5bb]=0;w[0x5a8]=p[0xd4];
 if(!c||!c->begin_room_jump)return fail(s,0x2c4548,GH_R020_UNSUPPORTED);
 gh_r020_room_jump_request request={room,{vector[0],vector[1],vector[2]},vector[3],1,1,0};uint32_t frontier=0;gh_r020_status r=c->begin_room_jump(c->context,&request,&frontier);if(r)return fail(s,frontier?frontier:0x2c4548,r);state(p,5,0,0);return GH_R020_OK;
}
static gh_r020_status move_title(gh_r020_title*s,const gh_r020_move_callbacks*c){
 fenv_t saved;if(fegetenv(&saved)||fesetround(FE_TONEAREST))return fail(s,0x1f7ef0,GH_R020_UNSUPPORTED);
 gh_r020_status r;switch(s->fields[1]){case 0:r=title_initialize(s);break;case 1:r=title_press_wait(s,c);break;case 2:r=game_select(s,c);break;case 3:r=level_select(s,c);break;case 4:r=title_exit(s,c);break;case 5:r=title_loop(s);break;default:r=GH_R020_OK;break;}
 if(!r)r=title_id(s,c);if(fesetenv(&saved))return fail(s,0x1f7ef0,GH_R020_UNSUPPORTED);return r;
}

gh_r020_status gh_r020_title_move(gh_r020_title*s,const gh_r020_move_callbacks*c){
 if(!s||!s->initialized||!s->exit_registered)return GH_R020_ARGUMENT;
 s->frontier=0;s->frontier_relative=0;s->move_serial++;s->draw_jobs=0;
 if(c&&c->begin_frame){uint32_t frontier=0;gh_r020_status e=c->begin_frame(c->context,&s->base,&frontier);if(e)return fail(s,frontier?frontier:0x2aac28,e);}
 uint8_t*raw=(uint8_t*)s->b.work;w32(raw+0x5d8,u32(raw+0x5d8)|0xf00c6000);w32(raw+0x5e0,u32(raw+0x5e0)|0xff800000);
 gh_r020_status e=GH_R020_OK;
 switch(s->fields[0]){case 0:e=move_wait(s);break;case 1:e=move_logo(s,c);break;case 2:break; /*actual3490 jrra/nop*/
 case 3:e=move_title(s,c);break;case 4:e=move_jump(s,c);break;default:break;}
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
