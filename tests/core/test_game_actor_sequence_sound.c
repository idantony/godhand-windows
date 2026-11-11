#include "game_actor_motion_call.h"
#include "game_actor_motion_ik.h"
#include "game_actor_motion_step.h"
#include "game_actor_sequence.h"
#include "game_effect_release.h"
#include "disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fenv.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL%d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t rd(const uint8_t*p){uint32_t x;memcpy(&x,p,4);return x;}
static void wr(uint8_t*p,uint32_t x){memcpy(p,&x,4);}
static uint8_t*load(gh_disc*d,const char*path,size_t*n){size_t i;gh_disc_entry e;C(!gh_disc_find(d,path,&i)&&!gh_disc_entry_at(d,i,&e));*n=(size_t)e.size;uint8_t*b=malloc(*n);C(b&&!gh_disc_read(d,i,0,b,*n));return b;}
static void sequence_effect_fixture(gh_disc *disc)
{
 size_t size;
 uint8_t *resource=load(disc,"em/em02.dat",&size);
 uint8_t actor[GH_TITLE_PLAYER_BYTES]={0};
 gh_actor_model model={.root={.bytes=actor,.size=sizeof(actor)}};
 gh_boot_work work={0};
 const size_t sequence=0x297300;
 gh_actor_motion motion={.b={.model=&model,.resource=resource,.resource_bytes=size,.work=&work},
    .bound=1,.complete=1,.sequence_valid=1,.sequence_offset=sequence};
 const uint8_t record[]={0,0,0,0,1,0,0,0x58,0x2d,0xff,3,0};
 C(size>=sequence+0xec && !memcmp(resource+sequence+0xe0,record,sizeof(record)));
 memset(actor+0x3b0,0xa5,12);
 wr(actor+0x3a8,0x3f800000);wr(actor+0x440,0x3f800000);wr(actor+0x444,0x3f800000);
 uint32_t frontier;
 const uint32_t frame[]={0,0x3f800000,0x40000000};
 for(unsigned i=0;i<3;i++){
  wr(actor+0x430,frame[i]);wr(actor+0x41c,frame[i]);
  C(!gh_actor_motion_sequence_002c7fd0(&motion,1,GH_STATIC_BINARY32_REFERENCE,&frontier)&&!frontier);
  C(actor[0x3ae]==(i==1) && actor[0x3af]==0 && actor[0x410]==(i==1?0x80:0));
  if(i==0)C(actor[0x3b0]==0xa5);
  if(i==1){
   const uint8_t expected[]={0x58,0x2d,0xa5,0xa5,0,0,0,0,0xff,3,0xa5,0xa5};
   C(!memcmp(actor+0x3b0,expected,sizeof(expected)));
  }
 }
 free(resource);
}
typedef struct {gh_effect_release_bindings effects;unsigned calls,completed,block42,block53;int32_t kinds[8];uint32_t modes[8];} bridge;
static gh_actor_motion_status release_effect(void*ctx,const gh_title_player_view*actor,int32_t kind,uint32_t mode,uint32_t*f){bridge*b=ctx;b->calls++;if((kind==42&&b->block42)||(kind==53&&b->block53)){if(kind==42)b->block42=0;else b->block53=0;*f=0x306200;return GH_ACTOR_MOTION_UNSUPPORTED;}
 unsigned slot=0;gh_effect_status e=gh_game_effect_release_00306200(&b->effects,actor,kind,mode,f,&slot);if(e)return GH_ACTOR_MOTION_CALLEE;C(b->completed<8);b->kinds[b->completed]=kind;b->modes[b->completed++]=mode;return GH_ACTOR_MOTION_OK;}

typedef struct {unsigned calls,completed;int block_second;int16_t expected;const gh_actor_model*model;uint32_t handles[8];} sound_observer;
static gh_actor_motion_status sound_fixture(void*o,const gh_motion_sound_request*r,uint32_t*handle,uint32_t*f){sound_observer*s=o;s->calls++;C(r->model==s->model&&r->actor==&s->model->root&&!r->other_actor&&!r->part&&r->bank==0&&r->program==s->expected&&!r->argument7&&!r->argument8);if(s->calls==2&&s->block_second){s->block_second=0;*f=0x2cdd18;return GH_ACTOR_MOTION_UNSUPPORTED;}*handle=s->completed?100+s->completed:0;s->handles[s->completed++]=*handle;return GH_ACTOR_MOTION_OK;}
int main(int argc,char**argv){C(argc==2);gh_disc*d;C(!gh_disc_open(argv[1],&d));sequence_effect_fixture(d);size_t rn,n,cn;uint8_t*r=load(d,"st1/r100.dat",&rn),*data=load(d,"pl/pl00.dat",&n),*core=load(d,"etc/core.dat",&cn);size_t en=(size_t)gh_disc_boot_elf_size(d);uint8_t*elf=malloc(en);C(elf&&!gh_disc_read_boot_elf(d,0,elf,en));gh_disc_close(d);
 gh_bundle room,bundle;gh_bundle_entry md,texture;C(!gh_bundle_open(r,rn,&room)&&!gh_bundle_open(data,n,&bundle)&&!gh_bundle_find(&bundle,"MD",0,&md)&&!gh_bundle_find(&bundle,"TM3",0,&texture));gh_heap*heap;C(!gh_heap_create(16*1024*1024,&heap));gh_room_pools pools={0};C(!gh_game_room_pools_initialize_002a5d80(&pools,&room,heap));uint8_t raw[GH_TITLE_PLAYER_BYTES]={0};gh_title_player_view view;C(!gh_title_player_construct_00107e40(raw,sizeof(raw),&view));gh_boot_work work={0};gh_actor_model model={0};int original;C(gh_actor_model_initialize_00149738(&model,&view,&work,&pools,&md,&texture,0,0,GH_STATIC_BINARY32_REFERENCE,&original)==GH_ACTOR_MODEL_UNSUPPORTED&&model.parts_ready);
 gh_game_effects*effects=calloc(1,sizeof(*effects));uint8_t*a=calloc(1,0x100180),*b=calloc(1,0x26060);C(a&&b&&effects&&!gh_game_effects_initialize_00305a68(effects,a,0x100180,b,0x26060,core,cn,&room));bridge observer={.effects={.effects=effects,.esp=a,.esp_bytes=0x100180,.emitters=b,.emitter_bytes=0x26060}};gh_motion_ik_globals globals={0};C(!gh_motion_ik_globals_from_elf(&globals,elf,en));gh_actor_motion current={0};gh_actor_motion_bindings bindings={.model=&model,.resource=data,.resource_bytes=n,.work=&work,.effect=release_effect,.context=&observer,.ik_globals=&globals};size_t motion=rd(data+0x28),seq=rd(data+0x2c);
 C(!gh_actor_motion_begin_002a8578(&current,&bindings,motion,motion,0)&&current.complete&&!current.sequence_valid&&observer.completed==1); /* cold570kind0 realcold306200 scan */

 uint8_t save[GH_CORE_SAVE_BYTES]={0};sound_observer sounds={.block_second=1,.expected=0x1e,.model=&model};bindings.save=(gh_core_save_view){save,sizeof(save)};bindings.save_binding_known=1;bindings.sound=sound_fixture;bindings.sound_context=&sounds;raw[0x2fe]=0;raw[0x2ff]=1;gh_actor_motion_call call={0};size_t alternate=rd(data+0x2b8),candidate=rd(data+0x2bc);C(!gh_actor_motion_call_open_parameters_002a8578(&call,&current,&bindings,alternate,candidate,0,10,0,1)&&!gh_actor_motion_call_step_002a8578(&call));unsigned sound_table=data[candidate+12]|(unsigned)data[candidate+13]<<8;uint8_t original_records[48];memcpy(original_records,data+candidate+sound_table,48);data[candidate+sound_table+12]=8;data[candidate+sound_table+13]=0;uint32_t pc;int ended;for(unsigned i=0;i<8;i++)C(!gh_actor_motion_step_00157c18(&current,1,GH_STATIC_BINARY32_REFERENCE,&ended,&pc));C(!sounds.calls&&rd(raw+0x430)==0x41000000&&rd(raw+0x438)==0x40000000);
 C(gh_actor_motion_step_00157c18(&current,1,GH_STATIC_BINARY32_REFERENCE,&ended,&pc)==GH_ACTOR_MOTION_UNSUPPORTED&&pc==0x2cdd18&&sounds.calls==2&&sounds.completed==1&&sounds.handles[0]==0&&current.sequence_stage==1&&current.sequence_index==1&&current.sequence_pending==4&&!current.step_stage&&rd(raw+0x430)==0x41000000&&rd(raw+0x438)==0x40000000);C(gh_actor_motion_call_open_parameters_002a8578(&call,&current,&bindings,motion,seq,0,10,0,1)==GH_ACTOR_MOTION_BUSY);C(gh_actor_motion_step_00157c18(&current,.5f,GH_STATIC_BINARY32_REFERENCE,&ended,&pc)==GH_ACTOR_MOTION_BUSY&&sounds.calls==2);
 /* MutateONLY privatefixture afterarguments were captured. Retrying original
  * secondrecord mustnot re-read these nor re-emit firstcompletedrecord. */
 data[candidate+sound_table+12+2]=0x4a;save[0xae]=4;wr(raw+0x474,3);raw[0x410]=0x5a;C(!gh_actor_motion_step_00157c18(&current,1,GH_STATIC_BINARY32_REFERENCE,&ended,&pc)&&sounds.calls==3&&sounds.completed==2&&!current.sequence_stage&&!current.sequence_pending&&raw[0x410]==0x5a&&rd(raw+0x430)==0x41100000&&rd(raw+0x438)==0x3f800000);memcpy(data+candidate+sound_table,original_records,48);
 /* Actualfour originalrecords: noISO edits, firstone is frame8 program1E.
  * Independently selected floors/costume demonstrate originalpuremapping. */
 const unsigned floors[]={2,4,0,0,3,0},costumes[]={0,0,4,5,0,4},programs[]={0x48,0x52,0x109,0x109,0x57,0x10b},source_programs[]={0x1e,0x1e,0x1e,0x1e,0x4a,0x4a};const int modes[]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};for(unsigned k=0;k<6;k++){data[candidate+sound_table+2]=(uint8_t)source_programs[k];C(!fesetround(modes[k%4]));feclearexcept(FE_ALL_EXCEPT);feraiseexcept(FE_DIVBYZERO);int exceptions=fetestexcept(FE_ALL_EXCEPT);sounds=(sound_observer){.model=&model,.expected=(int16_t)programs[k]};save[0xae]=(uint8_t)costumes[k];wr(raw+0x474,floors[k]);bindings.save=k==1?(gh_core_save_view){0}:(gh_core_save_view){save,sizeof(save)};C(!gh_actor_motion_call_open_parameters_002a8578(&call,&current,&bindings,alternate,candidate,0,10,0,1)&&!gh_actor_motion_call_step_002a8578(&call));for(unsigned i=0;i<9;i++)C(!gh_actor_motion_step_00157c18(&current,1,GH_STATIC_BINARY32_REFERENCE,&ended,&pc));C(sounds.calls==1&&sounds.completed==1&&sounds.handles[0]==0&&rd(raw+0x430)==0x41100000&&fegetround()==modes[k%4]&&fetestexcept(FE_ALL_EXCEPT)==exceptions);}
 C(!fesetround(FE_TONEAREST));memcpy(data+candidate+sound_table,original_records,48);save[0xae]=0;wr(raw+0x474,0);bindings.save=(gh_core_save_view){save,sizeof(save)};sounds=(sound_observer){.model=&model,.expected=0x1e};C(!gh_actor_motion_call_open_parameters_002a8578(&call,&current,&bindings,alternate,candidate,0,10,0,1)&&!gh_actor_motion_call_step_002a8578(&call));for(unsigned i=0;i<8;i++)C(!gh_actor_motion_step_00157c18(&current,1,GH_STATIC_BINARY32_REFERENCE,&ended,&pc));/* Explicitnumeric faultAFTERthe genuineSEQ callback path. */wr(raw+0x438,0x7fc00000);C(gh_actor_motion_step_00157c18(&current,1,GH_STATIC_BINARY32_REFERENCE,&ended,&pc)==GH_ACTOR_MOTION_UNSUPPORTED&&pc==0x157c18&&current.step_stage==4&&!current.sequence_stage&&sounds.calls==1);wr(raw+0x438,0x40000000);raw[0x410]=0x77;C(!gh_actor_motion_step_00157c18(&current,1,GH_STATIC_BINARY32_REFERENCE,&ended,&pc)&&sounds.calls==1&&raw[0x410]==0x77&&rd(raw+0x430)==0x41100000);
 sounds=(sound_observer){.model=&model,.expected=0x1e};data[candidate+sound_table+4]=2;C(!gh_actor_motion_call_open_parameters_002a8578(&call,&current,&bindings,alternate,candidate,0,10,0,1)&&!gh_actor_motion_call_step_002a8578(&call));for(unsigned i=0;i<8;i++)C(!gh_actor_motion_step_00157c18(&current,1,GH_STATIC_BINARY32_REFERENCE,&ended,&pc));C(gh_actor_motion_step_00157c18(&current,1,GH_STATIC_BINARY32_REFERENCE,&ended,&pc)==GH_ACTOR_MOTION_UNSUPPORTED&&pc==0x31cef8&&current.sequence_pending==3&&current.sequence_variations==2&&!sounds.calls);data[candidate+sound_table+4]=0;C(gh_actor_motion_step_00157c18(&current,1,GH_STATIC_BINARY32_REFERENCE,&ended,&pc)==GH_ACTOR_MOTION_UNSUPPORTED&&pc==0x31cef8&&current.sequence_variations==2&&!sounds.calls);memcpy(data+candidate+sound_table,original_records,48);
 C(!gh_actor_model_release(&model)&&!gh_game_room_pools_release(&pools)&&!gh_heap_destroy(heap));free(effects);free(a);free(b);free(r);free(data);free(core);free(elf);puts("PASS actualPL00 altSEQ frame8 typedattachedrequest,sameSaveAE/floor remap,costume4/5/nullrecord, originalhandle0 accepted; explicittwoeventfixture firstcomplete+secondfail retriesONLYpendingoriginalargs; rate/newcallblocked. Consumer isfixture, NOTaudio playbackproof");return 0;}
