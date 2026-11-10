#include "game_actor_external_parent.h"
#include <stdint.h>
static int canonical(const gh_actor_model*s){return s&&s->bound&&s->native_identity==s&&s->parts_ready;}
static int parent_live(const gh_actor_model*c){const gh_actor_model*p=c->external_parent;unsigned i=c->external_parent_index;return canonical(p)&&p->pools==c->pools&&i<p->parts_allocated&&p->parts[i].bytes==c->external_parent_bytes&&p->parts[i].handle==c->external_parent_handle&&p->external_borrowers;}
static const gh_actor_model *next_parent(const gh_actor_model *model) {
    if (model->external_parts) return model->external_parts->parent;
    return model->external_owner ? model->external_parent : NULL;
}
gh_actor_model_status gh_actor_model_external_parent_at(const gh_actor_model*c,unsigned child_index,const uint8_t**out){
 if(!out||!canonical(c)||!c->external_owner||child_index||!c->parts_allocated||c->parts[0].handle!=c->external_child_handle||!parent_live(c))return GH_ACTOR_MODEL_ARGUMENT;
 *out=c->external_parent_bytes;return GH_ACTOR_MODEL_OK;
}
gh_actor_model_status gh_actor_model_bind_external_parent(gh_actor_model*c,unsigned ci,gh_actor_model*p,unsigned pi,const void*owner){
 if(!owner||!canonical(c)||!canonical(p)||c==p||ci||!c->parts_allocated||pi>=p->parts_allocated||c->pools!=p->pools||c->external_parts)return GH_ACTOR_MODEL_ARGUMENT;
 /* Reject indirect cycles and copied owner graphs before changing a borrow. */
 unsigned guard=0;for(const gh_actor_model*n=p;n;n=next_parent(n)){if(n==c||++guard>255||!canonical(n))return GH_ACTOR_MODEL_ARGUMENT;if(n->external_owner&&!parent_live(n))return GH_ACTOR_MODEL_ARGUMENT;}
 if(c->external_owner){if(c->external_owner!=owner)return GH_ACTOR_MODEL_BUSY;if(!parent_live(c))return GH_ACTOR_MODEL_ARGUMENT;if(c->external_parent==p&&c->external_parent_index==pi&&c->external_child_handle==c->parts[0].handle)return GH_ACTOR_MODEL_OK;}
 if(p->external_borrowers==UINT32_MAX)return GH_ACTOR_MODEL_BUSY;
 if(c->external_owner)c->external_parent->external_borrowers--;
 c->external_parent=p;c->external_owner=owner;c->external_parent_bytes=p->parts[pi].bytes;c->external_parent_index=pi;c->external_parent_handle=p->parts[pi].handle;c->external_child_handle=c->parts[0].handle;p->external_borrowers++;return GH_ACTOR_MODEL_OK;
}
gh_actor_model_status gh_actor_model_clear_external_parent(gh_actor_model*c,const void*owner){
 if(!c||!owner||c->native_identity!=c)return GH_ACTOR_MODEL_ARGUMENT;if(!c->external_owner)return GH_ACTOR_MODEL_OK;if(c->external_owner!=owner||!parent_live(c))return GH_ACTOR_MODEL_ARGUMENT;
 c->external_parent->external_borrowers--;c->external_parent=NULL;c->external_owner=NULL;c->external_parent_bytes=NULL;c->external_parent_index=c->external_parent_handle=c->external_child_handle=0;return GH_ACTOR_MODEL_OK;
}

gh_actor_model_status gh_actor_model_bind_external_parts(gh_actor_external_parent_set *set,
    gh_actor_model *child, gh_actor_model *parent, const gh_actor_external_part_pair *pairs, unsigned count) {
    if (!set || set->count || !canonical(child) || !canonical(parent) || !pairs || !count || count > 7 ||
        child->pools != parent->pools || child->external_owner || child->external_parent || child->external_parts) {
        return GH_ACTOR_MODEL_ARGUMENT;
    }
    unsigned guard = 0;
    for (const gh_actor_model *model = parent; model; model = next_parent(model)) {
        if (model == child || ++guard > 255 || !canonical(model) ||
            (model->external_owner && !parent_live(model))) return GH_ACTOR_MODEL_ARGUMENT;
    }
    if (parent->external_borrowers == UINT32_MAX) return GH_ACTOR_MODEL_BUSY;
    gh_actor_external_parent_set candidate = {.child = child, .parent = parent, .count = (uint8_t)count};
    for (unsigned i = 0; i < count; ++i) {
        const unsigned ci = pairs[i].child, pi = pairs[i].parent;
        if (ci >= child->parts_allocated || pi >= parent->parts_allocated || !child->parts[ci].bytes ||
            !parent->parts[pi].bytes) return GH_ACTOR_MODEL_BOUNDS;
        for (unsigned j = 0; j < i; ++j) if (pairs[j].child == ci) return GH_ACTOR_MODEL_ARGUMENT;
        candidate.pairs[i] = pairs[i];
        candidate.child_handles[i] = child->parts[ci].handle;
        candidate.parent_handles[i] = parent->parts[pi].handle;
    }
    *set = candidate;
    child->external_parts = set;
    ++parent->external_borrowers;
    return GH_ACTOR_MODEL_OK;
}

gh_actor_model_status gh_actor_model_external_parts_at(const gh_actor_model *child, unsigned ci,
    const uint8_t **parent_bytes) {
    if (!parent_bytes || !canonical(child) || ci >= child->parts_allocated) return GH_ACTOR_MODEL_ARGUMENT;
    *parent_bytes = NULL;
    const gh_actor_external_parent_set *set = child->external_parts;
    if (!set) return GH_ACTOR_MODEL_OK;
    const gh_actor_model *parent = set->parent;
    if (set->child != child || !set->count || set->count > 7 || !canonical(parent) ||
        parent->pools != child->pools || !parent->external_borrowers) return GH_ACTOR_MODEL_ARGUMENT;
    for (unsigned i = 0; i < set->count; ++i) {
        if (set->pairs[i].child != ci) continue;
        const unsigned pi = set->pairs[i].parent;
        if (pi >= parent->parts_allocated || child->parts[ci].handle != set->child_handles[i] ||
            parent->parts[pi].handle != set->parent_handles[i] || !parent->parts[pi].bytes) {
            return GH_ACTOR_MODEL_ARGUMENT;
        }
        *parent_bytes = parent->parts[pi].bytes;
        break;
    }
    return GH_ACTOR_MODEL_OK;
}

gh_actor_model_status gh_actor_model_clear_external_parts(gh_actor_external_parent_set *set) {
    if (!set) return GH_ACTOR_MODEL_ARGUMENT;
    if (!set->count) return GH_ACTOR_MODEL_OK;
    if (!canonical(set->child) || set->child->external_parts != set || set->count > 7) return GH_ACTOR_MODEL_ARGUMENT;
    for (unsigned i = 0; i < set->count; ++i) {
        const uint8_t *bytes = NULL;
        if (gh_actor_model_external_parts_at(set->child, set->pairs[i].child, &bytes) || !bytes) {
            return GH_ACTOR_MODEL_ARGUMENT;
        }
    }
    --set->parent->external_borrowers;
    set->child->external_parts = NULL;
    *set = (gh_actor_external_parent_set){0};
    return GH_ACTOR_MODEL_OK;
}
