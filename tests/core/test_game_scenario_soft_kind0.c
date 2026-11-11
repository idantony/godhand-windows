#include "game_scenario_soft_event.h"
#include "game_scenario_end_soft_event.h"
#include "game_player_abort.h"
#include "game_player_child.h"
#include "disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) do{if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t word(const void *p){uint32_t n;memcpy(&n,p,4);return n;}
static void put(void *p,uint32_t n){memcpy(p,&n,4);}
typedef struct {
 gh_game_title_player player;
 gh_title_player_view object,particle,child;
 gh_game_init_state *manager;
 gh_boot_work *work;
 gh_event_start_bindings *objects;
 gh_sound_control sound;
 gh_game_player_abort abort;
 uint32_t object_handle,particle_handle;
 unsigned calls[4];
} fixture;
static gh_game_event_status view(void *context,uint32_t handle,gh_title_player_view *out,uint32_t *frontier)
{
 fixture *f=context;
 *frontier=0;
 if(handle==f->object_handle)*out=f->object;
 else if(handle==f->particle_handle)*out=f->particle;
 else return GH_EVENT_ARGUMENT;
 return GH_EVENT_OK;
}
static gh_soft_event_status abort_player(void *context,uint32_t *frontier)
{
 fixture *f=context;
 ++f->calls[0];
 gh_player_abort_bindings b={.player=&f->player,.sound=&f->sound};
 gh_player_abort_status status=gh_game_player_abort_00126770(&f->abort,&b);
 *frontier=f->abort.frontier;
 return status?GH_SOFT_EVENT_CALLEE:GH_SOFT_EVENT_OK;
}
static gh_soft_event_status empty_enemies(void *context,uint32_t *frontier)
{
 fixture *f=context;
 ++f->calls[1];
 CHECK(!f->objects->tables->scroll_link_words[0]);
 CHECK(!(word(f->object.bytes+0x5a0)&2));
 CHECK(!(word(f->object.bytes+0x250)&0x8000));
 /* The original empty 2948E8 branch does not read its suspend argument. */
 return gh_soft_enemies_unsuspend_empty_002948e8(f->manager,frontier)?
     GH_SOFT_EVENT_UNSUPPORTED:GH_SOFT_EVENT_OK;
}
static gh_soft_event_status effects(void *context,uint32_t *frontier)
{
 fixture *f=context;
 ++f->calls[2];
 CHECK(word(f->object.bytes+0x250)&0x8000);
 return gh_soft_event_suspend_effects_002fa470(f->objects,frontier);
}
static gh_soft_event_status player(void *context,uint32_t *frontier)
{
 fixture *f=context;
 ++f->calls[3];
 CHECK(word(f->particle.bytes+0x250)&0x8000);
 const gh_title_player_view *related[4]={&f->child,NULL,NULL,NULL};
 return gh_soft_event_suspend_player_0012c7d0(&f->player.view,related,15,frontier);
}
int main(int argc,char **argv)
{
 CHECK(argc==2);
 gh_disc *disc;
 CHECK(!gh_disc_open(argv[1],&disc));
 size_t index;
 gh_disc_entry entry;
 CHECK(!gh_disc_find(disc,"st1/r100.dat",&index)&&!gh_disc_entry_at(disc,index,&entry));
 uint8_t *room_bytes=malloc((size_t)entry.size);
 CHECK(room_bytes&&!gh_disc_read(disc,index,0,room_bytes,(size_t)entry.size));
 gh_bundle room;
 CHECK(!gh_bundle_open(room_bytes,(size_t)entry.size,&room));
 gh_heap *heap;
 gh_room_pools pools={0};
 CHECK(!gh_heap_create(16*1024*1024,&heap)&&!gh_game_room_pools_initialize_002a5d80(&pools,&room,heap));
 gh_game_scene_tables tables={0};
 CHECK(!gh_scene_actors_0012e670(&tables,&room));

 fixture f={0};
 gh_audio_registry *registry;
 CHECK(!gh_audio_registry_create(&registry)&&!gh_audio_startup_load(registry,disc));
 CHECK(!gh_sound_control_from_startup(&f.sound,disc,registry));
 gh_game_init_state *manager=calloc(1,sizeof(*manager));
 CHECK(manager);
 gh_game_init_state_initialize(manager);
 f.manager=manager;
 gh_boot_work work={.room=0x100};
 gh_game_title_player child={0};int child_present;
 uint8_t player_bytes[GH_TITLE_PLAYER_BYTES]={0},
     object_bytes[0x600]={0},effect_bytes[0x600]={0};
 CHECK(!gh_title_player_construct_00107e40(player_bytes,sizeof(player_bytes),&f.player.view));
 CHECK(!gh_game_player_child_construct_00312f70(&child,&pools,&work,&child_present)&&child_present);
 f.child=child.view;uint8_t *child_bytes=child.view.bytes;
 CHECK(!gh_game_object_construct_002a7338(object_bytes,sizeof(object_bytes),&f.object));
 CHECK(!gh_game_object_construct_002a7338(effect_bytes,sizeof(effect_bytes),&f.particle));
 /* Typed fixture IDs test original filters with actual R100 registry storage. */
 object_bytes[0x2ff]=3;effect_bytes[0x2ff]=5;
 put(object_bytes+0x250,0x400);put(object_bytes+0x5a0,2);
 put(effect_bytes+0x250,0x400);
 CHECK(!gh_room_object_register(&pools,&f.object,&f,3,9,&f.object_handle));
 CHECK(!gh_room_object_register(&pools,&f.particle,&f,5,1,&f.particle_handle));
 tables.scroll_link_words[0]=f.object_handle;

 f.work=&work;
 gh_event_start_bindings objects={.work=&work,.tables=&tables,.pools=&pools,.object_view=view,.context=&f};
 f.objects=&objects;
 gh_task_record32 records[3]={0};
 gh_task_manager tasks={.records=records,.capacity=3,.count=3,.current_index=1,.current=&records[1]};
 uint8_t scenario[0x114]={0};
 gh_soft_event_start call={0};
 gh_soft_event_kind0_bindings kind={.room_objects=&objects,.suspend_enemies=empty_enemies,.suspend_player=player};
 gh_soft_event_bindings b={.scenario={scenario,sizeof(scenario),&tasks},.work=&work,
    .player_abort_00126770=abort_player,.context=&f,.kind0=&kind};
 CHECK(gh_scenario_start_soft_event_002c39b0(&call,&b,0)==GH_SOFT_EVENT_UNSUPPORTED&&
       call.frontier==0x2fa470&&call.stage==9);
 CHECK(scenario[0xe8]==1&&word(scenario+0xe0)==0&&f.calls[0]==1&&f.calls[1]==1);
 CHECK(word(object_bytes+0x250)==0x8400&&word(effect_bytes+0x250)==0x400);
 CHECK(!tables.scroll_link_words[0]&&!tasks.flags&&!records[1].attributes);
 ((uint8_t *)&work)[0x178]=0x7b;
 kind.suspend_effects=effects;
 gh_soft_event_status resumed=gh_scenario_start_soft_event_002c39b0(&call,&b,0);
 if(resumed)fprintf(stderr,"resume status%d stage%u frontier%08x childVT %08x raw%08x\n",resumed,call.stage,call.frontier,f.child.original_vtable214,word(child_bytes+0x214));
 CHECK(!resumed&&!call.stage&&!call.frontier);
 CHECK(((uint8_t *)&work)[0x178]==0x7b&&scenario[0xe8]==1&&tasks.flags==1&&records[1].attributes==1);
 CHECK(word((uint8_t *)&work+0x5d8)==0x02540000&&word((uint8_t *)&work+0x5e0)==0x02100000&&
       word((uint8_t *)&work+0x5e4)==0x45010040);
 CHECK(word(effect_bytes+0x250)==0x8400&&word(player_bytes+0x250)&0x8000&&
       word(player_bytes+0x5a0)&8&&word(child_bytes+0x250)&0x8000);
 for(unsigned i=0;i<4;i++)CHECK(f.calls[i]==1);
 CHECK(!gh_scenario_start_soft_event_002c39b0(&call,&b,0)&&scenario[0xe8]==2);
 for(unsigned i=0;i<4;i++)CHECK(f.calls[i]==1);
 CHECK(!gh_room_object_unregister(&pools,f.object_handle,&f)&&!gh_room_object_unregister(&pools,f.particle_handle,&f));
 CHECK(!gh_game_title_player_release(&child));
 CHECK(!gh_scene_actors_reset_0012e800(&tables)&&!gh_game_room_pools_release(&pools)&&!gh_heap_destroy(heap));
 gh_audio_registry_destroy(registry);gh_disc_close(disc);free(manager);free(room_bytes);
 puts("PASS kind0 actual R100 owners, native abort/visibility/OM/effect/player suspend, nested and late retry, empty enemy gate");
 return 0;
}
