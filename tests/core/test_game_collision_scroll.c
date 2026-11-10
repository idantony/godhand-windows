#include "game_collision_scroll.h"
#include "../platform/disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t w(const uint8_t*p){uint32_t v;memcpy(&v,p,4);return v;}
int main(int argc,char**argv){gh_disc*d;size_t i;gh_disc_entry e;C(argc==2&&!gh_disc_open(argv[1],&d)&&!gh_disc_find(d,"st1/r100.dat",&i)&&!gh_disc_entry_at(d,i,&e));uint8_t*p=malloc((size_t)e.size),*original=malloc((size_t)e.size);C(p&&original&&!gh_disc_read(d,i,0,p,(size_t)e.size));memcpy(original,p,(size_t)e.size);gh_bundle b;C(!gh_bundle_open(p,(size_t)e.size,&b));gh_game_scene_tables tables={0};C(!gh_scene_actors_0012e670(&tables,&b));gh_collision_scroll_mutation m={0};uint32_t changed=1;gh_scene_table_view before=tables.actor_links[4].view;
 C(!gh_collision_scroll_set_enabled_001332d0(&tables,&m,"notfound",0,&changed)&&!changed&&!m.owned);
 C(!gh_collision_scroll_set_enabled_001332d0(&tables,&m,"H03_Jfl2",0,&changed)&&changed==12&&m.owned&&tables.actor_links[4].view.bytes==m.owned&&m.original.bytes==before.bytes);
 for(unsigned n=0;n<680;n++){uint32_t old=w(before.bytes+n*20+16),now=w(tables.actor_links[4].view.bytes+n*20+16);C(now==(n>=385&&n<397?old|2u:old));}
 C(!gh_collision_scroll_set_enabled_001332d0(&tables,&m,"H03_Jfl3",0,&changed)&&changed==12);for(unsigned n=397;n<409;n++)C(w(tables.actor_links[4].view.bytes+n*20+16)&2);
 C(!gh_collision_scroll_set_enabled_001332d0(&tables,&m,"H03_Jfl2",1,&changed)&&changed==12);for(unsigned n=385;n<397;n++)C(!(w(tables.actor_links[4].view.bytes+n*20+16)&2));
 C(!memcmp(p,original,(size_t)e.size)&&!gh_collision_scroll_mutation_release(&m)&&tables.actor_links[4].view.bytes==before.bytes);
 C(!gh_collision_scroll_set_enabled_001332d0(&tables,&m,"H03_Jfl2",0,&changed)&&!gh_scene_actors_reset_0012e800(&tables)&&!gh_collision_scroll_mutation_release(&m)&&!tables.actor_links[4].view.bytes);free(original);free(p);gh_disc_close(d);puts("PASS realR100168namedgroups/680triangles exacttwo12triangle toggles, sharedcanonicalview notshadow, immutableasset and safe nativeborrow release/reset");return 0;}
