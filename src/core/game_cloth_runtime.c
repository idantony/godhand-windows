#include "game_cloth_runtime.h"
#include "id_layout_rotation.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
static uint32_t u32(const uint8_t*p){uint32_t v;memcpy(&v,p,4);return v;}
static unsigned u16(const uint8_t*p){return p[0]|(unsigned)p[1]<<8;}
static float rf(const uint8_t*p){float f;memcpy(&f,p,4);return f;}
static void sf(uint8_t*p,float f){memcpy(p,&f,4);}
static float add(float a,float b){volatile float f=a+b;return f;}
static float mul(float a,float b){volatile float f=a*b;return f;}
static int map(const uint8_t*b,size_t n,uint32_t address,void*out,size_t bytes){
 if(n<52||memcmp(b,"\177ELF\1\1\1",7)||u16(b+18)!=8)return 0;size_t table=u32(b+28);unsigned stride=u16(b+42),count=u16(b+44);if(stride<32||count>128||table>n||(size_t)stride*count>n-table)return 0;int found=0;
 for(unsigned i=0;i<count;i++){const uint8_t*p=b+table+(size_t)i*stride;uint32_t base=u32(p+8),files=u32(p+16),memory=u32(p+20),offset=u32(p+4);if(u32(p)!=1||address<base||(uint64_t)address+bytes>(uint64_t)base+memory)continue;if(found||files>memory||offset>n||files>n-offset)return 0;found=1;memset(out,0,bytes);size_t delta=address-base;if(delta<files){size_t available=files-delta;if(available>bytes)available=bytes;memcpy(out,b+offset+delta,available);}}
 return found;
}
gh_cloth_status gh_cloth_runtime_from_elf(gh_cloth_runtime_globals*s,const void*b,size_t n){if(!s||!b||s->loaded)return GH_CLOTH_ARGUMENT;gh_cloth_runtime_globals value={0};if(!map(b,n,0x3bc730,value.data,sizeof(value.data))||!map(b,n,0x4615f0,value.wind,16))return GH_CLOTH_BOUNDS;value.loaded=1;*s=value;return GH_CLOTH_OK;}
gh_cloth_status gh_cloth_runtime_initialize_00107de0(gh_cloth_runtime_globals*s,int init,int priority){if(!s||!s->loaded)return GH_CLOTH_ARGUMENT;if(init&&priority==0xffff){memset(s->wind,0,16);uint32_t one=0x3f800000;memcpy(s->wind+12,&one,4);s->initialized=1;}return GH_CLOTH_OK;}
static gh_cloth_status fail(gh_cloth_step_prefix*s,uint32_t f,gh_cloth_status e){s->frontier=f;return e;}
static int same(const gh_cloth_step_bindings*a,const gh_cloth_step_bindings*b){return a->springs==b->springs&&a->attachment==b->attachment&&a->templates==b->templates&&a->globals==b->globals&&a->random==b->random&&a->actors==b->actors&&a->matrix_policy==b->matrix_policy&&a->random_policy==b->random_policy;}
static gh_cloth_status phase(gh_cloth_step_prefix*s,unsigned index,float coefficient){
 gh_game_player_springs*springs=s->bindings.springs;uint8_t*p=springs->child->object.view.bytes;uint32_t raw;if(gh_game_mt_next_0030f348(s->bindings.random,&raw))return fail(s,0x31d000,GH_CLOTH_ARGUMENT);s->random_words[index]=raw;
 uint32_t unit=gh_frame_unit_bits_0030f518_word(raw);float u;memcpy(&u,&unit,4);float before=add(rf(p+0x2098),mul(u,coefficient)),wrapped;
 sf(p+0x2098,before);if(!isfinite(before)||gh_id_angle_reference_001f7d48(before,&wrapped))return fail(s,0x1f7d48,GH_CLOTH_UNSUPPORTED);sf(p+0x2098,wrapped);return GH_CLOTH_OK;
}
static gh_cloth_status run(gh_cloth_step_prefix*s){
 const gh_cloth_step_bindings*b=&s->bindings;gh_game_player_springs*springs=b->springs;gh_game_player_child*child=springs->child;uint8_t*p=child->object.view.bytes;
 const gh_actor_model*parent=NULL;if(b->attachment&&b->attachment->attached){if(b->attachment->child!=child||!b->attachment->parent_model||child->model.external_parent!=b->attachment->parent_model)return fail(s,0x104088,GH_CLOTH_ARGUMENT);parent=b->attachment->parent_model;}
 if(s->stage==0){if(!springs->count){s->complete=1;s->frontier=0;s->stage=6;return GH_CLOTH_OK;}if(springs->count>44||u32(p+0x2050)!=springs->count||!child->model.parts_ready||!child->model.parts_allocated)return fail(s,0x1040a8,GH_CLOTH_BOUNDS);
  s->radius_scale=rf((parent?parent->root.bytes:p)+0x114);s->floor_y=rf(child->object.view.alias_f0+4);if(!isfinite(s->radius_scale)||!isfinite(s->floor_y))return fail(s,0x104090,GH_CLOTH_NUMERIC);
  if(!(u32(p+0x20ac)&0x100)){if(!b->actors)return fail(s,0x131e10,GH_CLOTH_UNSUPPORTED);const gh_actor_part*part;float v[3];if(gh_actor_model_part_at(&child->model,0,&part))return fail(s,0x1040dc,GH_CLOTH_BOUNDS);memcpy(v,part->alias_f0,12);gh_scene_ray_request request={.mask=2,.flags=0x400,.segmented=1};memcpy(request.from,v,12);memcpy(request.to,v,12);request.from[3]=request.to[3]=1;request.from[1]=add(v[1],0.5f);request.to[1]=add(v[1],-10);float hit[4]={0,0,0,1};request.hit_position=hit;int original;uint32_t f;gh_scene_ray_status status=gh_scene_ray_query_00131e10(b->actors,&request,&original,&f);if(status)return fail(s,f,GH_CLOTH_UNSUPPORTED);if(original==1)s->floor_y=add(hit[1],0.03f);}
  s->stage=1;
 }
 if(s->stage==1){if(u32(p+0x2f0)&1){gh_player_springs_status e=gh_player_springs_reset_00107c68(springs,b->matrix_policy);if(e)return fail(s,springs->frontier,GH_CLOTH_UNSUPPORTED);}s->stage=2;}
 if(s->stage==2){s->geometry=NULL;if(!(u32(p+0x20ac)&1)){uint32_t count=u32(p+0x2088);if(count){gh_cloth_sphere*source;size_t capacity;gh_cloth_status e=gh_cloth_templates_for_costume(b->templates,springs->costume,&source,&capacity);if(e)return fail(s,0x106218,e);if(count>capacity)return fail(s,0x106218,GH_CLOTH_BOUNDS);uint32_t f;e=gh_cloth_prepare_00106218(b->templates,parent?parent:&child->model,source,count,b->matrix_policy,&s->geometry,&f);if(e)return fail(s,f,e);}}s->stage=3;}
 if(s->stage==3){if(!(u32(p+0x20ac)&0x40)){gh_cloth_status e=phase(s,0,rf(b->globals->data+0x98));if(e){s->phase_failure=e;return e;}}s->stage=4;}
 if(s->stage==4){if(u32(p+0x20ac)&0x400){gh_cloth_status e=phase(s,1,rf(p+0x20c0));if(e){s->phase_failure=e;return e;}}s->stage=5;}
 if(s->stage==5){if(u32(p+0x20a4))return fail(s,0x10428c,GH_CLOTH_UNSUPPORTED);for(unsigned i=0;i<springs->count;i++)if(springs->indices[0][i]>=44)return fail(s,0x1042d4,GH_CLOTH_BOUNDS);for(unsigned i=0;i<springs->count;i++)p[0x4d0+(size_t)springs->indices[0][i]*0xa0+0x90]=0;s->stage=6;}
 return fail(s,0x10430c,GH_CLOTH_UNSUPPORTED);
}
gh_cloth_status gh_cloth_step_prefix_00104010(gh_cloth_step_prefix*s,const gh_cloth_step_bindings*b){
 if(!s||!b||!b->springs||!b->springs->initialized||!b->springs->child||!b->templates||!b->templates->initialized||!b->globals||!b->globals->initialized||!b->random)return GH_CLOTH_ARGUMENT;
 gh_game_player_child*child=b->springs->child;if(!child->complete||!child->object.view.bytes||child->object.view.size<GH_PLAYER_CHILD_BYTES||!child->object.view.alias_f0||child->model.native_identity!=&child->model||child->model.root.bytes!=child->object.view.bytes)return GH_CLOTH_ARGUMENT;
 if(s->stage&&!same(&s->bindings,b))return GH_CLOTH_ARGUMENT;if(s->complete)return GH_CLOTH_OK;if(s->phase_failure)return s->phase_failure;
 if(b->matrix_policy!=GH_STATIC_BINARY32_REFERENCE||b->random_policy!=GH_FRAME_CHOP_REFERENCE)return fail(s,0x104010,GH_CLOTH_UNSUPPORTED);if(!s->stage)s->bindings=*b;
 fenv_t env;if(feholdexcept(&env))return GH_CLOTH_NUMERIC;if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return GH_CLOTH_NUMERIC;}gh_cloth_status result=run(s);if(fesetenv(&env))return GH_CLOTH_NUMERIC;return result;
}
