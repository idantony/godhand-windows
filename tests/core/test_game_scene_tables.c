#include "game_scene_tables.h"
#include "disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t u32(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
static void put(uint8_t*p,uint32_t n){memcpy(p,&n,4);}
int main(int argc,char**argv){gh_disc*d;CHECK(argc==2&&gh_disc_open(argv[1],&d)==GH_DISC_OK);size_t idx;gh_disc_entry de;CHECK(gh_disc_find(d,"st0/r020.dat",&idx)==GH_DISC_OK&&gh_disc_entry_at(d,idx,&de)==GH_DISC_OK);uint8_t*r=malloc((size_t)de.size),*backup=malloc((size_t)de.size);CHECK(r&&backup&&gh_disc_read(d,idx,0,r,(size_t)de.size)==GH_DISC_OK);memcpy(backup,r,(size_t)de.size);gh_bundle b;CHECK(gh_bundle_open(r,(size_t)de.size,&b)==GH_BUNDLE_OK);
 gh_game_scene_tables s={0};gh_boot_work w;memset(&w,0xa5,sizeof(w));memset(s.scroll_pairs,0x5a,sizeof(s.scroll_pairs));memset(s.scroll_words,0x5a,sizeof(s.scroll_words));
 CHECK(gh_scene_environment_002a71b0(&s,&w,&b)==GH_TABLE_OK&&!s.environment.bytes&&!u32((uint8_t*)&w+0x608));
 CHECK(gh_scene_scrolls_0031d1b0(&s,&w,&b)==GH_TABLE_OK&&!s.scroll_texture.bytes&&!u32((uint8_t*)&w+0x5fc)&&!u32((uint8_t*)&w+0x600));
 for(unsigned i=0;i<32;i++)CHECK(!s.scroll_pairs[i][0]&&!s.scroll_pairs[i][1]&&!s.scroll_words[i]);
 CHECK(gh_scene_actors_0012e670(&s,&b)==GH_TABLE_OK&&s.actors_ready&&s.akg.bytes==r+0x6140&&s.akt.bytes==r+0x6120);
 static const uint32_t offsets[8]={0x68,0x7c,0x8c,0xbc,0xc8,0xdc,0xfc,0xffffffff};
 for(unsigned i=0;i<8;i++){CHECK(s.actor_links[i].relative_word==offsets[i]);CHECK(s.actor_links[i].view.bytes==s.akg.bytes+(i==7?-1:(int)offsets[i]));}
 CHECK(s.actor_links[7].view.bytes==r+0x613f&&s.actor_count60==2&&s.actor_count64==1&&s.actor_count68==2);
 CHECK(s.actor_vec30[0]==0xc0800000&&s.actor_vec40[0]==0x40800000&&s.actor_vec50[0]==0x40a00000);
 memset(s.scroll_link_words,0x5a,sizeof(s.scroll_link_words));CHECK(gh_scene_actor_scroll_links_00307a60(&s)==GH_TABLE_OK);for(unsigned i=0;i<32;i++)CHECK(!s.scroll_link_words[i]);
 CHECK(!memcmp(r,backup,(size_t)de.size));
 gh_game_scene_tables busy={.pending_actor_head=&w,.pending_actor_count=1};CHECK(gh_scene_actors_0012e670(&busy,&b)==GH_TABLE_UNSUPPORTED&&!busy.actors_ready&&busy.frontier==0x12ecf0&&busy.pending_actor_head==&w&&busy.pending_actor_count==1);
 CHECK(gh_scene_actors_0012e670(&busy,&b)==GH_TABLE_UNSUPPORTED);
 ((uint8_t*)s.akg.bytes)[0x2a]=1;memset(s.scroll_link_words,0x5a,sizeof(s.scroll_link_words));CHECK(gh_scene_actor_scroll_links_00307a60(&s)==GH_TABLE_UNSUPPORTED&&s.frontier==0x2c5420&&s.scroll_link_words[0]==0x5a5a5a5a);
 ((uint8_t*)s.akg.bytes)[0x2a]=0;put((uint8_t*)s.akg.bytes,0x80000000);gh_game_scene_tables malformed={0};CHECK(gh_scene_actors_0012e670(&malformed,&b)==GH_TABLE_BOUNDS&&!malformed.akg.bytes);
 free(r);free(backup);gh_disc_close(d);puts("PASS actualR020 ENV/SCPabsence, immutableAKGlinks inclheader-1/scalars/clear and livefrontiers");return 0;}
