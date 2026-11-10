#include "game_message_step.h"
#include "disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint8_t*read_file(gh_disc*d,const char*p,size_t*n){size_t i;gh_disc_entry e;C(!gh_disc_find(d,p,&i)&&!gh_disc_entry_at(d,i,&e));*n=(size_t)e.size;uint8_t*b=malloc(*n);C(b&&!gh_disc_read(d,i,0,b,*n));return b;}
static void put(uint8_t*p,uint32_t n){memcpy(p,&n,4);}
int main(int argc,char**argv){C(argc==2);gh_disc*d;C(gh_disc_open(argv[1],&d)==GH_DISC_OK);size_t rn,ln;uint8_t*r=read_file(d,"st0/r020.dat",&rn),*l=read_file(d,"etc/core_usa.bin",&ln);gh_bundle room,language;C(!gh_bundle_open(r,rn,&room)&&!gh_bundle_open(l,ln,&language));gh_menu_state menu={0};C(!gh_menu_load_tables_002ae990(&menu,&language));gh_game_scene_ui ui={0};C(!gh_scene_message_initialize_002aea18(&ui,&menu,&room,1));gh_boot_work work={0};uint32_t frontier;
 gh_game_scene_ui before=ui;gh_menu_state oldmenu=menu;
 C(gh_scene_message_update_002aefa8(&ui,&work,&frontier)==GH_SCENE_UI_OK&&!frontier&&!memcmp(&before,&ui,sizeof(ui))&&!memcmp(&oldmenu,&menu,sizeof(menu)));
 /* Activeblockscalarfixture isnot updated byafake2B2438 callback. */
 ui.active_head=0;C(gh_scene_message_update_002aefa8(&ui,&work,&frontier)==GH_SCENE_UI_UNSUPPORTED&&frontier==0x2b2438);
 work.flags=0x80000;C(gh_scene_message_update_002aefa8(&ui,&work,&frontier)==GH_SCENE_UI_UNSUPPORTED); /* actualgateis5D8, not584 */
 put((uint8_t*)&work+0x5d8,0xf00c6000);ui.active_head=12345;ui.ready=0;before=ui;
 C(gh_scene_message_update_002aefa8(&ui,&work,&frontier)==GH_SCENE_UI_OK&&!frontier&&!memcmp(&before,&ui,sizeof(ui)));
 put((uint8_t*)&work+0x5d8,0);ui.ready=1;C(gh_scene_message_update_002aefa8(&ui,&work,&frontier)==GH_SCENE_UI_BOUNDS);
 free(l);free(r);gh_disc_close(d);puts("PASS actualroommessageowner/defaultNULLhead, original5D8gatesbeforeownerreads, active2B2438frontier; rootMSDuntouched");return 0;}
