#include "game_esp_update.h"
#include "game_effects_frame.h"
#include "game_actor_matrix.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
static uint32_t w(const void*p){uint32_t n;memcpy(&n,p,4);return n;}

static void put(void*p,uint32_t n){memcpy(p,&n,4);}
static float rf(const void*p){float n;memcpy(&n,p,4);return n;}
static void sf(void*p,float n){memcpy(p,&n,4);}
static int sh(const uint8_t*p){uint16_t u=(uint16_t)(p[0]|(unsigned)p[1]<<8);int16_t n;memcpy(&n,&u,2);return n;}
static unsigned uh(const uint8_t*p){return (unsigned)(p[0]|(unsigned)p[1]<<8);}
static void half(uint8_t*p,int n){uint16_t u=(uint16_t)n;p[0]=(uint8_t)u;p[1]=(uint8_t)(u>>8);}
static float add(float a,float b){volatile float n=a+b;return n;}
static float mul(float a,float b){volatile float n=a*b;return n;}
static float divide(float a,float b){volatile float n=a/b;return n;}
static gh_effect_status stop(gh_esp_update_call*c,uint32_t f,gh_effect_status s){c->frontier=f;return s;}
/* New timing domains only admit exact CVT.W.S inputs, pending EE fractional
 * conversion evidence; no host rounding-mode dependent cast is substituted. */
static int integer(float x,int*out){if(!isfinite(x)||x < -2147483648.0f||x>=2147483648.0f)return 0;int n=(int)x;if((float)n!=x)return 0;*out=n;return 1;}
static int same(const gh_esp_update_bindings*a,const gh_esp_update_bindings*b){return a->storage.effects==b->storage.effects&&a->storage.esp==b->storage.esp&&a->storage.emitters==b->storage.emitters&&a->storage.esp_bytes==b->storage.esp_bytes&&a->storage.emitter_bytes==b->storage.emitter_bytes&&a->work==b->work&&a->globals==b->globals&&a->atan_coeff==b->atan_coeff&&a->policy==b->policy;}
static int scaled(const gh_esp_update_call*c,const uint8_t*p,const uint8_t*r){return (!(w(r+4)&0x2000u)&&(w(p+0x11c)&0x100u))||(w((const uint8_t*)c->b.work+0x5e4)&0x20000u);}
static int advance(gh_esp_update_call*c,float old,int scale,float*out){if(!isfinite(old))return 0;if(!scale){*out=add(old,1);return isfinite(*out);}float hundred=mul(old,100);if(!isfinite(hundred)||hundred < -2147483648.0f||hundred>=2147483648.0f)return 0;int whole;if(!integer(hundred,&whole))return 0;float speed=mul(mul(rf(&c->b.globals->factor_741dc0),rf((const uint8_t*)c->b.work+0x574)),100);int delta;if(!integer(speed,&delta))return 0;int64_t total=(int64_t)whole+delta;if(total<INT32_MIN||total>INT32_MAX)return 0;*out=divide((float)(int32_t)total,100);return isfinite(*out);}
static gh_effect_status retire(gh_esp_update_call*c){uint32_t f;gh_effect_status s=gh_effect_create_esp_pause_002fbe50(c->b.storage.effects,c->slot,c->serial,&f);if(s)return stop(c,f?f:0x2fbe50,s);c->retired=1;c->stage=15;return GH_EFFECT_OK;}
static int colour_curve(const uint8_t*p,unsigned base,unsigned channels,int length,float *out){
 float time=rf(p+0x234);
 if(!length||!isfinite(time)||time<0.0f||time>=2147483648.0f)return 0;
 unsigned stride=(channels+1)*4;
 for(unsigned key=0;key<3;key++){
    const uint8_t *first=p+base+key*stride,*next=first+stride;
    float begin=rf(first),end=rf(next);
    float threshold=add(mul(end,(float)length),0.0001f);
    if(!isfinite(threshold)||threshold<-2147483648.0f||threshold>=2147483648.0f)return 0;
    if((int32_t)time>(int32_t)threshold)continue;
    if(!isfinite(begin)||end==begin)return 0;
    float ratio=divide(add(divide(time,(float)length),-begin),add(end,-begin));
    float remaining=add(1.0f,-ratio);
    for(unsigned j=0;j<channels;j++)
       out[j]=add(mul(rf(first+4+j*4),remaining),mul(rf(next+4+j*4),ratio));
    return 1;
 }
 for(unsigned j=0;j<channels;j++)out[j]=1.0f;
 return 1;
}

static gh_effect_status colour(gh_esp_update_call*c,uint8_t*p,const uint8_t*r){
 if(p[0x187]==1){uint32_t fade=w(r+0x2c0),hold=w(r+0x2c4);int elapsed=sh(p+0x22e);if(fade>INT32_MAX||hold>INT32_MAX||elapsed<0)return stop(c,0x2fc1a0,GH_EFFECT_UNSUPPORTED);if(fade<(uint32_t)elapsed){if((uint32_t)elapsed<fade+hold)return GH_EFFECT_OK;float rgba[4];for(unsigned j=0;j<4;j++){rgba[j]=scaled(c,p,r)?add(rf(p+0x188+j*4),-mul(mul(rf(p+0x1c8+j*4),rf(&c->b.globals->factor_741dc0)),rf((const uint8_t*)c->b.work+0x574))):mul(rf(p+0x188+j*4),rf(p+0x1a8+j*4));if(rgba[j]>1)rgba[j]=1;if(!isfinite(rgba[j]))return stop(c,0x2fc1a0,GH_EFFECT_UNSUPPORTED);}if(rgba[3]<0.005f){memcpy(p+0x188,rgba,16);return retire(c);}memcpy(p+0x188,rgba,16);if(!(w(r+4)&0x2000u))for(unsigned j=0;j<4;j++)sf(p+0x1c8+j*4,add(rgba[j],-mul(rgba[j],rf(p+0x1a8+j*4))));return GH_EFFECT_OK;}if(fade){float progress=divide(rf(p+0x234),(float)fade);if(progress>1)progress=1;float alpha=mul(rf(p+0x1a4),progress);if(!isfinite(alpha))return stop(c,0x2fc1a0,GH_EFFECT_UNSUPPORTED);sf(p+0x194,alpha);}return GH_EFFECT_OK;}if(p[0x187]!=0)return stop(c,0x2fc1a0,GH_EFFECT_UNSUPPORTED);int length=sh(p+0x238);if(length<0)return GH_EFFECT_OK;float rgba[4];if(rf(p+0x1b8)==1)memcpy(rgba,p+0x1bc,12);else if(rf(p+0x208)>0)memcpy(rgba,p+0x1f8,12);else if(!colour_curve(p,0x1b8,3,length,rgba))return stop(c,0x2fc1a0,GH_EFFECT_UNSUPPORTED);
 if(rf(p+0x198)==1.0f)rgba[3]=rf(p+0x19c);else if(!colour_curve(p,0x198,1,length,rgba+3))return stop(c,0x2fc644,GH_EFFECT_UNSUPPORTED);for(unsigned j=0;j<4;j++)if(!isfinite(rgba[j]))return stop(c,0x2fc1a0,GH_EFFECT_UNSUPPORTED);if(p[0x186]&1)for(unsigned j=0;j<4;j++)rgba[j]=mul(mul(rgba[j],(float)p[0x20c+j]),0.003921568859368563f);memcpy(p+0x188,rgba,16);(void)r;return GH_EFFECT_OK;
}
static gh_effect_status body(gh_esp_update_call*c,const gh_esp_update_bindings*b,unsigned slot,uint32_t serial){
 if(!c||!b||!b->work||!b->globals||!b->globals->initialized||!b->storage.effects||!b->storage.esp||!b->storage.emitters||b->storage.esp_bytes<0x100180||b->storage.emitter_bytes<0x26060)return GH_EFFECT_ARGUMENT;if(c->identity&&(c->identity!=c||c->slot!=slot||c->serial!=serial||!same(&c->b,b)))return stop(c,0x2fbed0,GH_EFFECT_ARGUMENT);if(b->policy!=GH_STATIC_BINARY32_REFERENCE)return stop(c,0x2fbed0,GH_EFFECT_UNSUPPORTED);c->frontier=0;if(c->complete)return GH_EFFECT_OK;
 gh_effect_esp_view v;if(gh_effect_create_esp_at(b->storage.effects,slot,&v)||v.serial!=serial||!v.initialized||v.globals!=b->globals||v.payload!=b->storage.esp+(size_t)slot*0x400||v.payload_size!=0x400||v.record.size!=0x2f0)return stop(c,0x2fbed0,GH_EFFECT_ARGUMENT);uint8_t*p=v.payload;const uint8_t*r=v.record.bytes;uint32_t vtable=v.type==6?0x44e6d8u:v.type==0x12?0x44eb70u:v.type==0x1a?0x44f270u:0;
 if(!vtable||w(p+0xf0)!=vtable)return stop(c,0x2fbed0,GH_EFFECT_UNSUPPORTED);
 /* 2F01B8 calls the base Move before three optional oscillations. */
 if(v.type==0x12&&(rf(p+0x2c8)!=0.0f||rf(p+0x2cc)!=0.0f||rf(p+0x2dc)!=0.0f))
    return stop(c,0x2f01b8,GH_EFFECT_UNSUPPORTED);
 if(!c->identity){c->identity=c;c->b=*b;c->slot=slot;c->serial=serial;c->stage=1;}
 if(c->stage==1){if(v.actor||v.selector||w(p+0x114))return stop(c,0x2fbed0,GH_EFFECT_UNSUPPORTED);if(v.parent_emitter>=0){gh_effect_emitter_view parent;if(gh_effect_create_emitter_at(b->storage.effects,(unsigned)v.parent_emitter,&parent)||parent.serial!=v.parent_serial)return stop(c,0x2b5e50,GH_EFFECT_ARGUMENT);}c->stage=2;}
 if(c->stage==2){float time;int n;if(!advance(c,rf(p+0x228),scaled(c,p,r),&time)||!integer(time,&n))return stop(c,0x2fa350,GH_EFFECT_UNSUPPORTED);sf(p+0x228,time);half(p+0x224,n);c->stage=3;}
 if(c->stage==3){if(uh(p+0x22c)&&uh(p+0x224)>=uh(p+0x22c)){gh_effect_status status=retire(c);if(status)return status;}else c->stage=4;}
 if(c->stage==4){if(p[0x24c]){float time;int n;if(!advance(c,rf(p+0x250),scaled(c,p,r),&time)||!integer(time,&n))return stop(c,0x2fcac8,GH_EFFECT_UNSUPPORTED);sf(p+0x250,time);p[0x24d]=(uint8_t)n;c->stage=5;}else c->stage=6;}
 if(c->stage==5){if(p[0x24d]>=p[0x24c]){gh_effect_status status=retire(c);if(status)return status;}else c->stage=6;}
 if(c->stage==6){int length=sh(p+0x238);if(length>0&&sh(p+0x22e)>=length){if(!(w(p+0x11c)&0x800u)){gh_effect_status status=retire(c);if(status)return status;}else half(p+0x22e,(w(p+0x11c)&0x400u)?0:length);}if(!c->retired){float time;int n;if(!advance(c,rf(p+0x234),scaled(c,p,r),&time)||!integer(time,&n))return stop(c,0x2fc9a8,GH_EFFECT_UNSUPPORTED);sf(p+0x234,time);half(p+0x22e,n);c->stage=7;}}
 if(c->stage==7){gh_effect_status s=colour(c,p,r);if(s)return s;if(!c->retired)c->stage=8;}
 if(c->stage==8){if(w(p+0x11c)&0x4000000u){float time;int n;if(!advance(c,rf(p+0x240),scaled(c,p,r),&time)||!integer(time,&n))return stop(c,0x2fcf88,GH_EFFECT_UNSUPPORTED);uint8_t frame=p[0x244];if((int8_t)p[0x23f]<=(int8_t)(uint8_t)n){time=0;uint8_t next=(uint8_t)(frame+1);if((int8_t)p[0x2a0]<=(int8_t)next)next=(w(p+0x11c)&0x2000000u)?0:frame;frame=next;}sf(p+0x240,time);p[0x244]=frame;}c->stage=9;}
 if(c->stage==9){if(p[0x140]!=2||(w(p+0x11c)&0x400000u))return stop(c,0x2fd078,GH_EFFECT_UNSUPPORTED);if(scaled(c,p,r)&&sh(p+0x22e)==sh(p+0x230))c->stage=10;else {if(sh(p+0x160)||(!(r[1]&0x80u)&&sh(p+0x162)))return stop(c,0x1f7ef0,GH_EFFECT_UNSUPPORTED);float scale[3]={rf(p+0x144),rf(p+0x148),rf(p+0x28)},dx=rf(p+0x14c),dy=rf(p+0x150);scale[0]=add(scale[0],dx);if(r[1]&0x80u)scale[1]=scale[0];else scale[1]=add(scale[1],dy);float ndx=mul(dx,rf(p+0x154)),ndy=mul(dy,rf(p+0x158));if(!isfinite(ndx)||!isfinite(ndy))return stop(c,0x2fd078,GH_EFFECT_UNSUPPORTED);uint32_t f;gh_effect_status status=gh_effect_node_set_scale_native(b->storage.effects,0,slot,serial,scale,&f);if(status)return stop(c,f,status);sf(p+0x144,scale[0]);sf(p+0x14c,ndx);if(!(r[1]&0x80u)){sf(p+0x148,scale[1]);sf(p+0x150,ndy);}c->stage=10;}}
 if(c->stage==10){float angle[4]={rf(p+0x30),rf(p+0x34),rf(p+0x38),1},out[4]={0,0,0,1};float factor=scaled(c,p,r)?mul(rf(&b->globals->factor_741dc0),rf((const uint8_t*)b->work+0x574)):1;for(unsigned j=0;j<3;j++)angle[j]=add(angle[j],mul(rf(p+0x178+j*4),factor));uint32_t f;if(gh_actor_euler_adjust_001f7da0(angle,out,b->policy,&f))return stop(c,f?f:0x1f7da0,GH_EFFECT_UNSUPPORTED);gh_effect_status status=gh_effect_node_set_euler_native(b->storage.effects,0,slot,serial,out,&f);if(status)return stop(c,f,status);c->stage=11;}
 if(c->stage==11){if(w(r+4)&0x200000u)return stop(c,0x2fd610,GH_EFFECT_UNSUPPORTED);float position[3];float factor=scaled(c,p,r)?mul(rf(&b->globals->factor_741dc0),rf((const uint8_t*)b->work+0x574)):1;for(unsigned j=0;j<3;j++)position[j]=add(rf(p+0x40+j*4),mul(rf(p+0x124+j*4),factor));uint32_t f;gh_effect_status status=gh_effect_node_set_position_native(b->storage.effects,0,slot,serial,position,&f);if(status)return stop(c,f,status);c->stage=12;}
 if(c->stage==12){if(!(scaled(c,p,r)&&sh(p+0x230)==sh(p+0x22e))){float velocity[3];for(unsigned j=0;j<3;j++){velocity[j]=add(rf(p+0x124+j*4),rf(p+0x134+j*4));if(!isfinite(velocity[j]))return stop(c,0x2fc124,GH_EFFECT_UNSUPPORTED);}memcpy(p+0x124,velocity,12);}c->stage=13;}
 if(c->stage==13){if(!(!(w(r+4)&0x2000u)&&(w(p+0x11c)&0x100u)&&sh(p+0x230)==sh(p+0x22e))){float velocity[3];for(unsigned j=0;j<3;j++){velocity[j]=mul(rf(p+0x124+j*4),rf(p+0x130));if(!isfinite(velocity[j]))return stop(c,0x2fdd10,GH_EFFECT_UNSUPPORTED);}memcpy(p+0x124,velocity,12);}c->stage=14;}
 if(c->stage==14){uint32_t f;gh_effect_status status=gh_effect_node_detach_update_002fcb88(&c->detach,b->storage.effects,slot,serial,b->atan_coeff,b->policy,&f);if(status)return stop(c,f,status);if(!c->detach.complete)return stop(c,0x2fcb88,GH_EFFECT_ARGUMENT);half(p+0x230,sh(p+0x22e));c->stage=15;}
 if(c->stage==15){uint32_t flags=w(p+0x11c);if(w(r+4)&0x800u)flags=b->globals->region_active_741db8?flags&~0x800000u:flags|0x800000u;if(w(r+4)&0x400u)flags=b->globals->region_active_741db8?flags|0x800000u:flags&~0x800000u;put(p+0x11c,flags);c->stage=16;c->complete=1;}return GH_EFFECT_OK;
}
gh_effect_status gh_game_esp_update_002fbed0(gh_esp_update_call*c,const gh_esp_update_bindings*b,unsigned slot,uint32_t serial){fenv_t env;if(feholdexcept(&env))return GH_EFFECT_UNSUPPORTED;if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return GH_EFFECT_UNSUPPORTED;}gh_effect_status s=body(c,b,slot,serial);if(fesetenv(&env))return GH_EFFECT_UNSUPPORTED;return s;}
