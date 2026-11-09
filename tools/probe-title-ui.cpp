/* Direct recovered R020 UI/GPU QA. This does not bypass earlier scene callees
 * in the production host or claim that the original scene has been reached. */
#include "../src/rooms/r020_title.h"
#include "../src/platform/mc_renderer.h"
#include <cstdio>
#include <cstdlib>
#include <vector>

struct Probe {
    gh_mc_renderer *renderer=nullptr;
    gh_id_state *id=nullptr;
    unsigned draws=0,begins=0;
    uint32_t frontier=0;
};
static gh_r020_status begin(void *context,const gh_id_base *,uint32_t *)
{auto &p=*(Probe*)context;gh_mc_renderer_begin(p.renderer);++p.begins;return GH_R020_OK;}
static gh_r020_status draw(void *context,const gh_id_base *base,const gh_id_draw_job *job,uint32_t *frontier)
{
    auto &p=*(Probe*)context;gh_id_render_view view={nullptr,nullptr,p.id,base,nullptr};
    gh_id_error error=gh_mc_renderer_draw_view(p.renderer,&view,job);
    if(error){gh_mc_render_status status={};gh_mc_renderer_inspect(p.renderer,&status);
        p.frontier=status.frontier?status.frontier:job->consumer_address;
        *frontier=p.frontier;return GH_R020_ID;}
    ++p.draws;return GH_R020_OK;
}
static bool asset(gh_disc *disc,const char *path,std::vector<unsigned char> &bytes)
{
    size_t index=0;gh_disc_entry entry={};
    if(gh_disc_find(disc,path,&index)||gh_disc_entry_at(disc,index,&entry)||entry.size>SIZE_MAX)return false;
    bytes.resize((size_t)entry.size);
    return gh_disc_read(disc,index,0,bytes.data(),bytes.size())==GH_DISC_OK;
}
int main(int argc,char **argv)
{
    if(argc!=4){std::fprintf(stderr,"usage: probe-title-ui ISO new-frame.rgba new-report.json\n");return 2;}
    gh_disc *disc=nullptr;gh_audio_registry *audio=nullptr;gh_heap *heap=nullptr;
    gh_bgm_requests *bgm=nullptr;gh_id_runtime *runtime=nullptr;gh_r020_title *title=nullptr;
    gh_sound_control control={};gh_dvd_jobs dvd={};gh_screen_fade fade={};Probe p;
    gh_boot_work work={};work.language=1;work.room=0x20;
    gh_game_mt random={};gh_game_mt_seed_0030f2b0(&random,1);
    unsigned char save_bytes[GH_CORE_SAVE_BYTES];gh_core_save_view save={save_bytes,sizeof(save_bytes)};
    gh_scene_hook exit_hook={};std::vector<unsigned char> elf,language,jump;
    gh_r020_title_view final={};gh_r020_status error=GH_R020_OK;int result=1;
    do {
        if(gh_disc_open(argv[1],&disc)||gh_audio_registry_create(&audio)||gh_heap_create(24*1024*1024,&heap))break;
        if(gh_audio_startup_load(audio,disc)||gh_sound_control_from_startup(&control,disc,audio))break;
        elf.resize((size_t)gh_disc_boot_elf_size(disc));
        if(elf.empty()||gh_disc_read_boot_elf(disc,0,elf.data(),elf.size()))break;
        gh_adx_key key={};if(gh_adx_key_from_elf(elf.data(),elf.size(),&key))break;
        if(gh_bgm_requests_create(audio,&control,disc,heap,&bgm)||
           gh_bgm_requests_enable_native(bgm,&key,1,GH_BGM_GAIN_NATIVE_DB_REFERENCE))break;
        if(!asset(disc,"etc/core_usa.bin",language)||!asset(disc,"etc/debugres/RoomJmpTbl.dat",jump))break;
        gh_bundle bundle={};gh_bundle_entry idd={};
        p.id=(gh_id_state*)std::calloc(1,sizeof(*p.id));
        if(!p.id||gh_bundle_open(language.data(),language.size(),&bundle)||gh_id_find_language_data(&bundle,&idd)||
           gh_id_system_init_002acc70(p.id,&idd)||gh_id_runtime_create(p.id,1,&runtime)||gh_core_save_system_init(save))break;
        if(gh_mc_renderer_create(elf.data(),elf.size(),1,&p.renderer)||gh_mc_renderer_enable_reference_polygons(p.renderer,1))break;
        gh_dvd_jobs_init(&dvd,disc);
        gh_r020_title_bindings bindings={};bindings.work=&work;bindings.save=save;bindings.random=&random;
        bindings.dvd=&dvd;bindings.room_heap=heap;bindings.id=p.id;bindings.runtime=runtime;
        bindings.bgm=bgm;bindings.fade=&fade;bindings.room_jump={jump.data(),jump.size(),0};bindings.scenario_exit=&exit_hook;
        if((error=gh_r020_title_init(&bindings,&title))!=GH_R020_OK)break;
        gh_r020_move_callbacks callbacks={&p,begin,draw,nullptr};
        for(unsigned frame=0;frame<100;++frame){
            unsigned previous=p.draws;error=gh_r020_title_move(title,&callbacks);
            if(gh_r020_title_inspect(title,&final)||error)break;
            if(p.draws>previous){result=0;break;}
            gh_screen_fade_context context={512,1824,0,0};gh_fade_draw command={};
            if(gh_screen_fade_step_00307388(&fade,&context,&command))break;
        }
    }while(false);
    if(title)gh_r020_title_inspect(title,&final);
    if(!result){size_t bytes=0;const unsigned char *pixels=gh_mc_renderer_pixels(p.renderer,&bytes);
        FILE *file=std::fopen(argv[2],"wbx");if(!file||!pixels||std::fwrite(pixels,1,bytes,file)!=bytes)result=2;
        if(file&&std::fclose(file))result=2;}
    FILE *report=std::fopen(argv[3],"wbx");if(!report)result=2;
    else {std::fprintf(report,"{\"scope\":\"direct R020 Init/Move UI GPU fixture; explicit black512448 clear; final OT order and earlier scene chain unverified\",\"playable\":false,\"error\":%u,\"frontier\":\"%08X\",\"moves\":%llu,\"begins\":%u,\"draw_jobs\":%u,\"exit_code\":%d}\n",(unsigned)error,p.frontier?p.frontier:final.frontier,(unsigned long long)final.move_serial,p.begins,p.draws,result);if(std::fclose(report))result=2;}
    if(title&&gh_r020_title_destroy(title))result=2;
    gh_mc_renderer_destroy(p.renderer);
    if(runtime&&gh_id_runtime_destroy(runtime))result=2;
    std::free(p.id);gh_bgm_requests_destroy(bgm);
    if(heap&&gh_heap_destroy(heap))result=2;
    gh_audio_registry_destroy(audio);gh_disc_close(disc);return result;
}
