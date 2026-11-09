/* MIT LICENSE.upstream:001169d8.c/001e6780.c, actual1F72C0/1F77D0. */
#include "game_action_buttons.h"
void gh_game_action_buttons_reset_001f72c0(gh_game_action_buttons*s){
 if(!s)return;
 for(unsigned i=16;i>1;i--){s->pool[i-1].previous=s->pool+i-2;s->pool[i-1].flags=0;}
 s->pool[0].previous=0;s->pool[0].flags=0;
 for(unsigned i=0;i<8;i++){
  gh_action_prompt*e=s->entries+i;e->flags&=~1u;e->state0c=1;
  e->callback_id10=0;e->callback_argument=0;e->priority=0;e->life=0x31;
  e->kind=0;e->unknown20=0;e->matrix=0;e->unknown2c=0x1f;
  e->unknown30=0;e->command_count=0;e->unknown35=0;e->unknown32=0;e->unknown36=0;
  e->command_table=0;e->id_table=0;e->argument_table=0;
 }
}
