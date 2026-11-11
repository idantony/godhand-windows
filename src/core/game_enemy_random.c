#include "game_enemy_random.h"
gh_init_status gh_enemy_manager_random_check_002949e8(gh_game_init_state*s,uint32_t divisor,int*original){
 if(!s||!original||!divisor||s->random.index>625)return GH_INIT_ARGUMENT;
 uint32_t first,second;gh_init_status e=gh_game_mt_next_0030f348(&s->random,&first);if(e)return e;e=gh_game_mt_next_0030f348(&s->random,&second);if(e)return e;
 unsigned remainder=(first%divisor)&255u;if(!(second&3))s->enemies[0x53f]=0;
 unsigned raw=s->enemies[0x53f];int threshold=raw<128?(int)raw:(int)raw-256;
 *original=threshold>=(int)remainder;if(*original)s->enemies[0x53f]=0;else s->enemies[0x53f]=(uint8_t)(raw+1);return GH_INIT_OK;
}
