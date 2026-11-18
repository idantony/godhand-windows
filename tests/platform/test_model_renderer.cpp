#include "../../src/platform/model_renderer.h"
#include "../../src/platform/disc.h"
#include "../../src/assets/tm3.h"
#include "../../src/core/game_effects_trans.h"
#include <algorithm>
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
 constexpr long double gs_texel_limit=2047;
 int64_t u=(int64_t)(std::clamp(s*m.width,-gs_texel_limit,gs_texel_limit)*65536);
 int64_t v=(int64_t)(std::clamp(t*m.height,-gs_texel_limit,gs_texel_limit)*65536);
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
static void oracle(Surface&dst,const gh_stage_material&m,const gh_model_draw_state&s,const std::array<gh_static_gs_vertex,3>&v,bool tcc0=false){
 unsigned touched=0;
 for(unsigned y=64;y<320;y++)for(unsigned x=64;x<320;x++){
  if(x+y>=384)continue; // independent analytic top/left-inclusive coverage
  long double b=(long double)(x-64)/256,c=(long double)(y-64)/256,a=1-b-c;
  auto weighted=[&](long double x0,long double x1,long double x2){return a*x0+b*x1+c*x2;};
  size_t p=(size_t)y*W+x,o=p*4;
  uint32_t depth=(uint32_t)std::min(65535.0L,weighted(((uint32_t)v[0].xyz_fixed4[2]>>4)&0xffffff,
    ((uint32_t)v[1].xyz_fixed4[2]>>4)&0xffffff,((uint32_t)v[2].xyz_fixed4[2]>>4)&0xffffff));
  unsigned zm=(unsigned)(s.test>>17)&3;
  if((s.test&0x10000)&&(zm==0||(zm==2&&depth<dst.z[p])||(zm==3&&depth<=dst.z[p])))continue;
  if((s.test&0x4000)&&((dst.rgba[o+3]>>7)&1)!=((s.test>>15)&1))continue;
  long double q=weighted(v[0].stq[2],v[1].stq[2],v[2].stq[2]);
  auto tex=sample(m,weighted(v[0].stq[0],v[1].stq[0],v[2].stq[0])/q,weighted(v[0].stq[1],v[1].stq[1],v[2].stq[1])/q,(s.tex1&32)!=0);
  int shade[4],src[4];for(unsigned k=0;k<4;k++)shade[k]=(int)weighted(v[0].rgba[k],v[1].rgba[k],v[2].rgba[k]);
  for(unsigned k=0;k<4;k++){
   int64_t value=(int64_t)tex[k]*shade[k]/128;
   if(s.texture_function==3)value=k<3?value+shade[3]:tex[3];
   if(tcc0&&k==3)value=shade[3];
   src[k]=(int)std::min<int64_t>(255,value);
  }
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
    unsigned mask=(s.frame_mask>>(8*k))&255;
    dst.rgba[o+k]=(uint8_t)((dst.rgba[o+k]&mask)|(sat(rgb)&~mask));
   }
   if(pass||fail!=3){
    unsigned mask=s.frame_mask>>24;
    unsigned alpha=(unsigned)src[3]|(s.fba?128u:0u);
    dst.rgba[o+3]=(uint8_t)((dst.rgba[o+3]&mask)|(alpha&~mask));
   }
  }
  if(writez)dst.z[p]=depth;touched++;
 }
 (void)touched; // alpha/depth fixtures deliberately may reject all fragments
}
static void compare(gh_model_renderer*r,const Surface&expected,const char*label){std::vector<uint8_t>got(N*4);std::vector<uint32_t>z(N);C(!gh_model_renderer_readback(r,got.data(),got.size(),z.data(),z.size()));size_t bad=0;for(size_t i=0;i<N;i++){if(std::memcmp(got.data()+i*4,expected.rgba.data()+i*4,4)||z[i]!=expected.z[i]){if(!bad)std::fprintf(stderr,"%s mismatch(%zu,%zu) RGBA%u,%u,%u,%u/%u,%u,%u,%u Z%u/%u\n",label,i%W,i/W,got[i*4],got[i*4+1],got[i*4+2],got[i*4+3],expected.rgba[i*4],expected.rgba[i*4+1],expected.rgba[i*4+2],expected.rgba[i*4+3],z[i],expected.z[i]);bad++;}}C(!bad);}
static std::array<gh_static_gs_vertex,3> triangle(uint32_t z,unsigned fog,unsigned alpha){std::array<gh_static_gs_vertex,3>v{};const unsigned xy[3][2]={{64,64},{320,64},{64,320}};for(unsigned i=0;i<3;i++){v[i].xyz_fixed4[0]=(int32_t)(0x10000u+1792*16+xy[i][0]*16);v[i].xyz_fixed4[1]=(int32_t)(0x20000u+1824*16+xy[i][1]*16);v[i].xyz_fixed4[2]=(int32_t)(0x10000000u+(z<<4)+15);v[i].fog_fixed4=(uint16_t)((fog<<4)|15);v[i].stq[2]=1;for(unsigned c=0;c<3;c++)v[i].rgba[c]=128;v[i].rgba[3]=alpha;}v[0].stq[0]=-.25f;v[0].stq[1]=-.125f;v[1].stq[0]=1.75f;v[1].stq[1]=-.125f;v[2].stq[0]=-.25f;v[2].stq[1]=1.875f;return v;}
static void effect_packets(gh_model_renderer *r,const gh_stage_material &material,uint32_t texture,
 gh_model_draw_state state){
 state.primitive=0x54;state.test=0x5000d;state.alpha=UINT64_C(0x8000000048);state.z_write=0;
 const auto first=triangle(100,255,128);
 auto second=triangle(90,255,64);
 for(auto &v:second)v.rgba[0]=64;
 const gh_static_gs_vertex list[]={first[0],first[1],first[2],second[0],second[1],second[2]};
 const uint32_t indices[]={0,1,2,3,4,5};
 Surface trail(0x314159,50);
 C(!gh_model_renderer_begin(r,0x314159,50));
 C(!gh_model_renderer_draw_trail(r,&state,texture,list,6,indices));
 oracle(trail,material,state,first);
 oracle(trail,material,state,second);
 compare(r,trail,"PRIM54 ordered trail triangles");

 // Only the final triangle of a max-size trail passes depth. A renderer
 // dropping the tail above the old 150-vertex cap leaves the clear color.
 static_assert(GH_EFFECT_TRANS_TRAIL_VERTEX_CAPACITY==174,"EV04 trail capacity");
 constexpr size_t capacity=GH_EFFECT_TRANS_TRAIL_VERTEX_CAPACITY;
 std::array<gh_static_gs_vertex,capacity> long_trail{};
 std::array<uint32_t,capacity> long_indices{};
 const auto behind=triangle(45,255,128);
 for(size_t i=0;i<capacity;++i){
  long_trail[i]=(i<capacity-3?behind:second)[i%3];
  long_indices[i]=(uint32_t)i;
 }
 Surface long_expected(0x314159,50);
 C(!gh_model_renderer_begin(r,0x314159,50));
 C(!gh_model_renderer_draw_trail(r,&state,texture,long_trail.data(),long_trail.size(),long_indices.data()));
 oracle(long_expected,material,state,second);
 compare(r,long_expected,"PRIM54 EV04 29-segment trail");

 // R002's ESP draw resolves a 1024x128 PSMT4 atlas. Sample a texel beyond
 // column 512 so copying or wrapping to the display width fails the test.
 constexpr unsigned atlas_width=1024,atlas_height=128,atlas_x=768,atlas_y=32;
 std::vector<uint8_t> atlas_pixels(atlas_width*atlas_height*4);
 const uint8_t atlas_texel[]={220,100,40,128};
 std::memcpy(atlas_pixels.data()+(atlas_y*atlas_width+atlas_x)*4,atlas_texel,4);
 gh_stage_material atlas={};
 atlas.source=atlas.rgba_gs=atlas_pixels.data();
 atlas.source_bytes=atlas.rgba_bytes=atlas_pixels.size();
 atlas.cache_id=UINT64_MAX;
 atlas.width=atlas_width;atlas.height=atlas_height;atlas.psm=20;atlas.draw_tcc=1;
 uint32_t atlas_texture=0;
 C(!gh_model_renderer_upload(r,&atlas,&atlas_texture));
 state.tex1=0;
 auto atlas_triangle=triangle(100,255,128);
 for(auto &vertex:atlas_triangle){
  vertex.stq[0]=(atlas_x+0.5f)/atlas_width;
  vertex.stq[1]=(atlas_y+0.5f)/atlas_height;
 }
 Surface atlas_expected(0x314159,50);
 C(!gh_model_renderer_begin(r,0x314159,50));
 C(!gh_model_renderer_draw_trail(r,&state,atlas_texture,atlas_triangle.data(),3,indices));
 oracle(atlas_expected,atlas,state,atlas_triangle);
 compare(r,atlas_expected,"PRIM54 wide indexed sprite atlas");

 std::vector<uint8_t> image(512*512*4);
 const uint8_t rgb[]={220,100,40,128};
 constexpr unsigned x=122,y=344;
 std::memcpy(image.data()+(y*512+x)*4,rgb,4);
 gh_stage_material base0={};base0.width=base0.height=512;base0.rgba_gs=image.data();
 auto sample=triangle(100,255,128);
 for(unsigned i=0;i<3;++i){
  const float q=(float)(1u<<i);
  sample[i].stq[0]=(x+0.5f)*q/512;sample[i].stq[1]=(y+0.5f)*q/512;sample[i].stq[2]=q;
 }
 const gh_static_gs_vertex fan[]={sample[0],sample[1],sample[2],sample[2],sample[2],sample[2]};
 state.primitive=0x5d;state.test=0x5000c;state.alpha=UINT64_C(0x8000000044);
 state.tex1=0;state.frame_mask=0xff000000u;
 Surface warp(0x314159,50);
 C(!gh_model_renderer_begin(r,0x314159,50));
 C(!gh_model_renderer_draw_display_warp(r,&state,image.data(),512*448*4,0x80,fan));
 oracle(warp,base0,state,sample);
 compare(r,warp,"projected mode0 warp from display base zero");

 // Bilinear sampling across the CT24/scene FRAME page boundary.
 const uint8_t display_edge[]={220,100,40,128};
 const uint8_t live_scene[]={0x59,0x41,0x31,128};
 for(unsigned column=0;column<512;column++){
  std::memcpy(image.data()+(447*512+column)*4,display_edge,4);
  std::memcpy(image.data()+(448*512+column)*4,live_scene,4);
 }
 for(auto &v:sample){v.stq[0]=(x+0.5f)/512;v.stq[1]=(448.0f+1.0f/32)/512;v.stq[2]=1;}
 const gh_static_gs_vertex alias_fan[]={sample[0],sample[1],sample[2],sample[2],sample[2],sample[2]};
 state.tex1=96;
 Surface alias(0x314159,50);
 C(!gh_model_renderer_begin(r,0x314159,50));
 C(!gh_model_renderer_draw_display_warp(r,&state,image.data(),512*448*4,0x80,alias_fan));
 oracle(alias,base0,state,sample);
 compare(r,alias,"base-zero warp across original display/scene page boundary");
}

static void depth_saturation(gh_model_renderer *r,const gh_stage_material &material,uint32_t texture,
 gh_model_draw_state state){
 constexpr uint32_t indices[]={0,1,2};
 state.primitive=0x5b;
 state.test=0x70000;
 state.alpha=0x44;
 state.z_write=1;
 const auto r003=triangle(0x3dcc9,255,128);
 Surface saturated(0x314159,0xffff);
 C(!gh_model_renderer_begin(r,0x314159,0xffff));
 C(!gh_model_renderer_draw(r,&state,texture,r003.data(),r003.size(),indices,3));
 compare(r,saturated,"PSMZ16S saturates source before GREATER");

 auto mixed=triangle(0x2000,255,128);
 mixed[1].xyz_fixed4[2]=0x22000<<4;
 Surface interpolated(0x314159,0);
 C(!gh_model_renderer_begin(r,0x314159,0));
 C(!gh_model_renderer_draw(r,&state,texture,mixed.data(),mixed.size(),indices,3));
 oracle(interpolated,material,state,mixed);
 compare(r,interpolated,"PSMZ16S saturation follows triangle interpolation");
}
static void display_texel(gh_model_renderer *r,gh_model_draw_state state){
 const uint32_t indices[]={0,1,2};
 state.test=0x507fd;state.alpha=0x44;
 for(unsigned step=0;step<3;step++){
  const uint32_t packed=step==0?0x005faa72:0xffb3318d;
  const uint8_t pixel[]={(uint8_t)packed,(uint8_t)(packed>>8),(uint8_t)(packed>>16),(uint8_t)(packed>>24)};
  gh_stage_material texel={};texel.width=texel.height=1;texel.rgba_gs=pixel;
  auto v=triangle(320,255,step==2?127:160);
  for(auto &vertex:v){vertex.stq[0]=vertex.stq[1]=0;vertex.rgba[0]=64;vertex.rgba[2]=192;}
  state.tex1=step==0?0:96;
  Surface expected(0x16304c,50);C(!gh_model_renderer_begin(r,0x16304c,50));
  C(!gh_model_renderer_draw_display_texel(r,&state,packed,v.data(),v.size(),indices,3));
  oracle(expected,texel,state,v,true);compare(r,expected,"displayBase0/CT32/1x1/TCC0/MODULATE");
  C(expected.z[64*W+64]==(step==2?50u:320u));
 }
}
/* Independent analytic coverage of the two halves of the complete packedXY
 * square. Area=65535^2 exceeds signed32, and outside pixel edges can exceed
 * unsigned32. Exact integer inequalities also exercise shared top-left edges;
 * no copy of the shader's partial products or edge routine is used here. */
static void wide_triangles(gh_model_renderer*r,const gh_stage_material&m,uint32_t texture,const gh_model_draw_state&base){
 const uint32_t indices[3]={0,1,2};const uint32_t offsets[][2]={{28672,29184},{28671,29184},{0,0},{65535,65535},{65535,0},{0,65535}};
 const auto tex=sample(m,.25L,-.125L,false);
 for(const auto&offset:offsets)for(unsigned half=0;half<2;half++)for(unsigned reversed=0;reversed<2;reversed++){
  auto s=base;s.test=0x50003;s.tex1=0;s.alpha=0x44;s.primitive=0x1c;s.texture_function=0;s.fba=s.pabe=0;s.z_write=1;s.xy_offset_x16=offset[0];s.xy_offset_y16=offset[1];
  const uint32_t xy[2][3][2]={{{0,0},{65535,0},{0,65535}},{{65535,65535},{0,65535},{65535,0}}};
  std::array<gh_static_gs_vertex,3>v{};
  for(unsigned i=0;i<3;i++){v[i].xyz_fixed4[0]=(int32_t)(0x10000u+xy[half][i][0]);v[i].xyz_fixed4[1]=(int32_t)(0x20000u+xy[half][i][1]);v[i].xyz_fixed4[2]=(100<<4)|15;v[i].stq[0]=.25f;v[i].stq[1]=-.125f;v[i].stq[2]=1;for(auto&c:v[i].rgba)c=128;}
  if(reversed)std::swap(v[1],v[2]);
  Surface expected(0x314159,0);C(!gh_model_renderer_begin(r,0x314159,0));C(!gh_model_renderer_draw(r,&s,texture,v.data(),v.size(),indices,3));
  for(unsigned y=0;y<H;y++)for(unsigned x=0;x<W;x++){
   int64_t px=(int64_t)offset[0]+x*16,py=(int64_t)offset[1]+y*16;
   bool inside=half?(px<65535&&py<65535&&px+py>=65535):(px>=0&&py>=0&&px+py<65535);
   if(inside){size_t p=(size_t)y*W+x;for(unsigned c=0;c<4;c++)expected.rgba[p*4+c]=(uint8_t)tex[c];expected.z[p]=100;}
  }
  compare(r,expected,"fullPackedXY/analyticHalfPlanes/reversed/sharedEdge/extremeOffsets");
 }
 // Three collinear points must stay empty even when cancelling products
 // exceed signed32. No interpolation or texture sample may be invented.
 for(const auto&offset:offsets){auto s=base;s.xy_offset_x16=offset[0];s.xy_offset_y16=offset[1];s.test=0x50003;
  auto v=triangle(100,255,128);for(unsigned i=0;i<3;i++){uint32_t raw=i==0?0:i==1?65535:32767;v[i].xyz_fixed4[0]=(int32_t)raw;v[i].xyz_fixed4[1]=(int32_t)raw;}
  Surface expected(0x314159,0);C(!gh_model_renderer_begin(r,0x314159,0));C(!gh_model_renderer_draw(r,&s,texture,v.data(),v.size(),indices,3));compare(r,expected,"wideDegenerateCancellation");
 }
}
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
 // HIGHLIGHT2's vertex alpha brightens RGB while output alpha is the sampled
 // texture alpha. Independent analytic coverage/64-bit GS equations compare
 // the whole real-resource surface, including filtering/fog/blend/alpha-test.
 gh_stage_material highlight_material=material;highlight_material.cache_id=3;highlight_material.draw_tfx=3;uint32_t highlight_texture=0;C(!gh_model_renderer_upload(r,&highlight_material,&highlight_texture));
 for(unsigned linear=0;linear<2;linear++)for(unsigned shade_alpha:{0u,64u,128u,255u}){
  auto highlight=state;highlight.texture_function=3;highlight.tex1=linear?96:0;highlight.alpha=0x44;highlight.test=0x50003;
  auto v=triangle(100,128,shade_alpha);v[1].rgba[0]=64;v[2].rgba[1]=192;
  Surface expected_highlight(0x16304c,50);C(!gh_model_renderer_begin(r,0x16304c,50));
  C(!gh_model_renderer_draw(r,&highlight,highlight_texture,v.data(),3,indices,3));oracle(expected_highlight,highlight_material,highlight,v);compare(r,expected_highlight,"HIGHLIGHT2/realRGB/texturealpha/vertexhighlight/filter/fog/blend");
  auto reject=highlight;reject.test=0x507fd;Surface expected_reject(0x16304c,50);C(!gh_model_renderer_begin(r,0x16304c,50));
  C(!gh_model_renderer_draw(r,&reject,highlight_texture,v.data(),3,indices,3));oracle(expected_reject,highlight_material,reject,v);compare(r,expected_reject,"HIGHLIGHT2/alphaTESTusesTexture/notVertex");for(auto z:expected_reject.z)C(z==50);
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
 // The two imported planes participate independently in the next native draw.
 {
  Surface transferred(0x314159, 300);
  const size_t passed = 96 * W + 96, alpha_rejected = passed + 1, depth_rejected = passed + 2;
  transferred.rgba[passed * 4 + 3] = 128;
  transferred.rgba[depth_rejected * 4 + 3] = 128;
  transferred.z[depth_rejected] = 301;
  transferred.z.back() = 0xFEDCBA98u;
  transferred.rgba.back() = 255;
  C(!gh_model_renderer_begin(r, 0, 0));
  C(!gh_model_renderer_replace_surface(r, transferred.rgba.data(), transferred.rgba.size(),
     transferred.z.data(), transferred.z.size()));
  compare(r, transferred, "surface-import/raw-RGBA-and-u32-Z");
  C(gh_model_renderer_replace_surface(r, transferred.rgba.data(), transferred.rgba.size(),
     transferred.z.data(), transferred.z.size() - 1) == GH_MODEL_GPU_ARGUMENT);
  compare(r, transferred, "surface-import/invalid-depth-count-atomic");
  auto draw = state;
  draw.test = 0x5c003; // DATE requires destination alpha bit 7; ZTST is GEQUAL.
  auto vertices = triangle(300, 255, 64);
  const auto initial = transferred;
  C(!gh_model_renderer_draw(r, &draw, texture, vertices.data(), 3, indices, 3));
  oracle(transferred, material, draw, vertices);
  C(transferred.rgba[passed * 4 + 3] == 64);
  C(transferred.rgba[alpha_rejected * 4 + 3] == initial.rgba[alpha_rejected * 4 + 3]);
  C(transferred.rgba[depth_rejected * 4 + 3] == initial.rgba[depth_rejected * 4 + 3]);
  C(transferred.z[depth_rejected] == 301);
  compare(r, transferred, "surface-import/draw-after-DATE-and-GEQUAL");
  std::puts("PASS surface import: full RGBA8/R32_UINT readback, atomic invalid size, DATE and GEQUAL");
 }
 // Original actor opacity prepass writes depth through alpha testing,
 // preserving all four bytes of the existing framebuffer.
 {
  auto ordinary=state;ordinary.test=0x50003;ordinary.alpha=0x44;ordinary.tex1=0;
  auto prepass=ordinary;prepass.test=0x507fd;prepass.alpha=0x8000000044ull;
  prepass.tex1=0xffffff3800000360ull;prepass.frame_mask=UINT32_MAX;
  Surface expected_masked(0x314159,10);C(!gh_model_renderer_begin(r,0x314159,10));
  auto seed=triangle(100,255,128);
  C(!gh_model_renderer_draw(r,&ordinary,texture,seed.data(),3,indices,3));
  oracle(expected_masked,material,ordinary,seed);
  auto original_color=expected_masked.rgba;
  for(unsigned alpha:{128u,127u}){
   auto v=triangle(alpha==128?300:400,255,alpha);
   C(!gh_model_renderer_draw(r,&prepass,texture,v.data(),3,indices,3));
   oracle(expected_masked,material,prepass,v);
  }
  C(expected_masked.rgba==original_color&&expected_masked.z[96*W+96]==300);
  compare(r,expected_masked,"actor-opacity/FRAME-mask-all/alpha-test/depth");
  for(unsigned depth:{250u,300u}){
   auto v=triangle(depth,255,64);v[0].rgba[0]=64;
   C(!gh_model_renderer_draw(r,&ordinary,texture,v.data(),3,indices,3));
   oracle(expected_masked,material,ordinary,v);
   compare(r,expected_masked,"actor-opacity/FRAME-unmask/ZGEQUAL");
  }
 }
 // Same geometry: true ZGEQUAL/order, fractional tint and alpha test thresholds.
 state.tex1=96;state.alpha=0x44;Surface expected(0x102030,0);C(!gh_model_renderer_begin(r,0x102030,0));
 for(unsigned step=0;step<4;step++){auto v=triangle(step==1?99:100,255,step==2?127:128);for(auto&vertex:v)vertex.rgba[0]=step==3?64:128;C(!gh_model_renderer_draw(r,&state,texture,v.data(),3,indices,3));oracle(expected,material,state,v);}
 compare(r,expected,"depthGEQUAL/equality/rejectedfar/alphathreshold");
 // PerspectiveSTQ with varyingQ but constant normalized UV remains exact.
 auto perspective=triangle(200,128,128);for(unsigned i=0;i<3;i++){float q=(float)(1u<<i);perspective[i].stq[0]=q*.25f;perspective[i].stq[1]=q*-.125f;perspective[i].stq[2]=q;}
 C(!gh_model_renderer_draw(r,&state,texture,perspective.data(),3,indices,3));oracle(expected,material,state,perspective);compare(r,expected,"perspectiveQ/rationalUV");
 // A signed Q can cross zero between pixels, where the GS clamps
 // the projected texel index before the texture repeat operation.
 const auto crossing_vertices=[] {
  auto vertices=triangle(200,128,128);
  for(auto &vertex:vertices){
   vertex.stq[0]=0.5f;
   vertex.stq[1]=-0.75f;
   vertex.stq[2]=2.0f;
  }
  vertices[0].stq[2]=-1.0f;
  return vertices;
 }();
 Surface signed_projection(0x102030,0);
 C(!gh_model_renderer_begin(r,0x102030,0));
 C(!gh_model_renderer_draw(r,&state,texture,crossing_vertices.data(),3,indices,3));
 oracle(signed_projection,material,state,crossing_vertices);
 compare(r,signed_projection,"perspectiveQ/signed-crossing/clamp-before-repeat");
 expected=signed_projection;
 auto invalid=perspective;invalid[2].stq[2]=std::numeric_limits<float>::quiet_NaN();C(gh_model_renderer_draw(r,&state,texture,invalid.data(),3,indices,3)==GH_MODEL_GPU_UNSUPPORTED);compare(r,expected,"invalidlastvertexatomic");uint32_t bad[3]={0,1,3};C(gh_model_renderer_draw(r,&state,texture,perspective.data(),3,bad,3)==GH_MODEL_GPU_ARGUMENT);compare(r,expected,"invalidlastindexatomic");
 auto unsupported=state;unsupported.dthe=1;C(gh_model_renderer_draw(r,&unsupported,texture,perspective.data(),3,indices,3)==GH_MODEL_GPU_UNSUPPORTED);compare(r,expected,"unsupportedstateatomic");
 unsupported=state;unsupported.texture_function=2;C(gh_model_renderer_draw(r,&unsupported,texture,perspective.data(),3,indices,3)==GH_MODEL_GPU_UNSUPPORTED);compare(r,expected,"unsupportedTFX2atomic");
 // Recovered ADC/CLIP indices can leave a carry vertex unused. Its invalid
 // Q must not block the real triangles; referencing it remains a strict error.
 std::array<gh_static_gs_vertex,4> carry{};for(unsigned i=0;i<3;i++)carry[i]=perspective[i];carry[3].stq[2]=std::numeric_limits<float>::quiet_NaN();
 Surface carried(0x314159,0);C(!gh_model_renderer_begin(r,0x314159,0));
 C(!gh_model_renderer_draw(r,&state,texture,carry.data(),carry.size(),indices,3));oracle(carried,material,state,perspective);compare(r,carried,"unreferencedInvalidCarry/realIndices");
 const uint32_t used_carry[3]={0,1,3};C(gh_model_renderer_draw(r,&state,texture,carry.data(),carry.size(),used_carry,3)==GH_MODEL_GPU_UNSUPPORTED);compare(r,carried,"referencedInvalidCarryAtomic");
 C(!gh_model_renderer_draw(r,&state,texture,carry.data(),carry.size(),indices,0));compare(r,carried,"noIndicesNoVertexConsumption");expected=carried;
 wide_triangles(r,material,texture,state);
 display_texel(r,state);
 effect_packets(r,material,texture,state);
 depth_saturation(r,material,texture,state);
 // Resume the former image so the existing final failure guard stays atomic.
 C(!gh_model_renderer_begin(r,0x314159,0));C(!gh_model_renderer_draw(r,&state,texture,carry.data(),carry.size(),indices,3));
 C(gh_model_renderer_begin(r,0,65536)==GH_MODEL_GPU_UNSUPPORTED);compare(r,expected,"invalidclearatomic");gh_model_renderer_reset_resources(r);C(gh_model_renderer_draw(r,&state,texture,perspective.data(),3,indices,3)==GH_MODEL_GPU_ARGUMENT);gh_model_renderer_destroy(r);
 std::puts("PASS realR100TM3member37 GPU upload; full512x448 RGBA+Z independenthighprecision analyticcoverage/STQ/GSbilinear/MODULATE+HIGHLIGHT2/texturealpha+vertexhighlight/fog256/alpha48+44/depth/order/packedXYZF2/atomicfailure; explicitnumericfixture, notgameframe/hardwarebitexact");return 0;
}
