/* Recovered actual MoveObj/cScrArray prefix. MIT LICENSE.upstream;
 * eaae18d0dfab1f9a0df276ae5ca1581ad326faf3. ActualpatchedELF andstripped
 * debuggate evidence: analysis/game-object-step-evidence.json. */
#include "game_object_step.h"
#include <string.h>
_Static_assert(sizeof(((gh_game_init_state*)0)->side_counter)==0x10,"canonical754C38scalarspan");
static uint32_t word(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static void put(uint8_t*p,uint32_t n){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(n>>(8*i));}
static int32_t signed32(uint32_t n){return n<UINT32_C(0x80000000)?(int32_t)n:-(int32_t)(UINT32_MAX-n)-1;}
static gh_object_step_status stop(uint32_t*f,uint32_t pc,gh_object_step_status e){if(f)*f=pc;return e;}
gh_object_step_status gh_game_object_step_construct_002c53a8(gh_game_object_step*s,gh_game_init_state*init){
 if(!s||!init)return GH_OBJSTEP_ARGUMENT;s->init=init;put(init->side_counter,0);init->side_counter[4]=init->side_counter[5]=0;s->ratio10_bits=0x3f800000;s->constructed=1;return GH_OBJSTEP_OK;
}
gh_object_step_status gh_game_scroll_counter_move_002c53c8(gh_game_object_step*s){
 if(!s||!s->constructed||!s->init)return GH_OBJSTEP_ARGUMENT;
 uint8_t*p=s->init->side_counter;uint32_t count=word(p+8),progress=word(p+12);
 if(signed32(count)<=0)return GH_OBJSTEP_OK;
 if(p[4]){progress+=1;put(p+12,progress);if(signed32(progress)<signed32(count))return GH_OBJSTEP_OK;}
 else{progress-=1;put(p+12,progress);if(signed32(progress)>=0)return GH_OBJSTEP_OK;}
 put(p+12,0);put(p+8,0);return GH_OBJSTEP_OK;
}
gh_object_step_status gh_game_move_objects_0030f090(gh_game_object_step*s,const gh_room_pools*pools,const gh_boot_work*work,gh_object_profile_counter timer,void*ctx,uint32_t*f){
 if(f)*f=0;if(!s||!s->constructed||!s->init||!pools||!work||!timer)return GH_OBJSTEP_ARGUMENT;
 s->profile_752cf8=timer(ctx);uint32_t flags=word((const uint8_t*)work+0x5d8);
 if(!(flags&0x08000000u))s->ratio10_bits=0x3f800000;
 const gh_room_vector *objects,*scroll;
 if(gh_room_registry_view(pools,0x754c30,&objects)!=GH_POOL_OK||gh_room_registry_view(pools,0x754c38,&scroll)!=GH_POOL_OK)return stop(f,0x30f114,GH_OBJSTEP_POOLS);
 (void)scroll; /* identity iscanonical; count isNOT side_counter+8 */
 if(!(flags&0x20000u)&&objects->count)return stop(f,0x30f138,GH_OBJSTEP_UNSUPPORTED);
 if(!(flags&0x08000000u)){gh_object_step_status e=gh_game_scroll_counter_move_002c53c8(s);if(e)return e;}
 s->profile_752cf8-=timer(ctx); /* actualSUBU/SW modulo2^32 */
 /* Originaldebugcall isretainedsemantically:2E0DA8->2E0C80'sfirst2E0AD8
  * gate unconditionallyreturns1 (actualjrra/li1), soformat/renderunreachable.
  * This isverifiedstrippedcode, not a successful emptyloggercallback. */
 return GH_OBJSTEP_OK;
}
gh_object_step_status gh_game_objects_trans_0030f650(const gh_room_pools*pools,uint32_t*f){
 const gh_room_vector*objects;if(f)*f=0;if(!pools)return GH_OBJSTEP_ARGUMENT;
 if(gh_room_registry_view(pools,0x754c30,&objects)!=GH_POOL_OK)return stop(f,0x30f650,GH_OBJSTEP_POOLS);
 if(objects->count)return stop(f,0x30f69c,GH_OBJSTEP_UNSUPPORTED);
 return GH_OBJSTEP_OK;
}
