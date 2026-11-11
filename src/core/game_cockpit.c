#include "game_actor_matrix.h"
#include "game_ee_acos.h"
/* Actual patched SLUS_215.03, original140C88 and concrete widget initializers.
 * Host handles replace every guest pointer. Finite UI source reconstruction,
 * not a GS interpreter or a claim of Cockpit Move/Trans implementation. */
#include "game_cockpit.h"
#include "id_entry_reset.h"
#include "id_animation.h"
#include "id_tick.h"
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
 float radar_curve[5][2],radar_uv[7][4];uint8_t radar_color_curve[5][5],radar_pulse,frame_bound;
 unsigned update_stage,draw_stage,radar_stage,heat_stage;
 unsigned radar_draw_pass,radar_draw_index;uint8_t radar_arrow_moved;gh_cockpit_fan heat_fan;
 gh_cockpit_gameplay_bindings gameplay;const gh_title_player_view *radar_player;
 uint8_t gameplay_bound,hud_cold_known;unsigned gameplay_index,radar_move_stage,radar_move_index,radar_marker_count;
 gh_id_error gameplay_error;uint32_t gameplay_error_frontier;
 /* Original mutable function statics, initialized from PT_LOAD (including
  * its zero-fill), retained across room resets, never inferred from calloc. */
 float life_origin_y,life_shake;uint32_t life_origin_known;
 uint8_t life_level_counter,life_level_direction;unsigned heat_move_stage;
 unsigned widget_step;
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
 s->hud_cold_known=7;w32(s,0x2220+0x9c,0);w32(s,0x2220+0x94,0);w32(s,0x1a20+0xb0,0);w16(s,0x1a20+0xac,0xffff);
 for(unsigned i=0;i<11;i++){TRY(widget(s,i,save));s->widgets[i].initialized=1;}
 s->fields[0x23bc]=0;s->ready=1;return GH_ID_OK;
}
static gh_id_error reset_only(gh_game_cockpit*s,unsigned o){gh_id_base*b=base(s,o);uint8_t playing=b->playing;gh_id_error e=restart(s,o);b->playing=playing;return e;}
static const gh_id_animation_profile room_animation_profile={1,0}; /*explicitnativeadaptation: originalunwrittenstackdistance seed0*/
static gh_id_error move_only(gh_game_cockpit*s,unsigned o){gh_id_frontier f={0};gh_id_error e=gh_id_jobs_move_reference(base(s,o),NULL,NULL,&room_animation_profile,&f);if(e)s->frontier=f.address;return e;}
static gh_id_error reset_family(gh_game_cockpit*s,unsigned o,int id,int16_t hidden){gh_id_entry*e;gh_id_frontier f={0};TRY(work(s,o,id,0,&e));gh_id_error result=gh_id_entry_reset_family_reference(base(s,o),e,hidden,&room_animation_profile,&f);if(result)s->frontier=f.address;return result;}
static gh_id_error flag_entry(gh_game_cockpit*s,unsigned o,int id,uint32_t set,uint32_t clear){gh_id_entry*e;TRY(work(s,o,id,0,&e));e->flags=(e->flags&~clear)|set;return GH_ID_OK;}
/* 2A9C98 uses the live canonical current and maximum. Keep the
 * binary32 division under an explicit reference FEnv for room entry too. */
static int heat_ratio_002a9c98(const uint8_t heat[0x18],float *ratio)
{
 float maximum,current;
 memcpy(&maximum,heat,4);memcpy(&current,heat+4,4);
 if(!isfinite(maximum)||maximum<0||!isfinite(current))return 0;
 if(maximum==0){*ratio=0;return 1;}
 fenv_t env;
 if(feholdexcept(&env))return 0;
 if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return 0;}
 volatile float result=current/maximum;
 if(fesetenv(&env)||!isfinite(result))return 0;
 *ratio=result;
 return 1;
}
static gh_id_error room_heat_reset_00141ce8(gh_game_cockpit*s,gh_core_save_view save,const uint8_t heat[0x18]){
 gh_id_entry*e;unsigned o;float heat_ratio;
 if(!heat_ratio_002a9c98(heat,&heat_ratio)){s->frontier=0x2a9c98;return GH_ID_UNSUPPORTED;}
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
 wf(s,o+0x264,heat_ratio);wf(s,o+0x268,heat_ratio);
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
uint8_t *gh_game_cockpit_auxiliary_flags_1ca0(gh_game_cockpit*s){return s?s->fields+0x1ca0:NULL;}
static gh_id_error frame_stop(gh_game_cockpit*s,uint32_t f){s->frontier=f;return GH_ID_UNSUPPORTED;}
static const uint8_t *frame_elf(const uint8_t*e,size_t z,uint32_t va,size_t n){
 if(z<52||memcmp(e,"\177ELF\1\1\1",7)||e[18]!=8||e[19])return NULL;
 uint32_t table=u32(e+28);unsigned stride=e[42]|(unsigned)e[43]<<8,count=e[44]|(unsigned)e[45]<<8;
 if(stride<32||count>128||table>z||count>(z-table)/stride)return NULL;
 for(unsigned i=0;i<count;i++){const uint8_t*p=e+table+(size_t)i*stride;uint32_t off=u32(p+4),v=u32(p+8),files=u32(p+16);
  if(u32(p)!=1||v>va||(uint64_t)va+n>(uint64_t)v+files)continue;if(files>u32(p+20)||off>z||files>z-off)return NULL;return e+off+(va-v);
 }return NULL;
}
static int frame_elf_value(const uint8_t*e,size_t z,uint32_t va,void*out,size_t n){
 if(z<52||memcmp(e,"\177ELF\1\1\1",7)||e[18]!=8||e[19])return 0;
 uint32_t table=u32(e+28);unsigned stride=e[42]|(unsigned)e[43]<<8,count=e[44]|(unsigned)e[45]<<8;
 if(stride<32||count>128||table>z||count>(z-table)/stride)return 0;
 for(unsigned i=0;i<count;i++){const uint8_t*p=e+table+(size_t)i*stride;uint32_t off=u32(p+4),v=u32(p+8),files=u32(p+16),mem=u32(p+20);
  if(u32(p)!=1||va<v||(uint64_t)va+n>(uint64_t)v+mem)continue;
  if(files>mem||off>z||files>z-off)return 0;size_t delta=va-v,copied=delta<files?files-delta:0;if(copied>n)copied=n;
  memset(out,0,n);if(copied)memcpy(out,e+off+delta,copied);return 1;
 }return 0;
}
gh_id_error gh_game_cockpit_bind_frame_elf(gh_game_cockpit*s,const void*data,size_t z){
 if(!s||!data)return GH_ID_ARGUMENT;if(s->frame_bound)return GH_ID_BUSY;const uint8_t*e=data;
 const uint8_t*a=frame_elf(e,z,0x41e780,40),*b=frame_elf(e,z,0x41e7a8,25),*p=frame_elf(e,z,0x3bd6f4,1);
 const uint8_t *uv=frame_elf(e,z,0x41e710,sizeof(s->radar_uv));
 if(!a||!b||!p||!uv)return GH_ID_BOUNDS;
 uint8_t life[8],origin[8];if(!frame_elf_value(e,z,0x3bd6ec,life,8)||!frame_elf_value(e,z,0x466440,origin,8))return GH_ID_BOUNDS;
 float curve[5][2];memcpy(curve,a,40);
 for(unsigned i=0;i<5;i++)if(!isfinite(curve[i][0])||!isfinite(curve[i][1])||(i&&curve[i][0]<=curve[i-1][0])||(i&&b[i*5]<=b[(i-1)*5]))return GH_ID_FORMAT;
 if(curve[0][0]!=0||curve[4][0]!=16||b[0]!=0||b[20]!=16)return GH_ID_UNSUPPORTED;
 float shake,origin_y;memcpy(&shake,life,4);memcpy(&origin_y,origin,4);if(!isfinite(shake)||!isfinite(origin_y))return GH_ID_FORMAT;
 s->life_shake=shake;s->life_origin_y=origin_y;s->life_origin_known=u32(origin+4);s->life_level_counter=life[4];s->life_level_direction=life[5];
 memcpy(s->radar_uv,uv,sizeof(s->radar_uv));
 memcpy(s->radar_curve,curve,40);memcpy(s->radar_color_curve,b,25);s->radar_pulse=*p;s->frame_bound=1;return GH_ID_OK;
}
static gh_id_error frame_move_base(gh_game_cockpit*s,unsigned o,const gh_cockpit_frame_callbacks*c){gh_id_frontier f={0};
 gh_id_error e=gh_id_jobs_move_reference(base(s,o),c?c->post:NULL,c?c->post_context:NULL,&room_animation_profile,&f);if(e)s->frontier=f.address;return e;
}
static float frame_number(const uint8_t*p){uint32_t n=u32(p);float f;memcpy(&f,&n,4);return f;}
gh_id_error gh_game_cockpit_bind_gameplay(gh_game_cockpit*s,const gh_cockpit_gameplay_bindings*b){
 if(!s||!b||!b->init||!b->pools||b->player_known>1||(b->player&&(!b->player->bytes||b->player->size<0x300)))return GH_ID_ARGUMENT;
 if(s->gameplay_bound){const gh_cockpit_gameplay_bindings*a=&s->gameplay;int same=a->init==b->init&&a->pools==b->pools&&a->resolve_handle==b->resolve_handle&&a->context==b->context;if(!same)return GH_ID_BUSY;
  if(a->camera!=b->camera&&(s->update_stage||s->draw_stage||!b->camera||b->camera->identity!=b->init->active_camera))return GH_ID_BUSY;
  if(a->player_known&&(a->player_known!=b->player_known||a->player!=b->player))return GH_ID_BUSY;if(!a->player_known&&s->radar_move_stage)return GH_ID_BUSY;}
 s->gameplay=*b;s->gameplay_bound=1;return GH_ID_OK;
}
static gh_id_error gameplay_move_base(gh_game_cockpit*s,unsigned o,const gh_cockpit_frame_callbacks*c){
 if(s->gameplay_error){s->frontier=s->gameplay_error_frontier;return s->gameplay_error;}
 gh_id_error e=frame_move_base(s,o,c);if(e){s->gameplay_error=e;s->gameplay_error_frontier=s->frontier;}return e;
}
/* 143D20 uses an identity-seeded SDK Z,Y,X matrix and multiplies the
 * horizontal displacement as a column vector. Keep the original float
 * operation order under the surrounding binary32 reference environment. */
static gh_id_error radar_add_marker(gh_game_cockpit *s,const gh_title_player_view *actor)
{
 const unsigned o=0x6e0;
 const gh_title_player_view *player=s->radar_player;
 const unsigned code=actor->bytes[0x531];
 if(code==255)return GH_ID_OK;
 if(!player||!player->alias_f0||!actor->alias_f0)return frame_stop(s,0x2db6b0);
 if(actor->size<0x568)return GH_ID_BOUNDS;
 float dx=add(frame_number(actor->alias_f0),-frame_number(player->alias_f0));
 float dz=add(frame_number(actor->alias_f0+8),-frame_number(player->alias_f0+8));
 float limit=frame_number(s->fields+o+0x5d0);
 if(!isfinite(dx)||!isfinite(dz)||!isfinite(limit)||limit<=3)return frame_stop(s,0x143e4c);
 int special=code==5||code==6||code==1||code==4;
 if(!special&&(fabsf(dx)>limit||fabsf(dz)>limit))return GH_ID_OK;
 float distance=sqrtf(add(mul(dx,dx),mul(dz,dz)));
 if(!isfinite(distance))return frame_stop(s,0x1f7df8);
 if(!special&&distance>limit)return GH_ID_OK;
 float angle[3],matrix[16];
 for(unsigned i=0;i<3;i++)angle[i]=-frame_number(player->bytes+0x100+i*4);
 uint32_t frontier=0;
 if(gh_actor_matrix_rotation_00147728(angle,5,matrix,GH_STATIC_BINARY32_REFERENCE,&frontier))
  return frame_stop(s,frontier==0x147728?0x30a870:frontier);
 float v[3];
 for(unsigned i=0;i<3;i++)v[i]=add(mul(matrix[i],dx),mul(matrix[8+i],dz));
 float shifted=add(v[2],-6.0f);
 distance=sqrtf(add(add(mul(v[0],v[0]),mul(v[1],v[1])),mul(shifted,shifted)));
 if(!isfinite(distance))return frame_stop(s,0x1f7df8);
 float range=add(limit,-3.0f);
 unsigned record=o+0x9c+s->radar_marker_count*16;
 if(range<distance){
  if(!special)return GH_ID_OK;
  float length=sqrtf(add(add(mul(v[0],v[0]),mul(v[1],v[1])),mul(v[2],v[2])));
  if(!isfinite(length)||length==0)return frame_stop(s,0x30a370);
  volatile float inverse=1.0f/length;
  float heading;
  if(gh_game_ee_acos_reference_001f8140(mul(v[2],inverse),&heading,GH_CAMERA_NATIVE_BINARY32_REFERENCE,&frontier))
   return frame_stop(s,frontier?frontier:0x1f8140);
  int right=code==4||code==1;
  gh_id_entry *entry;
  TRY(gh_game_cockpit_entry_at(s,o+(right?0x5a8:0x5a4),&entry));
  entry->flags&=~0x8000000u;
  wf(s,record,v[0]>0?-heading:heading);
  s->fields[record+12]=(uint8_t)(right?9:8);
 }else{
  float height=fabsf(add(frame_number(actor->alias_f0+4),-frame_number(player->alias_f0+4)));
  if(!isfinite(height))return frame_stop(s,0x144208);
  volatile float inverse=1.0f/range;
  unsigned alpha=128;
  if(height>=3.0f)alpha=5;
  else if(height>0.5f){
   volatile float fraction=add(height,-0.5f)/2.5f;
   alpha=(unsigned)(int)mul(add(1.0f,-fraction),122.0f)+5;
  }
  if(u32(actor->bytes+0x564)==0x26a)alpha=128;
  wf(s,record,add(-mul(mul(v[0],inverse),38.0f),455.0f));
  wf(s,record+4,add(-mul(mul(v[2],inverse),38.0f),65.0f));
  w32(s,record+8,(alpha<<24)|0x808080);
  s->fields[record+12]=(uint8_t)code;
 }
 s->radar_marker_count++;
 return GH_ID_OK;
}
static gh_id_error gameplay_radar_body(gh_game_cockpit*s,const gh_cockpit_frame_callbacks*c){
 const unsigned o=0x6e0;const gh_cockpit_gameplay_bindings*b=&s->gameplay;gh_id_entry*entry;
 if(!s->radar_move_stage){if(!s->gameplay_bound||!b->player_known)return frame_stop(s,0x2db6b0);s->radar_player=b->player;s->radar_marker_count=0;TRY(gh_game_cockpit_entry_at(s,o+0x5a4,&entry));entry->flags|=0x8000000;TRY(gh_game_cockpit_entry_at(s,o+0x5a8,&entry));entry->flags|=0x8000000;s->radar_move_stage=1;}
 if(s->radar_move_stage==1){const gh_room_vector*v;if(gh_room_registry_view(b->pools,0x754c58,&v)||(!v->allocation.pointer&&v->count))return frame_stop(s,0x143d20);
  while(s->radar_move_index<v->count&&s->radar_marker_count<64){if(!b->resolve_handle)return frame_stop(s,0x143d20);uint32_t handle=u32((const uint8_t*)v->allocation.pointer+(size_t)s->radar_move_index*4),f=0;gh_title_player_view actor;gh_id_error e=b->resolve_handle(b->context,handle,&actor,&f);if(e){s->frontier=f?f:0x143d20;return e;}if(!actor.bytes||actor.size<0x300)return GH_ID_ARGUMENT;unsigned k=actor.bytes[0x2fe]|(unsigned)actor.bytes[0x2ff]<<8;
   if(k>=0x200&&k<0x500){if(actor.size<0x532)return GH_ID_BOUNDS;TRY(radar_add_marker(s,&actor));}
   s->radar_move_index++;
  }w32(s,o+0x49c,s->radar_marker_count);TRY(gh_game_cockpit_entry_at(s,o+0x4a4,&entry));entry->flags|=0x8000000;s->radar_move_stage=2;
 }
 if(s->radar_move_stage==2){TRY(gameplay_move_base(s,o,c));s->radar_move_stage=3;}
 if(s->radar_move_stage==3){TRY(gameplay_move_base(s,o+0x4b0,c));s->radar_move_stage=4;}
 if(s->radar_move_stage==4){gh_id_error e=gh_id_tick_entries_002aaba0(base(s,o+0x500));if(e){s->frontier=0x2aaba0;return e;}s->radar_move_stage=5;}
 if(s->radar_move_stage==5){gh_id_error e=gh_id_tick_entries_002aaba0(base(s,o+0x550));if(e){s->frontier=0x2aaba0;return e;}s->radar_move_stage=6;}
 return GH_ID_OK;
}
static gh_id_error gameplay_radar(gh_game_cockpit*s,const gh_cockpit_frame_callbacks*c){fenv_t env;if(feholdexcept(&env))return GH_ID_UNSUPPORTED;if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return GH_ID_UNSUPPORTED;}gh_id_error e=gameplay_radar_body(s,c);if(fesetenv(&env))return GH_ID_UNSUPPORTED;return e;}
static gh_id_error heat_entry(gh_game_cockpit*s,unsigned field,gh_id_entry**out){return gh_game_cockpit_entry_at(s,0x460+field,out);}
/* Actual readiness query stores its result back in the same Heat owner. */
static int heat_ready_002a9bc0(uint8_t *heat,float current,float threshold)
{
 uint32_t ready=(uint32_t)(!(current<threshold));
 memcpy(heat+0x14,&ready,4);
 return (int)ready;
}

static gh_id_error heat_visual(gh_game_cockpit *s,uint8_t *heat,float ratio,float smoothed,
 float release_alpha)
{
 const unsigned o=0x460;
 const unsigned releasing=heat[0x10]!=0;
 const uint32_t flags=u32(s->fields+o+0x90);
 gh_id_entry *e;
 unsigned alpha=s->fields[o+0x26e];
 if(!releasing){
  if(s->fields[o+0x26f]==1){
   alpha=(alpha+3)&255;
   if(alpha>76){alpha=76;s->fields[o+0x26f]=0;}
  }else if(alpha<3){alpha=0;s->fields[o+0x26f]=1;}
  else alpha-=3;
  s->fields[o+0x26e]=(uint8_t)alpha;
 }
 TRY(heat_entry(s,0xc4,&e));
 if(frame_number(s->fields+o+0x264)==ratio||!(e->flags&0x8000000)){
  if(!alpha)e->flags|=0x8000000;
 }else{
  e->flags&=~0x8000000u;
  e->scale_x=1;
  s->fields[o+0x26f]=1;
  s->fields[o+0x26e]=0;
 }
 wf(s,o+0x264,ratio);wf(s,o+0x268,smoothed);
 if(add(ratio,-smoothed)<.01f&&(flags&0x2000000))
  w32(s,o+0x90,flags&0xfcffffffu); /* Actual stopped handle is zero here. */
 gh_id_entry *bar,*flash,*fill;
 TRY(heat_entry(s,0xa4,&bar));bar->scale_x=smoothed;
 TRY(heat_entry(s,0xc0,&flash));flash->flags|=0x8000000;
 TRY(heat_entry(s,0xc4,&fill));
 fill->color_base=(fill->color_base&0xffffff)|(uint32_t)s->fields[o+0x26e]<<24;
 if(releasing){
  fill->flags&=~0x8000000u;flash->flags&=~0x8000000u;
  bar->scale_x=fill->scale_x=ratio;
  fill->color_base=(fill->color_base&0xffffff)|((uint32_t)(int32_t)release_alpha&255u)<<24;
  return GH_ID_OK;
 }
 const float current=frame_number(heat+4),threshold=frame_number(heat+0xc);
 if(bar->scale_x>.999f&&heat_ready_002a9bc0(heat,current,threshold)){
  flash->flags&=~0x8000000u;
  return GH_ID_OK;
 }
 TRY(heat_entry(s,0xbc,&e));
 if(current>=240.0f&&heat_ready_002a9bc0(heat,current,threshold))
  e->color_base=0x80808080;
 else if(current>=120.0f&&heat_ready_002a9bc0(heat,current,threshold))
  e->color_base=0x804e6060;
 else e->color_base=0x80744042;
 return GH_ID_OK;
}
static gh_id_error gameplay_heat_body(gh_game_cockpit*s,const gh_boot_work*work_state,const gh_cockpit_frame_callbacks*c){
 const unsigned o=0x460;gh_id_entry*e;const uint8_t*w=(const uint8_t*)work_state;
 if(s->gameplay_error){s->frontier=s->gameplay_error_frontier;return s->gameplay_error;}
 if(!s->heat_move_stage){
  if(!c||!c->save.bytes||c->save.size<GH_CORE_SAVE_BYTES)return frame_stop(s,0x1fac68);
  if(!s->gameplay_bound)return frame_stop(s,0x2a9c98);
  if(!s->frame_bound)return frame_stop(s,0x142060);
  gh_core_save_view save=c->save;uint8_t*h=s->gameplay.init->heat;
  int32_t life,maximum;uint32_t raw=u32(save.bytes+0x84);memcpy(&life,&raw,4);raw=u32(save.bytes+0x80);memcpy(&maximum,&raw,4);
  float heatmax=frame_number(h),current=frame_number(h+4),heat_ratio,release_alpha=0;
  if(!heat_ratio_002a9c98(h,&heat_ratio))return frame_stop(s,0x2a9c98);
  if(h[0x10]){
   float baseline=frame_number(h+8);
   volatile float fraction=add(current,-baseline)/add(heatmax,-baseline);
   release_alpha=mul(fraction,128.0f);
   if(!isfinite(release_alpha)||release_alpha< -2147483648.0f||release_alpha>=2147483648.0f)
    return frame_stop(s,0x2a9d28);
  }
  if(life < -16777216||life>16777216||maximum<=0||maximum>16777216)return frame_stop(s,0x1fac80);
  unsigned upgrades=0;for(unsigned i=0;i<6&&i<save.bytes[0x156];i++)if(save.bytes[0xa8+i])upgrades++;
  int lev=level(save);if(lev==4)lev=3;else if(lev==5)lev=4;lev--;
  unsigned oldlev=s->fields[o+0x17c]|(unsigned)s->fields[o+0x17d]<<8;
  if(lev<0||lev>=3||oldlev!=(unsigned)lev||s->life_level_counter||upgrades<s->fields[o+0x260])return frame_stop(s,0x142060);
  uint32_t heat_flags=u32(s->fields+o+0x90);
  float smoothed=heat_flags&0x40000000?frame_number(s->fields+o+0x268):heat_ratio;
  smoothed=add(smoothed,-mul(add(smoothed,-heat_ratio),.1f));
  float gap=add(heat_ratio,-smoothed);
  if(!isfinite(smoothed)||!isfinite(gap))return frame_stop(s,0x2a9c98);
  if(!h[0x10]&&(smoothed>.999f||current>=120.0f)&&!isfinite(frame_number(h+0xc)))
   return frame_stop(s,0x2a9bc0);
  /* No native start/stop callback has been bound for the original HUD loop. */
  if(gap>=.05f&&(heat_flags&0x3000000)==0x1000000)return frame_stop(s,0x2cb8a0);
  if(gap<.01f&&(heat_flags&0x2000000)&&u32(s->fields+o+0x270))return frame_stop(s,0x2cbba0);
  float ratio=(float)life/(float)maximum;
  s->heat_move_stage=10;/* A failed reset/calc cannot replay earlier mutations. */
  if(!s->life_origin_known){TRY(heat_entry(s,0xa8,&e));s->life_origin_y=e->pos_y;s->life_origin_known=1;}
  if(!(u32(s->fields+o+0x90)&0x40000000)){
   w32(s,o+0x94,u32(save.bytes+0x84));wf(s,o+0x264,heat_ratio);wf(s,o+0x268,heat_ratio);w32(s,o+0x90,u32(s->fields+o+0x90)|0x40000000);
   static const unsigned hidden[]={0xa0,0xb4,0xac,0xb0};for(unsigned i=0;i<4;i++){TRY(heat_entry(s,hidden[i],&e));e->flags|=0x8000000;}
   static const unsigned warnings[]={0x1ec,0x1f0,0x1f4};const float threshold[]={.8f,.5f,.25f};for(unsigned i=0;i<3;i++){TRY(heat_entry(s,warnings[i],&e));e->color_base=(e->color_base&0xffffff)|0x80000000;if(ratio<threshold[i])e->flags&=~0x8000000u;else e->flags|=0x8000000;}
   TRY(heat_entry(s,0xa4,&e));e->flags|=0x400;w32(s,o+0x90,u32(s->fields+o+0x90)|0x1c000000);
  }
  int32_t oldlife;raw=u32(s->fields+o+0x94);memcpy(&oldlife,&raw,4);
  if(life!=oldlife){
   s->fields[o+0x1ac]=1;s->life_shake=1;
   gh_id_entry*bar;TRY(heat_entry(s,0x9c,&bar));TRY(heat_entry(s,0xa0,&e));e->scale_x=bar->scale_x;TRY(heat_entry(s,0xb4,&e));e->scale_x=bar->scale_x;
   unsigned changed[]={2,7,life<oldlife?5u:17u,6};
   for(unsigned i=0;i<4;i++){TRY(heat_entry(s,0x98+changed[i]*4,&e));e->flags&=~0x8000000u;}
   for(unsigned i=0;i<4;i++)TRY(reset_family(s,o,(int)changed[i],0));
  }
  TRY(heat_entry(s,0xa8,&e));e->pos_y=add(s->life_origin_y,mul(s->life_shake,5));s->life_shake=mul(s->life_shake,-.8f);
  gh_id_entry*bar;TRY(heat_entry(s,0x9c,&bar));bar->scale_x=ratio;TRY(heat_entry(s,0xb8,&e));
  if(life<oldlife){e->pos_x=add(add(bar->draw_x,mul(bar->f68,bar->draw_scale_x)),-8);TRY(reset_family(s,o,8,0));TRY(heat_entry(s,0xb8,&e));e->flags&=~0x8000000u;}
  else if((e->flags&0x400)||life==oldlife)e->flags|=0x8000000;
  TRY(heat_visual(s,h,heat_ratio,smoothed,release_alpha));
  unsigned old=s->fields[o+0x260];for(unsigned i=0;i<6;i++){
   TRY(heat_entry(s,0x130+i*4,&e));if(i<old)e->flags&=~0x8000000u;else e->flags|=0x8000000;
   /* Original 1FAEE0 is called inside this loop, after 1FABB0 above. */
   if(u32(w+0x598)&0x8000000)save.bytes[0x156]=6;
   TRY(heat_entry(s,0x130+(i+7)*4,&e));if(i<save.bytes[0x156])e->flags&=~0x8000000u;else e->flags|=0x8000000;
  }s->fields[o+0x260]=(uint8_t)upgrades;
  if(u32(s->fields+o+0x90)&0x20000000){TRY(reset_family(s,o,lev+12,0));TRY(heat_entry(s,0x98+(unsigned)(lev+12)*4,&e));e->flags|=0x400;}
  TRY(heat_entry(s,0x1f8,&e));e->seed=(uint32_t)lev+1;e->flags&=~0x8000000u;TRY(heat_entry(s,0x1e4,&e));e->flags|=0x8000000;
  /* Actual 1FACB0 mutates Save.maximum, not the player's HP/profile. */
  if(u32(w+0x598)&0x8000000)save.bytes[0x154]=13;uint32_t maxlife=100u+20u*save.bytes[0x154];memcpy(save.bytes+0x80,&maxlife,4);
  TRY(heat_entry(s,0xa8,&e));e->scale_x=add(mul(add((float)maxlife,-100)/260,2.6f),1);
  TRY(heat_entry(s,0xd8,&e));e->scale_x=add(mul(add(heatmax,-120)/150,1.25f),1);w32(s,o+0x94,u32(save.bytes+0x84));s->heat_move_stage=1;
 }
 static const unsigned ordered[]={0x460,0x540,0x5f0,0x670};while(s->heat_move_stage<=4){TRY(gameplay_move_base(s,ordered[s->heat_move_stage-1],c));s->heat_move_stage++;}
 w32(s,o+0x90,u32(s->fields+o+0x90)&0xfeffffff);s->heat_move_stage=0;return GH_ID_OK;
}
gh_id_error gh_game_cockpit_heat_move_00142060(gh_game_cockpit*s,const gh_boot_work*w,const gh_cockpit_frame_callbacks*c){
 if(!s||!w||!s->room_ready)return GH_ID_ARGUMENT;fenv_t env;if(feholdexcept(&env))return GH_ID_UNSUPPORTED;if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return GH_ID_UNSUPPORTED;}
 gh_id_error e=gameplay_heat_body(s,w,c);if(e&&s->heat_move_stage==10){s->gameplay_error=e;s->gameplay_error_frontier=s->frontier;}if(fesetenv(&env))return GH_ID_UNSUPPORTED;return e;
}
static gh_id_error widget_moves(gh_game_cockpit*s,const gh_cockpit_frame_callbacks*c,const unsigned*list,unsigned count,int tick){
 if(!s->widget_step)s->widget_step=1;
 while(s->widget_step<=count){unsigned o=list[s->widget_step-1];if(tick){gh_id_error e=gh_id_tick_entries_002aaba0(base(s,o));if(e){s->frontier=0x2aaba0;return e;}}else TRY(gameplay_move_base(s,o,c));s->widget_step++;}
 s->widget_step=0;return GH_ID_OK;
}
static gh_id_error gameplay_widget_body(gh_game_cockpit*s,unsigned index,const gh_boot_work*work_state,const gh_cockpit_frame_callbacks*c){
 const unsigned o=registry[index].offset;const uint8_t*w=(const uint8_t*)work_state;gh_id_entry*e;unsigned list[5];
 if(s->gameplay_error){s->frontier=s->gameplay_error_frontier;return s->gameplay_error;}
 switch(index){
 case 2:{/*145238 ->145410 coldstate0. No request means no144C90. */
  unsigned state=s->fields[o+0x41e]|(unsigned)s->fields[o+0x41f]<<8;
  if(state>=1&&state<=4){static const uint32_t calls[]={0,0x145460,0x145810,0x145fc8,0x146190};return frame_stop(s,calls[state]);}
  if(state==0&&(u32(s->fields+o+0x90)&0x40000000))return frame_stop(s,0x144c90);
  for(unsigned i=0;i<5;i++)list[i]=o+0x100+i*0x50;return widget_moves(s,c,list,5,0);
 }
 case 3:/*13D100: five originalcounter advances, no wholeIDMove. */
  list[0]=o;list[1]=o+0x140;list[2]=o+0x190;list[3]=o+0xa0;list[4]=o+0xf0;return widget_moves(s,c,list,5,1);
 case 4:/*13BD40 explicitly disabled by bit0; no IDMove in this branch. */
  return (s->fields[o+0x94]&1)?frame_stop(s,0x13bd40):GH_ID_OK;
 case 5:/*13C318 four one-frame high-bit indicators, then clear requests. */
  if(!s->widget_step){uint32_t request=u32(s->fields+o+0x94);for(unsigned i=0;i<4;i++){TRY(gh_game_cockpit_entry_at(s,o+0x98+i*4,&e));if(request&(UINT32_C(0x80000000)>>i))e->flags&=~0x8000000u;else e->flags|=0x8000000;}w32(s,o+0x94,request&0xfffffff);}
  list[0]=o;return widget_moves(s,c,list,1,0);
 case 6:/*13F1C0's comparison is fullword, source value copied ashalf. */
  if(!s->widget_step&&u32(s->fields+o+0x9c)!=0xffff){uint16_t message=(uint16_t)u32(s->fields+o+0x9c);TRY(work(s,o,0,0,&e));e->message=message;TRY(work(s,o,1,0,&e));e->message=message;}
  list[0]=o;return widget_moves(s,c,list,1,0);
 case 7:/*13E0B0 all eight signedcounters-1 genuinely skip work. */
  for(unsigned i=0;i<8;i++)if(s->fields[o+0x33c+i]!=255)return frame_stop(s,0x13e0b0);return GH_ID_OK;
 case 8:/*146A18 coldstate0, no queueditem: clearactual104bit0. */
  if(s->fields[o+0x79]||(s->fields[o+0x100]|s->fields[o+0x101]))return frame_stop(s,0x146a18);s->fields[o+0x104]&=0xfe;return GH_ID_OK;
 case 9:/*13E4F0 cache9C is a nativeentry, never inferNULL fromraw0. */
  if((u32(w+0x584)&8)||(u32(w+0x58c)&0x80000000))return GH_ID_OK;
  if(!s->widget_step){if(!c||!c->save.bytes||c->save.size<GH_CORE_SAVE_BYTES)return frame_stop(s,0x1fa950);
   if(u32(s->fields+o+0x90))return frame_stop(s,0x13e4f0);
   for(unsigned i=0;i<2;i++){uint32_t value=u32(c->save.bytes+0x68+i*4);TRY(gh_game_cockpit_entry_at(s,o+0x9c+i*4,&e));if(!(value&0x80000000)&&value){e->flags&=~0x8000000u;e->seed=value;}else e->flags|=0x8000000;}
  }list[0]=o;return widget_moves(s,c,list,1,0);
 case 10:/*143670 no pendingmoney particles. Original display stillticks. */
  if(!s->widget_step){if(!c||!c->save.bytes||c->save.size<GH_CORE_SAVE_BYTES)return frame_stop(s,0x1fa710);uint8_t*save=c->save.bytes;
   for(unsigned i=0;i<16;i++)if(s->fields[o+0x1b0+i]==1)return frame_stop(s,0x143670);
   uint32_t money=u32(save+0x20);if(u32(w+0x594)&0x2000000)money=999999;
   if((money!=u32(s->fields+o+0x90)&&u32(save+0x24))||((s->fields[o+0x1c0]&3)==1))return frame_stop(s,0x143670);
   TRY(gh_game_cockpit_entry_at(s,o+0x9c,&e));e->flags|=0x8000000;TRY(gh_game_cockpit_entry_at(s,o+0x94,&e));
   uint32_t target=u32(s->fields+o+0x90),seed=e->seed;if(seed+1000u<target)seed+=1000;else if(seed+100u<target)seed+=100;else if(seed+10u<target)seed+=10;else seed++;if(target<seed)seed=target;e->seed=seed;
   /*1FA710 actualcheat write;1FA820 clears rewardcount after loop. */
   memcpy(save+0x20,&money,4);w32(s,o+0x90,money);memset(save+0x24,0,4);s->fields[o+0x1c0]&=0xfc;
  }list[0]=o;return widget_moves(s,c,list,1,0);
 default:return GH_ID_ARGUMENT;
 }
}
static gh_id_error gameplay_widget(gh_game_cockpit*s,unsigned i,const gh_boot_work*w,const gh_cockpit_frame_callbacks*c){
 fenv_t env;if(feholdexcept(&env))return GH_ID_UNSUPPORTED;if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return GH_ID_UNSUPPORTED;}gh_id_error e=gameplay_widget_body(s,i,w,c);if(fesetenv(&env))return GH_ID_UNSUPPORTED;return e;
}
gh_id_error gh_game_cockpit_move_00140e58(gh_game_cockpit*s,const gh_boot_work*work_state,const gh_cockpit_frame_callbacks*c){
 if(!s||!s->ready||!s->room_ready||!work_state)return GH_ID_ARGUMENT;const uint8_t*w=(const uint8_t*)work_state;s->frontier=0;
 if(!s->update_stage){if(u32(w+0x58c)&0x80000000)return GH_ID_OK;
  if(!(u32(w+0x5d8)&0x40000)){s->update_stage=10;}
  else{if(u32(s->fields+0x15bc)!=0xffff){gh_id_entry*a,*b;TRY(gh_game_cockpit_entry_at(s,0x15b0,&a));TRY(gh_game_cockpit_entry_at(s,0x15b4,&b));uint32_t n=u32(s->fields+0x15bc)&0xffff;a->seed=(a->seed&0xffff0000)|n;b->seed=(b->seed&0xffff0000)|n;}s->update_stage=1;}
 }
 if(s->update_stage==10){/* Actual13C808 constructor-established resourceNULL. */if(!(s->hud_cold_known&1)||u32(s->fields+0x1dc0+0x94)||base(s,0x1dc0)->owns_run)return frame_stop(s,0x13c950);s->update_stage=11;}
 if(s->update_stage==11){/*13F5D0 global guards precede real140860 cleanup. */if(!(u32(w+0x584)&8)&&!(u32(w+0x58c)&0x80000000)){if(!(s->hud_cold_known&2)||s->fields[0x1e90+0x358])return frame_stop(s,0x140860);if(u32(s->fields+0x1e90+0xa0))return frame_stop(s,0x13f5d0);}s->update_stage=12;}
 if(s->update_stage==12){if(!(s->hud_cold_known&4)||u32(s->fields+0x2220+0x94)||base(s,0x2220)->owns_run)return frame_stop(s,0x13e970);s->update_stage=13;}
 if(s->update_stage==13){if(u32(s->fields+0x1a20+0xb0)){unsigned state=s->fields[0x1a20+0xb8];if(state>=1&&state<=5)return frame_stop(s,0x1471f0);TRY(gameplay_move_base(s,0x1a20,c));}s->update_stage=14;}
 if(s->update_stage==14){/* First true registeredwidget,unlessoriginal77 gate. */if(!s->fields[0x6e0+0x77])TRY(gameplay_radar(s,c));s->gameplay_index=1;s->update_stage=15;}
 if(s->update_stage==15){while(s->gameplay_index<11){unsigned i=s->gameplay_index;if(!s->fields[registry[i].offset+0x77]){if(i==1)TRY(gh_game_cockpit_heat_move_00142060(s,work_state,c));else TRY(gameplay_widget(s,i,work_state,c));}s->gameplay_index++;}s->update_stage=2;}
 if(s->update_stage==1){TRY(frame_move_base(s,0x1520,c));s->update_stage=2;}
 if(s->update_stage==2){int a=u32(s->fields+0x1e5c)&&(u32(s->fields+0x1e60)&0x20000000);int b=s->fields[0x21e8]&&(u32(s->fields+0x1f20)&0x80000000);int d=u32(s->fields+0x22bc)&&(u32(s->fields+0x22c0)&0x20000000);if(a||b||d)s->fields[0x1ca0]|=2;if(a||b)s->fields[0x1db4]|=1;else if(d)s->fields[0x1db4]|=2;s->update_stage=3;}
 if(u32(w+0x584)&0x20000){if(!c||!c->set_draw_order)return frame_stop(s,0x2c7850);uint32_t f=0;gh_id_error e=c->set_draw_order(c->context,6,5,&f);if(e){s->frontier=f?f:0x2c7850;return e;}}
 s->update_stage=s->gameplay_index=s->radar_move_stage=s->radar_move_index=0;s->gameplay_error=GH_ID_OK;s->gameplay_error_frontier=0;return GH_ID_OK;
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
static gh_id_error radar_make_quad(const gh_game_cockpit *s,unsigned record,
 unsigned code,gh_cockpit_radar_quad *quad)
{
 fenv_t env;
 if(feholdexcept(&env))return GH_ID_UNSUPPORTED;
 if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return GH_ID_UNSUPPORTED;}
 const unsigned o=0x6e0;
 float size=code==4?10.0f:code==5?3.5f:code==6?4.3f:2.0f;
 uint32_t rgba=u32(s->fields+record+8);
 if(code==1){
  size=mul(frame_number(s->fields+o+0x5d4),7.0f);
  rgba=u32(s->fields+o+0x5d8);
 }else if(code==4)rgba=((uint32_t)s->radar_pulse<<24)|0x808080;
 float x=frame_number(s->fields+record),y=frame_number(s->fields+record+4);
 gh_cockpit_radar_quad out={.texture=s->icon_texture,
  .xy={add(x,-size),add(y,-size),add(x,size),add(y,size)},.rgba=rgba};
 memcpy(out.uv,s->radar_uv[code],sizeof(out.uv));
 if(fesetenv(&env))return GH_ID_UNSUPPORTED;
 *quad=out;
 return GH_ID_OK;
}

static gh_id_error radar_draw_markers(gh_game_cockpit *s,const gh_cockpit_frame_callbacks *c)
{
 const unsigned o=0x6e0;
 unsigned count=u32(s->fields+o+0x49c);
 if(count>64)return frame_stop(s,0x144368);
 if(!s->radar_draw_pass)s->radar_draw_pass=1;
 while(s->radar_draw_pass<7){
  while(s->radar_draw_index<count){
   unsigned record=o+0x9c+s->radar_draw_index*16;
   unsigned code=s->fields[record+12];
   if(code!=s->radar_draw_pass&&code!=8&&code!=9){
    if(code>=7||!s->icon_texture.bytes||!c||!c->radar)return frame_stop(s,0x15bcf0);
    gh_cockpit_radar_quad quad;
    gh_id_error e=radar_make_quad(s,record,code,&quad);
    if(e)return frame_stop(s,0x144888);
    uint32_t frontier=0;
    e=c->radar(c->context,&quad,&frontier);
    if(e){s->frontier=frontier?frontier:0x15bcf0;return e;}
    if(code==4){s->fields[record+12]=3;continue;}
   }
   s->radar_draw_index++;
  }
  s->radar_draw_pass++;
  s->radar_draw_index=0;
 }
 while(s->radar_draw_index<count){
  unsigned record=o+0x9c+s->radar_draw_index*16;
  unsigned code=s->fields[record+12];
  if(code==8||code==9){
   unsigned bo=o+(code==8?0x500:0x550);
   if(!s->radar_arrow_moved){
    gh_id_entry *entry;
    TRY(gh_game_cockpit_entry_at(s,o+(code==8?0x5a4:0x5a8),&entry));
    entry->rotation_speed=frame_number(s->fields+record);
    TRY(gameplay_move_base(s,bo,c));
    s->radar_arrow_moved=1;
   }
   TRY(frame_trans(s,bo,c));
   s->radar_arrow_moved=0;
  }
  s->radar_draw_index++;
 }
 return GH_ID_OK;
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
   if(s->radar_stage==2){TRY(radar_draw_markers(s,c));s->radar_stage=3;}
   TRY(frame_trans(s,0x6e0,c));s->radar_stage=0;
   s->radar_draw_pass=s->radar_draw_index=s->radar_arrow_moved=0;
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
 if(s->draw_stage==10){
  /* 13E6E0 draws its base only for a nonzero original stage, then the
   * auxiliary indicator when enabled. Keep those submissions separate. */
  if(s->fields[0x1910+0x78]||(u32(w+0x584)&8)||(u32(w+0x58c)&0x80000000u))s->draw_stage=12;
  else{if(w[0x5b1])TRY(frame_trans(s,0x1910,c));s->draw_stage=11;}
 }
 if(s->draw_stage==11){if(u32(s->fields+0x19a0))TRY(frame_trans(s,0x19c0,c));s->draw_stage=12;}
 if(s->draw_stage==12){if(!s->fields[0x1ae0+0x78]){TRY(frame_trans(s,0x1ae0,c));s->draw_stage=13;}else s->draw_stage=14;}
 if(s->draw_stage==13){gh_id_entry*e;TRY(work(s,0x1b90,0,0,&e));(void)e;for(unsigned i=0;i<16;i++)if(s->fields[0x1c90+i]==1)return frame_stop(s,0x143998);s->draw_stage=14;}
 if(s->draw_stage!=14)return frame_stop(s,0x1410a8);s->draw_stage=0;return GH_ID_OK;
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
 TRY(room_heat_reset_00141ce8(s,save,heat));s->gameplay_bound=0;s->radar_player=NULL;s->hud_cold_known=7;s->gameplay_index=s->radar_move_stage=s->radar_move_index=s->heat_move_stage=s->widget_step=0;s->gameplay_error=GH_ID_OK;s->gameplay_error_frontier=0;s->room_begun=s->room_ready=0;s->radar_draw_pass=s->radar_draw_index=s->radar_arrow_moved=0;s->update_stage=s->draw_stage=s->radar_stage=s->heat_stage=0;return GH_ID_OK;
}

gh_id_error gh_game_cockpit_action_view_001f7680(gh_game_cockpit*s,gh_cockpit_action_view*out){if(!s||!out||!s->ready)return GH_ID_ARGUMENT;*out=(gh_cockpit_action_view){s->fields+0x1414,s->fields+0x1415,s->fields+0x1420,s->fields+0x1424,s->fields+0x1464,s};return GH_ID_OK;}
