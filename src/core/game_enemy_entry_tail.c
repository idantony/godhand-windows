#include "game_enemy_entry_tail.h"
#include "game_enemy_row2.h"
#include <string.h>
static uint32_t word(const uint8_t*p,size_t o){uint32_t v;memcpy(&v,p+o,4);return v;}
static void put(uint8_t*p,size_t o,uint32_t v){memcpy(p+o,&v,4);}
static gh_actor_model_status initialize_springs(gh_enemy_entry_tail *s)
{
 gh_enemy_entry *entry=s->entry;
 gh_actor_model *model=&entry->initialize.model.model;
 if(!s->spring_records.pointer){
  if(gh_heap_allocate(entry->b.room_heap,0x1b80,16,&s->spring_records)!=GH_MEM_OK){
   s->frontier=0x30f578;return GH_ACTOR_MODEL_MEMORY;
  }
  /* 27144C initializes six VF0 vectors in each of 44 records. */
  memset(s->spring_records.pointer,0,0x1b80);
  static const unsigned vectors[]={0x00,0x10,0x30,0x40,0x50,0x80};
  for(unsigned i=0;i<44;++i){
   uint8_t *record=(uint8_t *)s->spring_records.pointer+i*0xa0;
   for(unsigned j=0;j<sizeof(vectors)/sizeof(vectors[0]);++j)put(record,vectors[j]+12,0x3f800000);
  }
 }
 if(!s->spring_profile.loaded&&gh_event_spring_profile_from_elf(&s->spring_profile,
    (uint16_t)entry->request.dependency_kind,entry->b.boot_elf,entry->b.boot_elf_bytes)){
  s->frontier=0x2deaec;return GH_ACTOR_MODEL_UNSUPPORTED;
 }
 uint8_t *config=entry->object.view.bytes+0x17e0;
 memset(config,0,0x74); /* Original pointers are owned by the typed profile. */
 put(config,0,s->spring_profile.count);
 put(config,0x38,4);put(config,0x3c,0x3c75c28f);put(config,0x40,0x3f4ccccd);
 put(config,0x44,1);put(config,0x4c,0x3dcccccd);put(config,0x50,0x3f000000);
 put(config,0x5c,0x100);put(config,0x70,0x3f860a92);
 if(gh_event_spring_records_001038e0(&s->spring_profile,model,s->spring_records.pointer,
     0x1b80,entry->b.policy,&s->frontier))return GH_ACTOR_MODEL_UNSUPPORTED;
 s->springs_ready=1;
 return GH_ACTOR_MODEL_OK;
}

gh_actor_model_status gh_enemy_entry_tail_abort(gh_enemy_entry_tail *s)
{
 if(!s)return GH_ACTOR_MODEL_ARGUMENT;
 if(!s->spring_records.pointer)return GH_ACTOR_MODEL_OK;
 if(gh_heap_release(s->entry->b.room_heap,s->spring_records)!=GH_MEM_OK)return GH_ACTOR_MODEL_BUSY;
 s->spring_records=(gh_r207_allocation){0};s->springs_ready=0;
 return GH_ACTOR_MODEL_OK;
}

static gh_actor_model_status continue_entry(gh_enemy_entry_tail*s,gh_enemy_entry*e,gh_game_cockpit*c,int devil){
 if(!s||!e||e->stage!=8||!e->published||e->initialize.stage!=3||!e->initialize.model.model.ready||!e->b.init)return GH_ACTOR_MODEL_ARGUMENT;
 if(s->stage&&s->entry!=e)return GH_ACTOR_MODEL_BUSY;
 const int r103=gh_enemy204_is_r103_entry(&e->request,e->b.work->room);
 if(!devil&&!r103&&(e->request.object_kind!=0x202||(e->request.dependency_kind!=0x20a&&!gh_enemy202_is_room_supported_profile(&e->request,e->b.work->room)))){s->frontier=0x2022d0;return GH_ACTOR_MODEL_UNSUPPORTED;}
 if(!s->stage){s->entry=e;s->stage=1;}
 uint8_t*p=e->object.view.bytes;
 if(s->stage==1){int original=0;gh_actor_model_status status=gh_enemy_base_initialize_0028ebf0(&s->base,&e->object.view,&e->request,e->argument,c,&e->b.init->random,&original);if(status){s->frontier=s->base.frontier;return status;}if(original!=1)return GH_ACTOR_MODEL_ARGUMENT;s->stage=2;}
 if(s->stage==2){/* Actual150128(self,4), notplayer's twoextraflagwrites. */p[0x140]=4;
  /* Actual 207EE0 default has no inserts for these dependencies. */
  s->original_counter=word(e->b.init->enemies,0x55c);put(e->b.init->enemies,0x55c,s->original_counter+UINT32_C(1));put(p,0x17d0,s->original_counter);
  /* 200/202/203 keep the spawn-counter scale from 42CCE0. 20A and 20F overwrite it. */
  static const uint32_t scales[]={0x3f851eb8,0x3f88f5c3,0x3f87ae14,0x3f8ccccd,
      0x3f8a3d71,0x3f8ccccd,0x3f866666,0x3f8b851f};
  /* The post-TXE runtime dependency 227 selects original 202A78. */
  uint32_t scale=devil||word(p,0x564)==0x227?0x3f99999a:
      (e->request.dependency_kind==0x20a||e->request.dependency_kind==0x20f)?
       0x3f800000:scales[s->original_counter&7];
  if(r103)scale=e->request.entry_number<2?0x3f8ccccd:0x3f800000;
  put(p,0x110,scale);put(p,0x118,scale);put(p,0x114,scale);s->stage=3;
 }
 /* Actual271410 dependency20A is another switchdefault with noallocations.
  * Next actualinstruction begins HPlookup; wholevirtualInit remainspending. */
 if(r103&&e->request.entry_number<2&&!s->springs_ready){
  const gh_actor_model_status status=initialize_springs(s);
  if(status){e->frontier=s->frontier;return status;}
 }
 s->frontier=0x202b88;e->frontier=s->frontier;return GH_ACTOR_MODEL_UNSUPPORTED;
}

gh_actor_model_status gh_enemy_entry_continue_002022d0(gh_enemy_entry_tail *s,
 gh_enemy_entry *e,gh_game_cockpit *cockpit)
{return continue_entry(s,e,cockpit,0);}

gh_actor_model_status gh_enemy_devil_continue_002022d0(gh_enemy_entry_tail *s,
 gh_enemy_devil_entry *devil,gh_game_cockpit *cockpit)
{
 if(!devil||!devil->parent||devil->entry.request.object_kind!=0x220||
    devil->entry.request.dependency_kind!=0x220)return GH_ACTOR_MODEL_ARGUMENT;
 /* The wrapper carries the native counterpart of guest +628. */
 return continue_entry(s,&devil->entry,cockpit,1);
}
