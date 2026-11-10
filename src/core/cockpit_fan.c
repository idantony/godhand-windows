#include "cockpit_fan.h"
#include "id_layout_rotation.h"
#include "../assets/tim2.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
static uint32_t w(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static float add(float a,float b){volatile float n=a+b;return n;}
static float mul(float a,float b){volatile float n=a*b;return n;}
static float divf(float a,float b){volatile float n=a/b;return n;}
static gh_id_error stop(uint32_t*f,uint32_t pc,gh_id_error e){if(f)*f=pc;return e;}
static gh_id_error picture(const gh_id_texture*t,gh_tim2_picture*p){
 if(!t||!t->source.bytes)return GH_ID_ARGUMENT;gh_tim2_error e=gh_tim2_first(t->source.bytes,t->source.size,p);if(e)return e==GH_TIM2_BOUNDS?GH_ID_BOUNDS:GH_ID_UNSUPPORTED;
 if(p->width!=16||p->height!=16||p->image_type!=4||p->clut_type!=3||p->colors!=16||p->tex0!=UINT64_C(0x111408000))return GH_ID_UNSUPPORTED;
 for(unsigned i=0;i<16;i++)if(p->clut[(size_t)i*4+3]>128)return GH_ID_UNSUPPORTED;return GH_ID_OK;
}
gh_id_error gh_cockpit_fan_texture_rgba(const gh_id_texture*t,void*out,size_t n){gh_tim2_picture p;gh_id_error e=picture(t,&p);if(e)return e;gh_tim2_error r=gh_tim2_rgba4(&p,out,n);return r==GH_TIM2_OK?GH_ID_OK:r==GH_TIM2_BOUNDS?GH_ID_CAPACITY:GH_ID_UNSUPPORTED;}
static gh_id_error texture(const gh_id_state*m,gh_id_texture*t){
 const gh_bundle_entry*b=&m->pair[1].texture_data;if(!b->bytes||b->size<4)return GH_ID_BOUNDS;uint32_t count=w(b->bytes);
 if(count>(b->size-4)/4||count<33)return GH_ID_BOUNDS;uint32_t off=w(b->bytes+33*4);size_t end=b->size;
 if(off<4+(size_t)count*4||off>=b->size)return GH_ID_BOUNDS;
 for(unsigned i=1;i<=count;i++){uint32_t a=w(b->bytes+(size_t)i*4);if(a&&(a<4+(size_t)count*4||a>b->size))return GH_ID_BOUNDS;if(a>off&&a<end)end=a;}
 *t=(gh_id_texture){.source={b->bytes+off,end-off,off},.width=16,.height=16,.tex0=UINT64_C(0x111408000)};gh_tim2_picture p;return picture(t,&p);
}
/* Column-major SDK30A650(z),30A7B8(y),30A700(x) multiplication order.
 * Each vmul/vmadd becomes separately rounded native reference operations. */
static void matrix(float*out,const float*a,const float*b){float m[16];for(unsigned col=0;col<4;col++)for(unsigned r=0;r<4;r++){float n=mul(a[r],b[col*4]);n=add(n,mul(a[4+r],b[col*4+1]));n=add(n,mul(a[8+r],b[col*4+2]));m[col*4+r]=add(n,mul(a[12+r],b[col*4+3]));}memcpy(out,m,sizeof(m));}
gh_id_error gh_cockpit_fan_prepare(const gh_id_state*m,gh_core_save_view save,gh_cockpit_fan*out,uint32_t*f){
 if(f)*f=0;if(!m||!m->initialized||!out)return GH_ID_ARGUMENT;if(!save.bytes||save.size<GH_CORE_SAVE_BYTES)return stop(f,0x1fa080,GH_ID_ARGUMENT);
 gh_cockpit_fan n={0};gh_id_error e=texture(m,&n.texture);if(e)return stop(f,0x2ace28,e);
 static const int points[]={1000,3000,4000,4000,5000};int value=(int16_t)(save.bytes[0x1c]|(unsigned)save.bytes[0x1d]<<8),level=0;
 while(level<5&&points[level]<=value)level++;level=(level<3?level:4)+1;int previous=level<2?0:points[level-2],den=points[level-1]-previous;if(den<=0)den=1000;
 if(value<previous||value-previous>den)return stop(f,0x1fa080,GH_ID_UNSUPPORTED);
 fenv_t env;if(feholdexcept(&env))return stop(f,0x1430e8,GH_ID_UNSUPPORTED);if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return stop(f,0x1430e8,GH_ID_UNSUPPORTED);}
 float ratio=divf((float)(value-previous),(float)den),theta=mul(.115f,ratio),sz,cz,s0,c0;
 e=gh_id_sincos_reference(theta,&sz,&cz);if(!e)e=gh_id_sincos_reference(0,&s0,&c0);
 if(e){(void)fesetenv(&env);return stop(f,0x30a5d0,e);}
 float rz[16]={cz,sz,0,0,-sz,cz,0,0,0,0,1,0,0,0,0,1};
 float ry[16]={c0,0,-s0,0,0,1,0,0,s0,0,c0,0,0,0,0,1};
 float rx[16]={1,0,0,0,0,c0,s0,0,0,-s0,c0,0,0,0,0,1},rotation[16];matrix(rotation,ry,rz);matrix(rotation,rx,rotation);
 n.x[0]=46;n.y[0]=394;n.u[0]=0;n.v[0]=1;n.x[1]=51;n.y[1]=434;
 float p[4]={5,40,0,1};for(unsigned i=2;i<33;i++){
  float q[4];for(unsigned r=0;r<4;r++){float a=mul(rotation[r],p[0]);a=add(a,mul(rotation[4+r],p[1]));a=add(a,mul(rotation[8+r],p[2]));q[r]=add(a,mul(rotation[12+r],p[3]));}memcpy(p,q,sizeof(p));
  n.x[i]=add(mul(p[0],.9f),46);n.y[i]=add(p[1],394);n.u[i]=mul(ratio,divf((float)(i-1),31));
 }
 if(fesetenv(&env))return stop(f,0x1430e8,GH_ID_UNSUPPORTED);
 n.color=0x80808080;n.bank=6;n.depth=5;n.ready=1;*out=n;return GH_ID_OK;
}
