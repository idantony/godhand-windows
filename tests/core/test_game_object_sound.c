#include "game_object_sound.h"
#include <stdio.h>
#include <stdlib.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);}}while(0)
static unsigned calls;static int fail_once;
static gh_object_status counting(void*ctx,int32_t slot,uint32_t*f){calls++;if(fail_once&&calls==4){fail_once=0;if(f)*f=0x2cb340;return GH_OBJECT_UNSUPPORTED;}return gh_sound_object_slot_release_002cb340(ctx,slot,f);}
int main(int argc,char**argv){gh_disc*d;gh_audio_registry*r;gh_sound_control c={0};gh_audio_output*out;int32_t err;gh_se_voice_engine*v;gh_se_mix mix;
 C(argc==2&&!gh_disc_open(argv[1],&d)&&!gh_audio_registry_create(&r)&&!gh_audio_startup_load(r,d)&&!gh_sound_control_from_startup(&c,d,r));
 C(!gh_audio_output_create(&out,&err)&&!gh_audio_output_set_gain(out,0)&&!gh_se_mix_boot_defaults_002c9e78(&mix,0,GH_SE_DSP_NATIVE_LINEAR_REFERENCE)&&!gh_se_voice_create(r,out,&mix,&v)&&!gh_se_voice_bind_origin_seh(v,NULL,0));
 uint8_t descriptor[48];static const int core_programs[]={0x2d,0x15e,0x160,0x161};for(unsigned i=0;i<4;i++)C(!gh_audio_get_program(r,0,core_programs[i],0,descriptor)&&!descriptor[0x21]);
 size_t index;gh_disc_entry entry;C(!gh_disc_find(d,"se/r020.ses",&index)&&!gh_disc_entry_at(d,index,&entry));uint8_t*ses=malloc((size_t)entry.size);gh_audio_token token;C(ses&&!gh_disc_read(d,index,0,ses,(size_t)entry.size)&&!gh_audio_commit_se(r,2,ses,(size_t)entry.size,&token));free(ses);static const int room_programs[]={0,10,11,12};for(unsigned i=0;i<4;i++)C(!gh_audio_get_program(r,2,room_programs[i],0,descriptor)&&!descriptor[0x21]);C(!gh_audio_release_bank(r,2));
 for(unsigned i=0;i<16;i++)C(c.recent[i].bank==-1&&c.recent[i].program==-1&&!c.recent[i].key);
 gh_object_sound_binding b={&c,v};uint32_t f=0;C(!gh_sound_object_slot_release_002cb340(&b,-1,&f)&&c.recent[10].bank==3&&!c.recent[10].program);
 c.recent[10]=(gh_sound_recent){0,0,123};C(!gh_sound_object_slot_release_002cb340(&b,-1,&f)&&!c.recent[10].bank&&c.recent[10].key==123);
 C(gh_sound_object_slot_release_002cb340(&b,0,&f)==GH_OBJECT_UNSUPPORTED&&f==0x2cb340);
 c.recent_pos=15;C(!gh_sound_recent_push_002cbdc0(&c,2,12,0x12345678)&&!c.recent_pos&&c.recent[0].bank==2&&c.recent[0].program==12&&c.recent[0].key==0x12345678);
 c.recent_pos=16;C(gh_sound_recent_push_002cbdc0(&c,0,0,0)==GH_VOICE_UNSUPPORTED);c.recent_pos=0;
 uint32_t handle;C(!gh_se_voice_request_origin(v,0,0x2d,-1,-1,0,0,&handle)&&handle);
 C(gh_sound_object_slot_release_002cb340(&b,-1,&f)==GH_OBJECT_UNSUPPORTED&&f==0x2cbe68&&c.recent[10].key==123);
 C(!gh_se_voice_scene_exit_begin(v)&&!gh_se_voice_active_nodes(v));
 gh_stage_slots slots={0};gh_game_objects*o;C(!gh_game_objects_create(&slots,1,&o)&&!gh_stage_slots_reset(&slots,NULL));
 C(!gh_game_objects_release_nonpinned_001fef30(o,gh_sound_object_slot_release_002cb340,&b,&f));
 for(unsigned i=0;i<32;i++)C(slots.records[i*0x5c]==8);
 C(c.recent[10].key==123&&!gh_game_objects_reset_records_001ff028(o,&f));for(unsigned i=0;i<32;i++)C(!slots.records[i*0x5c]);
 gh_game_objects_destroy(o);
 gh_stage_slots global={0};C(!gh_game_objects_create(&global,0,&o)&&!gh_stage_slots_reset(&global,NULL));uint8_t source=1;int original;C(!gh_game_objects_add_pinned_001fece0(o,0x100,&source,1,&original)&&original);
 calls=0;fail_once=1;C(gh_game_objects_release_nonpinned_001fef30(o,counting,&b,&f)==GH_OBJECT_UNSUPPORTED&&calls==4);C(!gh_game_objects_release_nonpinned_001fef30(o,counting,&b,&f)&&calls==791);gh_object_view view;C(!gh_game_objects_find(o,0x100,&view)&&view.bytes==&source&&view.use_count==-100&&view.state==2);gh_game_objects_destroy(o);gh_se_voice_destroy(v);gh_audio_output_destroy(out);gh_audio_registry_destroy(r);gh_disc_close(d);puts("PASS actual boot ring, proven minus1 alias, real active-list refusal, exact push and room32 release/reset");return 0;}
