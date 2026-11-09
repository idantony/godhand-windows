#include "../../src/platform/ui_fade_renderer.h"
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <algorithm>
#define C(x)do{if(!(x)){std::fprintf(stderr,"FAIL%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
static int floor128(int n){return n>=0?n/128:-((-n+127)/128);}
int main(int argc,char**argv){C(argc==2||argc==3);std::vector<unsigned char>back(512*448*4),out(back.size());FILE*f=std::fopen(argv[1],"rb");C(f&&std::fread(back.data(),1,back.size(),f)==back.size()&&std::fgetc(f)==EOF&&std::fclose(f)==0);gh_screen_fade s={};gh_screen_fade_context context={512,1824,0,0};gh_ui_fade_context render={512,448,1824,448.0f/480.0f,1,1,0,0,0};gh_ui_fade_renderer*r=nullptr;gh_fade_draw d;uint16_t xy[8];C(gh_ui_fade_create(&r)==GH_UI_OK);
 C(gh_screen_fade_start_00307208(&s,&context,0,10,0,0xff000000,0,15)==GH_FADE_OK);
 for(unsigned frame=0;frame<16;frame++){C(gh_screen_fade_step_00307388(&s,&context,&d)==GH_FADE_OK);C(gh_ui_fade_geometry(&d,&render,xy)==GH_UI_OK);const uint16_t known[]={47396,61405,9947,61405,47396,13351,9947,13355};for(unsigned i=0;i<8;i++)C(xy[i]==known[i]);C(gh_ui_fade_apply(r,&d,&render,back.data(),out.data(),out.size())==GH_UI_OK);unsigned alpha=d.rgba>>24;for(size_t i=0;i<back.size();i+=4){for(unsigned channel=0;channel<3;channel++){int src=(d.rgba>>(channel*8))&255;int expected=std::clamp((int)back[i+channel]+floor128((src-back[i+channel])*(int)alpha),0,255);C(out[i+channel]==expected);}C(out[i+3]==alpha);}}
 // ActualLogoMove alsofades0->80808080; RGBisrawuntexturedGScolor,
 // nottextureMODULATE orCSSalpha. Checksource128RGBnotdoubledto255.
 C(gh_screen_fade_start_00307208(&s,&context,0,10,0,0,0x80808080,0)==GH_FADE_OK);C(gh_screen_fade_step_00307388(&s,&context,&d)==GH_FADE_OK&&gh_ui_fade_apply(r,&d,&render,back.data(),out.data(),out.size())==GH_UI_OK);for(unsigned char b:out)C(b==128);
 auto invalid=render;invalid.gs_y_scale=1;C(gh_ui_fade_apply(r,&d,&invalid,back.data(),out.data(),out.size())==GH_UI_UNSUPPORTED);C(gh_ui_fade_apply(r,&d,&render,back.data(),out.data(),out.size()+4)==GH_UI_ARGUMENT);invalid=render;invalid.width=640;C(gh_ui_fade_apply(r,&d,&invalid,back.data(),out.data(),out.size())==GH_UI_UNSUPPORTED);d.test=0x70000;C(gh_ui_fade_apply(r,&d,&render,back.data(),out.data(),out.size())==GH_UI_UNSUPPORTED);d.test=0x30000;
 if(argc==3){FILE*file=std::fopen(argv[2],"wbx");C(file&&std::fwrite(out.data(),1,out.size(),file)==out.size()&&!std::fclose(file));}
 gh_ui_fade_destroy(r);std::puts("PASS actualNTSCfadepacketgeometrycoversviewport;16frames each229376GPU/independentCPU GSalpha0..255 exact; actualLogo128gray/rawalpha andunsupporteddepth/globalsreject");return 0;}
