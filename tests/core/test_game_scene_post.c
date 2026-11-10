#include "game_scene_post.h"
#include "game_ray_query.h"
#include "game_ray_static.h"
#include "disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fenv.h>
#define C(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static void put(uint8_t*p,uint32_t n){memcpy(p,&n,4);}
static float value(const uint8_t*p){float x;memcpy(&x,p,4);return x;}
int main(int argc,char**argv){
 C(argc==2);gh_disc*d;size_t idx;gh_disc_entry de;C(gh_disc_open(argv[1],&d)==GH_DISC_OK&&gh_disc_find(d,"st0/r020.dat",&idx)==GH_DISC_OK&&gh_disc_entry_at(d,idx,&de)==GH_DISC_OK);uint8_t*r=malloc((size_t)de.size);C(r&&gh_disc_read(d,idx,0,r,(size_t)de.size)==GH_DISC_OK);gh_bundle bundle;C(gh_bundle_open(r,(size_t)de.size,&bundle)==GH_BUNDLE_OK);
 gh_heap*heap;gh_room_pools pools={0};C(gh_heap_create(8*1024*1024,&heap)==GH_MEM_OK&&gh_game_room_pools_initialize_002a5d80(&pools,&bundle,heap)==GH_POOL_OK);
 gh_game_scene_tables tables={0};C(gh_scene_actors_0012e670(&tables,&bundle)==GH_TABLE_OK&&tables.akg.bytes&&tables.actors_ready&&gh_scene_actor_scroll_links_00307a60(&tables)==GH_TABLE_OK);
 static gh_game_init_state init;gh_game_init_state_initialize(&init);gh_game_pl_camera camera={0};C(gh_game_camera_construct_pl_00139bc8(&camera,&init.camera,GH_CAMERA_NATIVE_BINARY32_REFERENCE)==GH_CAMERA_OK);init.active_camera=&init.camera;init.camera.mode=1;
 uint8_t raw[GH_TITLE_PLAYER_BYTES]={0};gh_title_player_view player;C(gh_title_player_construct_00107e40(raw,sizeof(raw),&player)==GH_PLAYER_OK);player.initial_memory=GH_TITLE_MEMORY_ZERO_REFERENCE;
 gh_boot_work work={0};put((uint8_t*)&work+0x5d8,0xf00c6000);gh_scene_post_bindings b={&work,&pools,&tables,&init,&camera,&player,GH_CAMERA_NATIVE_BINARY32_REFERENCE};uint32_t frontier;gh_scene_model_ray rays[5];size_t count;
 C(fesetround(FE_UPWARD)==0);C(gh_scene_model_rays_00307b28(&b,rays,&count,&frontier)==GH_POST_OK&&count==5&&!frontier&&fegetround()==FE_UPWARD);C(fesetround(FE_TONEAREST)==0);
 C(rays[0].from[0]==0&&rays[0].from[1]==1.2f&&rays[0].to[0]==value(camera.fields+0x210));
 double dx=rays[0].to[0]-rays[0].from[0],dy=rays[0].to[1]-rays[0].from[1],dz=rays[0].to[2]-rays[0].from[2];
 for(unsigned k=0;k<5;k++)C(rays[k].from[1]>.1f&&rays[k].to[1]>.1f&&rays[k].from[3]==1&&rays[k].to[3]==1);
 /* Independentdoublegeometry: originaltwo yawrotations aroundworldup.
  * Nativepolynomial/VUreference isn't claimedhardwarebitexact. */
 for(unsigned k=0;k<2;k++){double angle=(k?1:-1)*.31415927,x=(cos(angle)*dx+sin(angle)*dz)*.71428573,z=(-sin(angle)*dx+cos(angle)*dz)*.71428573;
  C(fabs(rays[1+k].to[0]-x)<.00002&&fabs(rays[1+k].to[2]-z)<.00002&&fabs(rays[1+k].to[1]-(1.2+dy*.71428573))<.00002);
 }
 C(gh_scene_scroll_forward_002c5470(&pools,&work,&frontier)==GH_POST_OK&&!frontier);
 uint64_t before=tables.ray.native_queries;C(gh_scene_model_hide_00307b28(&b,&frontier)==GH_POST_OK&&!frontier&&tables.ray.native_queries==before+10);
 C(tables.ray.mask174==7&&!tables.ray.flags178&&!tables.ray.report_limit140&&!tables.ray.reports18c);
 for(unsigned i=0;i<32;i++)C(!tables.scroll_link_words[i]);
 /* Two actualqueriesperray reachrealAKG inclusiveAABBrejection, preserving
  * canonicalqueryscratch. No fakeNoHit callback suppliedbythis test. */
 for(unsigned k=0;k<3;k++){float x;memcpy(&x,&tables.ray.frome0[k],4);C(x==rays[4].from[k]);}
 /* Inside the floor AABB these upward/boundary R020 rays still genuinely
  * miss after complete static polygon traversal, not an empty-list shortcut. */
 put(player.alias_f0+4,0xbf99999a);before=tables.ray.native_queries;
 C(gh_scene_model_hide_00307b28(&b,&frontier)==GH_POST_OK&&!frontier&&tables.ray.native_queries==before+10);put(player.alias_f0+4,0);
 /* Original200000gate preservesexistinghiddenlist/querycounter. */
 for(unsigned i=0;i<32;i++)tables.scroll_link_words[i]=i+1;put((uint8_t*)&work+0x5d8,0x200000);before=tables.ray.native_queries;
 C(gh_scene_model_hide_00307b28(&b,&frontier)==GH_POST_OK&&tables.ray.native_queries==before&&tables.scroll_link_words[31]==32);
 put((uint8_t*)&work+0x5d8,0);C(gh_scene_model_hide_00307b28(&b,&frontier)==GH_POST_UNSUPPORTED&&frontier==0x14b6f8&&tables.scroll_link_words[31]==32);
 gh_scene_hidden_clear_00308d40(&tables);for(unsigned i=0;i<32;i++)C(!tables.scroll_link_words[i]);
 pools.vectors[2].count=1;C(gh_scene_scroll_forward_002c5470(&pools,&work,&frontier)==GH_POST_UNSUPPORTED&&frontier==0x319520);put((uint8_t*)&work+0x5d8,0x08000000);C(gh_scene_scroll_forward_002c5470(&pools,&work,&frontier)==GH_POST_OK);pools.vectors[2].count=0;
 pools.vectors[3].count=1;C(gh_scene_hidden_reset_00308bc0(&tables,&pools,&frontier)==GH_POST_UNSUPPORTED&&frontier==0x308bc0);pools.vectors[3].count=0;
 init.camera.mode=0;C(gh_scene_model_rays_00307b28(&b,rays,&count,&frontier)==GH_POST_OK&&count==3);C(rays[0].from[1]==value(camera.fields+0x204)&&rays[0].to[1]==value(camera.fields+0x214));
 gh_scene_ray_triangle_state_release(&tables);C(gh_game_room_pools_release(&pools)==GH_POOL_OK&&gh_heap_used(heap)==0&&gh_heap_destroy(heap)==GH_MEM_OK);free(r);gh_disc_close(d);
 puts("PASS actualR020 postscene registries/hiddenowner, fiveoriginalrays +10realAKGAABBqueries, nativequaternionreference, positivegeometryfrontier andoriginalgates");return 0;
}
