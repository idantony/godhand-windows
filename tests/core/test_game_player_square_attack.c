#include "game_player_square_attack.h"
#include "game_player_square_input.h"
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
 unsigned triangle;
} bridge;
static gh_player_init_status begin_motion(void*ctx,const gh_player_motion_request*r,uint32_t*f)
{
 bridge *b=ctx;
 b->begins++;
 if(b->triangle)C(r->motion_offset==word(r->resource+0x110)&&
    r->sequence_candidate_offset==word(r->resource+0x118)&&!r->start_frame_bits&&r->blend_frames==5&&
    r->flags==0x40&&r->release_previous_sequence);
 else C(r->motion_offset==0x955a0&&r->sequence_candidate_offset==0x96520&&
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
 if(b->triangle){
  C(r->bank==0&&r->program==(b->sounds?243:203));
  ++b->sounds;*handle=17;return GH_ACTOR_MOTION_OK;
 }
 C(r->bank==0&&r->program==(b->sounds<2?203:24));
 b->sounds++;
 if(b->sounds==1){*frontier=0x2cba48;return GH_ACTOR_MOTION_BUSY;}
 *handle=17;
 return GH_ACTOR_MOTION_OK;
}
int main(int argc,char**argv){
 C(argc==2);gh_disc*d;C(!gh_disc_open(argv[1],&d));size_t rn,pn;uint8_t*room_bytes=load(d,"st1/r100.dat",&rn),*pl=load(d,"pl/pl00.dat",&pn);size_t en=(size_t)gh_disc_boot_elf_size(d);uint8_t*elf=malloc(en),*copy=malloc(pn);C(elf&&copy&&!gh_disc_read_boot_elf(d,0,elf,en));memcpy(copy,pl,pn);gh_disc_close(d);gh_bundle room,bundle;gh_bundle_entry md,texture;C(!gh_bundle_open(room_bytes,rn,&room)&&!gh_bundle_open(pl,pn,&bundle)&&!gh_bundle_find(&bundle,"MD",0,&md)&&!gh_bundle_find(&bundle,"TM3",0,&texture));
 int modes[]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};for(unsigned mode=0;mode<8;mode++){
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
  const unsigned triangle=mode>=4;
  backend.triangle=triangle;
  if(triangle){
   int selected;
   raw[0x622]=3;
   C(word(save+0x1d4)==29&&word(save+0x1f4)==1);
   memcpy(raw+0x15f4,&(uint32_t){0x20},4);
   const gh_player_idle_input_bindings idle_bindings={
    .frame={.player=&player,.work=&work,.init=init,.save=save_view,
       .current_player=&player.view,.auxiliary_bindings_known=1},
    .targets={.pools=&pools,.actors=&actors,.player=&player.view,.elf=elf,.elf_bytes=en}};
   gh_player_idle_input idle_input={0};
   gh_player_frame_status selected_status=gh_game_player_idle_input_00123938(&idle_input,&idle_bindings,&selected);
   if(selected_status)fprintf(stderr,"Triangle input status %d pc %x\n",selected_status,idle_input.frontier);
   C(!selected_status&&selected&&!idle_input.frontier&&raw[0x2f5]==0x0b&&raw[0x5e2]==1&&!raw[0x5e0]);
  }
  gh_player_square_attack call={0};
  backend.block=1;
  C(!fesetround(modes[mode%4])&&!feclearexcept(FE_ALL_EXCEPT)&&!feraiseexcept(FE_DIVBYZERO));
  C(gh_player_square_attack_begin_00126918(&call,&bindings)==GH_PLAYER_FRAME_UNSUPPORTED&&
    call.frontier==0x2a8578&&call.stage==1&&backend.begins==1&&!raw[0x5e0]);
  gh_player_frame_status e=gh_player_square_attack_begin_00126918(&call,&bindings);
  if(e)fprintf(stderr,"Begin %d frontier %X native %X detail %X\n",e,call.frontier,backend.motion.frontier,backend.motion.detail);
  C(!e&&call.stage==2&&raw[0x5e0]==1&&raw[0x434]==0x40&&word(raw+0x1668)==(triangle?12u:3u)&&
    word(raw+0x166c)==(triangle?12u:3u)&&word(raw+0x1670)==(triangle?0x12u:7u)&&
    word(raw+0x1674)==(triangle?0x12u:7u)&&word(raw+0x1680)==(triangle?0x000a000au:0x00640064u)&&
    word(raw+0x1660)==triangle&&call.skill==(triangle?29:0));
  C(!gh_player_square_attack_begin_00126918(&call,&bindings)&&backend.begins==2);
  fprintf(stderr,"clip skill%u motion%zx seq%zx frames%u\n",call.skill,call.motion.motion_offset,
     backend.motion.sequence_offset,backend.motion.motion.frames);
  C(backend.motion.motion.frames==(triangle?49u:19u)&&backend.motion.sequence_valid&&
    backend.motion.sequence_offset==(triangle?word(pl+0x118):0x96520u)&&!memcmp(raw+0x310,raw+0x320,12));
  unsigned strike_frames=0;int moved=0;
  for(unsigned i=0;i<(triangle?20u:10u);i++){
   float before=num(raw+0x430);
   gh_actor_motion_status step=gh_actor_motion_step_00157c18(&backend.motion,rate,GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
   if(step==GH_ACTOR_MOTION_BUSY){
    C(pc==0x2cba48&&random.auxiliary_word==0xb4f5&&backend.sounds==1);
    step=gh_actor_motion_step_00157c18(&backend.motion,rate,GH_STATIC_BINARY32_REFERENCE,&ended,&pc);
    C(random.auxiliary_word==0xb4f5&&backend.sounds==2);
   }
   if(step)fprintf(stderr,"Step %u frame %.3f status %d frontier %X\n",i,before,step,pc);
   C(!step&&!ended);
   if(!i)C(num(raw+0x330)==0&&num(raw+0x334)==0&&num(raw+0x338)==0);
   moved|=num(raw+0x330)!=0||num(raw+0x334)!=0||num(raw+0x338)!=0;
   unsigned active=raw[0x3ac]&3;
   strike_frames+=active!=0;
   if(triangle)C(active==(before>=13&&before<=15?1u:0u));
   else C(active==(before>=6&&before<=8?2u:0u));
  }
  C((triangle?(strike_frames==3&&backend.sounds==2):(strike_frames==2&&backend.sounds==3&&moved))&&fegetround()==modes[mode%4]&&fetestexcept(FE_ALL_EXCEPT)==FE_DIVBYZERO);
  C(!fesetround(FE_TONEAREST));
  C(!gh_actor_model_release(&model)&&!gh_scene_actors_reset_0012e800(&actors)&&
    !gh_scene_at_units_release(&at)&&!gh_game_room_pools_release(&pools)&&
    !gh_heap_used(heap)&&!gh_heap_destroy(heap));
  free(init);
 }
 C(!memcmp(pl,copy,pn));free(copy);free(elf);free(pl);free(room_bytes);
 puts("PASS actual PL00 neutral Triangle skill29 and Square skill0, original motion, SEQ strike windows and FEnv");
 return 0;
}
