#include "game_scene_finish.h"
#include "disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
int main(int argc,char**argv){gh_disc*d;size_t i;gh_disc_entry e;CHECK(argc==2&&!gh_disc_open(argv[1],&d)&&!gh_disc_find(d,"st0/r020.dat",&i)&&!gh_disc_entry_at(d,i,&e));
 uint8_t*pack=malloc((size_t)e.size);gh_bundle b;gh_game_world_light*s=calloc(1,sizeof(*s));CHECK(pack&&s&&!gh_disc_read(d,i,0,pack,(size_t)e.size)&&!gh_bundle_open(pack,(size_t)e.size,&b));
 CHECK(!gh_game_world_light_construct_002d6d80(s)&&!gh_game_world_light_initialize_002d6f40(s,&b));
 uint8_t*prior=malloc(sizeof(s->scalar));CHECK(prior);memcpy(prior,s->scalar,sizeof(s->scalar));
 CHECK(!gh_scene_world_light_select_002d8408(s,0));const uint8_t*q=prior+0x15d00;
 CHECK(!memcmp(s->scalar+0x30,q,12)&&!memcmp(s->scalar+0x40,q+0x10,12)&&!memcmp(s->scalar,q+0x30,32)&&!memcmp(s->scalar+0x20,q+0x50,8)&&!memcmp(s->scalar+0x60,q+0x58,24));
 for(size_t at=0;at<sizeof(s->scalar);at++){int changed=at<0x28||(at>=0x30&&at<0x3c)||(at>=0x40&&at<0x4c)||(at>=0x60&&at<0x78)||(at>=0x16288&&at<0x1628c);if(!changed)CHECK(s->scalar[at]==prior[at]);}
 uint32_t positive=1;memcpy(s->scalar+0x15d70,&positive,4);CHECK(gh_scene_world_light_select_002d8408(s,0)==GH_LIGHT_UNSUPPORTED&&s->frontier==0x2d8408);
 uint8_t flag=0xa5;gh_scene_system_menu_enable_002b65f0(&flag);CHECK(flag==1);
 free(prior);free(s);free(pack);gh_disc_close(d);puts("PASS realR020 WorldLight preset0 scalarcopy/preservedfields/positive-domain rejection andtrue2B65F0leaf");return 0;}
