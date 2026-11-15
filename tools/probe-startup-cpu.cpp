/* Developer replay of the native scheduler and packet producers. */
#include "../tests/platform/startup_cpu_sink.h"
#include "../src/platform/startup_qa_repeat_square.h"
#include "../src/platform/startup_qa_r100_pad_tape.h"
#include "../src/platform/startup_qa_r102_walk.h"
#include "../src/platform/startup_qa_title.h"
#include "../src/platform/startup_qa_save_map_continue.h"
#include "../src/platform/disc.h"
#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
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

static void approach_trace(const gh_startup_snapshot &s,uint64_t room_frames)
{
    std::fprintf(stderr,"CPU_R102_APPROACH frame=%llu tick=%llu xyz=%.6f,%.6f,%.6f "
        "yaw=%.6f hp=%u state=%02X/%02X/%02X player_entry=%08X player_frontier=%08X target=%u\n",
        (unsigned long long)room_frames,(unsigned long long)s.ticks,
        (double)s.player_world_position[0],(double)s.player_world_position[1],
        (double)s.player_world_position[2],(double)s.player_yaw,s.player_hp,
        s.player_state[0],s.player_state[1],s.player_state[2],s.player_state_entry,
        s.player_update_frontier,s.player_attack_target);
}

static bool empty_card(const char *profile)
{
    WIN32_FIND_DATAA entry;
    const std::string pattern=std::string(profile)+"\\*";
    HANDLE scan=FindFirstFileA(pattern.c_str(),&entry);
    if(scan==INVALID_HANDLE_VALUE)return GetLastError()==ERROR_FILE_NOT_FOUND;
    bool empty=true;
    do{
        if(std::strcmp(entry.cFileName,".")&&std::strcmp(entry.cFileName,"..")){
            empty=false;
            break;
        }
    }while(FindNextFileA(scan,&entry));
    FindClose(scan);
    return empty;
}

struct R102CardProbe {
    bool save_selected=false,save_verified=false,title_list_seen=false;
    uint32_t phase=UINT32_MAX,subphase=UINT32_MAX;
    uint64_t last_frame=0;
    bool confirm_choice=false;

    gh_native_gamepad_sample save_pad(const gh_startup_snapshot &s,uint64_t presented)
    {
        gh_native_gamepad_sample pad={};
        pad.connected=1;
        if(!save_selected&&s.save_map_active&&s.save_map_phase==5&&s.save_map_selected==0&&
           s.scene_frame_ready&&presented&&s.scene_frame_serial==presented){
            save_selected=true;
            pad.buttons=0x1000;
            std::fprintf(stderr,"CPU_R102_SAVE_MAP_SELECT tick=%llu choice=0\n",(unsigned long long)s.ticks);
        }
        if(!save_selected||!s.mc_ui_live)return pad;
        if(phase!=s.mc_phase||subphase!=s.mc_subphase){
            if(phase!=s.mc_phase)confirm_choice=false;
            phase=s.mc_phase;
            subphase=s.mc_subphase;
            std::fprintf(stderr,"CPU_R102_SAVE_MC tick=%llu phase=%u sub=%u mask=%X done=%d error=%d\n",
                (unsigned long long)s.ticks,phase,subphase,s.mc_present_mask,s.mc_done,s.mc_error);
        }
        if(phase==14&&subphase==1&&s.mc_present_mask)save_verified=true;
        if(!s.mc_frame_ready||s.mc_frame_serial==last_frame||s.ticks%6)return pad;
        last_frame=s.mc_frame_serial;
        if((phase==6||phase==7)&&(subphase==1||subphase==2)){
            if(phase==7&&!confirm_choice){pad.buttons=0x0008;confirm_choice=true;}
            else pad.buttons=0x1000;
        }
        return pad;
    }

    gh_native_gamepad_sample load_pad(const gh_startup_snapshot &s)
    {
        gh_native_gamepad_sample pad={};
        pad.connected=1;
        if(s.mc_load_mode){
            if((s.mc_phase==3||s.mc_phase==4)&&(s.mc_subphase==1||s.mc_subphase==2)&&s.mc_present_mask){
                if(!title_list_seen)std::fprintf(stderr,"CPU_R102_TITLE_LOAD_LIST tick=%llu mask=%X\n",
                    (unsigned long long)s.ticks,s.mc_present_mask);
                title_list_seen=true;
                if(s.ticks%6==0)pad.buttons=0x1000;
            }
        }else if(s.title_live){
            if(s.title_mode==1&&s.ticks%30==0)pad.buttons=0x1000;
            else if(s.title_mode==3&&s.title_phase>=1&&s.title_phase<=3&&s.title_subphase==1&&
                    (s.title_phase==1||s.title_timer>=5)&&s.ticks%6==0){
                if(s.title_phase==1||s.title_game_selection==1)pad.buttons=0x1000;
                else if(s.title_phase==2&&s.title_game_selection==0)pad.buttons=0x0008;
            }
        }
        return pad;
    }
};

static int replay(gh_startup_host *host,fixture_sink &sink,const char *ffmpeg,gh_native_input &input,
    uint64_t max_ticks,bool controlled_r003,QAR100PadTape &tape,uint64_t after_victory,bool jump_r105,
    bool walk_r102,bool approach_r102,bool save_r102,uint64_t load_frames)
{
    if(!configure_cpu_sink(host,&sink,ffmpeg))return fail("consumer setup");
    QAStartupTitleProbe title;
    QASaveMapContinueProbe r002_save_map;
    R102CardProbe card;
    bool card_complete=false;
    QARepeatSquareProbe combat;
    combat.enabled=!controlled_r003&&!tape.enabled&&!load_frames;
    combat.triangle=combat.pair=combat.continue_after_pair=combat.packet_only=true;

    gh_startup_snapshot snapshot={};
    if(gh_startup_host_snapshot(host,&snapshot)!=GH_HOST_OK)return fail("initial snapshot");
    const auto started=std::chrono::steady_clock::now();
    uint64_t completed_frames=0,r003_handoff=0,r003_frames=0;
    uint64_t post_victory_frames=0;
    uint64_t r102_first_tick=0,r102_frames=0;
    unsigned r102_world=0,r102_actors=0;
    QAR102WalkProbe walk;
    walk.enabled=walk_r102;
    bool pad_complete=false;
    int r003_cut=-1,r002_cut=-2,r002_state=-1;
    while(snapshot.ticks<max_ticks&&snapshot.phase==GH_HOST_RUNNING&&!combat.complete()&&!pad_complete&&!walk.complete&&!card_complete){
        title.sample(snapshot,input,!load_frames,false);
        gh_native_gamepad_sample pad=tape.enabled?tape.sample(snapshot):combat.sample(snapshot);
        if(jump_r105&&snapshot.current_room==0x105&&snapshot.game_state==4&&
           snapshot.scene_frame_ready&&snapshot.ticks>3326&&(snapshot.ticks&1)){
            pad.connected=1;
            pad.buttons=0x2000;
        }
        if(save_r102&&!card.save_verified&&snapshot.current_room==2&&snapshot.game_state==4&&
           (snapshot.save_map_active||(card.save_selected&&snapshot.mc_ui_live)))
            pad=card.save_pad(snapshot,completed_frames);
        else if(jump_r105&&snapshot.current_room==2&&snapshot.game_state==4&&snapshot.save_map_active)
            pad=r002_save_map.sample(snapshot,completed_frames);
        if(load_frames&&snapshot.save_map_active)pad=r002_save_map.sample(snapshot,completed_frames);
        else if(load_frames&&(snapshot.title_live||snapshot.mc_load_mode))pad=card.load_pad(snapshot);
        if(walk.applies(snapshot))pad=walk.sample(snapshot,r102_frames);
        if(approach_r102&&snapshot.current_room==0x102&&snapshot.game_state==4&&snapshot.player_pose_ready){
            pad={};
            pad.connected=1;
            if(r102_frames>=8&&r102_frames<180)pad.left_y=32767;
            if(r102_frames==184)pad.buttons=0x4000;
        }
        gh_native_input_snapshot controls={};
        if(!gh_native_input_tick_device(&input,snapshot.work,&pad,&controls)||
           gh_startup_host_set_input(host,&controls)!=GH_HOST_OK)return fail("native input");
        const unsigned previous_world=sink.world,previous_actors=sink.actor;
        gh_startup_host_tick(host);
        if(gh_startup_host_snapshot(host,&snapshot)!=GH_HOST_OK)return fail("scheduler snapshot");
        if(jump_r105&&snapshot.current_room==2&&snapshot.game_state==4&&
           (snapshot.event_cut!=r002_cut||snapshot.event_state!=r002_state)){
            r002_cut=snapshot.event_cut;
            r002_state=snapshot.event_state;
            std::fprintf(stderr,"CPU_R002_EVENT tick=%llu number=%d state=%d cut=%d scene=%llu\n",
                (unsigned long long)snapshot.ticks,snapshot.event_number,r002_state,r002_cut,
                (unsigned long long)snapshot.scene_frame_serial);
        }
        if(controlled_r003&&!r003_handoff&&snapshot.current_room==0x20&&snapshot.game_state==5&&
           snapshot.next_room==0x100){
            if(gh_startup_host_qa_r003_title_handoff(host)!=GH_HOST_OK)return fail("R003 title handoff");
            r003_handoff=snapshot.ticks;
        }
        if(save_r102&&card.save_verified&&(snapshot.mc_done||!snapshot.mc_ui_live)){
            std::fprintf(stderr,"CPU_R102_SAVE_VERIFIED tick=%llu phase=%u mask=%X\n",
                (unsigned long long)snapshot.ticks,snapshot.mc_phase,snapshot.mc_present_mask);
            card_complete=true;
        }
        combat.observe(snapshot,controls);
        if(snapshot.phase==GH_HOST_RUNNING&&snapshot.scene_frame_ready&&snapshot.scene_frame_serial!=completed_frames){
            completed_frames=snapshot.scene_frame_serial;
            combat.presented(completed_frames);
            if(tape.victory_scene)++post_victory_frames;
            tape.presented(snapshot,completed_frames);
            pad_complete=tape.victory_scene&&post_victory_frames>=after_victory;
            if(snapshot.current_room==0x102&&snapshot.game_state==4){
                if(!r102_frames)r102_first_tick=snapshot.ticks;
                ++r102_frames;
                r102_world+=sink.world-previous_world;
                r102_actors+=sink.actor-previous_actors;
                if(load_frames&&card.title_list_seen&&r102_frames>=load_frames&&
                   r102_world&&r102_actors&&snapshot.player_pose_ready&&snapshot.player_hp){
                    card_complete=true;
                    std::fprintf(stderr,"CPU_R102_TITLE_LOAD_COMPLETE tick=%llu frames=%llu hp=%u world=%u actors=%u\n",
                        (unsigned long long)snapshot.ticks,(unsigned long long)r102_frames,
                        (unsigned)snapshot.player_hp,r102_world,r102_actors);
                }
                walk.observe_frame(snapshot,completed_frames,r102_frames);
                if(approach_r102&&(r102_frames==1||r102_frames%32==0))
                    approach_trace(snapshot,r102_frames);
            }
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
    tape.report("CPU_R100_PAD_TAPE");
    walk.report("CPU_R102_WALK",r102_frames);
    if(approach_r102&&snapshot.current_room==0x102)approach_trace(snapshot,r102_frames);
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
        "\"r102_first_frame_tick\":%llu,\"r102_packet_frames\":%llu,"
        "\"r102_world_commands\":%u,\"r102_actor_commands\":%u,"
        "\"movie_frames_decoded\":%u,\"post_victory_packet_frames\":%llu,"
        "\"probe_complete\":%s}\n",
        (unsigned long long)snapshot.ticks,seconds,(unsigned)snapshot.current_room,(unsigned)snapshot.next_room,
        (unsigned)snapshot.game_state,snapshot.scene_stage,(unsigned)snapshot.phase,snapshot.unavailable_entry,
        snapshot.last_move_entry,snapshot.current_enemy_stamp,snapshot.current_enemy_dependency,
        (unsigned long long)completed_frames,sink.world,sink.actor,sink.effects,
        (unsigned long long)r003_handoff,(unsigned long long)r003_frames,r003_cut,
        (unsigned long long)r102_first_tick,(unsigned long long)r102_frames,r102_world,r102_actors,sink.movie,
        (unsigned long long)post_victory_frames,combat.complete()||pad_complete||walk.complete||card_complete?"true":"false");
    if(combat.complete()||pad_complete||walk.complete||card_complete)return 0;
    return snapshot.phase==GH_HOST_FRONTIER?4:snapshot.phase==GH_HOST_RUNNING?3:1;
}

int main(int argc,char **argv)
{
    const bool walk_r102=argc==6&&!std::strcmp(argv[5],"--r102-walk");
    const bool approach_r102=argc==6&&!std::strcmp(argv[5],"--r102-approach");
    const bool save_r102=argc==6&&!std::strcmp(argv[5],"--r102-save");
    const bool load_r102=(argc==6||argc==7)&&!std::strcmp(argv[5],"--r102-title-load");
    const bool jump_r105=save_r102||walk_r102||approach_r102||(argc==6&&!std::strcmp(argv[5],"--r105-jump"));
    const bool controlled_r003=jump_r105||(argc==6&&!std::strcmp(argv[5],"--r003-controlled"));
    const bool post_victory=argc==9&&!std::strcmp(argv[7],"--after-victory-frames");
    const bool pad_tape=(argc==7||post_victory)&&!std::strcmp(argv[5],"--pad-tape");
    if(argc!=5&&!controlled_r003&&!pad_tape&&!load_r102){
        std::fprintf(stderr,"Usage: probe-startup-cpu <ISO> <FFmpeg> <profile path> <max ticks> "
            "[--r003-controlled | --r105-jump | --r102-walk | --r102-approach | --r102-save | --r102-title-load [frames] | --pad-tape CSV [--after-victory-frames N]]\n");
        return 2;
    }
    QAR100PadTape tape;
    if(pad_tape&&!tape.load(argv[6]))return fail("pad tape validation");
    char *end=nullptr;
    errno=0;
    const uint64_t max_ticks=std::strtoull(argv[4],&end,10);
    if(argv[4][0]<'1'||argv[4][0]>'9'||errno||*end)return fail("tick-limit parsing");
    uint64_t load_frames=load_r102?120:0;
    if(load_r102&&argc==7){
        errno=0;
        load_frames=std::strtoull(argv[6],&end,10);
        if(argv[6][0]<'1'||argv[6][0]>'9'||errno||*end||load_frames>=max_ticks)
            return fail("R102 frame-count parsing");
    }
    uint64_t after_victory=0;
    if(post_victory){
        errno=0;
        after_victory=std::strtoull(argv[8],&end,10);
        if(argv[8][0]<'1'||argv[8][0]>'9'||errno||*end||after_victory>=max_ticks)
            return fail("after-victory frame count parsing");
    }
    const DWORD profile_attributes=GetFileAttributesA(argv[3]);
    if(save_r102||load_r102){
        if(profile_attributes==INVALID_FILE_ATTRIBUTES||!(profile_attributes&FILE_ATTRIBUTE_DIRECTORY)||
           (profile_attributes&FILE_ATTRIBUTE_REPARSE_POINT))return fail("local card profile");
        if(save_r102&&!empty_card(argv[3]))return fail("empty card profile");
    }else{
        if(profile_attributes!=INVALID_FILE_ATTRIBUTES)return fail("absent profile check");
        const DWORD profile_error=GetLastError();
        if(profile_error!=ERROR_FILE_NOT_FOUND&&profile_error!=ERROR_PATH_NOT_FOUND)return fail("profile path lookup");
    }

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
    if(jump_r105&&gh_startup_host_qa_enable_r105_exit_fixture(host)!=GH_HOST_OK){
        gh_startup_host_destroy(host);
        return fail("controlled R105 exit setup");
    }
    fixture_sink sink={};
    const int result=replay(host,sink,argv[2],input,max_ticks,controlled_r003,tape,after_victory,jump_r105,
                            walk_r102,approach_r102,save_r102,load_frames);
    if(gh_startup_host_destroy(host)!=GH_HOST_OK){
        std::fprintf(stderr,"CPU replay host cleanup failed\n");
        if(result==0)return 1;
    }
    return result;
}
