#include "game_esp_create.h"
#include "game_esp14.h"
#include "game_effects_frame.h"
#include "game_effect_exp.h"
#include "game_effect_node.h"
#include "game_ray_query.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
static uint32_t w(const void*p){uint32_t n;memcpy(&n,p,4);return n;}
static unsigned h(const uint8_t*p){return p[0]|(unsigned)p[1]<<8;}
static void put(void*p,uint32_t n){memcpy(p,&n,4);}
static void half(void*p,uint16_t n){memcpy(p,&n,2);}
static gh_effect_status fail(gh_esp_create_call*c,uint32_t f,gh_effect_status e){c->frontier=f;return e;}
static int same(const gh_esp_create_bindings*a,const gh_esp_create_bindings*b){return a->storage.effects==b->storage.effects&&a->storage.esp==b->storage.esp&&a->storage.emitters==b->storage.emitters&&a->storage.esp_bytes==b->storage.esp_bytes&&a->storage.emitter_bytes==b->storage.emitter_bytes&&a->random==b->random&&a->work==b->work&&a->policy==b->policy&&a->globals==b->globals&&a->model_effect==b->model_effect&&a->camera==b->camera&&a->active_camera==b->active_camera&&(a->actors==b->actors||(!a->actors&&b->actors))&&(a->atan_coeff==b->atan_coeff||(!a->atan_coeff&&b->atan_coeff));}
gh_effect_status gh_esp_create_begin_00300e30(gh_esp_create_call*c,unsigned emitter,uint32_t serial){if(!c||emitter>=256||!serial||(c->active&&!c->complete))return GH_EFFECT_ARGUMENT;*c=(gh_esp_create_call){.identity=c,.emitter_slot=emitter,.emitter_serial=serial,.active=1};return GH_EFFECT_OK;}
/* Actual3060B0 uses signed bank and unsigned byte item; <count, not<=.
 * Pointer fields remain removed guesttokens, while metadata borrows the exact
 * immutable bounded table entry. The font specialbank48 is still strict. */
static gh_effect_status texture(gh_esp_create_call*c,int16_t bank,unsigned index,gh_effect_view*out){
 *out=(gh_effect_view){0};if(bank==0x48)return fail(c,0x3060b0,GH_EFFECT_UNSUPPORTED);gh_game_effects*e=c->b.storage.effects;gh_effect_resource*r=NULL;for(unsigned i=0;i<64;i++)if(e->resources[i].kind==bank){r=&e->resources[i];break;}if(!r||!r->tbl0.bytes)return GH_EFFECT_OK;gh_effect_view table=r->tbl0;if(table.size<4)return fail(c,0x3060b0,GH_EFFECT_BOUNDS);uint32_t count=w(table.bytes);if(count>(table.size-4)/4)return fail(c,0x3060b0,GH_EFFECT_BOUNDS);if(index>=count)return GH_EFFECT_OK;uint32_t offset=w(table.bytes+4+(size_t)index*4);if(!offset)return GH_EFFECT_OK;if(offset>=table.size)return fail(c,0x3060b0,GH_EFFECT_BOUNDS);size_t end=table.size;for(unsigned i=0;i<count;i++){uint32_t next=w(table.bytes+4+(size_t)i*4);if(next>offset&&next<end)end=next;}*out=(gh_effect_view){table.bytes+offset,end-offset};return GH_EFFECT_OK;
}
static void flag(uint32_t*value,uint32_t mask,int set){*value=set?*value|mask:*value&~mask;}
static uint32_t random_next(uint8_t*p){uint32_t next=w(p+0x220)*UINT32_C(0x19660d)+UINT32_C(0x3c6ef35f);put(p+0x220,next);return next;}
static float random_signed(uint8_t*p){uint32_t bits=(random_next(p)&0x7fffffu)|0x3f800000u;float value;memcpy(&value,&bits,4);volatile float twice=value+value,zero=twice-2.0f,result=zero-1.0f;return result;}
static float mul(float a,float b){volatile float value=a*b;return value;}
static float add(float a,float b){volatile float value=a+b;return value;}
static float floating(const uint8_t*p){float n;memcpy(&n,p,4);return n;}
static void store_float(uint8_t*p,float n){memcpy(p,&n,4);}
static gh_effect_status source_flags(gh_esp_create_call*c,uint8_t*p,const uint8_t*r,const uint8_t*source){
 /* One atomic native continuation stage; source mutation repair cannot repeat
  * random draws after a later validated-domain rejection. */
 unsigned frames=(unsigned)r[0x12c]*r[0x12d];uint32_t first=w(r),second=w(r+4);if((first&0x1000)&&!frames)return fail(c,0x30142c,GH_EFFECT_UNSUPPORTED);
 uint8_t next[0x400];memcpy(next,p,sizeof(next));half(next+0x23a,(uint16_t)h(source+0x18c));next[0x246]=r[0x12d];next[0x245]=r[0x12c];put(next+0x214,w(r+0x10));put(next+0x218,w(r+0x14));uint32_t bits=w(next+0x11c);
 if((first&0x200)||(int8_t)r[0xbe]==-4){bits&=0xfe7fffffu;put(next+0x114,0);}else {put(next+0x210,w(r+8));flag(&bits,0x80000000u,first&0x80000);flag(&bits,0x40000000u,first&0x40000);flag(&bits,0x20000000u,first&0x20000);bits|=0x1000000u;}
 flag(&bits,0x100000u,first&0x100);flag(&bits,0x80000u,first&8);flag(&bits,0x1000u,first&1);flag(&bits,0x800u,second&0x4000000u);flag(&bits,0x400u,second&0x2000000u);flag(&bits,0x200u,second&0x1000000u);if(!(second&0x2000u))flag(&bits,0x100u,w(source+0x114)&0x1000000u);if(w(source+0x1e8)&0x40u)bits&=~0x100u;flag(&bits,0x80u,second&0x80000u);
 flag(&bits,0x10000000u,(first&0x800000u)?(random_next(next)&1):(first&0x4000));flag(&bits,0x8000000u,(first&0x400000u)?(random_next(next)&1):(first&0x2000));next[0x103]=r[0xbf];flag(&bits,0x4000000u,first&0x200000);flag(&bits,0x2000000u,first&0x100000);put(next+0x11c,bits);next[0x23f]=next[0x23e]=r[0x130];store_float(next+0x240,(float)r[0x130]);
 if(first&0x1000)next[0x244]=(uint8_t)(random_next(next)%frames);else {int index=(int8_t)r[0x12f];next[0x244]=(uint8_t)(index<0||index>=(int)frames?0:index);}next[0x2a0]=(int8_t)r[0x2c9]>0?r[0x2c9]:(uint8_t)frames;uint16_t mode=w(source+0x1e4)?(uint16_t)h(source+0x1e4):(uint16_t)h(r+0xc);half(next+0x238,mode?mode:0xffffu);next[0x140]=r[0x290];memcpy(p,next,sizeof(next));return GH_EFFECT_OK;
}
static gh_effect_status initial_scale(gh_esp_create_call*c,uint8_t*p,const uint8_t*r,const uint8_t*source){
 if(r[0x290]!=2)return fail(c,0x301648,GH_EFFECT_UNSUPPORTED);if(floating(source+0x1e0)!=1.0f)return fail(c,0x301554,GH_EFFECT_UNSUPPORTED);for(unsigned offset=0x18;offset<=0x34;offset+=4)if(!isfinite(floating(r+offset)))return fail(c,0x3014b4,GH_EFFECT_UNSUPPORTED);
 uint8_t next[0x400];memcpy(next,p,sizeof(next));float x=add(floating(r+0x18),mul(floating(r+0x20),random_signed(next))),y=add(floating(r+0x1c),mul(floating(r+0x24),random_signed(next)));if(!isfinite(x)||!isfinite(y))return fail(c,0x3014b4,GH_EFFECT_UNSUPPORTED);store_float(next+0x144,x);store_float(next+0x148,y);memcpy(next+0x14c,r+0x28,16);memcpy(next+0x15c,r+0x38,8);half(next+0x164,0);half(next+0x166,0);if(r[1]&0x80u){y=x;store_float(next+0x148,y);}if(x==0&&y==0){uint32_t tiny=0x3727c5ac;memcpy(&x,&tiny,4);y=x;}
 /* 2B61D8 invalidates actual local/world cache before XYZ copy; W unchanged. */
 if(w(next+8)||w(next+0x10))return fail(c,0x2b6370,GH_EFFECT_UNSUPPORTED);next[0x16]=0;put(next+0x90,w(next+0x90)&~1u);store_float(next+0x20,x);store_float(next+0x24,y);put(next+0x28,0x3f800000u);memcpy(p,next,sizeof(next));return GH_EFFECT_OK;
}
static float random_range(uint8_t*p,float minimum,float maximum){float half=mul(add(maximum,-minimum),0.5f),midpoint=add(minimum,half);float product=mul(half,random_signed(p));return add(product,midpoint);}
static gh_effect_status initial_transform(gh_esp_create_call*c,uint8_t*p,const uint8_t*r,const uint8_t*source){
 if(r[0x291]>1||r[0x292]>1)return fail(c,0x30187c,GH_EFFECT_UNSUPPORTED);int part=(int8_t)r[0xbe],override=(int8_t)source[0x1ec];if(override>=-3&&override!=-1)part=override;/* With source bit 23 set, screen-space rows keep their authored origin. */if(part==-4&&!(w(r+4)&0x800000u))return fail(c,0x301bf8,GH_EFFECT_UNSUPPORTED);
 for(unsigned offset=0x48;offset<=0x90;offset+=4)if(!isfinite(floating(r+offset)))return fail(c,0x30187c,GH_EFFECT_UNSUPPORTED);if(floating(source+0x1e0)!=1.0f)return fail(c,0x301cf4,GH_EFFECT_UNSUPPORTED);if(w(p+8)||w(p+0x10))return fail(c,0x2b6370,GH_EFFECT_UNSUPPORTED);
 uint8_t next[0x400];memcpy(next,p,sizeof(next));float angles[3],velocity[3],position[3];memcpy(angles,r+0x48,12);memcpy(velocity,r+0x54,12);memcpy(position,r+0x7c,12);uint32_t first=w(r);int mirrored=(source[0x1e8]&0x80u)!=0;
 if(!r[0x292]){float variation=random_range(next,floating(r+0x64),floating(r+0x68));for(unsigned axis=0;axis<3;axis++)if(first&(0x40u>>axis))angles[axis]=add(angles[axis],variation);}else {for(unsigned axis=0;axis<3;axis++)angles[axis]=add(angles[axis],mul(random_signed(next),floating(r+0x64+axis*4)));for(unsigned axis=0;axis<3;axis++)velocity[axis]=add(velocity[axis],mul(random_signed(next),floating(r+0x70+axis*4)));}
 if(mirrored){velocity[1]=-velocity[1];velocity[2]=-velocity[2];angles[1]=-angles[1];angles[2]=-angles[2];}
 if(!r[0x291]){for(unsigned axis=0;axis<3;axis++)if(first&(UINT32_C(0x80000000)>>axis))position[axis]=add(position[axis],random_range(next,floating(r+0x88),floating(r+0x8c)));}else for(unsigned axis=0;axis<3;axis++)position[axis]=add(position[axis],mul(random_signed(next),floating(r+0x88+axis*4)));
 if(w(r+4)&0x200000u){uint32_t f=0;gh_effect_status status=gh_esp14_position_init_00301c20(next,(gh_effect_view){r,0x2f0},mirrored,position,&f);if(status)return fail(c,f?f:0x301c20,status);}
 else if(mirrored){position[0]=-position[0];store_float(next+0x26c,-floating(next+0x26c));}for(unsigned axis=0;axis<3;axis++)if(!isfinite(angles[axis])||!isfinite(velocity[axis])||!isfinite(position[axis]))return fail(c,0x301d80,GH_EFFECT_UNSUPPORTED);
 memcpy(next+0x178,velocity,12);next[0x16]=0;put(next+0x90,w(next+0x90)&~1u);memcpy(next+0x30,angles,12);memcpy(next+0x40,position,12);memcpy(p,next,sizeof(next));return GH_EFFECT_OK;
}
static gh_effect_status initial_direction(gh_esp_create_call*c,uint8_t*p,const uint8_t*r,const uint8_t*source){
 unsigned mode=r[0x28f]<2?r[0x28f]:0;
 if(mode){for(unsigned offset=0x94;offset<=0xb8;offset+=4)if(!isfinite(floating(r+offset)))return fail(c,0x302060,GH_EFFECT_UNSUPPORTED);if(floating(source+0x1e0)!=1.0f)return fail(c,0x302060,GH_EFFECT_UNSUPPORTED);uint8_t next[0x400];memcpy(next,p,sizeof(next));next[0x141]=1;const unsigned variation[]={0xa0,0xa8,0xac};for(unsigned axis=0;axis<3;axis++){float v=add(floating(r+0x94+axis*4),mul(floating(r+variation[axis]),random_signed(next)));if(!isfinite(v))return fail(c,0x302060,GH_EFFECT_UNSUPPORTED);if(!axis&&(source[0x1e8]&0x80u))v=-v;store_float(next+0x124+axis*4,v);}put(next+0x130,w(r+0xa4));float x=floating(r+0xb0);if(source[0x1e8]&0x80u)x=-x;store_float(next+0x134,x);memcpy(next+0x138,r+0xb4,8);memcpy(p,next,sizeof(next));return GH_EFFECT_OK;}

 /* Actual R100 first ambient: zero input direction AND zero perturbation.
  * Both original101E28 lengths are genuinely0; the <=0 branches retain
  * initializedVF0.xyz. Random calls still occur, even multiplying zero. */
 for(unsigned axis=0;axis<3;axis++)if(floating(r+0x94+axis*4)!=0)return fail(c,0x101e28,GH_EFFECT_UNSUPPORTED);if(floating(r+0xac)!=0)return fail(c,0x101e28,GH_EFFECT_UNSUPPORTED);
 for(unsigned offset=0xa0;offset<=0xb0;offset+=4)if(!isfinite(floating(r+offset)))return fail(c,0x300ef8,GH_EFFECT_UNSUPPORTED);
 if(floating(source+0x1e0)!=1.0f)return fail(c,0x300ef8,GH_EFFECT_UNSUPPORTED);
 uint8_t next[0x400];memcpy(next,p,sizeof(next));next[0x141]=(uint8_t)mode;uint32_t first=w(r);for(unsigned axis=0;axis<3;axis++)if(first&(UINT32_C(0x10000000)>>axis))(void)random_signed(next);float speed=add(floating(r+0xa0),mul(floating(r+0xb0),random_signed(next)));if(!isfinite(speed))return fail(c,0x300ef8,GH_EFFECT_UNSUPPORTED);for(unsigned axis=0;axis<3;axis++)store_float(next+0x124+axis*4,mul(0.0f,speed));if(source[0x1e8]&0x80u)store_float(next+0x124,-floating(next+0x124));put(next+0x130,w(r+0xa4));store_float(next+0x138,floating(r+0xa8));memcpy(p,next,sizeof(next));return GH_EFFECT_OK;
}
static float random_unit(uint8_t*p){uint32_t bits=(random_next(p)&0x7fffffu)|0x3f800000u;float n;memcpy(&n,&bits,4);return add(n,-1.0f);}
static gh_effect_status initial_colour(gh_esp_create_call*c,uint8_t*p,const uint8_t*r,const uint8_t*source){
 unsigned mode=r[0x293];if(mode>1)return fail(c,0x302150,GH_EFFECT_UNSUPPORTED);for(unsigned j=0;j<4;j++)if(floating(source+0x1d0+j*4)!=1.0f)return fail(c,0x30221c,GH_EFFECT_UNSUPPORTED);if(w(r+4)&0x4000u)return fail(c,0x3022a4,GH_EFFECT_UNSUPPORTED);
 float base[4];for(unsigned j=0;j<3;j++)base[j]=floating(r+(mode?0xc4:0xe8)+j*4);base[3]=floating(r+(mode?0xd0:0xc8));float variation=floating(r+0x124);for(unsigned j=0;j<4;j++)if(!isfinite(base[j]))return fail(c,0x302150,GH_EFFECT_UNSUPPORTED);if(!isfinite(variation))return fail(c,0x3022d8,GH_EFFECT_UNSUPPORTED);
 uint8_t next[0x400];memcpy(next,p,sizeof(next));next[0x187]=(uint8_t)mode;float result[4];memcpy(result,base,sizeof(result));if(!mode){float random[4];for(unsigned j=0;j<3;j++)random[j]=random_unit(next);random[3]=1.0f;float inverse=add(1.0f,-variation);for(unsigned j=0;j<4;j++){result[j]=add(mul(random[j],variation),mul(base[j],inverse));if(!isfinite(result[j]))return fail(c,0x3022d8,GH_EFFECT_UNSUPPORTED);}store_float(next+0x208,variation);memcpy(next+0x1f8,result,16);}else put(next+0x208,0);memcpy(next+0x188,result,16);
 if(!mode){memcpy(next+0x198,r+0xc4,0x60);memcpy(next+0x1bc,result,12);}else memcpy(next+0x198,r+0xc4,0x30);memcpy(p,next,sizeof(next));return GH_EFFECT_OK;
}
static gh_effect_status finish_source(gh_esp_create_call*c,uint8_t*p,const uint8_t*r){
 if(w(p+0x114))return fail(c,0x30273c,GH_EFFECT_UNSUPPORTED);if(!isfinite(floating(r+0x138)))return fail(c,0x302684,GH_EFFECT_UNSUPPORTED);uint8_t next[0x400];memcpy(next,p,sizeof(next));next[0x247]=r[0x129];next[0x24a]=r[0x134];next[0x24b]=r[0x135];half(next+0x276,(uint16_t)h(r+0x2ca));put(next+0x254,w(r+0x138)^0x80000000u);if(w(r)&0x1000000u){if(random_signed(next)>0)for(unsigned j=0;j<3;j++)store_float(next+0x178+j*4,-floating(next+0x178+j*4));}memcpy(p,next,sizeof(next));return GH_EFFECT_OK;
}
static float divide(float a,float b){volatile float n=a/b;return n;}
static gh_effect_status initialize_type6(gh_esp_create_call*c,uint8_t*p){
 gh_effects_frame_globals*g=c->b.globals;if(!g||!g->initialized)return fail(c,0x2e8038,GH_EFFECT_ARGUMENT);gh_effect_status status=gh_effect_create_bind_frame_globals(c->b.storage.effects,g);if(status)return fail(c,0x2e8038,status);
 unsigned count=w(p+0x2bc);if(!count||count>32)return fail(c,0x2e8038,GH_EFFECT_ARGUMENT);float radius=floating(p+0x2b4),variance=radius==0?25.0f:mul(radius,radius);if(!(variance>0)||!isfinite(variance))return fail(c,0x2e8038,GH_EFFECT_UNSUPPORTED);
 float spacing=floating(p+0x2b8);if(spacing==0)spacing=1.0f;if(!isfinite(spacing))return fail(c,0x2e8168,GH_EFFECT_UNSUPPORTED);
 /* Original pure synchronous numerical calls have no yield. Compute bounded
  * complete output before publishing to shared arrays; invalid domains never
  * expose a half-normalized kernel. Preserve all original unused tailwords. */
 uint32_t weights[32],offsets[32];float sum=0,distance=0;offsets[0]=0;
 for(unsigned i=0;i<count;i++){float exponent=divide(mul((float)(i*i),-.5f),variance),weight;uint32_t frontier=0;status=gh_effect_exp_reference_003a1c78(exponent,&weight,c->b.policy,&frontier);if(status)return fail(c,frontier?frontier:0x3a1c78,status);memcpy(weights+i,&weight,4);sum=add(sum,i?add(weight,weight):weight);}
 if(!(sum>0)||!isfinite(sum))return fail(c,0x2e8038,GH_EFFECT_UNSUPPORTED);for(unsigned i=0;i<count;i++){float weight;memcpy(&weight,weights+i,4);weight=divide(weight,sum);if(!isfinite(weight))return fail(c,0x2e8038,GH_EFFECT_UNSUPPORTED);memcpy(weights+i,&weight,4);}
 for(unsigned i=1;i<count;i++){float signed_distance=(i&1)?distance:-distance;if(!isfinite(signed_distance)||signed_distance < -2147483648.0f||signed_distance>=2147483648.0f)return fail(c,0x2e8168,GH_EFFECT_UNSUPPORTED);int32_t integer=(int32_t)signed_distance;memcpy(offsets+i,&integer,4);if(i&1)distance=add(distance,spacing);}
 memcpy(g->gaussian_weights_61b5c0,weights,count*4);memcpy(g->gaussian_offsets_61b540,offsets,count*4);return GH_EFFECT_OK;
}
/* 2F00E0 initializes the framebuffer-sampling offset and oscillation data. */
static gh_effect_status initialize_12(gh_esp_create_call*c,uint8_t*p,const uint8_t*r){
 if(w(p+0xf0)!=0x44eb70u)return fail(c,0x2f00e0,GH_EFFECT_UNSUPPORTED);
 if(r[0x18c]>2)return fail(c,0x2d9f68,GH_EFFECT_UNSUPPORTED);
 if((int8_t)r[0xbe]!=-4||!(w(r+4)&0x800000u)||w(p+0x114))
    return fail(c,0x2f00e0,GH_EFFECT_UNSUPPORTED);

 const unsigned offsets[]={0x2b0,0x2b4,0x2d0,0x2c8,0x2cc,0x2dc};
 float values[6];
 for(unsigned i=0;i<6;i++){
    values[i]=mul(floating(r+0x13c+i*4),i<3?0.01f:0.1f);
    if(!isfinite(values[i]))return fail(c,0x2f00e0,GH_EFFECT_UNSUPPORTED);
 }
 gh_effect_status status=gh_effect_create_esp_commit_12(c->b.storage.effects,c->slot,c->serial);
 if(status)return fail(c,0x2f00e0,status);
 for(unsigned i=0;i<6;i++)store_float(p+offsets[i],values[i]);
 memcpy(p+0x2e0,r+0x18c,3);
 store_float(p+0x2b8,values[0]);
 store_float(p+0x2d4,values[2]);
 store_float(p+0x2bc,values[1]);
 c->stage=12;
 c->complete=1;
 return GH_EFFECT_OK;
}
static gh_effect_status initialize_13(gh_esp_create_call*c,uint8_t*p,const uint8_t*r){
 if(w(p+0xf0)!=0x44ec68u||r[0x16c]!=0||r[0x170]!=12||w(p+0x114))
    return fail(c,0x2f11f0,GH_EFFECT_UNSUPPORTED);
 gh_effects_frame_globals*g=c->b.globals;
 put(p+0x2b0,r[0x16c]);
 put(p+0x2b4,r[0x170]);
 g->flare_setup_61b674=w(r+0x174);
 gh_effect_view est;
 gh_effect_status status=gh_effect_est_at_00306030(c->b.storage.effects,r[0x16c],r[0x170]+1u,&est);
 if(status||!est.bytes)return fail(c,0x306030,status?status:GH_EFFECT_UNSUPPORTED);
 if(w(est.bytes)!=0x545345u)return fail(c,0x2f1258,GH_EFFECT_UNSUPPORTED);
 status=gh_effect_create_esp_commit_13(c->b.storage.effects,c->slot,c->serial);
 if(status)return fail(c,0x2f11f0,status);
 g->flare_hold_3c3bb0=0;
 g->flare_intensity_3c3bb4=0;
 g->flare_active_61b670=0;
 g->flare_setup_61b674=0;
 c->complete=1;
 c->stage=12;
 return GH_EFFECT_OK;
}
/* 2F3E68 compares the particle world position to the selected camera eye. */
static gh_effect_status shake_range(gh_esp_create_call*c,const gh_game_pl_camera*camera,float radius,int*near){
 float position[4];
 uint32_t pc=0;
 gh_effect_status status=gh_effect_node_world_position_native(c->b.storage.effects,0,c->slot,c->serial,
     position,c->b.policy,&pc);
 if(status)return fail(c,pc?pc:0x2b6160,status);
 *near=1;
 if(radius==0.0f)return GH_EFFECT_OK;
 float delta[3];
 for(unsigned i=0;i<3;i++)
    delta[i]=mul(add(position[i],-floating(camera->fields+0x210+i*4)),1000.0f);
 float squared=add(add(mul(delta[0],delta[0]),mul(delta[1],delta[1])),mul(delta[2],delta[2]));
 float distance=sqrtf(squared);
 if(!isfinite(distance))return fail(c,0x1f7df8,GH_EFFECT_UNSUPPORTED);
 *near=distance<=radius;
 return GH_EFFECT_OK;
}

/* Cut2 uses two one-shot counters. Continuous (-1) shake stays at 2F40C0. */
static gh_effect_status initialize_18(gh_esp_create_call*c,uint8_t*p,const uint8_t*r){
 gh_game_pl_camera *camera=c->b.camera;
 if(!camera||!camera->constructed||camera->identity!=c->b.active_camera||
    camera->policy!=GH_CAMERA_NATIVE_BINARY32_REFERENCE)
    return fail(c,0x2f3f30,GH_EFFECT_ARGUMENT);
 int horizontal=(int8_t)r[0x18c],vertical=(int8_t)r[0x18d];
 if(w(p+0xf0)!=0x44f078u||horizontal<0||vertical<0||w(p+0x114))
    return fail(c,0x2f3f30,GH_EFFECT_UNSUPPORTED);
 float radius=mul(floating(r+0x15c),1000.0f);
 if(!isfinite(radius)||radius<0.0f)return fail(c,0x2f3e68,GH_EFFECT_UNSUPPORTED);
 for(unsigned i=0;i<8;i++)
    if(!isfinite(floating(r+0x13c+i*4)))return fail(c,0x2f3f30,GH_EFFECT_UNSUPPORTED);
 int near=0;
 gh_effect_status status=shake_range(c,camera,radius,&near);
 if(status)return status;
 status=gh_effect_create_esp_commit_18(c->b.storage.effects,c->slot,c->serial);
 if(status)return fail(c,0x2f3f30,status);
 memcpy(camera->fields+0x268,r+0x13c,16);
 memcpy(camera->fields+0x254,r+0x14c,16);
 store_float(p+0x2b8,radius);
 half(p+0x2b0,(uint16_t)horizontal);
 half(p+0x2b2,(uint16_t)vertical);
 if(near){
    camera->fields[0x278]=(uint8_t)horizontal;
    camera->fields[0x264]=(uint8_t)vertical;
 }
 c->stage=12;
 c->complete=1;
 return GH_EFFECT_OK;
}
static gh_effect_status initialize_1a(gh_esp_create_call*c,uint8_t*p,const uint8_t*r){
 uint32_t frontier=0;gh_effect_status status;
 if(c->stage==10){if(w(p+0xf0)!=0x44f270u)return fail(c,0x2f4be0,GH_EFFECT_UNSUPPORTED);status=gh_effect_node_detach_update_002fcb88(&c->detach,c->b.storage.effects,c->slot,c->serial,c->b.atan_coeff,c->b.policy,&frontier);if(status)return fail(c,frontier?frontier:0x2fcb88,status);if(!c->detach.complete)return fail(c,0x2fcb88,GH_EFFECT_ARGUMENT);c->stage=13;}
 if(c->stage==13){float distance=floating(r+0x144);if(distance==0)distance=10.0f;if(!isfinite(distance)||!isfinite(floating(r+0x140)))return fail(c,0x2f4be0,GH_EFFECT_UNSUPPORTED);status=gh_effect_node_world_position_native(c->b.storage.effects,0,c->slot,c->serial,c->ground_position,c->b.policy,&frontier);if(status)return fail(c,frontier,status);memcpy(c->ground_from,c->ground_position,16);memcpy(c->ground_to,c->ground_position,16);c->ground_from[1]=add(c->ground_from[1],1.0f);c->ground_to[1]=add(c->ground_to[1],-add(distance,1.0f));if(!isfinite(c->ground_from[1])||!isfinite(c->ground_to[1]))return fail(c,0x2f4c88,GH_EFFECT_UNSUPPORTED);memset(c->ground_hit,0,12);c->ground_hit[3]=1;c->stage=14;}
 if(c->stage==14){if(!c->b.actors)return fail(c,0x131e10,GH_EFFECT_UNSUPPORTED);gh_scene_ray_request request={.mask=2,.flags=0,.hit_position=c->ground_hit,.report_limit=0,.segmented=1};memcpy(request.from,c->ground_from,16);memcpy(request.to,c->ground_to,16);int hit=0;gh_scene_ray_status ray=gh_scene_ray_query_00131e10(c->b.actors,&request,&hit,&frontier);if(ray)return fail(c,frontier?frontier:0x131e10,ray==GH_RAY_BOUNDS?GH_EFFECT_BOUNDS:ray==GH_RAY_ARGUMENT?GH_EFFECT_ARGUMENT:GH_EFFECT_UNSUPPORTED);c->ground_ray_hit=hit;c->stage=15;}
 if(c->stage==15){float position[3];memcpy(position,c->ground_ray_hit==1?c->ground_hit:c->ground_position,12);if(c->ground_ray_hit==1)position[1]=add(position[1],floating(r+0x140));status=gh_effect_node_set_position_native(c->b.storage.effects,0,c->slot,c->serial,position,&frontier);if(status)return fail(c,frontier,status);c->stage=16;}
 if(c->stage==16){status=gh_effect_create_esp_commit_1a(c->b.storage.effects,c->slot,c->serial);if(status)return fail(c,0x2f4be0,status);c->complete=1;c->stage=17;return GH_EFFECT_OK;}
 return fail(c,0x2f4be0,GH_EFFECT_ARGUMENT);
}
static gh_effect_status create_body(gh_esp_create_call*c,const gh_esp_create_bindings*b){
 if(!c||!b||!c->active||c->identity!=c||!b->work)return GH_EFFECT_ARGUMENT;if(b->policy!=GH_STATIC_BINARY32_REFERENCE)return fail(c,0x300ef8,GH_EFFECT_UNSUPPORTED);if(c->stage&&!same(&c->b,b))return fail(c,0x300e30,GH_EFFECT_ARGUMENT);if(c->stage&&!c->b.actors&&b->actors)c->b.actors=b->actors;if(c->stage&&!c->b.atan_coeff&&b->atan_coeff)c->b.atan_coeff=b->atan_coeff;c->frontier=0;if(c->complete)return GH_EFFECT_OK;if(!c->stage)c->b=*b;
 gh_effect_emitter_view emitter;gh_effect_status status=gh_effect_create_emitter_at(b->storage.effects,c->emitter_slot,&emitter);if(status||emitter.serial!=c->emitter_serial||!emitter.initialized||emitter.payload!=b->storage.emitters+(size_t)c->emitter_slot*0x260)return fail(c,0x300e30,GH_EFFECT_ARGUMENT);uint8_t*source=emitter.payload;const uint8_t*r=emitter.record.bytes;
 status=gh_effect_create_bind_frame_globals(b->storage.effects,b->globals);if(status)return fail(c,0x2e8038,status);if(r[0x133]==0x1a&&b->actors){status=gh_effect_create_bind_frame_actors(b->storage.effects,b->actors);if(status)return fail(c,0x131e10,status);}
 if(!c->stage){int allocated=0;uint32_t f=0;status=gh_effect_create_esp_allocate_002faa88(&b->storage,c->emitter_slot,c->emitter_serial,&c->slot,&c->serial,&allocated,&f);if(status)return fail(c,f?f:0x2faa88,status);c->allocated=(uint8_t)allocated;if(!allocated){c->complete=1;return GH_EFFECT_OK;}c->stage=1;}
 gh_effect_esp_view view;status=gh_effect_create_esp_at(b->storage.effects,c->slot,&view);if(status||view.serial!=c->serial||view.emitter_slot!=c->emitter_slot||view.emitter_serial!=c->emitter_serial||view.payload!=b->storage.esp+(size_t)c->slot*0x400)return fail(c,0x300ef8,GH_EFFECT_ARGUMENT);uint8_t*p=view.payload;
 if(c->stage==1){uint32_t seed;if(!r[0x28e]){if(!b->random)return fail(c,0x31cfe0,GH_EFFECT_ARGUMENT);if(gh_game_mt_next_0030f348(b->random,&seed))return fail(c,0x31cfe0,GH_EFFECT_ARGUMENT);}else seed=w(source+0x220)*UINT32_C(0x19660d)+UINT32_C(0x3c6ef35f);put(source+0x220,seed);c->stage=2;}
 if(c->stage==2){unsigned remaining=0,duration=h(source+0x224);if(duration){float elapsed;memcpy(&elapsed,source+0x228,4);if(!isfinite(elapsed)||elapsed < -2147483648.0f||elapsed>=2147483648.0f||(float)(int32_t)elapsed!=elapsed)return fail(c,0x300e84,GH_EFFECT_UNSUPPORTED);remaining=(duration-(uint32_t)(int32_t)elapsed)&0xffffu;if(!remaining)remaining=1;}
  /* Original300EF8 arguments9..13 are explicit caller stackstores, not
   * uninitialized locals: cfg118,bank18C,flags114,seed220,remainingduration. */
  put(p+0x220,w(source+0x220));half(p+0x23c,(uint16_t)h(source+0x18e));put(p+0x108,0);put(p+0x114,0);put(p+0x10c,w(source+0x110));put(p+0x120,w(source+0x218));int part=(int8_t)r[0xbe],override=(int8_t)source[0x1ec];if(override>=-3&&override!=-1)part=override;if(part!=-4){status=gh_effect_create_esp_attach_emitter_002b5d58(b->storage.effects,c->slot,c->serial,c->emitter_slot,c->emitter_serial);if(status)return fail(c,0x2b5d58,status);}half(p+0x22c,(uint16_t)remaining);put(p+0x104,w(source+0x118));put(p+0x110,0);p[0x101]=r[0x12a];c->stage=3;
 }
 if(c->stage==3){gh_effect_view first,secondary;status=texture(c,(int16_t)h(r+0x28c),r[0x12e],&first);if(status)return status;if(!first.bytes){status=texture(c,0,0xff,&first);if(status)return status;}secondary=first;if(w(r+4)&0x08000000u){status=texture(c,(int16_t)h(r+0x28c),r[0x297],&secondary);if(status)return status;}status=gh_effect_create_esp_bind_textures(b->storage.effects,c->slot,c->serial,first,secondary);if(status)return fail(c,0x3060b0,status);c->stage=4;}
 if(c->stage==4){status=source_flags(c,p,r,source);if(status)return status;c->stage=5;}
 if(c->stage==5){status=initial_scale(c,p,r,source);if(status)return status;c->stage=6;}
 if(c->stage==6){status=initial_transform(c,p,r,source);if(status)return status;c->stage=7;}
 if(c->stage==7){status=initial_direction(c,p,r,source);if(status)return status;c->stage=8;}
 if(c->stage==8){status=initial_colour(c,p,r,source);if(status)return status;c->stage=9;}
 if(c->stage==9){status=finish_source(c,p,r);if(status)return status;c->stage=10;}
 if(view.type==0&&c->stage==10){
  if(w(p+0xf0)!=0x44fad0u||((w(p+0x11c)&0x01000000u)&&
      !gh_esp14_nested_sprite_is_bound(view.record,view.actor,view.selector)))
      return fail(c,0x2fbec8,GH_EFFECT_UNSUPPORTED);
  status=gh_effect_create_esp_commit_base(b->storage.effects,c->slot,c->serial);
  if(status)return fail(c,0x2fbec8,status);
  c->complete=1;
  c->stage=12;
  return GH_EFFECT_OK;
 }
 if(view.type==0x14&&c->stage==10){
  const gh_esp14_provider *provider=b->model_effect;
  if(!provider||!provider->initialize)return fail(c,0x2f1d58,GH_EFFECT_UNSUPPORTED);
  int original_return=0;
  uint32_t frontier=0;
  status=provider->initialize(provider->context,&view,c->slot,w(source+0x1e0),&original_return,&frontier);
  if(status)return fail(c,frontier?frontier:0x2f1d58,status);
  if(!original_return)return fail(c,0x2fbe50,GH_EFFECT_UNSUPPORTED);
  status=gh_effect_create_esp_commit_14(b->storage.effects,c->slot,c->serial);
  if(status)return fail(c,0x2f1d58,status);
  c->stage=12;
  c->complete=1;
  return GH_EFFECT_OK;
 }
 if(view.type==0x18&&c->stage==10)return initialize_18(c,p,r);
 if(view.type==0x13&&c->stage==10)return initialize_13(c,p,r);
 if(view.type==0x12&&c->stage==10)return initialize_12(c,p,r);
 if(view.type==0x1a&&c->stage>=10)return initialize_1a(c,p,r);
 if(c->stage==10){
  if(w(p+0xf0)!=0x44e6d8u)return fail(c,0x2e8210,GH_EFFECT_UNSUPPORTED);put(p+0x2b0,w(r+0x16c));uint32_t count=w(r+0x170);int32_t signed_count;memcpy(&signed_count,&count,4);if(signed_count<1)count=25;if(count>32)count=32;put(p+0x2bc,count);put(p+0x2b4,w(r+0x13c));put(p+0x2b8,w(r+0x140));c->stage=11;
 }
 if(c->stage==11){status=initialize_type6(c,p);if(status)return status;status=gh_effect_create_esp_commit_type6(b->storage.effects,c->slot,c->serial);if(status)return fail(c,0x2e8210,status);c->stage=12;c->complete=1;return GH_EFFECT_OK;}
 return fail(c,0x2e8210,GH_EFFECT_UNSUPPORTED);
}

gh_effect_status gh_game_esp_create_00300e30(gh_esp_create_call*c,const gh_esp_create_bindings*b){fenv_t env;if(feholdexcept(&env))return GH_EFFECT_UNSUPPORTED;if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return GH_EFFECT_UNSUPPORTED;}gh_effect_status status=create_body(c,b);if(fesetenv(&env))return GH_EFFECT_UNSUPPORTED;return status;}
