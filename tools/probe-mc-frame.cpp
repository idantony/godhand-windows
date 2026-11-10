/* Bounded actual startup probe through the same reusable renderer as the
 * native app. No replacement UI, successful empty consumer or user input. */
#include "../src/platform/mc_renderer.h"
#include "../src/platform/disc.h"
#include "../src/core/ui_order.h"
#include <cstdio>
#include <cstring>
#include <vector>
struct Record {unsigned entry,kind,address,error,opcode,frontier;size_t glyphs;};
struct Probe {gh_startup_host *host=nullptr;gh_mc_renderer *renderer=nullptr;std::vector<Record> records;std::vector<gh_ui_order_group> groups;bool ot=false;uint64_t flushed=0;};
static gh_id_error begin(void *context)
{auto &p=*(Probe*)context;gh_mc_renderer_begin(p.renderer);p.groups.clear();return GH_ID_OK;}
static gh_id_error draw(void *context,const gh_id_draw_job *job)
{
    auto &p=*(Probe*)context;gh_startup_snapshot s={};gh_mc_render_status before={},after={};
    if(gh_startup_host_snapshot(p.host,&s)!=GH_HOST_OK)return GH_ID_ARGUMENT;
    gh_mc_renderer_inspect(p.renderer,&before);
    gh_id_error e;
    if(p.ot){
        gh_id_render_view view={s.font,s.menu,s.id,s.mc_ui,s.mc_messages};gh_ui_ot_tag tag={};
        e=gh_mc_renderer_queue_view(p.renderer,&view,job,&tag);
        if(e==GH_ID_OK&&tag.index!=SIZE_MAX)p.groups.push_back({tag.bank,(uint32_t)tag.depth,(uint64_t)tag.index});
    }else e=gh_mc_renderer_draw(p.renderer,&s,job);
    gh_mc_renderer_inspect(p.renderer,&after);
    p.records.push_back({job->entry_index,job->entry->kind,job->consumer_address,(unsigned)e,after.opcode,after.frontier,after.glyphs-before.glyphs});
    return e;
}
static gh_id_error post(void *,uint16_t message)
{std::fprintf(stderr,"Unrecovered original postMessage %04X\n",message);return GH_ID_UNSUPPORTED;}
int main(int argc,char **argv)
{
    if(argc!=5&&(argc!=6||std::strcmp(argv[5],"--ot"))){std::fprintf(stderr,"usage: probe-mc-frame ISO native-profile new-output.rgba new-report.json [--ot]\n");return 2;}
    FILE *rgba=std::fopen(argv[3],"wbx");if(!rgba)return 2;
    FILE *report=std::fopen(argv[4],"wbx");if(!report){std::fclose(rgba);std::remove(argv[3]);return 2;}
    Probe p;p.ot=argc==6;gh_disc *disc=nullptr;gh_startup_snapshot s={};int rc=1;
    std::vector<unsigned char> elf;
    do {
        if(gh_disc_open(argv[1],&disc)!=GH_DISC_OK)break;
        elf.resize((size_t)gh_disc_boot_elf_size(disc));
        if(elf.empty()||gh_disc_read_boot_elf(disc,0,elf.data(),elf.size())!=GH_DISC_OK)break;
        if(gh_mc_renderer_create(elf.data(),elf.size(),1,&p.renderer)!=GH_ID_OK)break;
        if(gh_startup_host_create_with_profile(argv[1],argv[2],1,0,64*1024*1024,&p.host)!=GH_HOST_OK)break;
        if(gh_startup_host_set_mc_consumers(p.host,draw,&p,post,nullptr)!=GH_HOST_OK||
           gh_startup_host_set_mc_frame_begin(p.host,begin,&p)!=GH_HOST_OK)break;
        for(unsigned i=0;i<16;i++){
            gh_host_phase phase=gh_startup_host_tick(p.host);
            if(gh_startup_host_snapshot(p.host,&s)!=GH_HOST_OK)break;
            if(p.ot&&s.mc_frame_ready&&s.mc_frame_serial!=p.flushed){
                size_t written=0,omitted=0;std::vector<size_t> order(p.groups.size());uint32_t color=0;
                if(!s.work)break;
                std::memcpy(&color,(const unsigned char*)s.work+0x60c,4);
                if(gh_ui_order_indices(p.groups.data(),p.groups.size(),s.flags,(uint32_t)s.work->mode,
                    order.data(),order.size(),&written,&omitted)!=GH_UI_ORDER_OK||omitted||written!=order.size())break;
                if(gh_mc_renderer_flush_queued(p.renderer,order.data(),order.size(),color)!=GH_ID_OK)break;
                p.flushed=s.mc_frame_serial;
            }
            if(phase!=GH_HOST_RUNNING)break;
        }
        rc=(s.phase==GH_HOST_FRONTIER&&s.frontier_module&&!std::strcmp(s.frontier_module,"mcport.rel")&&
            s.unavailable_entry==0x2e8&&!p.records.empty()&&p.records.back().error==0&&s.mc_frame_serial==1)?0:1;
        if(p.ot&&p.flushed!=1)rc=1;
    }while(false);
    size_t bytes=0;const unsigned char *pixels=gh_mc_renderer_pixels(p.renderer,&bytes);
    bool written=pixels&&std::fwrite(pixels,1,bytes,rgba)==bytes;
    written=std::fclose(rgba)==0&&written;
    const char *scope=p.ot?"actual MC groups in original OT bank/bucket/LIFO order; CT32 RGB Work60C alpha0 clear under explicit FBA0 reference; outer parity not verified":"historical immediate MC submission-order reference with blackalpha128; final OT composition not claimed";
    std::fprintf(report,"{\n\"scope\":\"%s\",\n\"complete_sleep_frame\":%s,\n\"ticks\":%llu,\n\"phase\":%u,\n\"frontier_module\":\"%s\",\n\"frontier\":\"%08X\",\n\"frame_serial\":%llu,\n\"jobs\":[",scope,rc==0?"true":"false",(unsigned long long)s.ticks,(unsigned)s.phase,s.frontier_module?s.frontier_module:"main ELF",s.unavailable_entry,(unsigned long long)s.mc_frame_serial);
    for(size_t i=0;i<p.records.size();i++){
        const auto &r=p.records[i];
        std::fprintf(report,"%s{\"entry\":%u,\"kind\":%u,\"consumer\":\"%08X\",\"error\":%u,\"opcode\":%u,\"frontier\":\"%08X\",\"glyphs\":%zu}",i?",":"",r.entry,r.kind,r.address,r.error,r.opcode,r.frontier,r.glyphs);
    }
    std::fprintf(report,"]\n}\n");if(std::fclose(report)||!written)rc=2;
    if(p.host&&gh_startup_host_destroy(p.host)!=GH_HOST_OK)rc=2;
    gh_mc_renderer_destroy(p.renderer);if(disc)gh_disc_close(disc);
    return rc;
}
