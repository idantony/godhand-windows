/* Actual1538F0 and SDK3A1AE8/3A4830 bounded polynomial branch.
 * MIT upstream eaae18d0dfab1f9a0df276ae5ca1581ad326faf3; independent native
 * arithmetic profile, not EE/VU accumulator bit identity. */
#include "game_render_projection.h"
#include <math.h>
#include <string.h>
#include <fenv.h>
static float bits(uint32_t n){float f;memcpy(&f,&n,4);return f;}
static float readf(const uint8_t*p){float f;memcpy(&f,p,4);return f;}
static float add(float a,float b){volatile float x=a+b;return x;}
static float mul(float a,float b){volatile float x=a*b;return x;}
static float divf(float a,float b){volatile float x=a/b;return x;}
static float tangent(float x){
 static const uint32_t c[]={0x3eaaaaab,0x3e088889,0x3d5d0dd1,0x3cb327a4,0x3c11371f,0x3b6b6916,0x3abede48,0x3a1a26c8,0x398137b9,0x38a3f445,0x3895c07a,0xb79bae5f,0x37d95384};
 float z=mul(x,x),w=mul(z,z),s=mul(z,x),even=bits(c[12]),odd=bits(c[11]);
 for(int i=10;i>=2;i-=2)even=add(bits(c[i]),mul(w,even));
 for(int i=9;i>=1;i-=2)odd=add(bits(c[i]),mul(w,odd));
 float r=add(odd,mul(z,even));r=mul(s,r);r=add(r,0);r=mul(z,r);r=add(0,r);r=add(r,mul(bits(c[0]),s));return add(x,r);
}
static gh_camera_status fail(uint32_t*f,uint32_t pc,gh_camera_status e){if(f)*f=pc;return e;}
gh_camera_status gh_game_render_projection_001538f0(gh_game_render_projection*out,
 const gh_game_pl_camera*c,const gh_camera_projection_display*d,uint32_t*f){
 if(f)*f=0;if(!out||!c||!c->constructed||!d||!d->initialized||!d->screen_width||!d->screen_height_bits||!d->pixel_aspect_bits)return GH_CAMERA_ARGUMENT;
 if(!*d->initialized)return fail(f,0x2a48a8,GH_CAMERA_UNSUPPORTED);
 if(c->policy!=GH_CAMERA_NATIVE_BINARY32_REFERENCE||d->policy!=GH_CAMERA_NATIVE_BINARY32_REFERENCE)return fail(f,0x1538f0,GH_CAMERA_UNSUPPORTED);
 float fov=readf(c->fields+0x250),height=bits(*d->screen_height_bits),aspect=bits(*d->pixel_aspect_bits);
 if(!(fov>0)||!(height>0)||!(aspect>0)||*d->screen_width<=0||!isfinite(fov)||!isfinite(height)||!isfinite(aspect))return fail(f,0x1538f0,GH_CAMERA_NUMERIC);
 gh_game_render_projection n={0};for(unsigned i=0;i<16;i++){n.camera_clip[i]=readf(c->fields+0x40+i*4);if(!isfinite(n.camera_clip[i]))return fail(f,0x30a3f8,GH_CAMERA_NUMERIC);}
 fenv_t env;if(feholdexcept(&env))return fail(f,0x1538f0,GH_CAMERA_NUMERIC);
 if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return fail(f,0x1538f0,GH_CAMERA_NUMERIC);}
 float angle=divf(mul(fov,bits(0x40490fdb)),360);
 if(angle>bits(0x3f2ca13f)){(void)fesetenv(&env);return fail(f,0x3a4830,GH_CAMERA_UNSUPPORTED);}
 float t=tangent(angle),focal=divf(mul(divf(height,aspect),.5f),t);
 float width=divf((float)*d->screen_width,640),depth=divf(bits(0x49f7ff08),add(fov,fov));
 float sx=divf(bits(0x424ccccd),focal),sy=divf(bits(0x42333333),focal);depth=add(depth,depth);
 for(unsigned i=0;i<4;i++)n.projection[i*5]=1;
 n.projection[0]=divf(mul(mul(focal,width),sx),bits(0x3dcccccd));
 n.projection[5]=divf(mul(mul(focal,aspect),sy),bits(0x3dcccccd));
 n.projection[10]=mul(add(-depth,1),.5f);n.projection[12]=n.projection[13]=2048;
 n.projection[14]=mul(add(depth,1),.5f);n.projection[15]=1;
 int good=t>0&&isfinite(focal);for(unsigned i=0;i<16;i++)if(!isfinite(n.projection[i]))good=0;
 if(fesetenv(&env))return fail(f,0x1538f0,GH_CAMERA_NUMERIC);
 if(!good)return fail(f,0x1538f0,GH_CAMERA_NUMERIC);n.ready=1;*out=n;return GH_CAMERA_OK;
}
