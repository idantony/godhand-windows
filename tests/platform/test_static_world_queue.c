#include "static_world_queue.h"
#include "game_stage.h"
#include "disc.h"
#include "ui_order.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fenv.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);}}while(0)
int main(int argc,char**argv){C(argc==2);gh_disc*d;size_t i;gh_disc_entry e;C(!gh_disc_open(argv[1],&d)&&!gh_disc_find(d,"st1/r100.dat",&i)&&!gh_disc_entry_at(d,i,&e));uint8_t*raw=malloc((size_t)e.size);C(raw&&!gh_disc_read(d,i,0,raw,(size_t)e.size));gh_disc_close(d);
 gh_bundle room,scp;gh_bundle_entry child,scr,tm3;C(!gh_bundle_open(raw,(size_t)e.size,&room)&&!gh_bundle_find(&room,"SCP",0,&child)&&!gh_bundle_open(child.bytes,child.size,&scp)&&!gh_bundle_find(&scp,"SCR",0,&scr)&&!gh_bundle_find(&scp,"TM3",0,&tm3));
 gh_scene_mesh_object object={0};object.fog_rgb=0x123456;object.handle=17;object.constructor_complete=1;gh_scene_mesh_descriptor*desc=&object.descriptor;C(!gh_model_open_scr(scr.bytes,scr.size,&desc->script)&&!gh_model_mesh_at(&desc->script,0,&desc->mesh));desc->node_flags=0x1001000;desc->submesh_count=desc->script.bytes[desc->mesh.data_offset+10];C(desc->submesh_count==1);
 gh_scene_mesh_stream stream;C(!gh_scene_mesh_stream_at(desc,0,&stream)&&stream.vertices==48&&stream.material==37);gh_stage_textures*textures;C(!gh_stage_textures_create(&textures));const gh_stage_material*m;C(!gh_stage_tm3_resolve(textures,tm3.bytes,tm3.size,stream.material,1,&m));
 gh_object_static_command cmd={0};cmd.kind=GH_OBJECT_NODE_GEOMETRY;cmd.object=&object;cmd.handle=17;cmd.bank=1;cmd.depth=13;cmd.selected_program=10;cmd.material_count=1;cmd.materials[0]=m;
 gh_static_transform_inputs in={.screen_matrix={100,0,0,0,0,-100,0,0,0,0,1,.05f,2048,2048,1000,2},.clip_matrix={.25f,0,0,0,0,.25f,0,0,0,0,.25f,0,0,0,0,1},.uv_offset={.25f,-.125f},.color_scale={.75f,1,1.25f,.5f},.fog={100,-25}};cmd.packet.inputs=in;const float projection[16]={100,0,0,0,0,100,0,0,0,0,1,0,2048,2048,1000,1};memcpy(cmd.packet.projection,projection,sizeof(projection));
 gh_static_world_queue*q;C(!gh_static_world_queue_create(&q));size_t index=99;gh_static_transform_frontier f;C(!fesetround(FE_DOWNWARD)&&!gh_static_world_queue_append(q,&cmd,&index,&f)&&index==0&&fegetround()==FE_DOWNWARD);C(!fesetround(FE_TONEAREST));gh_static_world_group g;C(!gh_static_world_queue_group_at(q,0,&g)&&g.packets==2&&g.captured_fog_rgb==0x123456);object.fog_rgb=0xabcdef;C(!gh_static_world_queue_group_at(q,0,&g)&&g.captured_fog_rgb==0x123456);size_t vertices=0;
 for(size_t p=0;p<2;p++){gh_static_world_packet v;C(!gh_static_world_queue_packet_at(q,0,p,&v)&&v.batch==p&&v.substream==0&&v.material==m&&v.material_index==37);C(v.vertex_count==(p?12:38));vertices+=v.vertex_count;for(size_t j=0;j<v.triangles*3;j++)C(v.indices[j]<v.vertex_count);}
 C(vertices==50);cmd.kind=GH_OBJECT_SETUP;C(!gh_static_world_queue_append(q,&cmd,&index,&f)&&index==1&&!gh_static_world_queue_group_at(q,1,&g)&&!g.packets);gh_ui_order_group groups[2]={{1,13,0},{1,13,1}};size_t order[2],written,omitted;C(!gh_ui_order_indices(groups,2,0,0,order,2,&written,&omitted)&&written==2&&order[0]==1&&order[1]==0);
 cmd.kind=GH_OBJECT_NODE_GEOMETRY;cmd.selected_program=99;index=999;C(gh_static_world_queue_append(q,&cmd,&index,&f)==GH_WORLD_UNSUPPORTED&&index==999&&gh_static_world_queue_count(q)==2&&f.ee_entry==0x155dc8);
 cmd.selected_program=14;C(!gh_static_world_queue_append(q,&cmd,&index,&f)&&index==2);C(!gh_static_world_queue_group_at(q,2,&g)&&g.packets==2);C(gh_static_world_queue_packet_at(q,2,2,&(gh_static_world_packet){0})==GH_WORLD_BOUNDS);
 /* Explicit clip-plane fixture over actual immutable source attributes; no
  * runtime camera claim. Preserve fans before strip within each batch. */
 cmd.selected_program=10;cmd.packet.inputs.clip_matrix[0]=2;cmd.packet.inputs.clip_matrix[5]=2;
 C(!gh_static_world_queue_append(q,&cmd,&index,&f)&&index==3);C(!gh_static_world_queue_group_at(q,3,&g));size_t fans=0,last_batch=0;int strip=0;
 for(size_t p=0;p<g.packets;p++){gh_static_world_packet v;C(!gh_static_world_queue_packet_at(q,3,p,&v));if(v.batch!=last_batch){C(strip);strip=0;last_batch=v.batch;}if(v.primitive==0x7d){C(!strip);fans++;}else{C(v.primitive==0x7c&&!strip);strip=1;}for(size_t j=0;j<v.triangles*3;j++)C(v.indices[j]<v.vertex_count);}
 C(fans&&strip);gh_static_world_queue_reset(q);C(!gh_static_world_queue_count(q));gh_static_world_queue_destroy(q);gh_stage_textures_destroy(textures);free(raw);puts("PASS actualR100 SCR48/TM3member37 program10/14 two batches, setup OT LIFO, local indices/material identity, atomic unsupported/FEnv; CPU queue not GPU frame");return 0;}
