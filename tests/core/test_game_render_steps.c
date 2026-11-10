#include "../../src/core/game_render_steps.h"
#include "../../src/platform/disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL%d %s\n",__LINE__,#x);exit(1);}}while(0)
static void put(uint8_t*p,uint32_t n){memcpy(p,&n,4);}
static uint8_t*read_asset(gh_disc*d,const char*p,size_t*n){size_t i;gh_disc_entry e;C(!gh_disc_find(d,p,&i)&&!gh_disc_entry_at(d,i,&e));*n=(size_t)e.size;uint8_t*b=malloc(*n);C(b&&!gh_disc_read(d,i,0,b,*n));return b;}
int main(int argc,char**argv){
 C(argc==2);gh_disc*d;C(!gh_disc_open(argv[1],&d));size_t z=(size_t)gh_disc_boot_elf_size(d);uint8_t*elf=malloc(z);C(elf&&!gh_disc_read_boot_elf(d,0,elf,z));
 gh_render_draw_env env={0};C(!gh_render_draw_env_from_elf(&env,elf,z)&&env.ready&&!env.sprites.bytes&&!env.textures.bytes);free(elf);
 C(env.scalar.scale_x==0&&env.scalar.clip==0);size_t rn,ln,cn;uint8_t*r=read_asset(d,"st0/r020.dat",&rn),*l=read_asset(d,"etc/core_usa.bin",&ln),*c=read_asset(d,"etc/core.dat",&cn);gh_disc_close(d);
 gh_bundle room,language;C(!gh_bundle_open(r,rn,&room)&&!gh_bundle_open(l,ln,&language));
 gh_menu_state menu={0};C(!gh_menu_load_tables_002ae990(&menu,&language));gh_game_scene_ui ui={0};C(!gh_scene_message_initialize_002aea18(&ui,&menu,&room,1));
 env.sprites=menu.data;env.textures=menu.texture_data;env.scalar.reserved=0xa55a;
 C(!gh_render_draw_init_002c54e8(&env)&&env.scalar.shift_x==1&&env.scalar.shift_y==1&&env.scalar.scale_x==1&&env.scalar.scale_y==1&&env.scalar.alpha==1&&env.scalar.red==1&&env.scalar.green==1&&env.scalar.blue==1&&env.scalar.bank==6&&env.scalar.depth==5&&env.scalar.clip==0xffff&&env.scalar.reserved==0xa55a);
 C(env.sprites.bytes==menu.data.bytes&&env.textures.bytes==menu.texture_data.bytes);
 uint32_t f=99,slot;gh_game_scene_ui previous=ui;C(!gh_render_messages_002af028(&ui,&f)&&!f&&!memcmp(&ui,&previous,sizeof(ui)));
 ui.active_head=3;C(gh_render_messages_002af028(&ui,&f)==GH_RENDER_STEP_UNSUPPORTED&&f==0x2b16f0&&ui.active_head==3);ui.active_head=16;C(gh_render_messages_002af028(&ui,&f)==GH_RENDER_STEP_BOUNDS);ui=previous;
 uint8_t*a=calloc(1,0x100180),*b=calloc(1,0x26060);gh_game_effects*effects=calloc(1,sizeof(*effects));C(a&&b&&effects&&!gh_game_effects_initialize_00305a68(effects,a,0x100180,b,0x26060,c,cn,&room));
 gh_boot_work work={0};C(!gh_render_particles_002fb310(&work,effects,a,0x100180,&f,&slot)&&!f&&slot==UINT32_MAX);
 /* Original MSB-first bitsets, including31/32word boundary. Paused and
  * hidden eligible bits are suppressed; an unknown live slot never succeeds. */
 put(a+0x100000,1);C(gh_render_particles_002fb310(&work,effects,a,0x100180,&f,&slot)==GH_RENDER_STEP_UNSUPPORTED&&f==0x2fb310&&slot==31);
 put(a+0x100080,1);put(a+0x100004,0x80000000);C(gh_render_particles_002fb310(&work,effects,a,0x100180,&f,&slot)==GH_RENDER_STEP_UNSUPPORTED&&slot==32);
 put(a+0x100104,0x80000000);C(!gh_render_particles_002fb310(&work,effects,a,0x100180,&f,&slot));
 put((uint8_t*)&work+0x5e0,0xff800000);C(!gh_render_particles_002fb310(&work,NULL,NULL,0,&f,&slot));
 put((uint8_t*)&work+0x5e0,0);C(gh_render_particles_002fb310(&work,effects,a,0x10017f,&f,&slot)==GH_RENDER_STEP_BOUNDS);
 put((uint8_t*)&work+0x58c,0x80000000);C(!gh_render_cockpit_001410a8_gate(&work,&f)&&!gh_render_menu_00160b90_gate(&work,&f));
 put((uint8_t*)&work+0x58c,0);C(gh_render_cockpit_001410a8_gate(&work,&f)==GH_RENDER_STEP_UNSUPPORTED&&f==0x13cf20);C(gh_render_menu_00160b90_gate(&work,&f)==GH_RENDER_STEP_UNSUPPORTED&&f==0x160d50);
 put((uint8_t*)&work+0x5dc,0x100000);C(!gh_render_menu_00160b90_gate(&work,&f));
 free(effects);free(a);free(b);free(c);free(l);free(r);puts("PASS actual ELF shared DrawEnv/case3 R020 message+effects owners; pointer preservation, real gates/MSB bitsets and active frontiers");return 0;
}
