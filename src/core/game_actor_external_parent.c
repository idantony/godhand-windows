#include "game_actor_external_parent.h"
#include <stdint.h>
static int canonical(const gh_actor_model*s){return s&&s->bound&&s->native_identity==s&&s->parts_ready;}
static int parent_live(const gh_actor_model*c){const gh_actor_model*p=c->external_parent;unsigned i=c->external_parent_index;return canonical(p)&&p->pools==c->pools&&i<p->parts_allocated&&p->parts[i].bytes==c->external_parent_bytes&&p->parts[i].handle==c->external_parent_handle&&p->external_borrowers;}
gh_actor_model_status gh_actor_model_external_parent_at(const gh_actor_model*c,unsigned child_index,const uint8_t**out){
 if(!out||!canonical(c)||!c->external_owner||child_index||!c->parts_allocated||c->parts[0].handle!=c->external_child_handle||!parent_live(c))return GH_ACTOR_MODEL_ARGUMENT;
 *out=c->external_parent_bytes;return GH_ACTOR_MODEL_OK;
}
gh_actor_model_status gh_actor_model_bind_external_parent(gh_actor_model*c,unsigned ci,gh_actor_model*p,unsigned pi,const void*owner){
 if(!owner||!canonical(c)||!canonical(p)||c==p||ci||!c->parts_allocated||pi>=p->parts_allocated||c->pools!=p->pools)return GH_ACTOR_MODEL_ARGUMENT;
 /* Reject indirect cycles and copied owner graphs before changing a borrow. */
 unsigned guard=0;for(const gh_actor_model*n=p;n;n=n->external_parent){if(n==c||++guard>255)return GH_ACTOR_MODEL_ARGUMENT;if(n->external_owner&&!parent_live(n))return GH_ACTOR_MODEL_ARGUMENT;if(!n->external_owner)break;}
 if(c->external_owner){if(c->external_owner!=owner)return GH_ACTOR_MODEL_BUSY;if(!parent_live(c))return GH_ACTOR_MODEL_ARGUMENT;if(c->external_parent==p&&c->external_parent_index==pi&&c->external_child_handle==c->parts[0].handle)return GH_ACTOR_MODEL_OK;}
 if(p->external_borrowers==UINT32_MAX)return GH_ACTOR_MODEL_BUSY;
 if(c->external_owner)c->external_parent->external_borrowers--;
 c->external_parent=p;c->external_owner=owner;c->external_parent_bytes=p->parts[pi].bytes;c->external_parent_index=pi;c->external_parent_handle=p->parts[pi].handle;c->external_child_handle=c->parts[0].handle;p->external_borrowers++;return GH_ACTOR_MODEL_OK;
}
gh_actor_model_status gh_actor_model_clear_external_parent(gh_actor_model*c,const void*owner){
 if(!c||!owner||c->native_identity!=c)return GH_ACTOR_MODEL_ARGUMENT;if(!c->external_owner)return GH_ACTOR_MODEL_OK;if(c->external_owner!=owner||!parent_live(c))return GH_ACTOR_MODEL_ARGUMENT;
 c->external_parent->external_borrowers--;c->external_parent=NULL;c->external_owner=NULL;c->external_parent_bytes=NULL;c->external_parent_index=c->external_parent_handle=c->external_child_handle=0;return GH_ACTOR_MODEL_OK;
}
