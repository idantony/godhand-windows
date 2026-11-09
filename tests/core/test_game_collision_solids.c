#include "game_collision_solids.h"
#include <stdio.h>
#include <stdint.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);return 1;}}while(0)
int main(void){uint32_t foreign[32];gh_game_collision_solids s={.unit_head=foreign,.unit_count=8,.field88=17,.global_00460d18=99};for(unsigned i=0;i<32;i++){foreign[i]=i+0x12340000;s.words08[i]=i+0x12340000;}gh_game_collision_solids_reset_00134690(&s);CHECK(!s.unit_head&&!s.unit_count&&!s.field88&&!s.global_00460d18);for(unsigned i=0;i<32;i++)CHECK(!s.words08[i]&&foreign[i]==i+0x12340000);gh_game_collision_solids_reset_00134690(&s);puts("PASS canonicalcollisionSolid reset dropsreferences withoutdestroyingforeignobjects");return 0;}
