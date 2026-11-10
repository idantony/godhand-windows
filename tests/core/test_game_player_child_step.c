#include "game_player_child_step.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#define C(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static void put(uint8_t*p,uint32_t n){memcpy(p,&n,4);}
int main(void){uint8_t bytes[0x300],snapshot[0x300];memset(bytes,0xa5,sizeof(bytes));uint16_t kind=0x101;memcpy(bytes+0x2fe,&kind,2);gh_title_player_view view={0};view.bytes=bytes;view.size=sizeof(bytes);gh_boot_work work;memset(&work,0xa5,sizeof(work));uint8_t*w=(uint8_t*)&work;put(w+0x5d8,0xf00c6000);put(w+0x5e4,0);memcpy(snapshot,bytes,sizeof(bytes));gh_boot_work before=work;int skipped;uint32_t frontier;
 C(!gh_game_player_child_move_gate_00100530(&view,&work,&skipped,&frontier)&&skipped&&!frontier&&!memcmp(bytes,snapshot,sizeof(bytes))&&!memcmp(&work,&before,sizeof(work)));
 /* Adjacent flags584/58C/5DC/5E0 cannot substitute actual747A78/747A84. */
 put(w+0x584,0);put(w+0x58c,0);put(w+0x5dc,0);put(w+0x5e0,0x20000000);C(!gh_game_player_child_move_gate_00100530(&view,&work,&skipped,&frontier)&&skipped);put(w+0x5e4,0x20000000);C(gh_game_player_child_move_gate_00100530(&view,&work,&skipped,&frontier)==GH_PLAYER_UNSUPPORTED&&!skipped&&frontier==0x100590);put(w+0x5d8,0);put(w+0x5e4,0);C(gh_game_player_child_move_gate_00100530(&view,&work,&skipped,&frontier)==GH_PLAYER_UNSUPPORTED&&!skipped&&frontier==0x100590);
 kind=0x100;memcpy(bytes+0x2fe,&kind,2);put(w+0x5d8,0x40000000);C(!gh_game_player_child_move_gate_00100530(&view,&work,&skipped,&frontier)&&skipped);put(w+0x5d8,0x80000000);C(gh_game_player_child_move_gate_00100530(&view,&work,&skipped,&frontier)==GH_PLAYER_UNSUPPORTED&&!skipped&&frontier==0x100594);view.size=0x2ff;skipped=99;frontier=0x55;C(gh_game_player_child_move_gate_00100530(&view,&work,&skipped,&frontier)==GH_PLAYER_ARGUMENT&&skipped==99&&frontier==0x55);puts("PASS actual100530 child101/non101 SystemFlags5D8/5E4 gate, poisonedneighbors/actorWorkimmutable/activeprecisefrontier/boundsatomic; noactiveclothclaim");return 0;}
