#include "game_effect_node.h"
#include "game_esp_create.h"
#include "game_effects_frame.h"
#include "game_camera_envelope.h"
#include "game_actor_two_bone.h"
#include "game_esp_update.h"
#include "game_ray_static.h"
#include "game_scene_exit.h"
#include "disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fenv.h>
#define C(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static void put(void*p,uint32_t n){memcpy(p,&n,4);}
static uint32_t clock_value;static uint32_t test_clock(void*p){(void)p;return ++clock_value;}
static float value(const void*p){float n;memcpy(&n,p,4);return n;}
static uint8_t*load(gh_disc*d,const char*path,size_t*n){size_t i;gh_disc_entry e;C(!gh_disc_find(d,path,&i)&&!gh_disc_entry_at(d,i,&e));*n=(size_t)e.size;uint8_t*p=malloc(*n);C(p&&!gh_disc_read(d,i,0,p,*n));return p;}
static gh_effect_esp_view view(gh_game_effects*e,unsigned i){gh_effect_esp_view v;C(!gh_effect_create_esp_at(e,i,&v));return v;}
static gh_effect_emitter_view emitter(gh_game_effects*e,unsigned i){gh_effect_emitter_view v;C(!gh_effect_create_emitter_at(e,i,&v));return v;}
typedef struct {unsigned slot;uint32_t serial;} created;
static created make(gh_esp_create_bindings*b,unsigned source){gh_effect_emitter_view v=emitter(b->storage.effects,source);gh_esp_create_call c={0};C(!gh_esp_create_begin_00300e30(&c,source,v.serial));C(!gh_game_esp_create_00300e30(&c,b)&&c.complete);return (created){c.slot,c.serial};}
int main(int argc,char**argv){C(argc==3);FILE*file=fopen(argv[2],"rb");C(file&&!fseek(file,0,SEEK_END));long bytes=ftell(file);C(bytes>0&&!fseek(file,0,SEEK_SET));uint8_t*elf=malloc((size_t)bytes);C(elf&&fread(elf,1,(size_t)bytes,file)==(size_t)bytes);fclose(file);gh_effects_frame_globals globals={0};gh_camera_envelope envelope={0};uint32_t f;C(!gh_effects_frame_globals_from_elf(&globals,elf,(size_t)bytes)&&!gh_game_camera_envelope_from_elf(&envelope,elf,(size_t)bytes,&f));
 gh_disc*disc;C(!gh_disc_open(argv[1],&disc));size_t cn,rn;uint8_t*core=load(disc,"etc/core.dat",&cn),*roombytes=load(disc,"st1/r100.dat",&rn);gh_bundle room;C(!gh_bundle_open(roombytes,rn,&room));gh_game_effects*e=calloc(1,sizeof(*e));uint8_t*esp=calloc(1,0x100180),*emit=calloc(1,0x26060);C(e&&esp&&emit&&!gh_game_effects_initialize_00305a68(e,esp,0x100180,emit,0x26060,core,cn,&room));gh_boot_work work={0};gh_game_mt random;gh_game_mt_seed_0030f2b0(&random,123);gh_effect_release_bindings storage={.effects=e,.esp=esp,.esp_bytes=0x100180,.emitters=emit,.emitter_bytes=0x26060};gh_effect_create_bindings producer={.storage=storage,.work=&work,.random=&random};for(unsigned i=0;i<2;i++){gh_effect_create_call c={0};C(!gh_effect_create_ambient_begin_00305e88(&c,0)&&!gh_game_effect_create_00306528(&c,&producer)&&c.complete);}C(emitter(e,0).first_esp_child==-1&&emitter(e,1).first_esp_child==-1);
 gh_esp_create_bindings b={.storage=storage,.random=&random,.work=&work,.policy=GH_STATIC_BINARY32_REFERENCE,.globals=&globals,.atan_coeff=envelope.atan_coeff};gh_game_scene_tables actors={0};C(!gh_scene_actors_0012e670(&actors,&room));b.actors=&actors;gh_esp_update_bindings ub={.storage=storage,.work=&work,.globals=&globals,.atan_coeff=envelope.atan_coeff,.policy=GH_STATIC_BINARY32_REFERENCE};gh_effect_create_call group={0};C(!gh_effect_create_ambient_begin_00305e88(&group,1)&&!gh_game_effect_create_00306528(&group,&producer)&&group.complete);const unsigned sources[]={0,2,3,4};const int modes[]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};for(unsigned mode=0;mode<4;mode++)for(unsigned row=0;row<4;row++){C(!fesetround(FE_TONEAREST));created particle=make(&b,sources[row]);gh_effect_esp_view v=view(e,particle.slot);uint8_t before[0x400],held_payload[0x400];memcpy(before,v.payload,0x400);gh_game_mt held=random;v.payload[0x140]=1;gh_esp_update_call update={0};C(!fesetround(modes[mode])&&!feclearexcept(FE_ALL_EXCEPT)&&!feraiseexcept(FE_DIVBYZERO));C(gh_game_esp_update_002fbed0(&update,&ub,particle.slot,particle.serial)==GH_EFFECT_UNSUPPORTED&&update.frontier==0x2fd078&&update.stage==9);C(fegetround()==modes[mode]&&fetestexcept(FE_ALL_EXCEPT)==FE_DIVBYZERO);memcpy(held_payload,v.payload,0x400);for(unsigned retry=0;retry<3;retry++)C(gh_game_esp_update_002fbed0(&update,&ub,particle.slot,particle.serial)==GH_EFFECT_UNSUPPORTED&&update.stage==9&&!memcmp(held_payload,v.payload,0x400)&&!memcmp(&held,&random,sizeof(held)));gh_esp_update_call copied=update;C(gh_game_esp_update_002fbed0(&copied,&ub,particle.slot,particle.serial)==GH_EFFECT_ARGUMENT);v.payload[0x140]=2;C(!gh_game_esp_update_002fbed0(&update,&ub,particle.slot,particle.serial)&&update.complete&&update.stage==16&&!memcmp(&held,&random,sizeof(held))&&fegetround()==modes[mode]&&fetestexcept(FE_ALL_EXCEPT)==FE_DIVBYZERO);uint8_t complete[0x400];memcpy(complete,v.payload,0x400);C(!gh_game_esp_update_002fbed0(&update,&ub,particle.slot,particle.serial)&&!memcmp(complete,v.payload,0x400));C(!fesetround(FE_TONEAREST));C(value(v.payload+0x228)==value(before+0x228)+1&&value(v.payload+0x234)==value(before+0x234)+1);for(unsigned j=0;j<3;j++){volatile float expected_position=value(before+0x40+j*4)+value(before+0x124+j*4);volatile float accelerated=value(before+0x124+j*4)+value(before+0x134+j*4);volatile float expected_velocity=accelerated*value(before+0x130);C(value(v.payload+0x40+j*4)==expected_position&&value(v.payload+0x124+j*4)==expected_velocity);}if(row){volatile float fraction=1.0f/15.0f;volatile float alpha=value(before+0x1a4)*fraction;C(value(v.payload+0x194)==alpha&&v.parent_emitter==-1);}C(!gh_effect_create_esp_abort_native(e,particle.slot,particle.serial));}
C(!fesetround(FE_TONEAREST));
/* Focused original2FB0E8 continuation partition: earlier emitter/region/pool
 * stages are independently exercised by test_game_effects_frame. */
const uint32_t work_flags[]={0x10000,0x10000,0x10000,0x4010000,0,0};const uint32_t configs[]={0,2,0x40,0x40,0,0};for(unsigned i=0;i<6;i++){created particle=make(&b,0);gh_effect_esp_view v=view(e,particle.slot);put((uint8_t*)&work+0x5e4,work_flags[i]);put(v.payload+0x104,configs[i]);if(i==5)put(v.payload+0x11c,((uint32_t)v.payload[0x11c]|(uint32_t)v.payload[0x11d]<<8|(uint32_t)v.payload[0x11e]<<16|(uint32_t)v.payload[0x11f]<<24)|0x10000u);uint8_t saved[0x400];memcpy(saved,v.payload,sizeof(saved));gh_game_scene_aux profile={.effect_stage=1};gh_effects_frame_bindings fb={.storage=storage,.work=&work,.globals=&globals,.profile=&profile,.clock=test_clock,.atan_coeff=envelope.atan_coeff,.policy=GH_STATIC_BINARY32_REFERENCE};gh_game_effects_frame frame={.b=fb,.stage=4,.bound=1};frame.identity=&frame;C(!gh_game_effects_frame_update_0030f228(&frame,&fb)&&!frame.stage);int invoked=i!=0&&i!=2;int updated=invoked&&i!=5;C(profile.effect_count_3c3bb8==(uint32_t)invoked);if(!updated)C(!memcmp(saved,v.payload,sizeof(saved)));else C(value(v.payload+0x228)==value(saved+0x228)+1);C(!gh_effect_create_esp_abort_native(e,particle.slot,particle.serial));}put((uint8_t*)&work+0x5e4,0);
/* Four distinct originalearly-death sites pause+unlink, neverclearactive.
 * Then originalnextframe marks hidden and the following VDtor(flags2) clears
 * active via dispatcher; a foreign sibling remains alive throughout. */
for(unsigned death=0;death<4;death++){created victim=make(&b,death==3?2:0),sibling=make(&b,0);gh_effect_esp_view v=view(e,victim.slot);if(death==0){v.payload[0x22c]=1;v.payload[0x22d]=0;}if(death==1)v.payload[0x24c]=1;if(death==2){v.payload[0x238]=1;v.payload[0x239]=0;v.payload[0x22e]=1;v.payload[0x22f]=0;}if(death==3){float twenty=20,zero=0;memcpy(v.payload+0x234,&twenty,4);v.payload[0x22e]=20;v.payload[0x22f]=0;memcpy(v.payload+0x194,&zero,4);}gh_esp_update_call ending={0};C(!gh_game_esp_update_002fbed0(&ending,&ub,victim.slot,victim.serial)&&ending.retired&&ending.complete&&view(e,victim.slot).parent_emitter==-1&&view(e,sibling.slot).initialized);unsigned off=(victim.slot>>5)*4;uint32_t mask=0x80000000u>>(victim.slot&31),active_word,paused_word;memcpy(&active_word,esp+0x100000+off,4);memcpy(&paused_word,esp+0x100080+off,4);C((active_word&mask)&&(paused_word&mask));for(unsigned frame_index=0;frame_index<2;frame_index++){gh_game_scene_aux profile={.effect_stage=1};gh_effects_frame_bindings fb={.storage=storage,.work=&work,.globals=&globals,.profile=&profile,.clock=test_clock,.atan_coeff=envelope.atan_coeff,.policy=GH_STATIC_BINARY32_REFERENCE};gh_game_effects_frame frame={.b=fb,.stage=4,.bound=1};frame.identity=&frame;C(!gh_game_effects_frame_update_0030f228(&frame,&fb));memcpy(&active_word,esp+0x100000+off,4);C(frame_index?!(active_word&mask):(active_word&mask));}C(gh_effect_create_esp_at(e,victim.slot,&v)==GH_EFFECT_UNSUPPORTED&&view(e,sibling.slot).initialized);C(!gh_effect_create_esp_abort_native(e,sibling.slot,sibling.serial));}
/* Actual R100 emitter and ESP owners enter Main5 op4. Preserve the original
 * paused/hidden exclusions and leave active bits for the normal update. */
created ending=make(&b,0),hidden=make(&b,0);
const uint32_t ending_bit=0x80000000u>>(ending.slot&31),hidden_bit=0x80000000u>>(hidden.slot&31);
const unsigned ending_word=(ending.slot>>5)*4,hidden_word=(hidden.slot>>5)*4;
put(emit+0x26020,0x40000000);
put(esp+0x100100+hidden_word,hidden_bit);
C(gh_scene_exit_effects_00305b90(esp,0x100180,emit,0x26060,&f)==GH_SCENE_EXIT_UNSUPPORTED&&f==0x302918);
uint8_t save_bytes[GH_CORE_SAVE_BYTES];
gh_core_save_view save={save_bytes,sizeof(save_bytes)};
C(!gh_core_save_system_init(save));
gh_game_loop loop={.work=&work,.game_state={5,1}};
gh_game_scene_exit scene_exit={.phase=1,.operation=4};
const gh_game_scene_exit_bindings exit_bindings={.loop=&loop,.save=save,.effects=e,
 .scene_buffer_a=esp,.scene_buffer_a_size=0x100180,.scene_buffer_b=emit,.scene_buffer_b_size=0x26060};
C(gh_game_scene_exit_002a67d0(&scene_exit,&exit_bindings)==GH_SCENE_EXIT_ARGUMENT&&
 scene_exit.operation==5&&scene_exit.frontier==0x2c3858);
C((emitter(e,0).payload[0x117]&0x40)&&!(emitter(e,1).payload[0x117]&0x40));
C(view(e,ending.slot).initialized&&view(e,ending.slot).parent_emitter==-1&&view(e,hidden.slot).initialized);
uint32_t paused,active;
memcpy(&paused,esp+0x100080+ending_word,4);
memcpy(&active,esp+0x100000+ending_word,4);
C((paused&ending_bit)&&(active&ending_bit));
memcpy(&paused,esp+0x100080+hidden_word,4);
C(!(paused&hidden_bit));
puts("PASS actual R100 State5 op4 emitter retirement request and ESP pause, preserving active and skipped slots");
gh_scene_ray_triangle_state_release(&actors);gh_effect_create_forget_on_owner_destroy(e);gh_disc_close(disc);free(e);free(esp);free(emit);free(core);free(roombytes);free(elf);puts("PASS actualR100 fourambient ESP firstupdates16cases/fourFEnv realtimer/colourfade15/scale/euler/position/velocity/detach; lateerrorprefix noRNG/counterreplay and copiedcursor rejection; nativefixture notworld/renderclaim");return 0;}
