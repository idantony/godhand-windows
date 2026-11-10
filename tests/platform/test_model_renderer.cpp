#include "../../src/platform/model_renderer.h"
#include "../../src/platform/disc.h"
#include "../../src/assets/tm3.h"
#include <array>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <limits>
#define C(x) do{if(!(x)){std::fprintf(stderr,"line%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
constexpr unsigned W=512,H=448;constexpr size_t N=(size_t)W*H;
static std::vector<uint8_t> load(gh_disc*d,const char*p){size_t i;gh_disc_entry e;C(!gh_disc_find(d,p,&i)&&!gh_disc_entry_at(d,i,&e));std::vector<uint8_t>b((size_t)e.size);C(!gh_disc_read(d,i,0,b.data(),b.size()));return b;}
static int64_t floor_div(int64_t n,int64_t d){return n>=0?n/d:-((-n+d-1)/d);}
static unsigned sat(int64_t x){return x<0?0:x>255?255:(unsigned)x;}
struct Surface{std::vector<uint8_t>rgba;std::vector<uint32_t>z;Surface(uint32_t rgb,uint32_t depth):rgba(N*4),z(N,depth){for(size_t i=0;i<N;i++)for(unsigned c=0;c<3;c++)rgba[i*4+c]=(uint8_t)(rgb>>(8*c));}};
/* Independent geometry oracle: fixture is the half-open right triangle with
 * known legs256. Barycentrics follow its analytic coordinates, not a copy of
 * the GPU edge/interpolation implementation. Long double retains exact
 * powers-of-two fixture values until mandatory GS integer quantization. */
static std::array<int,4> sample(const gh_stage_material&m,long double s,long double t,bool linear){
 int64_t u=(int64_t)(s*m.width*65536),v=(int64_t)(t*m.height*65536);
 if(linear){u-=32768;v-=32768;}
 int64_t x=floor_div(u,65536),y=floor_div(v,65536);
 unsigned fx=(unsigned)(u-x*65536)/4096,fy=(unsigned)(v-y*65536)/4096;
 auto texel=[&](int64_t tx,int64_t ty,unsigned c){unsigned xx=(unsigned)((tx%(int64_t)m.width+m.width)%m.width),yy=(unsigned)((ty%(int64_t)m.height+m.height)%m.height);return (int64_t)m.rgba_gs[((size_t)yy*m.width+xx)*4+c];};
 std::array<int,4>out{};
 for(unsigned c=0;c<4;c++){
  if(!linear){out[c]=(int)texel(x,y,c);continue;}
  /* Exact rational separable GS4-bit weights, independent64-bit sums.
   * Quantization between horizontal and vertical is observable. */
  int64_t top=(texel(x,y,c)*(16-fx)+texel(x+1,y,c)*fx)/16;
  int64_t bottom=(texel(x,y+1,c)*(16-fx)+texel(x+1,y+1,c)*fx)/16;
  out[c]=(int)((top*(16-fy)+bottom*fy)/16);
 }return out;
}
static bool alpha_test(unsigned a,uint64_t test){if(!(test&1))return true;unsigned ref=(unsigned)(test>>4)&255;switch((test>>1)&7){case 0:return false;case 1:return true;case 2:return a<ref;case 3:return a<=ref;case 4:return a==ref;case 5:return a>=ref;case 6:return a>ref;default:return a!=ref;}}
static void oracle(Surface&dst,const gh_stage_material&m,const gh_model_draw_state&s,const std::array<gh_static_gs_vertex,3>&v){
 unsigned touched=0;
 for(unsigned y=64;y<320;y++)for(unsigned x=64;x<320;x++){
  if(x+y>=384)continue; // independent analytic top/left-inclusive coverage
  long double b=(long double)(x-64)/256,c=(long double)(y-64)/256,a=1-b-c;
  auto weighted=[&](long double x0,long double x1,long double x2){return a*x0+b*x1+c*x2;};
  size_t p=(size_t)y*W+x,o=p*4;
  uint32_t depth=(uint32_t)weighted(((uint32_t)v[0].xyz_fixed4[2]>>4)&0xffffff,((uint32_t)v[1].xyz_fixed4[2]>>4)&0xffffff,((uint32_t)v[2].xyz_fixed4[2]>>4)&0xffffff);
  unsigned zm=(unsigned)(s.test>>17)&3;
  if((s.test&0x10000)&&(zm==0||(zm==2&&depth<dst.z[p])||(zm==3&&depth<=dst.z[p])))continue;
  if((s.test&0x4000)&&((dst.rgba[o+3]>>7)&1)!=((s.test>>15)&1))continue;
  long double q=weighted(v[0].stq[2],v[1].stq[2],v[2].stq[2]);
  auto tex=sample(m,weighted(v[0].stq[0],v[1].stq[0],v[2].stq[0])/q,weighted(v[0].stq[1],v[1].stq[1],v[2].stq[1])/q,(s.tex1&32)!=0);
  int src[4];for(unsigned k=0;k<4;k++)src[k]=(int)std::min<int64_t>(255,(int64_t)tex[k]*(int64_t)weighted(v[0].rgba[k],v[1].rgba[k],v[2].rgba[k])/128);
  bool pass=alpha_test((unsigned)src[3],s.test);unsigned fail=(unsigned)(s.test>>12)&3;
  if(!pass&&!fail)continue;
  bool color=pass||fail!=2,writez=(pass||fail==2)&&s.z_write;
  if(color){
   if(s.primitive&32){unsigned f=(unsigned)weighted((v[0].fog_fixed4>>4)&255,(v[1].fog_fixed4>>4)&255,(v[2].fog_fixed4>>4)&255);for(unsigned k=0;k<3;k++)src[k]=(int)((((s.fog_rgb>>(8*k))&255)*(256-f)+(unsigned)src[k]*f)/256);}
   for(unsigned k=0;k<3;k++){
    int64_t rgb=src[k];
    if((s.primitive&64)&&(!s.pabe||(src[3]&128))){
     auto component=[&](unsigned selector){return selector==0?src[k]:selector==1?(int)dst.rgba[o+k]:0;};
     unsigned ca=(unsigned)s.alpha&3,cb=(unsigned)(s.alpha>>2)&3,cc=(unsigned)(s.alpha>>4)&3,cd=(unsigned)(s.alpha>>6)&3;
     int factor=cc==0?src[3]:cc==1?dst.rgba[o+3]:(int)((s.alpha>>32)&255);
     rgb=floor_div((int64_t)(component(ca)-component(cb))*factor,128)+component(cd);
    }
    dst.rgba[o+k]=(uint8_t)sat(rgb);
   }
   if(pass||fail!=3)dst.rgba[o+3]=(uint8_t)(src[3]|(s.fba?128:0));
  }
  if(writez)dst.z[p]=depth;touched++;
 }
 (void)touched; // alpha/depth fixtures deliberately may reject all fragments
}
static void compare(gh_model_renderer*r,const Surface&expected,const char*label){std::vector<uint8_t>got(N*4);std::vector<uint32_t>z(N);C(!gh_model_renderer_readback(r,got.data(),got.size(),z.data(),z.size()));size_t bad=0;for(size_t i=0;i<N;i++){if(std::memcmp(got.data()+i*4,expected.rgba.data()+i*4,4)||z[i]!=expected.z[i]){if(!bad)std::fprintf(stderr,"%s mismatch(%zu,%zu) RGBA%u,%u,%u,%u/%u,%u,%u,%u Z%u/%u\n",label,i%W,i/W,got[i*4],got[i*4+1],got[i*4+2],got[i*4+3],expected.rgba[i*4],expected.rgba[i*4+1],expected.rgba[i*4+2],expected.rgba[i*4+3],z[i],expected.z[i]);bad++;}}C(!bad);}
static std::array<gh_static_gs_vertex,3> triangle(uint32_t z,unsigned fog,unsigned alpha){std::array<gh_static_gs_vertex,3>v{};const unsigned xy[3][2]={{64,64},{320,64},{64,320}};for(unsigned i=0;i<3;i++){v[i].xyz_fixed4[0]=(int32_t)(0x10000u+1792*16+xy[i][0]*16);v[i].xyz_fixed4[1]=(int32_t)(0x20000u+1824*16+xy[i][1]*16);v[i].xyz_fixed4[2]=(int32_t)(0x10000000u+(z<<4)+15);v[i].fog_fixed4=(uint16_t)((fog<<4)|15);v[i].stq[2]=1;for(unsigned c=0;c<3;c++)v[i].rgba[c]=128;v[i].rgba[3]=alpha;}v[0].stq[0]=-.25f;v[0].stq[1]=-.125f;v[1].stq[0]=1.75f;v[1].stq[1]=-.125f;v[2].stq[0]=-.25f;v[2].stq[1]=1.875f;return v;}
int main(int argc,char**argv){C(argc==2);gh_disc*d;C(!gh_disc_open(argv[1],&d));auto bytes=load(d,"st1/r100.dat");gh_disc_close(d);gh_bundle room,scp;gh_bundle_entry e;C(!gh_bundle_open(bytes.data(),bytes.size(),&room)&&!gh_bundle_find(&room,"SCP",0,&e)&&!gh_bundle_open(e.bytes,e.size,&scp)&&!gh_bundle_find(&scp,"TM3",0,&e));gh_tm3_bundle tm3;gh_tm3_picture picture;C(!gh_tm3_open(e.bytes,e.size,&tm3)&&!gh_tm3_member(&tm3,37,&picture)&&picture.width==32&&picture.height==32);
 std::vector<uint8_t>pixels((size_t)picture.width*picture.height*4);C(!gh_tm3_rgba_gs(&picture,pixels.data(),pixels.size()));
 unsigned maxalpha=0;for(size_t i=3;i<pixels.size();i+=4)if(pixels[i]>maxalpha)maxalpha=pixels[i];std::printf("actualmember37 alphaMax%u\n",maxalpha);
 gh_stage_material material{};material.source=picture.source;material.source_bytes=picture.source_bytes;material.rgba_gs=pixels.data();material.rgba_bytes=pixels.size();material.cache_id=1;material.source_tex0=picture.tex0;material.width=picture.width;material.height=picture.height;material.psm=(uint8_t)((picture.tex0>>20)&63);material.draw_tcc=1;material.draw_tfx=0;
 gh_model_renderer*r=nullptr;C(!gh_model_renderer_create(W,H,&r));uint32_t texture=0,again=0;C(!gh_model_renderer_upload(r,&material,&texture)&&texture&& !gh_model_renderer_upload(r,&material,&again)&&again==texture);
 gh_model_draw_state state{};state.test=0x507fd;state.alpha=0x48;state.xy_offset_x16=1792*16;state.xy_offset_y16=1824*16;state.fog_rgb=0x2060a0;state.primitive=0x7c;state.zbuf_psm=58;state.z_write=1;state.colclamp=1;state.native_float_reference=1;const uint32_t indices[3]={0,1,2};
 // Actualmember37 alphaMax47 cannot passGREATER127 evenwithtint255.
 // Prove that originalTEST case rejects, then explicitly useATESTalways to
 // exercise realtexture RGB/filter/blend/fog rather than validating blanks.
 C(maxalpha==47);{auto actual=triangle(100,255,255);Surface rejected(0x16304c,50);C(!gh_model_renderer_begin(r,0x16304c,50));C(!gh_model_renderer_draw(r,&state,texture,actual.data(),3,indices,3));oracle(rejected,material,state,actual);compare(r,rejected,"actualmaterial37/originalTEST507FD-reject");for(auto z:rejected.z)C(z==50);}
 state.test=0x50003;
 for(unsigned linear=0;linear<2;linear++)for(unsigned blend=0;blend<2;blend++)for(unsigned fog: {0u,128u,255u}){
  state.tex1=linear?96:0;state.alpha=blend?0x44:0x48;auto v=triangle(100,fog,255);v[1].xyz_fixed4[2]+=256*16;v[2].xyz_fixed4[2]+=512*16;
  Surface expected(0x16304c,50);C(!gh_model_renderer_begin(r,0x16304c,50));C(!gh_model_renderer_draw(r,&state,texture,v.data(),3,indices,3));oracle(expected,material,state,v);compare(r,expected,"actualmaterial37/nearestbilinear/fog/alpha");
  size_t changed=0;for(auto depth:expected.z)if(depth!=50)changed++;C(changed>1000);
 }
 // Explicit alpha128 fixture keeps realmaterial37 RGB texels but replaces
 // alpha solely for positiveGREATER127/depth/control tests. It is NOT an
 // original material/cache view or a claim of visiblegamefragments.
 std::vector<uint8_t>fixture=pixels;for(size_t i=3;i<fixture.size();i+=4)fixture[i]=128;
 material.rgba_gs=fixture.data();material.source=fixture.data();material.source_bytes=fixture.size();material.cache_id=2;
 C(!gh_model_renderer_upload(r,&material,&texture));state.test=0x507fd;
 // GS alpha-test failure policies and destination alpha are independent
 // of normal blend acceptance; compare RGB/A/Z separately across fullsurface.
 for(unsigned fail=0;fail<4;fail++){
  auto reject=state;reject.test=0x50001u|((uint64_t)fail<<12);auto v=triangle(300,255,128);
  Surface expected_fail(0x314159,10);C(!gh_model_renderer_begin(r,0x314159,10));
  C(!gh_model_renderer_draw(r,&reject,texture,v.data(),3,indices,3));oracle(expected_fail,material,reject,v);compare(r,expected_fail,"ATSTnever/AFAILkeep-frame-z-rgb");
 }
 {
  auto dat=state;dat.test=0x5c003;auto v=triangle(300,255,128);Surface dstalpha(0x314159,10);
  C(!gh_model_renderer_begin(r,0x314159,10));C(!gh_model_renderer_draw(r,&dat,texture,v.data(),3,indices,3));oracle(dstalpha,material,dat,v);compare(r,dstalpha,"DAT1rejectinitialalpha0");
  auto write=state;write.test=0x50003;write.fba=1;write.pabe=1;v=triangle(300,255,64);
  C(!gh_model_renderer_draw(r,&write,texture,v.data(),3,indices,3));oracle(dstalpha,material,write,v);compare(r,dstalpha,"FBAalpha128/PABEnoblend");
  v=triangle(400,128,128);C(!gh_model_renderer_draw(r,&dat,texture,v.data(),3,indices,3));oracle(dstalpha,material,dat,v);compare(r,dstalpha,"DAT1afterFBA");
 }
 // Same geometry: true ZGEQUAL/order, fractional tint and alpha test thresholds.
 state.tex1=96;state.alpha=0x44;Surface expected(0x102030,0);C(!gh_model_renderer_begin(r,0x102030,0));
 for(unsigned step=0;step<4;step++){auto v=triangle(step==1?99:100,255,step==2?127:128);for(auto&vertex:v)vertex.rgba[0]=step==3?64:128;C(!gh_model_renderer_draw(r,&state,texture,v.data(),3,indices,3));oracle(expected,material,state,v);}
 compare(r,expected,"depthGEQUAL/equality/rejectedfar/alphathreshold");
 // PerspectiveSTQ with varyingQ but constant normalized UV remains exact.
 auto perspective=triangle(200,128,128);for(unsigned i=0;i<3;i++){float q=(float)(1u<<i);perspective[i].stq[0]=q*.25f;perspective[i].stq[1]=q*-.125f;perspective[i].stq[2]=q;}
 C(!gh_model_renderer_draw(r,&state,texture,perspective.data(),3,indices,3));oracle(expected,material,state,perspective);compare(r,expected,"perspectiveQ/rationalUV");
 auto invalid=perspective;invalid[2].stq[2]=std::numeric_limits<float>::quiet_NaN();C(gh_model_renderer_draw(r,&state,texture,invalid.data(),3,indices,3)==GH_MODEL_GPU_UNSUPPORTED);compare(r,expected,"invalidlastvertexatomic");uint32_t bad[3]={0,1,3};C(gh_model_renderer_draw(r,&state,texture,perspective.data(),3,bad,3)==GH_MODEL_GPU_ARGUMENT);compare(r,expected,"invalidlastindexatomic");
 auto unsupported=state;unsupported.dthe=1;C(gh_model_renderer_draw(r,&unsupported,texture,perspective.data(),3,indices,3)==GH_MODEL_GPU_UNSUPPORTED);compare(r,expected,"unsupportedstateatomic");
 C(gh_model_renderer_begin(r,0,65536)==GH_MODEL_GPU_UNSUPPORTED);compare(r,expected,"invalidclearatomic");gh_model_renderer_reset_resources(r);C(gh_model_renderer_draw(r,&state,texture,perspective.data(),3,indices,3)==GH_MODEL_GPU_ARGUMENT);gh_model_renderer_destroy(r);
 std::puts("PASS realR100TM3member37 GPU upload; full512x448 RGBA+Z independenthighprecision analyticcoverage/STQ/GSbilinear/TFX/fog256/alpha48+44/depth/order/packedXYZF2/atomicfailure; explicitnumericfixture, notgameframe/hardwarebitexact");return 0;
}
