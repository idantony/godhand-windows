#include "game_enemy_finalize.h"
#include "game_enemy_row2.h"
#include <string.h>
static uint32_t word(const uint8_t*p,size_t o){uint32_t v;memcpy(&v,p+o,4);return v;}
static void put(uint8_t*p,size_t o,uint32_t v){memcpy(p+o,&v,4);}
gh_actor_motion_status gh_enemy_finalize_00207d40(gh_enemy_finalize*s,gh_enemy_hitareas*hitareas,int*original){
 if(!s||!hitareas||hitareas->stage!=4||!hitareas->behavior||!original)return GH_ACTOR_MOTION_ARGUMENT;*original=0;
 if(s->stage&&s->hitareas!=hitareas)return GH_ACTOR_MOTION_BUSY;
 gh_enemy_entry*e=hitareas->behavior->timers->nodes->solids->shadow->pose->motion->hp->tail->entry;uint8_t*p=e->object.view.bytes;
 const int devil=e->request.object_kind==0x220&&e->request.dependency_kind==0x220&&
     e->b.work->room==0x100&&e->request.flags==0x400000&&e->request.entry_number==0xff;
 const int supported_exchange_room=e->b.work->room==0x105||
    (e->b.work->room==0x101&&e->request.entry_number==9&&e->request.appearance==4);
 const int exchange=supported_exchange_room&&word(p,0x564)==0x227&&
    gh_enemy202_is_room_supported_profile(&e->request,e->b.work->room)&&e->request.dependency_flags==1;
 if((!exchange&&word(p,0x564)!=e->request.dependency_kind)||!e->published||!e->object.view.alias_f0||
    (!devil&&(e->request.object_kind!=0x202||(e->request.dependency_kind!=0x20a&&
      !gh_enemy202_is_room_supported_profile(&e->request,e->b.work->room)))))return GH_ACTOR_MOTION_ARGUMENT;
 if(!s->stage){/* Actual26B9E8 only211 creates3B8;20Aguardreturnswithoutwrites. */
  s->hitareas=hitareas;put(p,0x560,0xffff);put(p,0x1728,0);put(p,0x17a4,0);put(p,0x17a8,0);p[0x17be]=0;put(p,0x624,0);
  if(exchange){
   uint32_t loot;
   if(gh_game_mt_next_0030f348(&e->b.init->random,&loot)){s->frontier=0x31cfe0;return GH_ACTOR_MOTION_ARGUMENT;}
   put(p,0x560,(loot&1)?0x3c3:0x9c1);
  }
  memcpy(p+0x1660,e->object.view.alias_f0,12);put(p,0x1670,word(p,0x104));p[0x617]=0;put(p,0x1730,0);p[0x17c2]=0;put(p,0x1750,0);put(p,0x1754,0);put(p,0x1758,0);
  put(p,0x16c8,0x41f00000);put(p,0x172c,0x43e10000);put(p,0x174c,0x43960000);s->stage=1;
 }
 if(s->stage==1){if(gh_game_mt_next_0030f348(&e->b.init->random,&s->final_random)){s->frontier=0x31cfe0;return GH_ACTOR_MOTION_ARGUMENT;}
  put(p,0x770,0);put(p,0x1718,0);p[0x17c0]=0;put(p,0x1764,0);put(p,0x1768,0);put(p,0x16e0,0);p[0x17bf]=(uint8_t)(s->final_random%3u);s->stage=2;s->complete=1;
 }
 s->frontier=0;e->frontier=0;*original=1;return GH_ACTOR_MOTION_OK;
}

static gh_actor_motion_status devil_finish_stop(gh_enemy_devil_finish *s,
 gh_enemy_entry *entry,uint32_t pc,gh_actor_motion_status status)
{
 entry->frontier=s->frontier=pc;
 return status;
}

gh_actor_motion_status gh_enemy_devil_finish_002055b8(gh_enemy_devil_finish *s,
 gh_enemy_devil_pose *pose,const gh_enemy_node_names *names,
 const gh_hitarea_shape_bindings *shapes,int *original)
{
 if(!s||!pose||!pose->init||!pose->init->owner||pose->solids.stage!=4||
    !pose->spawn_effect.complete||!names||!shapes||!original)return GH_ACTOR_MOTION_ARGUMENT;
 *original=0;
 if((s->pose&&s->pose!=pose)||(s->nodes.names&&s->nodes.names!=names)||
    (s->hitareas.stage&&(s->hitareas.b.pairs!=shapes->pairs||s->hitareas.b.math!=shapes->math||
      s->hitareas.b.policy!=shapes->policy)))return GH_ACTOR_MOTION_BUSY;
 if(!s->pose)s->pose=pose;
 gh_enemy_entry *entry=&pose->init->owner->entry;
 if(!s->stage){
  gh_player_nodes_status status=gh_enemy_node_cache_initialize_002055b8(
      &s->nodes,&pose->solids,names);
  if(status!=GH_PLAYER_NODES_UNSUPPORTED||s->nodes.stage!=2||s->nodes.frontier!=0x205ae4)
   return devil_finish_stop(s,entry,s->nodes.frontier?s->nodes.frontier:0x14b730,
       GH_ACTOR_MOTION_UNSUPPORTED);
  s->stage=1;
 }
 if(s->stage==1){
  gh_actor_motion_status status=gh_enemy_timers_initialize_00205ae4(
      &s->timers,&s->nodes,GH_EM_NUMERIC_PCSX2_CHOPZERO_REFERENCE);
  if(status!=GH_ACTOR_MOTION_UNSUPPORTED||s->timers.stage!=2||s->timers.frontier!=0x2064fc)
   return devil_finish_stop(s,entry,s->timers.frontier?s->timers.frontier:0x205ae4,status);
  s->stage=2;
 }
 if(s->stage==2){
  gh_actor_motion_status status=gh_enemy_behavior_initialize_002064fc(&s->behavior,&s->timers);
  if(status!=GH_ACTOR_MOTION_UNSUPPORTED||s->behavior.stage!=2||s->behavior.frontier!=0x1fe218)
   return devil_finish_stop(s,entry,s->behavior.frontier?s->behavior.frontier:0x2064fc,status);
  s->stage=3;
 }
 if(s->stage==3){
  gh_actor_motion_status status=gh_enemy_hitareas_initialize_00207018(
      &s->hitareas,&s->behavior,shapes);
  if(status!=GH_ACTOR_MOTION_UNSUPPORTED||s->hitareas.stage!=4||s->hitareas.frontier!=0x26b9e8)
   return devil_finish_stop(s,entry,s->hitareas.frontier?s->hitareas.frontier:0x207018,status);
  s->stage=4;
 }
 gh_actor_motion_status status=gh_enemy_finalize_00207d40(&s->final,&s->hitareas,original);
 return devil_finish_stop(s,entry,s->final.frontier,status);
}
