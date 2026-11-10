#include "game_player_child_update.h"
static int same(const gh_child_move_bindings*a,const gh_child_move_bindings*b){return a->work==b->work&&a->work5f4==b->work5f4&&a->cloth.springs==b->cloth.springs&&a->cloth.attachment==b->cloth.attachment&&a->cloth.templates==b->cloth.templates&&a->cloth.globals==b->cloth.globals&&a->cloth.random==b->cloth.random&&a->cloth.actors==b->cloth.actors&&a->cloth.matrix_policy==b->cloth.matrix_policy&&a->cloth.random_policy==b->cloth.random_policy;}
static gh_title_player_status fail(gh_child_update*s,uint32_t pc){s->frontier=pc;return GH_PLAYER_UNSUPPORTED;}
gh_title_player_status gh_game_player_child_update_00100530(gh_child_update*s,const gh_child_move_bindings*b,gh_player_shared_vector*pivot){
 if(!s||!b)return GH_PLAYER_ARGUMENT;if(s->stage&&(!same(&s->move.bindings,b)||s->shared_pivot!=pivot))return GH_PLAYER_ARGUMENT;if(s->original_returned)return GH_PLAYER_OK;
 if(s->stage==0){s->shared_pivot=pivot;gh_title_player_status e=gh_game_player_child_move_00100530(&s->move,b);if(!e&&s->move.complete){s->original_returned=1;s->stage=6;s->frontier=0;return GH_PLAYER_OK;}if(e!=GH_PLAYER_UNSUPPORTED||s->move.frontier!=0x10430c||s->move.simulation.stage!=6){s->frontier=s->move.frontier;return e;}s->shared_pivot=pivot;s->stage=1;}
 if(s->stage==1){gh_cloth_status e=gh_cloth_integrate_forces_0010430c(&s->forces,&s->move.simulation);if(e)return fail(s,s->forces.frontier);s->stage=2;}
 if(s->stage==2){if(!pivot||!pivot->initialized)return fail(s,0x15af30);gh_cloth_status e=gh_cloth_constrain_angles_0010497c(&s->angles,&s->forces,pivot);if(e)return fail(s,s->angles.frontier);s->stage=3;}
 if(s->stage==3){gh_cloth_status e=gh_cloth_constrain_distances_00104ec8(&s->distances,&s->angles);if(e)return fail(s,s->distances.frontier);s->stage=4;}
 if(s->stage==4){gh_cloth_status e=gh_cloth_finish_00105a94(&s->finish,&s->distances);if(e)return fail(s,s->finish.frontier);s->stage=5;}
 if(s->stage==5){/* The real104010 callee has NOW returned. Resume100CE0 and
  * caller100530 suffix exactlyonce; this is not a manual success bypass. */
  s->move.simulation.complete=1;s->move.simulation.frontier=0;gh_title_player_status e=gh_game_player_child_move_00100530(&s->move,b);if(e){s->frontier=s->move.frontier;return e;}if(!s->move.complete)return fail(s,0x100838);s->original_returned=1;s->stage=6;s->frontier=0;
 }
 return GH_PLAYER_OK;
}
