#include "game_event_start.h"
#include "../platform/disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t rd(const void*p){uint32_t x;memcpy(&x,p,4);return x;}
static void wr(void*p,uint32_t x){memcpy(p,&x,4);}
typedef struct {uint8_t raw[0x600];unsigned calls;gh_game_event *event;gh_boot_work*work;} fixture;
static gh_game_event_status view(void*ctx,uint32_t handle,gh_title_player_view*out,uint32_t*f){fixture*s=ctx;(void)f;C(handle==9);*out=(gh_title_player_view){.bytes=s->raw,.size=sizeof(s->raw)};return GH_EVENT_OK;}
static gh_game_event_status menu(void*ctx,uint32_t*f){fixture*s=ctx;(void)f;s->calls++;C(s->event->fields.state==0&&s->event->fields.phase==1);C(rd((uint8_t*)s->work+0x5e4)&0x01000000);return GH_EVENT_OK;}
int main(int argc,char**argv){
 C(argc==2);gh_disc*d=NULL;C(!gh_disc_open(argv[1],&d));size_t n=(size_t)gh_disc_boot_elf_size(d);uint8_t*elf=malloc(n);C(elf&&!gh_disc_read_boot_elf(d,0,elf,n));gh_disc_close(d);
 gh_game_event event={0};gh_game_event_config config={0};C(!gh_game_event_from_elf(&event,elf,n));C(!gh_game_event_config_from_elf(&config,elf,n));
 gh_boot_work work={0};gh_screen_fade fade={0};gh_screen_fade_context fc={512,1824,0,0};gh_game_scene_tables tables={0};
 gh_room_pools pools={.initialized=1};uint32_t handle=9;pools.vectors[3]=(gh_room_vector){.allocation={.pointer=&handle},.count=1,.capacity=1};
 fixture f={.event=&event,.work=&work};f.raw[0x2ff]=3;wr(f.raw+0x5a0,0xffffffff);wr(f.raw+0x250,0x1234);for(unsigned i=0;i<32;i++)tables.scroll_link_words[i]=0xffffffff;
 gh_event_start_bindings b={.event=&event,.config=&config,.work=&work,.fade=&fade,.fade_context=&fc,.tables=&tables,.pools=&pools,.object_view=view,.context=&f};gh_event_start_state s={0};int done=99;
 C(!gh_event_start_002965f0(&s,&b,&done)&&!done&&event.fields.phase==1&&fade.time==15&&fade.state==11);
 C(!gh_event_start_002965f0(&s,&b,&done)&&!done&&!s.stage);gh_fade_draw draw;
 for(unsigned i=0;i<16;i++)C(!gh_screen_fade_step_00307388(&fade,&fc,&draw));
 C(gh_event_start_002965f0(&s,&b,&done)==GH_EVENT_UNSUPPORTED&&s.frontier==0x2b65b0&&!done&&s.stage==5);
 C(rd(f.raw+0x5a0)==0xfffffffd&&rd(f.raw+0x250)==0x9234&&fade.time==0&&fade.state==9);for(unsigned i=0;i<32;i++)C(!tables.scroll_link_words[i]);
 fade.count=7;b.system_menu_start=menu;C(!gh_event_start_002965f0(&s,&b,&done)&&done&&f.calls==1&&event.fields.state==1&&!event.fields.phase&&fade.count==7);
 uint32_t frontier=0;wr(f.raw+0x250,0);wr(f.raw+0x5b8,0x40);C(!gh_event_hide_scenario_objects_002c5068(&b,1,&frontier)&&!rd(f.raw+0x250));wr(f.raw+0x250,0xffff);C(!gh_event_hide_scenario_objects_002c5068(&b,0,&frontier)&&rd(f.raw+0x250)==0x7fff);
 const unsigned ids[]={0xff,0x100,0x1fe,0x1ff,0x200,0x2ff,0x300,0x4ff,0x500,0x5ff,0x600};
 for(unsigned i=0;i<sizeof(ids)/sizeof(ids[0]);i++){unsigned id=ids[i];f.raw[0x2fe]=(uint8_t)id;f.raw[0x2ff]=(uint8_t)(id>>8);wr(f.raw+0x5a0,2);C(!gh_event_clear_object_hidden_00308a38(&b,&frontier));C(rd(f.raw+0x5a0)==((id>=0x100&&id<0x600&&id!=0x1ff)?0u:2u));}
 gh_scene_meshes meshes={.pools=&pools};uint8_t node[0x408]={0};gh_scene_mesh_object object={.node=node,.constructor_complete=1};uint32_t scroll_handle=7;
 gh_room_object_binding native={.object=&object,.owner=&meshes,.kind=GH_ROOM_OBJECT_SCROLL_MESH,.handle=7};pools.objects=&native;pools.object_slots=1;pools.vectors[1]=(gh_room_vector){.allocation={.pointer=&scroll_handle},.count=1,.capacity=1};b.meshes=&meshes;
 wr(node+0x380,0xdeadbeef);C(!gh_event_clear_object_hidden_00308a38(&b,&frontier)&&rd(node+0x380)==0xdeadbeee);wr(node+0x404,1);C(gh_event_clear_object_hidden_00308a38(&b,&frontier)==GH_EVENT_UNSUPPORTED&&frontier==0x308a38);wr(node+0x404,0);
 native.owner=NULL;C(gh_event_clear_object_hidden_00308a38(&b,&frontier)==GH_EVENT_UNSUPPORTED&&frontier==0x14b6f8);native.owner=&meshes;
 event.fields.state=0;event.fields.phase=0;config.flags=2;b.system_menu_start=menu;C(!gh_event_start_002965f0(&s,&b,&done)&&done&&f.calls==2); /* actual same-call fallthrough */
 memset(&s,0,sizeof(s));event.fields.phase=0;wr((uint8_t*)&work+0x58c,0x80000000);C(!gh_event_start_002965f0(&s,&b,&done)&&!done&&event.fields.phase==1);event.fields.phase=2;C(!gh_event_start_002965f0(&s,&b,&done)&&!done);
 free(elf);puts("PASS actual ISO Event/config cold state, real15-step Fade wait, same-call ready, ordered native prefix/menu retry, positive canonical OM view ID boundaries/protectedhide; no clear/create claim");return 0;
}
