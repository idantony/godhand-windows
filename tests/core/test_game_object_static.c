#include "game_object_step.h"
#include "game_stage.h"
#include "game_camera_projection.h"
#include "display_state.h"
#include "game_world_light_step.h"
#include "ui_order.h"
#include "disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fenv.h>
#include <math.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL%d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t w(const void*p){uint32_t x;memcpy(&x,p,4);return x;}
static void ww(void*p,uint32_t x){memcpy(p,&x,4);}
static void wf(void*p,float x){memcpy(p,&x,4);}
static float rf(const void*p){float x;memcpy(&x,p,4);return x;}
static uint8_t*load(gh_disc*d,const char*name,size_t*n){size_t i;gh_disc_entry e;C(!gh_disc_find(d,name,&i)&&!gh_disc_entry_at(d,i,&e));uint8_t*b=malloc((size_t)e.size);C(b&&!gh_disc_read(d,i,0,b,(size_t)e.size));*n=(size_t)e.size;return b;}
typedef struct{unsigned calls;} clock_fixture;
static uint32_t timer(void*raw){clock_fixture*c=raw;return 100+(c->calls++)*40;}
/* Queue-only fixture: validates real typed commands, never claims D3D pixels. */
typedef struct{int available,fail_setup;size_t count,geometry,setups;gh_ui_order_group groups[512];gh_object_static_command_kind kinds[512];size_t objects[512];unsigned program10,program14;} queue_fixture;
static gh_object_step_status emit(void*raw,const gh_object_static_command*cmd,uint32_t*f){queue_fixture*q=raw;
 if(!q->available){*f=0x14b810;return GH_OBJSTEP_UNSUPPORTED;}
 if(cmd->kind==GH_OBJECT_SETUP&&q->fail_setup){*f=0x14c6b8;return GH_OBJSTEP_CALLEE;}
 C(q->count<512&&cmd->object&&cmd->object->constructor_complete&&cmd->handle==cmd->object->handle&&(cmd->bank==1||cmd->bank==2)&&cmd->depth);
 if(cmd->kind==GH_OBJECT_NODE_GEOMETRY){C(cmd->material_count==cmd->object->descriptor.submesh_count&&cmd->material_count>0&&cmd->material_count<=16);for(unsigned i=0;i<cmd->material_count;i++)C(cmd->materials[i]&&cmd->materials[i]->rgba_gs&&cmd->materials[i]->draw_tcc==1);q->geometry++;if(cmd->selected_program==10)q->program10++;else{C(cmd->selected_program==14);q->program14++;}}
 else q->setups++;
 q->groups[q->count]=(gh_ui_order_group){cmd->bank,cmd->depth,(uint64_t)cmd->handle*2+(cmd->kind==GH_OBJECT_SETUP)};q->kinds[q->count]=cmd->kind;q->objects[q->count]=cmd->object_index;q->count++;return GH_OBJSTEP_OK;
}
/* Independent binary64 geometric oracle: transform each actuallocalbox
 * corner directly, instead of production's roundedcenter/basis composition. */
static int box_visible_double(const gh_scene_mesh_object*o,const gh_game_pl_camera*c){
 const uint8_t*p=o->allocation.pointer;unsigned common=63;
 for(unsigned corner=0;corner<8;corner++){double local[4],world[4],clip[4];
  for(unsigned axis=0;axis<3;axis++)local[axis]=(double)rf(p+0x260+axis*4)+((corner&(1u<<axis))?1:-1)*(double)rf(p+0x26c+axis*4);local[3]=1;
  for(unsigned row=0;row<4;row++){world[row]=0;for(unsigned k=0;k<4;k++)world[row]+=(double)rf(p+0x80+(k*4+row)*4)*local[k];}
  for(unsigned row=0;row<4;row++){clip[row]=0;for(unsigned k=0;k<4;k++)clip[row]+=(double)rf(c->fields+0x80+(k*4+row)*4)*world[k];}
  unsigned flags=0;for(unsigned axis=0;axis<3;axis++){if(clip[axis]>fabs(clip[3]))flags|=1u<<(2*axis);if(clip[axis]<-fabs(clip[3]))flags|=2u<<(2*axis);}common&=flags;
 }
 return !common;
}
int main(int argc,char**argv){C(argc==2);gh_disc*d;C(!gh_disc_open(argv[1],&d));size_t bytes,r020_bytes;uint8_t*raw=load(d,"st1/r100.dat",&bytes),*r020=load(d,"st0/r020.dat",&r020_bytes);size_t elf_bytes=(size_t)gh_disc_boot_elf_size(d);uint8_t*elf=malloc(elf_bytes);C(elf&&!gh_disc_read_boot_elf(d,0,elf,elf_bytes));gh_disc_close(d);
 gh_bundle room,room20;C(!gh_bundle_open(raw,bytes,&room)&&!gh_bundle_open(r020,r020_bytes,&room20));gh_heap*heap;gh_room_pools pools={0};C(!gh_heap_create(64*1024*1024,&heap)&&!gh_game_room_pools_initialize_002a5d80(&pools,&room,heap));
 gh_stage_textures*textures;C(!gh_stage_textures_create(&textures));gh_game_world_light*light=calloc(1,sizeof(*light));C(light&&!gh_game_world_light_construct_002d6d80(light)&&!gh_game_world_light_initialize_with_heap_002d6f40(light,&room,heap));
 gh_game_init_state init;gh_game_init_state_initialize(&init);gh_game_object_step state={0};C(!gh_game_object_step_construct_002c53a8(&state,&init));
 gh_boot_work work={0};wf((uint8_t*)&work+0x574,1);((uint8_t*)&work)[0x5d4]=20;
 gh_game_scene_tables tables={0};gh_scene_meshes meshes={0};uint8_t serial;C(!gh_scene_mesh_factory_serial_from_elf(elf,elf_bytes,&serial));meshes.factory_serial=&serial;gh_scene_mesh_construction_bindings build={textures,light,elf,elf_bytes};C(!gh_scene_meshes_bind_construction(&meshes,&build)&&!gh_scene_meshes_construct_0031d1b0(&meshes,&tables,&work,&room,&pools,heap)&&meshes.constructed_count==156);
 gh_game_pl_camera camera={0};C(!gh_game_camera_construct_pl_00139bc8(&camera,&init.camera,GH_CAMERA_NATIVE_BINARY32_REFERENCE));gh_display_state display;C(!gh_display_state_from_elf(&display,elf,elf_bytes)&&!gh_display_static_initialize_002a48a8(&display,1,0xffff));gh_camera_projection_display config={&display.screen_width,&display.screen_height_bits,&display.pixel_aspect_bits,&display.initialized,GH_CAMERA_NATIVE_BINARY32_REFERENCE};uint32_t f;C(!gh_game_camera_project_00138740(&camera,&config,&f));gh_game_render_projection projection;C(!gh_game_render_projection_001538f0(&projection,&camera,&config,&f));
 /* RealR020 empty layer-manager init supplies this isolated visibility0
  * configuration. This does not claim R100's positive SCA initialization. */
 gh_scene_at_manager at={0};C(!gh_scene_at_initialize_002c0f90(&at,&room20));gh_title_player_view player={0};uint8_t player_bytes[GH_TITLE_PLAYER_BYTES]={0};
 queue_fixture queue={0};gh_object_static_bindings bindings={&meshes,&at,&camera,&player,textures,light,&projection,emit,&queue};C(!gh_game_object_step_bind_static(&state,&bindings,elf,elf_bytes)&&state.globals_loaded&&state.depth_shift==0&&state.moved_objects_5caffc==0);
 clock_fixture clock={0};C(gh_game_move_objects_0030f090(&state,&pools,&work,timer,&clock,&f)==GH_OBJSTEP_UNSUPPORTED&&f==0x2db6b0&&clock.calls==1&&state.moved_objects_5caffc==1);
 C(!gh_title_player_construct_00107e40(player_bytes,sizeof(player_bytes),&player));player.initial_memory=GH_TITLE_MEMORY_ZERO_REFERENCE;
 C(!fesetround(FE_DOWNWARD)&&!gh_game_move_objects_0030f090(&state,&pools,&work,timer,&clock,&f)&&fegetround()==FE_DOWNWARD&&clock.calls==2&&state.moved_objects_5caffc==156&&state.profile_752cf8==UINT32_C(0xffffffd8));C(!fesetround(FE_TONEAREST));
 unsigned p10=0,p14=0,uv=0;for(size_t i=0;i<156;i++){const gh_scene_mesh_object*o;C(!gh_scene_meshes_object_at(&meshes,i,&o));uint8_t*p=o->allocation.pointer;C((w(p+0x2f0)&3u)==0);unsigned blend=(w(o->descriptor.script.bytes+o->descriptor.mesh.descriptor_offset+0x3c)>>4)&15;C(o->alpha==(blend?state.alpha_presets[blend]:UINT64_C(0x8000000044)));if(p[0x2be]==10)p10++;else{C(p[0x2be]==14);p14++;}if(rf((uint8_t*)o->node+0x38c)!=0||rf((uint8_t*)o->node+0x390)!=0)uv++;}C(p10&&p14&&uv==6&&meshes.first.test==0x51001);
 gh_light_player_parts parts={player.bytes,NULL,0};C(!gh_game_world_light_select_002d8408(light,0,&parts)&&!gh_game_world_light_move_002d7560(light,&work,&pools,GH_CAMERA_NATIVE_BINARY32_REFERENCE));
 /* Exercise original force-visible debug bit as an explicit CPU queue fixture,
  * not as a production visibility bypass or a rendered R100 frame. */
 ((uint8_t*)&work)[0x58d]=0x80;
 C(gh_game_objects_trans_static_0030f650(&state,&pools,&work,&f)==GH_OBJSTEP_UNSUPPORTED&&f==0x14b810&&state.trans_phase==3&&!queue.count);
 size_t first=state.pending.object_index;const gh_scene_mesh_object*first_object;C(!gh_scene_meshes_object_at(&meshes,first,&first_object));uint64_t visits=first_object->materials[0].consumer_visits;
 C(!fesetround(FE_UPWARD));queue.available=1;queue.fail_setup=1;C(gh_game_objects_trans_static_0030f650(&state,&pools,&work,&f)==GH_OBJSTEP_CALLEE&&f==0x14c6b8&&state.trans_phase==4&&queue.count==1&&queue.kinds[0]==GH_OBJECT_NODE_GEOMETRY);
 queue.fail_setup=0;C(!gh_game_objects_trans_static_0030f650(&state,&pools,&work,&f)&&!state.trans_stage&&first_object->materials[0].consumer_visits==visits&&queue.geometry==queue.setups&&queue.geometry>0&&fegetround()==FE_UPWARD);C(!fesetround(FE_TONEAREST));
 C(meshes.first.object_program[0]==0x80000000u||first!=0);for(size_t i=0;i<queue.count;i+=2)C(queue.kinds[i]==GH_OBJECT_NODE_GEOMETRY&&queue.kinds[i+1]==GH_OBJECT_SETUP&&queue.objects[i]==queue.objects[i+1]&&queue.groups[i].bank==queue.groups[i+1].bank&&queue.groups[i].depth==queue.groups[i+1].depth);
 for(size_t i=2;i<queue.count;i+=2)C(queue.objects[i]>queue.objects[i-2]);size_t order[512],written,omitted;C(!gh_ui_order_indices(queue.groups,queue.count,0,0,order,512,&written,&omitted)&&written==queue.count&&!omitted);for(size_t pair=0;pair<queue.count;pair+=2){size_t a=512,b=512;for(size_t i=0;i<written;i++){if(order[i]==pair)a=i;if(order[i]==pair+1)b=i;}C(b<a);}
 size_t forced_geometry=queue.geometry;uint8_t forced[156]={0},normal[156]={0};for(size_t i=0;i<queue.count;i++)if(queue.kinds[i]==GH_OBJECT_NODE_GEOMETRY)forced[queue.objects[i]]=1;
 ((uint8_t*)&work)[0x58d]=0;queue.count=queue.geometry=queue.setups=0;
 C(!gh_game_objects_trans_static_0030f650(&state,&pools,&work,&f));for(size_t i=0;i<queue.count;i++)if(queue.kinds[i]==GH_OBJECT_NODE_GEOMETRY)normal[queue.objects[i]]=1;
 size_t expected_visible=0;for(size_t i=0;i<156;i++){const gh_scene_mesh_object*o;C(!gh_scene_meshes_object_at(&meshes,i,&o));int expected=forced[i]&&!(o->descriptor.mesh.flags&0x100u)&&((w((uint8_t*)o->allocation.pointer+0x250)&0x10000u)||box_visible_double(o,&camera));C(normal[i]==expected);expected_visible+=(size_t)expected;}
 C(queue.geometry==expected_visible&&queue.geometry<=forced_geometry);printf("normalvisible groups%zu vsdebugvisible%zu, independent8cornerfrustum exact\n",queue.geometry,forced_geometry);
 if(queue.geometry){size_t normal_count=queue.geometry,target=0;while(target<156&&!normal[target])target++;const gh_scene_mesh_object*hidden;C(target<156&&!gh_scene_meshes_object_at(&meshes,target,&hidden));uint8_t*n=hidden->node;uint32_t flags=w(n+0x380);ww(n+0x380,flags|1u);queue.count=queue.geometry=queue.setups=0;C(!gh_game_objects_trans_static_0030f650(&state,&pools,&work,&f)&&queue.geometry+1==normal_count&&queue.setups==normal_count);unsigned geom=0,setup=0;for(size_t i=0;i<queue.count;i++)if(queue.objects[i]==target){if(queue.kinds[i]==GH_OBJECT_NODE_GEOMETRY)geom++;else setup++;}C(!geom&&setup==1);ww(n+0x380,flags);}
 /* Two original delayed-deletion flag transitions precede the unknown real
  * virtual destructor. No object is unregistered as a fabricated destructor. */
 for(size_t i=0;i<156;i++){const gh_scene_mesh_object*o;C(!gh_scene_meshes_object_at(&meshes,i,&o));ww((uint8_t*)o->allocation.pointer+0x250,i?0x8000u:0x80000000u);}
 queue.count=queue.geometry=queue.setups=0;C(!gh_game_objects_trans_static_0030f650(&state,&pools,&work,&f)&&w((uint8_t*)meshes.first.allocation.pointer+0x250)==0xc0000000u&&!queue.count);
 C(!gh_game_objects_trans_static_0030f650(&state,&pools,&work,&f)&&w((uint8_t*)meshes.first.allocation.pointer+0x250)==0xe0000000u);C(gh_game_objects_trans_static_0030f650(&state,&pools,&work,&f)==GH_OBJSTEP_UNSUPPORTED&&f==0x31a018&&pools.vectors[0].count==156&&pools.vectors[1].count==155);C(gh_game_objects_trans_static_0030f650(&state,&pools,&work,&f)==GH_OBJSTEP_UNSUPPORTED&&f==0x31a018&&pools.vectors[1].count==155);
 C(!gh_scene_meshes_release_prefix(&meshes)&&!gh_game_world_light_release_presets(light)&&!gh_game_room_pools_release(&pools)&&!gh_heap_used(heap)&&!gh_heap_destroy(heap));gh_stage_textures_destroy(textures);free(light);free(raw);free(r020);free(elf);
 printf("PASS actualR100156 static Move (program10:%u/14:%u,UVrates%u), sourceTEST/ALPHA, partialretry/timer/FEnv; real Trans constants/materials and twoOTqueuegroups+LIFO; requiredfrontend/deletionfrontiers. CPU queue fixture, no GPU frame claim\n",p10,p14,uv);return 0;
}
