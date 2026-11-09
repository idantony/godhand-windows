#include "../../src/platform/ui_polygon_renderer.h"
#include "../../src/platform/disc.h"
#include <vector>
#include <memory>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <fenv.h>
#include "../../src/core/id_animation.h"
#define C(x) do{if(!(x)){std::fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);std::exit(1);}}while(0)
static std::vector<unsigned char> load(gh_disc *d,const char *path){size_t i;gh_disc_entry e;C(gh_disc_find(d,path,&i)==GH_DISC_OK);C(gh_disc_entry_at(d,i,&e)==GH_DISC_OK);std::vector<unsigned char>b((size_t)e.size);C(gh_disc_read(d,i,0,b.data(),b.size())==GH_DISC_OK);return b;}
static int floor_div(int value,int divisor){return value>=0?value/divisor:-((-value+divisor-1)/divisor);}
static int lerp(int a,int b,int f){return a+floor_div((b-a)*f,16);}
static unsigned sample(const std::vector<unsigned char>&pixels,unsigned w,unsigned h,int x,int y,unsigned channel){
 unsigned v=pixels[((size_t)(y&(int)(h-1))*w+(x&(int)(w-1)))*4+channel];return channel==3?(v==255?128:v/2):v;
}

static int64_t edge(const int64_t*a,const int64_t*b,int64_t x,int64_t y){return (b[0]-a[0])*(y-a[1])-(b[1]-a[1])*(x-a[0]);}
static bool top(const int64_t*a,const int64_t*b){return b[1]<a[1]||(b[1]==a[1]&&b[0]>a[0]);}
static size_t oracle(std::vector<unsigned char>&dst,const gh_id_texture_polygon&q,const std::vector<unsigned char>&texture){
 int64_t xy[4][2];uint64_t us[4],vs[4];size_t changed=0;
 for(unsigned i=0;i<4;i++){xy[i][0]=(int64_t)(q.x[i]*16);xy[i][1]=(int64_t)(q.y[i]*16);us[i]=(uint64_t)(q.u[i]*64*65536.0f);vs[i]=(uint64_t)(q.v[i]*64*65536.0f);}
 for(unsigned y=0;y<448;y++)for(unsigned x=0;x<512;x++){uint64_t u=0,v=0;bool hit=false;
  for(unsigned triangle=0;triangle<2&&!hit;triangle++){unsigned a=triangle?1:0,b=triangle?3:1,c=2;int64_t area=edge(xy[a],xy[b],xy[c][0],xy[c][1]);if(area<0){std::swap(b,c);area=-area;}
   int64_t wa=edge(xy[b],xy[c],x*16,y*16),wb=edge(xy[c],xy[a],x*16,y*16),wc=edge(xy[a],xy[b],x*16,y*16);
   if(wa<0||wb<0||wc<0||(!wa&&!top(xy[b],xy[c]))||(!wb&&!top(xy[c],xy[a]))||(!wc&&!top(xy[a],xy[b])))continue;
   u=(us[a]*(uint64_t)wa+us[b]*(uint64_t)wb+us[c]*(uint64_t)wc)/(uint64_t)area;v=(vs[a]*(uint64_t)wa+vs[b]*(uint64_t)wb+vs[c]*(uint64_t)wc)/(uint64_t)area;hit=true;
  }if(!hit)continue;
  int uf=(int)u-32768,vf=(int)v-32768,tx=floor_div(uf,65536),ty=floor_div(vf,65536),fu=(uf-tx*65536)/4096,fv=(vf-ty*65536)/4096;int filtered[4];
  for(unsigned c=0;c<4;c++)filtered[c]=lerp(lerp((int)sample(texture,64,64,tx,ty,c),(int)sample(texture,64,64,tx+1,ty,c),fu),lerp((int)sample(texture,64,64,tx,ty+1,c),(int)sample(texture,64,64,tx+1,ty+1,c),fu),fv);
  int alpha=filtered[3]*(int)(q.color>>24)/128;if(alpha>255)alpha=255;if(alpha<=1)continue;changed++;size_t o=((size_t)y*512+x)*4;
  for(unsigned c=0;c<3;c++){int src=filtered[c]*(int)((q.color>>(8*c))&255)/128;if(src>255)src=255;int value=dst[o+c]+floor_div((src-dst[o+c])*alpha,128);dst[o+c]=(unsigned char)(value<0?0:value>255?255:value);}dst[o+3]=(unsigned char)alpha;
 }
 return changed;
}
int main(int argc,char**argv){C(argc==2||argc==3);gh_disc*d;C(gh_disc_open(argv[1],&d)==GH_DISC_OK);auto language=load(d,"etc/core_usa.bin");gh_disc_close(d);
 auto manager=std::make_unique<gh_id_state>();gh_bundle bundle;gh_bundle_entry idd;C(gh_bundle_open(language.data(),language.size(),&bundle)==GH_BUNDLE_OK&&gh_id_find_language_data(&bundle,&idd)==GH_MENU_OK&&gh_id_system_init_002acc70(manager.get(),&idd)==GH_MENU_OK);
 gh_id_runtime*r;gh_id_base b={};int ret,reclaimed;C(gh_id_runtime_create(manager.get(),1,&r)==GH_ID_OK&&gh_id_runtime_set_path_policy(r,GH_ID_PATH_REFERENCE_NEAREST)==GH_ID_OK);C(gh_id_base_initialize_002a9ea0(r,&b,1,14,nullptr,nullptr,&ret)==GH_ID_OK&&ret==1);
 gh_id_animation_profile profile={1,0};C(gh_id_jobs_move_reference(&b,nullptr,nullptr,&profile,nullptr)==GH_ID_OK);gh_ui_polygon_item items[2]={};gh_id_entry actual=b.entries[2];C(actual.b89==3&&actual.draw_rotation>2.35f&&actual.draw_rotation<2.36f);
 // Originalentry texture/size/angle/anchor/UV; explicittranslation only for
 // assetQA visibility, not a claim of actualCockpit frame/visibility policy.
 actual.draw_x=200;actual.draw_y=240;C(gh_id_texture_polygon_prepare(manager.get(),&b,&actual,&items[0].polygon)==GH_ID_OK);C(items[0].polygon.texture.width==64&&items[0].polygon.alignment==3);
 {double width=(double)actual.f68*actual.draw_scale_x,height=(double)actual.f6c*actual.draw_scale_y;const double localx[]={0,width,0,width},localy[]={-height,-height,0,0};for(unsigned i=0;i<4;i++){double x=201+std::cos(actual.draw_rotation)*localx[i]-std::sin(actual.draw_rotation)*localy[i],y=241+std::sin(actual.draw_rotation)*localx[i]+std::cos(actual.draw_rotation)*localy[i];C(std::fabs(items[0].polygon.x[i]-x)<0.001&&std::fabs(items[0].polygon.y[i]-y)<0.001);}} // mathematicalgeometryoracle only; productionusesoriginalpolynomial
 {fenv_t saved;C(fegetenv(&saved)==0);const int modes[]={FE_TONEAREST,FE_TOWARDZERO,FE_UPWARD,FE_DOWNWARD};for(int mode:modes){C(fesetround(mode)==0&&feraiseexcept(FE_DIVBYZERO)==0);int flags=fetestexcept(FE_ALL_EXCEPT);gh_id_texture_polygon q;C(gh_id_texture_polygon_prepare(manager.get(),&b,&actual,&q)==GH_ID_OK);C(!std::memcmp(q.x,items[0].polygon.x,sizeof(q.x))&&!std::memcmp(q.y,items[0].polygon.y,sizeof(q.y)));C(fegetround()==mode&&fetestexcept(FE_ALL_EXCEPT)==flags);}C(fesetenv(&saved)==0);}
 auto e=actual;e.b89=0;e.draw_scale_x=e.draw_scale_y=1;e.draw_rotation=0;e.draw_x=320;e.draw_y=200;C(gh_id_texture_polygon_prepare(manager.get(),&b,&e,&items[1].polygon)==GH_ID_OK);
 C(items[1].polygon.x[0]==301&&items[1].polygon.y[0]==181&&items[1].polygon.x[3]==341&&items[1].polygon.y[3]==221);
 e.b89=2;C(gh_id_texture_polygon_prepare(manager.get(),&b,&e,&items[1].polygon)==GH_ID_UNSUPPORTED);
 e=actual;e.draw_scale_x=e.draw_scale_y=1;e.draw_rotation=1.5707963705062866f;gh_id_texture_polygon quarter;C(gh_id_texture_polygon_prepare(manager.get(),&b,&e,&quarter)==GH_ID_OK);C(quarter.x[0]==241&&quarter.y[0]==241&&quarter.x[1]==241&&quarter.y[1]==281&&quarter.x[2]==201&&quarter.y[2]==241&&quarter.x[3]==201&&quarter.y[3]==281);
 std::vector<unsigned char> texture(64*64*4);C(gh_id_polygon_texture_rgba(&items[0].polygon.texture,texture.data(),texture.size())==GH_ID_OK);gh_ui_polygon_renderer*renderer;C(gh_ui_polygon_create(640,448,&renderer)==GH_UI_UNSUPPORTED&&renderer==nullptr);C(gh_ui_polygon_create(512,448,&renderer)==GH_UI_OK);C(gh_ui_polygon_upload(renderer,&items[0].polygon.texture,texture.data(),texture.size(),&items[0].texture)==GH_UI_OK);items[1].texture=items[0].texture;
 gh_ui_sprite_state state={UINT64_C(0x8000000044),0x3001d,0x54,0x60,0,1,1,0,0,0};std::vector<unsigned char> back(512*448*4),expected,output(back.size());for(size_t i=0;i<back.size();i++)back[i]=(unsigned char)((i*17+i/512)%131);
 size_t count=0;for(unsigned pass=0;pass<4;pass++){expected=back;for(unsigned i=0;i<2;i++)count+=oracle(expected,items[i].polygon,texture);C(gh_ui_polygon_batch(renderer,&state,items,2,back.data(),output.data(),output.size())==GH_UI_OK);if(output!=expected){for(size_t j=0;j<output.size();j++)if(output[j]!=expected[j]){std::fprintf(stderr,"mismatchbyte%zu actual%u expected%u\n",j,output[j],expected[j]);break;}C(false);}}
 C(count>1000);auto bad=items[0];bad.polygon.x[0]=2000;C(gh_ui_polygon_batch(renderer,&state,&bad,1,back.data(),output.data(),output.size())==GH_UI_UNSUPPORTED);state.test=0x70000;C(gh_ui_polygon_batch(renderer,&state,items,2,back.data(),output.data(),output.size())==GH_UI_UNSUPPORTED);unsigned h;texture[3]=3;C(gh_ui_polygon_upload(renderer,&items[0].polygon.texture,texture.data(),texture.size(),&h)==GH_UI_UNSUPPORTED);
 if(argc==3){FILE*f=std::fopen(argv[2],"wbx");C(f&&std::fwrite(expected.data(),1,expected.size(),f)==expected.size()&&std::fclose(f)==0);}
 gh_ui_polygon_destroy(renderer);C(gh_id_base_release_002aa078(r,&b,&ret,&reclaimed)==GH_ID_OK&&reclaimed&&gh_id_runtime_destroy(r)==GH_ID_OK);std::printf("PASS actualHUD64TIM2 angle2.35318/anchor3 andzeroangle0: %zu fragments,4*229376 GPU pixels exactindependentCPU; bounds/oddalpha reject\n",count);return 0;
}
