#include "game_event_create.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x)do{if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);}}while(0)
int main(int argc,char**argv){C(argc==2);gh_disc*d;C(!gh_disc_open(argv[1],&d));size_t en=(size_t)gh_disc_boot_elf_size(d);uint8_t*elf=malloc(en);C(elf&&!gh_disc_read_boot_elf(d,0,elf,en));gh_game_event event={0};C(!gh_game_event_from_elf(&event,elf,en));event.fields.data_number=1;event.fields.state=2;gh_boot_work work={0};gh_heap*heap;C(!gh_heap_create(2*1024*1024,&heap));gh_dvd_jobs dvd;gh_dvd_jobs_init(&dvd,d);gh_event_module module={0};gh_event_resource resource={0};gh_event_components components={0};gh_audio_registry*registry;C(!gh_audio_registry_create(&registry));size_t index;gh_disc_entry file;C(!gh_disc_find(d,"se/core.ses",&index)&&!gh_disc_entry_at(d,index,&file));uint8_t*core=malloc((size_t)file.size);gh_audio_token token;C(core&&!gh_disc_read(d,index,0,core,(size_t)file.size)&&!gh_audio_commit_se(registry,0,core,(size_t)file.size,&token));free(core);gh_audio_output*out;int32_t native;C(!gh_audio_output_create(&out,&native)&&!gh_audio_output_set_gain(out,0));gh_se_mix mix={0x5a00,1,0,GH_SE_DSP_NATIVE_LINEAR_REFERENCE};gh_se_voice_engine*voices;C(!gh_se_voice_create(registry,out,&mix,&voices));uint32_t stopped;C(!gh_se_voice_request_origin(voices,0,0x160,-1,-1,0,0,&stopped)&&stopped);gh_event_create_bindings b={.event=&event,.work=&work,.module=&module,.components=&components,.resource=&resource,.dvd=&dvd,.heap=heap,.voices=voices,.boot_elf=elf,.boot_elf_bytes=en};gh_event_create_state state={0};int done=9;C(gh_game_event_create_00296958(&state,&b,&done)==GH_EVENT_UNSUPPORTED&&!done&&state.frontier==0x2d1900&&state.bank==52&&state.substep==2&&module.requested&&module.allocation.pointer&&event.fields.phase==0);gh_se_voice_info info;C(gh_se_voice_query(voices,stopped,&info)==GH_VOICE_BOUNDS);uint32_t preserved;C(!gh_se_voice_request_origin(voices,0,0x161,-1,-1,0,0,&preserved)&&preserved);uint32_t job=module.job;C(gh_game_event_create_00296958(&state,&b,&done)==GH_EVENT_UNSUPPORTED&&state.frontier==0x2d1900&&module.job==job&&!gh_se_voice_query(voices,preserved,&info)&&info.pending);
 int ready=0;for(unsigned i=0;i<8&&!ready;i++)C(!gh_dvd_pump(&dvd)&&!gh_game_event_module_poll_00297428(&module,&ready));C(ready&&module.loaded&&module.file_bytes==80431);
 /* Isolated phase dispatch fixtures below. No successful fakeaudio/link
  * callback is supplied and no fullproductionphase0->phase1 claim is made. */
 state=(gh_event_create_state){0};event.fields.phase=1;C(gh_game_event_create_00296958(&state,&b,&done)==GH_EVENT_UNSUPPORTED&&state.frontier==0x2be890&&!components.bound&&!(event.fields.flags&0x80));
 state=(gh_event_create_state){0};event.fields.phase=2;C(gh_game_event_create_00296958(&state,&b,&done)==GH_EVENT_UNSUPPORTED&&state.frontier==0x2d19d8&&event.fields.phase==3&&resource.requested&&resource.file_bytes==873872&&(event.fields.flags&0x100)&&!resource.loaded);ready=0;for(unsigned i=0;i<8&&!ready;i++)C(!gh_dvd_pump(&dvd)&&!gh_game_event_resource_poll_002975d0(&resource,&ready));C(ready&&resource.loaded);state=(gh_event_create_state){0};event.fields.phase=4;C(gh_game_event_create_00296958(&state,&b,&done)==GH_EVENT_UNSUPPORTED&&state.frontier==0x2d1b70&&!done&&event.fields.state==2);C(!gh_game_event_resource_abort(&resource));
 /* Verify the native state transitions on the original component banks.
  * The flag writes below represent the separate actor task's results. */
 C(!gh_event_components_bind(&components,&module,elf,en));
 C(!gh_event_components_construct_00297680(&components,1));
 gh_event_component_view actor;
 C(!gh_event_component_at(&components,2,&actor));
 uint8_t menu_enabled=0;
 b.system_menu_enabled=&menu_enabled;
 event.fields.phase=5;
 event.fields.flags=0x183;
 state=(gh_event_create_state){0};
 C(!gh_game_event_create_00296958(&state,&b,&done)&&!done&&event.fields.phase==5&&!menu_enabled);
 actor.bytes[0]=3;
 C(gh_game_event_create_00296958(&state,&b,&done)==GH_EVENT_UNSUPPORTED&&state.frontier==0x2d1e18&&!done&&!menu_enabled);
 actor.bytes[0]=1;
 event.fields.unknown06=7;event.fields.unknown07=8;
 C(!gh_game_event_create_00296958(&state,&b,&done)&&done&&menu_enabled);
 C(event.fields.state==3&&!event.fields.phase&&!event.fields.unknown06&&!event.fields.unknown07&&event.fields.flags==0x193);
 menu_enabled=0;
 event.fields.state=2;event.fields.phase=5;event.fields.flags=0x1a3;
 state=(gh_event_create_state){0};
 C(!gh_game_event_create_00296958(&state,&b,&done)&&!done&&!menu_enabled);
 C(event.fields.state==2&&event.fields.phase==6&&event.fields.flags==0x1b3);
 C(!gh_game_event_create_00296958(&state,&b,&done)&&!done&&event.fields.phase==6&&!menu_enabled);
 event.fields.flags|=0x40;
 C(!gh_game_event_create_00296958(&state,&b,&done)&&done&&event.fields.state==3&&!event.fields.phase&&menu_enabled);
 gh_game_event absent={0};C(!gh_game_event_from_elf(&absent,elf,en));absent.fields.state=2;absent.fields.phase=2;absent.fields.data_number=0x123;gh_event_resource missing={0};b.event=&absent;b.resource=&missing;state=(gh_event_create_state){0};C(!gh_game_event_create_00296958(&state,&b,&done)&&!done&&state.finished&&absent.fields.state==4&&!absent.fields.phase&&!missing.allocation.pointer&&missing.requested);C(!gh_game_event_resource_abort(&missing));C(!gh_game_event_module_abort(&module)&&!gh_heap_used(heap)&&!gh_heap_destroy(heap));gh_se_voice_destroy(voices);gh_audio_output_destroy(out);gh_audio_registry_destroy(registry);gh_disc_close(d);free(elf);puts("PASS actual Eventcreate bank52/queuedSE cleanup->realModuleDVD->strictchannel1, stableprefixretry, realev01data873872 IO and requiredLink/voicewait; realmissingfile release-state0return. No fakeplaydone orsuccessfulstubservices.");return 0;}
