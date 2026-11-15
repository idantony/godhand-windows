#include "game_enemy_random.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);return 1;}}while(0)
int main(void){gh_game_init_state*s=calloc(1,sizeof(*s));C(s);int original=9;
 gh_game_mt_seed_0030f2b0(&s->random,5489);uint32_t a,b;gh_game_mt_next_0030f348(&s->random,&a);gh_game_mt_next_0030f348(&s->random,&b);C(a==3499211612u&&b==581869302u&&a%32==28u&&(b&3)==2);gh_game_mt_seed_0030f2b0(&s->random,5489);s->enemies[0x53f]=28;C(!gh_enemy_manager_random_check_002949e8(s,32,&original)&&original==1&&!s->enemies[0x53f]&&s->random.index==2);
 gh_game_mt_seed_0030f2b0(&s->random,5489);s->enemies[0x53f]=0;C(!gh_enemy_manager_random_check_002949e8(s,32,&original)&&original==0&&s->enemies[0x53f]==1);
 gh_game_mt_seed_0030f2b0(&s->random,5489);s->enemies[0x53f]=255;C(!gh_enemy_manager_random_check_002949e8(s,32,&original)&&!original&&!s->enemies[0x53f]);
 gh_game_mt_seed_0030f2b0(&s->random,5489);s->enemies[0x53f]=100;C(!gh_enemy_manager_random_check_002949e8(s,1000,&original)&&original&&!s->enemies[0x53f]);
 gh_game_mt_seed_0030f2b0(&s->random,3);s->enemies[0x53f]=100;C(!gh_enemy_manager_random_check_002949e8(s,32,&original)&&!original&&s->enemies[0x53f]==1);
 unsigned index=s->random.index;C(gh_enemy_manager_random_check_002949e8(s,0,&original)==GH_INIT_ARGUMENT&&s->random.index==index);
 free(s);puts("PASS actual2949E8 knownMTwords unsignedDIVU/low8/signedLB/secondwordreset/bytewrap/nonzeroprecondition");return 0;}
