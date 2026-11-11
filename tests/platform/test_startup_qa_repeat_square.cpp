#include "../../src/platform/startup_qa_repeat_square.h"
#include "../../src/platform/disc.h"
#include <cstdlib>
#include <vector>

#define CHECK(x) do { if(!(x)) { std::fprintf(stderr,"FAIL %d %s\n",__LINE__,#x); std::exit(1); } } while(0)

struct Fixture {
    gh_boot_work work={};
    gh_startup_snapshot state={};
    gh_native_input native={};
    QARepeatSquareProbe probe;

    explicit Fixture(gh_native_input_map map)
    {
        gh_native_input_config config={};
        gh_native_input_defaults(&config);
        CHECK(gh_native_input_init(&native,&config,map));
        gh_native_input_focus(&native,1);
        probe.enabled=true;
        state.current_room=0x100;
        state.game_state=4;
        state.work=&work;
        state.player_pose_ready=1;
        state.player_handle=7;
        state.r100_first_enemy_handle=25;
        state.r100_first_enemy_hp=6;
        state.r100_first_enemy_position[2]=1;
    }

    gh_native_input_snapshot step(const gh_native_gamepad_sample &pad,bool present=true)
    {
        gh_native_input_snapshot input={};
        CHECK(gh_native_input_tick_device(&native,&work,&pad,&input));
        CHECK(gh_native_input_store_work(&work,&input));
        ++state.ticks;
        ++state.player_updates_completed;
        ++state.scene_frame_serial;
        state.scene_frame_ready=1;
        probe.observe(state,input);
        if(present)probe.presented(state.scene_frame_serial);
        return input;
    }

    void neutral()
    {
        const auto pad=probe.sample(state);
        CHECK(pad.connected&&!pad.buttons&&!pad.left_x&&!pad.left_y);
        step(pad);
    }

    void attack_state(unsigned target,float sample)
    {
        state.player_state[1]=0x0b;
        state.player_state[2]=1;
        state.player_state_entry=0x10d3a8;
        state.player_attack_target=target;
        state.player_motion_sample=sample;
    }
};

static gh_native_input_map actual_map(const char *iso,std::vector<unsigned char> &elf)
{
    gh_disc *disc=nullptr;
    CHECK(gh_disc_open(iso,&disc)==GH_DISC_OK);
    elf.resize((size_t)gh_disc_boot_elf_size(disc));
    CHECK(!elf.empty()&&gh_disc_read_boot_elf(disc,0,elf.data(),elf.size())==GH_DISC_OK);
    gh_disc_close(disc);
    gh_native_input_map map={};
    CHECK(gh_native_input_map_from_elf(elf.data(),elf.size(),0,&map));
    return map;
}

int main(int argc,char **argv)
{
    CHECK(argc==2);
    std::vector<unsigned char> elf;
    Fixture f(actual_map(argv[1],elf));
    auto &s=f.state;
    auto &q=f.probe;
    f.neutral();
    CHECK(!q.saw_event&&!q.player_handle);
    s.event_number=1;
    s.event_state=1;
    f.neutral();
    CHECK(q.saw_event&&!q.player_handle);
    s.event_state=0;
    s.event_motion_camera_active=1;
    f.neutral();
    CHECK(!q.player_handle);
    s.event_motion_camera_active=0;
    f.neutral();
    f.neutral();
    CHECK(q.player_handle==7&&q.enemy_handle==25);
    for(unsigned i=0;i<27;++i){
        const auto pad=q.sample(s);
        CHECK(!pad.buttons&&pad.left_x==(i<7?28000:0)&&pad.left_y==(i<7?18000:32767));
        f.step(pad);
    }
    f.neutral();
    f.neutral();

    const auto first=q.sample(s);
    CHECK(first.buttons==0x4000&&!first.left_x&&!first.left_y&&q.attempts==1);
    CHECK(!q.sample(s).buttons);
    f.attack_state(26,1);
    const auto first_input=f.step(first);
    CHECK(first_input.raw_pressed==0x80&&first_input.pressed&&q.edges==1);
    s.player_motion_sample=2;
    s.r100_first_enemy_hp=5;
    s.r100_first_enemy_player_hit_kind=7;
    s.r100_first_enemy_player_hit_power=1;
    const auto release=f.step(q.sample(s));
    CHECK(release.raw_released==0x80&&q.animations==1&&q.hits==0);
    f.neutral();
    CHECK(q.attempts==1);

    s.player_state[1]=s.player_state[2]=0;
    s.player_state_entry=0;
    s.player_square_buffer=1;
    f.neutral();
    s.player_square_buffer=0;
    s.r100_first_enemy_handle=26;
    f.neutral();
    s.r100_first_enemy_handle=25;
    s.r100_first_enemy_position[0]=3;
    s.r100_first_enemy_position[2]=1;
    const auto steer=q.sample(s);
    CHECK(!steer.buttons&&steer.left_x<0&&steer.left_y>0);
    const auto steering=f.step(steer);
    CHECK(steering.analog.axes[0]<-64&&steering.analog.axes[1]>0);

    s.r100_first_enemy_position[0]=0;
    f.neutral();
    f.neutral();
    const auto second=q.sample(s);
    CHECK(second.buttons==0x4000&&q.attempts==2);
    f.attack_state(25,1);
    CHECK(f.step(second).raw_pressed==0x80&&q.edges==2);
    s.player_motion_sample=2;
    s.r100_first_enemy_hp=2;
    s.r100_first_enemy_player_hit_power=3;
    f.step(q.sample(s),false);
    CHECK(q.hits==0&&q.animations==1);
    q.presented(s.scene_frame_serial);
    q.presented(s.scene_frame_serial);
    CHECK(q.hits==1&&q.animations==2&&!q.defeat_scene);

    s.player_motion_sample=3;
    s.r100_first_enemy_hp=0;
    f.neutral();
    CHECK(q.hits==2&&!q.defeat_scene);
    s.r100_first_enemy_handle=0;
    f.neutral();
    CHECK(!q.defeat_scene);
    s.r100_first_enemy_handle=25;
    s.r100_first_enemy_state[0]=2;
    f.step(q.sample(s),false);
    CHECK(!q.defeat_scene);
    q.presented(s.scene_frame_serial);
    CHECK(q.defeat_scene==s.scene_frame_serial&&q.attempts==2);
    CHECK(!q.sample(s).buttons);
    Fixture triangle(actual_map(argv[1],elf));
    triangle.probe.triangle=true;
    auto &t=triangle.state;
    t.event_number=t.event_state=1;
    triangle.neutral();
    t.event_state=0;
    triangle.neutral();
    triangle.neutral();
    for(unsigned i=0;i<27;++i)triangle.step(triangle.probe.sample(t));
    triangle.neutral();
    triangle.neutral();
    const auto press=triangle.probe.sample(t);
    CHECK(press.buttons==0x8000);
    triangle.attack_state(25,1);
    t.player_attack_skill=29;
    CHECK(triangle.step(press).raw_pressed==0x10&&triangle.probe.edges==1);
    t.player_motion_sample=2;
    t.r100_first_enemy_hp=3;
    t.r100_first_enemy_player_hit_kind=7;
    t.r100_first_enemy_player_hit_power=3;
    triangle.step(triangle.probe.sample(t));
    CHECK(triangle.probe.animations==1&&!triangle.probe.hits);
    t.r100_first_enemy_hp=0;
    t.r100_first_enemy_player_hit_kind=0x12;
    t.r100_first_enemy_player_hit_power=12;
    triangle.step(triangle.probe.sample(t));
    CHECK(triangle.probe.hits==1&&!triangle.probe.defeat_scene);

    Fixture pair(actual_map(argv[1],elf));
    pair.probe.triangle=pair.probe.pair=true;
    auto &p=pair.state;
    p.r100_first_enemy_hp=12;
    p.r100_second_enemy_handle=26;
    p.r100_second_enemy_hp=12;
    p.r100_second_enemy_position[2]=2;
    p.event_number=p.event_state=1;
    pair.neutral();
    p.event_state=0;
    pair.neutral();
    p.player_updates_completed+=28;
    const auto first_pair=pair.probe.sample(p);
    CHECK(first_pair.buttons==0x8000);
    p.player_attack_target=25;
    CHECK(pair.step(first_pair).raw_pressed==0x10);
    p.r100_first_enemy_hp=0;
    p.r100_first_enemy_state[0]=2;
    p.r100_first_enemy_player_hit_kind=0x12;
    p.r100_first_enemy_player_hit_power=12;
    pair.step(pair.probe.sample(p),false);
    CHECK(!pair.probe.hits&&pair.probe.enemy_handle==25);
    pair.probe.presented(p.scene_frame_serial);
    CHECK(pair.probe.hits==1&&!pair.probe.enemy_handle&&!pair.probe.defeat_scene);
    p.r100_first_enemy_handle=0;
    const auto approach=pair.probe.sample(p);
    CHECK(approach.left_y&&!approach.buttons&&pair.probe.enemy_handle==26);
    pair.step(approach);
    p.r100_second_enemy_position[2]=1;
    pair.neutral();
    pair.neutral();
    const auto second_pair=pair.probe.sample(p);
    CHECK(second_pair.buttons==0x8000);
    p.player_attack_target=26;
    CHECK(pair.step(second_pair).raw_pressed==0x10);
    p.r100_second_enemy_hp=0;
    p.r100_second_enemy_state[0]=2;
    p.r100_second_enemy_player_hit_kind=0x12;
    p.r100_second_enemy_player_hit_power=12;
    pair.neutral();
    CHECK(pair.probe.hits==2&&!pair.probe.defeat_scene);
    p.r100_second_enemy_handle=0;
    p.r100_placement_known=3;
    p.r100_placement_dead=1;
    pair.neutral();
    CHECK(!pair.probe.defeat_scene);
    p.r100_placement_dead=3;
    pair.neutral();
    CHECK(pair.probe.defeat_scene==p.scene_frame_serial);
    std::puts("PASS repeated Square/Triangle input and presented evidence with patched ISO pad map");
    return 0;
}
