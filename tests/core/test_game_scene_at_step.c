#include "game_scene_at_step.h"
#include "../platform/disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fenv.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);return 1;}}while(0)
int main(int argc,char**argv){
 C(argc==2);gh_disc*d;C(!gh_disc_open(argv[1],&d));size_t i;gh_disc_entry e;
 C(!gh_disc_find(d,"st0/r020.dat",&i)&&!gh_disc_entry_at(d,i,&e));uint8_t*data=malloc((size_t)e.size);C(data&&!gh_disc_read(d,i,0,data,(size_t)e.size));gh_bundle room;
 C(!gh_bundle_open(data,(size_t)e.size,&room));gh_game_scene_tables actors={0};gh_scene_at_manager at={0};C(!gh_scene_actors_0012e670(&actors,&room)&&!gh_scene_at_initialize_002c0f90(&at,&room));
 uint8_t*memory=malloc(GH_TITLE_PLAYER_BYTES);C(memory);memset(memory,0xa5,GH_TITLE_PLAYER_BYTES);gh_title_player_view player;C(!gh_title_player_construct_00107e40(memory,GH_TITLE_PLAYER_BYTES,&player));
 at.field60=77;at.field64=88;gh_scene_at_manager original=at;int result=99;uint32_t frontier=99;
 fenv_t env;C(!fegetenv(&env));C(!fesetround(FE_DOWNWARD)&&!feraiseexcept(FE_INVALID));int flags=fetestexcept(FE_ALL_EXCEPT);
 C(!gh_scene_at_step_002c1618(&at,&actors,&player,&result,&frontier)&&!result&&!frontier&&actors.ray.native_queries==1&&!memcmp(&at,&original,sizeof(at)));
 C(fegetround()==FE_DOWNWARD&&fetestexcept(FE_ALL_EXCEPT)==flags);C(!fesetenv(&env));
 at.heads[15].kind=GH_AT_LINK_UNIT;C(gh_scene_at_step_002c1618(&at,&actors,&player,&result,&frontier)==GH_AT_BOUNDS&&frontier==0x2c30c8&&actors.ray.native_queries==2);at=original;
 memory[0x80]=1;C(!gh_scene_at_step_002c1618(&at,&actors,&player,&result,&frontier)&&!frontier&&actors.ray.native_queries==3);
 free(memory);free(data);gh_disc_close(d);puts("PASS actual R020 constructor aim ray before both empty interaction passes, canonical scratch, invalid-link frontier, finite general-worldtransform");return 0;
}
