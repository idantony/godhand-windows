#include "../../src/core/game_scene_exit.h"
#include "../../src/core/scene_at_units.h"
#include "../../src/platform/disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(e) do{if(!(e)){fprintf(stderr,"line %d: %s\n",__LINE__,#e);return 1;}}while(0)

int main(int argc,char **argv)
{
 gh_disc *disc;size_t index;gh_disc_entry file;
 CHECK(argc==2&&!gh_disc_open(argv[1],&disc)&&!gh_disc_find(disc,"st1/r100.dat",&index)&&
  !gh_disc_entry_at(disc,index,&file));
 uint8_t *bytes=malloc((size_t)file.size);
 gh_bundle room;
 CHECK(bytes&&!gh_disc_read(disc,index,0,bytes,(size_t)file.size)&&!gh_bundle_open(bytes,(size_t)file.size,&room));
 gh_heap *heap;
 CHECK(!gh_heap_create(16*1024*1024,&heap));
 gh_scene_at_manager at={0};
 CHECK(!gh_scene_at_initialize_units_002c0f90(&at,&room,heap,0x100,1)&&at.ready&&at.jump_slots[0].bytes);
 size_t used=gh_heap_used(heap);
 CHECK(used>0);
 uint32_t frontier;
 gh_at_unit_view original=at.jump_slots[1];
 at.jump_slots[1]=at.jump_slots[0];
 CHECK(gh_scene_at_quit_002c1270(&at,&frontier)==GH_AT_UNSUPPORTED&&frontier==0x30f5a0&&
  at.ready&&gh_heap_used(heap)==used);
 at.jump_slots[1]=original;

 gh_boot_work work={.room=0x100};
 gh_game_loop loop={.work=&work,.game_state={5,1}};
 uint8_t save_bytes[GH_CORE_SAVE_BYTES];
 gh_core_save_view save={save_bytes,sizeof(save_bytes)};
 CHECK(!gh_core_save_system_init(save));
 gh_game_event event={.initialized=1,.fields={.data_number=3}};
 const gh_game_scene_exit_bindings bindings={.loop=&loop,.save=save,.event=&event,.scene_at=&at};
 gh_game_scene_exit exit_state={.operation=17,.phase=1};
 CHECK(gh_game_scene_exit_002a67d0(&exit_state,&bindings)==GH_SCENE_EXIT_UNSUPPORTED&&
  exit_state.frontier==0x2ad108&&exit_state.operation==18&&!at.native_units&&!at.ready&&
  !gh_heap_used(heap)&&event.fields.data_number==3);
 for(unsigned i=0;i<8;++i)CHECK(!at.jump_slots[i].bytes);
 CHECK(!gh_scene_at_quit_002c1270(&at,&frontier)&&!frontier);
 CHECK(!gh_heap_destroy(heap));
 gh_disc_close(disc);
 free(bytes);
 puts("PASS actual R100 positive JMP slots, native Main5 op17 release and continued op18 frontier");
 return 0;
}
