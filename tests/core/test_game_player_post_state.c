#include "game_player_post_state.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL%d %s\n",__LINE__,#x);return 1;}}while(0)
static uint32_t word(const uint8_t *p){uint32_t v;memcpy(&v,p,4);return v;}
static void put(uint8_t *p,uint32_t v){memcpy(p,&v,4);}
int main(void){
 uint8_t bytes[GH_TITLE_PLAYER_BYTES]={0};gh_title_player_view v={.bytes=bytes,.size=sizeof(bytes)};
 gh_game_player_post_state s={0};bytes[0x2f4]=1;bytes[0x2f5]=6;put(bytes+0x15f4,0x80000000u);
 C(gh_player_state_effect_0012bb38(&s,&v,NULL)==GH_PLAYER_POST_UNSUPPORTED&&s.frontier==0x306900&&word(bytes+0x15f4)==0x80000040u);
 put(bytes+0x15f4,0x200);C(gh_player_state_effect_0012bb38(&s,&v,NULL)==GH_PLAYER_POST_UNSUPPORTED&&s.frontier==0x306900&&word(bytes+0x15f4)==0x200); /* committedprefix is not replayed */
 s=(gh_game_player_post_state){0};put(bytes+0x15f4,0x140);C(!gh_player_state_effect_0012bb38(&s,&v,NULL)&&s.effect_phase==2&&word(bytes+0x15f4)==0x140);
 s=(gh_game_player_post_state){0};bytes[0x2f4]=0;bytes[0x2f5]=0;put(bytes+0x15f4,0x80000140u);put(bytes+0x2f0,0xffffffffu);
 C(gh_player_state_effect_0012bb38(&s,&v,NULL)==GH_PLAYER_POST_UNSUPPORTED&&s.frontier==0x306200&&word(bytes+0x15f4)==0x80000100u);
 gh_game_effects *effects=calloc(1,sizeof(*effects));uint8_t *esp=calloc(1,0x100180),*emitters=calloc(1,0x26060);C(effects&&esp&&emitters);effects->ready=1;
 gh_effect_release_bindings storage={.effects=effects,.esp=esp,.esp_bytes=0x100180,.emitters=emitters,.emitter_bytes=0x26060};
 C(!gh_player_state_effect_0012bb38(&s,&v,&storage)&&s.effect_phase==2&&!s.frontier&&word(bytes+0x15f4)==0x80000100u);
 put(bytes+0x15f4,0x40);C(!gh_player_state_effect_0012bb38(&s,&v,NULL)&&word(bytes+0x15f4)==0x40); /* completed invocation is stable */
 s=(gh_game_player_post_state){0};gh_title_player_view short_view=v;short_view.size=0x15f7;C(gh_player_state_effect_0012bb38(&s,&short_view,NULL)==GH_PLAYER_POST_ARGUMENT&&word(bytes+0x15f4)==0x40);
 s=(gh_game_player_post_state){0};bytes[0x2f4]=0x61;bytes[0x2f5]=0;put(bytes+0x15f4,0);C(!gh_player_state_effect_0012bb38(&s,&v,NULL)&&!word(bytes+0x15f4));
 gh_game_init_state *init=calloc(1,sizeof(*init));C(init);const int32_t modes[]={0,1,2,3,-1,INT32_MIN};uint32_t result;
 for(unsigned i=0;i<sizeof(modes)/sizeof(modes[0]);i++){init->camera.mode=modes[i];C(!gh_player_camera_mode_00123908(init,&result)&&result==((uint32_t)modes[i]&1u)&&init->camera.mode==modes[i]);}
 free(init);free(emitters);free(esp);free(effects);puts("PASS actual12BB38 state601 gate, committedeffect-prefix retry, genuineemptyrelease, boundedview, exact138F88 mask");return 0;
}
