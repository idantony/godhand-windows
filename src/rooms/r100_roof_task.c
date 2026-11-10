#include "r100_roof_task.h"
#include <string.h>
static uint32_t rd(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
static unsigned half(const uint8_t*p){return p[0]|(unsigned)p[1]<<8;}
static uint64_t key(const char*p){uint64_t n=0;for(unsigned i=0;i<8&&p[i];i++)n|=(uint64_t)(int64_t)(int8_t)p[i]<<(i*8);return n;}
static gh_r100_script_status fail(gh_r100_roof_task*s,uint32_t pc,gh_r100_script_status e){s->frontier=pc;s->status=e;return e;}
gh_r100_script_status gh_r100_roof_is_broken_002c5168(const gh_r100_roof_bindings*b,const char*name,int*out,uint32_t*f){
 if(!b||!name||!out||!f||!b->pools||!b->room||!b->room->active||b->room->size<0x1610)return GH_R100_SCRIPT_ARGUMENT;*f=0;const gh_room_vector*v;if(gh_room_registry_view(b->pools,0x754c58,&v)){*f=0x14fd98;return GH_R100_SCRIPT_BOUNDS;}
 uint64_t wanted=key(name);gh_r100_om_view found={0};
 for(uint32_t i=0;i<v->count;i++){if(!b->object_view){*f=0x14fd98;return GH_R100_SCRIPT_UNSUPPORTED;}gh_r100_om_view view={0};gh_r100_init_status e=b->object_view(b->context,rd((const uint8_t*)v->allocation.pointer+(size_t)i*4),&view,f);if(e){if(!*f)*f=0x14fd98;return GH_R100_SCRIPT_CALLEE;}if(!view.bytes||view.size<0x540||!view.identity){*f=0x14fd98;return GH_R100_SCRIPT_BOUNDS;}unsigned type=half(view.bytes+0x2fe);uint64_t k=(uint64_t)rd(view.bytes+0x538)|(uint64_t)rd(view.bytes+0x53c)<<32;if(type>=0x300&&type<0x500&&k==wanted){found=view;break;}}
 if(!found.bytes){*out=1;return GH_R100_SCRIPT_OK;}if(found.size<0x5b4){*f=0x2c5110;return GH_R100_SCRIPT_BOUNDS;}
 uint64_t object_key=(uint64_t)rd(found.bytes+0x538)|(uint64_t)rd(found.bytes+0x53c)<<32;
 if(object_key){for(unsigned i=0;i<256;i++){const uint8_t*p=b->room->active+0x610+(size_t)i*16;uint64_t stored=(uint64_t)rd(p)|(uint64_t)rd(p+4)<<32;if(stored==object_key){if(p[8]==1){*out=1;return GH_R100_SCRIPT_OK;}break;}}}
 *out=(rd(found.bytes+0x5b0)&2)!=0;return GH_R100_SCRIPT_OK;
}
gh_r100_script_status gh_r100_roof_bind(gh_r100_roof_task*s,const gh_r100_roof_bindings*b){
 if(!s||s->bound||!b||!b->room_init||!b->room_init->work.pointer||!b->room||!b->room->active||b->room->size<0x1610||!b->pools||!b->pools->initialized||!b->actors||!b->actors->actors_ready||!b->actors->akg.bytes||!b->threads||gh_task_threads_manager(b->threads)!=b->scenario.manager||!b->scenario.bytes||b->scenario.size<0x114)return GH_R100_SCRIPT_ARGUMENT;s->b=*b;s->bound=1;s->slot=-1;return GH_R100_SCRIPT_OK;
}
static gh_r100_script_status sleep1(gh_r100_roof_task*s){s->sleep_calls++;return gh_task_threads_sleep(s->b.threads,1)?fail(s,0x2d5aa0,GH_R100_SCRIPT_THREAD):GH_R100_SCRIPT_OK;}
static gh_r100_script_status advance(gh_r100_roof_task*s){
 static const char*const objects[]={"rofF00","rofF01"},*const collisions[]={"H03_Jfl2","H03_Jfl3"};s->frontier=0;s->status=GH_R100_SCRIPT_OK;
 for(;;){while(s->scan_index<2){unsigned i=s->scan_index;if(!s->pending_enable){int broken=0;uint32_t f=0;gh_r100_script_status e=gh_r100_roof_is_broken_002c5168(&s->b,objects[i],&broken,&f);if(e)return fail(s,f?f:0x2c5168,e);
   if(broken==1&&!s->enabled[i]){s->enabled[i]=1;s->pending_enable=1;}else{s->scan_index++;continue;}}
   uint32_t changed=0;gh_scene_table_status e=gh_collision_scroll_set_enabled_001332d0(s->b.actors,&s->b.room_init->collision,collisions[i],1,&changed);if(e){s->detail=e;return fail(s,0x1332d0,GH_R100_SCRIPT_CALLEE);}s->changed[i]=changed;s->pending_enable=0;s->scan_index++;
  }
  if(s->enabled[0]&&s->enabled[1]){s->finished=1;return GH_R100_SCRIPT_OK;}s->scan_index=0;gh_r100_script_status e=sleep1(s);if(e)return e;
 }
}
static void roof(void*ctx,int32_t arg){gh_r100_roof_task*s=ctx;(void)arg;s->entered=1;while(!s->finished){s->status=advance(s);if(s->status!=GH_R100_SCRIPT_OK&&sleep1(s))return;}}
gh_r100_script_status gh_r100_roof_register_00002f30(gh_r100_roof_task*s,int32_t request,int32_t*slot){
 if(!s||!s->bound||!slot)return GH_R100_SCRIPT_ARGUMENT;if(s->registered){*slot=s->slot;return GH_R100_SCRIPT_OK;}uint32_t f=0;gh_scenario_exec_status e=gh_scenario_exec_002c3890(s->b.scenario,s->b.threads,request,roof,s,0,slot,&f);if(e)return fail(s,f?f:0x2c3890,GH_R100_SCRIPT_THREAD);s->slot=*slot;s->registered=1;s->status=GH_R100_SCRIPT_OK;s->frontier=0;return GH_R100_SCRIPT_OK;
}
