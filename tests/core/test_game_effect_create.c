#include "game_effect_create.h"
#include "game_camera_envelope.h"
#include <math.h>
#include "game_player_init_tail.h"
#include "game_player_child_tail.h"
#include "game_world_light.h"
#include "game_actor_external_parent.h"
#include <fenv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);}}while(0)
static void put(uint8_t*p,uint32_t v){memcpy(p,&v,4);}
static uint32_t w(const uint8_t*p){uint32_t v;memcpy(&v,p,4);return v;}
static uint8_t*load(gh_disc*d,const char*path,size_t*n){size_t i;gh_disc_entry e;C(!gh_disc_find(d,path,&i)&&!gh_disc_entry_at(d,i,&e));*n=(size_t)e.size;uint8_t*p=malloc(*n);C(p&&!gh_disc_read(d,i,0,p,*n));return p;}
typedef struct {gh_actor_model model;gh_game_player_factory*factory;gh_heap*heap;gh_game_world_light*light;gh_player_shared_vector vector;gh_player_child_tail child_tail;gh_game_player_child*publication;gh_core_save_view save;uint8_t*elf;size_t elf_size;} context;
static gh_player_init_status model_init(void*raw,gh_game_title_player*player,gh_boot_work*work,const gh_bundle_entry*md,const gh_bundle_entry*tm,uint32_t mode,int*original,uint32_t*frontier){context*c=raw;gh_actor_model_status e=gh_actor_model_initialize_00149738(&c->model,&player->view,work,c->factory->b.pools,md,tm,0,mode,GH_STATIC_BINARY32_REFERENCE,original);*frontier=c->model.frontier;return e?GH_PLAYER_INIT_UNSUPPORTED:GH_PLAYER_INIT_OK;}
static gh_title_player_status child_init(void*raw,gh_game_player_child*child,int*original,uint32_t*frontier){context*c=raw;if(!child->model.bound)C(!gh_actor_model_bind_packet_inputs(&child->model,c->heap,c->light->scalar+4));gh_title_player_status e=gh_game_player_child_initialize_00100378(child,original);if(e==GH_PLAYER_UNSUPPORTED&&child->frontier==0x100420)e=gh_game_player_child_finish_00100420(&c->child_tail,child,&c->vector,&c->publication,c->save,c->elf,c->elf_size,GH_STATIC_BINARY32_REFERENCE,original);*frontier=e?(c->child_tail.frontier?c->child_tail.frontier:child->frontier):0;return e;}
static void test_om324_break(const void *core,size_t core_size,const gh_bundle *room,
 const void *elf,size_t elf_size)
{
 uint8_t *esp=calloc(1,0x100180),*emit=calloc(1,0x26060);
 gh_game_effects *effects=calloc(1,sizeof(*effects));
 C(esp&&emit&&effects&&!gh_game_effects_initialize_00305a68(effects,esp,0x100180,emit,0x26060,core,core_size,room));
 gh_effect_view est;
 C(!gh_effect_est_at_00306030(effects,0,0x1c,&est)&&est.bytes==(const uint8_t *)core+0x10b820&&est.size==2272);
 C(w(est.bytes)==0x545345&&w(est.bytes+4)==3);
 gh_boot_work work={.room=0x100};
 gh_game_mt random;
 gh_game_mt_seed_0030f2b0(&random,0x324);
 gh_game_mt oracle=random;
 uint32_t seeds[3];
 for(unsigned i=0;i<3;++i)C(!gh_game_mt_next_0030f348(&oracle,seeds+i));
 gh_effect_create_bindings b={.storage={.effects=effects,.esp=esp,.emitters=emit,
  .esp_bytes=0x100180,.emitter_bytes=0x26060},.work=&work,.random=&random};
 const uint32_t position[]={0x421a0000,0x40e80000,0xc2b60000};
 const uint32_t yaw=0x3f400000;
 gh_effect_create_call call={0};
 const uint32_t invalid[]={0x7fc00000,0,0};
 C(gh_effect_create_om324_break_begin_001bfd28(&call,invalid,yaw)==GH_EFFECT_UNSUPPORTED&&!call.active);
 C(!gh_effect_create_om324_break_begin_001bfd28(&call,position,yaw));
 C(gh_game_effect_create_00306528(&call,&b)==GH_EFFECT_UNSUPPORTED&&call.frontier==0x147c88&&
   call.stage==3&&call.row==0&&call.slot==0&&w(emit+0x26000)==0x80000000&&w(emit+0x220)==seeds[0]);
 C(gh_effect_create_om324_break_begin_001bfd28(&call,position,yaw)==GH_EFFECT_ARGUMENT);
 gh_effect_emitter_view view;
 C(!gh_effect_create_emitter_at(effects,0,&view)&&!view.initialized);
 gh_camera_envelope camera={0};
 uint32_t frontier=0;
 C(!gh_game_camera_envelope_from_elf(&camera,elf,elf_size,&frontier));
 b.atan_coeff=camera.atan_coeff;
 C(!gh_game_effect_create_00306528(&call,&b)&&call.complete&&call.original_return==1&&
   call.row==3&&w(emit+0x26000)==0xe0000000&&!memcmp(&oracle,&random,sizeof(random)));
 for(unsigned i=0;i<3;++i){
  const uint8_t *row=est.bytes+16+i*0x2f0;
  C(!gh_effect_create_emitter_at(effects,i,&view)&&view.initialized&&view.record.bytes==row&&
    !view.actor&&!view.selector&&!view.model&&!view.part&&view.first_esp_child==-1);
  C(!row[0x128]&&!row[0x136]&&row[0xbe]==0xff&&!row[0x28e]&&w(row+4)==0x800000);
  const uint8_t *p=view.payload;
  C(w(p+0x110)==0xffffffff&&w(p+0x18c)==0x001c0000&&w(p+0x220)==seeds[i]);
  C(w(p+0x1b4)==yaw&&!memcmp(p+0x1c0,position,12)&&!memcmp(p+0x40,position,12));
  float euler[3];memcpy(euler,p+0x30,12);
  C(euler[0]==0&&fabsf(euler[1]-.75f)<0.00001f&&euler[2]==0&&!p[0x16]&&!w(p+0x90));
 }
 C(!gh_game_effect_create_00306528(&call,&b)&&!memcmp(&oracle,&random,sizeof(random))&&w(emit+0x26000)==0xe0000000);
 gh_effect_create_forget_on_owner_destroy(effects);
 free(effects);free(esp);free(emit);
}

int main(int argc,char**argv){C(argc==3);context c={0};FILE*f=fopen(argv[2],"rb");C(f&&!fseek(f,0,SEEK_END));long size=ftell(f);C(size>0&&!fseek(f,0,SEEK_SET));c.elf_size=(size_t)size;c.elf=malloc(c.elf_size);C(c.elf&&fread(c.elf,1,c.elf_size,f)==c.elf_size);fclose(f);C(!gh_player_shared_vector_from_elf(&c.vector,c.elf,c.elf_size));gh_game_math_globals math={0};gh_boot_work work={.room=0x100};C(!gh_game_math_globals_from_elf(&math,c.elf,c.elf_size)&&!gh_game_math_static_initialize_003079c8(&math,&work,1,0xffff));gh_game_init_state*init=malloc(sizeof(*init));C(init);gh_game_init_state_initialize(init);C(!gh_hitarea_objects_construct_001fde00(init,NULL)&&!gh_game_object_pool_reset_001fdea8(init));uint8_t save_bytes[GH_CORE_SAVE_BYTES];c.save=(gh_core_save_view){save_bytes,sizeof(save_bytes)};C(!gh_core_save_system_init(c.save)&&!gh_core_save_game_init(c.save,&work));gh_disc*d;C(!gh_disc_open(argv[1],&d));size_t room_size,pl_size,child_size;uint8_t*room=load(d,"st1/r100.dat",&room_size),*pl=load(d,"pl/pl00.dat",&pl_size),*hair=load(d,"pl/pl01.dat",&child_size);gh_bundle bundle;C(!gh_bundle_open(room,room_size,&bundle));gh_room_pools pools={0};C(!gh_heap_create(32*1024*1024,&c.heap)&&!gh_game_room_pools_initialize_002a5d80(&pools,&bundle,c.heap));gh_game_world_light*light=calloc(1,sizeof(*light));C(light);c.light=light;C(!gh_game_world_light_construct_002d6d80(light)&&!gh_game_world_light_initialize_with_heap_002d6f40(light,&bundle,c.heap));gh_stage_slots gs={0},rs={0};gh_game_objects*global,*objects;C(!gh_game_objects_create(&gs,0,&global)&&!gh_game_objects_create(&rs,1,&objects)&&!gh_stage_slots_reset(&gs,NULL)&&!gh_stage_slots_reset(&rs,NULL));int added;C(!gh_game_objects_add_pinned_001fece0(global,0x100,pl,pl_size,&added)&&!gh_game_objects_set_file_extent(global,0x100,pl,pl_size)&&!gh_game_objects_add_pinned_001fece0(global,0x101,hair,child_size,&added)&&!gh_game_objects_set_file_extent(global,0x101,hair,child_size));uint8_t serial=33;gh_game_title_player player={0};gh_game_player_factory factory={0};c.factory=&factory;gh_player_factory_bindings fb={&player,&pools,global,objects,&work,d,c.save,&serial};int original;C(!gh_game_player_factory_create_0030fa98(&factory,&fb,&original)&&original&&serial==34);C(!gh_actor_model_bind_packet_inputs(&c.model,c.heap,light->scalar+4));gh_game_player_init prefix={0};gh_player_init_bindings pb={.factory=&factory,.work=&work,.current_player=&player,.model_initialize_00149738=model_init,.model_context=&c};C(gh_game_player_initialize_00107f58(&prefix,&pb)==GH_PLAYER_INIT_UNSUPPORTED&&prefix.frontier==0x30fa98&&prefix.stage==4&&prefix.model_call_complete&&c.model.ready&&c.model.parts_allocated==32);
 gh_collision_pairs pairs={0};C(!gh_collision_pairs_construct_002b8ae0(&pairs,c.heap)&&!gh_collision_pairs_initialize_002b8b50(&pairs));gh_game_collision_solids solids={0};gh_game_player_child child={0};gh_player_child_attachment attachment={0};gh_game_player_init_tail tail={0};gh_player_init_tail_bindings tb={.prefix=&prefix,.parent_model=&c.model,.child=&child,.attachment=&attachment,.solids=&solids,.default_heap=c.heap,.save=c.save,.initialize_child=child_init,.child_context=&c,.init=init,.pairs=&pairs,.math_globals=&math,.shape_policy=GH_STATIC_BINARY32_REFERENCE};gh_player_init_status status=gh_game_player_initialize_tail_00107f58(&tail,&tb);fprintf(stderr,"actualtail status%d stage%u frontier%08X take%u give%u serial%u\n",status,tail.stage,tail.frontier,tail.take_step,tail.give_step,serial);C(status==GH_PLAYER_INIT_UNSUPPORTED&&tail.frontier==0x14b730&&tail.stage==11&&tail.take_step==4&&tail.give_step==5&&tail.pending_lookup_name==0x41b828&&tail.active_child&&attachment.attached&&c.publication==&child&&c.child_tail.complete&&c.child_tail.springs.initialized&&serial==35&&tail.take_object.index==160&&tail.solid_unit&&tail.sphere&&gh_collision_pair_shapes_have_live(&pairs));for(unsigned i=0;i<4;i++){C(tail.take_sphere_indices[i]==(int)i&&w(player.view.bytes+0x67c+i*4)==i);const uint8_t*node;gh_collision_pair_shape shape;C(!gh_hitarea_node_at(&tail.take_object,i,&node,&shape)&&w(shape.record)==3&&w(shape.record+0x10)==2&&w(shape.record+0x1f0)==0x3e800000);}for(unsigned i=0;i<5;i++){C(tail.give_objects[i].index==i&&tail.give_shape_indices[i]==(int)i&&w(player.view.bytes+0x5b4+i*8)==i);const uint8_t*node;gh_collision_pair_shape shape;C(!gh_hitarea_node_at(&tail.give_objects[i],0,&node,&shape)&&w(shape.record)==1&&w(shape.record+0x10)==1&&w(shape.record+0xe0)==(i<4?2u:4u));}

 size_t core_size;uint8_t*core=load(d,"etc/core.dat",&core_size);uint8_t*esp=calloc(1,0x100180),*emit=calloc(1,0x26060);gh_game_effects*effects=calloc(1,sizeof(*effects));C(esp&&emit&&effects&&!gh_game_effects_initialize_00305a68(effects,esp,0x100180,emit,0x26060,core,core_size,&bundle));
 gh_effect_create_bindings eb={.storage={.effects=effects,.esp=esp,.emitters=emit,.esp_bytes=0x100180,.emitter_bytes=0x26060},.work=&work,.random=&init->random,.model=&c.model};

 gh_effect_view saved_table=effects->resources[0].tbl1;uint8_t malformed[4];put(malformed,0xffffffff);effects->resources[0].tbl1=(gh_effect_view){malformed,sizeof(malformed)};
 gh_effect_create_request bad_request={.bank=0,.index=0x21,.actor=&player.view,.mode=3,.selector=-1};gh_effect_create_call bad={0};C(!gh_effect_create_begin(&bad,&bad_request)&&gh_game_effect_create_00306528(&bad,&eb)==GH_EFFECT_BOUNDS&&bad.frontier==0x306030&&!w(emit+0x26000));effects->resources[0].tbl1=saved_table;
 C(!fesetround(FE_UPWARD));feclearexcept(FE_ALL_EXCEPT);feraiseexcept(FE_INVALID);int old_exceptions=fetestexcept(FE_ALL_EXCEPT);
 gh_game_mt_seed_0030f2b0(&init->random,123);gh_game_mt oracle=init->random;
 gh_effect_create_request request={.bank=0,.index=0x2e,.actor=&player.view,.mode=-1,.selector=-1};gh_effect_create_call ec={0};C(!gh_effect_create_begin(&ec,&request));
 C(!gh_game_effect_create_00306528(&ec,&eb)&&ec.complete&&ec.original_return==1&&ec.row==11&&w(emit+0x26000)==0xffe00000);
 C(fegetround()==FE_UPWARD&&fetestexcept(FE_ALL_EXCEPT)==old_exceptions);
 gh_effect_emitter_view view;C(!gh_effect_create_emitter_at(effects,0,&view)&&view.initialized&&view.actor->bytes==player.view.bytes&&view.selector->bytes==player.view.bytes&&view.model==&c.model&&view.part==&c.model.parts[9]&&view.record.size==0x2f0);
 const uint8_t expected_seeds[11]={3,0,3,0,0,0,0,2,3,1,1};for(unsigned i=0;i<11;i++){uint32_t seed=expected_seeds[i];if(!seed)C(!gh_game_mt_next_0030f348(&oracle,&seed));C(w(emit+i*0x260+0x220)==seed&&w(emit+i*0x260+0x110)==0xffffffff&&emit[i*0x260+0x18e]==0x2e&&w(emit+i*0x260+0xf0)==0x44fd58);}
 C(!memcmp(&oracle,&init->random,sizeof(oracle)));gh_game_mt after=init->random;C(!gh_game_effect_create_00306528(&ec,&eb)&&!memcmp(&after,&init->random,sizeof(after))&&w(emit+0x26000)==0xffe00000);
 int matched;uint32_t frontier;gh_title_player_view wrapper=player.view;C(!gh_effect_create_actor_match(effects,1,0,&wrapper,&matched,&frontier)&&matched);gh_effect_release_bindings rb=eb.storage;rb.actor_match=gh_effect_create_actor_match;rb.context=effects;unsigned slot;C(gh_game_effect_release_00306200(&rb,&player.view,-1,0,&frontier,&slot)==GH_EFFECT_UNSUPPORTED&&frontier==0x302918);
 request.index=0x21;request.mode=3;gh_effect_create_call heat={0};C(!gh_effect_create_begin(&heat,&request));C(!gh_game_effect_create_00306528(&heat,&eb)&&heat.complete&&heat.original_return==1&&heat.row==10);
 for(unsigned i=0;i<10;i++){uint32_t seed;C(!gh_game_mt_next_0030f348(&oracle,&seed));C(w(emit+(11+i)*0x260+0x220)==seed);}C(!memcmp(&oracle,&init->random,sizeof(oracle))&&w(emit+0x26000)==0xfffff800);
 for(unsigned i=0;i<3;i++)C(!gh_game_effect_create_00306528(&heat,&eb)&&!memcmp(&oracle,&init->random,sizeof(oracle))&&w(emit+0x26000)==0xfffff800);
 C(w(emit+16*0x260+0x34)==0x80000000&&w(emit+16*0x260+0x50)==0x3f800000&&w(emit+16*0x260+0xb0)==0x3f800000);
 gh_effect_create_call copied=heat;C(gh_game_effect_create_00306528(&copied,&eb)==GH_EFFECT_ARGUMENT);

 /* Late model-provider failure preserves actual reserved slot and one MT
  * draw; retry never allocates/reseeds that same emitter again. */
 gh_effect_create_call late={0};C(!gh_effect_create_begin(&late,&request));eb.model=NULL;C(gh_game_effect_create_00306528(&late,&eb)==GH_EFFECT_UNSUPPORTED&&late.frontier==0x303a30&&late.stage==3&&late.row==0&&late.slot==21);
 uint32_t late_seed;C(!gh_game_mt_next_0030f348(&oracle,&late_seed)&&w(emit+21*0x260+0x220)==late_seed&&!memcmp(&oracle,&init->random,sizeof(oracle))&&w(emit+0x26000)==0xfffffc00);
 for(unsigned i=0;i<3;i++)C(gh_game_effect_create_00306528(&late,&eb)==GH_EFFECT_UNSUPPORTED&&late.frontier==0x303a30&&!memcmp(&oracle,&init->random,sizeof(oracle))&&w(emit+0x26000)==0xfffffc00);
 eb.model=&c.model;C(!gh_game_effect_create_00306528(&late,&eb)&&late.complete&&late.original_return==1);for(unsigned i=1;i<10;i++){uint32_t seed;C(!gh_game_mt_next_0030f348(&oracle,&seed));}C(!memcmp(&oracle,&init->random,sizeof(oracle))&&w(emit+0x26000)==0xfffffffe);
 uint32_t saved5d8=w((uint8_t*)&work+0x5d8);put((uint8_t*)&work+0x5d8,0x10000000);gh_effect_create_call suppressed={0};C(!gh_effect_create_begin(&suppressed,&request)&&!gh_game_effect_create_00306528(&suppressed,&eb)&&suppressed.complete&&suppressed.original_return==1&&!memcmp(&oracle,&init->random,sizeof(oracle))&&w(emit+0x26000)==0xfffffffe);put((uint8_t*)&work+0x5d8,saved5d8);
 gh_effect_view original_table=effects->resources[0].tbl1;uint8_t unsupported_type[8+16+0x2f0]={0};put(unsupported_type,1);put(unsupported_type+4,8);put(unsupported_type+8,0x545345);put(unsupported_type+12,1);unsupported_type[8+16+0x128]=1;unsupported_type[8+16+0xbe]=0xff;effects->resources[0].tbl1=(gh_effect_view){unsupported_type,sizeof(unsupported_type)};
 gh_effect_create_request unknown={.bank=0,.index=1,.actor=&player.view,.mode=-1,.selector=-1};gh_effect_create_call pending={0};C(!gh_effect_create_begin(&pending,&unknown)&&gh_game_effect_create_00306528(&pending,&eb)==GH_EFFECT_UNSUPPORTED&&pending.frontier==0x304d70&&w(emit+0x26000)==0xfffffffe);effects->resources[0].tbl1=original_table;
 /* Native host destruction detaches only metadata, never fake kills slots. */
 test_om324_break(core,core_size,&bundle,c.elf,c.elf_size);
 gh_effect_create_forget_on_owner_destroy(effects);C(!effects->native&&w(emit+0x26000)==0xfffffffe);free(effects);free(esp);free(emit);free(core);
 C(!gh_hitarea_shapes_abort(init,&frontier)&&!gh_game_object_pool_release_001fdfa8(init,&frontier)&&!gh_collision_solid_units_shutdown(&solids,&frontier));gh_game_player_child_attachment_clear(&attachment);c.publication=NULL;C(!gh_game_player_child_release_prefix(&child)&&!gh_actor_model_release(&c.model)&&!gh_game_player_factory_release_prefix(&factory)&&!gh_collision_pairs_release(&pairs)&&!gh_game_world_light_release_presets(light));free(light);gh_game_objects_destroy(objects);gh_game_objects_destroy(global);C(!gh_game_room_pools_release(&pools)&&!gh_heap_used(c.heap)&&!gh_heap_destroy(c.heap));free(hair);free(pl);free(room);free(init);free(c.elf);gh_disc_close(d);puts("PASS actualISO effects2E eleven realtype0 constructors/partbinds/virtualInit/RNG/occupancy/typedrelease; effect21 ten real Init incl boundedunattachedidentity/retry/cleanup; OM324 actual3-row bank0/1C world transform");return 0;}
