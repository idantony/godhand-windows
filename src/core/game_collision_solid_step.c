#include "game_collision_solid_step.h"
#include "game_scene_at_geometry.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
#include <float.h>
static uint32_t w(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
static void ww(uint8_t*p,uint32_t n){memcpy(p,&n,4);}
static float fl(const uint8_t*p){float v;memcpy(&v,p,4);return v;}
static float add(float a,float b){volatile float v=a+b;return v;}
static float sub(float a,float b){volatile float v=a-b;return v;}
static float mul(float a,float b){volatile float v=a*b;return v;}
static float divide(float a,float b){volatile float v=a/b;return v;}
static int reference(float v){return isfinite(v)&&(v==0||fabsf(v)>=FLT_MIN);}
static int32_t signed_word(uint32_t u){int32_t v;memcpy(&v,&u,4);return v;}
static gh_solid_unit_status fail(uint32_t*f,uint32_t pc,gh_solid_unit_status e){*f=pc;return e;}
static int unit_valid(const gh_game_collision_solids*m,const gh_collision_solid_unit*u){return u&&u->manager==m&&u->object&&u->object->bytes&&u->object->size>=0x5a4&&u->allocation.pointer;}
static gh_solid_unit_status validate(gh_game_collision_solids*m,uint32_t*f){
 if(m->unit_head!=m->native_units)return fail(f,0x1346c8,GH_SOLID_UNIT_UNSUPPORTED);uint32_t count=0,total=0;
 for(gh_collision_solid_unit*u=m->native_units;u;u=u->next){if(++count>m->unit_count||!unit_valid(m,u))return fail(f,0x1346c8,GH_SOLID_UNIT_UNSUPPORTED);for(gh_collision_native_sphere*s=u->spheres;s;s=s->next){if(++total>m->global_00460d18||!s->constructed||!s->allocation.pointer||!s->matrix)return fail(f,0x133aa8,GH_SOLID_UNIT_UNSUPPORTED);}}
 return count==m->unit_count&&total==m->global_00460d18?GH_SOLID_UNIT_OK:fail(f,0x1346c8,GH_SOLID_UNIT_UNSUPPORTED);
}
static gh_solid_unit_status update_sphere(gh_collision_native_sphere*s,float*minimum,float*maximum,uint32_t*f){
 uint8_t*p=s->allocation.pointer;if(w(p+0x30)!=0x41d7e0)return fail(f,0x133aa8,GH_SOLID_UNIT_UNSUPPORTED);
 float matrix[16],local[3],center[4],low[3],high[3];memcpy(matrix,s->matrix,64);memcpy(local,p+0x50,12);if(gh_scene_at_apply_world_001508d8(matrix,local,center))return fail(f,0x30a2b0,GH_SOLID_UNIT_UNSUPPORTED);
 float radius=fl(p+0x70);if(!reference(radius))return fail(f,0x133aa8,GH_SOLID_UNIT_UNSUPPORTED);for(unsigned k=0;k<3;k++){low[k]=sub(center[k],radius);high[k]=add(center[k],radius);if(!reference(low[k])||!reference(high[k]))return fail(f,0x133aa8,GH_SOLID_UNIT_UNSUPPORTED);}
 memcpy(p+0x60,center,16);ww(p+8,w(p+8)&~2u);memcpy(p+0x10,low,12);memcpy(p+0x20,high,12);for(unsigned k=0;k<3;k++){if(low[k]<minimum[k])minimum[k]=low[k];if(maximum[k]<high[k])maximum[k]=high[k];}return GH_SOLID_UNIT_OK;
}
static gh_solid_unit_status aabb(const gh_collision_solid_unit*a,const gh_collision_solid_unit*b,int*out,uint32_t*f){
 const uint8_t*p=a->allocation.pointer,*q=b->allocation.pointer;*out=1;for(unsigned k=0;k<3;k++){float lo=fl(p+0x20+4*k),hi=fl(p+0x30+4*k),ol=fl(q+0x20+4*k),oh=fl(q+0x30+4*k);if(!reference(lo)||!reference(hi)||!reference(ol)||!reference(oh))return fail(f,0x135250,GH_SOLID_UNIT_UNSUPPORTED);if(!(ol<=hi&&lo<=oh))*out=0;}return GH_SOLID_UNIT_OK;
}
static gh_solid_unit_status sphere_pair(gh_collision_solid_unit*source,gh_collision_native_sphere*a,gh_collision_solid_unit*other,gh_collision_native_sphere*b,int*out,uint32_t*f){
 const uint8_t*p=a->allocation.pointer,*q=b->allocation.pointer;float d[3];for(unsigned k=0;k<3;k++){float av=fl(p+0x60+4*k),bv=fl(q+0x60+4*k);if(!reference(av)||!reference(bv))return fail(f,0x134cb0,GH_SOLID_UNIT_UNSUPPORTED);d[k]=sub(av,bv);}
 float r=add(fl(p+0x70),fl(q+0x70)),square=add(mul(d[0],d[0]),mul(d[2],d[2])),rr=mul(r,r);if(!reference(r)||!reference(square)||!reference(rr))return fail(f,0x134cb0,GH_SOLID_UNIT_UNSUPPORTED);
 *out=0;if(rr<square)return GH_SOLID_UNIT_OK;*out=1;volatile float distance=sqrtf(square);float penetration=sub(r,distance);uint32_t source_type=w((const uint8_t*)source->allocation.pointer+8),other_type=w((const uint8_t*)other->allocation.pointer+8);if(!source_type)return GH_SOLID_UNIT_OK;
 float factor;if(source_type==other_type)factor=fl((const uint8_t*)other->allocation.pointer+12);else factor=signed_word(other_type)<signed_word(source_type)?1.0f:0.0f;if(!reference(factor)||!reference(penetration))return fail(f,0x134cb0,GH_SOLID_UNIT_UNSUPPORTED);
 /* Original XZ acceptance, FULLXYZ SDK normalization. Do not collapse to
  * planar direction, nor simplify push-push*(1-factor) into push*factor. */
 float norm_square=add(add(mul(d[0],d[0]),mul(d[1],d[1])),mul(d[2],d[2]));if(!reference(norm_square))return fail(f,0x30a370,GH_SOLID_UNIT_UNSUPPORTED);volatile float length=sqrtf(norm_square);if(!reference(length))return fail(f,0x30a370,GH_SOLID_UNIT_UNSUPPORTED);float inverse=length==0?FLT_MAX:divide(1,length);if(!reference(inverse))return fail(f,0x30a370,GH_SOLID_UNIT_UNSUPPORTED);
 float push[3];for(unsigned k=0;k<3;k++){push[k]=mul(mul(d[k],inverse),penetration);if(!reference(push[k]))return fail(f,0x30a4e8,GH_SOLID_UNIT_UNSUPPORTED);}
 gh_title_player_view*object=source->object;uintptr_t start=(uintptr_t)object->bytes,alias=(uintptr_t)object->alias_f0;if(alias<start||alias-start>object->size||12>object->size-(alias-start))return fail(f,0x134cb0,GH_SOLID_UNIT_ARGUMENT);
 float remain=sub(1,factor),x=add(fl(object->alias_f0),sub(push[0],mul(push[0],remain))),z=add(fl(object->alias_f0+8),sub(push[2],mul(push[2],remain)));if(!reference(x)||!reference(z))return fail(f,0x134cb0,GH_SOLID_UNIT_UNSUPPORTED);memcpy(object->alias_f0,&x,4);memcpy(object->alias_f0+8,&z,4);return GH_SOLID_UNIT_OK;
}
static gh_solid_unit_status body(gh_game_collision_solids*m,const gh_title_player_view*actor,gh_collision_solid_frame*c,int*out,uint32_t*f){
 if(c->phase==0){for(unsigned i=0;i<32;i++){m->words08[i]=0;m->contacts08[i]=NULL;}m->field88=0;c->phase=5;}
 if(c->phase==5){gh_solid_unit_status e=validate(m,f);if(e)return e;c->source=m->native_units;while(c->source&&c->source->object->bytes!=actor->bytes)c->source=c->source->next;if(!c->source){*out=0;*c=(gh_collision_solid_frame){0};return GH_SOLID_UNIT_OK;}c->source_sphere=c->source->spheres;for(unsigned k=0;k<3;k++){c->minimum[k]=10000;c->maximum[k]=-10000;}c->phase=1;}
 if(c->phase==1){while(c->source_sphere){if(c->source_visited>=m->global_00460d18)return fail(f,0x133aa8,GH_SOLID_UNIT_UNSUPPORTED);gh_solid_unit_status e=update_sphere(c->source_sphere,c->minimum,c->maximum,f);if(e)return e;c->source_sphere=c->source_sphere->next;c->source_visited++;}memcpy((uint8_t*)c->source->allocation.pointer+0x20,c->minimum,12);memcpy((uint8_t*)c->source->allocation.pointer+0x30,c->maximum,12);c->source_sphere=c->source->spheres;c->source_visited=0;c->phase=2;}
 while(c->source_sphere){uint8_t*source_bytes=c->source_sphere->allocation.pointer;
  if(c->phase==2){if(++c->source_visited>m->global_00460d18)return fail(f,0x1348e8,GH_SOLID_UNIT_UNSUPPORTED);if(!(w(source_bytes+8)&1)){c->source_sphere=c->source_sphere->next;continue;}c->other=m->native_units;c->other_visited=0;c->phase=3;}
  while(c->other){if(c->phase==3){if(c->other_visited>=m->unit_count||!unit_valid(m,c->other))return fail(f,0x1346c8,GH_SOLID_UNIT_UNSUPPORTED);if(c->other==c->source||(w(c->other->object->bytes+0x5a0)&8)){c->other=c->other->next;c->other_visited++;continue;}int overlap;gh_solid_unit_status e=aabb(c->source,c->other,&overlap,f);if(e)return e;if(!overlap){c->other=c->other->next;c->other_visited++;continue;}c->other_sphere=c->other->spheres;c->sphere_visited=0;c->phase=4;}
   while(c->other_sphere){uint8_t*other_bytes=c->other_sphere->allocation.pointer;if(c->sphere_visited>=m->global_00460d18)return fail(f,0x1348e8,GH_SOLID_UNIT_UNSUPPORTED);if(w(other_bytes+8)&1){unsigned a=w(source_bytes+4),b=w(other_bytes+4);int hit=0;if((a==1&&b==3)||(a==3&&b==1))return fail(f,0x134e18,GH_SOLID_UNIT_UNSUPPORTED);if(a==3&&b==3)return fail(f,0x135040,GH_SOLID_UNIT_UNSUPPORTED);if(a==1&&b==1){gh_solid_unit_status e=sphere_pair(c->source,c->source_sphere,c->other,c->other_sphere,&hit,f);if(e)return e;}if(hit){if(c->other->object&&m->field88<32){m->contacts08[m->field88]=c->other->object;m->field88++;}c->hit=1;ww(source_bytes+8,w(source_bytes+8)|2);ww(other_bytes+8,w(other_bytes+8)|2);}}
    c->other_sphere=c->other_sphere->next;c->sphere_visited++;
   }c->other=c->other->next;c->other_visited++;c->phase=3;
  }c->source_sphere=c->source_sphere->next;c->phase=2;
 }
 *out=c->hit;*c=(gh_collision_solid_frame){0};return GH_SOLID_UNIT_OK;
}
gh_solid_unit_status gh_collision_solid_step_resume_001348e8(gh_game_collision_solids*m,const gh_title_player_view*actor,gh_static_transform_policy policy,gh_collision_solid_frame*c,int*out,uint32_t*f){
 if(!m||!actor||!actor->bytes||actor->size<0x5a4||!c||!out||!f)return GH_SOLID_UNIT_ARGUMENT;*f=0;*out=0;
 if(c->active&&(c->identity!=c||c->manager!=m||c->actor_identity!=actor->bytes))return GH_SOLID_UNIT_ARGUMENT;
 if(!c->active){if(w(actor->bytes+0x5a0)&12)return GH_SOLID_UNIT_OK;*c=(gh_collision_solid_frame){.identity=c,.manager=m,.actor_identity=actor->bytes,.active=1};}
 if(policy!=GH_STATIC_BINARY32_REFERENCE)return fail(f,0x1348e8,GH_SOLID_UNIT_UNSUPPORTED);
 fenv_t env;if(feholdexcept(&env))return fail(f,0x1348e8,GH_SOLID_UNIT_UNSUPPORTED);if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return fail(f,0x1348e8,GH_SOLID_UNIT_UNSUPPORTED);}gh_solid_unit_status e=body(m,actor,c,out,f);if(fesetenv(&env))return fail(f,0x1348e8,GH_SOLID_UNIT_UNSUPPORTED);return e;
}
gh_solid_unit_status gh_collision_solid_contact_at(const gh_game_collision_solids*m,unsigned index,const gh_title_player_view**out,uint32_t*f){if(!m||!out||!f)return GH_SOLID_UNIT_ARGUMENT;*out=NULL;*f=0;if(m->unit_head!=m->native_units)return fail(f,0x1346c8,GH_SOLID_UNIT_UNSUPPORTED);if(index>=m->field88||index>=32)return GH_SOLID_UNIT_ARGUMENT;if(!m->contacts08[index])return fail(f,0x1348e8,GH_SOLID_UNIT_UNSUPPORTED);unsigned seen=0;for(const gh_collision_solid_unit*u=m->native_units;u;u=u->next){if(++seen>m->unit_count||u->manager!=m)return fail(f,0x1346c8,GH_SOLID_UNIT_UNSUPPORTED);if(u->object==m->contacts08[index]){*out=u->object;return GH_SOLID_UNIT_OK;}}return fail(f,0x1348e8,GH_SOLID_UNIT_UNSUPPORTED);}
