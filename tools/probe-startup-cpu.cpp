/* Developer replay of the native scheduler and packet producers. */
#include "../tests/platform/startup_cpu_sink.h"
#include "../src/platform/startup_qa_repeat_square.h"
#include "../src/platform/startup_qa_title.h"
#include "../src/platform/disc.h"
#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <vector>

static int fail(const char *operation)
{
    std::fprintf(stderr,"CPU replay failed during %s\n",operation);
    return 2;
}

static bool load_input_map(const char *iso,std::vector<uint8_t> &elf,gh_native_input_map &map)
{
    gh_disc *opened=nullptr;
    if(gh_disc_open(iso,&opened)!=GH_DISC_OK)return false;
    const std::unique_ptr<gh_disc,decltype(&gh_disc_close)> disc(opened,gh_disc_close);
    const uint64_t bytes=gh_disc_boot_elf_size(disc.get());
    if(!bytes||bytes>elf.max_size())return false;
    elf.resize(static_cast<size_t>(bytes));
    return gh_disc_read_boot_elf(disc.get(),0,elf.data(),elf.size())==GH_DISC_OK&&
           gh_native_input_map_from_elf(elf.data(),elf.size(),1,&map);
}

static int replay(gh_startup_host *host,fixture_sink &sink,const char *ffmpeg,gh_native_input &input,
    uint64_t max_ticks,bool controlled_r003)
{
    if(!configure_cpu_sink(host,&sink,ffmpeg))return fail("consumer setup");
    QAStartupTitleProbe title;
    QARepeatSquareProbe combat;
    combat.enabled=!controlled_r003;
    combat.triangle=combat.pair=combat.continue_after_pair=combat.packet_only=true;

    gh_startup_snapshot snapshot={};
    if(gh_startup_host_snapshot(host,&snapshot)!=GH_HOST_OK)return fail("initial snapshot");
    const auto started=std::chrono::steady_clock::now();
    uint64_t completed_frames=0,r003_handoff=0,r003_frames=0;
    int r003_cut=-1;
    while(snapshot.ticks<max_ticks&&snapshot.phase==GH_HOST_RUNNING&&!combat.complete()){
        title.sample(snapshot,input,true,false);
        const gh_native_gamepad_sample pad=combat.sample(snapshot);
        gh_native_input_snapshot controls={};
        if(!gh_native_input_tick_device(&input,snapshot.work,&pad,&controls)||
           gh_startup_host_set_input(host,&controls)!=GH_HOST_OK)return fail("native input");
        gh_startup_host_tick(host);
        if(gh_startup_host_snapshot(host,&snapshot)!=GH_HOST_OK)return fail("scheduler snapshot");
        if(controlled_r003&&!r003_handoff&&snapshot.current_room==0x20&&snapshot.game_state==5&&
           snapshot.next_room==0x100){
            if(gh_startup_host_qa_r003_title_handoff(host)!=GH_HOST_OK)return fail("R003 title handoff");
            r003_handoff=snapshot.ticks;
        }
        combat.observe(snapshot,controls);
        if(snapshot.phase==GH_HOST_RUNNING&&snapshot.scene_frame_ready&&snapshot.scene_frame_serial!=completed_frames){
            completed_frames=snapshot.scene_frame_serial;
            combat.presented(completed_frames);
            if(r003_handoff&&snapshot.current_room==3&&snapshot.game_state==4){
                ++r003_frames;
                if(snapshot.event_number==2&&snapshot.event_cut>=0&&snapshot.event_cut!=r003_cut){
                    r003_cut=snapshot.event_cut;
                    std::fprintf(stderr,"CPU_R003_CUT cut=%d scene=%llu\n",r003_cut,
                        (unsigned long long)completed_frames);
                }
            }
        }
        // Let the real asynchronous video decoder produce Movie0's next frame.
        if(sink.movie&&!snapshot.movie_video.complete)Sleep(15);
    }
    const double seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count();
    combat.report();
    const gh_se_attached_failure &voice=snapshot.attached_voice_failure;
    if(voice.frontier&&voice.frontier==snapshot.unavailable_entry)
        std::fprintf(stderr,"CPU_REPLAY_VOICE {\"bank\":%d,\"program\":%d,\"kind\":%u,\"dependency\":%u,"
            "\"flags\":%u,\"part\":%d,\"update\":%u,\"mode\":%u}\n",voice.bank,voice.program,
            (unsigned)voice.actor_kind,voice.actor_dependency,voice.flags,voice.part_index,
            (unsigned)voice.during_update,(unsigned)voice.position_mode);
    if(snapshot.effect_frontier&&snapshot.effect_frontier==snapshot.unavailable_entry)
        std::fprintf(stderr,"CPU_REPLAY_EFFECT {\"pc\":\"%08X\",\"emitter_slot\":%u,\"emitter_serial\":%u,"
            "\"mode\":%u,\"class\":%u,\"bank\":%u,\"index\":%u,\"flags\":\"%08X\","
            "\"est_flags\":\"%08X\",\"part_override\":%u,\"est_part\":%u,"
            "\"actor_kind\":%u,\"actor_dependency\":%u,\"actor_stamp\":%u,\"esp_slot\":%u,\"esp_step\":%u}\n",
            snapshot.effect_frontier,snapshot.effect_emitter_slot,snapshot.effect_emitter_serial,
            snapshot.effect_emitter_mode,snapshot.effect_emitter_class,snapshot.effect_emitter_bank,
            snapshot.effect_emitter_index,snapshot.effect_emitter_flags,snapshot.effect_emitter_est_flags,
            snapshot.effect_emitter_part_override,snapshot.effect_emitter_est_part,snapshot.effect_emitter_actor_kind,
            snapshot.effect_emitter_actor_dependency,snapshot.effect_emitter_actor_stamp,
            snapshot.effect_esp_slot,snapshot.effect_esp_step);
    if(snapshot.unavailable_entry&&snapshot.current_enemy_placement_ready)
        std::fprintf(stderr,"CPU_REPLAY_ENEMY {\"handle\":%u,\"stamp\":%u,\"dependency\":\"%04X\","
            "\"appearance\":%u,\"state\":\"%04X\",\"phase\":%u,\"variant\":%u,"
            "\"distance_xz_squared_bits\":\"%08X\",\"flags_16d0\":\"%08X\",\"flags_16d4\":\"%08X\"}\n",
            snapshot.last_move_handle,snapshot.current_enemy_stamp,snapshot.current_enemy_dependency,
            snapshot.current_enemy_appearance,snapshot.current_actor_state,snapshot.current_enemy_phase,
            snapshot.current_enemy_variant,snapshot.current_enemy_distance_bits,
            snapshot.current_enemy_flags_16d0,snapshot.current_enemy_flags_16d4);
    std::printf("{\"scope\":\"native scheduler and packets, no graphics presentation\","
        "\"ticks\":%llu,\"elapsed_seconds\":%.3f,\"room\":\"%04X\",\"next_room\":\"%04X\","
        "\"game_state\":%u,\"scene_stage\":%u,\"phase\":%u,\"frontier\":\"%08X\","
        "\"last_move\":\"%08X\",\"actor_stamp\":%u,\"actor_dependency\":%u,"
        "\"packet_frame_serial\":%llu,\"world_commands\":%u,\"actor_commands\":%u,\"effect_commands\":%u,"
        "\"r003_controlled_handoff_tick\":%llu,\"r003_packet_frames\":%llu,\"r003_last_ev02_cut\":%d,"
        "\"movie_frames_decoded\":%u,\"probe_complete\":%s}\n",
        (unsigned long long)snapshot.ticks,seconds,(unsigned)snapshot.current_room,(unsigned)snapshot.next_room,
        (unsigned)snapshot.game_state,snapshot.scene_stage,(unsigned)snapshot.phase,snapshot.unavailable_entry,
        snapshot.last_move_entry,snapshot.current_enemy_stamp,snapshot.current_enemy_dependency,
        (unsigned long long)completed_frames,sink.world,sink.actor,sink.effects,
        (unsigned long long)r003_handoff,(unsigned long long)r003_frames,r003_cut,sink.movie,
        combat.complete()?"true":"false");
    if(combat.complete())return 0;
    return snapshot.phase==GH_HOST_FRONTIER?4:snapshot.phase==GH_HOST_RUNNING?3:1;
}

int main(int argc,char **argv)
{
    const bool controlled_r003=argc==6&&!std::strcmp(argv[5],"--r003-controlled");
    if(argc!=5&&!controlled_r003){
        std::fprintf(stderr,"Usage: probe-startup-cpu <ISO> <FFmpeg> <absent profile path> <max ticks> [--r003-controlled]\n");
        return 2;
    }
    char *end=nullptr;
    errno=0;
    const uint64_t max_ticks=std::strtoull(argv[4],&end,10);
    if(argv[4][0]<'1'||argv[4][0]>'9'||errno||*end)return fail("tick-limit parsing");
    if(GetFileAttributesA(argv[3])!=INVALID_FILE_ATTRIBUTES)return fail("absent profile check");
    const DWORD profile_error=GetLastError();
    if(profile_error!=ERROR_FILE_NOT_FOUND&&profile_error!=ERROR_PATH_NOT_FOUND)return fail("profile path lookup");

    std::vector<uint8_t> elf;
    gh_native_input_map map={};
    if(!load_input_map(argv[1],elf,map))return fail("disc input map");
    gh_native_input input={};
    gh_native_input_config config;
    gh_native_input_defaults(&config);
    if(!gh_native_input_init(&input,&config,map))return fail("input setup");
    gh_native_input_focus(&input,1);
    gh_startup_host *host=nullptr;
    if(gh_startup_host_create_with_profile(argv[1],argv[3],1,0,64*1024*1024,&host)!=GH_HOST_OK)
        return fail("native startup creation");
    fixture_sink sink={};
    const int result=replay(host,sink,argv[2],input,max_ticks,controlled_r003);
    if(gh_startup_host_destroy(host)!=GH_HOST_OK){
        std::fprintf(stderr,"CPU replay host cleanup failed\n");
        if(result==0)return 1;
    }
    return result;
}
