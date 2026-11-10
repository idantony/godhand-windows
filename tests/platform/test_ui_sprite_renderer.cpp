#include "../../src/platform/ui_sprite_renderer.h"
#include "../../src/platform/disc.h"
#include <vector>
#include <memory>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#define C(x) do{if(!(x)){std::fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);std::exit(1);}}while(0)
static std::vector<unsigned char> load(gh_disc *d,const char *path){size_t i;gh_disc_entry e;C(gh_disc_find(d,path,&i)==GH_DISC_OK);C(gh_disc_entry_at(d,i,&e)==GH_DISC_OK);std::vector<unsigned char>b((size_t)e.size);C(gh_disc_read(d,i,0,b.data(),b.size())==GH_DISC_OK);return b;}
static int floor_div(int value,int divisor){return value>=0?value/divisor:-((-value+divisor-1)/divisor);}
static int lerp(int a,int b,int f){return a+floor_div((b-a)*f,16);}
static unsigned sample(const std::vector<unsigned char>&pixels,unsigned w,unsigned h,int x,int y,unsigned channel){
 unsigned v=pixels[((size_t)(y&(int)(h-1))*w+(x&(int)(w-1)))*4+channel];return channel==3?(v==255?128:v/2):v;
}
static void oracle(std::vector<unsigned char>&dst,unsigned w,unsigned h,const gh_id_texture_quad&q,const std::vector<unsigned char>&texture){
 int r[4]={(int)(q.x0*16),(int)(q.y0*16),(int)(q.x1*16),(int)(q.y1*16)};
 int st[4]={(int)(q.u0*q.texture.width*65536.0f),(int)(q.v0*q.texture.height*65536.0f),(int)(q.u1*q.texture.width*65536.0f),(int)(q.v1*q.texture.height*65536.0f)};
 for(unsigned y=0;y<h;y++)for(unsigned x=0;x<w;x++){
  if((int)x*16<r[0]||(int)x*16>=r[2]||(int)y*16<r[1]||(int)y*16>=r[3])continue;
  int64_t un=(int64_t)(st[2]-st[0])*((int)x*16-r[0]),ud=r[2]-r[0];int u=st[0]+(int)(un>=0?un/ud:-((-un+ud-1)/ud))-32768;
  int64_t vn=(int64_t)(st[3]-st[1])*((int)y*16-r[1]),vd=r[3]-r[1];int v=st[1]+(int)(vn>=0?vn/vd:-((-vn+vd-1)/vd))-32768;
  int tx=floor_div(u,65536),ty=floor_div(v,65536),fu=(u-tx*65536)/4096,fv=(v-ty*65536)/4096;int filtered[4];
  for(unsigned c=0;c<4;c++)filtered[c]=lerp(lerp((int)sample(texture,q.texture.width,q.texture.height,tx,ty,c),(int)sample(texture,q.texture.width,q.texture.height,tx+1,ty,c),fu),lerp((int)sample(texture,q.texture.width,q.texture.height,tx,ty+1,c),(int)sample(texture,q.texture.width,q.texture.height,tx+1,ty+1,c),fu),fv);
  int alpha=(filtered[3]*(int)(q.color>>24))/128;if(alpha>255)alpha=255;if(alpha<=1)continue;
  size_t o=((size_t)y*w+x)*4;for(unsigned c=0;c<3;c++){int tint=(q.color>>(8*c))&255,src=filtered[c]*tint/128;if(src>255)src=255;int rgb=dst[o+c]+floor_div((src-dst[o+c])*alpha,128);dst[o+c]=(unsigned char)(rgb<0?0:rgb>255?255:rgb);}dst[o+3]=(unsigned char)alpha;
 }
}
int main(int argc,char**argv){
 C(argc==2||argc==3);gh_disc *disc;C(gh_disc_open(argv[1],&disc)==GH_DISC_OK);auto language=load(disc,"etc/core_usa.bin"),idsave=load(disc,"id/local/US/idsavecore_US.idd"),title=load(disc,"id/local/us/idtitlecore_us.idd");gh_disc_close(disc);
 auto manager=std::make_unique<gh_id_state>();gh_bundle lang;gh_bundle_entry idd;
 C(gh_bundle_open(language.data(),language.size(),&lang)==GH_BUNDLE_OK);C(gh_id_find_language_data(&lang,&idd)==GH_MENU_OK);C(gh_id_system_init_002acc70(manager.get(),&idd)==GH_MENU_OK);idd.bytes=idsave.data();idd.size=idsave.size();C(gh_id_set_data_002accf8(manager.get(),11,&idd)==GH_MENU_OK);
 idd.bytes=title.data();idd.size=title.size();C(gh_id_set_data_002accf8(manager.get(),2,&idd)==GH_MENU_OK);
 gh_id_runtime *runtime;gh_id_base object={};int original,reclaimed;C(gh_id_runtime_create(manager.get(),1,&runtime)==GH_ID_OK);C(gh_id_base_initialize_002a9ea0(runtime,&object,11,0,nullptr,nullptr,&original)==GH_ID_OK);C(gh_id_jobs_move(&object,nullptr,nullptr,nullptr)==GH_ID_OK);
 gh_ui_sprite_item items[2]={};C(gh_id_texture_quad_prepare(manager.get(),&object,&object.entries[63],&items[0].quad)==GH_ID_OK);
 // Actual MCworker DispInit/Control values for ID40 entry6. This QA combines
 // the original background and first window; not a claim of fullMC visibility.
 object.entries[6].draw_x=256;object.entries[6].draw_y=310;object.entries[6].draw_scale_x=1;object.entries[6].draw_scale_y=1.4f;
 C(gh_id_texture_quad_prepare(manager.get(),&object,&object.entries[6],&items[1].quad)==GH_ID_OK);
 std::vector<unsigned char> textures[2];gh_ui_sprite_renderer *renderer;C(gh_ui_sprite_create(512,448,&renderer)==GH_UI_OK);
 for(unsigned i=0;i<2;i++){textures[i].resize((size_t)items[i].quad.texture.width*items[i].quad.texture.height*4);C(gh_id_texture_rgba(&items[i].quad.texture,textures[i].data(),textures[i].size())==GH_ID_OK);C(gh_ui_sprite_upload(renderer,&items[i].quad.texture,textures[i].data(),textures[i].size(),&items[i].texture)==GH_UI_OK);}
 gh_ui_sprite_state state={UINT64_C(0x8000000044),0x3001d,0x54,0x60,0,1,1,0,0,0};std::vector<unsigned char> back(512*448*4),expected=back,output(back.size());
 oracle(expected,512,448,items[0].quad,textures[0]);oracle(expected,512,448,items[1].quad,textures[1]);
 C(gh_ui_sprite_batch(renderer,&state,items,2,back.data(),output.data(),output.size())==GH_UI_OK);
 size_t mismatch=0;for(size_t i=0;i<output.size();i++)mismatch+=output[i]!=expected[i];C(!mismatch);
 C(gh_ui_sprite_batch(renderer,&state,items,2,back.data(),output.data(),output.size())==GH_UI_OK);C(output==expected); // cached device/shader/textures reused
 // Real languageTBL1 item47 is the original32x32TIM2 confirmationcursor,
 // notT32. Same15BCF0 pipeline dispatches15E0A0 andlinearPSMT4/CLUT16.
 gh_ui_sprite_item cursor={};object.entries[3].draw_x=396;object.entries[3].draw_y=388;
 C(gh_id_texture_quad_prepare(manager.get(),&object,&object.entries[3],&cursor.quad)==GH_ID_OK);
 C(cursor.quad.texture.width==32&&cursor.quad.texture.height==32&&((cursor.quad.texture.tex0>>20)&63)==20);
 std::vector<unsigned char> cursor_rgba(32*32*4);C(gh_id_texture_rgba(&cursor.quad.texture,cursor_rgba.data(),cursor_rgba.size())==GH_ID_OK);
 C(gh_ui_sprite_upload(renderer,&cursor.quad.texture,cursor_rgba.data(),cursor_rgba.size(),&cursor.texture)==GH_UI_OK);
 auto cursor_expected=output;oracle(cursor_expected,512,448,cursor.quad,cursor_rgba);
 auto previous=output;C(gh_ui_sprite_batch(renderer,&state,&cursor,1,previous.data(),output.data(),output.size())==GH_UI_OK);C(output==cursor_expected);C(output!=previous);
 auto invalid_cursor=cursor.quad.texture;invalid_cursor.tex0^=UINT64_C(1)<<26;uint32_t invalid_handle;
 C(gh_ui_sprite_upload(renderer,&invalid_cursor,cursor_rgba.data(),cursor_rgba.size(),&invalid_handle)==GH_UI_UNSUPPORTED);
 output=previous; // ExistingoptionalT32QAartifact remainsbyte-identical.
 // DecodedoriginalSofdec/ADXlogo atlas, fulltextureassetQAquad. Thisdoesnot
 // pretendtheR020title/BGMcallerhasalreadyrunorchooseitsactualscreenlayout.
 gh_ui_sprite_item logo={};C(gh_id_texture_open(manager.get(),2,7,&logo.quad.texture)==GH_ID_OK);
 C(logo.quad.texture.width==512&&logo.quad.texture.height==512&&((logo.quad.texture.tex0>>20)&63)==20);
 logo.quad.x1=512;logo.quad.y1=448;logo.quad.u1=logo.quad.v1=1;logo.quad.color=0x80808080;logo.quad.color_mode=6;logo.quad.font_mode=5;
 std::vector<unsigned char> logo_rgba(512*512*4);C(gh_id_texture_rgba(&logo.quad.texture,logo_rgba.data(),logo_rgba.size())==GH_ID_OK);C(gh_ui_sprite_upload(renderer,&logo.quad.texture,logo_rgba.data(),logo_rgba.size(),&logo.texture)==GH_UI_OK);
 auto logo_expected=back;oracle(logo_expected,512,448,logo.quad,logo_rgba);C(gh_ui_sprite_batch(renderer,&state,&logo,1,back.data(),output.data(),output.size())==GH_UI_OK&&output==logo_expected);
 auto invalid_logo=logo.quad.texture;invalid_logo.tex0|=UINT64_C(1)<<56;C(gh_ui_sprite_upload(renderer,&invalid_logo,logo_rgba.data(),logo_rgba.size(),&invalid_handle)==GH_UI_UNSUPPORTED);output=previous;
 // Observed realMain TitleLevel firstfade-in entry36@tick292: originalasset
 // and captured native scalar state, not a newmenu/layout or wholeframeclaim.
 gh_ui_sprite_item level={};C(!gh_id_texture_open(manager.get(),2,11,&level.quad.texture));C(level.quad.texture.width==256&&level.quad.texture.height==128);
 level.quad.x0=445-(124*.6f)*.5f;level.quad.x1=445+(124*.6f)*.5f;level.quad.y0=395-(40*.800000191f)*.5f;level.quad.y1=395+(40*.800000191f)*.5f;
 level.quad.u0=.000117192685f;level.quad.v0=.267812282f;level.quad.u1=.487695187f;level.quad.v1=.591581762f;level.quad.color=0x02808080;level.quad.color_mode=6;level.quad.font_mode=5;
 std::vector<unsigned char> level_rgba(256*128*4);C(!gh_id_texture_rgba(&level.quad.texture,level_rgba.data(),level_rgba.size())&&!gh_ui_sprite_upload(renderer,&level.quad.texture,level_rgba.data(),level_rgba.size(),&level.texture));auto level_expected=back;oracle(level_expected,512,448,level.quad,level_rgba);C(!gh_ui_sprite_batch(renderer,&state,&level,1,back.data(),output.data(),output.size())&&output==level_expected&&output!=back);output=previous;
 gh_ui_sprite_item loading={};C(!gh_id_texture_open(manager.get(),1,31,&loading.quad.texture));C(loading.quad.texture.width==32&&loading.quad.texture.height==16&&loading.quad.texture.tex0==UINT64_C(0x115408000));
 loading.quad.x0=-18;loading.quad.x1=532;loading.quad.y0=-50;loading.quad.y1=500;loading.quad.u1=loading.quad.v1=1;loading.quad.color=0x80808080;loading.quad.color_mode=6;loading.quad.font_mode=5;
 std::vector<unsigned char> loading_rgba(32*16*4);C(!gh_id_texture_rgba(&loading.quad.texture,loading_rgba.data(),loading_rgba.size())&&!gh_ui_sprite_upload(renderer,&loading.quad.texture,loading_rgba.data(),loading_rgba.size(),&loading.texture));auto loading_expected=back;oracle(loading_expected,512,448,loading.quad,loading_rgba);C(!gh_ui_sprite_batch(renderer,&state,&loading,1,back.data(),output.data(),output.size())&&output==loading_expected&&output!=back);output=previous;
 gh_ui_sprite_item scroll={};C(!gh_id_texture_open(manager.get(),1,34,&scroll.quad.texture));scroll.quad.x0=286;scroll.quad.x1=542;scroll.quad.y0=337;scroll.quad.y1=465;scroll.quad.u0=2.02999997f;scroll.quad.u1=1.02999997f;scroll.quad.v1=1;scroll.quad.color=0x80808080;scroll.quad.color_mode=6;scroll.quad.font_mode=5;
 std::vector<unsigned char> scroll_rgba((size_t)scroll.quad.texture.width*scroll.quad.texture.height*4);C(!gh_id_texture_rgba(&scroll.quad.texture,scroll_rgba.data(),scroll_rgba.size())&&!gh_ui_sprite_upload(renderer,&scroll.quad.texture,scroll_rgba.data(),scroll_rgba.size(),&scroll.texture));
 for(unsigned variant=0;variant<3;variant++){if(variant==1){scroll.quad.u0=-1.125f;scroll.quad.u1=.875f;scroll.quad.v0=1.75f;scroll.quad.v1=-.25f;}if(variant==2){scroll.quad.u0=3.5f;scroll.quad.u1=-2.5f;scroll.quad.v0=-2;scroll.quad.v1=3;}
  auto scroll_expected=back;oracle(scroll_expected,512,448,scroll.quad,scroll_rgba);C(!gh_ui_sprite_batch(renderer,&state,&scroll,1,back.data(),output.data(),output.size())&&output==scroll_expected&&output!=back);
 }scroll.quad.u0=4.01f;C(gh_ui_sprite_batch(renderer,&state,&scroll,1,back.data(),output.data(),output.size())==GH_UI_UNSUPPORTED);output=previous;
 auto saved=textures[0][3];textures[0][3]=3;uint32_t bad;C(gh_ui_sprite_upload(renderer,&items[0].quad.texture,textures[0].data(),textures[0].size(),&bad)==GH_UI_UNSUPPORTED);textures[0][3]=saved;
 state.tex1=0;C(gh_ui_sprite_batch(renderer,&state,items,2,back.data(),output.data(),output.size())==GH_UI_UNSUPPORTED);
 if(argc==3){FILE*f=std::fopen(argv[2],"wbx");C(f);C(std::fwrite(output.data(),1,output.size(),f)==output.size());C(std::fclose(f)==0);}
 gh_ui_sprite_destroy(renderer);C(gh_id_base_release_002aa078(runtime,&object,&original,&reclaimed)==GH_ID_OK);C(gh_id_runtime_destroy(runtime)==GH_ID_OK);
 std::puts("PASS hardware MC T32 scaled1.4Y, real32x32TIM2cursor,TitlePSMT4Logo+actual256x128Level: each229376pixels exactindependentCPU; invalidTIM2extent/LogoCSA rejected");return 0;
}


