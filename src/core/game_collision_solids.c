/* MIT LICENSE.upstream; actual134690 / src/cod/00107e40.c. */
#include "game_collision_solids.h"
void gh_game_collision_solids_reset_00134690(gh_game_collision_solids*s){
 if(!s)return;s->unit_head=0;s->unit_count=0;s->global_00460d18=0;
 for(unsigned i=0;i<32;i++)s->words08[i]=0;
 s->field88=0;
}
