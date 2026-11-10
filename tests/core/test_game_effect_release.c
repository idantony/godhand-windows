#include "game_effect_release.h"
#include "disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL%d %s\n",__LINE__,#x);exit(1);}}while(0)
static void wr(uint8_t*p,uint32_t x){memcpy(p,&x,4);}
static uint8_t*load(gh_disc*d,const char*path,size_t*n){size_t i;gh_disc_entry e;C(!gh_disc_find(d,path,&i)&&!gh_disc_entry_at(d,i,&e));*n=(size_t)e.size;uint8_t*b=malloc(*n);C(b&&!gh_disc_read(d,i,0,b,*n));return b;}
static gh_effect_status identity(void*ctx,int emitter,unsigned slot,const gh_title_player_view*actor,int*match,uint32_t*f){(void)f;C(actor&&actor->bytes==ctx);C(emitter==0||emitter==1);C(slot==31);*match=1;return GH_EFFECT_OK;}
int main(int argc,char**argv){C(argc==2);gh_disc*d;C(!gh_disc_open(argv[1],&d));size_t cn,rn;uint8_t*core=load(d,"etc/core.dat",&cn),*r=load(d,"st1/r100.dat",&rn);gh_disc_close(d);gh_bundle room;C(!gh_bundle_open(r,rn,&room));gh_game_effects*effects=calloc(1,sizeof(*effects));uint8_t*a=calloc(1,0x100180),*b=calloc(1,0x26060);C(effects&&a&&b&&!gh_game_effects_initialize_00305a68(effects,a,0x100180,b,0x26060,core,cn,&room));
 uint8_t raw[0x600]={0};gh_title_player_view actor={.bytes=raw,.size=sizeof(raw)};gh_effect_release_bindings bindings={.effects=effects,.esp=a,.emitters=b,.esp_bytes=0x100180,.emitter_bytes=0x26060};uint32_t frontier;unsigned slot;
 for(unsigned mode=0;mode<3;mode++)C(!gh_game_effect_release_00306200(&bindings,&actor,0,mode,&frontier,&slot)&&!frontier); /* slot0/kind0 not automaticallyempty: scans actualresetmaps */
 wr(b+0x26000,1);C(gh_game_effect_release_00306200(&bindings,&actor,0,0,&frontier,&slot)==GH_EFFECT_UNSUPPORTED&&frontier==0x3039e8&&slot==31);
 wr(b+0x26020,1);C(!gh_game_effect_release_00306200(&bindings,&actor,0,0,&frontier,&slot));wr(b+0x26020,0);wr(b+0x26040,1);C(!gh_game_effect_release_00306200(&bindings,&actor,0,0,&frontier,&slot));wr(b+0x26040,0);
 wr(b+31*0x260+0x110,1);C(!gh_game_effect_release_00306200(&bindings,&actor,0,0,&frontier,&slot));wr(b+31*0x260+0x110,0);bindings.actor_match=identity;bindings.context=raw;C(gh_game_effect_release_00306200(&bindings,&actor,0,0,&frontier,&slot)==GH_EFFECT_UNSUPPORTED&&frontier==0x302918);
 wr(b+0x26000,0);wr(a+0x100000,1);C(gh_game_effect_release_00306200(&bindings,&actor,0,2,&frontier,&slot)==GH_EFFECT_UNSUPPORTED&&frontier==0x2fbe50&&slot==31);
 C(!gh_game_effect_release_00306200(NULL,NULL,-1,0,&frontier,&slot));C(!gh_game_effect_release_00306200(NULL,&actor,0,3,&frontier,&slot));bindings.esp_bytes--;C(gh_game_effect_release_00306200(&bindings,&actor,0,0,&frontier,&slot)==GH_EFFECT_ARGUMENT);
 free(core);free(r);free(effects);free(a);free(b);puts("PASS actualR100/core effects initialization +real306200 cold occupancy scans, kind0 active31 strictidentity/destructorfrontiers, paused/hidden/kind mismatch gates, modeorder and bounds; no fakekill");return 0;}
