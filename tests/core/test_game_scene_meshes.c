#include "game_scene_meshes.h"
#include "game_stage.h"
#include <fenv.h>
#include <math.h>
#include "../platform/disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t u32(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
int main(int argc,char**argv){
 CHECK(argc==2);gh_disc*d;CHECK(gh_disc_open(argv[1],&d)==GH_DISC_OK);size_t ix;gh_disc_entry file;
 CHECK(gh_disc_find(d,"st1/r100.dat",&ix)==GH_DISC_OK&&gh_disc_entry_at(d,ix,&file)==GH_DISC_OK&&file.size==1440352);
 size_t elf_bytes=(size_t)gh_disc_boot_elf_size(d);uint8_t*elf=malloc(elf_bytes);uint8_t cold=0x5a;CHECK(elf&&gh_disc_read_boot_elf(d,0,elf,elf_bytes)==GH_DISC_OK);
 CHECK(gh_scene_mesh_factory_serial_from_elf(elf,elf_bytes,&cold)==GH_MESH_OK);printf("actualELF factory3C3D18 coldbyte%u\n",cold);
 CHECK(gh_scene_mesh_factory_serial_from_elf(elf,51,&cold)==GH_MESH_BOUNDS);elf[5]=2;CHECK(gh_scene_mesh_factory_serial_from_elf(elf,elf_bytes,&cold)==GH_MESH_UNSUPPORTED);elf[5]=1;
 /* Synthetic ELF bounds only: explicit declared BSS and overlapping mappings. */
 uint8_t tiny[116]={0};memcpy(tiny,"\177ELF\1\1\1",7);tiny[18]=8;tiny[28]=52;tiny[42]=32;tiny[44]=1;
 tiny[52]=1;tiny[56]=116;tiny[60]=0x18;tiny[61]=0x3d;tiny[62]=0x3c;tiny[72]=1;
 cold=99;CHECK(gh_scene_mesh_factory_serial_from_elf(tiny,sizeof(tiny),&cold)==GH_MESH_OK&&cold==0);
 tiny[44]=2;memcpy(tiny+84,tiny+52,32);CHECK(gh_scene_mesh_factory_serial_from_elf(tiny,sizeof(tiny),&cold)==GH_MESH_BOUNDS);
 uint8_t*raw=malloc((size_t)file.size);CHECK(raw&&gh_disc_read(d,ix,0,raw,(size_t)file.size)==GH_DISC_OK);
 gh_bundle room;CHECK(gh_bundle_open(raw,(size_t)file.size,&room)==GH_BUNDLE_OK);gh_heap*heap;
 CHECK(gh_heap_create(32*1024*1024,&heap)==GH_MEM_OK);gh_room_pools pools={0};
 CHECK(gh_game_room_pools_initialize_002a5d80(&pools,&room,heap)==GH_POOL_OK);
 size_t initial=gh_heap_used(heap);int objects[3],owners[2];uint32_t h1,h2,h3;void*found;
 CHECK(gh_room_object_register(&pools,&objects[0],&owners[0],7,3,&h1)==GH_POOL_OK);
 CHECK(u32(pools.vectors[0].allocation.pointer)==h1&&u32(pools.vectors[1].allocation.pointer)==h1);
 CHECK(gh_room_object_register(&pools,&objects[1],&owners[1],8,9,&h2)==GH_POOL_OK&&h2!=h1);
 CHECK(gh_room_object_register(&pools,&objects[0],&owners[1],9,3,&h3)==GH_POOL_BUSY);
 CHECK(gh_room_object_lookup(&pools,h1,7,&owners[0],&found)==GH_POOL_OK&&found==&objects[0]);
 CHECK(gh_room_object_lookup(&pools,h1,8,&owners[0],&found)==GH_POOL_ARGUMENT&&!found);
 CHECK(gh_room_object_unregister(&pools,h1,&owners[1])==GH_POOL_ARGUMENT&&pools.vectors[0].count==2);
 CHECK(gh_game_room_pools_release(&pools)==GH_POOL_UNSUPPORTED&&pools.failed_entry==0x30f8b0&&gh_heap_used(heap)==initial);
 CHECK(gh_room_object_unregister(&pools,h1,&owners[0])==GH_POOL_OK&&u32(pools.vectors[0].allocation.pointer)==h2);
 CHECK(gh_room_object_register(&pools,&objects[2],&owners[0],7,3,&h3)==GH_POOL_OK&&h3>h2);
 CHECK(gh_room_object_unregister(&pools,h3,&owners[0])==GH_POOL_OK&&gh_room_object_unregister(&pools,h2,&owners[1])==GH_POOL_OK);
 gh_game_scene_tables tables={0};gh_boot_work work={0};gh_scene_meshes meshes={0};((uint8_t*)&work)[0x600]=0x55;((uint8_t*)&work)[0x601]=0xa1;((uint8_t*)&work)[0x602]=0xb2;((uint8_t*)&work)[0x603]=0xc3;
 CHECK(gh_scene_meshes_construct_0031d1b0(&meshes,&tables,&work,&room,&pools,heap)==GH_MESH_UNSUPPORTED&&meshes.frontier==0x30fe68);
 CHECK(((uint8_t*)&work)[0x600]==0&&((uint8_t*)&work)[0x601]==0xa1&&((uint8_t*)&work)[0x602]==0xb2&&((uint8_t*)&work)[0x603]==0xc3);
 CHECK(meshes.count==156&&meshes.first.allocation.pointer&&!meshes.first.handle&&!pools.vectors[0].count);
 CHECK(tables.scroll_texture.bytes&&tables.scroll_texture.size==465152&&u32((uint8_t*)&work+0x5fc)==1);
 CHECK(!memcmp(meshes.descriptors[0].mesh.name,"zzz_H04c",8));
 size_t streams=0,vertices=0;int16_t min[3]={32767,32767,32767},max[3]={-32768,-32768,-32768};
 for(size_t i=0;i<meshes.count;i++){
  const gh_scene_mesh_descriptor*m=&meshes.descriptors[i];CHECK(m->mesh.data_num==1&&m->submesh_count>0&&m->submesh_count<=16);
  for(unsigned j=0;j<m->submesh_count;j++){gh_scene_mesh_stream s;CHECK(gh_scene_mesh_stream_at(m,j,&s)==GH_MESH_OK);streams++;vertices+=s.vertices;
   for(size_t k=0;k<s.vertices;k++){int16_t xyz[3];uint16_t w;CHECK(gh_scene_mesh_position(&s,k,xyz,&w)==GH_MESH_OK);for(unsigned axis=0;axis<3;axis++){if(xyz[axis]<min[axis])min[axis]=xyz[axis];if(xyz[axis]>max[axis])max[axis]=xyz[axis];}}
  }
 }
 gh_scene_mesh_stream s;int16_t xyz[3];uint16_t w;
 CHECK(gh_scene_mesh_stream_at(&meshes.descriptors[0],0,&s)==GH_MESH_OK&&s.vertices==48&&s.material==37&&s.header_offset==128&&s.position_offset==160&&s.uv_offset==544&&s.color_offset==736);
 CHECK(gh_scene_mesh_position(&s,0,xyz,&w)==GH_MESH_OK&&xyz[0]==73&&xyz[1]==33&&xyz[2]==63&&w==0x8000);
 CHECK(gh_scene_mesh_position(&s,48,xyz,&w)==GH_MESH_BOUNDS);
 gh_scene_mesh_descriptor invalid=meshes.descriptors[0];invalid.mesh.data_offset=invalid.script.size-8;
 CHECK(gh_scene_mesh_stream_at(&invalid,0,&s)==GH_MESH_BOUNDS);
 CHECK(gh_scene_meshes_release_prefix(&meshes)==GH_MESH_OK&&gh_heap_used(heap)==initial);
 uint8_t serial=254;meshes.factory_serial=&serial;
 CHECK(gh_scene_meshes_construct_0031d1b0(&meshes,&tables,&work,&room,&pools,heap)==GH_MESH_UNSUPPORTED&&meshes.frontier==0x155678&&serial==255);
 CHECK(meshes.first.node_handle && meshes.first.packet_allocation.pointer && meshes.first.packet_bytes==608 && meshes.first.test==0x507fd && pools.blocks[1].used_count==1);
 CHECK(pools.vectors[0].count==1&&pools.vectors[1].count==1&&meshes.first.handle>h3);
 CHECK(gh_room_object_lookup(&pools,meshes.first.handle,GH_ROOM_OBJECT_SCROLL_MESH,&meshes,&found)==GH_POOL_OK&&found==&meshes.first);
 uint8_t*b=meshes.first.allocation.pointer;CHECK(b[0x2fc]==254&&u32(b+0x110)==0x3f800000&&u32(b+0x2c0)==10&&b[0x2b0]==200&&b[0x2be]==254);
 CHECK(meshes.first.local_matrix_offset==0x30&&meshes.first.rotation_matrix_offset==0xb0);
 CHECK(gh_scene_meshes_construct_0031d1b0(&meshes,&tables,&work,&room,&pools,heap)==GH_MESH_UNSUPPORTED&&serial==255&&pools.vectors[0].count==1);
 gh_stage_textures*textures;gh_game_world_light*light=calloc(1,sizeof(*light));CHECK(light&&gh_stage_textures_create(&textures)==GH_STAGE_OK&&gh_game_world_light_construct_002d6d80(light)==GH_LIGHT_OK);
 light->scalar[4]=0xaa;light->scalar[5]=0x55;light->scalar[6]=0x33;light->scalar[7]=0xff;gh_scene_mesh_construction_bindings bindings={textures,light,elf,elf_bytes};CHECK(gh_scene_meshes_bind_construction(&meshes,&bindings)==GH_MESH_OK);
 size_t before_finish=gh_heap_used(heap);uint8_t*rotation=(uint8_t*)meshes.first.descriptor.script.bytes+meshes.first.descriptor.mesh.descriptor_offset+0x1c;rotation[2]=0x80;rotation[3]=0x40;
 CHECK(gh_scene_meshes_construct_0031d1b0(&meshes,&tables,&work,&room,&pools,heap)==GH_MESH_UNSUPPORTED&&meshes.frontier==0x147728&&!meshes.first.constructor_complete&&gh_heap_used(heap)==before_finish);rotation[2]=rotation[3]=0;
 int rounding=fegetround();CHECK(fesetround(FE_DOWNWARD)==0);gh_scene_mesh_status status=gh_scene_meshes_construct_0031d1b0(&meshes,&tables,&work,&room,&pools,heap);printf("construction status%d frontier%08x current%zu completed%zu serial%u\n",status,meshes.frontier,meshes.current_index,meshes.constructed_count,serial);CHECK(status==GH_MESH_OK&&!meshes.frontier&&meshes.current_index==156&&meshes.constructed_count==156&&serial==154&&pools.vectors[0].count==156&&pools.vectors[1].count==156&&pools.blocks[1].used_count==156&&fegetround()==FE_DOWNWARD);CHECK(fesetround(rounding)==0);
 CHECK(meshes.first.constructor_complete&&!meshes.first.render_ready&&meshes.first.materials[0].owner==textures&&meshes.first.materials[0].slot==37&&meshes.first.materials[0].tm3_override_required&&!meshes.first.materials[0].initial_resolved);
 CHECK(meshes.first.fog_rgb==0x3355aa);float actual_bounds[6];memcpy(actual_bounds,b+0x260,24);CHECK(actual_bounds[0]==0.0f&&fabsf(actual_bounds[1]-.335f)<.000001f&&fabsf(actual_bounds[2]+.375f)<.000001f&&actual_bounds[3]==.75f&&fabsf(actual_bounds[4]-1.215f)<.000001f&&fabsf(actual_bounds[5]-1.325f)<.000001f);
 for(unsigned parity=0;parity<2;parity++)for(unsigned matrix=0;matrix<2;matrix++)for(unsigned i=0;i<16;i++)CHECK(u32((uint8_t*)meshes.first.node+0x170+parity*0xd0+matrix*0x80+i*4)==(i%5==0?0x3f800000u:0));
 CHECK(meshes.first.object_packet_allocation.pointer&&meshes.first.node_program[0]==0x8000000au&&meshes.first.node_program[1]==0x8000000au&&meshes.first.object_program[0]==10&&meshes.first.object_program[1]==10);
 CHECK(u32(b+0x250)==0x400480&&u32(b+0x254)==0xc0000000&&u32((uint8_t*)meshes.first.node+0x380)==0x1005000&&b[0x32c]==1&&b[0x2b0]==30&&u32(b+0x2b8)==0x40a00000);
 CHECK(!memcmp(b+0xb0,meshes.descriptors[0].script.bytes+meshes.descriptors[0].mesh.descriptor_offset+0x28,12));
 for(size_t index=0;index<156;index++){const gh_scene_mesh_object*obj;CHECK(gh_scene_meshes_object_at(&meshes,index,&obj)==GH_MESH_OK&&obj->constructor_complete&&!obj->render_ready);CHECK(gh_room_object_lookup(&pools,obj->handle,GH_ROOM_OBJECT_SCROLL_MESH,&meshes,&found)==GH_POOL_OK&&found==obj);}
 const gh_scene_mesh_object*rotated;CHECK(gh_scene_meshes_object_at(&meshes,153,&rotated)==GH_MESH_OK);float rotated_matrix[16];memcpy(rotated_matrix,(uint8_t*)rotated->allocation.pointer+0x80,64);double angle=.26179900765419006;
 CHECK(fabs((double)rotated_matrix[0]-cos(angle))<.000002&&fabs((double)rotated_matrix[2]+sin(angle))<.000002&&fabs((double)rotated_matrix[8]-sin(angle))<.000002&&fabs((double)rotated_matrix[10]-cos(angle))<.000002);
 CHECK(fabs((double)rotated_matrix[0]*rotated_matrix[0]+(double)rotated_matrix[2]*rotated_matrix[2]-1)<.000002&&rotated_matrix[5]==1&&rotated_matrix[15]==1);
 CHECK(gh_scene_meshes_object_at(&meshes,156,&rotated)==GH_MESH_BOUNDS&&!rotated);
 size_t complete_used=gh_heap_used(heap);CHECK(gh_scene_meshes_construct_0031d1b0(&meshes,&tables,&work,&room,&pools,heap)==GH_MESH_OK&&gh_heap_used(heap)==complete_used&&serial==154);
 size_t material_visits=0;for(size_t index=0;index<156;index++){const gh_scene_mesh_object*obj;CHECK(gh_scene_meshes_object_at(&meshes,index,&obj)==GH_MESH_OK);for(unsigned sub=0;sub<obj->descriptor.submesh_count;sub++){const gh_stage_material*view,*again;CHECK(gh_scene_meshes_material_00148d30(&meshes,index,sub,textures,&view)==GH_MESH_OK&&view->rgba_gs&&view->draw_tcc==1&&view->draw_tfx==0);CHECK(gh_scene_meshes_material_00148d30(&meshes,index,sub,textures,&again)==GH_MESH_OK&&again==view&&obj->materials[sub].consumer_visits==2&&!obj->render_ready);material_visits++;}}
 CHECK(material_visits==353);printf("actual148D30 material consumer records%zu, mandatoryTM3cache/nativepixels resolved, GPUstillpending\n",material_visits);
 CHECK(gh_scene_meshes_release_prefix(&meshes)==GH_MESH_OK&&!pools.vectors[0].count&&!pools.vectors[1].count&&gh_heap_used(heap)==initial);
 CHECK(gh_game_room_pools_release(&pools)==GH_POOL_OK&&gh_heap_used(heap)==0&&gh_heap_destroy(heap)==GH_MEM_OK);
 printf("PASS R100 28 SCR/156 mesh descriptors: %zu streams/%zu rawVIF vertices, XYZ[%d,%d,%d]..[%d,%d,%d]; realfactory prefix, sharedregistry identity, default155678 frontier; 156boundconstructors complete including actualSDK rotated objects, FEnv preserved, reverse host cleanup\n",streams,vertices,min[0],min[1],min[2],max[0],max[1],max[2]);
 gh_stage_textures_destroy(textures);free(light);free(elf);free(raw);gh_disc_close(d);return 0;
}
