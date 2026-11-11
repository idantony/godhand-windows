/* Original first normal combo call, decoded against patched SLUS_215.03. */
#include "game_player_square_attack.h"
#include "game_actor_motion.h"
#include <string.h>
static uint32_t word(const void *p){uint32_t n;memcpy(&n,p,4);return n;}
static uint16_t half(const void *p){uint16_t n;memcpy(&n,p,2);return n;}
static void put(void *p,uint32_t n){memcpy(p,&n,4);}
static void sh(void *p,uint16_t n){memcpy(p,&n,2);}
static gh_player_frame_status stop(gh_player_square_attack *s,uint32_t pc,gh_player_frame_status e)
{
 s->frontier=pc;
 return e;
}

/* 1FB830/1FB6C8 read the same active CoreSave combo fields. */
static unsigned combo_limit(gh_core_save_view save,unsigned set)
{
 return set<6?word(save.bytes+0x1d0+set*0x24):0;
}
static uint32_t combo_skill(gh_core_save_view save,unsigned set,unsigned slot)
{
 return set<6&&slot<6&&slot<combo_limit(save,set)?
        word(save.bytes+0x1b0+set*0x24+slot*4):0;
}

static int elf_word(const gh_player_frame_bindings *b,uint32_t address,uint32_t *value)
{
 const uint8_t *elf=b->elf;
 size_t size=b->elf_bytes;
 if(!elf||size<52||memcmp(elf,"\177ELF\1\1\1",7)||half(elf+18)!=8)return 0;
 size_t offset=word(elf+28),stride=half(elf+42),count=half(elf+44);
 if(stride<32||offset>size||count>(size-offset)/stride)return 0;
 for(size_t i=0;i<count;i++){
  const uint8_t *ph=elf+offset+i*stride;
  uint32_t va=word(ph+8),files=word(ph+16),file=word(ph+4);
  if(word(ph)!=1||address<va||(uint64_t)address+4>(uint64_t)va+files)continue;
  if(file>size||files>size-file)return 0;
  *value=word(elf+file+(address-va));
  return 1;
 }
 return 0;
}

gh_player_frame_status gh_player_square_attack_begin_00126918(
 gh_player_square_attack *s,const gh_player_frame_bindings *b)
{
 if(!s||!b||!b->player||!b->player->view.bytes||
    b->player->view.size<GH_TITLE_PLAYER_BYTES||!b->work||!b->model||!b->rate)
  return GH_PLAYER_FRAME_ARGUMENT;
 uint8_t *p=b->player->view.bytes;
 if(b->model->root.bytes!=p)return GH_PLAYER_FRAME_ARGUMENT;
 if(s->bindings&&(s->bindings!=b||s->motion.player!=b->player||
    s->motion.work!=b->work||s->motion.model!=b->model||
    s->motion.resource!=b->resource||s->motion.resource_bytes!=b->resource_bytes||
    s->motion.rate!=b->rate))return GH_PLAYER_FRAME_BUSY;
 if(!s->stage){
  if(!b->save.bytes||b->save.size<GH_CORE_SAVE_BYTES)
   return stop(s,0x1fb830,GH_PLAYER_FRAME_UNSUPPORTED);
  unsigned set=half(p+0x5e2),slot=half(p+0x5e0);
  if(slot>=combo_limit(b->save,set))set=slot=0;
  if(set||slot||combo_skill(b->save,set,slot))
   return stop(s,0x1269e0,GH_PLAYER_FRAME_UNSUPPORTED);
  uint32_t dispatch,power;
  if(!elf_word(b,0x41cb80,&dispatch)||dispatch!=0x126a58||
     !elf_word(b,0x3bc7d0,&power))
   return stop(s,0x126a50,GH_PLAYER_FRAME_UNSUPPORTED);
  if(!b->resource||b->resource_bytes<0xf0)
   return stop(s,0x126a58,GH_PLAYER_FRAME_ARGUMENT);
  unsigned variant=half(p+0x5f2);
  size_t motion=word(b->resource+0xe0);
  size_t sequence=word(b->resource+(variant<3?0xe8:variant==3?0xec:0xe4));
  gh_motion_view view;
  if(gh_motion_open_mtb3(b->resource,b->resource_bytes,motion,&view)||
     sequence>b->resource_bytes||b->resource_bytes-sequence<0x20||
     memcmp(b->resource+sequence,"SEQ",4))
   return stop(s,0x1293c0,GH_PLAYER_FRAME_UNSUPPORTED);
  s->bindings=b;
  s->motion=(gh_player_motion_request){.player=b->player,.work=b->work,
   .model=b->model,.resource=b->resource,.resource_bytes=b->resource_bytes,
   .motion_offset=motion,.sequence_candidate_offset=sequence,.blend_frames=variant==3?5:7,
   .flags=0x40,.release_previous_sequence=1,.rate=b->rate};
  sh(p+0x5e2,(uint16_t)set);sh(p+0x5e0,(uint16_t)slot);
  sh(p+0x5e4,0);sh(p+0x5e6,0);
  p[0x1684]=0;
  /* Actual skill0 table row 3BC7D0 begins with power 3. Both
   * 10A408/10A420 write the same initial parameters here. */
  for(unsigned i=0;i<2;i++){
   put(p+0x1660+i*4,0);put(p+0x1668+i*4,power);
   put(p+0x1670+i*4,7);put(p+0x1678+i*4,0);sh(p+0x1680+i*2,100);
  }
  s->stage=1;
 }
 if(s->stage==1){
  if(!b->initialize_motion)return stop(s,0x2a8578,GH_PLAYER_FRAME_UNSUPPORTED);
  uint32_t pc=0;
  gh_player_init_status e=b->initialize_motion(b->motion_context,&s->motion,&pc);
  if(e)return stop(s,pc?pc:0x2a8578,e==GH_PLAYER_INIT_UNSUPPORTED?
                   GH_PLAYER_FRAME_UNSUPPORTED:GH_PLAYER_FRAME_CALLEE);
  sh(p+0x5e0,(uint16_t)(half(p+0x5e0)+1u));
  s->stage=2;
 }
 s->frontier=0;
 return GH_PLAYER_FRAME_OK;
}
