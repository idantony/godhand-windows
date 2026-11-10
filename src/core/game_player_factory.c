#include "game_player_factory.h"
#include <string.h>
#include <stdio.h>
static uint32_t w(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
static void ww(uint8_t*p,uint32_t v){memcpy(p,&v,4);}
static void wh(uint8_t*p,uint16_t v){memcpy(p,&v,2);}
static gh_title_player_status fail(gh_game_player_factory*s,uint32_t pc,gh_title_player_status e){s->frontier=pc;return e;}
static int same(const gh_player_factory_bindings*a,const gh_player_factory_bindings*b){return a->player==b->player&&a->pools==b->pools&&a->global_objects==b->global_objects&&a->room_objects==b->room_objects&&a->work==b->work&&a->disc==b->disc&&a->factory_serial==b->factory_serial&&a->save.bytes==b->save.bytes&&a->save.size==b->save.size;}
static gh_title_player_status extent(gh_game_player_factory*s){
 if(s->resource.file_extent_known){if(!s->resource.file_extent||s->resource.file_extent>s->resource.size||!s->resource.bytes)return fail(s,0x30fe68,GH_PLAYER_BOUNDS);s->file_bytes=s->resource.file_extent;return GH_PLAYER_OK;}
 if(s->resource_from_room){gh_object_view global;if(gh_game_objects_find(s->b.global_objects,0x100,&global)!=GH_OBJECT_OK||global.bytes!=s->resource.bytes)return fail(s,0x30fe68,GH_PLAYER_UNSUPPORTED);}
 static const uint8_t variants[]={0,2,5,15,7,16,14,17};unsigned costume=gh_core_save_costume(s->b.save),object=variants[costume<8?costume:0];char path[16];int n=snprintf(path,sizeof(path),"pl/pl%02x.dat",object);size_t index;gh_disc_entry e;
 if(n<0||(size_t)n>=sizeof(path)||gh_disc_find(s->b.disc,path,&index)||gh_disc_entry_at(s->b.disc,index,&e))return fail(s,0x1ff9c0,GH_PLAYER_BOUNDS);
 if(e.size>SIZE_MAX||e.size>s->resource.size||!s->resource.bytes)return fail(s,0x30fe68,GH_PLAYER_BOUNDS);s->file_bytes=(size_t)e.size;return GH_PLAYER_OK;
}
gh_title_player_status gh_game_player_factory_create_0030fa98(gh_game_player_factory*s,const gh_player_factory_bindings*b,int*original){
 if(!s||!b||!original||!b->player||!b->pools||!b->global_objects||!b->room_objects||!b->work||!b->disc||!b->factory_serial||!b->save.bytes||b->save.size<GH_CORE_SAVE_BYTES)return GH_PLAYER_ARGUMENT;*original=0;
 if(s->stage&&!same(&s->b,b))return GH_PLAYER_ARGUMENT;if(s->complete){*original=1;return GH_PLAYER_OK;}
 if(!s->stage){const gh_room_vector*all,*objects;if(gh_room_registry_view(b->pools,0x754c30,&all)||gh_room_registry_view(b->pools,0x754c58,&objects))return fail(s,0x30fb70,GH_PLAYER_ARGUMENT);if(all->count==all->capacity||objects->count==objects->capacity)return GH_PLAYER_OK;
  if(b->player->allocation.pointer)return GH_PLAYER_BUSY;int present=0;gh_title_player_status e=gh_game_player_construct_00312f00(b->player,b->pools,b->work,&present);if(e)return fail(s,b->player->frontier?b->player->frontier:0x312f00,e);if(!present)return GH_PLAYER_OK;s->b=*b;s->stage=1;
 }
 uint8_t*raw=s->b.player->view.bytes;
 if(s->stage==1){raw[0x2fc]=*s->b.factory_serial;*s->b.factory_serial=(uint8_t)(*s->b.factory_serial+1);s->stage=2;}
 if(s->stage==2){wh(raw+0x2fe,0x100);ww(raw+0x2d0,0x80000000);wh(raw+0x300,0x100);s->stage=3;}
 if(s->stage==3){
  if(!s->association_stage){gh_object_view v;gh_object_status e=gh_game_objects_acquire_001fea48(s->b.room_objects,0x100,&v);if(e!=GH_OBJECT_OK&&e!=GH_OBJECT_MISSING)return fail(s,0x1fea48,GH_PLAYER_UNSUPPORTED);if(e==GH_OBJECT_OK){s->room_reference=1;if(v.bytes){s->resource=v;s->resource_from_room=1;s->association_stage=2;}}if(!s->association_stage)s->association_stage=1;}
  if(s->association_stage==1){gh_object_view v;gh_object_status e=gh_game_objects_acquire_001fea48(s->b.global_objects,0x100,&v);if(e==GH_OBJECT_MISSING)return fail(s,0x1fe8a0,GH_PLAYER_UNSUPPORTED);if(e!=GH_OBJECT_OK)return fail(s,0x1fe8a0,GH_PLAYER_UNSUPPORTED);s->global_reference=1;s->resource=v;s->association_stage=2;if(!v.bytes)return fail(s,0x1fe8a0,GH_PLAYER_UNSUPPORTED);}
  if(!s->resource.bytes){gh_object_view refreshed;gh_game_objects*manager=s->resource_from_room?s->b.room_objects:s->b.global_objects;if(gh_game_objects_find(manager,0x100,&refreshed)!=GH_OBJECT_OK||!refreshed.bytes)return fail(s,0x1fe8a0,GH_PLAYER_UNSUPPORTED);s->resource=refreshed;}
  gh_title_player_status e=extent(s);if(e)return e;s->resource_heap_original=0x754220;/* Nativeview replacesoriginal304/2C8 pointers. */ww(raw+0x304,0);ww(raw+0x2c8,0);if(!s->resource_from_room)ww(raw+0x250,w(raw+0x250)|0x2000000);s->stage=4;
 }
 if(s->stage==4){gh_room_pool_status e=gh_room_object_register(s->b.pools,s->b.player,s->b.player,GH_ROOM_OBJECT_PLAYER,9,&s->handle);if(e)return fail(s,0x30fcc0,GH_PLAYER_UNSUPPORTED);s->stage=5;}
 s->complete=1;s->frontier=0;*original=1;return GH_PLAYER_OK;
}
gh_title_player_status gh_game_player_factory_scalar_view(gh_game_player_factory*s,uint32_t handle,const gh_title_player_view**out){if(!s||!out)return GH_PLAYER_ARGUMENT;*out=NULL;if(!s->complete||!handle||handle!=s->handle)return GH_PLAYER_ARGUMENT;void*p=NULL;if(gh_room_object_lookup(s->b.pools,handle,GH_ROOM_OBJECT_PLAYER,s->b.player,&p)||p!=s->b.player)return GH_PLAYER_ARGUMENT;*out=&s->b.player->view;return GH_PLAYER_OK;}
gh_title_player_status gh_game_player_factory_release_prefix(gh_game_player_factory*s){
 if(!s)return GH_PLAYER_ARGUMENT;if(!s->b.player)return GH_PLAYER_OK;if(s->handle&&gh_room_object_unregister(s->b.pools,s->handle,s->b.player))return GH_PLAYER_UNSUPPORTED;s->handle=0;
 if(s->room_reference){if(gh_game_objects_release_reference_001ff0b0(s->b.room_objects,0x100))return GH_PLAYER_UNSUPPORTED;s->room_reference=0;}
 if(s->global_reference){if(gh_game_objects_release_reference_001ff0b0(s->b.global_objects,0x100))return GH_PLAYER_UNSUPPORTED;s->global_reference=0;}
 gh_title_player_status e=gh_game_title_player_release(s->b.player);if(e)return e;memset(s,0,sizeof(*s));return GH_PLAYER_OK;
}
