#include "game_enemy_manager.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);return 1;}}while(0)
static uint32_t word(const uint8_t*p){uint32_t v;memcpy(&v,p,4);return v;}
int main(void){gh_game_init_state*s=calloc(1,sizeof(*s));C(s);gh_game_init_state_initialize(s);gh_game_mt_seed_0030f2b0(&s->random,7);C(!gh_game_enemy_reset_00290230(s));int index;uint32_t f;int actors[64];
 C(!gh_enemy_manager_find_free_00290958(s,&index)&&index==0);
 C(!gh_enemy_manager_insert_00290048(s,7,&actors[7],0,&f));C(s->enemy_top==s->enemies+140&&s->enemy_last==s->enemy_top&&word(s->enemies+0x508)==1&&word(s->enemies+0x50c)==1);
 C(!gh_enemy_manager_insert_00290048(s,2,&actors[2],0,&f));C(s->enemy_next[7]==2&&s->enemy_prev[2]==7&&word(s->enemies+0x50c)==1);
 C(!gh_enemy_manager_insert_00290048(s,63,&actors[63],-1,&f)&&word(s->enemies+0x50c)==2);
 gh_game_init_state*copy=malloc(sizeof(*copy));C(copy);memcpy(copy,s,sizeof(*s));C(gh_enemy_manager_insert_00290048(s,2,&actors[2],0,&f)==GH_INIT_LINKS&&f==0x290048&&!memcmp(copy,s,sizeof(*s)));
 s->enemy_next[63]=7;memcpy(copy,s,sizeof(*s));C(gh_enemy_manager_insert_00290048(s,3,&actors[3],0,&f)==GH_INIT_LINKS&&f==0x290988&&!memcmp(copy,s,sizeof(*s)));s->enemy_next[63]=255;
 for(unsigned i=0;i<64;i++)if(i!=7&&i!=2&&i!=63)C(!gh_enemy_manager_insert_00290048(s,i,&actors[i],0,&f));
 C(word(s->enemies+0x508)==64&&word(s->enemies+0x50c)==2&&!gh_enemy_manager_find_free_00290958(s,&index)&&index==-1);
 C(!gh_game_enemy_reset_00290230(s)&&!s->enemy_top&&!s->enemy_last&&!word(s->enemies+0x508));for(unsigned i=0;i<64;i++)C(s->enemy_prev[i]==255&&s->enemy_next[i]==255&&!s->enemy_actors[i]);
 free(copy);free(s);puts("PASS SAME64 enemy slots/order/duplicate-kind/unique-count/full/reset/cycle-atomic; no virtualInit claim");return 0;}
