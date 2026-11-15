#include "game_om309.h"
#include "game_om041c.h"
#include "game_om35d.h"
#include "game_om324.h"
#include "game_om310.h"
#include "game_actor_model_nodes.h"
#include <math.h>
#include "disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fenv.h>
#define C(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint8_t*load(gh_disc*d,const char*path,size_t*n){size_t i;gh_disc_entry e;C(!gh_disc_find(d,path,&i)&&!gh_disc_entry_at(d,i,&e));*n=(size_t)e.size;uint8_t*b=malloc(*n);C(b&&!gh_disc_read(d,i,0,b,*n));return b;}
static uint32_t w(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
int main(int argc,char**argv){C(argc==3);FILE*ef=fopen(argv[2],"rb");C(ef&&!fseek(ef,0,SEEK_END));long en=ftell(ef);C(en>0&&!fseek(ef,0,SEEK_SET));uint8_t*elf=malloc((size_t)en);C(elf&&fread(elf,1,(size_t)en,ef)==(size_t)en);fclose(ef);gh_disc*d;C(!gh_disc_open(argv[1],&d));size_t rn,fn,f3n,f4n,f5n,f7n,fan,fen,f10n,f20n,f22n,f23n,f24n,f3fn,f46n,f54n,f5dn,f9n;uint8_t*raw=load(d,"st1/r100.dat",&rn),*file=load(d,"om/om00.dat",&fn);uint8_t*file3=load(d,"om/om03.dat",&f3n);uint8_t*file4=load(d,"om/om04.dat",&f4n);uint8_t*file5=load(d,"om/om05.dat",&f5n);uint8_t*file7=load(d,"om/om07.dat",&f7n);uint8_t*filea=load(d,"om/om0a.dat",&fan);uint8_t*filee=load(d,"om/om0e.dat",&fen);uint8_t*file10=load(d,"om/om10.dat",&f10n);uint8_t*file20=load(d,"om/om20.dat",&f20n);uint8_t*file22=load(d,"om/om22.dat",&f22n);uint8_t*file23=load(d,"om/om23.dat",&f23n);uint8_t*file24=load(d,"om/om24.dat",&f24n);uint8_t*file3f=load(d,"om/om3f.dat",&f3fn);uint8_t*file46=load(d,"om/om46.dat",&f46n);uint8_t*file54=load(d,"om/om54.dat",&f54n);uint8_t*file5d=load(d,"om/om5d.dat",&f5dn);uint8_t*file9=load(d,"om/om09.dat",&f9n);gh_bundle room;gh_bundle_entry oms;C(!gh_bundle_open(raw,rn,&room)&&!gh_bundle_find(&room,"OMS",0,&oms));gh_om_scene_placements placement={0};int original;C(gh_om_scene_bind_001c3cb8(&placement,&oms,&original)==GH_PLACE_UNSUPPORTED);gh_om_placement_record first;C(!gh_om_scene_record_at(&placement,0,&first)&&first.type==0x300&&first.bytes[0x2d]==0);gh_game_prepare prepare={0};uint8_t state=0;C(!gh_game_prepare_002a5d40(&prepare,(gh_core_save_view){0},0x100,&state));gh_room_state room_state;C(!gh_room_state_select_002bebc8(&prepare,0x100,0,&room_state));gh_heap*heap;gh_room_pools pools={0};C(!gh_heap_create(16*1024*1024,&heap)&&!gh_game_room_pools_initialize_002a5d80(&pools,&room,heap));size_t baseline=gh_heap_used(heap);gh_stage_slots gs={0},rs={0};gh_game_objects*global,*local;C(!gh_game_objects_create(&gs,0,&global)&&!gh_game_objects_create(&rs,1,&local)&&!gh_stage_slots_reset(&gs,NULL)&&!gh_stage_slots_reset(&rs,NULL));int added;C(!gh_game_objects_add_pinned_001fece0(global,0x300,file,fn,&added)&&!gh_game_objects_set_file_extent(global,0x300,file,fn));C(!gh_game_objects_add_pinned_001fece0(global,0x303,file3,f3n,&added)&&!gh_game_objects_set_file_extent(global,0x303,file3,f3n));C(!gh_game_objects_add_pinned_001fece0(global,0x304,file4,f4n,&added)&&!gh_game_objects_set_file_extent(global,0x304,file4,f4n));C(!gh_game_objects_add_pinned_001fece0(global,0x305,file5,f5n,&added)&&!gh_game_objects_set_file_extent(global,0x305,file5,f5n));C(!gh_game_objects_add_pinned_001fece0(global,0x307,file7,f7n,&added)&&!gh_game_objects_set_file_extent(global,0x307,file7,f7n));C(!gh_game_objects_add_pinned_001fece0(global,0x30a,filea,fan,&added)&&!gh_game_objects_set_file_extent(global,0x30a,filea,fan));C(!gh_game_objects_add_pinned_001fece0(global,0x30e,filee,fen,&added)&&!gh_game_objects_set_file_extent(global,0x30e,filee,fen));C(!gh_game_objects_add_pinned_001fece0(global,0x310,file10,f10n,&added)&&!gh_game_objects_set_file_extent(global,0x310,file10,f10n));C(!gh_game_objects_add_pinned_001fece0(global,0x320,file20,f20n,&added)&&!gh_game_objects_set_file_extent(global,0x320,file20,f20n));C(!gh_game_objects_add_pinned_001fece0(global,0x322,file22,f22n,&added)&&!gh_game_objects_set_file_extent(global,0x322,file22,f22n));C(!gh_game_objects_add_pinned_001fece0(global,0x323,file23,f23n,&added)&&!gh_game_objects_set_file_extent(global,0x323,file23,f23n));C(!gh_game_objects_add_pinned_001fece0(global,0x324,file24,f24n,&added)&&!gh_game_objects_set_file_extent(global,0x324,file24,f24n));C(!gh_game_objects_add_pinned_001fece0(global,0x33f,file3f,f3fn,&added)&&!gh_game_objects_set_file_extent(global,0x33f,file3f,f3fn));C(!gh_game_objects_add_pinned_001fece0(global,0x346,file46,f46n,&added)&&!gh_game_objects_set_file_extent(global,0x346,file46,f46n));C(!gh_game_objects_add_pinned_001fece0(global,0x354,file54,f54n,&added)&&!gh_game_objects_set_file_extent(global,0x354,file54,f54n));C(!gh_game_objects_add_pinned_001fece0(global,0x35d,file5d,f5dn,&added)&&!gh_game_objects_set_file_extent(global,0x35d,file5d,f5dn));C(!gh_game_objects_add_pinned_001fece0(global,0x309,file9,f9n,&added)&&!gh_game_objects_set_file_extent(global,0x309,file9,f9n));gh_boot_work work={0};uint8_t serial=0;uint32_t fog=0x123456;gh_om_initializer_bindings init={.work=&work,.packet_heap=heap,.fog_rgb=&fog};gh_om_factory_bindings b={.pools=&pools,.work=&work,.room_state=&room_state,.objects=global,.room_objects=local,.factory_serial=&serial,.initializer=&init};const int modes[]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};gh_game_math_globals math={0};C(!gh_game_math_globals_from_elf(&math,elf,(size_t)en)&&!gh_game_math_static_initialize_003079c8(&math,&work,1,0xffff));gh_collision_pairs pairs={0};C(!gh_collision_pairs_construct_002b8ae0(&pairs,heap)&&!gh_collision_pairs_initialize_002b8b50(&pairs));baseline=gh_heap_used(heap);gh_game_init_state*hitstate=malloc(sizeof(*hitstate));C(hitstate);gh_game_init_state_initialize(hitstate);uint32_t frontier;C(!gh_hitarea_objects_construct_001fde00(hitstate,&frontier)&&!gh_game_object_pool_reset_001fdea8(hitstate));gh_hitarea_shape_bindings shapes={&pairs,&math,GH_STATIC_BINARY32_REFERENCE};uint8_t foreign_raw[GH_TITLE_PLAYER_BYTES]={0};gh_title_player_view foreign_actor;C(!gh_title_player_construct_00107e40(foreign_raw,sizeof(foreign_raw),&foreign_actor));gh_hitarea_object foreign_unit;int nonnull;C(!gh_hitarea_object_create_001fe218(hitstate,&foreign_actor,0,&foreign_unit,&nonnull,&frontier)&&nonnull);uint32_t zero[3]={0};int index;C(!gh_hitarea_add_sphere_001fdaf0(&foreign_unit,&shapes,foreign_raw+0x80,zero,0x3e800000,&index,&frontier)&&index==0);const uint8_t*foreign_node;gh_collision_pair_shape foreign_shape;C(!gh_hitarea_node_at(&foreign_unit,0,&foreign_node,&foreign_shape));gh_game_collision_solids solids={0};gh_game_collision_solids_reset_00134690(&solids);gh_collision_solid_unit*foreign_solid;gh_collision_native_sphere*foreign_solid_sphere;C(!gh_collision_solid_create_unit_001346f8(&solids,heap,&foreign_actor,5,0x3e4ccccd,&foreign_solid,&frontier)&&!gh_collision_solid_create_sphere_00134880(&solids,&foreign_actor,foreign_raw+0x80,zero,0x3f000000,&foreign_solid_sphere,&frontier));baseline=gh_heap_used(heap);gh_game_scene_tables tables={0};C(!gh_scene_actors_0012e670(&tables,&room));gh_scene_dynamic_owner dynamic={0};C(!gh_scene_dynamic_bind(&dynamic,&tables,heap));gh_om_door_shared shared={0};C(!gh_om_door_shared_from_elf(&shared,elf,(size_t)en));

 _Alignas(16) uint8_t fragment308[0x920];
 memset(fragment308,0xa5,sizeof(fragment308));
 gh_title_player_view view308;
 C(gh_om308_construct_0018a598(fragment308,0x90f,&view308)==GH_PLACE_BOUNDS&&fragment308[0]==0xa5);
 C(!gh_om308_construct_0018a598(fragment308,0x910,&view308)&&view308.original_vtable214==0x423700);
 for(unsigned i=0;i<8;++i){
  const uint8_t *segment=fragment308+0x610+i*0x60;
  for(unsigned j=0;j<5;++j) C(!w(segment+j*16)&&!w(segment+j*16+4)&&!w(segment+j*16+8)&&w(segment+j*16+12)==0x3f800000);
  C(!segment[0x54]&&segment[0x55]==0xa5);
 }
 C(fragment308[0x910]==0xa5);
 _Alignas(16)uint8_t constructor[0x9c0];memset(constructor,0xa5,sizeof(constructor));gh_title_player_view constructed;C(gh_om309_construct_0018b290(constructor,sizeof(constructor)-1,&constructed)==GH_PLACE_BOUNDS&&constructor[0]==0xa5);C(!gh_om309_construct_0018b290(constructor,sizeof(constructor),&constructed)&&w(constructor+0x214)==0x4237e8&&(w(constructor+0x5b0)&8));for(unsigned i=0;i<9;i++){uint8_t*p=constructor+0x660+i*0x60;for(unsigned j=0;j<5;j++)C(!w(p+j*16)&&w(p+j*16+12)==0x3f800000);C(!p[0x54]&&p[0x55]==0xa5);}
 for(unsigned mode=0;mode<4;mode++){init.solids=NULL;((uint8_t*)&work)[0x5b1]=(uint8_t)(mode==0?0:mode==1?2:mode==2?5:8);init.player=NULL;init.init=hitstate;init.shapes=&shapes;init.dynamic=&dynamic;init.door_shared=&shared;gh_om_factory factory={0};C(!gh_om_factory_bind(&factory,&b,69));gh_object_resource_bindings resources={0};factory.bindings.resources=&resources;C(!fesetround(modes[mode])&&!feclearexcept(FE_ALL_EXCEPT)&&!feraiseexcept(FE_DIVBYZERO));
 gh_om_scene_placements live={0};C(gh_om_scene_bind_native_001c3cb8(&live,&oms,&factory,&original)==GH_PLACE_UNSUPPORTED&&!original&&live.cursor==44&&live.created==44&&live.frontier==0x2db6b0&&factory.count==44);uint32_t factor=0x3fa00000;memcpy(foreign_raw+0x600,&factor,4);init.player=&foreign_actor;C(gh_om_scene_bind_native_001c3cb8(&live,&oms,&factory,&original)==GH_PLACE_UNSUPPORTED&&!original&&live.cursor==66&&live.created==66&&live.frontier==0x1346f8&&factory.count==66);init.solids=&solids;C(!gh_om_scene_bind_native_001c3cb8(&live,&oms,&factory,&original)&&original&&live.cursor==69&&live.created==69&&!live.skipped&&!live.frontier&&factory.count==69);C(!gh_om_scene_bind_native_001c3cb8(&live,&oms,&factory,&original)&&original&&factory.count==69);
 C(tables.pending_actor_count==5&&solids.unit_count==2&&solids.global_00460d18==2);C(pools.blocks[0].used_count==472&&pools.blocks[1].used_count==137&&pools.vectors[0].count==69&&pools.vectors[3].count==69);C(fegetround()==modes[mode]&&fetestexcept(FE_ALL_EXCEPT)==FE_DIVBYZERO);for(unsigned oi=67;oi<69;oi++){gh_om_object*o=factory.objects+oi;gh_om309_state*st=(gh_om309_state*)o->initialization;uint8_t*p=o->view.bytes;C(st->common.model.part_count==9&&st->common.model.script.mesh_count==2);C(o->source.type==0x309&&o->allocation.bytes==0x9c0&&st->stage==5&&st->common.model.ready&&w(p+0x548)==0x10001&&st->common.hitarea_created);const uint8_t*node,*matrix;gh_collision_pair_shape cylinder;C(!gh_hitarea_node_at(&st->common.hitarea,0,&node,&cylinder)&&!gh_collision_pair_shape_source_matrix(&cylinder,&matrix)&&matrix==p+0x80&&st->common.hitarea.bytes[0x48]==2&&w(cylinder.record+0x1f0)==0x3fa4dd2f&&w(cylinder.record+0x1f4)==0x3f028f5c&&!w(cylinder.record+0x120)&&!w(cylinder.record+0x124)&&!w(cylinder.record+0x128));const gh_actor_node_packets*first,*second;C(!gh_actor_model_packet_at(&st->common.model,0,&first)&&!gh_actor_model_packet_at(&st->common.model,1,&second)&&(w(first->node+0x380)&1)&&!(w(second->node+0x380)&1));for(unsigned i=0;i<st->common.model.part_count;i++)C((w(st->common.model.parts[i].bytes+0x154)&0x18)==((i>0&&i<9)?0x18u:0));C(!memcmp(p+0x490,o->source.bytes,12)&&!memcmp(p+0x100,o->source.bytes+0x10,12));}
 gh_om_placement_record next;C(gh_om_scene_record_at(&placement,69,&next)==GH_PLACE_BOUNDS);C(!gh_om_factory_release(&factory)&&!pools.blocks[0].used_count&&!pools.blocks[1].used_count&&!pools.vectors[0].count&&!pools.arenas[2].live_allocations&&gh_heap_used(heap)==baseline&&!tables.pending_actor_head&&!tables.pending_actor_count&&foreign_unit.bytes[0x4a]&&w(foreign_shape.record)==3&&solids.unit_count==1&&solids.native_units==foreign_solid&&solids.global_00460d18==1);}
 C(!fesetround(FE_TONEAREST));
 size_t r101_size;
 uint8_t *r101=load(d,"st1/r101.dat",&r101_size),*r101_models[12];
 gh_bundle r101_room;gh_bundle_entry r101_oms;
 C(!gh_bundle_open(r101,r101_size,&r101_room)&&!gh_bundle_find(&r101_room,"OMS",0,&r101_oms));
 gh_om_scene_placements r101_placement={0};
 C(gh_om_scene_bind_001c3cb8(&r101_placement,&r101_oms,&original)==GH_PLACE_UNSUPPORTED&&r101_placement.count==46);
 const unsigned rows[]={8,19,23};
 const unsigned kinds[]={0x308,0x30b,0x30c};
 const uint32_t vtables[]={0x423700,0x4239c8,0x423ab8};
 const char *paths[]={"om/om08.dat","om/om0b.dat","om/om0c.dat"};
 gh_om_factory fragments={0};
 C(!gh_om_factory_bind(&fragments,&b,12));
 for(unsigned i=0;i<3;++i){
  size_t model_size;
  r101_models[i]=load(d,paths[i],&model_size);
  C(!gh_game_objects_add_pinned_001fece0(global,kinds[i],r101_models[i],model_size,&added)&&
    !gh_game_objects_set_file_extent(global,kinds[i],r101_models[i],model_size));
  gh_om_placement_record record;
  C(!gh_om_scene_record_at(&r101_placement,rows[i],&record)&&record.type==kinds[i]);
  gh_object_resource_bindings resource_bindings={0};fragments.bindings.resources=&resource_bindings;
  C(!gh_om_factory_create_record(&fragments,&record,&original)&&original);
  const gh_om_object *object=fragments.objects+i;
  const gh_om309_state *fragment=(const gh_om309_state*)object->initialization;
  C(object->view.original_vtable214==vtables[i]&&fragment->common.hitarea_created&&fragment->common.model.ready);
  const uint8_t *node;gh_collision_pair_shape cylinder;
  C(!gh_hitarea_node_at(&fragment->common.hitarea,0,&node,&cylinder)&&
    w(cylinder.record+0x1f0)==0x3fa4dd2f&&w(cylinder.record+0x1f4)==0x3f028f5c);
  for(unsigned j=0;j<fragment->common.model.script.mesh_count;++j){
   const gh_actor_node_packets *packet;
   C(!gh_actor_model_packet_at(&fragment->common.model,j,&packet));
   const unsigned layer=packet->node[0x40c],hidden=w(packet->node+0x380)&1u;
   if(!i&&j<2)C(hidden==(j==0));
   if(i&&(layer==0||layer==0x40))C(hidden==(layer==0x40));
  }
 }
 const unsigned next_rows[]={28,30,31,33,38};
 const unsigned next_kinds[]={0x313,0x317,0x318,0x330,0x351};
 const uint32_t next_vtables[]={0x424128,0x4244d8,0x4245c0,0x425b90,0x427400};
 const char *next_paths[]={"om/om13.dat","om/om17.dat","om/om18.dat","om/om30.dat","om/om51.dat"};
 for(unsigned i=0;i<5;++i){
  size_t model_size;
  r101_models[3+i]=load(d,next_paths[i],&model_size);
  C(!gh_game_objects_add_pinned_001fece0(global,next_kinds[i],r101_models[3+i],model_size,&added)&&
    !gh_game_objects_set_file_extent(global,next_kinds[i],r101_models[3+i],model_size));
  gh_om_placement_record record;
  C(!gh_om_scene_record_at(&r101_placement,next_rows[i],&record)&&record.type==next_kinds[i]);
  gh_object_resource_bindings resources={0};fragments.bindings.resources=&resources;
  C(!gh_om_factory_create_record(&fragments,&record,&original)&&original);
  const gh_om_object *object=fragments.objects+3+i;
  const gh_om_init_state *model=(i==1||i==2)?&((const gh_om303_state*)object->initialization)->common:object->initialization;
  C(object->view.original_vtable214==next_vtables[i]&&model->hitarea_created&&model->model.ready);
  const uint8_t *node;gh_collision_pair_shape shape;
  C(!gh_hitarea_node_at(&model->hitarea,0,&node,&shape));
  if(i==1)C(!gh_hitarea_node_at(&model->hitarea,1,&node,&shape));
  if(i==0||i==3)C(model->geometry_state.box&&model->door_segments==object->view.bytes+0x650);
  else if(i==4)C(w(object->view.bytes+0x548)==0x00280028&&
    w(shape.record+0x1f0)==0x400f5c29&&w(shape.record+0x1f4)==0x3f4ccccd);
  else C(object->view.bytes[0x60c]==(i==1?6:5));
 }
 size_t canopy_size;
 r101_models[8]=load(d,"om/om57.dat",&canopy_size);
 C(!gh_game_objects_add_pinned_001fece0(global,0x357,r101_models[8],canopy_size,&added)&&
    !gh_game_objects_set_file_extent(global,0x357,r101_models[8],canopy_size));
 gh_om_placement_record canopy_record;
 C(!gh_om_scene_record_at(&r101_placement,39,&canopy_record)&&canopy_record.type==0x357);
 gh_object_resource_bindings canopy_resources={0};fragments.bindings.resources=&canopy_resources;
 init.elf=elf;init.elf_bytes=(size_t)en;
 C(!gh_om_factory_create_record(&fragments,&canopy_record,&original)&&original);
 const gh_om_object *canopy=fragments.objects+8;
 const gh_om041c_state *cloth=(const gh_om041c_state*)canopy->initialization;
 const uint8_t *canopy_bytes=canopy->view.bytes;
 C(cloth->initialized&&cloth->common.model.ready&&canopy->view.original_vtable214==0x427a58);
 C(w(canopy_bytes+0xe40)==2&&w(canopy_bytes+0xe44)==4&&w(canopy_bytes+0xe48)==38&&
    (w(canopy_bytes+0x5a0)&0x10));
 float cloth_mass=0;
 for(unsigned i=0;i<15;++i){
  float mass;memcpy(&mass,canopy_bytes+0x600+i*0x50+0x40,4);
  C(isfinite(mass)&&mass>0);
  cloth_mass+=mass;
 }
 C(fabsf(cloth_mass-5.0f)<0.00001f&&w(canopy_bytes+0xab0+37*0x18+0x10));
 uint8_t item_save_bytes[GH_CORE_SAVE_BYTES];
 init.save=(gh_core_save_view){item_save_bytes,sizeof(item_save_bytes)};
 C(!gh_core_save_system_init(init.save));
 work.room=0x101;
 const char *item_paths[]={"om/omd3.dat","om/omd4.dat","om/omd5.dat"};
 for(unsigned i=0;i<3;++i){
  size_t item_size;
  const unsigned kind=0x3d3+i;
  r101_models[9+i]=load(d,item_paths[i],&item_size);
  C(!gh_game_objects_add_pinned_001fece0(global,kind,r101_models[9+i],item_size,&added)&&
    !gh_game_objects_set_file_extent(global,kind,r101_models[9+i],item_size));
  gh_om_placement_record record;
  C(!gh_om_scene_record_at(&r101_placement,43+i,&record)&&record.type==kind);
  C(!gh_om_factory_create_record(&fragments,&record,&original)&&original);
  C(fragments.objects[9+i].view.original_vtable214==0x429b88);
  const uint8_t *saved=item_save_bytes+0x28a+16*i;
  C((w(saved)&0xffff)==0x101&&w(saved)>>16==kind&&w(saved+4)==((i+1)<<16|1u));
  for(unsigned j=0;j<3;++j){
   float xyz;int16_t saved_coord;
   memcpy(&xyz,record.bytes+4*j,4);memcpy(&saved_coord,saved+8+2*j,2);
   C(saved_coord==(int16_t)(xyz*10.0f));
  }
 }
 C((w(item_save_bytes+0x288)&0xffff)==4);
 C(!gh_om_factory_release(&fragments)&&!pools.arenas[2].live_allocations);
 free(r101);
 /* Numeric leaf is deliberately boundedPCSX2-defaultChopZeroREFERENCE,
 nothardwareproof. Fourcallerroundingmodes mustnotchange111 vsnearest112. */
 for(unsigned m=0;m<4;m++){C(!fesetround(modes[m])&&!feclearexcept(FE_ALL_EXCEPT)&&!feraiseexcept(FE_DIVBYZERO));int32_t value=777;uint32_t pc=0;C(!gh_om310_damage_rz_reference(0x3fb33333,&value,&pc)&&value==111&&!pc);C(!gh_om310_damage_rz_reference(0xbfb33333,&value,&pc)&&value==-111);C(!gh_om310_damage_rz_reference(0x3fa00000,&value,&pc)&&value==100);C(!gh_om310_damage_rz_reference(0x3f800000,&value,&pc)&&value==80);value=777;C(gh_om310_damage_rz_reference(0x49800000,&value,&pc)==GH_PLACE_UNSUPPORTED&&pc==0x18f218&&value==777);C(gh_om310_damage_rz_reference(0x7fc00000,&value,&pc)==GH_PLACE_UNSUPPORTED&&value==777);C(fegetround()==modes[m]&&fetestexcept(FE_ALL_EXCEPT)==FE_DIVBYZERO);}
 for(unsigned m=0;m<4;m++){C(!fesetround(modes[m])&&!feclearexcept(FE_ALL_EXCEPT)&&!feraiseexcept(FE_DIVBYZERO));for(unsigned selector=0;selector<256;selector++){uint32_t result[3],pc=0;C(!gh_om324_scale_reference((uint8_t)selector,result,&pc)&&!pc);const uint32_t raw[3]={0x40000000,0x3f99999a,0x3e4ccccd};float den[3],num[3],actual[3];memcpy(den,raw,12);memcpy(num,raw,12);memcpy(actual,result,12);if(selector==1){num[0]=1;uint32_t n=0x3fb33333;memcpy(num+1,&n,4);}else if(selector==2){uint32_t n=0x3f666666;memcpy(num,&n,4);memcpy(num+1,&n,4);}fenv_t oracle;C(!feholdexcept(&oracle));for(unsigned k=0;k<3;k++)C(fabsl((long double)actual[k]-(long double)num[k]/den[k])<0.00000006L);C(!fesetenv(&oracle));C(fegetround()==modes[m]&&fetestexcept(FE_ALL_EXCEPT)==FE_DIVBYZERO);}}
 /* Independent exactbinary64 rational-product oracle for all256originalswitchinputs. */for(unsigned m=0;m<4;m++){C(!fesetround(modes[m])&&!feclearexcept(FE_ALL_EXCEPT)&&!feraiseexcept(FE_DIVBYZERO));const uint32_t factors[]={0x3f800000,0x3f800000,0x3fa66666,0x3fcccccd,0x40000000,0x40133333,0x40266666,0x40400000,0x40533333};for(unsigned level=0;level<256;level++){uint16_t hp;uint32_t pc=0;C(!gh_om35d_hp_chopzero_reference((uint8_t)level,&hp,&pc)&&!pc);uint32_t bits=factors[level<=8?level:0];float f;memcpy(&f,&bits,4);fenv_t oracle;C(!feholdexcept(&oracle));double exact=(double)f*140.0;volatile uint16_t expected=(uint16_t)exact;C(hp==expected);C(!fesetenv(&oracle));C(fegetround()==modes[m]&&fetestexcept(FE_ALL_EXCEPT)==FE_DIVBYZERO);}uint16_t hp;uint32_t pc=0;C(!gh_om35d_hp_chopzero_reference(2,&hp,&pc)&&hp==181&&!gh_om35d_hp_chopzero_reference(5,&hp,&pc)&&hp==321&&!gh_om35d_hp_chopzero_reference(8,&hp,&pc)&&hp==461);}
 C(!fesetround(FE_TONEAREST));float factor;uint32_t bits=0x3fb33333;memcpy(&factor,&bits,4);volatile float nearest=factor*80.0f;C((int)nearest==112);
 C(!gh_collision_solid_release_unit_001347e8(&solids,&foreign_actor,&frontier)&&gh_collision_solid_units_empty(&solids));C(!gh_hitarea_shapes_abort_object(&foreign_unit,&frontier)&&!gh_hitarea_object_release_empty_001fe278(&foreign_unit,&frontier)&&!gh_game_object_pool_release_001fdfa8(hitstate,&frontier));free(hitstate);C(!gh_scene_dynamic_release(&dynamic)&&!gh_scene_actors_reset_0012e800(&tables)&&!gh_collision_pairs_release(&pairs));C(!fesetround(FE_TONEAREST));gh_game_objects_destroy(local);gh_game_objects_destroy(global);C(!gh_game_room_pools_release(&pools)&&!gh_heap_used(heap)&&!gh_heap_destroy(heap));for(unsigned i=0;i<12;++i)free(r101_models[i]);free(file9);free(file5d);free(file54);free(file46);free(file3f);free(file24);free(file23);free(file22);free(file20);free(file10);free(filee);free(filea);free(file7);free(file5);free(file4);free(file3);free(elf);free(file);free(raw);gh_disc_close(d);puts("PASS actual R100 69-object/4-FEnv fixture and actual R101 fragment, door, canopy, pillar and persistent pickup placement");return 0;}
