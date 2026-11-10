/* Actual patched SLUS_215.03, original140C88 and concrete widget initializers.
 * Host handles replace every guest pointer. Finite UI source reconstruction,
 * not a GS interpreter or a claim of Cockpit Move/Trans implementation. */
#include "game_cockpit.h"
#include "id_entry_reset.h"
#include "id_animation.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fenv.h>
static const uint32_t offsets[]={0,0xd0,0x3a0,0x460,0x540,0x5f0,0x670,0x6e0,0xb90,0xbe0,0xc30,
 0xcc0,0xd60,0xdb0,0xe00,0xe50,0xea0,0xfa0,0xff0,0x1040,0x1090,0x10e0,0x12d0,0x1320,
 0x1380,0x1470,0x1520,0x15c0,0x1650,0x16a0,0x16f0,0x1740,0x1790,0x17e0,0x1830,0x1880,
 0x1910,0x19c0,0x1a20,0x1ae0,0x1b90,0x1cb0,0x1d50,0x1dc0,0x1e90,0x1f50,0x1fa0,0x1ff0,0x2040,0x2090,0x2220};
#define BASES (sizeof(offsets)/sizeof(*offsets))
static const gh_cockpit_widget registry[]={
 {0x6e0,1,0x41e6c8,0x143b70,0},{0x460,3,0x41e620,0x141b18,0},
 {0xea0,6,0x41e7f8,0x144b50,0},{0xcc0,4,0x41de90,0x13d048,0},
 {0x1380,7,0x41dc20,0x13bc28,0},{0x1470,8,0x41dc80,0x13c220,0},
 {0x1520,14,0x41e138,0x13f118,0},{0x15c0,9,0x41dfb0,0x13df70,0},
 {0x1cb0,15,0x41e8c0,0x1467e8,0},{0x1910,16,0x41e038,0x13e380,0},
 {0x1ae0,17,0x41e668,0x143558,0}};
struct gh_game_cockpit {
 gh_id_runtime *runtime;const gh_id_state *manager;gh_game_mt *random;
 gh_id_base bases[BASES],*owned[BASES];unsigned owned_count;
 gh_cockpit_widget widgets[11];uint8_t fields[0x23c0];
 struct {uint32_t offset;gh_id_entry *entry;} refs[160];unsigned ref_count;
 gh_id_view gauge_texture[5],icon_texture;uint32_t frontier;int begun,ready,room_begun,room_ready;
 float radar_curve[5][2];uint8_t radar_color_curve[5][5],radar_pulse,frame_bound;
 unsigned update_stage,draw_stage,radar_stage,heat_stage;gh_cockpit_fan heat_fan;
};
static uint32_t u32(const uint8_t *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static void w32(gh_game_cockpit *s,unsigned o,uint32_t n){for(unsigned i=0;i<4;i++)s->fields[o+i]=(uint8_t)(n>>(8*i));}
static void w16(gh_game_cockpit *s,unsigned o,unsigned n){s->fields[o]=(uint8_t)n;s->fields[o+1]=(uint8_t)(n>>8);}
static void wf(gh_game_cockpit *s,unsigned o,float f){uint32_t n;memcpy(&n,&f,4);w32(s,o,n);}
static float add(float a,float b){volatile float n=a+b;return n;}
static float mul(float a,float b){volatile float n=a*b;return n;}
static uint32_t rnd(void *p){uint32_t n=0;(void)gh_game_mt_next_0030f348(p,&n);return n;}
static gh_id_base *base(gh_game_cockpit *s,unsigned o){for(unsigned i=0;i<BASES;i++)if(offsets[i]==o)return &s->bases[i];return NULL;}
static gh_id_error init(gh_game_cockpit *s,unsigned o,int id){
 gh_id_base *b=base(s,o);int original;gh_id_error e;
 if(!b)return GH_ID_ARGUMENT;e=gh_id_base_initialize_002a9ea0(s->runtime,b,1,id,rnd,s->random,&original);
 if(b->owns_run)s->owned[s->owned_count++]=b;if(e)s->frontier=b->first_unported;return e;
}
static gh_id_error work(gh_game_cockpit *s,unsigned o,int id,unsigned field,gh_id_entry **out){
 int dummy;gh_id_error e=gh_id_base_get_work_002aa410(s->runtime,base(s,o),id,out,&dummy);
 if(e)return e;if(field){
  /* Original writes a pointer at self+field. The native pointer sidecar is
   * keyed by that field, not a history of stores across room resets. */
  for(unsigned i=0;i<s->ref_count;i++)if(s->refs[i].offset==field){s->refs[i].entry=*out;return GH_ID_OK;}
  if(s->ref_count>=160)return GH_ID_CAPACITY;s->refs[s->ref_count].offset=field;s->refs[s->ref_count++].entry=*out;
 }return GH_ID_OK;
}
static gh_id_error hide(gh_game_cockpit *s,unsigned o,int id){return gh_id_base_set_family_display(s->runtime,base(s,o),id,0);}
static gh_id_error restart(gh_game_cockpit *s,unsigned o){return gh_id_base_restart_anim(base(s,o),rnd,s->random);}
#define TRY(x) do{gh_id_error error_=(x);if(error_)return error_;}while(0)
static gh_id_error cache(gh_game_cockpit *s,unsigned o,unsigned count,unsigned field,int hidden){
 for(unsigned i=0;i<count;i++){gh_id_entry *e;if(hidden)TRY(hide(s,o,(int)i));TRY(work(s,o,(int)i,field+i*4,&e));}return GH_ID_OK;
}
/* Actual2ACDC8 pointer lookup, without assuming its texture format/decoding. */
static gh_id_error texture(gh_game_cockpit *s,uint32_t n,gh_id_view *out){
 const gh_bundle_entry *t=&s->manager->pair[1].texture_data;uint32_t count,off;size_t end;
 if(!t->bytes||t->size<4)return GH_ID_BOUNDS;count=u32(t->bytes);if(count>(t->size-4)/4)return GH_ID_BOUNDS;
 if(!n||n>count)return GH_ID_NOT_FOUND;off=u32(t->bytes+4*n);if(!off)return GH_ID_NOT_FOUND;
 if(off<4+(size_t)count*4||off>=t->size)return GH_ID_BOUNDS;end=t->size;
 for(unsigned i=1;i<=count;i++){uint32_t v=u32(t->bytes+4*i);if(v&&(v<4+(size_t)count*4||v>t->size))return GH_ID_BOUNDS;if(v>off&&v<end)end=v;}
 out->bytes=t->bytes+off;out->size=end-off;out->offset=off;return GH_ID_OK;
}
static int level(gh_core_save_view save){static const int threshold[]={1000,3000,4000,4000,5000};int n=0,v=(int)(save.bytes[0x1c]|(unsigned)save.bytes[0x1d]<<8);if(v>=32768)v-=65536;while(n<5&&v>=threshold[n])n++;return (n<3?n:4)+1;}
static gh_id_error gauge(gh_game_cockpit *s){
 const unsigned o=0xea0;gh_id_entry *m[5][5];static const float scale[]={0,1.2f,1,.8f};
 wf(s,o+0x290,155);wf(s,o+0x294,-59);wf(s,o+0x298,0);
 for(unsigned id=0;id<5;id++)for(unsigned b=0;b<5;b++){
  unsigned p=o+0x100+b*0x50;TRY(hide(s,p,(int)id));TRY(restart(s,p));base(s,p)->playing=0;
  TRY(work(s,p,(int)id,o+0x94+b*0x14+id*4,&m[b][id]));
 }
 for(unsigned b=0;b<4;b++){
  gh_id_entry *a=m[b][0],*c=m[b][1];a->color_base=b==1?u32(a->source+0x50):0x50808080;
  c->texture=s->gauge_texture[b];a->scale_x=a->scale_y=c->scale_x=c->scale_y=0;
  a->pos_x=add(add(a->pos_x,155),mul(add(mul(a->f68,scale[b]),-a->f68),.5f));
  a->pos_y=add(add(a->pos_y,-59),(float)(b*32));wf(s,o+0x33c+b*4,a->pos_y);wf(s,o+0x350+b*4,a->pos_x);
 }
 m[4][1]->texture=s->gauge_texture[4];m[4][0]->scale_x=m[4][0]->scale_y=m[4][1]->scale_x=m[4][1]->scale_y=0;
 m[4][0]->pos_x=add(add(m[4][0]->pos_x,155),-mul(m[4][0]->f68,.5f));m[4][0]->pos_y=m[3][0]->pos_y;
 wf(s,o+0x360,m[4][0]->pos_x);wf(s,o+0x34c,m[4][0]->pos_y);m[4][0]->flags|=8;
 w16(s,o+0x41e,1);w16(s,o+0x420,0);w32(s,o+0x368,0x80808080);w32(s,o+0x364,0x80808080);w32(s,o+0x2a4,0);
 for(unsigned i=0;i<5;i++){gh_id_frontier f={0};gh_id_error e=gh_id_jobs_move(base(s,o+0x100+i*0x50),NULL,NULL,&f);if(e){s->frontier=f.address;return e;}}
 return GH_ID_OK;
}
static gh_id_error widget(gh_game_cockpit *s,unsigned i,gh_core_save_view save){
 unsigned o=registry[i].offset;gh_id_entry *e;
 switch(i){
 case 0:w32(s,o+0x90,0);TRY(init(s,o,15));TRY(init(s,o+0x500,14));TRY(init(s,o+0x550,21));TRY(init(s,o+0x4b0,13));TRY(texture(s,0x36,&s->icon_texture));break;
 case 1:
  TRY(init(s,o,0));TRY(init(s,o+0xe0,12));TRY(init(s,o+0x190,1));TRY(init(s,o+0x210,20));
  TRY(cache(s,o,18,o+0x98,1));TRY(cache(s,o+0xe0,19,o+0x130,1));TRY(cache(s,o+0x190,12,o+0x1e0,1));
  TRY(restart(s,o));TRY(restart(s,o+0x210));for(int id=12;id<=15;id++){TRY(work(s,o,id,0,&e));e->flags|=0x400;}
  w16(s,o+0x17c,(unsigned)level(save));s->fields[o+0x26d]=1;s->fields[o+0x26c]=0;break;
 case 2:
  for(unsigned j=0;j<5;j++)TRY(init(s,o+0x100+j*0x50,5));TRY(init(s,o+0x430,25));TRY(init(s,o+0x480,26));
  w32(s,o+0x368,0x80808080);w32(s,o+0x364,0x80808080);TRY(gauge(s));w16(s,o+0x41e,0);w16(s,o+0x420,0);w32(s,o+0x90,0);w16(s,o+0x4d0,95);break;
 case 3:TRY(init(s,o,6));TRY(init(s,o+0x140,7));TRY(init(s,o+0x190,8));TRY(init(s,o+0xa0,27));TRY(init(s,o+0xf0,28));break;
 case 4:w32(s,o+0x90,0);TRY(init(s,o,4));TRY(cache(s,o,13,o+0xb0,0));TRY(work(s,o,0,0,&e));wf(s,o+0xa0,e->pos_x);wf(s,o+0xa4,e->pos_y);w32(s,o+0xe4,0xffffffff);break;
 case 5:w32(s,o+0x90,0);TRY(init(s,o,10));TRY(cache(s,o,4,o+0x98,0));break;
 case 6:TRY(init(s,o,17));w16(s,o+0x9c,0xffff);break;
 case 7:for(unsigned j=0;j<8;j++){unsigned p=o+0x90+j*0x50;uint32_t bits=0x3bed72ca;TRY(init(s,p,18));memcpy(&base(s,p)->f40,&bits,4);base(s,p)->h44=1;}break;
 case 8:
  TRY(init(s,o,29));TRY(init(s,o+0xa0,30));TRY(cache(s,o,4,o+0x90,0));w16(s,o+0x100,0);
  memset(s->fields+o+0x79,0,4);for(unsigned j=0;j<4;j++)TRY(hide(s,o,(int)j));TRY(work(s,o,0,0,&e));e->pos_y=-50;
  for(unsigned j=0;j<4;j++)TRY(hide(s,o+0xa0,(int)j));TRY(work(s,o+0xa0,0,0,&e));e->pos_y=-50;w16(s,o+0x102,90);break;
 case 9:TRY(init(s,o,31));TRY(cache(s,o,2,o+0x9c,0));w32(s,o+0x90,0);break;
 case 10:TRY(init(s,o,19));TRY(init(s,o+0xb0,24));break;
 default:return GH_ID_ARGUMENT;
 }return GH_ID_OK;
}
gh_id_error gh_game_cockpit_create(gh_id_runtime *r,const gh_id_state *m,gh_game_mt *random,gh_game_cockpit **out){
 gh_game_cockpit *s;if(!r||!m||!m->initialized||!random||!out)return GH_ID_ARGUMENT;*out=NULL;
 s=calloc(1,sizeof(*s));if(!s)return GH_ID_MEMORY;s->runtime=r;s->manager=m;s->random=random;
 for(unsigned i=0;i<BASES;i++){
  unsigned o=offsets[i];gh_id_base_clear_002a9df8(&s->bases[i]);
  w32(s,o+0x2c,0x3f800000);w32(s,o+0x3c,0x3f800000);w32(s,o+0x40,0x3c23d70a);w16(s,o+0x44,128);s->fields[o+0x46]=6;
 }
 /* Cold BSS storage supplies the untouched fields. 2D5D60 on five7C-byte
  * subobjects writes only0,+4,+8,+28,+48,+5C, all zero here. */
 static const unsigned vectors[]={0x1130,0x1420,0x2100,0x2110,0x2120,0x2130,0x2140};
 for(unsigned i=0;i<sizeof(vectors)/sizeof(*vectors);i++)w32(s,vectors[i]+12,0x3f800000);
 for(unsigned i=0;i<11;i++)w32(s,registry[i].offset+0x80,registry[i].vtable);
 w32(s,0x80,0x41df60);w32(s,0x420,0x41e0f0);w32(s,0x1e40,0x41de48);w32(s,0x1f10,0x41e548);w32(s,0x22a0,0x41e0a0);w32(s,0x1aa0,0x41e9b8);
 *out=s;return GH_ID_OK;
}
gh_id_error gh_game_cockpit_initialize_00140c88(gh_game_cockpit *s,gh_core_save_view save){
 if(!s||!save.bytes||save.size<GH_CORE_SAVE_BYTES)return GH_ID_ARGUMENT;if(s->begun)return GH_ID_BUSY;s->begun=1;
 memset(s->fields+0x22f0,0,0xcc);memcpy(s->widgets,registry,sizeof(registry));
 for(unsigned i=0;i<11;i++)w32(s,0x22f0+i*12,registry[i].kind);
 /* Original six direct initializer calls precede the registered dispatch. */
 TRY(init(s,0,3));TRY(init(s,0xd0,32));w16(s,0x92,0);
 TRY(init(s,0x3a0,2));w16(s,0x3a0+0xbe,0xffff);
 w32(s,0x1dc0+0x9c,0);w32(s,0x1dc0+0x94,0);w32(s,0x1e90+0xa0,0);
 for(unsigned i=0;i<5;i++)w32(s,0x1e90+0x250+i*4,0);s->fields[0x1e90+0x358]=0;
 w32(s,0x2220+0x9c,0);w32(s,0x2220+0x94,0);w32(s,0x1a20+0xb0,0);w16(s,0x1a20+0xac,0xffff);
 for(unsigned i=0;i<11;i++){TRY(widget(s,i,save));s->widgets[i].initialized=1;}
 s->fields[0x23bc]=0;s->ready=1;return GH_ID_OK;
}
static gh_id_error reset_only(gh_game_cockpit*s,unsigned o){gh_id_base*b=base(s,o);uint8_t playing=b->playing;gh_id_error e=restart(s,o);b->playing=playing;return e;}
static const gh_id_animation_profile room_animation_profile={1,0}; /*explicitnativeadaptation: originalunwrittenstackdistance seed0*/
static gh_id_error move_only(gh_game_cockpit*s,unsigned o){gh_id_frontier f={0};gh_id_error e=gh_id_jobs_move_reference(base(s,o),NULL,NULL,&room_animation_profile,&f);if(e)s->frontier=f.address;return e;}
static gh_id_error reset_family(gh_game_cockpit*s,unsigned o,int id,int16_t hidden){gh_id_entry*e;gh_id_frontier f={0};TRY(work(s,o,id,0,&e));gh_id_error result=gh_id_entry_reset_family_reference(base(s,o),e,hidden,&room_animation_profile,&f);if(result)s->frontier=f.address;return result;}
static gh_id_error flag_entry(gh_game_cockpit*s,unsigned o,int id,uint32_t set,uint32_t clear){gh_id_entry*e;TRY(work(s,o,id,0,&e));e->flags=(e->flags&~clear)|set;return GH_ID_OK;}
static gh_id_error room_heat_reset_00141ce8(gh_game_cockpit*s,gh_core_save_view save,const uint8_t heat[0x18]){
 gh_id_entry*e;unsigned o;
 /*141CE8: saved level, upgrade icons and original per-entry/directchild
  * reset do not call wholeMove or advance unrelatedentries. */
 o=0x460;w32(s,o+0x90,0);TRY(restart(s,o));TRY(restart(s,o+0x210));for(int id=12;id<=15;id++)TRY(flag_entry(s,o,id,0x8000000,0));
 TRY(restart(s,o+0xe0));TRY(restart(s,o+0x190));base(s,o+0x190)->playing=0;w32(s,o+0x180,0);
 int lev=level(save);if(lev==4)lev=3;else if(lev==5)lev=4;lev--;w16(s,o+0x17c,(unsigned)lev);
 TRY(flag_entry(s,o,lev+12,0,0x8000000));TRY(reset_family(s,o,lev+12,0));
 for(int id=7;id<=9;id++)TRY(flag_entry(s,o+0x190,id,0x8000000,0));for(int id=12;id<=15;id++)TRY(flag_entry(s,o,id,0x400,0));
 TRY(flag_entry(s,o+0x190,6,0x1000,0));TRY(work(s,o+0x190,6,0,&e));e->scale_x=e->scale_y=.75f;
 unsigned upgrades=0;for(unsigned i=0;i<6&&i<save.bytes[0x156];i++)if(save.bytes[0xa8+i])upgrades++;s->fields[o+0x260]=(uint8_t)upgrades;
 for(unsigned i=0;i<6;i++)TRY(flag_entry(s,o+0xe0,(int)i,i<upgrades?0:0x8000000,i<upgrades?0x8000000:0));
 /* Actualfirstboot Heat.current=0. This branch is exactlyzero foranyfinite
  * nonzeroMaximum; other runtime ratios need explicitnumericalpolicy. */
 if(u32(heat+4)!=0&&u32(heat+4)!=0x80000000){s->frontier=0x2a9c98;return GH_ID_UNSUPPORTED;}
 w32(s,o+0x264,0);w32(s,o+0x268,0);
 for(int id=13;id<=18;id++)TRY(flag_entry(s,o+0xe0,id,0x8000000,0));TRY(flag_entry(s,o,17,0x8000000,0));TRY(flag_entry(s,o,6,0x8000000,0));
 TRY(reset_family(s,o,11,1));TRY(work(s,o,11,0,&e));e->color_base&=0xffffff;
 static const int reset_ids[]={2,10,11};for(unsigned i=0;i<3;i++){TRY(reset_family(s,o+0x190,reset_ids[i],1));TRY(work(s,o+0x190,reset_ids[i],0,&e));e->color_base=(e->color_base&0xffffff)|(i==2?0x80000000:0);}
 s->fields[o+0x26f]=0;
 return GH_ID_OK;
}

gh_id_error gh_game_cockpit_room_reset_00140de0(gh_game_cockpit*s,gh_core_save_view save,const gh_boot_work*work_state,const uint8_t heat[0x18]){
 gh_id_entry*e;const uint8_t*wr=(const uint8_t*)work_state;unsigned o;
 if(!s||!s->ready||!save.bytes||save.size<GH_CORE_SAVE_BYTES||!work_state||!heat)return GH_ID_ARGUMENT;
 if(s->room_begun)return GH_ID_BUSY;s->room_begun=1;s->frontier=0;
 /*143BE8, firstnonNULL registryrow. Allpointercachefields stay native. */
 o=0x6e0;w32(s,o+0x90,0);if(u32(wr+0x5f4)){s->frontier=0x2db6b0;return GH_ID_UNSUPPORTED;}w32(s,o+0x98,0);wf(s,o+0x5d0,20);
 TRY(restart(s,o));TRY(restart(s,o+0x500));TRY(move_only(s,o+0x500));TRY(restart(s,o+0x550));TRY(move_only(s,o+0x550));TRY(restart(s,o+0x4b0));
 TRY(cache(s,o,2,o+0x4a0,0));TRY(work(s,o+0x500,0,o+0x5a4,&e));TRY(work(s,o+0x550,0,o+0x5a8,&e));TRY(work(s,o+0x4b0,0,o+0x5a0,&e));
 s->fields[o+0x5dc]=s->fields[o+0x5dd]=0;w32(s,o+0x5d8,0x80808080);
 TRY(room_heat_reset_00141ce8(s,save,heat));
 /*144C08 then13D0B8: actualreset-only preserves baseplaying. */
 o=0xea0;TRY(gauge(s));for(unsigned i=0;i<5;i++)TRY(reset_only(s,o+0x100+i*0x50));TRY(reset_only(s,o+0x430));TRY(reset_only(s,o+0x480));w16(s,o+0x41e,0);w16(s,o+0x420,0);w32(s,o+0x90,0);w16(s,o+0x4d0,95);
 o=0xcc0;TRY(reset_only(s,o));TRY(reset_only(s,o+0x140));TRY(reset_only(s,o+0x190));TRY(reset_only(s,o+0xa0));TRY(reset_only(s,o+0xf0));
 o=0x1380;w32(s,o+0x90,0);TRY(restart(s,o));TRY(cache(s,o,13,o+0xb0,0));s->fields[o+0x95]=s->fields[o+0x94]=0;
 o=0x1470;w32(s,o+0x90,0);TRY(restart(s,o));TRY(cache(s,o,4,o+0x98,0));w32(s,o+0x94,0);
 o=0x1520;TRY(restart(s,o));TRY(cache(s,o,3,o+0x90,0));
 o=0x15c0;for(unsigned i=0;i<8;i++)TRY(restart(s,o+0x90+i*0x50));memset(s->fields+o+0x33c,255,8);
 /*146878->1468B0, sameoriginalfamilyhide semantics asinitialization. */
 o=0x1cb0;memset(s->fields+o+0x79,0,4);for(int id=0;id<4;id++)TRY(hide(s,o,id));TRY(work(s,o,0,0,&e));e->pos_y=-50;
 for(int id=0;id<4;id++)TRY(hide(s,o+0xa0,id));TRY(work(s,o+0xa0,0,0,&e));e->pos_y=-50;w16(s,o+0x102,90);w16(s,o+0x100,0);TRY(restart(s,o));
 s->fields[0x1910+0x104]=0;
 o=0x1ae0;TRY(restart(s,o));if(u32(wr+0x594)&0x2000000)for(unsigned i=0;i<4;i++)save.bytes[0x20+i]=(uint8_t)(999999u>>(8*i));
 uint32_t money=u32(save.bytes+0x20);w32(s,o+0x90,money);TRY(cache(s,o,4,o+0x94,0));memset(s->fields+o+0x1b0,0,16);TRY(flag_entry(s,o,2,0x8000000,0));TRY(work(s,o,0,0,&e));e->seed=money;memset(save.bytes+0x24,0,4);s->fields[o+0x1c0]=0;
 s->fields[0x23bc]=0;for(unsigned i=0;i<11;i++){s->fields[registry[i].offset+0x76]=0;s->fields[registry[i].offset+0x75]=1;}
 s->room_ready=1;return GH_ID_OK;
}

gh_id_error gh_game_cockpit_destroy(gh_game_cockpit *s){
 gh_id_error result=GH_ID_OK;if(!s)return GH_ID_ARGUMENT;
 while(s->owned_count){int original,freed;gh_id_error e=gh_id_base_release_002aa078(s->runtime,s->owned[--s->owned_count],&original,&freed);if(e)result=e;else if(!freed)result=GH_ID_BUSY;}
 free(s);return result;
}
gh_id_error gh_game_cockpit_base(gh_game_cockpit *s,uint32_t o,gh_id_base **out){if(!s||!out)return GH_ID_ARGUMENT;*out=base(s,o);return *out?GH_ID_OK:GH_ID_NOT_FOUND;}
gh_id_error gh_game_cockpit_widget_at(const gh_game_cockpit *s,unsigned i,gh_cockpit_widget *out){if(!s||!out)return GH_ID_ARGUMENT;if(i>=11)return GH_ID_BOUNDS;*out=s->widgets[i];return GH_ID_OK;}
gh_id_error gh_game_cockpit_entry_at(const gh_game_cockpit *s,uint32_t o,gh_id_entry **out){if(!s||!out)return GH_ID_ARGUMENT;for(unsigned i=0;i<s->ref_count;i++)if(s->refs[i].offset==o){*out=s->refs[i].entry;return GH_ID_OK;}return GH_ID_NOT_FOUND;}
const uint8_t *gh_game_cockpit_fields(const gh_game_cockpit *s,size_t *n){if(n)*n=s?sizeof(s->fields):0;return s?s->fields:NULL;}
uint32_t gh_game_cockpit_frontier(const gh_game_cockpit *s){return s?s->frontier:0;}
int gh_game_cockpit_ready(const gh_game_cockpit *s){return s&&s->ready;}
uint8_t *gh_game_cockpit_action_flags_1414(gh_game_cockpit*s){return s?s->fields+0x1414:NULL;}
static gh_id_error frame_stop(gh_game_cockpit*s,uint32_t f){s->frontier=f;return GH_ID_UNSUPPORTED;}
static const uint8_t *frame_elf(const uint8_t*e,size_t z,uint32_t va,size_t n){
 if(z<52||memcmp(e,"\177ELF\1\1\1",7)||e[18]!=8||e[19])return NULL;
 uint32_t table=u32(e+28);unsigned stride=e[42]|(unsigned)e[43]<<8,count=e[44]|(unsigned)e[45]<<8;
 if(stride<32||count>128||table>z||count>(z-table)/stride)return NULL;
 for(unsigned i=0;i<count;i++){const uint8_t*p=e+table+(size_t)i*stride;uint32_t off=u32(p+4),v=u32(p+8),files=u32(p+16);
  if(u32(p)!=1||v>va||(uint64_t)va+n>(uint64_t)v+files)continue;if(files>u32(p+20)||off>z||files>z-off)return NULL;return e+off+(va-v);
 }return NULL;
}
gh_id_error gh_game_cockpit_bind_frame_elf(gh_game_cockpit*s,const void*data,size_t z){
 if(!s||!data)return GH_ID_ARGUMENT;if(s->frame_bound)return GH_ID_BUSY;const uint8_t*e=data;
 const uint8_t*a=frame_elf(e,z,0x41e780,40),*b=frame_elf(e,z,0x41e7a8,25),*p=frame_elf(e,z,0x3bd6f4,1);
 if(!a||!b||!p)return GH_ID_BOUNDS;
 float curve[5][2];memcpy(curve,a,40);
 for(unsigned i=0;i<5;i++)if(!isfinite(curve[i][0])||!isfinite(curve[i][1])||(i&&curve[i][0]<=curve[i-1][0])||(i&&b[i*5]<=b[(i-1)*5]))return GH_ID_FORMAT;
 if(curve[0][0]!=0||curve[4][0]!=16||b[0]!=0||b[20]!=16)return GH_ID_UNSUPPORTED;
 memcpy(s->radar_curve,curve,40);memcpy(s->radar_color_curve,b,25);s->radar_pulse=*p;s->frame_bound=1;return GH_ID_OK;
}
static gh_id_error frame_move_base(gh_game_cockpit*s,unsigned o,const gh_cockpit_frame_callbacks*c){gh_id_frontier f={0};
 gh_id_error e=gh_id_jobs_move_reference(base(s,o),c?c->post:NULL,c?c->post_context:NULL,&room_animation_profile,&f);if(e)s->frontier=f.address;return e;
}
gh_id_error gh_game_cockpit_move_00140e58(gh_game_cockpit*s,const gh_boot_work*work_state,const gh_cockpit_frame_callbacks*c){
 if(!s||!s->ready||!s->room_ready||!work_state)return GH_ID_ARGUMENT;const uint8_t*w=(const uint8_t*)work_state;s->frontier=0;
 if(!s->update_stage){if(u32(w+0x58c)&0x80000000)return GH_ID_OK;
  /* Optional1415E0 is verified jrra/nop;13C950 is the first real
   * unported operation of the other global branch. */
  if(!(u32(w+0x5d8)&0x40000))return frame_stop(s,0x13c950);
  /* Actual13F1C0 on registered kind14 only in this global branch. */
  if(u32(s->fields+0x15bc)!=0xffff){gh_id_entry*a,*b;TRY(gh_game_cockpit_entry_at(s,0x15b0,&a));TRY(gh_game_cockpit_entry_at(s,0x15b4,&b));uint32_t n=u32(s->fields+0x15bc)&0xffff;a->seed=(a->seed&0xffff0000)|n;b->seed=(b->seed&0xffff0000)|n;}
  s->update_stage=1;
 }
 if(s->update_stage==1){TRY(frame_move_base(s,0x1520,c));s->update_stage=2;}
 if(s->update_stage==2){int a=u32(s->fields+0x1e5c)&&(u32(s->fields+0x1e60)&0x20000000);
  int b=s->fields[0x21e8]&&(u32(s->fields+0x1f20)&0x80000000);
  int d=u32(s->fields+0x22bc)&&(u32(s->fields+0x22c0)&0x20000000);
  if(a||b||d)s->fields[0x1ca0]|=2;
  if(a||b)s->fields[0x1db4]|=1;else if(d)s->fields[0x1db4]|=2;s->update_stage=3;
 }
 if(u32(w+0x584)&0x20000){if(!c||!c->set_draw_order)return frame_stop(s,0x2c7850);uint32_t f=0;gh_id_error e=c->set_draw_order(c->context,6,5,&f);if(e){s->frontier=f?f:0x2c7850;return e;}}
 s->update_stage=0;return GH_ID_OK;
}
static gh_id_error frame_trans(gh_game_cockpit*s,unsigned o,const gh_cockpit_frame_callbacks*c){
 if(!c||!c->trans)return frame_stop(s,0x2aac28);uint32_t f=0;gh_id_error e=c->trans(c->context,base(s,o),&f);if(e)s->frontier=f?f:0x2aac28;return e;
}
static gh_id_error radar_prefix(gh_game_cockpit*s){
 if(!s->frame_bound)return frame_stop(s,0x144368);unsigned frame=s->fields[0x6e0+0x5dc];if(frame>=16)return frame_stop(s,0x144368);
 fenv_t env;if(feholdexcept(&env))return GH_ID_UNSUPPORTED;if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return GH_ID_UNSUPPORTED;}
 unsigned i=0,j=0;while(i<3&&s->radar_curve[i+1][0]<=(float)frame)i++;while(j<3&&s->radar_color_curve[j+1][0]<=frame)j++;
 volatile float a=(float)frame-s->radar_curve[i][0],b=s->radar_curve[i+1][0]-s->radar_curve[i][0],q=a/b;
 volatile float delta=s->radar_curve[i+1][1]-s->radar_curve[i][1],v=delta*q;float scale=add(v,s->radar_curve[i][1]);
 volatile float colorq=((float)frame-(float)s->radar_color_curve[j][0])/(float)(s->radar_color_curve[j+1][0]-s->radar_color_curve[j][0]);uint8_t color[4];
 for(unsigned k=0;k<4;k++){int old=s->radar_color_curve[j][k+1],next=s->radar_color_curve[j+1][k+1];volatile float p=(float)(next-old)*colorq;color[k]=(uint8_t)(old+(int)p);}
 if(fesetenv(&env))return GH_ID_UNSUPPORTED;
 unsigned pulse=(s->radar_pulse+5u)&255u;s->radar_pulse=(uint8_t)(pulse>128?0:pulse);
 wf(s,0x6e0+0x5d4,scale);s->fields[0x6e0+0x5db]=color[0];s->fields[0x6e0+0x5d8]=color[1];s->fields[0x6e0+0x5d9]=color[2];s->fields[0x6e0+0x5da]=color[3];s->fields[0x6e0+0x5dc]=(uint8_t)((frame+1)&15);return GH_ID_OK;
}
gh_id_error gh_game_cockpit_trans_001410a8(gh_game_cockpit*s,const gh_boot_work*work_state,const gh_cockpit_frame_callbacks*c){
 if(!s||!s->ready||!s->room_ready||!work_state)return GH_ID_ARGUMENT;const uint8_t*w=(const uint8_t*)work_state;s->frontier=0;
 if(!s->draw_stage){if(u32(w+0x58c)&0x80000000)return GH_ID_OK;
  if((u32(w+0x584)&8)&&!(u32(w+0x594)&0x800))return frame_stop(s,0x1410a8);
  if(u32(w+0x5e0)&0x2000000){if(u32(s->fields+0x15bc)!=0xffff)TRY(frame_trans(s,0x1520,c));return GH_ID_OK;}
  /* Four direct draw gates, in original order, before17registered slots. */
  if(u32(s->fields+0x1e54)&&u32(s->fields+0x1e5c))return frame_stop(s,0x13cf20);
  if(u32(s->fields+0x1f30)&&(u32(s->fields+0x1f20)&0x80000000))return frame_stop(s,0x140638);
  if(u32(s->fields+0x22b4)&&u32(s->fields+0x22bc))return frame_stop(s,0x13eba0);
  unsigned kind=s->fields[0x1ad8];if(u32(s->fields+0x1ad0)&&kind>0&&kind<5)return frame_stop(s,0x147460);
  s->draw_stage=1;
 }
 /* ActualfirstregisteredRadar, no widget hidden to bypass its consumer. */
 if(s->draw_stage==1){if(!s->fields[0x6e0+0x78]){
   if(!s->radar_stage){TRY(radar_prefix(s));s->radar_stage=1;}
   if(s->radar_stage==1){TRY(frame_trans(s,0x6e0+0x4b0,c));s->radar_stage=2;}
   if(u32(s->fields+0x6e0+0x49c))return frame_stop(s,0x144368);
   TRY(frame_trans(s,0x6e0,c));s->radar_stage=0;
  }s->draw_stage=2;
 }
 if(s->draw_stage==2){if(!s->fields[0x460+0x78]){
   if(!s->heat_stage){TRY(frame_trans(s,0x5f0,c));s->heat_stage=1;}
   if(s->heat_stage==1){if(!c)return frame_stop(s,0x1fa080);uint32_t f=0;gh_id_error e=gh_cockpit_fan_prepare(s->manager,c->save,&s->heat_fan,&f);if(e){s->frontier=f?f:0x1430e8;return e;}wf(s,0x5e0,s->heat_fan.u[32]);s->heat_stage=2;}
   if(s->heat_stage==2){if(!c||!c->fan)return frame_stop(s,0x15c410);uint32_t f=0;gh_id_error e=c->fan(c->context,&s->heat_fan,&f);if(e){s->frontier=f?f:0x15c410;return e;}s->heat_stage=3;}
   if(s->heat_stage==3){TRY(frame_trans(s,0x460,c));s->heat_stage=4;}
   if(s->heat_stage==4){TRY(frame_trans(s,0x540,c));s->heat_stage=5;}
   if(s->heat_stage==5){if(!c||!c->save.bytes||c->save.size<GH_CORE_SAVE_BYTES)return frame_stop(s,0x1fac80);int32_t current=(int32_t)u32(c->save.bytes+0x84),maximum=(int32_t)u32(c->save.bytes+0x80);
    if(current<0||maximum<=0)return frame_stop(s,0x1fac80);fenv_t env;if(feholdexcept(&env))return GH_ID_UNSUPPORTED;if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return GH_ID_UNSUPPORTED;}volatile float ratio=(float)current/(float)maximum;int warning=ratio<.2f;if(fesetenv(&env))return GH_ID_UNSUPPORTED;s->heat_stage=warning?6:7;
   }
   if(s->heat_stage==6){TRY(frame_trans(s,0x670,c));s->heat_stage=7;}s->heat_stage=0;
  }s->draw_stage=3;
 }
 if(s->draw_stage==3){if(!s->fields[0xea0+0x78]){unsigned mode=s->fields[0x12be]|(unsigned)s->fields[0x12bf]<<8;if(mode)return frame_stop(s,0x146328);}s->draw_stage=4;}
 /* kind4 Draw13D148 is actual jrra/nop. */
 if(s->draw_stage==4)s->draw_stage=5;
 if(s->draw_stage==5){if(!s->fields[0x1380+0x78]&&(s->fields[0x1380+0x94]&1))TRY(frame_trans(s,0x1380,c));s->draw_stage=6;}
 if(s->draw_stage==6){if(!s->fields[0x1470+0x78])TRY(frame_trans(s,0x1470,c));s->draw_stage=7;}
 if(s->draw_stage==7){if(!s->fields[0x1520+0x78]&&u32(s->fields+0x15bc)!=0xffff)TRY(frame_trans(s,0x1520,c));s->draw_stage=8;}
 if(s->draw_stage==8){if(!s->fields[0x15c0+0x78]){for(unsigned i=0;i<8;i++)if(s->fields[0x18fc+i]!=255)return frame_stop(s,0x13e260);}s->draw_stage=9;}
 if(s->draw_stage==9){if(!s->fields[0x1cb0+0x78]&&s->fields[0x1d29]&&s->fields[0x1d29]!=6)TRY(frame_trans(s,(s->fields[0x1da8]||s->fields[0x1da9])?0x1d50:0x1cb0,c));s->draw_stage=10;}
 if(s->draw_stage==10){if(!s->fields[0x1910+0x78]&&!(u32(w+0x584)&8)){
   if(w[0x5b1])return frame_stop(s,0x13e6e0);if(u32(s->fields+0x19a0))TRY(frame_trans(s,0x19c0,c));
  }s->draw_stage=11;
 }
 if(s->draw_stage==11){if(!s->fields[0x1ae0+0x78]){TRY(frame_trans(s,0x1ae0,c));s->draw_stage=12;}else s->draw_stage=13;}
 if(s->draw_stage==12){gh_id_entry*e;TRY(work(s,0x1b90,0,0,&e));(void)e;for(unsigned i=0;i<16;i++)if(s->fields[0x1c90+i]==1)return frame_stop(s,0x143998);s->draw_stage=13;}
 if(s->draw_stage!=13)return frame_stop(s,0x1410a8);s->draw_stage=0;return GH_ID_OK;
}

gh_id_error gh_game_cockpit_get_system_menu(gh_game_cockpit*s,gh_cockpit_system_menu_view*out){
 if(!s||!out||!s->ready)return GH_ID_ARGUMENT;
 *out=(gh_cockpit_system_menu_view){base(s,0x3a0),s->fields+0x3a0+0x90,0x30};return GH_ID_OK;
}

gh_id_error gh_game_cockpit_get_tutorial(gh_game_cockpit*s,gh_cockpit_tutorial_view*out){
 if(!s||!out||!s->ready)return GH_ID_ARGUMENT;
 *out=(gh_cockpit_tutorial_view){base(s,0x1a20),s->manager,u32(s->fields+0x1a20+0xb4),
 (uint16_t)(s->fields[0x1a20+0xac]|(unsigned)s->fields[0x1a20+0xad]<<8),
 u32(s->fields+0x1a20+0xb0)!=0,s->fields[0x1a20+0xb8]};return GH_ID_OK;
}
gh_id_error gh_game_cockpit_tutorial_mark_resource(gh_game_cockpit*s,uint32_t job){
 if(!s||!s->ready||!job)return GH_ID_ARGUMENT;
 w32(s,0x1a20+0xb4,job);w32(s,0x1a20+0xb0,1);return GH_ID_OK;
}
gh_id_error gh_game_cockpit_tutorial_begin_base(gh_game_cockpit*s){
 if(!s||!s->ready)return GH_ID_ARGUMENT;gh_id_base*b=base(s,0x1a20);
 if(b->owns_run)return GH_ID_BUSY;
 if(s->owned_count>=BASES)return GH_ID_CAPACITY;
 int original;gh_id_error e=gh_id_base_initialize_002a9ea0(s->runtime,b,0x1d,0,rnd,s->random,&original);
 if(b->owns_run)s->owned[s->owned_count++]=b;
 if(e){s->frontier=b->first_unported?b->first_unported:0x2a9ea0;return e;}
 if(original!=1){s->frontier=0x2a9ea0;return GH_ID_NOT_FOUND;}return GH_ID_OK;
}
gh_id_error gh_game_cockpit_tutorial_finish_00147028(gh_game_cockpit*s){
 if(!s||!s->ready)return GH_ID_ARGUMENT;unsigned o=0x1a20;
 TRY(restart(s,o));base(s,o)->playing=1;
 gh_id_entry*entries[7];for(unsigned i=0;i<7;i++)TRY(work(s,o,(int)i,o+0x90+i*4,&entries[i]));
 entries[0]->scale_x=0;entries[0]->scale_y=0;
 entries[1]->flags|=0x8000000;entries[2]->flags|=0x8000000;
 entries[3]->message=0x200c;entries[4]->message=0x200c;
 w16(s,o+0xac,0xffff);s->fields[o+0xb8]=0;return GH_ID_OK;
}
gh_id_error gh_game_cockpit_tutorial_detach(gh_game_cockpit*s){
 if(!s||!s->ready)return GH_ID_ARGUMENT;gh_id_base*b=base(s,0x1a20);
 if(b->owns_run){int original,freed;gh_id_error e=gh_id_base_release_002aa078(s->runtime,b,&original,&freed);if(e)return e;(void)original;(void)freed;}
 for(unsigned i=0;i<s->owned_count;)if(s->owned[i]==b){memmove(s->owned+i,s->owned+i+1,(s->owned_count-i-1)*sizeof(*s->owned));s->owned_count--;}else i++;
 for(unsigned i=0;i<s->ref_count;)if(s->refs[i].offset>=0x1a20+0x90&&s->refs[i].offset<0x1a20+0xac){memmove(s->refs+i,s->refs+i+1,(s->ref_count-i-1)*sizeof(*s->refs));s->ref_count--;}else i++;
 w32(s,0x1a20+0xb0,0);w16(s,0x1a20+0xac,0xffff);s->fields[0x1a20+0xb8]=0;return GH_ID_OK;
}

gh_id_error gh_game_cockpit_room_quit_00141628(gh_game_cockpit*s,gh_core_save_view save,const uint8_t heat[0x18]){
 if(!s||!s->ready||!save.bytes||save.size<GH_CORE_SAVE_BYTES||!heat)return GH_ID_ARGUMENT;s->frontier=0;
 /*13E400,13C930,13F510 then13E908,147188,1434A8->Heat141CE8. */
 w32(s,0x1910+0x90,0);unsigned o=0x1dc0;w32(s,o+0xb8,0);w32(s,o+0xa4,0x3f800000);w32(s,o+0x94,0);w32(s,o+0x9c,0);w32(s,o+0xa0,0);
 o=0x1e90;for(unsigned i=0;i<5;i++){w32(s,o+0x260-4*i,0);w32(s,o+0x2d0-4*i,0x3f800000);w32(s,o+0xb8-4*i,0);}s->fields[o+0x384]=0;w32(s,o+0xa0,0);s->fields[o+0x358]=0;
 o=0x2220;if(u32(s->fields+o+0x94)){s->frontier=0x2aa078;return GH_ID_UNSUPPORTED;}w32(s,o+0xa0,0);w32(s,o+0xa4,0x3f800000);w32(s,o+0x94,0);w32(s,o+0x9c,0);
 o=0x1a20;if(u32(s->fields+o+0xb0)){s->fields[o+0xb8]=0;w16(s,o+0xac,0xffff);w32(s,o+0xb0,0);}
 if(u32(s->fields+0x460+0x80)!=0x41e620){s->frontier=0x1434a8;return GH_ID_UNSUPPORTED;}
 TRY(room_heat_reset_00141ce8(s,save,heat));s->room_begun=s->room_ready=0;s->update_stage=s->draw_stage=s->radar_stage=s->heat_stage=0;return GH_ID_OK;
}
