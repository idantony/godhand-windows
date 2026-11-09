/* Original2ABDC0/2AC048/2AC1C0/2AC298 + bounded interpolation leaves.
 * Actual local ELF is authority; see analysis/id-animation-evidence.json.
 * Explicit host numeric/stack-reference adaptation, not a generic EE/VU VM. */
#include "id_animation.h"
#include <fenv.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
static uint32_t word(const unsigned char*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static int half(const unsigned char*p){unsigned n=p[0]|(unsigned)p[1]<<8;return n<32768?(int)n:(int)n-65536;}
static float scalar(const unsigned char*p){uint32_t n=word(p);float f;memcpy(&f,&n,4);return f;}
static float mul(float a,float b){volatile float v=a*b;return v;}
static float add(float a,float b){volatile float v=a+b;return v;}
static float divide(float a,float b){volatile float v=a/b;return v;}
static void tick(gh_id_entry*e,unsigned i){uint16_t n=(uint16_t)((uint16_t)e->jitter[i]+1);e->jitter[i]=(int16_t)(n<32768?(int)n:(int)n-65536);}
static gh_id_error fail(gh_id_base*b,gh_id_entry*e,gh_id_frontier*f,uint32_t a,gh_id_error error){b->first_unported=a;if(f){f->entry_index=(uint32_t)(e-b->entries);f->address=a;f->kind=e->kind;}return error;}
static float hermite(float t,float tangent0,float tangent1){float t2=mul(t,t),t3=mul(t2,t),three=mul(t2,3),twice=add(t3,t3),h11=add(t3,-t2),h10=add(add(h11,-t2),t);return add(add(add(0,add(-twice,three)),mul(tangent0,h10)),mul(tangent1,h11));}
static gh_id_error curve(const unsigned char*keys,int n,uint16_t time,float*out){int t=(int16_t)time,index=-1,j,delta;float amount,from;
 if(half(keys+8)<t){index=n;if(n>1){index=0;if(half(keys+20)<t){index=n;for(j=2;j<n;j++)if(t<=half(keys+(size_t)j*12+8)){index=j-1;break;}}}else index=1;}
 if(index<0){*out=0;return GH_ID_OK;}if(index>=n-1)return GH_ID_BOUNDS;
 keys+=(size_t)index*12;delta=half(keys+20)-half(keys+8);if(!delta)return GH_ID_BOUNDS;
 amount=divide((float)(t-half(keys+8)),(float)delta);amount=hermite(amount,mul((float)half(keys+6),1.0f/4096),mul((float)half(keys+16),1.0f/4096));from=scalar(keys);*out=add(from,mul(add(scalar(keys+12),-from),amount));return isfinite(*out)?GH_ID_OK:GH_ID_UNSUPPORTED;
}
static float catmull(float t,float p0,float p1,float p2,float p3){float t2=mul(t,t),t3=mul(t2,t),basis[4];
 /* Original44E050 matrix columns via30A2B0 VU ACC, explicitbinary32 profile. */
 basis[0]=add(add(add(mul(-0.5f,t3),mul(1,t2)),mul(-0.5f,t)),0);
 basis[1]=add(add(add(mul(1.5f,t3),mul(-2.5f,t2)),mul(0,t)),1);
 basis[2]=add(add(add(mul(-1.5f,t3),mul(2,t2)),mul(0.5f,t)),0);
 basis[3]=add(add(add(mul(0.5f,t3),mul(-0.5f,t2)),mul(0,t)),0);
 return add(add(add(mul(p0,basis[0]),mul(p1,basis[1])),mul(p2,basis[2])),mul(p3,basis[3]));
}
static gh_id_error path(gh_id_base*b,gh_id_entry*e,float sample,const gh_id_animation_profile*profile,gh_id_frontier*f,int*changed){
 const unsigned char*s=e->source,*p=e->parts[0];int n=half(s+0x6e),segment=1,i,mode=s[0x67];float cumulative[32],distance=0,t,x,y;uint32_t address=mode==0?0x2ac6e0:0x2ac878;
 *changed=0;if(sample>1)return GH_ID_OK;if(mode>1)return GH_ID_OK;if(n<4)return GH_ID_OK;if(n>17||p!=s+0x130)return GH_ID_BOUNDS;
 if(!profile||!profile->has_distance_seed)return fail(b,e,f,address,GH_ID_UNSUPPORTED);
 if(!isfinite(profile->distance_seed)||!isfinite(e->path_length))return GH_ID_UNSUPPORTED;
 cumulative[0]=profile->distance_seed;distance=mul(sample,e->path_length);
 for(i=1;i<n-2;i++){float dx=fabsf(add(scalar(p+(size_t)i*8),-scalar(p+(size_t)(i+1)*8))),dy=fabsf(add(scalar(p+(size_t)i*8+4),-scalar(p+(size_t)(i+1)*8+4)));float squared=add(mul(dx,dx),mul(dy,dy));
  if(!isfinite(squared)||squared<0)return GH_ID_UNSUPPORTED;cumulative[i]=add(sqrtf(squared),cumulative[i-1]);segment=i;if(distance<cumulative[i])break;segment=i+1;
 }
 /* Originalfallthrough can reference uninitialized next cumulative slot when
  * distance reaches/passes the final segment; retain that separate frontier. */
 if(segment>=n-2)return fail(b,e,f,address,GH_ID_UNSUPPORTED);
 if(!isfinite(cumulative[segment])||cumulative[segment]==cumulative[segment-1])return GH_ID_UNSUPPORTED;
 t=divide(add(distance,-cumulative[segment-1]),add(cumulative[segment],-cumulative[segment-1]));p+=(size_t)segment*8;
 if(mode==1){x=add(scalar(p),mul(add(scalar(p+8),-scalar(p)),t));y=add(scalar(p+4),mul(add(scalar(p+12),-scalar(p+4)),t));}
 else{x=catmull(t,scalar(p-8),scalar(p),scalar(p+8),scalar(p+16));y=catmull(t,scalar(p-4),scalar(p+4),scalar(p+12),scalar(p+20));}
 if(!isfinite(x)||!isfinite(y))return GH_ID_UNSUPPORTED;e->pos_x=add(scalar(s+8),add(x,-256));e->pos_y=add(scalar(s+12),add(y,-224));*changed=1;return GH_ID_OK;
}
static gh_id_error uv(gh_id_base*b,gh_id_entry*e){const unsigned char*s=e->source;unsigned time=(uint16_t)e->jitter[0],rate=s[0x69],w=s[0x6a],h=s[0x6b],mode=s[0x68],frames=w*h;int index,base;
 if(!rate||!w||!h||frames>1024||mode>3)return GH_ID_UNSUPPORTED;
 if(time%rate==0){b->b1d=0;base=(int)((time/rate)%(frames*frames));index=base;
  if((int)frames<=base)index=((int)frames-1)*2-(int)time;
  if(mode==0)index=base;else if(mode==2)index=(int)frames-(base+1);else if(mode==3)index=(int)frames-(index+1);
  e->f70[0]=divide((float)(index%(int)w),(float)w);e->f70[2]=divide((float)(index%(int)w+1),(float)w);index=(index/(int)w)%(int)h;e->f70[1]=divide((float)index,(float)h);e->f70[3]=divide((float)(index+1),(float)h);
  if(((mode==0||mode==2)&&base>=(int)frames)||((mode==1||mode==3)&&base>=(int)frames*2-1)){if(word(s+4)&0x100000)e->flags&=~UINT32_C(0x4000);else e->flags|=0x4000;}
 }if(!(e->flags&0x20))tick(e,0);return GH_ID_OK;
}
static gh_id_error animate(gh_id_base*b,gh_id_entry*e,unsigned gate,const gh_id_animation_profile*profile,gh_id_frontier*f){
 static const uint32_t source[]={0x2000000,0x1000000,0x800000,0x400000},done[]={0x4000,0x2000,0x1000,0x800},loop[]={0x100000,0x80000,0x40000,0x20000};
 static const unsigned hold[]={0x5c,0x5d,0x5e,0x5f},count[]={0,0x6c,0x1b0,0x214},keys[]={0,0x70,0x1b4,0x218};
 const unsigned char*s=e->source;uint16_t time;int n,changed;float value;gh_id_error error;
 if(!(word(s+4)&source[gate])||(e->flags&done[gate])||s[hold[gate]])return GH_ID_OK;
 if(gate==0)return uv(b,e);n=half(s+count[gate]);if(n<1||n>(gate==1?16:8)||e->parts[gate]!=s+keys[gate])return GH_ID_BOUNDS;
 /* Actual2AC298 samples jitter[2] but incrementsjitter[3]; do not silentlyfix. */
 time=(uint16_t)e->jitter[gate==3?2:gate];
 if(half(s+keys[gate]+(size_t)(n-1)*12+8)<(int)time){if(word(s+4)&loop[gate]){time=0;e->jitter[gate==3?2:gate]=0;}else{e->flags|=done[gate];return GH_ID_OK;}}
 error=curve(s+keys[gate],n,time,&value);if(error)return error;
 if(gate==1){error=path(b,e,value,profile,f,&changed);if(error)return error;}
 else if(gate==2)e->scale_x=e->scale_y=value;else e->rotation_speed=mul(value,360);
 if(!(e->flags&0x20))tick(e,gate);return GH_ID_OK;
}
gh_id_error gh_id_animation_reference(gh_id_base*b,gh_id_entry*e,unsigned gate,const gh_id_animation_profile*profile,gh_id_frontier*f){fenv_t env;gh_id_error error;uintptr_t a,p,source,base;
 if(!b||!e||gate>3||!b->entries)return GH_ID_ARGUMENT;a=(uintptr_t)b->entries;p=(uintptr_t)e;
 if(p<a||p-a>=(size_t)b->entry_count*sizeof(*e)||(p-a)%sizeof(*e))return GH_ID_BOUNDS;
 source=(uintptr_t)e->source;base=(uintptr_t)b->source_records.bytes;if(!source||source<base||source-base>b->source_records.size||b->source_records.size-(size_t)(source-base)<0x2e0)return GH_ID_BOUNDS;
 if(fegetenv(&env))return GH_ID_UNSUPPORTED;if(fesetround(FE_TONEAREST)){fesetenv(&env);return GH_ID_UNSUPPORTED;}error=animate(b,e,gate,profile,f);if(fesetenv(&env))return GH_ID_UNSUPPORTED;return error;
}
