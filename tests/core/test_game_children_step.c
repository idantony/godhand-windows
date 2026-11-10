#include "game_children.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t w(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
static void put(uint8_t*p,size_t o,uint32_t n){memcpy(p+o,&n,4);}
static void half(uint8_t*p,size_t o,uint16_t n){memcpy(p+o,&n,2);}
int main(void){
 gh_game_children*s;C(gh_game_children_create(NULL,&s)==GH_CHILD_OK);gh_boot_work work={0};uint32_t frontier;const uint8_t*view;size_t size;
 /* Originalglobal58Csign gate executesbeforeactivechildlookup. */
 put((uint8_t*)&work,0x58c,0x80000000);C(gh_game_children_move_001601d8(s,&work,&frontier)==GH_CHILD_OK&&!frontier);
 put((uint8_t*)&work,0x58c,0);put((uint8_t*)&work,0x588,0x80000000);C(gh_game_children_move_001601d8(s,&work,&frontier)==GH_CHILD_BOUNDS);
 put((uint8_t*)&work,0x588,0);
 C(gh_game_children_initialize_0015fd98(s)==GH_CHILD_OK&&gh_game_children_manager_view(s,&view,&size)==GH_CHILD_OK&&size==0xf18);
 uint8_t before[0xf18];memcpy(before,view,size);gh_id_base bases[9];
 for(unsigned i=0;i<9;i++){gh_game_child_view child;C(gh_game_child_at(s,i,&child)==GH_CHILD_OK&&child.native_base);bases[i]=*child.native_base;}
 work.flags=0x80092c00;put((uint8_t*)&work,0x5d8,0xf00c6000);gh_boot_work work_before=work;
 const gh_game_child_view*active;C(gh_game_children_active_00161288(s,&active)==GH_CHILD_OK&&!active);
 for(unsigned i=0;i<100;i++)C(gh_game_children_move_001601d8(s,&work,&frontier)==GH_CHILD_OK&&!frontier);
 C(!memcmp(before,view,size)&&!memcmp(&work_before,&work,sizeof(work)));
 for(unsigned i=0;i<9;i++){gh_game_child_view child;C(gh_game_child_at(s,i,&child)==GH_CHILD_OK&&!memcmp(&bases[i],child.native_base,sizeof(bases[i])));}
 /* Testfixtures below set originalscalarpreconditions on thisownedinstance;
  * they never supply fakechildcallbacks or allocate alternate IDowners. */
 uint8_t*p=(uint8_t*)(void*)view;
 put(p,0x48,0);C(gh_game_children_active_00161288(s,&active)==GH_CHILD_OK&&active&&active->kind==0&&active->bytes==p+0x60);
 gh_id_base*base;C(gh_game_children_id_base(s,0x60,&base)==GH_CHILD_OK&&active->native_base==base);
 half(p,0x50,1);p[0x60+0x58]=1;memcpy(before,p,size);C(gh_game_children_move_001601d8(s,&work,&frontier)==GH_CHILD_OK&&!memcmp(before,p,size));p[0x60+0x58]=0;
 C(gh_game_children_move_001601d8(s,&work,&frontier)==GH_CHILD_UNSUPPORTED&&frontier==0x1601d8);
 half(p,0x50,5);put((uint8_t*)&work,0x5dc,0x100000);C(gh_game_children_move_001601d8(s,&work,&frontier)==GH_CHILD_OK);put((uint8_t*)&work,0x5dc,0);
 C(gh_game_children_move_001601d8(s,&work,&frontier)==GH_CHILD_UNSUPPORTED&&frontier==0x1601d8);
 put(p,0x48,0xffffffff);half(p,0x50,2);C(gh_game_children_move_001601d8(s,&work,&frontier)==GH_CHILD_OK&&p[0x50]==3);C(gh_game_children_move_001601d8(s,&work,&frontier)==GH_CHILD_OK&&p[0x50]==4);
 C(gh_game_children_move_001601d8(s,&work,&frontier)==GH_CHILD_UNSUPPORTED&&frontier==0x1601d8); /* realFade required */
 half(p,0x50,9);put(p,0xee4,0x12345678);C(gh_game_children_move_001601d8(s,&work,&frontier)==GH_CHILD_OK&&w((uint8_t*)&work+0x5e0)==0x12345678&&w(p+0x48)==0xffffffff&&p[0x50]==0x15);
 half(p,0x50,7);put(p,0x4c,0xb);work.flags=0;C(gh_game_children_move_001601d8(s,&work,&frontier)==GH_CHILD_OK&&work.flags==0x8000&&w(p+0x48)==0xb);
 for(unsigned phase=2;phase<=3;phase++){
  p[0xed4]=(uint8_t)phase;put(p,0xed8,0);put(p,0xed0,0x99112233);
  for(unsigned count=0;count<=4;count++){
   C(gh_game_children_alpha_step_00160c48(s,&frontier)==GH_CHILD_OK&&!frontier);
   unsigned alpha=phase==2?count*32:128-count*32;
   C(w(p+0xed0)==(0x00112233u|alpha<<24)&&w(p+0xed8)==count+1);
  }
  C(p[0xed4]==(phase==2?0:1));memcpy(before,p,size);C(gh_game_children_alpha_step_00160c48(s,&frontier)==GH_CHILD_OK&&!memcmp(before,p,size));
 }
 p[0xed4]=255;put(p,0xed8,0xffffffff);put(p,0xed0,0xaabbccdd);C(gh_game_children_alpha_step_00160c48(s,&frontier)==GH_CHILD_OK&&!w(p+0xed8)&&w(p+0xed0)==0xaabbccdd);
 p[0xed4]=2;put(p,0xed8,0xffffffff);memcpy(before,p,size);C(gh_game_children_alpha_step_00160c48(s,&frontier)==GH_CHILD_UNSUPPORTED&&frontier==0x160c48&&!memcmp(before,p,size));
 p[0xed4]=2;put(p,0xed8,1);half(p,0x50,0);work.flags=0x20000;
 C(gh_game_children_move_001601d8(s,&work,&frontier)==GH_CHILD_UNSUPPORTED&&frontier==0x2c7850&&w(p+0xed8)==2&&w(p+0xed0)==0x20bbccdd);
 gh_game_children_destroy(s);puts("PASS actualR020 coldchildrenstate, same9owners/nativebases, selectedchild/gates, nullselectedtransitions, exactquarteralpha andsourcefrontiers");return 0;
}
