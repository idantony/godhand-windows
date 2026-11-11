/* Actual308BC0/307B28/3089F0, native continuation of original model hide.
 * Canonical registry handles replace only this producer's original pointers. */
#include "game_scene_visibility.h"
#include "game_actor_scroll_links.h"
#include "game_ray_query.h"
#include "game_scene_box_visibility.h"
#include <stdlib.h>
#include <string.h>
struct gh_scene_visibility {gh_scene_visibility_bindings b;const void*identity;
 uint32_t known_previous[32];unsigned phase,previous,index,registry_count,ray_index,report_count,report_index;
 unsigned static_row,actor_row,actor_index,dynamic_index,dynamic_count;
 gh_scene_model_ray rays[5];size_t ray_count;gh_scene_ray_report reports[8];const void*dynamic_objects[8];
 uint8_t bound,complete,registry_bound;};
static uint32_t word(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
static unsigned half(const uint8_t*p){return p[0]|(unsigned)p[1]<<8;}
static uint64_t key(const uint8_t*p){uint64_t n;memcpy(&n,p,8);return n;}
static void put(uint8_t*p,uint32_t n){memcpy(p,&n,4);}
static int eligible(unsigned k){return (k>0xff&&k<0x1ff)||(k>0x1ff&&k<0x600);}
static gh_scene_post_status fail(uint32_t*f,uint32_t pc,gh_scene_post_status e){if(f)*f=pc;return e;}
static int same(const gh_scene_visibility_bindings*a,const gh_scene_visibility_bindings*b){return a->scene.work==b->scene.work&&a->scene.pools==b->scene.pools&&a->scene.tables==b->scene.tables&&a->scene.init==b->scene.init&&a->scene.camera==b->scene.camera&&a->scene.player==b->scene.player&&a->scene.policy==b->scene.policy&&a->resolve_handle==b->resolve_handle&&a->mask_nodes==b->mask_nodes&&a->context==b->context;}
gh_scene_post_status gh_scene_visibility_create(gh_scene_visibility**out){if(!out)return GH_POST_ARGUMENT;gh_scene_visibility*s=calloc(1,sizeof(*s));if(!s)return GH_POST_POOLS;s->identity=s;*out=s;return GH_POST_OK;}
void gh_scene_visibility_destroy(gh_scene_visibility*s){free(s);}
gh_scene_post_status gh_scene_visibility_next_call(gh_scene_visibility*s){if(!s||s->identity!=s||!s->bound||!s->complete)return GH_POST_ARGUMENT;s->phase=s->previous=s->index=s->registry_count=s->ray_index=s->report_count=s->report_index=s->static_row=s->actor_row=s->actor_index=s->dynamic_index=s->dynamic_count=0;s->ray_count=0;s->complete=s->registry_bound=0;return GH_POST_OK;}
static gh_scene_post_status registry(gh_scene_visibility*s,const gh_room_vector**out,uint32_t*f){const gh_room_vector*v;if(gh_room_registry_view(s->b.scene.pools,0x754c58,&v)||(!v->allocation.pointer&&v->count))return fail(f,0x308bc0,GH_POST_POOLS);if(s->registry_bound&&s->registry_count!=v->count)return fail(f,0x308bc0,GH_POST_POOLS);s->registry_bound=1;s->registry_count=v->count;*out=v;return GH_POST_OK;}
static gh_scene_post_status actor(gh_scene_visibility*s,const gh_room_vector*v,unsigned i,gh_title_player_view*out,uint32_t*f){if(i>=v->count)return GH_POST_ARGUMENT;if(!s->b.resolve_handle)return fail(f,0x308bc0,GH_POST_UNSUPPORTED);uint32_t h=word((uint8_t*)v->allocation.pointer+(size_t)i*4),pc=0;gh_scene_post_status e=s->b.resolve_handle(s->b.context,h,out,&pc);if(e)return fail(f,pc?pc:0x308bc0,e);if(!out->bytes||out->size<0x300)return fail(f,0x308bc0,GH_POST_ARGUMENT);return GH_POST_OK;}
static gh_scene_post_status mask(gh_scene_visibility*s,uint32_t h,int hide,uint32_t*f){if(!s->b.mask_nodes)return fail(f,0x14b6f8,GH_POST_UNSUPPORTED);uint32_t pc=0;gh_scene_post_status e=s->b.mask_nodes(s->b.context,h,hide?UINT32_MAX:~1u,hide?1u:0u,&pc);return e?fail(f,pc?pc:0x14b6f8,e):GH_POST_OK;}
static gh_scene_post_status insert(gh_scene_visibility*s,uint32_t h,int*accepted,uint32_t*f){*accepted=0;for(unsigned i=0;i<32;i++){uint32_t old=s->b.scene.tables->scroll_link_words[i];if(old){if(s->known_previous[i]!=old)return fail(f,0x3089f0,GH_POST_UNSUPPORTED);if(old==h){*accepted=1;return GH_POST_OK;}}else{s->known_previous[i]=h;s->b.scene.tables->scroll_link_words[i]=h;*accepted=1;return GH_POST_OK;}}return GH_POST_OK;}
static gh_scene_post_status report(gh_scene_visibility*s,const gh_room_vector*v,uint32_t*f){
 gh_scene_ray_report*r=&s->reports[s->report_index];if(!r->attributes||r->attribute_bytes<12)return fail(f,0x1333b8,GH_POST_ARGUMENT);const uint8_t*a=r->attributes;unsigned base=half(a+8),sc=a[10],ac=a[11];
 while(s->static_row<sc){unsigned n=base+s->static_row;if(n>65535)return fail(f,0x1333b8,GH_POST_ARGUMENT);gh_actor_scroll_entry entry;if(gh_scene_actor_scroll_entry_001333b8(s->b.scene.tables,(uint16_t)n,&entry))return fail(f,0x1333b8,GH_POST_UNSUPPORTED);
  if(!(uint32_t)entry.current_key){if(entry.resolved&&entry.scroll_handle){int accepted;gh_scene_post_status e=insert(s,entry.scroll_handle,&accepted,f);if(e)return e;if(accepted){e=mask(s,entry.scroll_handle,1,f);if(e)return e;}}else if(entry.current_key)return fail(f,0x14b6f8,GH_POST_UNSUPPORTED);}
  s->static_row++;
 }
 while(s->actor_row<ac){unsigned n=base+sc+s->actor_row;if(n>65535)return fail(f,0x1333b8,GH_POST_ARGUMENT);gh_actor_scroll_entry entry;if(gh_scene_actor_scroll_entry_001333b8(s->b.scene.tables,(uint16_t)n,&entry)||entry.resolved)return fail(f,0x1333b8,GH_POST_UNSUPPORTED);
  while(s->actor_index<v->count){gh_title_player_view x;gh_scene_post_status e=actor(s,v,s->actor_index,&x,f);if(e)return e;if(eligible(half(x.bytes+0x2fe))){if(x.size<0x5a4)return fail(f,0x307b28,GH_POST_ARGUMENT);if(key(x.bytes+0x538)==entry.current_key)put(x.bytes+0x5a0,word(x.bytes+0x5a0)|2u);}s->actor_index++;}s->actor_index=0;s->actor_row++;
 }return GH_POST_OK;
}
gh_scene_post_status gh_scene_visibility_step_00307b28(gh_scene_visibility*s,const gh_scene_visibility_bindings*b,uint32_t*f){
 if(f)*f=0;if(!s||s->identity!=s||!b||!b->scene.work||!b->scene.pools||!b->scene.tables)return GH_POST_ARGUMENT;
 if(s->bound){
  gh_scene_visibility_bindings previous=s->b;
  if(!s->phase&&!s->complete)previous.scene.camera=b->scene.camera;
  if(!same(&previous,b))return GH_POST_ARGUMENT;
  s->b.scene.camera=previous.scene.camera;
 }else{s->b=*b;s->bound=1;}
 if(s->complete)return GH_POST_OK;
 if(!s->phase){if(word((const uint8_t*)b->scene.work+0x5d8)&0x200000u){s->complete=1;return GH_POST_OK;}s->phase=1;}
 if(s->phase==1){while(s->previous<32){uint32_t h=b->scene.tables->scroll_link_words[s->previous];if(h){if(s->known_previous[s->previous]!=h)return fail(f,0x14b6f8,GH_POST_UNSUPPORTED);gh_scene_post_status e=mask(s,h,0,f);if(e)return e;}s->previous++;}s->phase=2;}
 const gh_room_vector*v;gh_scene_post_status e=registry(s,&v,f);if(e)return e;
 if(s->phase==2){while(s->index<v->count){gh_title_player_view x;e=actor(s,v,s->index,&x,f);if(e)return e;if(eligible(half(x.bytes+0x2fe))){if(x.size<0x5a4)return fail(f,0x308bc0,GH_POST_ARGUMENT);put(x.bytes+0x5a0,word(x.bytes+0x5a0)&~2u);}s->index++;}s->phase=3;}
 if(s->phase==3){gh_scene_hidden_clear_00308d40(b->scene.tables);memset(s->known_previous,0,sizeof(s->known_previous));e=gh_scene_model_rays_00307b28(&b->scene,s->rays,&s->ray_count,f);if(e)return e;s->phase=4;}
 while(s->ray_index<s->ray_count){
  if(s->phase==4){memset(s->reports,0,sizeof(s->reports));gh_scene_ray_request q={0};memcpy(q.from,s->rays[s->ray_index].from,16);memcpy(q.to,s->rays[s->ray_index].to,16);q.mask=7;q.report_limit=8;q.native_reports=s->reports;int hit=0;uint32_t pc=0;gh_scene_ray_status z=gh_scene_ray_query_00131e10(b->scene.tables,&q,&hit,&pc);if(z)return fail(f,pc?pc:0x131e10,z==GH_RAY_UNSUPPORTED?GH_POST_UNSUPPORTED:GH_POST_QUERY);s->report_count=hit?b->scene.tables->ray.triangle17c:0;if(s->report_count>8)return fail(f,0x3085a4,GH_POST_ARGUMENT);s->phase=5;}
  if(s->phase==5){while(s->report_index<s->report_count){e=report(s,v,f);if(e)return e;s->report_index++;s->static_row=s->actor_row=s->actor_index=0;}s->phase=6;}
  if(s->phase==6){gh_scene_ray_request q={0};memcpy(q.from,s->rays[s->ray_index].from,16);memcpy(q.to,s->rays[s->ray_index].to,16);q.mask=7;s->dynamic_count=0;gh_ray_dynamic_reports d={s->dynamic_objects,8,&s->dynamic_count};q.dynamic_reports=&d;int hit=0;uint32_t pc=0;gh_scene_ray_status z=gh_scene_ray_query_00131e10(b->scene.tables,&q,&hit,&pc);if(z)return fail(f,pc?pc:0x131e10,z==GH_RAY_UNSUPPORTED?GH_POST_UNSUPPORTED:GH_POST_QUERY);if(!hit)s->dynamic_count=0;if(s->dynamic_count>8)return fail(f,0x308810,GH_POST_ARGUMENT);s->phase=7;}
  if(s->phase==7){while(s->dynamic_index<s->dynamic_count){const void*id=s->dynamic_objects[s->dynamic_index];if(id){int found=0;for(unsigned i=0;i<v->count;i++){gh_title_player_view x;e=actor(s,v,i,&x,f);if(e)return e;if(x.bytes==id){found=1;unsigned k=half(x.bytes+0x2fe);if(k>0x2ff&&k<0x500){if(x.size<0x5bc)return fail(f,0x308810,GH_POST_ARGUMENT);if(word(x.bytes+0x5b8)&4)put(x.bytes+0x5a0,word(x.bytes+0x5a0)|2u);}break;}}if(!found)return fail(f,0x308810,GH_POST_UNSUPPORTED);}s->dynamic_index++;}s->ray_index++;s->report_count=s->report_index=s->static_row=s->actor_row=s->actor_index=s->dynamic_index=0;s->phase=4;}
 }
 if(s->phase==4)s->phase=8;if(s->phase==8){if(s->index==s->registry_count)s->index=0;while(s->index<v->count){gh_title_player_view x;e=actor(s,v,s->index,&x,f);if(e)return e;if(eligible(half(x.bytes+0x2fe))){if(x.size<0x5a4)return fail(f,0x307b28,GH_POST_ARGUMENT);if(word(x.bytes+0x5a0)&0x10){int hit=0;uint32_t pc=0;e=gh_scene_box_rays_001f92c0(x.bytes+0x260,x.bytes+0x80,s->rays,s->ray_count,GH_STATIC_BINARY32_REFERENCE,&hit,&pc);if(e)return fail(f,pc?pc:0x1f92c0,e);if(hit)put(x.bytes+0x5a0,word(x.bytes+0x5a0)|2u);}}s->index++;}s->phase=9;s->complete=1;}return GH_POST_OK;
}
