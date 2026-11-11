#include "game_camera_follow.h"
#include "id_layout_rotation.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
static uint32_t word(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static float number(const uint8_t*p){uint32_t n=word(p);float x;memcpy(&x,&n,4);return x;}
static void put(uint8_t*p,uint32_t n){memcpy(p,&n,4);}
static void store(uint8_t*p,float x){memcpy(p,&x,4);}
static float bits(uint32_t n){float x;memcpy(&x,&n,4);return x;}
static float add(float a,float b){volatile float v=a+b;return v;}
static float mul(float a,float b){volatile float v=a*b;return v;}
static gh_camera_status stop(gh_camera_follow*s,uint32_t*f,uint32_t pc,gh_camera_status e){if(s)s->frontier=pc;if(f)*f=pc;return e;}
static int valid_player(const gh_title_player_view*p){return p&&p->bytes&&p->size>=GH_TITLE_PLAYER_BYTES&&p->alias_f0==p->bytes+0xb0&&p->initial_memory!=GH_TITLE_MEMORY_UNSPECIFIED;}
static int elf_read(const uint8_t*b,size_t n,uint32_t va,void*out,size_t bytes){
 if(!b||n<52||memcmp(b,"\177ELF\1\1\1",7)||b[18]!=8||b[19])return 0;size_t ph=word(b+28);unsigned step=b[42]|(unsigned)b[43]<<8,count=b[44]|(unsigned)b[45]<<8;if(step<32||count>128||ph>n||(size_t)count*step>n-ph)return 0;unsigned found=0;
 for(unsigned i=0;i<count;i++){const uint8_t*p=b+ph+(size_t)i*step;if(word(p)!=1)continue;uint32_t off=word(p+4),base=word(p+8),files=word(p+16),mem=word(p+20);if(files>mem||off>n||files>n-off||(uint64_t)base+mem>UINT64_C(0x100000000))return 0;if(base<=va&&(uint64_t)va+bytes<=(uint64_t)base+files){if(found++)return 0;memcpy(out,b+off+(va-base),bytes);}}
 return found==1;
}
static void identity(float*m){memset(m,0,64);for(unsigned i=0;i<4;i++)m[i*5]=1;}
static void matrix(float*out,const float*a,const float*b){float m[16];for(unsigned c=0;c<4;c++){for(unsigned r=0;r<4;r++){float v=mul(a[r],b[c*4]);v=add(v,mul(a[r+4],b[c*4+1]));v=add(v,mul(a[r+8],b[c*4+2]));m[c*4+r]=add(v,mul(a[r+12],b[c*4+3]));}}memcpy(out,m,64);}
static int rotation(const float*a,float*m){float s[3],c[3];for(unsigned i=0;i<3;i++)if(gh_id_sincos_reference(a[i],s+i,c+i))return 0;float x[16]={1,0,0,0,0,c[0],s[0],0,0,-s[0],c[0],0,0,0,0,1},y[16]={c[1],0,-s[1],0,0,1,0,0,s[1],0,c[1],0,0,0,0,1},z[16]={c[2],s[2],0,0,-s[2],c[2],0,0,0,0,1,0,0,0,0,1};identity(m);matrix(m,x,m);matrix(m,y,m);matrix(m,z,m);return 1;}
static void apply(float*out,const float*m,const float*p){float v[4];for(unsigned i=0;i<4;i++){float x=mul(m[i],p[0]);x=add(x,mul(m[i+4],p[1]));x=add(x,mul(m[i+8],p[2]));v[i]=add(x,m[i+12]);}memcpy(out,v,16);}
static int target_matrix(const gh_title_player_view*p,float*m){float a[3];for(unsigned i=0;i<3;i++)a[i]=number(p->bytes+0x100+i*4);if(!rotation(a,m))return 0;for(unsigned i=0;i<3;i++){float x=number(p->alias_f0+i*4);if(!isfinite(x))return 0;m[12+i]=x;}return 1;}
static int eye_position(gh_game_pl_camera*c,const gh_title_player_view*p,const float*target,float*out){float a[3],m[16],local[3]={0,0,-number(c->fields+0x420)};for(unsigned i=0;i<3;i++){a[i]=number(p->bytes+0x100+i*4);if(i<2)a[i]=add(a[i],number(c->fields+0x410+i*4));if(gh_id_angle_reference_001f7d48(a[i],a+i))return 0;}if(!rotation(a,m))return 0;memcpy(m+12,target,12);apply(out,m,local);return 1;}
static int finite3(const float*p){return isfinite(p[0])&&isfinite(p[1])&&isfinite(p[2]);}
gh_camera_status gh_game_camera_bind_default_00139db8(gh_camera_follow*s,gh_game_pl_camera*c,const gh_game_init_state*init,const gh_title_player_view*p,const gh_boot_work*w,const void*elf,size_t en,uint32_t*f){
 if(f)*f=0;if(!s||!c||!c->constructed||!init||c->identity!=&init->camera||!w||!valid_player(p))return GH_CAMERA_ARGUMENT;if(c->policy!=GH_CAMERA_NATIVE_BINARY32_REFERENCE)return stop(s,f,0x147728,GH_CAMERA_UNSUPPORTED);
 uint32_t coeff[4],expected[4]={0x362e9c14,0xb94fb21f,0x3c08873e,0xbe2aaaa4};float up[3];if(!elf_read(elf,en,0x3c3c90,coeff,16)||memcmp(coeff,expected,16)||!elf_read(elf,en,0x3bd8b0,up,12)||!finite3(up))return stop(s,f,0x30a5d0,GH_CAMERA_UNSUPPORTED);
 if(init->active_camera!=c->identity)return stop(s,f,0x13b9b8,GH_CAMERA_UNSUPPORTED);
 gh_game_pl_camera temp=*c;uint8_t*b=temp.fields;put(b+0x430,0);put(b+0x3f0,0);put(b+0x440,0xbe4ccccd);put(b+0x444,w->room==0x104?0x3f947ae1:0x3f87ae14);put(b+0x448,0);put(b+0x450,w->room==0x104?0x3d4ccccd:0x3d8f5c29);put(b+0x454,0);put(b+0x458,0);put(b+0x460,0x40800000);memcpy(b+0x400,b+0x440,12);memcpy(b+0x410,b+0x450,12);put(b+0x420,word(b+0x460));put(b+0x51c,0);put(b+0x524,word(b+0x520));put(b+0x520,0);put(b+0x528,0);
 fenv_t env;if(feholdexcept(&env))return GH_CAMERA_NUMERIC;if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return GH_CAMERA_NUMERIC;}float m[16],local[3],target[4],eye[4];int ok=target_matrix(p,m);for(unsigned i=0;i<3;i++)local[i]=number(b+0x400+i*4);if(ok){apply(target,m,local);ok=eye_position(&temp,p,target,eye)&&finite3(target)&&finite3(eye);}if(fesetenv(&env))return GH_CAMERA_NUMERIC;if(!ok)return stop(s,f,0x147728,GH_CAMERA_UNSUPPORTED);
 memcpy(b+0x200,target,12);memcpy(b+0x210,eye,12);put(b+0x3d0,0);put(b+0x3d4,0x3f800000);put(b+0x3d8,0);put(b+0x250,0x41f80000);float id[16];identity(id);memcpy(b+0x180,id,64);*c=temp;memset(s,0,sizeof(*s));s->camera=c;s->player=p;s->bound=1;memcpy(s->up_global,up,12);return GH_CAMERA_OK;
}
static gh_camera_status query(gh_camera_follow*s,gh_game_scene_tables*t,gh_camera_follow_ray cb,void*ctx,int*hit,uint32_t*f){uint32_t pc=0;gh_scene_ray_status e=cb?cb(ctx,t,&s->ray,hit,&pc):gh_scene_ray_query_00131e10(t,&s->ray,hit,&pc);if(e)return stop(s,f,pc?pc:0x131e10,e==GH_RAY_UNSUPPORTED?GH_CAMERA_UNSUPPORTED:e==GH_RAY_NUMERIC?GH_CAMERA_NUMERIC:GH_CAMERA_ARGUMENT);return GH_CAMERA_OK;}
static gh_camera_status follow_impl(gh_camera_follow*s,gh_game_scene_tables*t,gh_camera_follow_ray cb,void*ctx,uint32_t*f){
 if(f)*f=0;if(!s||!s->bound||!s->camera||!valid_player(s->player)||!t)return GH_CAMERA_ARGUMENT;gh_game_pl_camera*c=s->camera;const gh_title_player_view*p=s->player;if(c->policy!=GH_CAMERA_NATIVE_BINARY32_REFERENCE)return stop(s,f,0x139ff8,GH_CAMERA_UNSUPPORTED);
 if(!s->stage){uint32_t count=word(c->fields+0x518);if(count)put(c->fields+0x518,count-1);s->stage=1;}
 if(s->stage==1){fenv_t env;if(feholdexcept(&env))return GH_CAMERA_NUMERIC;if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return GH_CAMERA_NUMERIC;}float m[16],local[3],target[4],forward[4],pos[3],next[3];int ok=target_matrix(p,m);s->weight=number(p->bytes+0x15f0);float remaining=add(1,-s->weight);for(unsigned i=0;i<3;i++){local[i]=number(c->fields+0x400+i*4);pos[i]=number(p->alias_f0+i*4);}if(ok){apply(target,m,local);float point[3]={0,0,1},pm[16];for(unsigned i=0;i<16;i++)pm[i]=number(p->bytes+0x80+i*4);apply(forward,pm,point);forward[1]=add(pos[1],1);for(unsigned i=0;i<3;i++){float x=target[i];if(i==1)x=add(x,number(c->fields+0x510));next[i]=add(mul(number(c->fields+0x200+i*4),s->weight),mul(x,remaining));}ok=finite3(target)&&finite3(forward)&&finite3(next)&&isfinite(s->weight);}
  if(fesetenv(&env))return GH_CAMERA_NUMERIC;if(!ok)return stop(s,f,0x147728,GH_CAMERA_UNSUPPORTED);memcpy(s->target,target,16);for(unsigned i=0;i<3;i++)store(c->fields+0x200+i*4,next[i]);memset(&s->ray,0,sizeof(s->ray));memcpy(s->ray.from,pos,12);s->ray.from[1]=add(pos[1],bits(0x3e99999a));s->ray.from[3]=s->ray.to[3]=1;memcpy(s->ray.to,forward,12);s->ray.mask=1;s->ray.report_limit=0;s->ray.segmented=1;s->ray.hit_position=s->hit;s->stage=2;
 }
 if(s->stage==2){int hit=0;gh_camera_status e=query(s,t,cb,ctx,&hit,f);if(e)return e;if(hit==1)s->stage=4;else{memcpy(s->ray.from,s->ray.to,16);memcpy(s->ray.to,p->alias_f0,12);s->ray.to[1]=add(number(p->alias_f0+4),-1);s->ray.mask=7;s->stage=3;}}
 if(s->stage==3){int hit=0;gh_camera_status e=query(s,t,cb,ctx,&hit,f);if(e)return e;float height=hit==1?add(s->hit[1],-number(p->alias_f0+4)):0;s->hit[3]=mul(height,-1.5f);s->stage=5;}
 if(s->stage==4){s->hit[3]=0;s->stage=5;}
 if(s->stage==5){fenv_t env;if(feholdexcept(&env))return GH_CAMERA_NUMERIC;if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return GH_CAMERA_NUMERIC;}float terrain=add(mul(number(c->fields+0x514),bits(0x3f4ccccd)),mul(s->hit[3],bits(0x3e4ccccd))),eye[4],next[3];int ok=eye_position(c,p,s->target,eye);float remaining=add(1,-s->weight);if(ok){for(unsigned i=0;i<3;i++){float v=eye[i];if(i==1)v=add(add(v,terrain),-mul(number(c->fields+0x510),bits(0x3f19999a)));next[i]=add(mul(number(c->fields+0x210+i*4),s->weight),mul(v,remaining));}ok=finite3(next)&&isfinite(terrain);}if(fesetenv(&env))return GH_CAMERA_NUMERIC;if(!ok)return stop(s,f,0x147728,GH_CAMERA_UNSUPPORTED);store(c->fields+0x514,terrain);for(unsigned i=0;i<3;i++)store(c->fields+0x210+i*4,next[i]);memcpy(c->fields+0x3d0,s->up_global,12);s->stage=6;
 }
 return stop(s,f,0x13a3c0,GH_CAMERA_UNSUPPORTED);
}
gh_camera_status gh_game_camera_follow_00139ff8(gh_camera_follow*s,gh_game_scene_tables*t,gh_camera_follow_ray cb,void*ctx,uint32_t*f){
 fenv_t env;if(feholdexcept(&env))return GH_CAMERA_NUMERIC;if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return GH_CAMERA_NUMERIC;}gh_camera_status e=follow_impl(s,t,cb,ctx,f);if(fesetenv(&env))return GH_CAMERA_NUMERIC;return e;
}
gh_camera_status gh_game_camera_step_follow_00138fc8(gh_game_pl_camera*c,gh_game_init_state*i,gh_acd_scene_binding*p,const gh_title_player_view*v,const gh_boot_work*w,gh_camera_follow*s,gh_game_scene_tables*t,gh_camera_follow_ray cb,void*ctx,uint32_t*f){if(f)*f=0;if(!c||!i||!p||!v||!w||c->identity!=&i->camera)return GH_CAMERA_ARGUMENT;/* Resume the SAME pending update, not a new path-blend frame. */if(s&&s->bound&&s->stage){if(s->camera!=c||s->player!=v)return GH_CAMERA_ARGUMENT;return gh_game_camera_follow_00139ff8(s,t,cb,ctx,f);}uint32_t pc=0;gh_camera_status e=gh_game_camera_step_00138fc8(c,i,p,v,w,&pc);if(e==GH_CAMERA_UNSUPPORTED&&pc==0x139ff8){if(!s||!s->bound||s->camera!=c||s->player!=v)return stop(s,f,0x139db8,GH_CAMERA_UNSUPPORTED);e=gh_game_camera_follow_00139ff8(s,t,cb,ctx,&pc);}if(f)*f=pc;return e;}

gh_camera_status gh_game_camera_reset_pose_0013b9b8(gh_camera_follow *follow,
    const gh_game_init_state *init, uint32_t *frontier) {
    if (frontier) *frontier = 0x13b9b8;
    if (!init) return GH_CAMERA_ARGUMENT;
    /* The original reads no camera fields when the active global is NULL. */
    if (!init->active_camera) {
        if (frontier) *frontier = 0;
        return GH_CAMERA_OK;
    }
    if (!follow || !follow->bound || follow->stage || !follow->camera ||
        !follow->camera->constructed || follow->camera->identity != &init->camera ||
        !valid_player(follow->player) ||
        follow->camera->policy != GH_CAMERA_NATIVE_BINARY32_REFERENCE)
        return GH_CAMERA_UNSUPPORTED;

    gh_game_pl_camera *camera = follow->camera;
    float matrix_data[16], local[3], target[4], eye[4];
    fenv_t env;
    if (feholdexcept(&env)) return GH_CAMERA_NUMERIC;
    if (fesetround(FE_TONEAREST)) {
        (void)fesetenv(&env);
        return GH_CAMERA_NUMERIC;
    }
    int valid = target_matrix(follow->player, matrix_data);
    for (unsigned i = 0; i < 3; ++i) local[i] = number(camera->fields + 0x400 + i * 4);
    if (valid) {
        apply(target, matrix_data, local);
        valid = finite3(target) && eye_position(camera, follow->player, target, eye) && finite3(eye);
    }
    if (fesetenv(&env)) return GH_CAMERA_NUMERIC;
    if (!valid) {
        if (frontier) *frontier = 0x147728;
        return GH_CAMERA_UNSUPPORTED;
    }

    put(camera->fields + 0x51c, 0);
    memcpy(camera->fields + 0x200, target, 12);
    memcpy(camera->fields + 0x210, eye, 12);
    put(camera->fields + 0x3d0, 0);
    put(camera->fields + 0x3d4, 0x3f800000);
    put(camera->fields + 0x3d8, 0);
    put(camera->fields + 0x250, 0x41f80000);
    if (frontier) *frontier = 0;
    return GH_CAMERA_OK;
}
