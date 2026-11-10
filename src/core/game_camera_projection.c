/* Recovered original138740/138648 and SDK math chain. MIT LICENSE.upstream,
 * eaae18d0dfab1f9a0df276ae5ca1581ad326faf3. ActualpatchedELF evidence in
 * analysis/game-camera-projection-evidence.json. Explicit native FP reference. */
#include "game_camera_projection.h"
#include <math.h>
#include <string.h>
#include <fenv.h>
static uint32_t word(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static float bits(uint32_t n){float x;memcpy(&x,&n,4);return x;}
static float readf(const uint8_t*p){return bits(word(p));}
static void writef(uint8_t*p,float x){uint32_t n;memcpy(&n,&x,4);for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(n>>(8*i));}
static float add(float a,float b){volatile float r=a+b;return r;}
static float sub(float a,float b){volatile float r=a-b;return r;}
static float mul(float a,float b){volatile float r=a*b;return r;}
static float divide(float a,float b){volatile float r=a/b;return r;}
static void identity(float*m){memset(m,0,64);for(unsigned i=0;i<4;i++)m[i*5]=1;}
static void matrix(float*out,const float*a,const float*b){float t[16];for(unsigned c=0;c<4;c++)for(unsigned r=0;r<4;r++){float v=mul(a[r],b[c*4]);v=add(v,mul(a[4+r],b[c*4+1]));v=add(v,mul(a[8+r],b[c*4+2]));t[c*4+r]=add(v,mul(a[12+r],b[c*4+3]));}memcpy(out,t,sizeof(t));}
static void apply(float*out,const float*m,const float*v){for(unsigned r=0;r<4;r++){float n=mul(m[r],v[0]);n=add(n,mul(m[4+r],v[1]));n=add(n,mul(m[8+r],v[2]));out[r]=add(n,mul(m[12+r],v[3]));}}
static void cross(float*out,const float*a,const float*b){out[0]=sub(mul(a[1],b[2]),mul(a[2],b[1]));out[1]=sub(mul(a[2],b[0]),mul(a[0],b[2]));out[2]=sub(mul(a[0],b[1]),mul(a[1],b[0]));out[3]=0;}
static int normalize(float*v){float sum=add(add(mul(v[0],v[0]),mul(v[1],v[1])),mul(v[2],v[2]));float length=sqrtf(sum);if(!(length>0)||!isfinite(length))return 0;float q=divide(1,length);for(unsigned i=0;i<3;i++)v[i]=mul(v[i],q);v[3]=0;return 1;}
/* Actual3A1AE8->3A4830, iy1,y0 and |x|<=3F2CA13F branch only.
 * Coefficients45EFF0..45F020 verified against user's ELF; operation sequence
 * follows3A4924..3A49E8 (no hostlibm tan substitution). */
static float tangent(float x){
 static const uint32_t cb[]={0x3eaaaaab,0x3e088889,0x3d5d0dd1,0x3cb327a4,0x3c11371f,0x3b6b6916,0x3abede48,0x3a1a26c8,0x398137b9,0x38a3f445,0x3895c07a,0xb79bae5f,0x37d95384};
 float z=mul(x,x),w=mul(z,z),s=mul(z,x),even=bits(cb[12]),odd=bits(cb[11]);
 for(int i=10;i>=2;i-=2)even=add(bits(cb[i]),mul(w,even));
 for(int i=9;i>=1;i-=2)odd=add(bits(cb[i]),mul(w,odd));
 float r=add(odd,mul(z,even));r=mul(s,r);r=add(r,0);r=mul(z,r);r=add(0,r);r=add(r,mul(bits(cb[0]),s));return add(x,r);
}
static int view_matrix(float*out,const uint8_t*p){float model[16],eye[4],target[4],pos[4],end[4],z[4],up[4],x[4],y[4];for(unsigned i=0;i<16;i++)model[i]=readf(p+0x180+i*4);for(unsigned i=0;i<3;i++){eye[i]=readf(p+0x210+i*4);target[i]=readf(p+0x200+i*4);up[i]=readf(p+0x240+i*4);}eye[3]=up[3]=1;target[3]=readf(p+0x20c);
 apply(pos,model,eye);apply(end,model,target);for(unsigned i=0;i<3;i++)z[i]=sub(end[i],eye[i]);z[3]=1;
 cross(x,up,z);if(!normalize(x)||!normalize(z))return 0;cross(y,z,x);
 /* 30A8C8 forms rigidworldmatrix then30A3F8 transposes basis and
  * inverse-rotates translation. Preserve sequential multiply/add order. */
 identity(out);for(unsigned r=0;r<3;r++){out[r*4]=x[r];out[r*4+1]=y[r];out[r*4+2]=z[r];}
 for(unsigned r=0;r<3;r++)out[12+r]=-add(add(mul(out[r],pos[0]),mul(out[4+r],pos[1])),mul(out[8+r],pos[2]));return 1;
}
static void perspective(float*out,float focal,float width,float aspect,float far_depth){float base[16],screen[16];identity(base);base[0]=base[5]=focal;base[10]=base[15]=0;base[11]=base[14]=1;identity(screen);float near=bits(0x3dcccccd),far=600,den=add(-near,far);screen[0]=width;screen[5]=aspect;screen[10]=divide(mul(mul(far,near),add(-1,far_depth)),den);screen[12]=screen[13]=2048;screen[14]=divide(add(mul(-far_depth,near),far),den);matrix(out,screen,base);out[0]=-out[0];out[5]=-out[5];}
static gh_camera_status fail(uint32_t*f,uint32_t pc,gh_camera_status e){if(f)*f=pc;return e;}
gh_camera_status gh_game_camera_project_00138740(gh_game_pl_camera*c,const gh_camera_projection_display*d,uint32_t*f){
 if(f)*f=0;if(!c||!c->constructed||!d||!d->screen_width||!d->screen_height_bits||!d->pixel_aspect_bits||!d->initialized)return GH_CAMERA_ARGUMENT;
 if(!*d->initialized)return fail(f,0x2a48a8,GH_CAMERA_UNSUPPORTED);
 if(c->policy!=GH_CAMERA_NATIVE_BINARY32_REFERENCE||d->policy!=GH_CAMERA_NATIVE_BINARY32_REFERENCE)return fail(f,0x138740,GH_CAMERA_UNSUPPORTED);
 if(c->fields[0x278]||c->fields[0x264])return fail(f,0x138740,GH_CAMERA_UNSUPPORTED);
 float fov=readf(c->fields+0x250),height=bits(*d->screen_height_bits),aspect=bits(*d->pixel_aspect_bits);
 if(!(fov>0)||!(height>0)||!(aspect>0)||*d->screen_width<=0||!isfinite(fov)||!isfinite(height)||!isfinite(aspect))return fail(f,0x138740,GH_CAMERA_NUMERIC);
 fenv_t env;if(feholdexcept(&env))return fail(f,0x138740,GH_CAMERA_NUMERIC);if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return fail(f,0x138740,GH_CAMERA_NUMERIC);}
 float angle=divide(mul(fov,bits(0x40490fdb)),360);
 if(angle>bits(0x3f2ca13f)){(void)fesetenv(&env);return fail(f,0x3a4830,GH_CAMERA_UNSUPPORTED);}
 float t=tangent(angle),half_height=mul(divide(height,aspect),.5f),focal=divide(half_height,t);
 float proj[16],view[16],combined[16],clip[3][16];int good=isfinite(focal)&&t>0;
 if(good){float far_depth=divide(bits(0x49f7ff08),add(fov,fov));far_depth=add(far_depth,far_depth);perspective(proj,focal,divide((float)*d->screen_width,640),aspect,far_depth);good=view_matrix(view,c->fields);}
 if(good){matrix(combined,proj,view);for(unsigned k=0;k<3;k++){float m[16];identity(m);m[10]=bits(0x3f800aec);m[14]=bits(0xbe4cd58a);m[11]=1;m[15]=0;
  float fx=k==2?divide(160,focal):divide(51.2f,focal),fy=k==2?fx:divide(44.8f,focal);
  m[0]=-divide(.2f,add(fx,fx));m[5]=-divide(.2f,add(fy,fy));matrix(clip[k],m,view);}
  for(unsigned i=0;i<16;i++)if(!isfinite(proj[i])||!isfinite(view[i])||!isfinite(combined[i])||!isfinite(clip[0][i])||!isfinite(clip[1][i])||!isfinite(clip[2][i]))good=0;
 }
 if(fesetenv(&env))return fail(f,0x138740,GH_CAMERA_NUMERIC);if(!good)return fail(f,0x138740,GH_CAMERA_NUMERIC);
 for(unsigned i=0;i<16;i++){writef(c->fields+0x100+i*4,proj[i]);writef(c->fields+0x140+i*4,view[i]);writef(c->fields+i*4,combined[i]);for(unsigned k=0;k<3;k++)writef(c->fields+0x40+k*0x40+i*4,clip[k][i]);}return GH_CAMERA_OK;
}
gh_camera_status gh_game_camera_step_projected_00138fc8(gh_game_pl_camera*c,gh_game_init_state*i,gh_acd_scene_binding*p,const gh_title_player_view*v,const gh_boot_work*w,const gh_camera_projection_display*d,uint32_t*f){uint32_t pc=0;gh_camera_status e=gh_game_camera_step_00138fc8(c,i,p,v,w,&pc);if(e==GH_CAMERA_UNSUPPORTED&&pc==0x138740)e=gh_game_camera_project_00138740(c,d,&pc);if(f)*f=pc;return e;}
