#include "game_loaded_room.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t w(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static void put(uint8_t*p,uint32_t n){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(n>>(8*i));}
static uint64_t q(const uint8_t*p){return (uint64_t)w(p)|((uint64_t)w(p+4)<<32);}
static void put64(uint8_t*p,uint64_t n){put(p,(uint32_t)n);put(p+4,(uint32_t)(n>>32));}
static void finish_fade(gh_screen_fade*f,const gh_display_state*d){gh_screen_fade_context c={d->screen_width,d->gs_y_origin,0,0};gh_fade_draw draw;for(unsigned i=0;i<1000&&!(f->state&4);i++)CHECK(!gh_screen_fade_step_00307388(f,&c,&draw));CHECK(f->state&4);}
static void expected_input(uint8_t*p){const uint64_t mask=UINT64_C(0xf00807ff00000);put64(p+64,q(p+64)&mask);put64(p+56,q(p+56)&mask);put64(p,q(p+24)&mask);put64(p+40,q(p+40)&mask);put64(p+48,q(p+48)&mask);memset(p+0x68,0x80,4);memset(p+0x6c,0,16);}

static void loaded_room_fixture(gh_disc *disc,const gh_display_state *display)
{
    gh_boot_work work={.room=0x20,.flags=0x46};
    uint8_t *raw=(uint8_t*)&work;
    gh_heap *heap;
    CHECK(!gh_heap_create(32*1024*1024,&heap));
    gh_game_title_player player={0};
    const gh_title_memory_profile profile={.policy=GH_TITLE_MEMORY_ZERO_REFERENCE};
    CHECK(!gh_game_title_player_create_reference_0031c4e0(&player,heap,&work,&profile));
    const uint32_t xyz[]={0x41480000,0xc0e80000,0x42f60000};
    memcpy(player.view.alias_f0,xyz,sizeof(xyz));
    put(player.view.bytes+0x104,0xbf400000);
    player.view.bytes[0x54a]=60;

    gh_game_door door={0};
    CHECK(!gh_game_door_from_disc(&door,disc));
    gh_game_loop loop={.work=&work,.game_state={4}};
    gh_menu_state menu={0};
    gh_game_scene_ui messages={.root=&menu,.ready=1,.active_head=-1};
    gh_task_manager scenario_manager={0};
    uint8_t scenario_fields[0x114]={0};
    static gh_game_prepare prepare;
    static gh_game_init_state init;
    static uint8_t core[GH_CORE_SAVE_BYTES],payload[GH_CHECKPOINT_BYTES];
    gh_game_checkpoint checkpoint={0};
    gh_screen_fade fade={.state=11,.count=0};
    gh_display_state local_display=*display;
    gh_game_door_bindings door_bindings={.work=&work,.loop=&loop,
        .scenario={scenario_fields,sizeof(scenario_fields),&scenario_manager},
        .messages=&messages,.prepare=&prepare,.player=&player,.fade=&fade,.display=&local_display};
    const gh_checkpoint_bindings checkpoint_bindings={.work=&work,.save={core,sizeof(core)},
        .prepare=&prepare,.init=&init,.player=&player.view,.destination=payload,.destination_size=sizeof(payload)};
    gh_audio_registry *registry;
    gh_sound_control sound={0};
    gh_bgm_requests *bgm;
    CHECK(!gh_audio_registry_create(&registry)&&!gh_audio_startup_load(registry,disc));
    CHECK(!gh_sound_control_from_startup(&sound,disc,registry)&&!gh_bgm_requests_create(registry,&sound,disc,heap,&bgm));
    const gh_scene_loaded_room_bindings bindings={&door_bindings,&door,&checkpoint,&checkpoint_bindings,bgm,&sound};

    put(raw+0x5e8,0x40000020);
    put(raw+0x5e4,0x4000);
    put(raw+0x5b4,0x0100);
    init.world_time=444;prepare.global_0061a994=555;prepare.fields[0x1660]=0x77;
    put(init.heat,0x43480000);put(init.heat+4,0x42f00000);
    put(payload+0x10,0x1234);payload[0x1f]=2;put(payload+0xb90,0xfacecafe);
    put(payload+0x170,1);put(payload+0x174,1);put(payload+0x178,3);
    gh_game_scene_aux aux={.load_flags_saved=1};
    aux.save_load[0]=5;aux.save_load[0xa]=3;aux.save_load[0xc]=1;
    put(aux.save_load+0x70,0x100);
    put(aux.save_load+0x74,0x60000000);put(aux.save_load+0x78,0x12345678);
    gh_scene_loaded_room state={0};
    CHECK(!gh_scene_cursor_loaded_room_002bf568(&aux,&state,&bindings)&&aux.save_load[0]==5);
    fade.state=15;
    CHECK(!gh_scene_cursor_loaded_room_002bf568(&aux,&state,&bindings));
    CHECK(!state.stage&&!aux.save_load[0]&&!aux.save_load[0xa]&&aux.save_load[0xc]==1);
    CHECK(w(raw+0x5e8)==0x20&&w(raw+0x5dc)==0x12345678&&!(w(raw+0x5e4)&0x4000));
    CHECK((w(raw+0x5e0)&0x100)&&(w(raw+0x5d8)&0x60000000)==0x60000000);
    CHECK((sound.flags_ac&0x200000)&&(sound.flags_b0&0x200000));
    CHECK(door.target_room==0x100&&door.kind==1&&door.force3a==1&&door.flags==0x80000000&&door.point==255);
    CHECK(!memcmp(door.position_bits,xyz,12)&&door.yaw_bits==0xbf400000&&fade.state==15);
    CHECK(w(core+0xc)==0x100&&w(core+0x10)==0x1234&&core[0x1f]==2&&w(core+0xb8c)==1);
    CHECK(w(core+0xb90)==0xfacecafe&&w(core+0x170)==1&&w(core+0x174)==1&&w(core+0x178)==3);
    CHECK(w(core+0x84)==60&&!init.world_time&&prepare.global_0061a994==555&&payload[0x14a0]==0x77);
    CHECK(checkpoint.tail.heat==0x42f00000&&checkpoint.tail.heat_max==0x43480000);
    CHECK(w(init.heat)==0x42f00000&&!w(init.heat+4));
    CHECK(loop.game_state[0]==4&&scenario_manager.flags==1);
    CHECK(!gh_game_door_step_001ffe10(&door,&door_bindings)&&loop.game_state[0]==5&&work.room==0x20);
    CHECK(w(raw+0x5b4)==0x100&&raw[0x5b8]==1&&raw[0x5ba]==255&&!memcmp(raw+0x5c0,xyz,12));
    CHECK(!prepare.global_0061a994&&fade.state==15);
    gh_bgm_requests_destroy(bgm);
    gh_audio_registry_destroy(registry);
    CHECK(!gh_game_title_player_release(&player)&&!gh_heap_destroy(heap));
    puts("PASS loaded R100 cursor handoff, special checkpoint reset, and canonical Door Main5");
}

int main(int argc,char**argv){
 gh_disc*disc;CHECK(argc==2&&!gh_disc_open(argv[1],&disc));size_t n=(size_t)gh_disc_boot_elf_size(disc);uint8_t*elf=malloc(n);CHECK(elf&&!gh_disc_read_boot_elf(disc,0,elf,n));
 gh_game_door owner={0},from_memory={0};CHECK(!gh_game_door_from_disc(&owner,disc)&&!gh_game_door_from_elf(&from_memory,elf,n));
 uint8_t zero[0x3c]={0};CHECK(!memcmp(&owner,zero,sizeof(zero))&&!memcmp(&from_memory,zero,sizeof(zero))&&owner.initialized);
 CHECK(gh_game_door_from_disc(&owner,disc)==GH_DOOR_BUSY);
 gh_game_door invalid={0};CHECK(gh_game_door_from_elf(&invalid,elf,20)==GH_DOOR_FORMAT&&!invalid.initialized);elf[0]=0;CHECK(gh_game_door_from_elf(&invalid,elf,n)==GH_DOOR_FORMAT&&!invalid.initialized);elf[0]=0x7f;
 gh_display_state display={0};CHECK(!gh_display_state_from_elf(&display,elf,n)&&!gh_display_static_initialize_002a48a8(&display,1,0xffff));
 uint8_t saved[0x3c];memset(&from_memory,0xa5,0x3c);memcpy(saved,&from_memory,0x3c);saved[0]=saved[1]=0;CHECK(!gh_game_door_reset_001ffe08(&from_memory)&&!memcmp(&from_memory,saved,0x3c));
 gh_boot_work work={0};work.room=0x20;uint8_t*raw=(uint8_t*)&work;gh_game_loop loop={.work=&work,.game_state={4,2,3,4}};
 gh_menu_state menu={0};gh_game_scene_ui messages={.root=&menu,.ready=1,.active_head=-1,.free_head=7};
 gh_game_prepare*prepare=malloc(sizeof(*prepare));CHECK(prepare);memset(prepare->fields,0xa5,sizeof(prepare->fields));prepare->global_0061a994=0x12345678;
 gh_game_scene_step scene={.cooldown_1b0=15};gh_task_manager scenario_manager={.flags=0xabcdefe4};uint8_t scenario_fields[0x114]={0};
 gh_screen_fade fade={0};gh_screen_fade_context fc={display.screen_width,display.gs_y_origin,0,0};CHECK(!gh_screen_fade_start_00307208(&fade,&fc,0,10,0,0xff000000,0xff000000,1));finish_fade(&fade,&display);
 gh_game_door_bindings b={.work=&work,.loop=&loop,.scenario={scenario_fields,sizeof(scenario_fields),&scenario_manager},.messages=&messages,.prepare=prepare,.scene_step=&scene,.fade=&fade,.display=&display};
 float position[3]={12.5f,-7.25f,123.0f},yaw=-.75f;uint8_t input[128];for(unsigned i=0;i<128;i++)input[i]=(uint8_t)(i*37+7);memcpy(raw+0x178,input,128);expected_input(input);raw[0x177]=0x67;raw[0x1f8]=0x89;
 owner.wait=7;put(raw+0x5d8,0x10002000);put(raw+0x5e0,0x40);put(raw+0x5e4,0x20);gh_screen_fade held=fade;
 CHECK(!gh_game_door_request_001ffeb8(&owner,&b,0x100,position,yaw,1,0,1));
 CHECK(owner.target_room==0x100&&owner.point==255&&owner.kind==1&&owner.force3a==1&&owner.wait==7&&!owner.begin_stage);
 CHECK(loop.game_state[0]==4&&scenario_manager.flags==0xabcdefe5&&!memcmp(raw+0x178,input,128)&&raw[0x177]==0x67&&raw[0x1f8]==0x89);
 CHECK(!memcmp(&fade,&held,sizeof(fade))&&scene.cooldown_1b0==15&&(w(raw+0x5e4)&0x2000000)&&(w(raw+0x5e0)&0x100000));
 CHECK(!gh_game_door_step_001ffe10(&owner,&b)&&owner.wait==6&&!owner.step_stage&&!owner.frontier);
 CHECK(loop.game_state[0]==5&&!loop.game_state[1]&&!loop.game_state[2]&&!loop.game_state[3]&&work.room==0x20); /*next-room only*/
 CHECK(raw[0x5b4]==0&&raw[0x5b5]==1&&raw[0x5b8]==1&&raw[0x5ba]==255&&!memcmp(raw+0x5c0,position,12)&&!memcmp(raw+0x5d0,&yaw,4));
 CHECK(!(w(raw+0x5e4)&0x20)&&(w(raw+0x5e4)&0x2008000)==0x2008000&&!(w(raw+0x5e0)&0x100000)&&!(w(raw+0x5d8)&0x500000));
 CHECK(prepare->global_0061a994==0&&prepare->fields[0]==0xa5&&prepare->fields[sizeof(prepare->fields)-1]==0xa5&&scenario_manager.flags==0xabcdefe5);
 /* Pending request still overwrites original destination fields, while the
  * begin flag short-circuit avoids restarting Fade/input/Menu state. */
 owner.phase38=9;messages.active_head=99;CHECK(!gh_game_door_request_001ffeb8(&owner,&b,0x101,position,1.0f,2,3,0)&&owner.phase38==9&&owner.target_room==0x101);messages.active_head=-1;
 /* Actual live KillAll marks termination state, without unlink/free. */
 memset(&work,0,sizeof(work));work.room=0x20;messages.active_head=2;messages.blocks[2].next=4;messages.blocks[4].next=-1;messages.blocks[2].payload[0]=0;messages.blocks[2].payload[1]=9;messages.blocks[2].payload[3]=9;put(messages.blocks[2].payload+8,0x100);
 messages.blocks[4].payload[0]=7;messages.blocks[4].payload[1]=9;messages.blocks[4].payload[3]=8;CHECK(!gh_game_door_request_001ffeb8(&owner,&b,0x101,position,yaw,1,0,1));
 CHECK(messages.active_head==2&&messages.free_head==7&&messages.blocks[2].next==4&&messages.blocks[4].next==-1&&messages.blocks[2].payload[0]==6&&!messages.blocks[2].payload[1]&&!messages.blocks[2].payload[3]&&w(messages.blocks[2].payload+8)==0x140&&messages.blocks[4].payload[0]==7&&messages.blocks[4].payload[1]==9&&messages.blocks[4].payload[3]==8);
 /* Branch1 fade waits for actual shared Fade completion and reverses only at
  * original changeRoom tail. No status query advances its count. */
 memset(&work,0,sizeof(work));work.room=0x20;messages.active_head=-1;loop.game_state[0]=4;owner.wait=3;prepare->global_0061a994=99;
 CHECK(!gh_game_door_request_001ffeb8(&owner,&b,0x101,position,yaw,1,1,1)&&fade.state==11&&fade.color_from==0xa0a0a0&&fade.color_to==0x80a0a0a0&&fade.time==30&&scene.cooldown_1b0==30);
 uint32_t count=fade.count;CHECK(!gh_game_door_step_001ffe10(&owner,&b)&&owner.wait==2&&loop.game_state[0]==4&&fade.count==count);
 finish_fade(&fade,&display);CHECK(!gh_game_door_step_001ffe10(&owner,&b)&&loop.game_state[0]==5&&fade.state==9&&fade.color_from==0x80a0a0a0&&fade.color_to==0xa0a0a0&&fade.time==15&&prepare->global_0061a994==99);
 /* No guessed same-room transform or sub-screen readiness. */
 memset(&work,0,sizeof(work));work.room=0x20;memset(&fade,0,sizeof(fade));loop.game_state[0]=4;
 CHECK(!gh_game_door_request_001ffeb8(&owner,&b,0x20,position,yaw,1,0,1)&&owner.target_room==-1);CHECK(gh_game_door_step_001ffe10(&owner,&b)==GH_DOOR_UNSUPPORTED&&owner.frontier==0x2c4440&&loop.game_state[0]==4);
 /* Isolate the proven phase1 cleanup leaf without claiming its missing
  * phase0 player transforms completed. Exact native player scalar store. */
 uint8_t player_bytes[GH_TITLE_PLAYER_BYTES];memset(player_bytes,0xa5,sizeof(player_bytes));gh_game_title_player player={.view={.bytes=player_bytes,.size=sizeof(player_bytes)},.ready=1};b.player=&player;
 owner.phase38=1;owner.step_stage=0;put(raw+0x5dc,0x12345678);CHECK(!gh_game_door_step_001ffe10(&owner,&b)&&!(w(raw+0x5e4)&0x2000000)&&!(scenario_manager.flags&1)&&!w(raw+0x5d8)&&!w(raw+0x5dc)&&w(player_bytes+0x54c)==0x41f00000&&player_bytes[0x54b]==0xa5&&player_bytes[0x550]==0xa5);
 b.player=NULL;
 owner.step_stage=0;memset(&work,0,sizeof(work));work.room=0x20;CHECK(!gh_game_door_request_001ffeb8(&owner,&b,0x100,position,yaw,0,0,0));CHECK(gh_game_door_step_001ffe10(&owner,&b)==GH_DOOR_UNSUPPORTED&&owner.frontier==0x2006b8&&loop.game_state[0]==4);
 /* A host frontier preserves the wait decrement on retry of that same call. */
 owner.wait=10;owner.step_stage=0;CHECK(gh_game_door_step_001ffe10(&owner,&b)==GH_DOOR_UNSUPPORTED&&owner.wait==9);CHECK(gh_game_door_step_001ffe10(&owner,&b)==GH_DOOR_UNSUPPORTED&&owner.wait==9);
 owner.step_stage=0;memset(&work,0,sizeof(work));owner.wait=UINT16_MAX;CHECK(!gh_game_door_step_001ffe10(&owner,&b)&&owner.wait==UINT16_MAX-1);owner.wait=0;CHECK(!gh_game_door_step_001ffe10(&owner,&b)&&!owner.wait);
 /* Invalid native active chain is bounded before any kill mutation; original
  * valid chains remain linked for later Menu teardown, never freed early. */
 memset(&work,0,sizeof(work));work.room=0x20;owner.step_stage=owner.begin_stage=0;messages.active_head=2;messages.blocks[2].next=2;uint8_t payload[0xb0];memcpy(payload,messages.blocks[2].payload,sizeof(payload));
 CHECK(gh_game_door_request_001ffeb8(&owner,&b,0x100,position,yaw,1,0,1)==GH_DOOR_BOUNDS&&owner.frontier==0x2aecd0&&!memcmp(payload,messages.blocks[2].payload,sizeof(payload))&&!(w(raw+0x5e4)&0x2000000));
 messages.blocks[2].next=-1;CHECK(!gh_game_door_request_001ffeb8(&owner,&b,0x100,position,yaw,1,0,1));
 memset(&work,0,sizeof(work));work.room=0x20;work.flags=0x100;owner.step_stage=0;b.messages=NULL;CHECK(!gh_game_door_request_001ffeb8(&owner,&b,0x100,position,yaw,1,0,1)&&!(work.flags&0x100));
 loaded_room_fixture(disc,&display);
 free(prepare);free(elf);gh_disc_close(disc);puts("PASS realELF DoorBSS/only-wait-reset/input-width/MenuKill-state/ONE-Scenario-flags/Fade-readiness/nextroom-Main5/clock+4/frontiers");return 0;
}
