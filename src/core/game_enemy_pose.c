#include "game_enemy_pose.h"
gh_actor_motion_status gh_enemy_pose_initialize_00204af8(gh_enemy_pose*s,gh_enemy_motion*m,const gh_motion_solve_bindings*b,gh_static_transform_policy policy){
 if(!s||!m||m->stage!=4||!m->hp||!m->hp->tail||!m->hp->tail->entry)return GH_ACTOR_MOTION_ARGUMENT;
 gh_enemy_entry*e=m->hp->tail->entry;
 if(!b||!b->actors||!b->ik_globals||!b->atan_coeff){s->frontier=0x152a20;return GH_ACTOR_MOTION_UNSUPPORTED;}
 if(b->model!=&e->initialize.model.model||b->work!=e->b.work||b->ik_globals!=m->motion.b.ik_globals)return GH_ACTOR_MOTION_ARGUMENT;
 if(s->stage&&(s->motion!=m||(s->solve.actors&&s->solve.actors!=b->actors)))return GH_ACTOR_MOTION_BUSY;
 if(!s->stage){s->motion=m;s->stage=1;}
 if(s->stage==1){gh_actor_motion_status status=gh_actor_motion_solve_00152a20(&s->solve,b,policy,&s->frontier);if(status)return status;s->stage=2;}
 if(s->stage==2){gh_actor_model_status status=gh_actor_finish_model_0014a038(b->model,policy);if(status){s->frontier=b->model->frontier;return GH_ACTOR_MOTION_UNSUPPORTED;}s->stage=3;}
 s->frontier=0x14ef50;e->frontier=s->frontier;return GH_ACTOR_MOTION_UNSUPPORTED;
}
