#include "../../src/platform/ui_renderer.h"
#include "../../src/platform/disc.h"
#include "../../src/assets/font_texture.h"
#include <cstdio>
#include <cstdlib>
#include <vector>
#define C(x) do{if(!(x)){std::fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);return 1;}}while(0)
int main(int argc,char **argv){
 C(argc==2||argc==3);gh_disc *d=nullptr;size_t index;gh_disc_entry entry;
 C(gh_disc_open(argv[1],&d)==GH_DISC_OK);C(gh_disc_find(d,"etc/core_usa.bin",&index)==GH_DISC_OK);C(gh_disc_entry_at(d,index,&entry)==GH_DISC_OK);
 std::vector<unsigned char> language((size_t)entry.size),atlas(512*256*4),back(128*96*4),output(back.size());
 C(gh_disc_read(d,index,0,language.data(),language.size())==GH_DISC_OK);gh_disc_close(d);
 gh_bundle pack;gh_font_state font={};C(gh_bundle_open(language.data(),language.size(),&pack)==GH_BUNDLE_OK);C(gh_font_initialize(&font,&pack,1)==GH_FONT_OK);
 C(gh_font_atlas_bank_rgba(&font.slots[0],1,atlas.data(),atlas.size())==GH_FONT_OK);
 gh_font_draw_state draw={1,1,9,512,448,1824,0x80804080};gh_font_quad quad;
 C(gh_font_draw_quad(&font,&draw,1,10,20,0,&quad)==GH_FONT_DRAW_OK);
 for(size_t i=0;i<back.size();i+=4){back[i]=32;back[i+1]=48;back[i+2]=64;back[i+3]=128;}
 gh_ui_gs_state state={UINT64_C(0x8000000044),0x3001d,UINT64_C(0x2000000625420000),0x156,0,0,1,1,0,0,0};
 C(gh_ui_font_quad(&quad,&state,28672,29184,128,96,atlas.data(),back.data(),output.data(),output.size())==GH_UI_OK);
 C(gh_ui_font_quad(&quad,&state,INT32_MIN,29184,128,96,atlas.data(),back.data(),output.data(),output.size())==GH_UI_UNSUPPORTED);
 {unsigned char saved=atlas[3];atlas[3]=3;C(gh_ui_font_quad(&quad,&state,28672,29184,128,96,atlas.data(),back.data(),output.data(),output.size())==GH_UI_UNSUPPORTED);atlas[3]=saved;}
 unsigned changed=0;
 // Independent per-fragment check directly evaluates the recovered GS
 // equation and test for every actual atlas sample and untouched background.
 for(unsigned y=0;y<96;y++)for(unsigned x=0;x<128;x++){
  size_t o=((size_t)y*128+x)*4;unsigned char expected[4]={32,48,64,128};
  if(x>=10&&x<36&&y>=20&&y<52){size_t a=((size_t)(y-20)*512+(x-10+3))*4;unsigned alpha=atlas[a+3]==255?128:atlas[a+3]/2;
   if(alpha>1){for(unsigned c=0;c<3;c++){int tint=c==1?64:128;int src=(atlas[a+c]*tint)/128;int product=(src-expected[c])*(int)alpha;int term=product>=0?product/128:-((-product+127)/128);int value=expected[c]+term;expected[c]=(unsigned char)(value<0?0:value>255?255:value);}expected[3]=(unsigned char)alpha;changed++;}}
  for(unsigned c=0;c<4;c++)C(output[o+c]==expected[c]);
 }
 C(changed>50);state.alpha=0x44;C(gh_ui_font_quad(&quad,&state,28672,29184,128,96,atlas.data(),back.data(),output.data(),output.size())==GH_UI_UNSUPPORTED);
 if(argc==3){FILE *f=std::fopen(argv[2],"wbx");C(f);C(std::fwrite(output.data(),1,output.size(),f)==output.size());C(std::fclose(f)==0);}
 std::printf("PASS hardware D3D11 original glyph quad, GS tint/test/blend; %u covered fragments, all12288pixels matched\n",changed);return 0;
}
