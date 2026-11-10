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
gh_object_step_status gh_object_static_lookup(gh_game_object_step *,const gh_room_pools *,uint32_t,gh_scene_mesh_object **,size_t *,uint32_t *,uint32_t);
gh_object_step_status gh_object_static_move_one(gh_game_object_step *,gh_scene_mesh_object *,const gh_boot_work *,uint32_t *);
gh_object_step_status gh_game_object_step_bind_live(gh_game_object_step*s,const gh_object_move_bindings*b){
 if(!s||!s->constructed||s->move_stage||!b||!b->resolve||!b->invoke||!b->elf||b->elf_bytes<52)return GH_OBJSTEP_ARGUMENT;
 s->live_objects=*b;s->live_bound=1;return GH_OBJSTEP_OK;
}
static int descriptor(const gh_object_move_bindings*b,uint32_t address,uint32_t*out){
 const uint8_t*p=b->elf;size_t n=b->elf_bytes;if(n<52||memcmp(p,"\177ELF\1\1\1",7)||(p[18]|(unsigned)p[19]<<8)!=8)return 0;
 uint32_t ph=word(p+28);unsigned stride=p[42]|(unsigned)p[43]<<8,count=p[44]|(unsigned)p[45]<<8;
 if(stride<32||count>128||ph>n||(size_t)count*stride>n-ph)return 0;int found=0;
 for(unsigned i=0;i<count;i++){const uint8_t*q=p+ph+(size_t)i*stride;uint32_t base=word(q+8),off=word(q+4),size=word(q+16);
  if(word(q)==1&&address>=base&&(uint64_t)address+4<=(uint64_t)base+size){uint64_t at=(uint64_t)off+address-base;if(at>n||4>n-at||found++)return 0;*out=word(p+(size_t)at);}}
 return found==1;
}
static gh_object_step_status resolve(gh_game_object_step*s,const gh_room_pools*pools,uint32_t handle,gh_title_player_view*out,gh_scene_mesh_object**mesh,uint32_t*f,uint32_t pc){
 size_t index;uint32_t error=0;gh_object_step_status e=gh_object_static_lookup(s,pools,handle,mesh,&index,&error,pc);
 if(!e){*out=(gh_title_player_view){.bytes=(*mesh)->allocation.pointer,.size=0x2f4};if(f)*f=0;return GH_OBJSTEP_OK;}
 if(error!=pc||!s->live_bound)return stop(f,error?error:pc,e);
 *mesh=NULL;e=s->live_objects.resolve(s->live_objects.context,handle,out,&error);
 if(e)return stop(f,error?error:pc,e);if(!out->bytes||out->size<0x2f4)return stop(f,pc,GH_OBJSTEP_ARGUMENT);
 if(f)*f=0;return GH_OBJSTEP_OK;
}
gh_object_step_status gh_game_move_objects_0030f090(gh_game_object_step*s,const gh_room_pools*pools,const gh_boot_work*work,gh_object_profile_counter timer,void*ctx,uint32_t*f){
 if(f)*f=0;if(!s||!s->constructed||!s->init||!pools||!work||!timer)return GH_OBJSTEP_ARGUMENT;
 const gh_room_vector *objects,*scroll;
 if(gh_room_registry_view(pools,0x754c30,&objects)!=GH_POOL_OK||gh_room_registry_view(pools,0x754c38,&scroll)!=GH_POOL_OK)return stop(f,0x30f114,GH_OBJSTEP_POOLS);
 uint32_t flags=word((const uint8_t*)work+0x5d8);(void)scroll;
 if(!s->move_stage){
  s->profile_752cf8=timer(ctx);if(!(flags&0x08000000u))s->ratio10_bits=0x3f800000;
  s->move_count=objects->count;s->move_index=0;s->move_object_stage=0;s->move_native_phase=0;s->move_skip=(flags&0x20000u)!=0;s->move_pools=pools;s->move_work=work;s->move_stage=1;
 }else if(s->move_pools!=pools||s->move_work!=work||s->move_count!=objects->count)return stop(f,0x30f138,GH_OBJSTEP_POOLS);
 if(s->move_stage==1){
  if(!s->move_skip)for(;s->move_index<s->move_count;s->move_index++){
   gh_object_step_status e;
   if(!s->move_native_phase){
    uint32_t handle=word((const uint8_t*)objects->allocation.pointer+s->move_index*4);gh_title_player_view view={0};gh_scene_mesh_object*o=NULL;
    e=resolve(s,pools,handle,&view,&o,f,0x30f138);if(e)return e;
    if(word(view.bytes+0x250)&(0x8000u|0x80000000u))continue;
    gh_object_move_request request={.handle=handle,.actor=view};
    if(!o){uint32_t vtable=word(view.bytes+0x214),delta,entry,first;
     if(vtable>UINT32_MAX-0x58||!descriptor(&s->live_objects,vtable+0x50,&delta)||!descriptor(&s->live_objects,vtable+0x54,&entry)||!descriptor(&s->live_objects,entry,&first))return stop(f,0x30f15c,GH_OBJSTEP_UNSUPPORTED);
     uint16_t low=(uint16_t)delta;request.this_delta=low<32768?(int16_t)low:(int16_t)((int)low-65536);request.original_entry=entry;(void)first;
    }
    s->move_request=request;s->move_static_object=o;s->move_native_phase=1;
   }
   if(s->move_native_phase==1){
    if(s->move_static_object)e=gh_object_static_move_one(s,s->move_static_object,work,f);
    else{uint32_t pc=0;e=s->live_objects.invoke(s->live_objects.context,&s->move_request,work,&pc);if(e)return stop(f,pc?pc:s->move_request.original_entry,e);}
    if(e)return e;s->move_native_phase=2;
   }
   /* 30F16C reloads the registry slot AFTER Move, including a replacement
      object. A late host resolution failure must not invoke Move twice. */
   gh_title_player_view after={0};gh_scene_mesh_object*ignored=NULL;
   uint32_t handle=word((const uint8_t*)objects->allocation.pointer+s->move_index*4);
   e=resolve(s,pools,handle,&after,&ignored,f,0x30f16c);if(e)return e;
   put(after.bytes+0x2f0,word(after.bytes+0x2f0)&~3u);s->move_native_phase=0;s->move_static_object=NULL;
  }
  s->move_stage=2;
 }
 if(s->move_stage==2){if(!(word((const uint8_t*)work+0x5d8)&0x08000000u)){gh_object_step_status e=gh_game_scroll_counter_move_002c53c8(s);if(e)return e;}s->move_stage=3;}
 s->profile_752cf8-=timer(ctx);s->move_stage=0;
 /*2E0DA8 debug text is unreachable through actual2E0AD8 stripped gate.*/
 return GH_OBJSTEP_OK;
}

gh_object_step_status gh_game_objects_trans_0030f650(const gh_room_pools*pools,uint32_t*f){
 const gh_room_vector*objects;if(f)*f=0;if(!pools)return GH_OBJSTEP_ARGUMENT;
 if(gh_room_registry_view(pools,0x754c30,&objects)!=GH_POOL_OK)return stop(f,0x30f650,GH_OBJSTEP_POOLS);
 if(objects->count)return stop(f,0x30f69c,GH_OBJSTEP_UNSUPPORTED);
 return GH_OBJSTEP_OK;
}
