#include "game_enemy_2a2_pose.h"
#include "game_actor_motion_step.h"
#include <string.h>

enum {
    SAMPLE=1, HIERARCHY, SOLVE, FINISH, DISABLE_SOLIDS, SHADOW,
    CREATE_HITAREA, ADD_HIT_SPHERES, CREATE_SOLID, ADD_SOLID_SPHERE, RANDOM_TIMER
};

static void put(uint8_t *bytes,uint32_t word)
{
    memcpy(bytes,&word,sizeof(word));
}

static gh_actor_model_status stop(gh_enemy2a2_pose *s,uint32_t frontier,
    gh_actor_model_status status)
{
    s->frontier=frontier;
    return status;
}

static int same_bindings(const gh_enemy2a2_pose_bindings *a,const gh_enemy2a2_pose_bindings *b)
{
    return a->rate==b->rate && a->actors==b->actors && a->atan_coeff==b->atan_coeff &&
        a->init==b->init && a->shapes.pairs==b->shapes.pairs && a->shapes.math==b->shapes.math &&
        a->shapes.policy==b->shapes.policy && a->solids==b->solids && a->solid_heap==b->solid_heap;
}

gh_actor_model_status gh_enemy2a2_pose_continue_002851b8(gh_enemy2a2_pose *s,
    gh_enemy2a2_init *prefix,const gh_enemy2a2_pose_bindings *b)
{
    if (!s || !prefix || !prefix->root || !prefix->motion.complete ||
        prefix->frontier!=0x157c18 || !b || !b->rate || !b->actors || !b->atan_coeff ||
        !b->init || !b->shapes.pairs || !b->shapes.math || !b->solids || !b->solid_heap)
        return GH_ACTOR_MODEL_ARGUMENT;
    gh_title_player_view *actor=prefix->root;
    gh_actor_model *model=&prefix->model.model;
    if (!actor->alias_f0 || model->root.bytes!=actor->bytes || prefix->motion.b.model!=model ||
        prefix->bindings.random!=&b->init->random) return GH_ACTOR_MODEL_ARGUMENT;
    if (s->stage && (s->prefix!=prefix || !same_bindings(&s->bindings,b))) return GH_ACTOR_MODEL_BUSY;
    if (!s->stage) {
        s->prefix=prefix;
        s->bindings=*b;
        s->stage=SAMPLE;
    }
    s->frontier=0;
    uint32_t frontier=0;
    const gh_static_transform_policy policy=prefix->bindings.policy;
    uint8_t *bytes=actor->bytes;
    if (s->stage==SAMPLE) {
        int ended;
        gh_actor_motion_status status=gh_actor_motion_step_00157c18(&prefix->motion,*b->rate,
            policy,&ended,&frontier);
        if (status) return stop(s,frontier?frontier:0x157c18,GH_ACTOR_MODEL_UNSUPPORTED);
        s->stage=HIERARCHY;
    }
    if (s->stage==HIERARCHY) {
        gh_actor_model_status status=gh_actor_model_update_hierarchy_00149eb8(model,policy);
        if (status) return stop(s,model->frontier?model->frontier:0x149eb8,status);
        s->stage=SOLVE;
    }
    if (s->stage==SOLVE) {
        gh_motion_solve_bindings solve={.model=model,.work=prefix->bindings.work,.actors=b->actors,
            .ik_globals=prefix->bindings.ik,.atan_coeff=b->atan_coeff};
        if (gh_actor_motion_solve_00152a20(&s->solve,&solve,policy,&frontier))
            return stop(s,frontier?frontier:0x152a20,GH_ACTOR_MODEL_UNSUPPORTED);
        s->stage=FINISH;
    }
    if (s->stage==FINISH) {
        gh_actor_model_status status=gh_actor_finish_model_resume_0014a038(&s->finish,model,policy);
        if (status) return stop(s,model->frontier?model->frontier:0x14a038,status);
        s->stage=DISABLE_SOLIDS;
    }
    if (s->stage==DISABLE_SOLIDS) {
        if (gh_collision_solid_set_enabled_001351d8(b->solids,actor,0,&frontier))
            return stop(s,frontier?frontier:0x1351d8,GH_ACTOR_MODEL_UNSUPPORTED);
        memcpy(bytes+0x1590,actor->alias_f0,12);
        bytes[0x140]=4;
        s->stage=SHADOW;
    }
    if (s->stage==SHADOW) {
        if (gh_game_actor_shadow_initialize_0014ef50(&s->shadow,actor,0x690,0x3c3f68))
            return stop(s,0x14ef50,GH_ACTOR_MODEL_UNSUPPORTED);
        s->stage=CREATE_HITAREA;
    }
    if (s->stage==CREATE_HITAREA) {
        int nonnull;
        if (gh_hitarea_object_create_001fe218(b->init,actor,1,&s->hitarea,&nonnull,&frontier))
            return stop(s,frontier?frontier:0x1fe218,GH_ACTOR_MODEL_UNSUPPORTED);
        s->hitarea_created=1;
        put(bytes+0x670,0); /* Original pointer is the typed B-unit view. */
        s->stage=ADD_HIT_SPHERES;
    }
    if (s->stage==ADD_HIT_SPHERES) {
        static const unsigned bone[]={1,3,19,23};
        const uint32_t zero[]={0,0,0};
        for (;s->hit_cursor<4;s->hit_cursor++) {
            unsigned i=s->hit_cursor;
            const gh_actor_part *part;
            if (gh_actor_model_part_at(model,bone[i],&part))
                return stop(s,0x1fdaf0,GH_ACTOR_MODEL_UNSUPPORTED);
            if (gh_hitarea_add_sphere_001fdaf0(&s->hitarea,&b->shapes,part->bytes+0x80,
                zero,0x3e800000,&s->hit_indices[i],&frontier))
                return stop(s,frontier?frontier:0x1fdaf0,GH_ACTOR_MODEL_UNSUPPORTED);
            put(bytes+0x674+i*4,(uint32_t)s->hit_indices[i]);
        }
        s->stage=CREATE_SOLID;
    }
    if (s->stage==CREATE_SOLID) {
        if (gh_collision_solid_create_unit_001346f8(b->solids,b->solid_heap,actor,5,
            0x3e4ccccd,&s->unit,&frontier))
            return stop(s,frontier?frontier:0x1346f8,GH_ACTOR_MODEL_UNSUPPORTED);
        s->stage=ADD_SOLID_SPHERE;
    }
    if (s->stage==ADD_SOLID_SPHERE) {
        const uint32_t local[]={0,0x3f000000,0};
        if (gh_collision_solid_create_sphere_00134880(b->solids,actor,bytes+0x80,local,
            0x3f000000,&s->sphere,&frontier))
            return stop(s,frontier?frontier:0x134880,GH_ACTOR_MODEL_UNSUPPORTED);
        put(bytes+0x15b8,0);
        put(bytes+0x15b4,0x3ca3d70a); /* Delay slot of JAL 31D000. */
        s->stage=RANDOM_TIMER;
    }
    return stop(s,0x31d000,GH_ACTOR_MODEL_UNSUPPORTED);
}

gh_actor_model_status gh_enemy2a2_pose_release_prefix(gh_enemy2a2_pose *s)
{
    if (!s) return GH_ACTOR_MODEL_ARGUMENT;
    if (s->unit) {
        gh_game_collision_solids *manager=s->bindings.solids;
        unsigned seen=0;
        gh_collision_solid_unit *unit=manager->native_units;
        for (;unit && unit!=s->unit;unit=unit->next)
            if (++seen>=manager->unit_count) return GH_ACTOR_MODEL_BUSY;
        if (!unit || unit->object!=s->prefix->root) return GH_ACTOR_MODEL_BUSY;
        if (gh_collision_solid_release_unit_001347e8(manager,s->prefix->root,&s->frontier))
            return GH_ACTOR_MODEL_UNSUPPORTED;
        s->unit=NULL;
        s->sphere=NULL;
    }
    if (s->hitarea_created && !s->hitarea.fallback) {
        if (gh_hitarea_shapes_abort_object(&s->hitarea,&s->frontier) ||
            gh_hitarea_object_release_empty_001fe278(&s->hitarea,&s->frontier))
            return GH_ACTOR_MODEL_UNSUPPORTED;
    }
    *s=(gh_enemy2a2_pose){0};
    return GH_ACTOR_MODEL_OK;
}
