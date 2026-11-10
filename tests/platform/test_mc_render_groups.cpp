#include "../../src/platform/mc_renderer.h"
#include "../../src/core/game_cockpit.h"
#include "../../src/core/ui_order.h"
#include "../../src/platform/disc.h"
#include <vector>
#include <memory>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "../../src/assets/tim2.h"
#include "../../src/core/id_number.h"
#include "../../src/core/id_animation.h"
#define C(x) do{if(!(x)){std::fprintf(stderr,"FAIL%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
struct Fixture {gh_mc_renderer*r;gh_id_render_view view;std::vector<gh_ui_order_group>groups;size_t ids=0,fans=0;gh_id_entry zero={};gh_id_base zero_base={};bool has_zero=false;gh_child_cover_quad cover={};};
static gh_id_error trans(void*p,gh_id_base*b,uint32_t*f){auto&q=*(Fixture*)p;q.view.base=b;gh_id_draw_job jobs[512];size_t count;gh_id_frontier frontier={};gh_id_error e=gh_id_jobs_collect_visible(b,jobs,512,&count,&frontier);if(e){*f=frontier.address;return e;}
 for(size_t i=0;i<count;i++){if(jobs[i].entry->kind==0&&jobs[i].entry->b8a==1&&jobs[i].entry->b8b==12){q.zero=*jobs[i].entry;q.zero_base=*b;q.has_zero=true;}gh_ui_ot_tag tag;e=gh_mc_renderer_queue_view(q.r,&q.view,jobs+i,&tag);if(e){*f=jobs[i].consumer_address;std::fprintf(stderr,"queue base%d index%u kind%u consumer%08X error%d\n",b->id,jobs[i].entry_index,jobs[i].entry->kind,*f,e);return e;}if(tag.index!=SIZE_MAX)q.groups.push_back({tag.bank,(uint32_t)tag.depth,tag.index});q.ids++;}return GH_ID_OK;
}
static gh_id_error fan(void*p,const gh_cockpit_fan*v,uint32_t*f){auto&q=*(Fixture*)p;gh_ui_ot_tag tag;auto temporary=*v;gh_id_error e=gh_mc_renderer_queue_fan(q.r,&temporary,&tag);temporary.x[0]=2000;if(e){*f=0x15c410;return e;}q.groups.push_back({tag.bank,(uint32_t)tag.depth,tag.index});q.fans++;return GH_ID_OK;}
static gh_child_status cover(void*p,const gh_child_cover_quad*v,int*emitted,uint32_t*f){auto&q=*(Fixture*)p;*emitted=0;gh_ui_ot_tag tag;gh_id_error e=gh_mc_renderer_queue_cover(q.r,v,&tag);if(e){*f=0x15b880;return GH_CHILD_UNSUPPORTED;}q.groups.push_back({tag.bank,(uint32_t)tag.depth,tag.index});q.cover=*v;*emitted=1;return GH_CHILD_OK;}
static int fd(int n,int d){return n>=0?n/d:-((-n+d-1)/d);}
static int lerp(int a,int b,int f){return a+fd((b-a)*f,16);}
static int sample(const std::vector<unsigned char>&t,int x,int y,unsigned c){int v=t[((y&127)*128+(x&127))*4+c];return c==3?(v==255?128:v/2):v;}
static size_t number_oracle(std::vector<unsigned char>&dst,const gh_id_texture_quad&q,const std::vector<unsigned char>&tex){
 int left=(int)(q.x0*16),right=(int)(q.x1*16),top=(int)(q.y0*16),bottom=(int)(q.y1*16);C(right>left&&bottom>top);int u0=(int)(q.u0*128*65536),u1=(int)(q.u1*128*65536),v0=(int)(q.v0*128*65536),v1=(int)(q.v1*128*65536);size_t count=0;
 for(int y=0;y<448;y++)for(int x=0;x<512;x++){if(x*16<left||x*16>=right||y*16<top||y*16>=bottom)continue;
  int u=u0+(int)((int64_t)(u1-u0)*(x*16-left)/(right-left))-32768,v=v0+(int)((int64_t)(v1-v0)*(y*16-top)/(bottom-top))-32768,tx=fd(u,65536),ty=fd(v,65536),fu=(u-tx*65536)/4096,fv=(v-ty*65536)/4096,p[4];
  for(unsigned k=0;k<4;k++)p[k]=lerp(lerp(sample(tex,tx,ty,k),sample(tex,tx+1,ty,k),fu),lerp(sample(tex,tx,ty+1,k),sample(tex,tx+1,ty+1,k),fu),fv);
  int alpha=p[3]*(int)(q.color>>24)/128;if(alpha>255)alpha=255;if(alpha<=1)continue;count++;size_t o=((size_t)y*512+x)*4;
  for(unsigned k=0;k<3;k++){int src=p[k]*(int)((q.color>>(k*8))&255)/128;if(src>255)src=255;int value=dst[o+k]+fd((src-dst[o+k])*alpha,128);dst[o+k]=(unsigned char)(value<0?0:value>255?255:value);}dst[o+3]=(unsigned char)alpha;
 }
 return count;
}
int main(int argc,char**argv){
 C(argc==2||argc==3);gh_disc*d;C(!gh_disc_open(argv[1],&d));size_t i;gh_disc_entry de;C(!gh_disc_find(d,"etc/core_usa.bin",&i)&&!gh_disc_entry_at(d,i,&de));std::vector<unsigned char>lang((size_t)de.size);C(!gh_disc_read(d,i,0,lang.data(),lang.size()));std::vector<unsigned char>elf((size_t)gh_disc_boot_elf_size(d));C(!gh_disc_read_boot_elf(d,0,elf.data(),elf.size()));gh_disc_close(d);
 auto manager=std::make_unique<gh_id_state>();gh_bundle language;gh_bundle_entry idd;C(!gh_bundle_open(lang.data(),lang.size(),&language)&&!gh_id_find_language_data(&language,&idd)&&!gh_id_system_init_002acc70(manager.get(),&idd));gh_id_runtime*runtime;C(!gh_id_runtime_create(manager.get(),1,&runtime)&&!gh_id_runtime_set_path_policy(runtime,GH_ID_PATH_REFERENCE_NEAREST));
 auto fonts=std::make_unique<gh_font_state>();C(!gh_font_initialize(fonts.get(),&language,1));gh_menu_state menu={};C(!gh_menu_load_tables_002ae990(&menu,&language));gh_packed_message_state messages={};
 unsigned char bytes[GH_CORE_SAVE_BYTES];gh_core_save_view save={bytes,sizeof(bytes)};gh_boot_work work={};C(!gh_core_save_system_init(save)&&!gh_core_save_game_init(save,&work));gh_game_mt random;gh_game_mt_seed_0030f2b0(&random,1);gh_game_cockpit*c;C(!gh_game_cockpit_create(runtime,manager.get(),&random,&c)&&!gh_game_cockpit_initialize_00140c88(c,save));unsigned char heat[0x18]={};C(!gh_game_cockpit_room_reset_00140de0(c,save,&work,heat)&&!gh_game_cockpit_bind_frame_elf(c,elf.data(),elf.size()));uint32_t flags=0xf00c6000;std::memcpy((unsigned char*)&work+0x5d8,&flags,4);
 Fixture q={};q.view={fonts.get(),&menu,manager.get(),nullptr,&messages};C(!gh_mc_renderer_create(elf.data(),elf.size(),1,&q.r)&&!gh_mc_renderer_enable_reference_polygons(q.r,1));gh_mc_renderer_begin(q.r);gh_cockpit_frame_callbacks callbacks={};callbacks.context=&q;callbacks.trans=trans;callbacks.fan=fan;callbacks.save=save;
 C(!gh_game_cockpit_move_00140e58(c,&work,&callbacks));gh_id_error result=gh_game_cockpit_trans_001410a8(c,&work,&callbacks);if(result)std::fprintf(stderr,"Cockpit %d frontier%08X\n",result,gh_game_cockpit_frontier(c));C(!result&&q.ids==19&&q.fans==1);
 gh_game_children*children;C(!gh_game_children_create(nullptr,&children)&&!gh_game_children_initialize_0015fd98(children)&&!gh_game_children_room_reset_0015fee0(children));gh_render_draw_env env={};C(!gh_render_draw_env_from_elf(&env,elf.data(),elf.size()));uint32_t cover_frontier;C(!gh_game_children_draw_00160b90(children,&work,&env,cover,&q,&cover_frontier));gh_game_children_destroy(children);C(q.cover.rgba==0&&q.cover.test_override==0x3001d);
 gh_screen_fade state={};gh_screen_fade_context context={512,1824,0,0};C(!gh_screen_fade_start_00307208(&state,&context,0,10,0,0xff000000,0xff000000,1));gh_fade_draw draw={};C(!gh_screen_fade_step_00307388(&state,&context,&draw)&&draw.visible);gh_ui_fade_context gpucontext={512,448,1824,448.0f/480.0f,1,1,0,0,0};gh_ui_ot_tag tag;C(!gh_mc_renderer_queue_fade(q.r,&draw,&gpucontext,&tag));q.groups.push_back({tag.bank,(uint32_t)tag.depth,tag.index});
 draw.rgba=0xffffffff;gpucontext.width=0; /* queued scalars/context are owned */
 size_t n,omitted;std::vector<size_t>order(q.groups.size());C(!gh_ui_order_indices(q.groups.data(),q.groups.size(),0,0,order.data(),order.size(),&n,&omitted)&&n==q.groups.size()&&!omitted);
 auto invalid=order;if(invalid.size()>1)invalid[1]=invalid[0];C(gh_mc_renderer_flush_queued(q.r,invalid.data(),invalid.size(),0x123456)==GH_ID_ARGUMENT);
 result=gh_mc_renderer_flush_queued(q.r,order.data(),order.size(),0x123456);gh_mc_render_status status={};gh_mc_renderer_inspect(q.r,&status);if(result)std::fprintf(stderr,"flush %d consumer%08X frontier%08X opcode%04X gpu%u doneGroups%zu\n",result,status.consumer,status.frontier,status.opcode,status.gpu_error,status.jobs);C(!result&&status.jobs==q.groups.size());
 size_t size;const unsigned char*pixels=gh_mc_renderer_pixels(q.r,&size);C(size==512*448*4);for(size_t k=0;k<size;k+=4)C(!pixels[k]&&!pixels[k+1]&&!pixels[k+2]&&pixels[k+3]==255);
 if(argc==3){FILE*out=std::fopen(argv[2],"wbx");C(out&&std::fwrite(pixels,1,size,out)==size&&!std::fclose(out));}
 gh_mc_renderer_begin(q.r);auto badcover=q.cover;badcover.rgba=0x02000000;C(gh_mc_renderer_queue_cover(q.r,&badcover,&tag)==GH_ID_UNSUPPORTED);badcover=q.cover;badcover.test_override=0x30000;C(gh_mc_renderer_queue_cover(q.r,&badcover,&tag)==GH_ID_UNSUPPORTED);C(!gh_mc_renderer_queue_cover(q.r,&q.cover,&tag)&&tag.index!=SIZE_MAX);badcover=q.cover;badcover.rgba=0x01000000;C(!gh_mc_renderer_queue_cover(q.r,&badcover,&tag));size_t one=0;size_t two[]={0,1};C(!gh_mc_renderer_flush_queued(q.r,two,2,0x654321));pixels=gh_mc_renderer_pixels(q.r,&size);for(size_t k=0;k<size;k+=4)C(pixels[k]==0x21&&pixels[k+1]==0x43&&pixels[k+2]==0x65&&pixels[k+3]==0);
 // Separate real zero-width/zero-height identity paths: actualTIM2/T8 material
 // validated, current canonical geometry consumed. Positive unknown profile,
 // foreign material, rotation/f80 cannot be hidden by this optimization.
 C(q.has_zero);gh_mc_renderer_begin(q.r);q.view.base=&q.zero_base;gh_id_entry z=q.zero;gh_id_draw_job job={z.source_index,&z,0x2aaef0};std::vector<unsigned char>before(gh_mc_renderer_pixels(q.r,&size),gh_mc_renderer_pixels(q.r,&size)+size);
 z.draw_scale_x=0;z.draw_scale_y=1;C(!gh_mc_renderer_draw_view(q.r,&q.view,&job));C(!std::memcmp(before.data(),gh_mc_renderer_pixels(q.r,&size),size));
 z.draw_scale_x=1;z.draw_scale_y=0;C(!gh_mc_renderer_draw_view(q.r,&q.view,&job)&&!std::memcmp(before.data(),gh_mc_renderer_pixels(q.r,&size),size));
 z.draw_scale_y=1;C(gh_mc_renderer_draw_view(q.r,&q.view,&job)==GH_ID_UNSUPPORTED&&!std::memcmp(before.data(),gh_mc_renderer_pixels(q.r,&size),size));z.draw_scale_x=0;z.f80=2;C(gh_mc_renderer_draw_view(q.r,&q.view,&job)==GH_ID_UNSUPPORTED);z.f80=1;z.draw_rotation=.1f;C(gh_mc_renderer_draw_view(q.r,&q.view,&job)==GH_ID_UNSUPPORTED);z.draw_rotation=0;
 auto foreign=std::make_unique<gh_id_state>(*manager);const unsigned char bad[]={1,0,0,0,8,0,0,0,'N','O','P','E'};foreign->pair[1].texture_data={};foreign->pair[1].texture_data.bytes=bad;foreign->pair[1].texture_data.size=sizeof(bad);q.view.manager=foreign.get();z.b8b=0;C(gh_mc_renderer_draw_view(q.r,&q.view,&job)!=GH_ID_OK);
 // Isolated actualID19 Move establishes positive digit geometry. This is an
 // API/unit consumer QA, not an extra Move inserted into the R020 frame.
 gh_id_base*money;C(!gh_game_cockpit_base(c,0x1ae0,&money));gh_id_animation_profile profile={1,0};C(!gh_id_jobs_move_reference(money,nullptr,nullptr,&profile,nullptr));q.view.manager=manager.get();q.view.base=money;gh_id_entry digit=money->entries[3];digit.seed=0;gh_id_texture_quad quads[10];size_t digits;uint32_t frontier;C(!gh_id_number_prepare(manager.get(),money,&digit,quads,10,&digits,&frontier)&&digits==1);std::vector<unsigned char>tex(128*128*4);C(!gh_id_number_texture_rgba(&quads[0].texture,tex.data(),tex.size()));
 gh_mc_renderer_begin(q.r);job={digit.source_index,&digit,0x2aaff0};gh_ui_ot_tag tags[10];size_t written=99;C(!gh_mc_renderer_queue_view_groups(q.r,&q.view,&job,tags,10,&written)&&written==1);std::vector<unsigned char>expected(512*448*4);for(size_t k=0;k<expected.size();k+=4){expected[k]=0x30;expected[k+1]=0x20;expected[k+2]=0x10;}size_t numeric_pixels=number_oracle(expected,quads[0],tex);C(numeric_pixels>0&&!gh_mc_renderer_flush_queued(q.r,&one,1,0x102030));pixels=gh_mc_renderer_pixels(q.r,&size);C(!std::memcmp(expected.data(),pixels,size));
 gh_mc_renderer_begin(q.r);digit.seed=103;written=99;C(gh_mc_renderer_queue_view_groups(q.r,&q.view,&job,tags,2,&written)==GH_ID_CAPACITY&&written==99);C(!gh_mc_renderer_queue_view_groups(q.r,&q.view,&job,tags,10,&written)&&written==3);C(!gh_id_number_prepare(manager.get(),money,&digit,quads,10,&digits,&frontier)&&digits==3);size_t reversed[]={2,1,0};C(!gh_mc_renderer_flush_queued(q.r,reversed,3,0x102030));
 for(size_t k=0;k<expected.size();k+=4){expected[k]=0x30;expected[k+1]=0x20;expected[k+2]=0x10;expected[k+3]=0;}for(int k=2;k>=0;k--)numeric_pixels+=number_oracle(expected,quads[k],tex);pixels=gh_mc_renderer_pixels(q.r,&size);C(!std::memcmp(expected.data(),pixels,size));
 gh_mc_renderer_destroy(q.r);C(!gh_game_cockpit_destroy(c)&&!gh_id_runtime_destroy(runtime));std::printf("PASS actualR020 19IDcalls+fan+Cover+Fade sorted%zu nativegroups (%zu NULLreturns), actualGPUfan/Fade; zero/cover-discard+positiveunknown; numeric0/103GPU %zu samples exactCPU/pluralatomic\n",q.groups.size(),22-q.groups.size(),numeric_pixels);return 0;
}
