/* Readable recovery of2AA6E0 and1F7D48/1F7EF0/1F7FB8 from the user's ELF;
 * MIT decomp reference002aa6e0.c. Numeric limits/evidence are separate from
 * original control flow: this explicit reference is not an EE/VU emulator. */
#include "id_layout_rotation.h"
#include <fenv.h>
#include <float.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
static float bits(uint32_t n){float f;memcpy(&f,&n,4);return f;}
static float mul(float a,float b){volatile float v=a*b;return v;}
static float add(float a,float b){volatile float v=a+b;return v;}
static int normal(float f){return isfinite(f)&&(f==0||fabsf(f)>=FLT_MIN);}
static gh_id_error angle(float f,float*out){float q,t;int32_t n;uint32_t raw;
 if(!normal(f))return GH_ID_UNSUPPORTED;
 q=mul(f,bits(0x3ea2f983));
 /* CVT.W.S / VFTOI0 truncate. Saturated conversion is outside this
  * finite reference. The last admitted positive binary32 q is
  * 2147483520, leaving room for the sign correction before even rounding. */
 if(!(q>=-0x1p31f&&q<0x1p31f))return GH_ID_UNSUPPORTED;
 n=(int32_t)q;memcpy(&raw,&f,4);
 n+=(raw>0&&raw<0x80000000u);
 n-=(int32_t)((uint32_t)n&1u);
 t=mul((float)n,bits(0x40490fdb));*out=add(f,-t);

 return normal(*out)?GH_ID_OK:GH_ID_UNSUPPORTED;
}
static gh_id_error sincos_original(float theta,float*s,float*c){
 /* vf8 = coefficient*t, then squared-t lanes create powers3/5/7/9.
  * Polynomial sum order is cubic, quintic, septic, nonic, exactly as VU. */
 static const uint32_t coeff[4]={0x362e9c14,0xb94fb21f,0x3c08873e,0xbe2aaaa4};
 float t,t2,p,v[4],radical;unsigned i;
 if(!normal(theta)||fabsf(theta)>bits(0x40490fdb))return GH_ID_UNSUPPORTED;
 t=theta<0?add(bits(0x3fc90fdb),theta):add(bits(0x3fc90fdb),-theta);t2=mul(t,t);
 for(i=0;i<4;i++)v[i]=mul(mul(bits(coeff[i]),t),t2);
 for(i=0;i<3;i++)v[i]=mul(v[i],t2);p=add(t,v[3]);
 for(i=0;i<2;i++)v[i]=mul(v[i],t2);p=add(p,v[2]);
 v[0]=mul(v[0],t2);p=add(p,v[1]);p=add(p,v[0]);
 radical=add(1,-mul(p,p));
 /* Supported observed branch has nonnegative normal SQRT argument. Rather
  * than invent VU invalid/denormal behavior, retain the precise boundary. */
 if(!normal(p)||!normal(radical)||radical<0)return GH_ID_UNSUPPORTED;
 *c=p;*s=sqrtf(radical);if(theta<0)*s=-*s;
 return normal(*s)?GH_ID_OK:GH_ID_UNSUPPORTED;
}
static int enter(fenv_t*saved){if(fegetenv(saved))return 0;if(fesetround(FE_TONEAREST)){fesetenv(saved);return 0;}return 1;}
gh_id_error gh_id_angle_reference_001f7d48(float f,float*out){fenv_t env;gh_id_error e;float value;if(!out)return GH_ID_ARGUMENT;if(!enter(&env))return GH_ID_UNSUPPORTED;e=angle(f,&value);if(fesetenv(&env))return GH_ID_UNSUPPORTED;if(!e)*out=value;return e;}
gh_id_error gh_id_sincos_reference(float f,float*s,float*c){fenv_t env;gh_id_error e;float sv,cv;if(!s||!c)return GH_ID_ARGUMENT;if(!enter(&env))return GH_ID_UNSUPPORTED;e=sincos_original(f,&sv,&cv);if(fesetenv(&env))return GH_ID_UNSUPPORTED;if(!e){*s=sv;*c=cv;}return e;}
static int owned(const gh_id_base*o,const gh_id_entry*e){uintptr_t a=(uintptr_t)o->entries,b=(uintptr_t)e;size_t n=(size_t)o->entry_count*sizeof(*e);return b>=a&&b-a<n&&(b-a)%sizeof(*e)==0;}
static gh_id_error fail(gh_id_base*o,gh_id_frontier*f,uint32_t i,gh_id_error error,uint32_t address){o->first_unported=address;if(f){f->address=address;f->entry_index=i;f->kind=o->entries[i].kind;}return error;}
static gh_id_error layout(gh_id_base*o,gh_id_frontier*f){uint32_t i;
 for(i=0;i<o->entry_count;i++){gh_id_entry*e=o->entries+i,*p;float sx=e->scale_x,sy=e->scale_y,ox=e->off_x,oy=e->off_y;unsigned steps=0;gh_id_error error;
  if(!normal(sx)||!normal(sy)||!normal(e->pos_x)||!normal(e->pos_y)||!normal(ox)||!normal(oy))return fail(o,f,i,GH_ID_UNSUPPORTED,0x2aa6e0);
  e->draw_rotation=0;e->draw_scale_x=sx;e->draw_scale_y=sy;
  if(e->parent_ref>=0)for(p=e->parent;p;p=p->parent){
   if(!owned(o,p)||++steps>o->entry_count)return fail(o,f,i,GH_ID_BOUNDS,0x2aa6e0);
   if(!normal(p->scale_x)||!normal(p->scale_y))return fail(o,f,i,GH_ID_UNSUPPORTED,0x2aa6e0);
   if(e->flags&0x40000000U){e->draw_scale_x=mul(e->draw_scale_x,p->scale_x);e->draw_scale_y=mul(e->draw_scale_y,p->scale_y);}
   sx=mul(sx,p->scale_x);sy=mul(sy,p->scale_y);
   if(!(e->flags&0x80)&&p->rotation_speed!=0){error=angle(add(e->draw_rotation,p->rotation_speed),&e->draw_rotation);if(error)return fail(o,f,i,error,0x1f7d48);}
  }
  if(e->draw_rotation!=0){float s,c;error=sincos_original(e->draw_rotation,&s,&c);if(error)return fail(o,f,i,error,0x1f7ef0);
   e->rotation_y=add(mul(s,e->pos_x),mul(c,e->pos_y));e->rotation_x=add(mul(c,e->pos_x),-mul(s,e->pos_y));
  }else{e->rotation_x=e->pos_x;e->rotation_y=e->pos_y;}
  if(e->flags&0x04000000){e->rotation_x=mul(e->rotation_x,sx);e->rotation_y=mul(e->rotation_y,sy);}
  error=angle(add(e->draw_rotation,e->rotation_speed),&e->draw_rotation);if(error)return fail(o,f,i,error,0x1f7d48);
  e->draw_x=e->rotation_x;e->draw_y=e->rotation_y;e->color=e->color_base;steps=0;
  if(e->parent_ref>=0)for(p=e->parent;p;p=p->parent){unsigned c;if(!owned(o,p)||++steps>o->entry_count)return fail(o,f,i,GH_ID_BOUNDS,0x2aa6e0);
   if(p->flags&0x10)e->kind=p->kind;e->draw_x=add(e->draw_x,p->rotation_x);e->draw_y=add(e->draw_y,p->rotation_y);ox=add(ox,p->off_x);oy=add(oy,p->off_y);
   if(!(p->flags&0x10000000)&&!(e->flags&0x10000000)){uint32_t color=0;for(c=0;c<4;c++){unsigned a=(e->color>>(c*8))&255,b=(p->color_base>>(c*8))&255;color|=((a*b/128)&255)<<(c*8);}e->color=color;}
  }
  e->draw_x=add(e->draw_x,ox);e->draw_y=add(e->draw_y,oy);
  if(!normal(e->draw_x)||!normal(e->draw_y)||!normal(e->draw_scale_x)||!normal(e->draw_scale_y))return fail(o,f,i,GH_ID_UNSUPPORTED,0x2aa6e0);
 }return GH_ID_OK;
}
gh_id_error gh_id_layout_rotation_reference(gh_id_base*o,gh_id_frontier*f){fenv_t env;gh_id_error e;if(!o||(!o->entries&&o->entry_count))return GH_ID_ARGUMENT;if(!enter(&env))return GH_ID_UNSUPPORTED;e=layout(o,f);if(fesetenv(&env))return GH_ID_UNSUPPORTED;return e;}
