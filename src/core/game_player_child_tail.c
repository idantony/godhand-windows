/* Readable original100378 suffix, actual patchedELF internal100420. */
#include "game_player_child_tail.h"
#include <string.h>
static uint32_t u32(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static unsigned u16(const uint8_t*p){return p[0]|(unsigned)p[1]<<8;}
static void put(uint8_t*p,size_t o,uint32_t n){memcpy(p+o,&n,4);}
gh_title_player_status gh_player_shared_vector_from_elf(gh_player_shared_vector*s,const void*data,size_t n){
 if(!s||!data)return GH_PLAYER_ARGUMENT;if(s->initialized)return GH_PLAYER_UNSUPPORTED;const uint8_t*b=data;if(n<52||memcmp(b,"\177ELF\1\1\1",7)||u16(b+18)!=8)return GH_PLAYER_BOUNDS;size_t ph=u32(b+28);unsigned stride=u16(b+42),count=u16(b+44);if(stride<32||count>128||ph>n||(size_t)count*stride>n-ph)return GH_PLAYER_BOUNDS;const uint8_t*source=NULL;
 for(unsigned i=0;i<count;i++){const uint8_t*p=b+ph+(size_t)i*stride;uint32_t base=u32(p+8),files=u32(p+16),off=u32(p+4);if(u32(p)!=1||base>0x3bd880||(uint64_t)0x3bd880+16>(uint64_t)base+files)continue;size_t delta=0x3bd880-base;if(off>n||files>n-off||source)return GH_PLAYER_BOUNDS;source=b+off+delta;}
 if(!source)return GH_PLAYER_BOUNDS;memcpy(s->bytes,source,16);s->initialized=1;return GH_PLAYER_OK;
}
static gh_title_player_status fail(gh_player_child_tail*s,gh_game_player_child*c,uint32_t f,gh_title_player_status e){s->frontier=f;c->frontier=f;return e;}
gh_title_player_status gh_game_player_child_finish_00100420(gh_player_child_tail*s,gh_game_player_child*c,
 gh_player_shared_vector*v,gh_game_player_child**publication,gh_core_save_view save,const void*elf,size_t n,gh_static_transform_policy policy,int*out){
 if(!s||!c||!v||!v->initialized||!publication||!out||!elf||!save.bytes||save.size<GH_CORE_SAVE_BYTES||!c->complete||!c->object.view.bytes||c->object.view.size<GH_PLAYER_CHILD_BYTES||!c->model.ready||!c->model_original_return||c->model.root.bytes!=c->object.view.bytes)return GH_PLAYER_ARGUMENT;*out=0;
 if(s->child&&(s->child!=c||s->shared_vector!=v||s->work5f8!=publication))return GH_PLAYER_ARGUMENT;
 if(s->complete){*out=1;return GH_PLAYER_OK;}if(c->init_stage!=2)return GH_PLAYER_ARGUMENT;
 if(!s->child){s->child=c;s->shared_vector=v;s->work5f8=publication;}
 if(s->stage==0){if(c->model.part_count){const gh_actor_part*p;gh_actor_model_status e=gh_actor_model_part_at(&c->model,0,&p);if(e)return fail(s,c,0x14bfe0,GH_PLAYER_BOUNDS);memcpy(p->bytes+0xe0,v->bytes,12);memcpy(p->alias_d0,v->bytes,12);}s->stage=1;}
 if(s->stage==1){gh_actor_model_status e=gh_actor_model_update_hierarchy_00149eb8(&c->model,policy);if(e)return fail(s,c,c->model.frontier,e==GH_ACTOR_MODEL_UNSUPPORTED?GH_PLAYER_UNSUPPORTED:GH_PLAYER_BOUNDS);s->stage=2;}
 if(s->stage==2){gh_player_springs_status e=gh_game_player_springs_initialize_00100870(&s->springs,c,save,elf,n,policy);if(e)return fail(s,c,s->springs.frontier?s->springs.frontier:0x100870,e==GH_SPRINGS_ARGUMENT?GH_PLAYER_ARGUMENT:GH_PLAYER_BOUNDS);s->stage=3;}
 *publication=c;uint8_t*p=c->object.view.bytes;put(p,0x254,u32(p+0x254)|0x8000000u);put(p,0x2f0,u32(p+0x2f0)|1u);c->init_stage=3;c->frontier=0;s->frontier=0;s->complete=1;*out=1;return GH_PLAYER_OK;
}
