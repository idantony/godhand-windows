#include "game_scene_at_step.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
static uint32_t word(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
static gh_scene_at_status stop(uint32_t*f,uint32_t pc,gh_scene_at_status status){if(f)*f=pc;return status;}
gh_scene_at_status gh_scene_at_step_002c1618(gh_scene_at_manager*s,gh_game_scene_tables*actors,const gh_title_player_view*player,int*out,uint32_t*f){
 gh_scene_ray_request ray={.to={0,.4f,1,1},.mask=7,.flags=0x10};float hit_position[4]={0,0,0,1};fenv_t env;int hit;
 if(f)*f=0;if(!s||!s->ready||!actors||!player||!player->bytes||player->size<GH_TITLE_PLAYER_BYTES||!out)return GH_AT_ARGUMENT;
 *out=0;
 if(player->alias_f0!=player->bytes+0xb0)return stop(f,0x2db6b0,GH_AT_ARGUMENT);
 /* Actual1508D8->30A2B0 applies world matrix+80 to (0,.4,1,1).
  * Constructor identity makes that result exact; other poses need the real
  * general transform instead of inventing an aim direction. */
 for(unsigned i=0;i<16;++i)if(word(player->bytes+0x80+i*4)!=(i%5==0?0x3f800000u:0u))return stop(f,0x1508d8,GH_AT_UNSUPPORTED);
 memcpy(ray.from,player->alias_f0,12);ray.from[3]=1;
 for(unsigned i=0;i<3;++i)if(!isfinite(ray.from[i]))return stop(f,0x2c1618,GH_AT_UNSUPPORTED);
 if(feholdexcept(&env))return GH_AT_BOUNDS;
 if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return GH_AT_BOUNDS;}
 volatile float y=ray.from[1]+.4f;ray.from[1]=y;
 if(fesetenv(&env))return GH_AT_BOUNDS;
 ray.hit_position=hit_position;
 gh_scene_ray_status status=gh_scene_ray_query_00131e10(actors,&ray,&hit,f);
 if(status)return status==GH_RAY_UNSUPPORTED?GH_AT_UNSUPPORTED:GH_AT_BOUNDS;
 if(hit)return stop(f,0x2c1618,GH_AT_UNSUPPORTED);
 /* Both original tagged-list passes visit no units in this verified domain.
  * Check actual links as well as resource counts; neither alone proves empty. */
 for(unsigned i=0;i<4;++i)if(s->units[i].count)return stop(f,0x2c30c8,GH_AT_UNSUPPORTED);
 for(unsigned i=0;i<16;++i)if(s->heads[i].kind!=(i?GH_AT_LINK_SLOT:GH_AT_LINK_END)||(i&&s->heads[i].index!=i-1))return stop(f,0x2c30c8,GH_AT_UNSUPPORTED);
 return GH_AT_OK;
}
