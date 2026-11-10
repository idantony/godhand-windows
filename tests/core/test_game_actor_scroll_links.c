#include "game_actor_scroll_links.h"
#include "game_stage.h"
#include "game_collision_scroll.h"
#include "disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x)do{if(!(x)){fprintf(stderr,"FAIL%d %s\n",__LINE__,#x);exit(1);}}while(0)

static uint64_t key(const void*p){uint64_t v;memcpy(&v,p,8);return v;}
int main(int argc,char**argv){C(argc==2);gh_disc*d;size_t i;gh_disc_entry e;C(!gh_disc_open(argv[1],&d)&&!gh_disc_find(d,"st1/r100.dat",&i)&&!gh_disc_entry_at(d,i,&e));uint8_t*raw=malloc((size_t)e.size);size_t en=(size_t)gh_disc_boot_elf_size(d);uint8_t*elf=malloc(en);C(raw&&elf&&!gh_disc_read(d,i,0,raw,(size_t)e.size)&&!gh_disc_read_boot_elf(d,0,elf,en));gh_disc_close(d);gh_bundle room;C(!gh_bundle_open(raw,(size_t)e.size,&room));
 gh_heap*heap;gh_room_pools pools={0};C(!gh_heap_create(64*1024*1024,&heap)&&!gh_game_room_pools_initialize_002a5d80(&pools,&room,heap));gh_game_world_light*light=calloc(1,sizeof(*light));C(light&&!gh_game_world_light_construct_002d6d80(light)&&!gh_game_world_light_initialize_with_heap_002d6f40(light,&room,heap));gh_stage_textures*textures;C(!gh_stage_textures_create(&textures));gh_boot_work work={0};gh_game_scene_tables tables={0};gh_scene_meshes meshes={0};uint8_t serial;C(!gh_scene_mesh_factory_serial_from_elf(elf,en,&serial));meshes.factory_serial=&serial;gh_scene_mesh_construction_bindings bindings={textures,light,elf,en};C(!gh_scene_meshes_bind_construction(&meshes,&bindings)&&!gh_scene_meshes_construct_0031d1b0(&meshes,&tables,&work,&room,&pools,heap)&&meshes.constructed_count==156);C(!gh_scene_actors_0012e670(&tables,&room));unsigned count=tables.akg.bytes[0x2a]|(unsigned)tables.akg.bytes[0x2b]<<8;C(count==193);gh_scene_table_view names=tables.actor_links[7].view;uint8_t original[193*8];memcpy(original,names.bytes,sizeof(original));
 C(gh_scene_actor_scroll_links_00307a60(&tables)==GH_TABLE_UNSUPPORTED&&tables.frontier==0x2c5420);gh_actor_scroll_entry a;C(!gh_scene_actor_scroll_entry_001333b8(&tables,0,&a)&&!a.resolved&&a.current_key==key(original));C(gh_scene_actor_scroll_entry_001333b8(&tables,193,&a)==GH_TABLE_BOUNDS);
 // Sharedcollision link4 mutation remains the SAME canonical view throughout.
 gh_collision_scroll_mutation mutation={0};char collision_name[9]={0};C(tables.actor_links[0].view.size>=20);memcpy(collision_name,tables.actor_links[0].view.bytes+12,8);uint32_t changed;C(!gh_collision_scroll_set_enabled_001332d0(&tables,&mutation,collision_name,0,&changed));gh_scene_table_view collision=tables.actor_links[4].view;
 uint32_t*list=pools.vectors[1].allocation.pointer;uint32_t first=list[0];list[0]=0xffffffffu;for(unsigned j=0;j<32;j++)tables.scroll_link_words[j]=0xaabbccdd;C(gh_scene_actor_scroll_links_native_00307a60(&tables,&pools,&meshes)==GH_TABLE_UNSUPPORTED&&tables.frontier==0x2c5420);C(tables.scroll_link_words[0]==0xaabbccdd);list[0]=first;
 C(!gh_scene_actor_scroll_links_native_00307a60(&tables,&pools,&meshes));unsigned matches=0,missing=0;
 for(unsigned r=0;r<count;r++){uint64_t source=key(original+r*8);uint32_t expected=0;if((uint32_t)source)for(uint32_t j=0;j<pools.vectors[1].count;j++){void*p;uint32_t h=list[j];C(!gh_room_object_lookup(&pools,h,GH_ROOM_OBJECT_SCROLL_MESH,&meshes,&p));const gh_scene_mesh_object*o=p;const uint8_t*name=o->descriptor.script.bytes+o->descriptor.mesh.descriptor_offset+8;if(!memcmp(name,original+r*8,8)){expected=h;break;}}
  C(!gh_scene_actor_scroll_entry_001333b8(&tables,(uint16_t)r,&a)&&a.source_key==source&&a.scroll_handle==expected&&a.resolved==(expected!=0)&&a.current_key==(expected?0:source));if(expected)matches++;else missing++;
 }
 C(matches&&missing);for(unsigned j=0;j<32;j++)C(!tables.scroll_link_words[j]);C(!memcmp(names.bytes,original,sizeof(original))&&tables.actor_links[7].view.bytes==names.bytes&&tables.actor_links[4].view.bytes==collision.bytes&&tables.actor_links[4].view.size==collision.size);
 const gh_scene_mesh_object*firstobj;C(!gh_scene_meshes_object_at(&meshes,0,&firstobj));uint64_t wanted=key(firstobj->descriptor.script.bytes+firstobj->descriptor.mesh.descriptor_offset+8);uint32_t found=0;C(!gh_scene_search_scroll_002c5420(&pools,&meshes,wanted,&found)&&found==firstobj->handle);C(!gh_scene_search_scroll_002c5420(&pools,&meshes,wanted^UINT64_C(0xffff000000000000),&found)&&!found);
 gh_scene_table_view saved=tables.actor_links[7].view;tables.actor_links[7].view.size=8;C(gh_scene_actor_scroll_links_native_00307a60(&tables,&pools,&meshes)==GH_TABLE_BOUNDS);tables.actor_links[7].view=saved;C(!gh_scene_actor_scroll_links_native_00307a60(&tables,&pools,&meshes));C(!gh_collision_scroll_mutation_release(&mutation)&&!gh_scene_actors_reset_0012e800(&tables)&&!tables.actor_scroll_native);C(!gh_scene_actors_0012e670(&tables,&room)&&!gh_scene_actor_scroll_entry_001333b8(&tables,0,&a)&&!a.resolved);C(!gh_scene_actor_scroll_links_native_00307a60(&tables,&pools,&meshes)&&!gh_scene_actors_reset_0012e800(&tables));
 C(!gh_scene_meshes_release_prefix(&meshes));C(!gh_game_world_light_release_presets(light));free(light);gh_stage_textures_destroy(textures);C(!gh_game_room_pools_release(&pools)&&!gh_heap_destroy(heap));free(raw);free(elf);printf("PASS actualR100 AKG193 links: %u matches/%u originalnomatches, canonical156mesh first-name-order, 64bitkeys, immutableassets, collisionlink4 coexist, invalidhandle/bounds/prefix, reset/rebind lifecycle\n",matches,missing);return 0;
}




