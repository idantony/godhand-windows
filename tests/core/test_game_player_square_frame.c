#include "game_player_square_frame.h"
#include "game_player_attack_hitarea.h"
#include "game_effect_release.h"
#include "game_player_idle_input.h"
#include "game_player_door_probe.h"
#include "game_actor_motion_call.h"
#include "game_actor_motion_step.h"
#include "game_actor_motion_ik.h"
#include "disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fenv.h>
#define C(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t word(const void*p){uint32_t n;memcpy(&n,p,4);return n;}
static float num(const void*p){float n;memcpy(&n,p,4);return n;}
static uint8_t*load(gh_disc*d,const char*name,size_t*n){size_t i;gh_disc_entry e;C(!gh_disc_find(d,name,&i)&&!gh_disc_entry_at(d,i,&e));uint8_t*p=malloc(*n=(size_t)e.size);C(p&&!gh_disc_read(d,i,0,p,*n));return p;}
typedef struct {
 gh_actor_motion motion;
 gh_actor_motion_call call;
 gh_actor_motion_bindings bindings;
 unsigned begins,sounds;
 int block;
} bridge;
static gh_player_init_status begin_motion(void*ctx,const gh_player_motion_request*r,uint32_t*f)
{
 bridge *b=ctx;
 b->begins++;
 C(r->motion_offset==0x955a0&&r->sequence_candidate_offset==0x96520&&
   !r->start_frame_bits&&r->blend_frames==7&&r->flags==0x40&&r->release_previous_sequence);
 if(b->block){b->block=0;*f=0x2a8578;return GH_PLAYER_INIT_UNSUPPORTED;}
 gh_actor_motion_status e=gh_actor_motion_call_open_parameters_002a8578(
  &b->call,&b->motion,&b->bindings,r->motion_offset,r->sequence_candidate_offset,
  r->start_frame_bits,r->blend_frames,r->flags,r->release_previous_sequence);
 if(!e)e=gh_actor_motion_call_step_002a8578(&b->call);
 *f=b->call.frontier;
 return e?GH_PLAYER_INIT_UNSUPPORTED:GH_PLAYER_INIT_OK;
}
static gh_actor_motion_status sound(void *context,const gh_motion_sound_request *r,uint32_t *handle,uint32_t *frontier)
{
 bridge *b=context;
 C(r->actor==&b->bindings.model->root&&r->model==b->bindings.model&&!r->part);
 C(r->bank==0&&r->program==(b->sounds<2?203:24));
 b->sounds++;
 if(b->sounds==1){*frontier=0x2cba48;return GH_ACTOR_MOTION_BUSY;}
 *handle=17;
 return GH_ACTOR_MOTION_OK;
}
static gh_player_frame_status advance_motion(void *context,const gh_player_motion_request *r,
 int *ended,uint32_t *frontier)
{
 bridge *b=context;
 gh_actor_motion_status e=gh_actor_motion_step_00157c18(
  &b->motion,*r->rate,GH_STATIC_BINARY32_REFERENCE,ended,frontier);
 return e?GH_PLAYER_FRAME_UNSUPPORTED:GH_PLAYER_FRAME_OK;
}
static gh_player_frame_status strike(void *context,const gh_title_player_view *player,
 gh_hitarea_object give[5],uint32_t *frontier)
{
 (void)context;
 return gh_player_attack_hitarea_0010a438(player,give,frontier)?
        GH_PLAYER_FRAME_UNSUPPORTED:GH_PLAYER_FRAME_OK;
}
int main(int argc,char**argv){
 C(argc==2);gh_disc*d;C(!gh_disc_open(argv[1],&d));size_t rn,pn,cn;uint8_t *core=load(d,"etc/core.dat",&cn);uint8_t*room_bytes=load(d,"st1/r100.dat",&rn),*pl=load(d,"pl/pl00.dat",&pn);size_t en=(size_t)gh_disc_boot_elf_size(d);uint8_t*elf=malloc(en),*copy=malloc(pn);C(elf&&copy&&!gh_disc_read_boot_elf(d,0,elf,en));memcpy(copy,pl,pn);gh_disc_close(d);gh_bundle room,bundle;gh_bundle_entry md,texture;C(!gh_bundle_open(room_bytes,rn,&room)&&!gh_bundle_open(pl,pn,&bundle)&&!gh_bundle_find(&bundle,"MD",0,&md)&&!gh_bundle_find(&bundle,"TM3",0,&texture));
 int modes[]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};for(unsigned mode=0;mode<4;mode++){
  gh_heap*heap;C(!gh_heap_create(16*1024*1024,&heap));gh_room_pools pools={0};C(!gh_game_room_pools_initialize_002a5d80(&pools,&room,heap));gh_scene_at_manager at={0};C(!gh_scene_at_initialize_units_002c0f90(&at,&room,heap,0x100,1));gh_game_scene_tables actors={0};C(!gh_scene_actors_0012e670(&actors,&room));
  gh_game_init_state*init=malloc(sizeof(*init));C(init);gh_game_init_state_initialize(init);gh_boot_work work={0};work.room=0x100;uint8_t raw[GH_TITLE_PLAYER_BYTES]={0};gh_game_title_player player={0};C(!gh_title_player_construct_00107e40(raw,sizeof(raw),&player.view));gh_actor_model model={0};int original;C(gh_actor_model_initialize_00149738(&model,&player.view,&work,&pools,&md,&texture,0,0,GH_STATIC_BINARY32_REFERENCE,&original)==GH_ACTOR_MODEL_UNSUPPORTED&&model.parts_ready&&model.part_count==32);
  /* This focused consumer needs canonical32skeletalparts only. Whole model
   * packets/PlayerInit are not fabricated as complete for this test. */
  raw[0x570]=raw[0x571]=255;gh_motion_ik_globals ik={0};C(!gh_motion_ik_globals_from_elf(&ik,elf,en));bridge backend={0};gh_frame_random random={0};C(!gh_frame_seed_0031ceb8(&random,&init->random,GH_FRAME_CHOP_REFERENCE));backend.bindings=(gh_actor_motion_bindings){.model=&model,.resource=pl,.resource_bytes=pn,.work=&work,.ik_globals=&ik,.sequence_random=&random,.sound=sound,.sound_context=&backend};size_t idle=word(pl+0x28);C(!gh_actor_motion_begin_002a8578(&backend.motion,&backend.bindings,idle,idle,0));uint32_t pc;int ended;C(!gh_actor_motion_step_00157c18(&backend.motion,1,GH_STATIC_BINARY32_REFERENCE,&ended,&pc)&&!gh_actor_model_update_hierarchy_00149eb8(&model,GH_STATIC_BINARY32_REFERENCE));
  uint8_t save[GH_CORE_SAVE_BYTES]={0};
  gh_core_save_view save_view={save,sizeof(save)};
  C(!gh_core_save_system_init(save_view)&&!gh_core_save_game_init(save_view,&work));
  C(word(save+0x1b0)==0&&word(save+0x1d0)>0);
  const float rate=1;
  const gh_player_frame_bindings bindings={.player=&player,.work=&work,.init=init,
   .current_player=&player.view,.elf=elf,.elf_bytes=en,.model=&model,.resource=pl,
   .resource_bytes=pn,.rate=&rate,.initialize_motion=begin_motion,.motion_context=&backend,
   .save=save_view,.numeric_policy=GH_CAMERA_NATIVE_BINARY32_REFERENCE};
  gh_game_effects *effects=calloc(1,sizeof(*effects));
  uint8_t *esp=calloc(1,0x100180),*emitters=calloc(1,0x26060);
  C(effects&&esp&&emitters&&!gh_game_effects_initialize_00305a68(
    effects,esp,0x100180,emitters,0x26060,core,cn,&room));
  gh_effect_release_bindings effect={.effects=effects,.esp=esp,.esp_bytes=0x100180,
   .emitters=emitters,.emitter_bytes=0x26060};
  gh_hitarea_object give[5];
  gh_collision_pairs pairs={0};
  gh_game_math_globals math={0};
  C(!gh_game_math_globals_from_elf(&math,elf,en)&&!gh_game_math_static_initialize_003079c8(&math,&work,1,0xffff));
  C(!gh_collision_pairs_construct_002b8ae0(&pairs,heap)&&!gh_collision_pairs_initialize_002b8b50(&pairs));
  const gh_hitarea_shape_bindings shapes={&pairs,&math,GH_STATIC_BINARY32_REFERENCE};
  const uint32_t heights[]={0x3f800000,0x3f000000,0x3f000000,0x40a00000};
  const uint32_t radii[]={0x3f800000,0x3e4ccccd,0x3e4ccccd,0x3e99999a};
  const uint32_t offsets[5][3]={{0,0x3f000000,0},{0,0x3fa66666,0x3e4ccccd},
    {0,0x3f4ccccd,0x3e4ccccd},{0,0x3f800000,0xbe4ccccd},{0,0x3fa66666,0}};
  const uint32_t rotations[5][3]={{0},{0x3fc90fdb,0,0},{0x3fc90fdb,0,0},{0xbfc90fdb,0,0},{0}};
  const uint32_t box_size[]={0x3f800000,0x3f000000,0x3f800000};
  C(!gh_hitarea_objects_construct_001fde00(init,&pc)&&!gh_game_object_pool_reset_001fdea8(init));
  for(unsigned i=0;i<5;i++){
   int present=0,index=-1;
   C(!gh_hitarea_object_create_001fe0e8(init,&player.view,0,&give[i],&present,&pc)&&present);
   if(i<4)C(!gh_hitarea_add_cylinder_001fdc00(&give[i],&shapes,raw+0x80,
        offsets[i],rotations[i],heights[i],radii[i],&index,&pc));
   else C(!gh_hitarea_add_box_001fd9d8(&give[i],&shapes,raw+0x80,
        offsets[i],rotations[i],box_size,&index,&pc));
   C(index==(int)i&&!gh_hitarea_object_set_active_001fd668(&give[i],0,&pc));
  }
  gh_player_frame_bindings live=bindings;
  live.give_objects=give;live.give_binding_known=1;live.give_present_mask=31;
  live.auxiliary_bindings_known=1;live.advance_motion=advance_motion;live.effects=&effect;
  gh_player_target selected={0};
  const gh_player_target_bindings targets={.pools=&pools,.actors=&actors,.player=&player.view,.elf=elf,.elf_bytes=en};
  gh_player_square_frame_bindings attack={.frame=&live,.target_bindings=&targets,.target_640=&selected};
  gh_player_square_frame call={0};
  raw[0x2f5]=0xb;
  backend.block=1;
  C(!fesetround(modes[mode])&&!feclearexcept(FE_ALL_EXCEPT)&&!feraiseexcept(FE_DIVBYZERO));
  C(gh_game_player_square_frame_0010d3a8(&call,&attack)==GH_PLAYER_FRAME_UNSUPPORTED&&
    call.frontier==0x2a8578&&call.stage==4&&backend.begins==1&&!raw[0x5e0]);
  gh_player_frame_status e=gh_game_player_square_frame_0010d3a8(&call,&attack);
  if(e)fprintf(stderr,"initial frame stage %u frontier %X\n",call.stage,call.frontier);
  C(!e&&raw[0x2f6]==1&&raw[0x5e0]==1&&num(raw+0x430)>0&&word(raw+0x15f4)==0x500);
  unsigned complete_frames=1;
  for(unsigned i=0;i<12;i++){
   C(!gh_player_square_frame_reset(&call));
   e=gh_game_player_square_frame_0010d3a8(&call,&attack);
   if(e&&call.frontier==0x2cba48)e=gh_game_player_square_frame_0010d3a8(&call,&attack);
   if(e)break;
   complete_frames++;
  }
  if(call.frontier!=0x10a438)fprintf(stderr,"strike stage %u frontier %X after %u\n",call.stage,call.frontier,complete_frames);
  C(e==GH_PLAYER_FRAME_UNSUPPORTED&&call.frontier==0x10a438&&call.stage==14&&
    complete_frames>=4&&backend.begins==2&&backend.sounds==3&&(raw[0x3ac]&3)==2);
  uint32_t time=word(raw+0x430),turn=word(raw+0x568);
  C(gh_game_player_square_frame_0010d3a8(&call,&attack)==GH_PLAYER_FRAME_UNSUPPORTED&&
    call.frontier==0x10a438&&word(raw+0x430)==time&&word(raw+0x568)==turn);
  attack.hit_area_0010a438=strike;
  C(!gh_game_player_square_frame_0010d3a8(&call,&attack)&&call.stage==17&&word(raw+0x430)==time);
  const uint8_t *node;
  gh_collision_pair_shape box;
  C(!gh_hitarea_node_at(&give[4],0,&node,&box)&&(word(box.record)&2)&&
    word(give[4].bytes+0x4c)==3&&give[4].bytes[0x46]==7&&raw[0x5e6]==1);
  unsigned active=1;
  for(unsigned i=0;i<24&&raw[0x2f5];i++){
   for(unsigned k=0;k<5;k++)C(!gh_hitarea_object_set_active_001fd668(&give[k],0,&pc));
   C(!gh_player_square_frame_reset(&call));
   e=gh_game_player_square_frame_0010d3a8(&call,&attack);
   if(e)fprintf(stderr,"retire stage %u frontier %X\n",call.stage,call.frontier);
   C(!e);
   C((word(box.record)&2)==((raw[0x3ac]&3)?2u:0u));
   active+=(raw[0x3ac]&3)!=0;
  }
  C(!raw[0x2f5]&&!raw[0x2f6]&&call.ended&&active==2&&raw[0x5e4]==1&&
    !raw[0x5e6]&&!word(raw+0x1668)&&!word(raw+0x166c)&&!(word(raw+0x15f4)&2));
  C(fegetround()==modes[mode]&&fetestexcept(FE_ALL_EXCEPT)==FE_DIVBYZERO);
  for(unsigned i=0;i<5;i++)C(!gh_hitarea_shapes_abort_object(&give[i],&pc)&&
                             !gh_hitarea_object_release_empty_001fe278(&give[i],&pc));
  C(!gh_collision_pairs_release(&pairs));
  free(effects);free(esp);free(emitters);
  C(!fesetround(FE_TONEAREST));
  C(!gh_actor_model_release(&model)&&!gh_scene_actors_reset_0012e800(&actors)&&
    !gh_scene_at_units_release(&at)&&!gh_game_room_pools_release(&pools)&&
    !gh_heap_used(heap)&&!gh_heap_destroy(heap));
  free(init);
 }
 C(!memcmp(pl,copy,pn));free(copy);free(elf);free(pl);free(room_bytes);free(core);
 puts("PASS actual PL00 first Square 10D3A8 ordinary state, native Give4 strike, power3, and clip retirement without camera reset");
 return 0;
}
