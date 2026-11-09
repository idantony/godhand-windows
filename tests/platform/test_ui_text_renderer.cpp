#include "../../src/platform/ui_text_renderer.h"
#include "../../src/assets/font_texture.h"
#include "../../src/platform/disc.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <algorithm>
#define C(x) do{if(!(x)){std::fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);std::exit(1);}}while(0)
static std::vector<unsigned char> asset(gh_disc*d,const char*path){size_t i;gh_disc_entry e;C(gh_disc_find(d,path,&i)==GH_DISC_OK);C(gh_disc_entry_at(d,i,&e)==GH_DISC_OK);std::vector<unsigned char>b((size_t)e.size);C(gh_disc_read(d,i,0,b.data(),b.size())==GH_DISC_OK);return b;}
static int floor128(int x){return x>=0?x/128:-((-x+127)/128);}
int main(int argc,char**argv){
 C(argc==2||argc==3);gh_disc*d;C(gh_disc_open(argv[1],&d)==GH_DISC_OK);
 auto language=asset(d,"etc/core_usa.bin"),save=asset(d,"id/local/US/idsavecore_US.idd");
 gh_bundle lang;gh_bundle_entry idd;gh_font_state font={};gh_menu_state menu={};gh_packed_message_state packed={};gh_id_base object={};auto*manager=new gh_id_state{};gh_id_runtime*runtime;int original,reclaimed;
 C(gh_bundle_open(language.data(),language.size(),&lang)==GH_BUNDLE_OK);C(gh_font_initialize(&font,&lang,1)==GH_FONT_OK);C(gh_menu_load_tables_002ae990(&menu,&lang)==GH_MENU_OK);C(gh_id_find_language_data(&lang,&idd)==GH_MENU_OK);C(gh_id_system_init_002acc70(manager,&idd)==GH_MENU_OK);idd.bytes=save.data();idd.size=save.size();C(gh_id_set_data_002accf8(manager,11,&idd)==GH_MENU_OK);C(gh_id_runtime_create(manager,1,&runtime)==GH_ID_OK);C(gh_id_base_initialize_002a9ea0(runtime,&object,11,0,nullptr,nullptr,&original)==GH_ID_OK);C(gh_packed_message_bind(&object,manager,&font,&packed,&lang,1,11,1,&original)==GH_ID_OK);
 std::vector<unsigned char> elf((size_t)gh_disc_boot_elf_size(d));C(!elf.empty());C(gh_disc_read_boot_elf(d,0,elf.data(),elf.size())==GH_DISC_OK);gh_id_text_context context={1,512,448,1824,{}};C(gh_id_text_palette_from_elf(elf.data(),elf.size(),&context.palette)==GH_ID_OK);
 auto&e=object.entries[0];e.message=0x2007;e.draw_x=39;e.draw_y=213;e.draw_scale_x=.849999726f;e.draw_scale_y=.899999976f;
 gh_id_text_glyph glyphs[512];gh_id_text_result result;C(gh_id_text_prepare(&object,&e,&menu,&packed,&font,&context,glyphs,512,&result)==GH_ID_OK);C(result.glyph_count==165&&result.line_count==5);
 std::vector<unsigned char> banks[2];for(unsigned i=0;i<2;i++){banks[i].resize(512*256*4);C(gh_font_atlas_bank_rgba(&font.slots[0],i,banks[i].data(),banks[i].size())==GH_FONT_OK);}
 const unsigned width=512,height=448;std::vector<unsigned char> back(width*height*4),out(back.size()),expected(back.size());for(size_t p=0;p<back.size();p+=4){back[p]=20;back[p+1]=30;back[p+2]=40;back[p+3]=128;}expected=back;
 // Independent CPU oracle: uint64 rational UV, scalar signed-floor GS blend.
 size_t writes=0;for(size_t i=0;i<result.glyph_count;i++){const auto&q=glyphs[i].quad;int x0=(int)q.x0-28672,y0=(int)q.y0-29184,x1=(int)q.x1-28672,y1=(int)q.y1-29184;unsigned tint[]={q.rgba&255,(q.rgba>>8)&255,(q.rgba>>16)&255,q.rgba>>24};
  for(int y=0;y<(int)height;y++)for(int x=0;x<(int)width;x++){if(x*16<x0||x*16>=x1||y*16<y0||y*16>=y1)continue;
   uint64_t u=((uint64_t)q.u0<<12)+(((uint64_t)(q.u1-q.u0)<<12)*(unsigned)(x*16-x0))/(unsigned)(x1-x0),v=((uint64_t)q.v0<<12)+(((uint64_t)(q.v1-q.v0)<<12)*(unsigned)(y*16-y0))/(unsigned)(y1-y0);
   const unsigned char*t=banks[q.palette_parity].data()+(((v>>16)&255)*512+((u>>16)&511))*4;int a=((t[3]==255?128:t[3]/2)*(int)tint[3])/128;if(a<=1)continue;auto*p=expected.data()+((size_t)y*width+x)*4;
   for(unsigned c=0;c<3;c++){int src=std::min(255,(int)(t[c]*tint[c]/128));p[c]=(unsigned char)std::max(0,std::min(255,(int)p[c]+floor128((src-p[c])*a)));}p[3]=(unsigned char)a;writes++;
  }
 }
 gh_ui_text_renderer*r=nullptr;C(gh_ui_text_create(&font.slots[0],width,height,&r)==GH_UI_OK);gh_ui_gs_state state={};state.alpha=UINT64_C(0x8000000044);state.test=0x3001d;state.tex0=UINT64_C(0x2000000625420000);state.prim=0x156;state.colclamp=state.prmodecont=1;
 C(gh_ui_text_batch(r,&state,28672,29184,glyphs,result.glyph_count,back.data(),out.data(),out.size())==GH_UI_OK);for(size_t i=0;i<out.size();i++)if(out[i]!=expected[i]){std::fprintf(stderr,"pixel mismatch byte%zu gpu%u cpu%u\n",i,out[i],expected[i]);return 1;}C(writes>1000);
 C(gh_ui_text_batch(r,&state,28672,29184,glyphs,result.glyph_count,back.data(),out.data(),out.size())==GH_UI_OK);C(out==expected);state.tex1=0x60;C(gh_ui_text_batch(r,&state,28672,29184,glyphs,result.glyph_count,back.data(),out.data(),out.size())==GH_UI_UNSUPPORTED);state.tex1=0;state.tex0^=UINT64_C(1)<<26;C(gh_ui_text_batch(r,&state,28672,29184,glyphs,result.glyph_count,back.data(),out.data(),out.size())==GH_UI_UNSUPPORTED);state.tex0^=UINT64_C(1)<<26;C(gh_ui_text_batch(r,&state,INT32_MIN,29184,glyphs,result.glyph_count,back.data(),out.data(),out.size())==GH_UI_UNSUPPORTED);
 if(argc==3){FILE*f=std::fopen(argv[2],"wbx");C(f);C(std::fwrite(out.data(),1,out.size(),f)==out.size());C(std::fclose(f)==0);}
 std::printf("PASS actual message2007: 165 glyphs/5 lines, %u pixels GPU/CPU exact, %zu fragment writes, cached repeat and nearest-state rejection\n",width*height,writes);gh_ui_text_destroy(r);C(gh_id_base_release_002aa078(runtime,&object,&original,&reclaimed)==GH_ID_OK);C(gh_id_runtime_destroy(runtime)==GH_ID_OK);delete manager;gh_disc_close(d);return 0;
}
