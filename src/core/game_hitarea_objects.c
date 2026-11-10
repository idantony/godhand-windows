#include "game_hitarea_objects.h"
#include <stdlib.h>
#include <string.h>
#include "game_hitarea_internal.h"
static uint32_t w(const uint8_t*p){uint32_t v;memcpy(&v,p,4);return v;}
static void ww(uint8_t*p,uint32_t v){memcpy(p,&v,4);}
static gh_hitarea_status fail(uint32_t*f,uint32_t pc,gh_hitarea_status e){if(f)*f=pc;return e;}
static int valid(const gh_hitarea_object*v){if(!v||!v->state||!v->bytes||v->generation!=v->state->hitarea_generation)return 0;gh_game_init_state*s=v->state;if(s->hitarea_native&&s->hitarea_native->owner!=s)return 0;if(!v->fallback&&(v->index>=416||(s->hitarea_native?s->hitarea_native->serials[v->index]:0)!=v->serial))return 0;
 if(v->group==1){if(v->fallback)return v->index==416&&v->bytes==s->object_pool+0xac00&&v->size==0x60;return v->index<160&&v->bytes==s->object_pool+(size_t)v->index*0x60&&v->size==0x60;}
 if(v->group==2){if(v->fallback)return v->index==417&&v->bytes==s->object_pool+0xac60&&v->size==0x50;return v->index>=160&&v->index<416&&v->bytes==s->object_pool+0x3c00+(size_t)(v->index-160)*0x70&&v->size==0x70;}return 0;}
gh_hitarea_status gh_hitarea_object_set_mode_001fd988(const gh_hitarea_object*v,uint8_t mode,uint32_t*f){
 if(f)*f=0;if(!valid(v))return GH_HITAREA_ARGUMENT;uint8_t*b=v->bytes;gh_game_damage_link*head=v->fallback?NULL:v->state->object_links[v->index];if(w(b+0x3c)&&!head)return fail(f,0x1fd988,GH_HITAREA_UNSUPPORTED);size_t guard=0;for(gh_game_damage_link*n=head;n;n=n->next){if(!n->target_mask||++guard>4096)return fail(f,0x1fd988,GH_HITAREA_UNSUPPORTED);}b[0x48]=mode;uint32_t type=mode<4?mode*2+(b[0x49]==1?1u:2u):0;for(gh_game_damage_link*n=head;n;n=n->next)*n->target_mask=type;return GH_HITAREA_OK;
}
static gh_hitarea_status create(gh_game_init_state*s,gh_title_player_view*actor,uint8_t mode,unsigned group,gh_hitarea_object*out,int*original,uint32_t*f){
 if(f)*f=0;if(!s||!out||!original||(actor&&(!actor->bytes||actor->size<0x214)))return GH_HITAREA_ARGUMENT;*original=0;if(s->hitarea_native&&s->hitarea_native->owner!=s)return GH_HITAREA_ARGUMENT;
 unsigned count=group==1?160:256,index=0;size_t stride=group==1?0x60:0x70,base=group==1?0:0x3c00;uint8_t*b=s->object_pool+base;while(index<count&&b[0x4a]){index++;b+=stride;}
 int fallback=index==count;if(fallback)b=s->object_pool+(group==1?0xac00:0xac60);unsigned slot=fallback?(group==1?416:417):index+(group==1?0:160);*out=(gh_hitarea_object){s,b,fallback?(group==1?0x60:0x50):stride,s->hitarea_generation,(uint16_t)slot,(uint8_t)group,(uint8_t)fallback,0};
 if(b[0x49]){if(fallback)return fail(f,group==1?0x1fe1e0:0x1fe310,GH_HITAREA_BOUNDS);if(!s->hitarea_native){s->hitarea_native=calloc(1,sizeof(*s->hitarea_native));if(!s->hitarea_native)return fail(f,0x1fd530,GH_HITAREA_MEMORY);s->hitarea_native->owner=s;}
  /* Validate unknownlive-chain before touching pointer/occupancy; nativeerror
   * does not falselycomplete original1FD988's requiredupdates. */
  gh_game_damage_link*head=s->object_links[slot];if(w(b+0x3c)&&!head)return fail(f,0x1fd988,GH_HITAREA_UNSUPPORTED);size_t guard=0;for(gh_game_damage_link*n=head;n;n=n->next)if(!n->target_mask||++guard>4096)return fail(f,0x1fd988,GH_HITAREA_UNSUPPORTED);
  if(s->hitarea_native->serials[slot]==UINT32_MAX)return fail(f,0x1fd530,GH_HITAREA_UNSUPPORTED);out->serial=++s->hitarea_native->serials[slot];s->hitarea_native->actors[slot]=actor;ww(b+0x30,0);if(group==2)b[0x4a]=1;else ww(b+0x34,0);gh_hitarea_status e=gh_hitarea_object_set_mode_001fd988(out,mode,f);if(e)return e;if(group==2)ww(b+0x34,0);else b[0x4a]=1;
 }
 *original=1;return GH_HITAREA_OK;
}
gh_hitarea_status gh_hitarea_object_create_001fe218(gh_game_init_state*s,gh_title_player_view*a,uint8_t mode,gh_hitarea_object*out,int*original,uint32_t*f){return create(s,a,mode,2,out,original,f);}
gh_hitarea_status gh_hitarea_object_create_001fe0e8(gh_game_init_state*s,gh_title_player_view*a,uint8_t mode,gh_hitarea_object*out,int*original,uint32_t*f){return create(s,a,mode,1,out,original,f);}
gh_hitarea_status gh_hitarea_object_actor(const gh_hitarea_object*v,gh_title_player_view**out){if(!out||!valid(v))return GH_HITAREA_ARGUMENT;*out=NULL;if(!v->fallback&&v->bytes[0x49]){if(!v->state->hitarea_native)return GH_HITAREA_ARGUMENT;*out=v->state->hitarea_native->actors[v->index];}return GH_HITAREA_OK;}
gh_hitarea_status gh_hitarea_object_release_empty_001fe278(gh_hitarea_object*v,uint32_t*f){
 if(f)*f=0;if(!valid(v))return GH_HITAREA_ARGUMENT;gh_game_init_state*s=v->state;if((!v->fallback&&s->object_links[v->index])||w(v->bytes+0x3c))return fail(f,0x1fd580,GH_HITAREA_UNSUPPORTED);
 if(s->hitarea_native&&!v->fallback){for(unsigned i=0;i<672;i++)if(s->hitarea_native->nodes[i].associated&&s->hitarea_native->nodes[i].object.index==v->index&&s->hitarea_native->nodes[i].object.generation==v->generation)return fail(f,0x1fd580,GH_HITAREA_UNSUPPORTED);if(s->hitarea_native->serials[v->index]==UINT32_MAX)return fail(f,0x1fe278,GH_HITAREA_UNSUPPORTED);}
 gh_game_damage_link*unused=NULL;gh_init_status e=gh_hitarea_clear_record_bytes(v->bytes,v->fallback?&unused:&s->object_links[v->index],v->group);if(e)return fail(f,0x1fcde8,GH_HITAREA_UNSUPPORTED);
 if(!v->fallback&&s->hitarea_native){s->hitarea_native->actors[v->index]=NULL;v->serial=++s->hitarea_native->serials[v->index];}return GH_HITAREA_OK;
}
void gh_hitarea_detach_after_pool_reset(gh_game_init_state*s){if(!s)return;if(s->hitarea_native&&s->hitarea_native->owner!=s)return;free(s->hitarea_native);s->hitarea_native=NULL;s->hitarea_generation++;}
