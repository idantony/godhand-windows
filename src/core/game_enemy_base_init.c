#include "game_enemy_base_init.h"
#include <string.h>
static uint32_t word(const uint8_t*p,size_t o){uint32_t v;memcpy(&v,p+o,4);return v;}
static void put(uint8_t*p,size_t o,uint32_t v){memcpy(p+o,&v,4);}
static void half(uint8_t*p,size_t o,uint16_t v){memcpy(p+o,&v,2);}
static uint32_t random_word(void*context){uint32_t v=0;(void)gh_game_mt_next_0030f348(context,&v);return v;}
gh_actor_model_status gh_enemy_base_initialize_0028ebf0(gh_enemy_base_initialize*s,const gh_title_player_view*root,const gh_enemy_request*r,uint32_t argument,gh_game_cockpit*c,gh_game_mt*mt,int*original){
 if(!s||!root||!root->bytes||root->size<0x670||!root->alias_f0||!r||!original)return GH_ACTOR_MODEL_ARGUMENT;*original=0;if(s->stage&&s->root!=root)return GH_ACTOR_MODEL_BUSY;
 uint8_t*p=root->bytes;if(!s->stage){s->root=root;memcpy(root->alias_f0,r->position_xyz,12);memcpy(p+0x490,root->alias_f0,12);put(p,0x104,r->yaw_bits);put(p,0x628,argument);p[0x531]=3;put(p,0x644,0x3f800000);put(p,0x610,0);p[0x614]=p[0x615]=p[0x616]=p[0x623]=p[0x617]=p[0x622]=0;half(p,0x62c,0);put(p,0x630,0x3f800000);put(p,0x660,0);s->stage=1;}
 if(s->stage==1){if(!c||!mt){s->frontier=0x2abbe8;return GH_ACTOR_MODEL_UNSUPPORTED;}uint16_t kind;memcpy(&kind,p+0x2fe,2);unsigned offset=kind>=0x2a0&&kind<=0x2ff?0xe50:0xe00;
  gh_id_base*base;if(gh_game_cockpit_base(c,offset,&base)){s->frontier=0x2abbe8;return GH_ACTOR_MODEL_UNSUPPORTED;}
  gh_id_error e=gh_id_base_restart_anim(base,random_word,mt);if(e){s->frontier=base->first_unported?base->first_unported:0x2aba78;return GH_ACTOR_MODEL_UNSUPPORTED;}base->playing=1;s->cockpit_base=base;s->stage=2;}
 put(p,0x3a8,0x3f800000);put(p,0x254,word(p,0x254)|0x8000000);half(p,0x570,0xffff);put(p,0x574,2);s->frontier=0;*original=1;return GH_ACTOR_MODEL_OK;
}
