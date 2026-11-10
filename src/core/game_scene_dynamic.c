/*Actual12EAD8 append +12D970 boxconstructor; NOT dynamicquery130700.*/
#include "game_scene_dynamic.h"
#include "game_scene_dynamic_internal.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stddef.h>
typedef struct {uint32_t next,kind,flags,unwritten0c;uint8_t vec10[16],vec20[16];uint32_t actor30;uint8_t reset34[32];uint32_t field54,matrix58,vtable5c;uint8_t position60[16],offset70[16],rotation80[16],extra90[16],sizea0[16],attributesb0[96],corners110[128];} original_box_layout;
_Static_assert(sizeof(original_box_layout)==0x190,"Originalboxsize");_Static_assert(offsetof(original_box_layout,attributesb0)==0xb0,"Originalattributes");_Static_assert(offsetof(original_box_layout,corners110)==0x110,"Originalcorners");
static void put(uint8_t*p,size_t o,uint32_t n){memcpy(p+o,&n,4);}static void vf0(uint8_t*p,size_t o){memset(p+o,0,12);put(p,o+12,0x3f800000);}
static int known(const gh_scene_dynamic_owner*s,const struct gh_scene_dynamic_box*b){for(size_t i=0;i<s->allocation_count;i++)if(s->allocations[i]==b)return 1;return 0;}
gh_scene_dynamic_status gh_scene_dynamic_validate_owner(gh_scene_dynamic_owner*s){if(!s||!s->bound||s->identity!=s||!s->tables||s->tables->dynamic_native_owner!=s||!s->heap)return GH_DYNAMIC_ARGUMENT;struct gh_scene_dynamic_box*b=s->tables->pending_actor_head;size_t n=0;while(b){if(!known(s,b)||b->owner!=s||n>=s->allocation_count){s->frontier=0x12ecf0;return GH_DYNAMIC_UNSUPPORTED;}n++;b=b->next;}if(n!=s->allocation_count||n!=s->tables->pending_actor_count){s->frontier=0x12ecf0;return GH_DYNAMIC_BUSY;}return GH_DYNAMIC_OK;}
gh_scene_dynamic_status gh_scene_dynamic_bind(gh_scene_dynamic_owner*s,gh_game_scene_tables*t,gh_heap*h){if(!s||!t||!h)return GH_DYNAMIC_ARGUMENT;if(s->bound)return s->identity==s&&s->tables==t&&s->heap==h&&t->dynamic_native_owner==s?GH_DYNAMIC_OK:GH_DYNAMIC_BUSY;if(t->dynamic_native_owner&&t->dynamic_native_owner!=s)return GH_DYNAMIC_BUSY;if(t->pending_actor_head||t->pending_actor_count){s->frontier=0x12ecf0;return GH_DYNAMIC_UNSUPPORTED;}s->tables=t;s->heap=h;s->bound=1;s->identity=s;t->dynamic_native_owner=s;return GH_DYNAMIC_OK;}
gh_scene_dynamic_status gh_scene_dynamic_box_create_0012ead8(gh_scene_dynamic_owner*s,gh_title_player_view*a,const uint8_t*matrix,const float*offset,const float*rotation,const float*size,const uint8_t*attributes,struct gh_scene_dynamic_box**out,int*original){if(!s||!out||!original)return GH_DYNAMIC_ARGUMENT;*out=NULL;*original=0;gh_scene_dynamic_status e=gh_scene_dynamic_validate_owner(s);if(e)return e;if(s->update_active)return GH_DYNAMIC_BUSY;s->frontier=0;if(!a)return GH_DYNAMIC_OK;if(!a->bytes||a->size<0x10c||!a->alias_f0||(uintptr_t)a->alias_f0<(uintptr_t)a->bytes||(uintptr_t)a->alias_f0-(uintptr_t)a->bytes>a->size||12>a->size-((uintptr_t)a->alias_f0-(uintptr_t)a->bytes)||!offset||!rotation||!size||!attributes)return GH_DYNAMIC_ARGUMENT;for(unsigned j=0;j<3;j++){float p,r;memcpy(&p,a->alias_f0+j*4,4);memcpy(&r,a->bytes+0x100+j*4,4);if(!isfinite(p)||!isfinite(r)||!isfinite(offset[j])||!isfinite(rotation[j])||!isfinite(size[j])){s->frontier=0x12d970;return GH_DYNAMIC_UNSUPPORTED;}}
 if(s->allocation_count==s->capacity){size_t cap=s->capacity?s->capacity*2:8;if(cap<s->capacity||cap>SIZE_MAX/sizeof(*s->allocations))return GH_DYNAMIC_MEMORY;void*p=realloc(s->allocations,cap*sizeof(*s->allocations));if(!p)return GH_DYNAMIC_MEMORY;s->allocations=p;s->capacity=cap;}
 struct gh_scene_dynamic_box*b=calloc(1,sizeof(*b));if(!b)return GH_DYNAMIC_MEMORY;if(gh_heap_allocate(s->heap,0x190,16,&b->allocation)){free(b);s->frontier=0x30f550;return GH_DYNAMIC_MEMORY;}b->owner=s;b->actor=a;b->matrix=matrix;b->original_bytes=0x190;uint8_t*p=b->allocation.pointer;
 put(p,0,0);put(p,4,2);put(p,8,0);vf0(p,0x10);vf0(p,0x20);put(p,0x30,0);memset(p+0x34,0,0x20);put(p,0x54,0);put(p,0x58,0);put(p,0x5c,0x41d558);for(unsigned i=0;i<5;i++)vf0(p,0x60+i*16);for(unsigned i=0;i<8;i++)vf0(p,0x110+i*16);
 memcpy(p+0x60,a->alias_f0,12);memcpy(p+0x70,offset,12);memcpy(p+0x80,a->bytes+0x100,12);memcpy(p+0x90,rotation,12);for(unsigned i=0;i<3;i++){float f=size[i];if(f<0)f=-f;memcpy(p+0xa0+i*4,&f,4);}for(unsigned i=0;i<6;i++){memcpy(p+0xb0+i*16,attributes+i*16,16);uint16_t sentinel=0xffff;memcpy(p+0xb8+i*16,&sentinel,2);p[0xba+i*16]=p[0xbb+i*16]=0;}put(p,8,5);
 if(s->tables->pending_actor_head){struct gh_scene_dynamic_box*tail=s->tables->pending_actor_head;while(tail->next)tail=tail->next;tail->next=b;}else s->tables->pending_actor_head=b;s->tables->pending_actor_count++;s->allocations[s->allocation_count++]=b;*out=b;*original=1;return GH_DYNAMIC_OK;}
gh_scene_dynamic_status gh_scene_dynamic_box_view_at(const gh_scene_dynamic_owner*s,const struct gh_scene_dynamic_box*b,gh_scene_dynamic_box_view*out){if(!s||!s->bound||s->identity!=s||!b||!out||!known(s,b))return GH_DYNAMIC_ARGUMENT;if(b->original_bytes!=0x100&&b->original_bytes!=0x190)return GH_DYNAMIC_ARGUMENT;*out=(gh_scene_dynamic_box_view){b->allocation.pointer,b->original_bytes,b->actor,b->matrix,b->next,b->world_ready};return GH_DYNAMIC_OK;}
gh_scene_dynamic_status gh_scene_dynamic_box_abort(gh_scene_dynamic_owner*s,struct gh_scene_dynamic_box*b){gh_scene_dynamic_status e=gh_scene_dynamic_validate_owner(s);if(e)return e;if(s->update_active)return GH_DYNAMIC_BUSY;if(!b||!known(s,b))return GH_DYNAMIC_ARGUMENT;struct gh_scene_dynamic_box*previous=NULL,*cur=s->tables->pending_actor_head;while(cur!=b){previous=cur;cur=cur->next;}if(gh_heap_release(s->heap,b->allocation))return GH_DYNAMIC_BUSY;if(previous)previous->next=b->next;else s->tables->pending_actor_head=b->next;s->tables->pending_actor_count--;size_t index=0;while(s->allocations[index]!=b)index++;memmove(s->allocations+index,s->allocations+index+1,(s->allocation_count-index-1)*sizeof(*s->allocations));s->allocation_count--;free(b);s->frontier=0;return GH_DYNAMIC_OK;}
gh_scene_dynamic_status gh_scene_dynamic_release(gh_scene_dynamic_owner*s){if(!s)return GH_DYNAMIC_ARGUMENT;if(!s->bound){if(s->allocations)return GH_DYNAMIC_BUSY;return GH_DYNAMIC_OK;}gh_scene_dynamic_status e=gh_scene_dynamic_validate_owner(s);if(e)return e;if(s->update_active)return GH_DYNAMIC_BUSY;while(s->tables->pending_actor_head){e=gh_scene_dynamic_box_abort(s,s->tables->pending_actor_head);if(e)return e;}free(s->allocations);if(s->tables->dynamic_native_owner==s)s->tables->dynamic_native_owner=NULL;*s=(gh_scene_dynamic_owner){0};return GH_DYNAMIC_OK;}

void gh_scene_dynamic_cancel_update(gh_scene_dynamic_owner*s){if(!s||s->identity!=s)return;s->update_cursor=NULL;s->update_tables=NULL;s->update_work=NULL;s->update_completed=s->update_allocation_count=0;s->update_active=0;}

/* No invented per-frame reset: actual12E840/12DCB8 do not write+54.
 * Constructor12D970 clears raw34..54 and calloc clears these companions. */
gh_scene_dynamic_status gh_scene_dynamic_sphere_contact_001311a0(gh_scene_dynamic_owner*s,struct gh_scene_dynamic_box*b,const gh_title_player_view*actor,int*inserted){
 if(!inserted||!actor||!actor->bytes)return GH_DYNAMIC_ARGUMENT;*inserted=0;gh_scene_dynamic_status e=gh_scene_dynamic_validate_owner(s);if(e)return e;
 if(!b||!known(s,b)||b->owner!=s)return GH_DYNAMIC_ARGUMENT;uint8_t*p=b->allocation.pointer;uint32_t count;memcpy(&count,p+0x54,4);if(count>=8)return GH_DYNAMIC_OK;
 for(unsigned i=0;i<count;i++){
  if(!b->sphere_contact_known[i]||!b->sphere_contacts[i]){s->frontier=0x1311a0;return GH_DYNAMIC_UNSUPPORTED;}
  if(b->sphere_contacts[i]->bytes==actor->bytes)return GH_DYNAMIC_OK;
 }
 b->sphere_contacts[count]=actor;b->sphere_contact_known[count]=1;/*raw+34 word remains removedguestpointer0*/
 count++;memcpy(p+0x54,&count,4);*inserted=1;return GH_DYNAMIC_OK;
}
gh_scene_dynamic_status gh_scene_dynamic_sphere_contact_at(const gh_scene_dynamic_owner*s,const struct gh_scene_dynamic_box*b,unsigned index,const gh_title_player_view**out){
 if(!s||!out||!b||!known(s,b)||s->identity!=s||b->owner!=s)return GH_DYNAMIC_ARGUMENT;uint32_t count;memcpy(&count,(const uint8_t*)b->allocation.pointer+0x54,4);
 if(index>=count||index>=8)return GH_DYNAMIC_ARGUMENT;if(!b->sphere_contact_known[index]||!b->sphere_contacts[index])return GH_DYNAMIC_UNSUPPORTED;*out=b->sphere_contacts[index];return GH_DYNAMIC_OK;
}
void gh_scene_dynamic_forget_sphere_actor(gh_scene_dynamic_owner*s,const gh_title_player_view*actor){
 if(!s||s->identity!=s||!actor)return;for(size_t i=0;i<s->allocation_count;i++){struct gh_scene_dynamic_box*b=s->allocations[i];if(!b||b->owner!=s)continue;for(unsigned j=0;j<8;j++)if(b->sphere_contacts[j]==actor)b->sphere_contacts[j]=NULL;}
}

gh_scene_dynamic_status gh_scene_dynamic_quad_create_0012e990(gh_scene_dynamic_owner*s,gh_title_player_view*a,const uint8_t*matrix,const float*offset,const float*rotation,float width,float height,const uint8_t*attributes,struct gh_scene_dynamic_box**out,int*original){
 if(!s||!out||!original)return GH_DYNAMIC_ARGUMENT;*out=NULL;*original=0;gh_scene_dynamic_status e=gh_scene_dynamic_validate_owner(s);if(e)return e;if(s->update_active)return GH_DYNAMIC_BUSY;s->frontier=0;if(!a)return GH_DYNAMIC_OK;
 if(!a->bytes||a->size<0x10c||!a->alias_f0||(uintptr_t)a->alias_f0<(uintptr_t)a->bytes||(uintptr_t)a->alias_f0-(uintptr_t)a->bytes>a->size||12>a->size-((uintptr_t)a->alias_f0-(uintptr_t)a->bytes)||!offset||!rotation||!attributes)return GH_DYNAMIC_ARGUMENT;
 if(!isfinite(width)||!isfinite(height)){s->frontier=0x12cf70;return GH_DYNAMIC_UNSUPPORTED;}
 for(unsigned k=0;k<3;k++){float position,angle;memcpy(&position,a->alias_f0+k*4,4);memcpy(&angle,a->bytes+0x100+k*4,4);if(!isfinite(position)||!isfinite(angle)||!isfinite(offset[k])||!isfinite(rotation[k])){s->frontier=0x12cf70;return GH_DYNAMIC_UNSUPPORTED;}}
 if(s->allocation_count==s->capacity){size_t cap=s->capacity?s->capacity*2:8;if(cap<s->capacity||cap>SIZE_MAX/sizeof(*s->allocations))return GH_DYNAMIC_MEMORY;void*p=realloc(s->allocations,cap*sizeof(*s->allocations));if(!p)return GH_DYNAMIC_MEMORY;s->allocations=p;s->capacity=cap;}
 struct gh_scene_dynamic_box*b=calloc(1,sizeof(*b));if(!b)return GH_DYNAMIC_MEMORY;if(gh_heap_allocate(s->heap,0x100,16,&b->allocation)){free(b);s->frontier=0x30f550;return GH_DYNAMIC_MEMORY;}b->owner=s;b->actor=a;b->matrix=matrix;b->original_bytes=0x100;uint8_t*p=b->allocation.pointer;
 put(p,0,0);put(p,4,1);put(p,8,0);vf0(p,0x10);vf0(p,0x20);put(p,0x30,0);memset(p+0x34,0,0x20);put(p,0x54,0);put(p,0x58,0);put(p,0x5c,0x41d580);for(unsigned i=0;i<4;i++){vf0(p,0x60+i*16);vf0(p,0xc0+i*16);}
 memcpy(p+0x60,a->alias_f0,12);memcpy(p+0x70,offset,12);memcpy(p+0x80,a->bytes+0x100,12);memcpy(p+0x90,rotation,12);if(width<0)width=-width;if(height<0)height=-height;memcpy(p+0xa0,&width,4);memcpy(p+0xa4,&height,4);memcpy(p+0xa8,attributes,16);uint16_t sentinel=0xffff;memcpy(p+0xb0,&sentinel,2);p[0xb2]=p[0xb3]=0;put(p,8,5);
 if(s->tables->pending_actor_head){struct gh_scene_dynamic_box*tail=s->tables->pending_actor_head;while(tail->next)tail=tail->next;tail->next=b;}else s->tables->pending_actor_head=b;s->tables->pending_actor_count++;s->allocations[s->allocation_count++]=b;*out=b;*original=1;return GH_DYNAMIC_OK;
}
