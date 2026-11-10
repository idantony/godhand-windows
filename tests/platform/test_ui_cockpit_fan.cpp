#include "../../src/platform/ui_polygon_renderer.h"
#include "../../src/platform/disc.h"
#include <vector>
#include <memory>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <fenv.h>
#include "../../src/assets/tim2.h"
#define C(x) do{if(!(x)){std::fprintf(stderr,"FAIL%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
static int fd(int n,int d){return n>=0?n/d:-((-n+d-1)/d);}
static int lerp(int a,int b,int f){return a+fd((b-a)*f,16);}
static int64_t edge(const int64_t*a,const int64_t*b,int64_t x,int64_t y){return (b[0]-a[0])*(y-a[1])-(b[1]-a[1])*(x-a[0]);}
static bool top(const int64_t*a,const int64_t*b){return b[1]<a[1]||(b[1]==a[1]&&b[0]>a[0]);}
static int texel(const std::vector<unsigned char>&t,int x,int y,unsigned c){int n=t[((y&15)*16+(x&15))*4+c];return c==3?(n==255?128:n/2):n;}
static size_t oracle(std::vector<unsigned char>&dst,const gh_cockpit_fan&q,const std::vector<unsigned char>&t){
 int64_t xy[33][2];uint64_t us[33],vs[33];size_t fragments=0;
 for(unsigned i=0;i<33;i++){xy[i][0]=(int64_t)(q.x[i]*16);xy[i][1]=(int64_t)(q.y[i]*16);us[i]=(uint64_t)(q.u[i]*16*65536.0f);vs[i]=(uint64_t)(q.v[i]*16*65536.0f);}
 for(unsigned tri=0;tri<31;tri++){unsigned a=0,b=tri+1,c=tri+2;int64_t area=edge(xy[a],xy[b],xy[c][0],xy[c][1]);if(!area)continue;if(area<0){std::swap(b,c);area=-area;}
  for(unsigned y=0;y<448;y++)for(unsigned x=0;x<512;x++){
   int64_t wa=edge(xy[b],xy[c],x*16,y*16),wb=edge(xy[c],xy[a],x*16,y*16),wc=edge(xy[a],xy[b],x*16,y*16);
   if(wa<0||wb<0||wc<0||(!wa&&!top(xy[b],xy[c]))||(!wb&&!top(xy[c],xy[a]))||(!wc&&!top(xy[a],xy[b])))continue;
   int u=(int)((us[a]*(uint64_t)wa+us[b]*(uint64_t)wb+us[c]*(uint64_t)wc)/(uint64_t)area)-32768,v=(int)((vs[a]*(uint64_t)wa+vs[b]*(uint64_t)wb+vs[c]*(uint64_t)wc)/(uint64_t)area)-32768;
   int tx=fd(u,65536),ty=fd(v,65536),fu=(u-tx*65536)/4096,fv=(v-ty*65536)/4096,p[4];
   for(unsigned k=0;k<4;k++)p[k]=lerp(lerp(texel(t,tx,ty,k),texel(t,tx+1,ty,k),fu),lerp(texel(t,tx,ty+1,k),texel(t,tx+1,ty+1,k),fu),fv);
   int alpha=p[3];if(alpha<=1)continue;fragments++;size_t o=((size_t)y*512+x)*4;
   for(unsigned k=0;k<3;k++){int rgb=dst[o+k]+fd((p[k]-dst[o+k])*alpha,128);dst[o+k]=(unsigned char)(rgb<0?0:rgb>255?255:rgb);}dst[o+3]=(unsigned char)alpha;
  }
 }return fragments;
}
int main(int argc,char**argv){
 C(argc==2||argc==3);gh_disc*d;C(!gh_disc_open(argv[1],&d));size_t i;gh_disc_entry de;C(!gh_disc_find(d,"etc/core_usa.bin",&i)&&!gh_disc_entry_at(d,i,&de));std::vector<unsigned char>lang((size_t)de.size);C(!gh_disc_read(d,i,0,lang.data(),lang.size()));gh_disc_close(d);
 auto m=std::make_unique<gh_id_state>();gh_bundle b;gh_bundle_entry idd;C(!gh_bundle_open(lang.data(),lang.size(),&b)&&!gh_id_find_language_data(&b,&idd)&&!gh_id_system_init_002acc70(m.get(),&idd));
 unsigned char save[GH_CORE_SAVE_BYTES];gh_core_save_view sv={save,sizeof(save)};C(!gh_core_save_system_init(sv));gh_cockpit_fan fan={};uint32_t f;
 C(!gh_cockpit_fan_prepare(m.get(),sv,&fan,&f)&&!f&&fan.ready&&fan.x[0]==46&&fan.y[0]==394&&fan.x[1]==51&&fan.y[1]==434&&fan.u[32]==0);
 std::vector<unsigned char>texture(1024);C(!gh_cockpit_fan_texture_rgba(&fan.texture,texture.data(),texture.size()));gh_ui_polygon_renderer*r;C(!gh_ui_polygon_create(512,448,&r));uint32_t handle;
 C(gh_ui_polygon_upload(r,&fan.texture,texture.data(),texture.size(),&handle)==GH_UI_UNSUPPORTED);C(!gh_ui_polygon_upload_fan16(r,&fan.texture,texture.data(),texture.size(),&handle));
 gh_ui_sprite_state state={UINT64_C(0x8000000044),0x3001d,0x55,0x60,0,1,1,0,0,0};std::vector<unsigned char>back(512*448*4),gpu(back.size()),cpu;for(size_t k=0;k<back.size();k++)back[k]=(unsigned char)((k*13+k/512)%149);
 size_t total=0;const unsigned points[]={0,500,999};for(unsigned value:points){save[0x1c]=(unsigned char)value;save[0x1d]=(unsigned char)(value>>8);
  C(fesetround(FE_DOWNWARD)==0);C(!gh_cockpit_fan_prepare(m.get(),sv,&fan,&f)&&fegetround()==FE_DOWNWARD);C(fesetround(FE_TONEAREST)==0);
  // Independent double-precision circular geometry, not polynomial replay.
  double ratio=(double)value/1000,theta=.115*ratio;for(unsigned k=2;k<33;k++){double angle=(k-1)*theta,x=(5*std::cos(angle)-40*std::sin(angle))*.9+46,y=5*std::sin(angle)+40*std::cos(angle)+394;C(std::fabs(fan.x[k]-x)<.1&&std::fabs(fan.y[k]-y)<.1);C(std::fabs(fan.u[k]-ratio*(k-1)/31)<1e-6);}
  cpu=back;total+=oracle(cpu,fan,texture);C(!gh_ui_polygon_fan(r,&state,handle,&fan,back.data(),gpu.data(),gpu.size()));if(gpu!=cpu){for(size_t k=0;k<gpu.size();k++)if(gpu[k]!=cpu[k]){std::fprintf(stderr,"mismatch%zu gpu%u cpu%u\n",k,gpu[k],cpu[k]);break;}C(false);}
 }
 C(total>0);auto bad=fan;bad.x[2]=2000;C(gh_ui_polygon_fan(r,&state,handle,&bad,back.data(),gpu.data(),gpu.size())==GH_UI_UNSUPPORTED);state.prim=0x54;C(gh_ui_polygon_fan(r,&state,handle,&fan,back.data(),gpu.data(),gpu.size())==GH_UI_UNSUPPORTED);
 auto prior=fan;save[0x1c]=save[0x1d]=255;C(gh_cockpit_fan_prepare(m.get(),sv,&fan,&f)==GH_ID_UNSUPPORTED&&f==0x1fa080&&!std::memcmp(&prior,&fan,sizeof(fan)));
 auto corrupted=std::vector<unsigned char>(fan.texture.source.bytes,fan.texture.source.bytes+fan.texture.source.size);gh_tim2_picture picture;C(!gh_tim2_first(corrupted.data(),corrupted.size(),&picture));corrupted[(size_t)(picture.clut-corrupted.data())+3]=129;auto invalid=fan.texture;invalid.source.bytes=corrupted.data();C(gh_cockpit_fan_texture_rgba(&invalid,texture.data(),texture.size())==GH_ID_UNSUPPORTED);
 if(argc==3){FILE*out=std::fopen(argv[2],"wbx");C(out&&std::fwrite(cpu.data(),1,cpu.size(),out)==cpu.size()&&!std::fclose(out));}gh_ui_polygon_destroy(r);
 std::printf("PASS actual16TIM2/33vertexHUDfan progress0/.5/.999;3*229376 GPU pixels exactindependentCPU, %zu fragments, doublegeometry/FEnv/statebounds\n",total);return 0;
}
