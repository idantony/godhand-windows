#include "game_enemy_shadow.h"
#include "game_enemy_row2.h"
gh_actor_motion_status gh_enemy_shadow_initialize_00204b10(gh_enemy_shadow*s,gh_enemy_pose*pose){
 if(!s||!pose||pose->stage!=3||!pose->motion||!pose->motion->hp||!pose->motion->hp->tail||!pose->motion->hp->tail->entry)return GH_ACTOR_MOTION_ARGUMENT;
 if(s->stage&&s->pose!=pose)return GH_ACTOR_MOTION_BUSY;gh_enemy_entry*e=pose->motion->hp->tail->entry;
 if(e->request.dependency_kind!=0x20a&&!gh_enemy202_is_r100_supported_profile(&e->request))return GH_ACTOR_MOTION_ARGUMENT;
 if(!s->stage){gh_title_player_status status=gh_game_actor_shadow_initialize_0014ef50(&s->shadow,&e->object.view,0x780,0x3c3f68);if(status){s->frontier=0x14ef50;return GH_ACTOR_MOTION_UNSUPPORTED;}s->pose=pose;s->stage=1;}
 /* Next2022D0 scalar/AI initializer prefix pending. */
 s->frontier=0x204b4c;e->frontier=s->frontier;return GH_ACTOR_MOTION_UNSUPPORTED;
}
