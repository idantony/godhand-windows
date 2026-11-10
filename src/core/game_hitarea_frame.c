#include "game_hitarea_frame.h"
#include "game_hitarea_internal.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
static uint32_t w(const uint8_t*p){uint32_t v;memcpy(&v,p,4);return v;}
static void ww(uint8_t*p,uint32_t v){memcpy(p,&v,4);}
static float fl(const uint8_t*p){float v;memcpy(&v,p,4);return v;}
static gh_hitarea_status fail(uint32_t*f,uint32_t pc,gh_hitarea_status e){if(f)*f=pc;return e;}
static uint8_t*raw(gh_game_init_state*s,unsigned i){return i<160?s->object_pool+(size_t)i*0x60:s->object_pool+0x3c00+(size_t)(i-160)*0x70;}
/* Equality lookup precedes every dereference of a possibly foreign link. */
static gh_hitarea_status find(gh_game_init_state*s,unsigned slot,gh_game_damage_link*link,unsigned*out,uint32_t*f){
 *out=0;if(!link)return GH_HITAREA_OK;if(!s->hitarea_native||s->hitarea_native->owner!=s)return fail(f,0x1fd770,GH_HITAREA_ARGUMENT);
 for(unsigned i=0;i<672;i++){gh_native_hitarea_node*n=&s->hitarea_native->nodes[i];if(&n->link!=link)continue;uint8_t*r=i<160?s->records_a[i]:s->records_b[i-160];
  if(!n->associated||n->record!=r||!r[0x2e]||n->object.state!=s||n->object.index!=slot||n->object.bytes!=raw(s,slot)||n->object.generation!=s->hitarea_generation||n->object.serial!=s->hitarea_native->serials[slot])return fail(f,0x1fd770,GH_HITAREA_ARGUMENT);
  const uint8_t*p;if(gh_collision_pair_shape_position(&n->shape,&p))return fail(f,0x1fc810,GH_HITAREA_ARGUMENT);*out=i+1;return GH_HITAREA_OK;}
 return fail(f,0x1fd770,GH_HITAREA_ARGUMENT);
}
static gh_hitarea_status prepare(gh_game_init_state*s,gh_hitarea_frame*c,uint32_t*f){
 gh_game_damage_link*l=s->object_links[c->slot];if(w(raw(s,c->slot)+0x3c)&&!l)return fail(f,0x1fceb0,GH_HITAREA_UNSUPPORTED);
 unsigned count=0,i;for(;l;){gh_hitarea_status e=find(s,c->slot,l,&i,f);if(e)return e;if(++count>672)return fail(f,0x1fd770,GH_HITAREA_BOUNDS);l=s->hitarea_native->nodes[i-1].link.next;}
 gh_hitarea_status e=find(s,c->slot,s->object_links[c->slot],&i,f);if(e)return e;c->node=i;c->visited=0;c->slot_started=1;c->query_done=0;c->slot_serial=s->hitarea_native?s->hitarea_native->serials[c->slot]:0;return GH_HITAREA_OK;
}
static gh_hitarea_status reset_b(gh_game_init_state*s,gh_hitarea_frame*c,const gh_boot_work*work,float rate,uint32_t*f){
 uint8_t*b=raw(s,c->slot);gh_title_player_view*a=s->hitarea_native?s->hitarea_native->actors[c->slot]:NULL;uint32_t flags=w((const uint8_t*)work+0x5d8);float next=0;int update=0;
 if(!(flags&0x80000000u)&&a){if(!a->bytes||a->size<0x5ac)return fail(f,0x1fd448,GH_HITAREA_BOUNDS);float old=fl(a->bytes+0x54c),speed=fl(a->bytes+0x5a8);if(!isfinite(old)||!isfinite(speed))return fail(f,0x1fd448,GH_HITAREA_UNSUPPORTED);if(old!=0){volatile float product=speed*rate;volatile float sub=old-product;next=sub;if(!isfinite(next))return fail(f,0x1fd448,GH_HITAREA_UNSUPPORTED);if(next<0)next=0;update=1;}}
 ww(b+0x34,0);ww(b+0x40,UINT32_MAX);b[0x44]=b[0x45]=0;ww(b+0x60,w(b+0x60)&0xfffffc40u);memset(b+0x10,0,12);if(update)memcpy(a->bytes+0x54c,&next,4);return GH_HITAREA_OK;
}
static gh_hitarea_status visit(gh_game_init_state*s,gh_hitarea_frame*c,uint32_t*f){
 while(c->node){if(c->node>672||c->visited>=672)return fail(f,0x1fd770,GH_HITAREA_BOUNDS);gh_native_hitarea_node*n=&s->hitarea_native->nodes[c->node-1];unsigned i;gh_hitarea_status e=find(s,c->slot,&n->link,&i,f);if(e)return e;
  if(c->phase==0){const uint8_t*p;if(gh_collision_pair_shape_position(&n->shape,&p))return fail(f,0x1fceb0,GH_HITAREA_ARGUMENT);memmove(n->record,p,12);memcpy(n->record+0x10,n->record,12);}
  else if(c->phase==2&&!c->query_done){gh_title_player_view*a=s->hitarea_native->actors[c->slot];if(a&&(!a->bytes||a->size<0x254))return fail(f,0x1fcf20,GH_HITAREA_BOUNDS);if(!a||!(w(a->bytes+0x250)&0x8000)){if((w(n->shape.record)&0x203)==3&&(int32_t)w(n->shape.record+0xd8)>0)return fail(f,0x1fc810,GH_HITAREA_UNSUPPORTED);}}
  else {uint16_t old;memcpy(&old,n->record+0x30,2);if(old){uint16_t value=(uint16_t)(old-1);memcpy(n->record+0x30,&value,2);if(old==1)ww(n->shape.record,w(n->shape.record)&~2u);}}
  e=find(s,c->slot,n->link.next,&i,f);if(e)return e;c->node=i;c->visited++;
 }return GH_HITAREA_OK;
}
gh_hitarea_status gh_hitarea_frame_resume_001fdfd8(gh_game_init_state*s,const gh_boot_work*work,float rate,gh_static_transform_policy policy,gh_hitarea_frame*c,uint32_t*f){
 if(f)*f=0;if(!s||!work||!c||!isfinite(rate)||policy!=GH_STATIC_BINARY32_REFERENCE)return GH_HITAREA_ARGUMENT;if(s->hitarea_native&&s->hitarea_native->owner!=s)return GH_HITAREA_ARGUMENT;
 if(c->active&&(c->cursor_identity!=c||c->identity!=s||c->generation!=s->hitarea_generation||c->phase>3||c->slot>=416))return GH_HITAREA_ARGUMENT;
 if(!c->active){uint32_t pause=w(s->object_pool+0xacd0);if(pause){ww(s->object_pool+0xacd0,pause-1);return GH_HITAREA_OK;}*c=(gh_hitarea_frame){.cursor_identity=c,.identity=s,.generation=s->hitarea_generation,.active=1};}
 fenv_t env;if(feholdexcept(&env))return fail(f,0x1fd448,GH_HITAREA_UNSUPPORTED);if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return fail(f,0x1fd448,GH_HITAREA_UNSUPPORTED);}gh_hitarea_status e=GH_HITAREA_OK;
 while(c->phase<4){unsigned end=(c->phase==0||c->phase==2)?160:416;while(c->slot<end){uint8_t*b=raw(s,c->slot);if(!b[0x4a]){if(c->slot_started){e=GH_HITAREA_ARGUMENT;goto done;}c->slot++;continue;}
   if(!c->slot_started){e=prepare(s,c,f);if(e)goto done;if(c->phase==0)ww(b+0x34,0);else if(c->phase==1){e=reset_b(s,c,work,rate,f);if(e){c->slot_started=0;goto done;}}}
   else if((s->hitarea_native?s->hitarea_native->serials[c->slot]:0)!=c->slot_serial){e=GH_HITAREA_ARGUMENT;goto done;}
   if(c->phase!=1){e=visit(s,c,f);if(e)goto done;if(c->phase==2&&!c->query_done){unsigned i;e=find(s,c->slot,s->object_links[c->slot],&i,f);if(e)goto done;c->node=i;c->visited=0;c->query_done=1;e=visit(s,c,f);if(e)goto done;}}
   c->slot++;c->slot_started=0;
  }c->phase++;c->slot=(c->phase==1||c->phase==3)?160:0;
 }*c=(gh_hitarea_frame){0};
 done:if(fesetenv(&env))return fail(f,0x1fd448,GH_HITAREA_UNSUPPORTED);return e;
}
