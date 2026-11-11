#include "game_cloth_forces.h"
#include "id_layout_rotation.h"
#include "sdk_rotation_trig.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
static uint32_t u32(const uint8_t*p){uint32_t v;memcpy(&v,p,4);return v;}
static float rf(const uint8_t*p){float v;memcpy(&v,p,4);return v;}
static void sf(uint8_t*p,float v){memcpy(p,&v,4);}
static float add(float a,float b){volatile float f=a+b;return f;}
static float mul(float a,float b){volatile float f=a*b;return f;}
static float divide(float a,float b){volatile float f=a/b;return f;}
static gh_cloth_status fail(gh_cloth_forces*s,uint32_t f,gh_cloth_status e){s->frontier=f;return e;}
static gh_cloth_status wave(const gh_cloth_step_prefix *prefix, unsigned row,float bias,
                            float *out, uint32_t *frontier)
{
    const gh_cloth_operands *springs = &prefix->operands;
    *out = 0;
    if (!(springs->tables & (1u << 8))) return GH_CLOTH_OK;
    float coefficient, phase;
    memcpy(&coefficient, springs->coefficients[1] + row, sizeof(coefficient));
    if (gh_id_angle_reference_001f7d48(add(rf(springs->config + 0x48), coefficient), &phase)) {
        *frontier = 0x1f7d48;
        return GH_CLOTH_UNSUPPORTED;
    }
    /* Patched ELF 1F7EF0 and SDK 30A5D0 return the same signed VF4.x
     * with identical coefficient bits. VSQRT takes the radical's magnitude. */
    float sine, cosine;
    if (gh_sdk_rotation_sincos_reference_0030a5d0(phase, &sine, &cosine,
            prefix->bindings.matrix_policy, frontier)) {
        *frontier = 0x1f7ef0;
        return GH_CLOTH_UNSUPPORTED;
    }
    *out = add(sine, bias);
    return GH_CLOTH_OK;
}

static gh_cloth_status running_force(const gh_cloth_step_prefix *prefix,unsigned row,
    uint8_t *record,const float center[3],float strength,uint32_t *frontier)
{
    const float x=add(rf(record+0x40),-center[0]);
    const float z=add(rf(record+0x48),-center[2]);
    const float squared=add(mul(x,x),mul(z,z));
    if(!isfinite(squared)){
        *frontier=0x101e28;
        return GH_CLOTH_NUMERIC;
    }
    float force[3]={0};
    const float distance=sqrtf(squared);
    if(distance>0){
        const float reciprocal=divide(1,distance);
        force[0]=mul(x,reciprocal);
        force[2]=mul(z,reciprocal);
    }
    for(unsigned axis=0;axis<3;axis++)force[axis]=mul(force[axis],strength);
    if(prefix->operands.tables&(1u<<8)){
        float amplitude;
        gh_cloth_status status=wave(prefix,row,0.8f,&amplitude,frontier);
        if(status)return status;
        for(unsigned axis=0;axis<3;axis++)force[axis]=mul(force[axis],amplitude);
    }
    for(unsigned axis=0;axis<3;axis++)
        sf(record+0x40+4*axis,add(rf(record+0x40+4*axis),force[axis]));
    return GH_CLOTH_OK;
}
static gh_cloth_status body(gh_cloth_forces*s,gh_cloth_step_prefix*prefix){
 const gh_cloth_operands*springs=&prefix->operands;uint8_t*p=springs->config;uint32_t flags=u32(p+0x5c);
 if(flags&0x20){s->complete=1;s->frontier=0x10497c;return GH_CLOTH_OK;}
 if(u32(p+0x20))return fail(s,0x1044cc,GH_CLOTH_UNSUPPORTED);
 if(!(flags&0x100))return fail(s,0x10485c,GH_CLOTH_UNSUPPORTED);
 const gh_actor_model*owner=springs->parent?springs->parent:springs->model;
 const gh_title_player_view*root=&owner->root;
 float translation[3]={0},center[3]={0},radial=0;
 const unsigned kind=root->bytes[0x2fe]|(unsigned)root->bytes[0x2ff]<<8;
 if(kind>=0x100&&kind<=0x1fe){
  const uint32_t name=u32(root->bytes+0x4ac);
  if(name==0x41b800){
   if(root->size<0x678)return fail(s,0x10436c,GH_CLOTH_BOUNDS);
   memcpy(translation,root->bytes+0x650,12);
   radial=rf(root->bytes+0x674);
   if(!isfinite(radial))return fail(s,0x1043f8,GH_CLOTH_NUMERIC);
   if(radial>0){
    if(root->alias_f0!=root->bytes+0xb0)return fail(s,0x1043dc,GH_CLOTH_BOUNDS);
    memcpy(center,root->alias_f0,12);
   }
  }else if(name!=0x41eb80)return fail(s,0x3a5678,GH_CLOTH_UNSUPPORTED);
 }
 uint8_t records[44*0xa0];memcpy(records,springs->records,sizeof(records));
 for(unsigned i=0;i<springs->count;i++){unsigned id=springs->indices[0][i];if(id>=44)return fail(s,0x104480,GH_CLOTH_BOUNDS);uint8_t*r=records+id*0xa0;
  for(unsigned k=0;k<3;k++)sf(r+0x40+4*k,add(rf(r+0x40+4*k),translation[k]));memcpy(r+0x50,r+0x40,12);
  sf(r+0x34,add(rf(r+0x34),-rf(p+0x3c)));for(unsigned k=0;k<3;k++)sf(r+0x40+4*k,add(rf(r+0x40+4*k),rf(r+0x30+4*k)));
  if(!(flags&0x40)&&(springs->tables&(1u<<9))){float amplitude;uint32_t pc;gh_cloth_status e=wave(prefix,i,1,&amplitude,&pc);if(e)return fail(s,pc,e);float coef;memcpy(&coef,springs->coefficients[2]+i,4);for(unsigned k=0;k<3;k++){float wind=mul(rf(prefix->bindings.globals->wind+4*k),amplitude);wind=mul(wind,coef);sf(r+0x40+4*k,add(rf(r+0x40+4*k),wind));}}
  if(flags&0x400){float amplitude;uint32_t pc;gh_cloth_status e=wave(prefix,i,1,&amplitude,&pc);if(e)return fail(s,pc,e);float coef=(springs->tables&(1u<<9))?rf((const uint8_t*)(springs->coefficients[2]+i)):1;for(unsigned k=0;k<3;k++){float wind=mul(rf(p+0x60+4*k),amplitude);if((springs->tables&(1u<<9)))wind=mul(wind,coef);sf(r+0x40+4*k,add(rf(r+0x40+4*k),wind));}}
  if(radial>0){uint32_t pc=0;gh_cloth_status e=running_force(prefix,i,r,center,radial,&pc);if(e)return fail(s,pc,e);}
  if(r[0x91]&1)memcpy(r+0x40,r+0x80,12);
  for(unsigned off=0x30;off<=0x58;off+=4)if(off!=0x3c&&off!=0x4c&&!isfinite(rf(r+off)))return fail(s,0x104978,GH_CLOTH_NUMERIC);
 }
 memcpy(springs->records,records,sizeof(records));s->complete=1;s->frontier=0x10497c;return GH_CLOTH_OK;
}
gh_cloth_status gh_cloth_integrate_forces_0010430c(gh_cloth_forces*s,gh_cloth_step_prefix*prefix){
 if(!s||!prefix||prefix->stage!=6||prefix->complete||prefix->frontier!=0x10430c||!prefix->operands.model||!prefix->bindings.globals)return GH_CLOTH_ARGUMENT;if(s->prefix&&s->prefix!=prefix)return GH_CLOTH_ARGUMENT;if(s->complete)return GH_CLOTH_OK;s->prefix=prefix;
 fenv_t env;if(feholdexcept(&env))return GH_CLOTH_NUMERIC;if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return GH_CLOTH_NUMERIC;}gh_cloth_status e=body(s,prefix);if(fesetenv(&env))return GH_CLOTH_NUMERIC;return e;
}
