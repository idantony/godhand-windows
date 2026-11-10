#include "r100_init.h"
#include <string.h>
static uint16_t h(const uint8_t*p){return (uint16_t)(p[0]|(unsigned)p[1]<<8);}
static uint32_t w(const uint8_t*p){return (uint32_t)h(p)|(uint32_t)h(p+2)<<16;}
static uint64_t key(const char*p){uint64_t n=0;for(unsigned i=0;i<8&&p[i];i++)n|=(uint64_t)(int64_t)(signed char)p[i]<<(8*i);return n;}
static void ww(uint8_t*p,uint32_t v){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(v>>(i*8));}
static gh_r100_init_status fail(gh_r100_init*s,uint32_t pc,int relative,gh_r100_init_status e){s->frontier=pc;s->frontier_relative=(uint8_t)relative;return e;}
gh_r100_init_status gh_r100_scroll_layer_002db2b0(gh_room_pools*p,gh_scene_meshes*m,uint32_t layer,int enabled,uint32_t*f){
 if(f)*f=0;if(!p||!m)return GH_R100_INIT_ARGUMENT;const gh_room_vector*v;gh_room_pool_status e=gh_room_registry_view(p,0x754c38,&v);if(e)return GH_R100_INIT_ARGUMENT;
 for(uint32_t i=0;i<v->count;i++){uint32_t handle=w((uint8_t*)v->allocation.pointer+(size_t)i*4);void*obj=NULL;if(gh_room_object_lookup(p,handle,GH_ROOM_OBJECT_SCROLL_MESH,m,&obj)){if(f)*f=0x2db2b0;return GH_R100_INIT_UNSUPPORTED;}gh_scene_mesh_object*o=obj;if(!o->allocation.pointer||!o->constructor_complete){if(f)*f=0x2db2b0;return GH_R100_INIT_UNSUPPORTED;}}
 for(uint32_t i=0;i<v->count;i++){uint32_t handle=w((uint8_t*)v->allocation.pointer+(size_t)i*4);void*obj=NULL;(void)gh_room_object_lookup(p,handle,GH_ROOM_OBJECT_SCROLL_MESH,m,&obj);gh_scene_mesh_object*o=obj;uint8_t*b=o->allocation.pointer;if(b[0x2fd]==layer)ww(b+0x250,enabled?w(b+0x250)&~2u:w(b+0x250)|2u);}
 return GH_R100_INIT_OK;
}
static gh_r100_init_status search(gh_r100_init*s,const gh_r100_init_bindings*b,const char*name,gh_r100_om_view*out){
 *out=(gh_r100_om_view){0};const gh_room_vector*v;if(gh_room_registry_view(b->pools,0x754c58,&v))return fail(s,0x14fd98,0,GH_R100_INIT_ARGUMENT);
 for(uint32_t i=0;i<v->count;i++){uint32_t handle=w((uint8_t*)v->allocation.pointer+(size_t)i*4),f=0;gh_r100_om_view view={0};if(!b->object_view)return fail(s,0x14fd98,0,GH_R100_INIT_UNSUPPORTED);gh_r100_init_status e=b->object_view(b->context,handle,&view,&f);if(e)return fail(s,f?f:0x14fd98,0,e);if(!view.bytes||view.size<0x540||!view.identity)return fail(s,0x14fd98,0,GH_R100_INIT_BOUNDS);uint16_t id=h(view.bytes+0x2fe);uint64_t n=(uint64_t)w(view.bytes+0x538)|(uint64_t)w(view.bytes+0x53c)<<32;if(id>0x2ff&&id<0x500&&n==key(name)){*out=view;return GH_R100_INIT_OK;}}
 return GH_R100_INIT_OK;
}
static gh_r100_init_status task(gh_r100_init*s,const gh_r100_init_bindings*b,uint32_t entry){s->pending_script=entry;if(!b->task_exec)return fail(s,entry,1,GH_R100_INIT_UNSUPPORTED);uint32_t f=0;gh_r100_init_status e=b->task_exec(b->context,entry,-1,&f);if(e)return fail(s,f?f:entry,f?0:1,e);s->pending_script=0;return GH_R100_INIT_OK;}
gh_r100_init_status gh_r100_initialize_00000208(gh_r100_init*s,const gh_r100_init_bindings*b){
 if(!s||!b||!b->work||!b->room_state||!b->room_state->active||b->room_state->size<8||!b->at||!b->at->ready||!b->pools||!b->meshes||!b->default_heap)return GH_R100_INIT_ARGUMENT;
 if(s->ready)return GH_R100_INIT_OK;uint8_t*raw=(uint8_t*)b->work;gh_r100_init_status e;uint32_t f=0;
 if(!s->work.pointer){if(gh_heap_allocate(b->default_heap,12,16,&s->work))return fail(s,0x30ee08,0,GH_R100_INIT_MEMORY);s->heap=b->default_heap;}else if(s->heap!=b->default_heap)return GH_R100_INIT_ARGUMENT;
 while(s->stage<15){switch(s->stage){
 case 0:case 1:{uint32_t index=s->stage?3:0;gh_at_native_unit unit;int ignored;gh_scene_at_status a=gh_scene_at_get_jump_002c2ae0(b->at,index,&unit);if(a)return fail(s,0x2c2ae0,0,GH_R100_INIT_CALLEE);int alternate=(w(raw+0x594)&0x800)!=0;int enabled=s->stage?alternate:!alternate;a=gh_scene_at_set_enabled(b->at,unit,enabled,&ignored);if(a)return fail(s,b->at->frontier?b->at->frontier:(enabled?0x2c28f8:0x2c29a0),0,GH_R100_INIT_CALLEE);}break;
 case 2:e=gh_r100_scroll_layer_002db2b0(b->pools,b->meshes,1,(w(raw+0x594)&0x800)!=0,&f);if(e)return fail(s,f?f:0x2db2b0,0,e);break;
 case 3:if(!(w(b->room_state->active+4)&0x400000)){gh_at_native_unit unit;gh_scene_at_status a=gh_scene_at_get_jump_002c2ae0(b->at,0,&unit);if(a)return fail(s,0x2c2ae0,0,GH_R100_INIT_CALLEE);a=gh_scene_at_set_jump_info_002c2750(b->at,unit,0xff,0xa38,0,0);s->jump_callback_module=0x100;s->jump_callback_relative=0xa38;if(a)return fail(s,0x2c2750,0,GH_R100_INIT_CALLEE);}break;
 case 4:e=search(s,b,"rofF00",&s->roofs[0]);if(e)return e;break;
 case 5:e=search(s,b,"rofF01",&s->roofs[1]);if(e)return e;break;
 case 6:case 7:{unsigned i=s->stage-6;if(s->roofs[i].bytes){if(b->actor_tables){uint32_t changed;gh_scene_table_status a=gh_collision_scroll_set_enabled_001332d0(b->actor_tables,&s->collision,i?"H03_Jfl3":"H03_Jfl2",0,&changed);if(a)return fail(s,0x1331b8,0,GH_R100_INIT_CALLEE);}else{if(!b->collision_enable)return fail(s,0x1332d0,0,GH_R100_INIT_UNSUPPORTED);e=b->collision_enable(b->context,i?"H03_Jfl3":"H03_Jfl2",0,&f);if(e)return fail(s,f?f:0x1332d0,0,e);}}}break;
 case 8:if(s->roofs[0].bytes||s->roofs[1].bytes){e=task(s,b,0x2f30);if(e)return e;}break;
 case 9:case 10:case 11:case 12:case 13:{static const char*const names[]={"nosa000","nosa200","nosa100","nosa001","nosa300"};unsigned i=s->stage-9;gh_r100_om_view object;e=search(s,b,names[i],&object);if(e)return e;if(object.bytes){if(object.size<0x5f4)return fail(s,0x208,1,GH_R100_INIT_BOUNDS);ww(object.bytes+0x5ec,i+1);ww(object.bytes+0x5f0,1);}}break;
 case 14:((uint8_t*)s->work.pointer)[4]=0;s->known_work_bytes|=1u<<4;break;
 default:break;}s->stage++;}
 /* Original firsttask dispatch is deferred onlyuntil its REALnativebody is
  * supplied. Unresolved functions are never registered as empty callbacks. */
 if(s->stage==15){if(!(w(b->room_state->active+4)&0x1000000)&&!(w(raw+0x594)&0x800)){e=task(s,b,0x9b0);if(e)return e;s->stage=17;}else return fail(s,0xbc0,1,GH_R100_INIT_UNSUPPORTED);}
 if(s->stage==17)return fail(s,0x1fa6c0,0,GH_R100_INIT_UNSUPPORTED);
 return fail(s,0x208,1,GH_R100_INIT_UNSUPPORTED);
}
gh_r100_init_status gh_r100_init_release(gh_r100_init*s){if(!s)return GH_R100_INIT_ARGUMENT;if(gh_collision_scroll_mutation_release(&s->collision))return GH_R100_INIT_CALLEE;if(s->work.pointer&&(!s->heap||gh_heap_release(s->heap,s->work)))return GH_R100_INIT_MEMORY;memset(s,0,sizeof(*s));return GH_R100_INIT_OK;}
