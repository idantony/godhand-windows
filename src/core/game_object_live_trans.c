#include "game_object_live_trans.h"
#include <string.h>
static uint32_t word(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
static void put(uint8_t*p,uint32_t n){memcpy(p,&n,4);}
static uint16_t half(const uint8_t*p){uint16_t n;memcpy(&n,p,2);return n;}
static gh_object_step_status stop(uint32_t*f,uint32_t pc,gh_object_step_status e){if(f)*f=pc;return e;}
static const gh_room_object_binding*binding(const gh_room_pools*p,uint32_t handle){
 if(!p||!p->initialized||!p->objects||p->object_slots>p->object_capacity)return NULL;const gh_room_object_binding*found=NULL;
 for(size_t i=0;i<p->object_slots;i++)if(p->objects[i].handle==handle&&p->objects[i].object){if(found)return NULL;found=p->objects+i;}return found;
}
int gh_object_live_trans_handle(const gh_room_pools*p,uint32_t handle){const gh_room_object_binding*b=binding(p,handle);return b&&b->kind!=GH_ROOM_OBJECT_SCROLL_MESH;}
static int elfword(const gh_object_move_bindings*b,uint32_t pc,uint32_t*out){
 const uint8_t*p=b->elf;size_t n=b->elf_bytes;if(!p||n<52||memcmp(p,"\177ELF\1\1\1",7)||half(p+18)!=8)return 0;
 uint32_t ph=word(p+28);unsigned stride=half(p+42),count=half(p+44),found=0;if(stride<32||count>128||ph>n||(size_t)count*stride>n-ph)return 0;
 for(unsigned i=0;i<count;i++){const uint8_t*q=p+ph+(size_t)i*stride;uint32_t off=word(q+4),va=word(q+8),size=word(q+16);if(word(q)!=1)continue;if(off>n||size>n-off)return 0;
  if(va<=pc&&(uint64_t)pc+4<=(uint64_t)va+size){if(found++)return 0;*out=word(p+(size_t)off+(size_t)(pc-va));}}
 return found==1;
}
static gh_object_step_status descriptor(gh_game_object_step*s,unsigned offset,uint32_t*f){
 gh_object_trans_request*r=&s->trans_live_request;uint32_t vt=word(r->actor.bytes+0x214),delta,entry,first;
 if(vt>UINT32_MAX-offset-8||!elfword(&s->live_objects,vt+offset,&delta)||!elfword(&s->live_objects,vt+offset+4,&entry)||!elfword(&s->live_objects,entry,&first))return stop(f,offset==8?0x30f7a0:offset==24?0x30f864:0x30f848,GH_OBJSTEP_UNSUPPORTED);
 uint16_t low=(uint16_t)delta;r->this_delta=low<32768?(int16_t)low:(int16_t)((int)low-65536);r->original_entry=entry;r->original_vtable=vt;(void)first;return GH_OBJSTEP_OK;
}
static gh_object_step_status detach(const gh_room_pools*borrowed,uint32_t handle,int scroll,uint32_t*f){
 gh_room_pools*p=(gh_room_pools*)borrowed;unsigned vectors[2]={scroll?1u:3u,2};size_t found[2]={0};unsigned used=scroll?2:1;
 for(unsigned k=0;k<used;k++){gh_room_vector*v=p->vectors+vectors[k];if(v->count>v->capacity||(v->count&&!v->allocation.pointer))return stop(f,0x312db8,GH_OBJSTEP_POOLS);found[k]=v->count;unsigned matches=0;
  for(size_t i=0;i<v->count;i++)if(word((const uint8_t*)v->allocation.pointer+i*4)==handle){found[k]=i;matches++;}if(matches>1||(!k&&!matches))return stop(f,0x312db8,GH_OBJSTEP_POOLS);
 }
 for(unsigned k=0;k<used;k++){gh_room_vector*v=p->vectors+vectors[k];if(found[k]<v->count){uint8_t*q=v->allocation.pointer;memmove(q+found[k]*4,q+(found[k]+1)*4,((size_t)v->count-found[k]-1)*4);v->count--;}}
 return GH_OBJSTEP_OK;
}
gh_object_step_status gh_game_object_step_bind_trans(gh_game_object_step*s,const gh_object_trans_bindings*b){if(!s||!s->constructed||!b||s->trans_stage||s->trans_live_phase)return GH_OBJSTEP_ARGUMENT;s->trans_live=*b;return GH_OBJSTEP_OK;}
gh_object_step_status gh_object_live_trans_one_0030f650(gh_game_object_step*s,const gh_room_pools*pools,const gh_boot_work*work,uint32_t handle,int*erased,uint32_t*f){
 if(f)*f=0;if(!s||!pools||!work||!erased)return GH_OBJSTEP_ARGUMENT;*erased=0;gh_object_step_status e;
 if(!s->trans_live_phase){
  if(!s->live_bound||!s->live_objects.resolve)return stop(f,0x30f69c,GH_OBJSTEP_UNSUPPORTED);const gh_room_object_binding*b=binding(pools,handle);if(!b||b->kind==GH_ROOM_OBJECT_SCROLL_MESH)return stop(f,0x30f69c,GH_OBJSTEP_POOLS);
  gh_title_player_view actor={0};uint32_t pc=0;e=s->live_objects.resolve(s->live_objects.context,handle,&actor,&pc);if(e)return stop(f,pc?pc:0x30f69c,e);if(!actor.bytes||actor.size<0x300)return stop(f,0x30f69c,GH_OBJSTEP_ARGUMENT);
  uint32_t flags=word(actor.bytes+0x250);if(flags&0x80000000u){
   if(!(flags&0x20000000u)){put(actor.bytes+0x250,flags|((flags&0x40000000u)?0x20000000u:0x40000000u));return GH_OBJSTEP_OK;}
  }else if(flags&0x8000u)return GH_OBJSTEP_OK;
  s->trans_live_request=(gh_object_trans_request){.handle=handle,.actor=actor};s->trans_live_handle=handle;s->trans_live_owner=b->owner;
  if(flags&0x80000000u){s->trans_live_request.kind=GH_OBJECT_TRANS_DESTROY;s->trans_live_request.argument=3;s->trans_live_phase=1;}
  else{put(actor.bytes+0x250,flags|0x200000u);s->trans_live_request.kind=(flags&0x80000u)?GH_OBJECT_TRANS_ALTERNATE:GH_OBJECT_TRANS_NORMAL;s->trans_live_phase=2;}
 }else if(s->trans_live_handle!=handle)return stop(f,0x30f69c,GH_OBJSTEP_POOLS);
 if(s->trans_live_phase==1){e=detach(pools,handle,half(s->trans_live_request.actor.bytes+0x2fe)==0,f);if(e)return e;s->trans_live_phase=3;}
 if(s->trans_live_phase==2){
  if(s->trans_live_request.kind==GH_OBJECT_TRANS_NORMAL){const gh_scene_at_manager*at=s->trans_live.scene_at?s->trans_live.scene_at:s->static_objects.scene_at;if(!at)return stop(f,0x2c3190,GH_OBJSTEP_UNSUPPORTED);
   if(at->field68){if(!s->trans_live.layer)return stop(f,0x2c3190,GH_OBJSTEP_UNSUPPORTED);int visible=0;uint32_t pc=0;e=s->trans_live.layer(s->trans_live.context,at,s->trans_live_request.actor.bytes[0x2fd],&visible,&pc);if(e)return stop(f,pc?pc:0x2c3190,e);if(!visible)goto done;}
  }s->trans_live_phase=3;
 }
 if(s->trans_live_phase==3){unsigned offset=s->trans_live_request.kind==GH_OBJECT_TRANS_DESTROY?8:s->trans_live_request.kind==GH_OBJECT_TRANS_ALTERNATE?24:16;e=descriptor(s,offset,f);if(e)return e;s->trans_live_phase=4;}
 if(s->trans_live_phase==4){
  gh_object_trans_request*r=&s->trans_live_request;if(!s->trans_live.invoke)return stop(f,r->original_entry,GH_OBJSTEP_UNSUPPORTED);uint32_t pc=0;e=s->trans_live.invoke(s->trans_live.context,r,work,&pc);if(e)return stop(f,pc?pc:r->original_entry,e);s->trans_live_phase=5;
 }
 if(s->trans_live_phase==5&&s->trans_live_request.kind==GH_OBJECT_TRANS_DESTROY){
  if(gh_room_object_unregister((gh_room_pools*)pools,handle,s->trans_live_owner)!=GH_POOL_OK)return stop(f,0x30f7d0,GH_OBJSTEP_POOLS);*erased=1;
 }
done:s->trans_live_phase=0;s->trans_live_handle=0;s->trans_live_owner=NULL;memset(&s->trans_live_request,0,sizeof(s->trans_live_request));return GH_OBJSTEP_OK;
}
