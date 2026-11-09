#include "game_scene_ui.h"
#include "disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint8_t*read_file(gh_disc*d,const char*p,size_t*n){size_t i;gh_disc_entry e;CHECK(!gh_disc_find(d,p,&i)&&!gh_disc_entry_at(d,i,&e));*n=(size_t)e.size;uint8_t*b=malloc(*n);CHECK(b&&!gh_disc_read(d,i,0,b,*n));return b;}
int main(int argc,char**argv){
 gh_disc*d;CHECK(argc==2&&!gh_disc_open(argv[1],&d));size_t rn,ln;uint8_t*r=read_file(d,"st0/r020.dat",&rn),*l=read_file(d,"etc/core_usa.bin",&ln);gh_bundle room,language;
 CHECK(!gh_bundle_open(r,rn,&room)&&!gh_bundle_open(l,ln,&language));gh_font_state font={0};gh_menu_state menu={0},prior_menu;
 CHECK(!gh_font_initialize(&font,&language,1)&&!gh_menu_load_tables_002ae990(&menu,&language));prior_menu=menu;
 gh_bundle_entry mst,msa;CHECK(!gh_bundle_find(&language,"MST",0,&mst)&&!gh_bundle_find(&language,"MSA",0,&msa));
 font.draw_word_2a08c=0xdeadbeef;CHECK(!gh_font_bind_pair_002a4cb0(&font,2,&mst,&msa)&&font.slots[2].bound&&font.draw_word_2a08c==0xdeadbeef);
 gh_font_state prior=font;CHECK(!gh_font_bind_pair_002a4cb0(&font,2,NULL,&msa)&&!memcmp(&prior,&font,sizeof(font)));
 gh_bundle_entry absent;CHECK(gh_bundle_find(&room,"MST",1,&absent)==GH_BUNDLE_NOT_FOUND&&gh_bundle_find(&room,"MSA",1,&absent)==GH_BUNDLE_NOT_FOUND);
 CHECK(!gh_scene_font_bind_002a4c38(&font,&room,1)&&!memcmp(&font,&prior,sizeof(font)));
 gh_game_scene_ui ui={0};memset(ui.blocks,0xa5,sizeof(ui.blocks));CHECK(!gh_scene_message_initialize_002aea18(&ui,&menu,&room,1));
 CHECK(ui.root==&menu&&ui.ready&&ui.free_head==0&&ui.active_head==-1&&ui.generation_bd8==1&&!memcmp(&menu,&prior_menu,sizeof(menu)));
 for(unsigned i=0;i<16;i++){CHECK(!ui.blocks[i].flags&&!ui.blocks[i].mode&&ui.blocks[i].previous==-1&&ui.blocks[i].next==(i<15?(int)i+1:-1));for(unsigned j=0;j<sizeof(ui.blocks[i].payload);j++)CHECK(ui.blocks[i].payload[j]==0xa5);}
 gh_bundle_entry msd={0};gh_bundle_error result=gh_bundle_find(&room,"MSD",1,&msd);CHECK(result==GH_BUNDLE_OK||result==GH_BUNDLE_NOT_FOUND);CHECK(ui.room_data.bytes==msd.bytes&&ui.room_data.size==msd.size);
 gh_menu_state foreign={0};CHECK(gh_scene_message_initialize_002aea18(&ui,&foreign,&room,1)==GH_SCENE_UI_ARGUMENT&&ui.root==&menu);
 free(l);free(r);gh_disc_close(d);puts("PASS realR020 roomFontNULLpreserve/sharedbinder/roomMSD/16messageheaders/freechain/payloadpreserve/rootidentity");return 0;
}
