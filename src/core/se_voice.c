/* Recovered MC origin SE slice; upstream eaae18d0dfab1f9a0df276ae5ca1581ad326faf3
 * MIT LICENSE.upstream. Actual ELF and user's snddrv.irx evidence:
 * analysis/se-voice-semantics.json. Native DSP adaptation is explicit. */
#include "se_voice.h"
#include "sound_control.h"
#include "../assets/adx.h"
#include "game_actor_model.h"
#include "game_camera.h"
#include "game_actor_scroll_links.h"
#include "game_ray_query.h"
#include "../assets/se_pitch.h"
#include <math.h>
#include <fenv.h>
#include <stdlib.h>
#include <string.h>
#define NONE 255
struct voice {gh_se_voice_info info;gh_adpcm_pcm pcm;uint32_t age,ee_age;uint16_t base_gain,origin_right_gain;int next;uint8_t used,priority,group_first,group_count,replace_equal;
 gh_actor_model *model;int32_t part_index;const void *source_owner,*request_key;
 gh_se_attached_probe probe;void *probe_context;gh_se_attached_random random;void *random_context;
 uint8_t descriptor[48],setup_stage,variants;int16_t requested_bank,requested_program;
 const gh_se_attached_environment *ts_environment;uint16_t ts_distance;
 uint32_t flags18,flags1c;uint16_t selected_pitch,submitted_pitch;int16_t selected_offset;uint8_t pitch_setup,spatial_slow,combat_profile,external_channels,door_spatial,emad_loop,emad_occluded,verified_live_gain;uint16_t spatial_gain;};
typedef struct {
 gh_disc *disc;
 const gh_sound_control *control;
 gh_adx_key key;
 int16_t decibels[128];
} sequence_audio;
struct gh_se_voice_engine {const gh_audio_registry *registry;gh_audio_output *output;gh_se_mix mix;struct voice voices[64];int channels[51],head;uint32_t serial,frontier;const void *origin_seh;size_t origin_seh_size;uint8_t origin_scene_bound;struct {uint32_t handle;int16_t ticks;} stops[48];unsigned stop_count;uint32_t actor_stop_retry[8];gh_se_pitch_reference pitch;sequence_audio sequence;gh_se_attached_failure attached_failure;};
static unsigned h(const uint8_t *p){return (unsigned)p[0]|(unsigned)p[1]<<8;}
static uint32_t w(const uint8_t *p){return (uint32_t)h(p)|(uint32_t)h(p+2)<<16;}
static float sound_float(uint32_t bits){float value;memcpy(&value,&bits,4);return value;}
static int32_t floor_shift(int64_t n,unsigned shift){int64_t d=INT64_C(1)<<shift;return (int32_t)(n>=0?n/d:-((-n+d-1)/d));}
static gh_voice_status unsupported(gh_se_voice_engine *s,uint32_t a){s->frontier=a;return GH_VOICE_UNSUPPORTED;}
gh_voice_status gh_se_mix_boot_defaults_002c9e78(gh_se_mix *mix,uint32_t flags,gh_se_dsp_policy policy)
{
 if(!mix)return GH_VOICE_ARGUMENT;if(policy!=GH_SE_DSP_NATIVE_LINEAR_REFERENCE)return GH_VOICE_UNSUPPORTED;
 *mix=(gh_se_mix){0x5a00,1,flags,policy};return GH_VOICE_OK;
}
static void drop(gh_se_voice_engine *s,int i,int stop)
{
 struct voice *v=s->voices+i;
 if(v->info.native_voice && stop)(void)gh_audio_output_stop(s->output,v->info.native_voice);
 if(v->info.channel<51 && s->channels[v->info.channel]==i)s->channels[v->info.channel]=-1;
 int *link=&s->head;while(*link!=-1){if(*link==i){*link=v->next;break;}link=&s->voices[*link].next;}
 if(v->model){(void)gh_actor_model_sound_return(v->model);v->model=NULL;}
 gh_audio_pcm_release(&v->pcm);v->used=0;v->request_key=NULL;
}
/* Actual 80FF with 5FC0 or 5FCB shares the key-on envelope. The renderer
 * ends at the one-shot sample end. The release rate only applies at key-off.
 * Native linear resampling uses the existing pinned envelope reference. */
typedef struct {uint32_t level,counter,phase;} keyon_envelope;

static uint32_t keyon_envelope_step(keyon_envelope *e)
{
 if(e->phase==1){
  e->counter+=e->level>0x6000?8192:32768;
  if(e->counter>=32768){
   e->counter=0;
   e->level+=14336;
   if(e->level>=32767){e->level=32767;e->phase=2;}
  }
 }else if(e->phase==2){
  e->counter+=2048;
  if(e->counter>=32768){e->counter=0;e->level-=(e->level*8+32767)/32768;}
  e->phase=3;
 }else if(++e->counter>=32768){
  e->counter=0;
  e->level=e->level>=5?e->level-5:0;
 }
 return e->level;
}

static gh_voice_status render(gh_adpcm_pcm *pcm,uint16_t pitch)
{
 uint64_t count;int16_t *out;
 keyon_envelope env={.phase=1};
 if(pcm->frames>(UINT64_MAX-pitch+1)/4096)return GH_VOICE_BOUNDS;
 count=((uint64_t)pcm->frames*4096+pitch-1)/pitch;
 if(count>SIZE_MAX/sizeof(*out))return GH_VOICE_BOUNDS;
 out=malloc((size_t)count*sizeof(*out));if(!out)return GH_VOICE_MEMORY;
 for(uint64_t i=0;i<count;++i){
  uint64_t pos=i*pitch;size_t index=(size_t)(pos>>12);unsigned fraction=(unsigned)(pos&4095);int32_t a=pcm->samples[index],b=index+1<pcm->frames?pcm->samples[index+1]:0,value;
  const uint32_t level=keyon_envelope_step(&env);
  value=floor_shift((int64_t)a*(4096-fraction)+(int64_t)b*fraction,12);
  out[i]=(int16_t)floor_shift((int64_t)value*level,15);
 }
 free(pcm->samples);pcm->samples=out;pcm->frames=(size_t)count;return GH_VOICE_OK;
}
/* These two physical programs are the two original MT-selected
 * recordings of EM0A impact program 2A. The model supplies dependency 20A;
 * the attached adapter has already resolved that dependency in the live bank. */
/* The attached adapter resolves this model's original dependency through
 * the live enemy-bank registry. Check exact EM0A program and sample profiles. */
static const uint8_t *r103_voice_actor(const struct voice *v, uint8_t appearance)
{
 const gh_actor_model *model=v->model;
 if(v->part_index!=-1||!model||!model->work||model->work->room!=0x103||
    !model->root.bytes||model->root.size<=0x1650||model->root.original_vtable214!=0x4479c8)
  return NULL;
 const uint8_t *actor=model->root.bytes;
 return h(actor+0x2fe)==0x204&&actor[0x1650]==appearance?actor:NULL;
}

static const uint8_t *r103_boss_voice_actor(const struct voice *v)
{
 const uint8_t *actor=r103_voice_actor(v,15);
 if(!actor)return NULL;
 const unsigned stamp=h(actor+0x640);
 return stamp<2&&w(actor+0x564)==0x214u+stamp?actor:NULL;
}

static int r103_boss_grab_attempt(const uint8_t *actor)
{
 return actor&&h(actor+0x2f4)==0x4b00&&actor[0x2f6]==1&&!actor[0x2f7];
}

static int r103_em14_hold_voice(const struct voice *v,const uint8_t *actor,unsigned phase)
{
 return actor&&w(actor+0x564)==0x214&&h(actor+0x2f4)==0x4e00&&
    actor[0x2f6]==phase&&!actor[0x2f7]&&v->part_index==-1;
}

static int r103_hold_death_player_voice(const struct voice *v)
{
 const gh_actor_model *model=v->model;
 if(!model||!model->work||model->work->room!=0x103||v->part_index!=-1||
    !model->root.bytes||model->root.size<0x54c||model->root.original_vtable214!=0x41d220)return 0;
 const uint8_t *actor=model->root.bytes;
 return h(actor+0x2fe)==0x100&&h(actor+0x2f4)==0x0b01&&actor[0x2f6]==5&&
    !actor[0x2f7]&&!h(actor+0x54a);
}

static int r103_boss_paired_attack(const uint8_t *boss)
{
 return boss&&h(boss+0x2f4)==0x1700&&boss[0x2f6]==1&&boss[0x2f7]==1;
}

static int r103_em15_throw_in_motion(const uint8_t *actor)
{
 return actor&&w(actor+0x564)==0x215&&h(actor+0x2f4)==0x5200&&actor[0x2f6]==1&&!actor[0x2f7];
}

static const uint8_t *r103_em00_voice_actor(const struct voice *v)
{
 const uint8_t *actor=r103_voice_actor(v,24);
 if(!actor||w(actor+0x564)!=0x200)return NULL;
 const unsigned stamp=h(actor+0x640);
 return stamp==2||stamp==3?actor:NULL;
}

static const uint8_t *r103_em03_voice_actor(const struct voice *v)
{
 const uint8_t *actor=r103_voice_actor(v,24);
 if(!actor||w(actor+0x564)!=0x203)return NULL;
 const unsigned stamp=h(actor+0x640);
 return stamp==6||stamp==7?actor:NULL;
}

static const uint8_t *r103_em0a_voice_actor(const struct voice *v)
{
 const uint8_t *actor=r103_voice_actor(v,24);
 if(!actor||w(actor+0x564)!=0x20a)return NULL;
 const unsigned stamp=h(actor+0x640);
 return stamp==4||stamp==5?actor:NULL;
}

static int em0a_actor_bank(const struct voice *v,int16_t bank)
{
 if(!v->source_owner||bank<1||bank==2||bank==3)return 0;
 if(v->combat_profile)return 1;
 const gh_actor_model *model=v->model;
 if(!model||!model->root.bytes||model->root.size<0x568||w(model->root.bytes+0x564)!=0x20a)
  return 0;
 const uint8_t *actor=model->root.bytes;
 if(h(actor+0x2fe)==0x202)return 1;
 return r103_em0a_voice_actor(v)!=NULL;
}

static int em0a_impact_profile(const struct voice *v,int16_t bank,int16_t program,const uint8_t *p)
{
 if(!em0a_actor_bank(v,bank)||v->requested_program!=0x2a||(program!=0x2a&&program!=0x2b))return 0;
 int alternate=program==0x2b;
 return h(p+4)==(unsigned)(0x11+alternate)&&w(p+6)==0x00500406&&
        h(p+0xe)==0x21&&w(p+0x10)==0x006e006e&&w(p+0x14)==0x005a0040&&
        h(p+0x18)==(alternate?0xfe00:0xff80)&&h(p+0x1a)==0x80&&
        w(p+0x1c)==0x00ff0000&&w(p+0x20)==(alternate?5u:0x105u)&&w(p+0x24)==0x300;
}

static int r102_east_em27_voice(const struct voice *v,uint16_t state,unsigned phase);

/* Original 274750 attached recovery cues use the EM50 sound bank. */
static int r104_em03_recovery_voice(const struct voice *v,int16_t bank,int16_t program,
 const uint8_t *row,const uint8_t *sample)
{
 const gh_actor_model *model=v->model;
 if(!v->source_owner||bank<1||bank==2||bank==3||v->requested_bank!=bank||
    v->part_index!=-1||!model||!model->work||
    model->work->room!=0x104||!model->root.bytes||model->root.size<0x1652||
    model->root.original_vtable214!=0x4479c8)return 0;
 const uint8_t *actor=model->root.bytes;
 if(h(actor+0x2fe)!=0x203||w(actor+0x564)!=0x250||h(actor+0x640)||
    w(actor+0x1644)!=0x80000||actor[0x1650]||actor[0x1651])return 0;
 if(h(row)!=(uint16_t)bank||h(row+2)!=(uint16_t)program||w(row+12)!=0x00200000u||
    w(row+20)!=0x005a0040u||w(row+40)!=0xffffu||w(row+44))return 0;
 /* EM03 entries 1A30/1A38/1A40 also contain ordinary bank-1 SEQ sounds.
  * Program 23 selects its original next-row variant inside the SES player. */
 const int direct_cue=program>=0x1e&&program<=0x20&&v->requested_program==program;
 const int variant_cue=v->requested_program==0x23&&(program==0x23||program==0x24);
 if(direct_cue||variant_cue){
  static const uint32_t offsets[]={0x51790,0x52a90,0x546a0,0x46270,0x47730};
  const unsigned i=direct_cue?(unsigned)(program-0x1e):3u+(program==0x24);
  const uint32_t header=(direct_cue?0x021f0000u:0x02200000u)+(0x17u+i);
  return w(row+4)==header&&w(row+8)==0x246&&
   w(row+16)==(direct_cue?0x005f005fu:0x00550055u)&&w(row+24)==0x0080ff00u&&!w(row+28)&&
   w(row+32)==(program==0x23?0x00010105u:0x00010005u)&&w(row+36)==0x300u&&
   (!sample||(w(sample)==offsets[i]&&w(sample+4)==16000&&
    w(sample+8)==0x5fcb80ffu&&!w(sample+12)));
 }
 if(v->requested_program!=program)return 0;
 /* These two original 00/23 clips play separate em50 launch sounds. */
 if(program==0x32||program==0x35){
  const int alternate=program==0x32;
  return bank==1&&w(row+4)==(alternate?0x02230021u:0x03240024u)&&
   w(row+8)==(alternate?0x246:0x250)&&w(row+16)==0x007d007du&&
   w(row+24)==(alternate?0x01000100u:0x0100ff00u)&&!w(row+28)&&
   w(row+32)==0x00010005u&&w(row+36)==0x300u&&
   (!sample||(w(sample)==(alternate?0x3f2c0u:0x48ef0u)&&w(sample+4)==16000&&
    w(sample+8)==0x5fcb80ffu&&!w(sample+12)));
 }
 static const struct {int16_t program,slot;uint32_t offset;} cues[]={
  {0x0a,0x0a,0x21c60},{0x0b,0x0b,0x23f50},{0x0e,0x0d,0x27290},
  {0x0f,0x0e,0x2a6f0},{0x10,0x0f,0x2e220},
 };
 for(unsigned i=0;i<sizeof(cues)/sizeof(cues[0]);++i){
  if(program!=cues[i].program)continue;
  return w(row+4)==(0x022b0000u|(unsigned)cues[i].slot)&&w(row+8)==0x255&&
   w(row+16)==0x007d007du&&
   w(row+24)==0x0064ff38u&&w(row+28)==0x00080000u&&w(row+32)==0x00010085u&&
   w(row+36)==0x400u&&
   (!sample||(w(sample)==cues[i].offset&&w(sample+4)==11025&&
    w(sample+8)==0x5fcb80ffu&&!w(sample+12)));
 }
 return 0;
}

static int enemy_reaction_profile(const struct voice *v,int16_t bank,int16_t program,
 const uint8_t *p,const uint8_t *sample)
{
 if(r104_em03_recovery_voice(v,bank,program,p,sample))return 1;
 /* Original program 0 and its MT-selected variations in R100 enemy banks.
  * A detached pending voice still has the verified descriptor and sample. */
 static const struct {
  uint32_t dependency,pitch_range;
  uint32_t offsets[3];
  uint8_t gains[3],variations;
 } profiles[]={
  {0x200,0x00c8ff38,{0x5530,0x5b00,0x6350},{127,120,127},1},
  {0x202,0x00c8ff38,{0x5220,0x5970,0x6050},{125,125,125},1},
  {0x203,0x00c8ff38,{0x8bf0,0x9230,0x98f0},{110,125,125},1},
  {0x20a,0,{0xdca0,0xe580,0xf040},{125,120,120},2},
  {0x214,0x00c8ff38,{0x12f90,0x13750,0x14070},{127,127,127},2},
  {0x227,0x00c8ff38,{0xbe80,0xc560,0xccc0},{127,125,125},1},
 };
 if(!v->source_owner||bank<1||bank==2||bank==3||v->requested_program!=0||program<0||program>2||
    h(p+4)!=(unsigned)program||w(p+6)!=0x02550228||h(p+0xe)!=0x20||
    w(p+0x14)!=0x005a0040||w(p+0x1c)!=0x00010000||w(p+0x24)!=0x400)return 0;
 uint32_t dependency=0;
 if(v->model){
  const gh_title_player_view *root=&v->model->root;
  if(!root->bytes||root->size<0x568)return 0;
  const uint16_t state=h(root->bytes+0x2f4);
  const uint8_t phase=root->bytes[0x2f6];
  const int arena_em03=r103_em03_voice_actor(v)&&
      (v->combat_profile||(state==0x4e00&&phase==7)||
       ((state==0x0001||state==0x0101||state==0x0701)&&phase==0));
  const int arena_em0a=r103_em0a_voice_actor(v)&&
      (v->combat_profile||(state==0x7500&&phase==3)||(state==0x4c00&&phase==5)||
       ((state==0x0101||state==0x0701)&&!phase));
  const int arena_boss=r103_boss_voice_actor(v)&&v->part_index==-1&&
      (v->combat_profile||((state==0x0001||state==0x0101)&&!phase&&!root->bytes[0x2f7])||
       (state==0x0101&&phase==2&&!root->bytes[0x2f7])||
       (state==0x0701&&!phase&&!h(root->bytes+0x54a))||
       (state==0x1101&&phase<=1&&!root->bytes[0x2f7]&&root->size>=0x17b8&&
        w(root->bytes+0x564)==0x214&&w(root->bytes+0x17b4)==10)||
       r103_em14_hold_voice(v,root->bytes,7));
  if(h(root->bytes+0x2fe)!=0x202&&!r103_em00_voice_actor(v)&&!arena_em03&&!arena_em0a&&!arena_boss)return 0;
  dependency=arena_boss?0x214:w(root->bytes+0x564);
  if(!dependency)return 0;
  const int east_reaction=r102_east_em27_voice(v,1,0)||r102_east_em27_voice(v,0x1101,0)||
      (v->combat_profile&&(r102_east_em27_voice(v,1,1)||r102_east_em27_voice(v,0x1101,1)));
  if(dependency==0x227&&!east_reaction&&
     (!v->model->work||v->model->work->room!=0x101||v->part_index!=-1||
      v->model->root.size<0x642||h(v->model->root.bytes+0x640)!=9))return 0;
 }else if(!v->combat_profile)return 0;
 for(unsigned i=0;i<sizeof(profiles)/sizeof(*profiles);++i){
  if(dependency&&dependency!=profiles[i].dependency)continue;
  const unsigned gain=profiles[i].gains[program];
  const unsigned flags=program?0x10045u:0x10045u|((unsigned)profiles[i].variations<<8);
  if(w(p+0x10)!=gain*0x10001u||w(p+0x18)!=profiles[i].pitch_range||w(p+0x20)!=flags)continue;
  if(!sample||(w(sample)==profiles[i].offsets[program]&&w(sample+4)==11025))return 1;
 }
 return 0;
}
/* Original state71 timed program 11 exists for EM0A. The other R100
 * 0202 dependencies retain their original disabled-program path. */
static int em0a_timed_profile(const struct voice *v,int16_t bank,int16_t program,const uint8_t *p)
{
 return em0a_actor_bank(v,bank)&&v->requested_program==0x11&&program==0x11&&
        h(p+4)==12&&w(p+6)==0x023c0223&&h(p+0xe)==0x20&&
        w(p+0x10)==0x00500050&&w(p+0x14)==0x005a0040&&
        w(p+0x18)==0x0080ff80&&w(p+0x1c)==0x00070000&&
        w(p+0x20)==0x00050005&&w(p+0x24)==0x300;
}

/* EM02 selector71 SEQ frame50 selects logical bank1 program28.
 * Each admitted actor dependency supplies its own authenticated SES sample. */
static int enemy_state71_sequence_profile(const struct voice *v,int16_t bank,int16_t program,
 const uint8_t *p,const uint8_t *sample)
{
 if(!v->source_owner||bank<1||bank==2||bank==3||v->requested_program!=0x28||program!=0x28||
    !v->model||v->model->root.size<0x568||v->model->root.original_vtable214!=0x4479c8||
    h(v->model->root.bytes+0x2fe)!=0x202||h(p)!=(unsigned)bank||h(p+2)!=0x28||
    w(p+6)!=0x02500028||h(p+0xe)!=0x20||w(p+0x14)!=0x005a0040||
    w(p+0x18)!=0x00c8ff38||w(p+0x24)!=0x401||
    w(p+0x28)!=0xffff||w(p+0x2c))return 0;
 unsigned slot, gain, age, curve_preset=3;
 uint32_t offset;
 switch(w(v->model->root.bytes+0x564)){
 case 0x200:slot=15;gain=127;age=1;offset=13360;break;
 case 0x202:slot=16;gain=125;age=3;offset=15232;break;
 case 0x203:slot=17;gain=125;age=3;offset=23392;break;
 case 0x227:slot=16;gain=127;age=1;offset=0x97a0;curve_preset=1;break;
 default:return 0;
 }
 if(h(p+4)!=slot||w(p+0x10)!=gain*0x10001u||
    w(p+0x1c)!=(curve_preset<<16)||w(p+0x20)!=(age<<16|0x85u))return 0;
 return !sample||(w(sample)==offset&&w(sample+4)==11025&&w(sample+8)==0x5fcb80ff&&!w(sample+12));
}

static int r102_east_em27_voice(const struct voice *v,uint16_t state,unsigned phase)
{
 const gh_actor_model *model=v->model;
 if(!model||!model->work||model->work->room!=0x102||v->part_index!=-1)return 0;
 const gh_title_player_view *root=&model->root;
 const uint8_t *actor=root->bytes;
 return actor&&root->size>0x1650&&root->original_vtable214==0x4479c8&&
    h(actor+0x2fe)==0x202&&w(actor+0x564)==0x227&&h(actor+0x640)==4&&!actor[0x1650]&&
    h(actor+0x2f4)==state&&actor[0x2f6]==phase&&!actor[0x2f7];
}

static int r101_key_carrier_attack_voice(const struct voice *v,uint16_t state)
{
 const gh_actor_model *model=v->model;
 if(!model||!model->work||model->work->room!=0x101||v->part_index!=-1)return 0;
 const gh_title_player_view *root=&model->root;
 const uint8_t *actor=root->bytes;
 return actor&&root->size>0x1650&&root->original_vtable214==0x4479c8&&
    h(actor+0x2fe)==0x202&&w(actor+0x564)==0x227&&h(actor+0x640)==9&&actor[0x1650]==4&&
    h(actor+0x2f4)==state&&actor[0x2f6]==1&&!actor[0x2f7];
}

static int r105_em27_voice_source(const struct voice *v)
{
 if(!v->model||!v->model->work||v->model->work->room!=0x105||v->part_index!=-1)return 0;
 const gh_title_player_view *root=&v->model->root;
 return root->bytes&&root->size>=0x642&&root->original_vtable214==0x4479c8&&
    h(root->bytes+0x2fe)==0x202&&w(root->bytes+0x564)==0x227&&h(root->bytes+0x640)==2;
}

static const uint8_t *r104_em03_voice_actor(const struct voice *v)
{
 const gh_actor_model *m=v->model;
 if(!v->source_owner||!m||!m->work||m->work->room!=0x104||
    !m->root.bytes||m->root.size<=0x1651||m->root.original_vtable214!=0x4479c8)return NULL;
 const uint8_t *p=m->root.bytes;
 return h(p+0x2fe)==0x203&&w(p+0x564)==0x250&&w(p+0x1644)==0x80000&&
    !h(p+0x640)&&!p[0x1650]&&!p[0x1651]?p:NULL;
}
static int r103_heavy_recoil_source(const struct voice *v);

/* EM00/EM02/EM03/EM27 attack SEQs use local programs 4 and 6. EM27's
 * unarmed selector16 also requests the fixed program 5 directly. */
static int enemy_attack_sequence_profile(const struct voice *v,int16_t bank,int16_t program,
 const uint8_t *p,const uint8_t *sample)
{
 const int16_t request=v->requested_program;
 if(bank==1&&v->requested_bank==1&&request==0x28&&(program==0x28||program==0x29)&&
    v->source_owner&&v->part_index==-1&&v->model&&v->model->work&&v->model->work->room==0x104&&
    v->model->root.bytes&&v->model->root.size>=0x300&&
    v->model->root.original_vtable214==0x41d220&&h(v->model->root.bytes+0x2fe)==0x100){
  /* 109928 plays the resolved EM50 cue at the struck player's position. */
  const uint32_t alternate=(uint32_t)(program-0x28);
  return w(p)==(((uint32_t)program<<16)|1u)&&w(p+4)==0x0321001cu+alternate&&
    w(p+8)==0x50&&w(p+12)==0x00210000u&&w(p+16)==0x007d007du&&w(p+20)==0x005a0040u&&
    w(p+24)==0x0080ff00u&&w(p+28)==0x00ff0000u&&w(p+32)==(alternate?0x00010005u:0x00010105u)&&
    w(p+36)==0x300u&&w(p+40)==0xffffu&&!w(p+44)&&
    (!sample||(w(sample)==(alternate?0x3c820u:0x39f80u)&&w(sample+4)==18000u&&
               w(sample+8)==0x5fcb80ffu&&!w(sample+12)));
 }
 const uint8_t *boss=r103_boss_voice_actor(v);
 const unsigned boss_state=boss?h(boss+0x2f4):0;
 if(request==0x0c&&program==0x0c&&bank>=1&&bank!=2&&bank!=3&&v->requested_bank==bank){
  return v->source_owner&&(r103_heavy_recoil_source(v)||(!v->model&&v->combat_profile))&&
    w(p)==(0x000c0000u|(uint16_t)bank)&&w(p+4)==0x012e000bu&&w(p+8)==0x255u&&
    w(p+12)==0x00200000u&&w(p+16)==0x007d007du&&w(p+20)==0x005a0040u&&
    w(p+24)==0x0064ff9cu&&w(p+28)==0x00020000u&&w(p+32)==0x00010045u&&
    w(p+36)==0x400u&&w(p+40)==0xffffu&&!w(p+44)&&
    (!sample||(w(sample)==0x194b0u&&w(sample+4)==11025&&w(sample+8)==0x5fcb80ffu&&!w(sample+12)));
 }
 if(boss&&v->source_owner&&!boss[0x2f7]&&
    ((boss_state==0x6c00&&boss[0x2f6]==1)||
     (boss_state==0x3b00&&(boss[0x2f6]==3||boss[0x2f6]==5)))&&
    v->part_index==-1&&bank>=1&&bank!=2&&bank!=3&&v->requested_bank==bank&&request==0x21&&program==0x21){
  return w(p)==(0x00210000u|(uint16_t)bank)&&w(p+4)==0x04230018u&&w(p+8)==0x264u&&
    w(p+12)==0x00210000u&&w(p+16)==0x00320032u&&w(p+20)==0x005a0040u&&!w(p+24)&&
    w(p+28)==0x00ff0000u&&w(p+32)==0x00010005u&&!w(p+36)&&w(p+40)==0xffffu&&!w(p+44)&&
    (!sample||(w(sample)==0xb0b0u&&w(sample+4)==14000&&w(sample+8)==0x5fcb80ffu&&!w(sample+12)));
 }
 const uint8_t *charge=r104_em03_voice_actor(v);
 /* Deferred PCM setup can observe the newly opened reaction at phase 3. */
 const uint16_t em03_state=charge?h(charge+0x2f4):0;
 const unsigned em03_phase=charge?charge[0x2f6]:0;
 const int em03_hit=(em03_state==0x0101||em03_state==0x0001)&&em03_phase<=3;
 const int em03_grab=em03_state==0x4b00&&em03_phase==1;
 const int em03_turn=em03_state==0x0800&&em03_phase==1&&charge[0x2f7]<=1;
 const int em03_selector21=em03_state==0x2100&&em03_phase==1;
 const int em03_melee=em03_state==0x1500&&em03_phase==1;
 const int em03_rear=em03_state==0x0a00&&em03_phase==1&&charge[0x2f7]==1;
 const int em03_approach=em03_state==0x0500&&em03_phase==1;
 if(charge&&(em03_turn||((em03_hit||em03_grab||em03_selector21||em03_melee)&&!charge[0x2f7]))&&
    !h(charge+0x424)&&v->part_index==-1&&v->requested_bank==1&&bank==1){
  if(request==35&&(program==35||program==36)){
   const unsigned variation=(unsigned)(program-35);
   return w(p)==(((uint32_t)program<<16)|1u)&&w(p+4)==0x0220001au+variation&&w(p+8)==0x246u&&
      w(p+12)==0x00200000u&&w(p+16)==0x00550055u&&w(p+20)==0x005a0040u&&
      w(p+24)==0x0080ff00u&&!w(p+28)&&w(p+32)==(variation?0x00010005u:0x00010105u)&&
      w(p+36)==0x300u&&w(p+40)==0xffffu&&!w(p+44)&&
      (!sample||(w(sample)==(variation?0x47730u:0x46270u)&&w(sample+4)==16000u&&
                 w(sample+8)==0x5fcb80ffu&&!w(sample+12)));
  }
  if(em03_melee&&request==4&&program>=4&&program<=6){
   const unsigned alternate=(unsigned)(program-4);
   static const uint32_t samples[]={0x13b40u,0x143e0u,0x14e40u};
   return w(p)==(((uint32_t)program<<16)|1u)&&w(p+4)==0x022a0004u+alternate&&
      w(p+8)==0x250u&&w(p+12)==0x00200000u&&
      w(p+16)==(alternate==2?0x007a007au:0x007f007fu)&&w(p+20)==0x005a0040u&&
      w(p+24)==0x00c8ff38u&&w(p+28)==0x00060000u&&w(p+32)==(alternate?0x10085u:0x10285u)&&
      w(p+36)==0x400u&&w(p+40)==0xffffu&&!w(p+44)&&
      (!sample||(w(sample)==samples[alternate]&&w(sample+4)==11025u&&
                 w(sample+8)==0x5fcb80ffu&&!w(sample+12)));
  }
  if(em03_melee&&(program==30||program==31)&&request==program){
   const unsigned alternate=(unsigned)(program-30);
   return w(p)==(((uint32_t)program<<16)|1u)&&w(p+4)==0x021f0017u+alternate&&w(p+8)==0x246u&&
      w(p+12)==0x00200000u&&w(p+16)==0x005f005fu&&w(p+20)==0x005a0040u&&
      w(p+24)==0x0080ff00u&&!w(p+28)&&w(p+32)==0x00010005u&&w(p+36)==0x300u&&
      w(p+40)==0xffffu&&!w(p+44)&&
      (!sample||(w(sample)==(alternate?0x52a90u:0x51790u)&&w(sample+4)==16000u&&
                 w(sample+8)==0x5fcb80ffu&&!w(sample+12)));
  }
  if(em03_selector21&&request==55&&program==55){
   /* em50.ses row 55 is called by SEQ 2DBBC0 at frame 13. */
   return w(p)==0x00370001u&&w(p+4)==0x03250025u&&w(p+8)==0x250u&&
      w(p+12)==0x00200000u&&w(p+16)==0x00410041u&&w(p+20)==0x005a0040u&&
      w(p+24)==0x0100ff00u&&!w(p+28)&&w(p+32)==0x00010005u&&w(p+36)==0x300u&&
      w(p+40)==0xffffu&&!w(p+44)&&
      (!sample||(w(sample)==0x4bfc0u&&w(sample+4)==16000u&&
                w(sample+8)==0x5fcb80ffu&&!w(sample+12)));
  }
  if(em03_hit&&request==0&&program>=0&&program<=2){
   const unsigned variation=(unsigned)program;
   static const uint32_t samples[]={0x17350u,0x17cb0u,0x18430u};
   return w(p)==((variation<<16)|1u)&&w(p+4)==0x02280000u+variation&&
      w(p+8)==0x255u&&w(p+12)==0x00200000u&&
      w(p+16)==(variation?0x007f007fu:0x00780078u)&&w(p+20)==0x005a0040u&&
      w(p+24)==0x00c8ff38u&&w(p+28)==0x00010000u&&
      w(p+32)==(variation?0x00010045u:0x00010245u)&&w(p+36)==0x400u&&
      w(p+40)==0xffffu&&!w(p+44)&&
      (!sample||(w(sample)==samples[variation]&&w(sample+4)==11025u&&
                 w(sample+8)==0x5fcb80ffu&&!w(sample+12)));
  }
 }
 const int approach_voice=charge&&em03_state==0x0b00&&charge[0x2f7]<=1;
 if(charge&&em03_state==0x1600&&em03_phase==1&&!charge[0x2f7]&&v->part_index==-1&&
    v->requested_bank==1&&bank==1&&request==4&&program>=4&&program<=6){
  static const uint32_t offsets[]={0x13b40,0x143e0,0x14e40};
  return w(p)==(((uint32_t)program<<16)|1u)&&w(p+4)==0x022a0000u+(unsigned)program&&
    w(p+8)==0x250&&w(p+12)==0x00200000u&&w(p+16)==(program==6?0x007a007au:0x007f007fu)&&
    w(p+20)==0x005a0040u&&w(p+24)==0x00c8ff38u&&w(p+28)==0x00060000u&&
    w(p+32)==(program==4?0x00010285u:0x00010085u)&&w(p+36)==0x400u&&w(p+40)==0xffffu&&!w(p+44)&&
    (!sample||(w(sample)==offsets[program-4]&&w(sample+4)==11025u&&w(sample+8)==0x5fcb80ffu&&!w(sample+12)));
 }
 if(charge&&(!charge[0x2f7]||approach_voice||em03_turn||em03_rear)&&v->part_index==-1&&v->requested_bank==1&&
    bank==1&&request==program){
  const uint16_t state=h(charge+0x2f4);
  const unsigned phase=charge[0x2f6];
  /* Program17 is explicit in the charge and is also dep0250's normalized AA footstep. */
  if(program==17&&((state==0x1a00&&phase>=3&&phase<=5)||
                   ((state==0x0600||state==0x2300)&&phase==1)||
                   (approach_voice&&phase==5)||
                   (em03_hit&&(phase==1||phase==3))||em03_grab||em03_turn||em03_melee||em03_rear||em03_approach)){
   /* Original em50.ses row requested by the active charge SEQ. */
   return w(p)==0x00110001u&&w(p+4)==0x021e0010u&&w(p+8)==0x23c&&
     w(p+12)==0x00200000u&&w(p+16)==0x005a005au&&w(p+20)==0x005a0040u&&
     w(p+24)==0x0080ff80u&&w(p+28)==0x00070000u&&w(p+32)==0x00050005u&&
     w(p+36)==0x300u&&w(p+40)==0xffffu&&!w(p+44)&&
     (!sample||(w(sample)==0x50b40u&&w(sample+4)==14000u&&
               w(sample+8)==0x5fcb80ffu&&!w(sample+12)));
  }
  if(program==6&&(((state==0x1a00||state==0x2300||approach_voice)&&phase==1)||em03_grab)){
   /* Original charge, ranged windup and approach each request local program 6. */
   return w(p)==0x00060001u&&w(p+4)==0x022a0006u&&w(p+8)==0x250&&
     w(p+12)==0x00200000u&&w(p+16)==0x007a007au&&w(p+20)==0x005a0040u&&
     w(p+24)==0x00c8ff38u&&w(p+28)==0x00060000u&&w(p+32)==0x00010085u&&
     w(p+36)==0x400u&&w(p+40)==0xffffu&&!w(p+44)&&
     (!sample||(w(sample)==0x14e40u&&w(sample+4)==11025u&&
               w(sample+8)==0x5fcb80ffu&&!w(sample+12)));
  }
 }
 const int direct_five=request==5&&program==5;
 if(!v->source_owner||bank<1||bank==2||bank==3||(request!=4&&request!=6&&!direct_five)||
    program<request||program>request+1||h(p)!=(unsigned)bank||h(p+2)!=(unsigned)program)
  return 0;
 const unsigned gain=h(p+0x10);
 const unsigned index=(unsigned)(program-4);
 const gh_title_player_view *root=v->model?&v->model->root:NULL;
 const int detached=v->combat_profile&&!root&&sample;
 static const uint32_t em02_offsets[]={0x2c00,0x32a0,0x3b80};
 static const uint32_t em27_offsets[]={0x87d0,0x9010,0x97a0,0xaa40};
 const int em02=program<=6&&gain==(program==4?120u:125u)&&
    ((root&&root->bytes&&root->size>=0x568&&w(root->bytes+0x564)==0x202)||
     (detached&&w(sample)==em02_offsets[index]));
 const int r101_gate=root&&root->bytes&&root->size>0x1650&&v->model->work&&
    v->model->work->room==0x101&&v->part_index==-1&&w(root->bytes+0x564)==0x227&&
    (h(root->bytes+0x640)==10||h(root->bytes+0x640)==11)&&root->bytes[0x1650]==4&&
    h(root->bytes+0x2f4)==0x1600&&root->bytes[0x2f6]==1&&!root->bytes[0x2f7];
 const int r101_stamp9=(request==4&&r101_key_carrier_attack_voice(v,0x1500))||
    (request==6&&r101_key_carrier_attack_voice(v,0x4b00));
 const int em27=gain==127&&(r101_stamp9||r101_gate||r105_em27_voice_source(v)||r102_east_em27_voice(v,0x1600,1)||
    (request==4&&r102_east_em27_voice(v,0x1500,1))||
    (detached&&w(sample)==em27_offsets[index]));
 const uint32_t dependency=em02?0x202:em27?0x227:gain==127?0x200:gain==125?0x203:0;
 if(!dependency||(direct_five&&!em27))return 0;
 if(root){
  if(!root->bytes||root->size<0x568||root->original_vtable214!=0x4479c8||
     w(root->bytes+0x564)!=dependency)return 0;
  const uint16_t state=h(root->bytes+0x2f4);
  const int arena_em03=request==4&&r103_em03_voice_actor(v)&&(state==0x4e00||state==0x1500);
  if(h(root->bytes+0x2fe)!=0x202&&!r103_em00_voice_actor(v)&&!arena_em03)return 0;
 }else if(!v->combat_profile)return 0;
 const int random_variant=program==request&&!direct_five&&(!em02||program==4);
 const unsigned variants=random_variant?0x10185u:0x10085u;
 if(h(p+4)!=(unsigned)(program-1)||w(p+6)!=0x0250022au||h(p+0xe)!=0x20||
    h(p+0x12)!=gain||w(p+0x14)!=0x005a0040||w(p+0x18)!=0x00c8ff38u||
    w(p+0x1c)!=0x00060000u||w(p+0x20)!=variants||w(p+0x24)!=0x400||
    w(p+0x28)!=0xffff||w(p+0x2c))return 0;
 static const uint32_t offsets[][4]={{8928,11408,13360,16624},{0x4a20,0x5280,0x5b60,0x7700}};
 const uint32_t offset=em02?em02_offsets[index]:em27?em27_offsets[index]:
    offsets[dependency==0x203][index];
 return !sample||(w(sample)==offset&&w(sample+4)==11025&&
    w(sample+8)==0x5fcb80ffu&&!w(sample+12));
}

/* The original paired attack's EM04 SEQ uses the authored EM14.SES bank. */
static int r103_paired_attack_voice_profile(const struct voice *v,int16_t bank,int16_t program,
 const uint8_t *p,const uint8_t *sample)
{
 if(!v->source_owner||bank<1||bank==2||bank==3||v->requested_bank!=bank||v->requested_program!=program||
    !r103_boss_paired_attack(r103_boss_voice_actor(v)))return 0;
 if(w(p)!=(((uint32_t)program<<16)|(uint16_t)bank)||w(p+20)!=0x005a0040u||
    w(p+40)!=0xffffu||w(p+44))return 0;
 if(program==0x1a){
  return w(p+4)==0x022b0014u&&w(p+8)==0x255u&&w(p+12)==0x00200000u&&
    w(p+16)==0x00780078u&&w(p+24)==0x0064ff38u&&w(p+28)==0x00080000u&&
    w(p+32)==0x00010085u&&w(p+36)==0x400u&&(!sample||
     (w(sample)==0x1b2c0u&&w(sample+4)==11025&&w(sample+8)==0x5fcb80ffu&&!w(sample+12)));
 }
 static const struct {uint32_t row,gain,pitch,sample,rate;} original[]={
  {0x041e0015u,0x007d007du,0x0080ff80u,0,16000},
  {0x041e0016u,0x007d007du,0x02bc02bcu,0x4100u,16000},
  {0x04200017u,0x007d007du,0x0100ff00u,0x8370u,16000},
  {0x04230018u,0x00320032u,0,0xb0b0u,14000}
 };
 if(program<0x1e||program>0x21)return 0;
 const unsigned i=program-0x1e;
 return w(p+4)==original[i].row&&w(p+8)==0x264&&w(p+12)==0x00210000u&&
   w(p+16)==original[i].gain&&w(p+24)==original[i].pitch&&w(p+28)==0x00ff0000u&&
   w(p+32)==0x00010005u&&!w(p+36)&&(!sample||
     (w(sample)==original[i].sample&&w(sample+4)==original[i].rate&&
      w(sample+8)==0x5fcb80ffu&&!w(sample+12)));
}

/* Resolve the live dependency bank for recovery cues and SEQ program 10. */
static int enemy_local_cue_profile(const struct voice *v,int16_t bank,int16_t program,
 const uint8_t *p,const uint8_t *sample)
{
 /* Both original West capture clips request their dependency-specific 1E row. */
 if(program==0x1e){
  const gh_actor_model *model=v->model;
  if(!v->source_owner||v->requested_program!=0x1e||v->part_index!=-1||
     !model||!model->work||model->work->room!=0x100||!model->root.bytes||model->root.size<0x16c8||
     model->root.original_vtable214!=0x4479c8)return 0;
  const uint8_t *a=model->root.bytes;
  const unsigned stamp=h(a+0x640),appearance=a[0x1650];
  const uint32_t mode=w(a+0x16c4),dependency=w(a+0x564);
  if(h(a+0x2fe)!=0x202||h(a+0x2f4)!=0x1800||a[0x2f6]!=1)return 0;
  const int em03=bank==7&&dependency==0x203&&stamp==19&&!appearance;
  const int em02=bank==9&&dependency==0x202&&((stamp==4&&appearance==2&&mode==6)||
       (!appearance&&((stamp==29&&mode==2)||(stamp==32&&!mode))));
  if(!em03&&!em02)return 0;
  return w(p)==(0x001e0000u|(unsigned)bank)&&w(p+4)==(em03?0x04230010u:0x0221000fu)&&
      w(p+8)==0x250u&&w(p+12)==0x00210000u&&
      w(p+16)==0x00500050u&&w(p+20)==0x005a0040u&&w(p+24)==0x00800000u&&
      w(p+28)==0x00ff0000u&&w(p+32)==5&&w(p+36)==0x300u&&w(p+40)==0xffffu&&!w(p+44)&&
      (!sample||(w(sample)==(em03?0u:0x14560u)&&w(sample+4)==14000&&w(sample+8)==0x5fcb80ffu&&!w(sample+12)));
 }
 if(!v->source_owner||bank<1||bank==2||bank==3||
    (program!=0x0a&&program!=0x0b&&program!=0x10)||v->requested_program!=program||
    h(p)!=(unsigned)bank||h(p+2)!=(unsigned)program)return 0;
 uint32_t dependency=0;
 if(v->model){
  const gh_title_player_view *root=&v->model->root;
  if(!root->bytes||root->size<0x568||root->original_vtable214!=0x4479c8)return 0;
  dependency=w(root->bytes+0x564);
  const uint8_t *boss=r103_boss_voice_actor(v);
  const unsigned boss_state=boss?h(boss+0x2f4):0;
  const int throw_partner=boss&&dependency==0x214&&boss_state==0x5200&&
     boss[0x2f6]==3&&!boss[0x2f7];
  const int recovery=boss&&boss_state==0x6c00&&boss[0x2f6]==1&&!boss[0x2f7];
  const int pincer=boss&&(boss_state==0x3b00||boss_state==0x3c00)&&boss[0x2f6]<=1&&!boss[0x2f7];
  if((recovery||throw_partner||pincer||(program==0xb&&r103_em14_hold_voice(v,boss,5)))&&
     v->part_index==-1&&v->requested_bank==bank&&program!=0x10){
   /* Original EM14.SES exertion cues for recovery and paired moves. */
   const unsigned alternate=program==0x0b;
   return w(p+4)==0x022b0009u+alternate&&w(p+8)==0x255u&&w(p+12)==0x00200000u&&
      w(p+16)==0x00780078u&&w(p+20)==0x005a0040u&&w(p+24)==0x0064ff38u&&
      w(p+28)==0x00080000u&&w(p+32)==0x00010085u&&w(p+36)==0x400u&&
      w(p+40)==0xffffu&&!w(p+44)&&(!sample||(w(sample)==(alternate?0x1f450u:0x1d090u)&&
         w(sample+4)==11025u&&w(sample+8)==0x5fcb80ffu&&!w(sample+12)));
  }
  const int arena_em03=r103_em03_voice_actor(v)&&h(root->bytes+0x2f4)==0x6c00&&
      root->bytes[0x2f6]==1;
  if(h(root->bytes+0x2fe)!=0x202&&!r103_em00_voice_actor(v)&&!arena_em03)return 0;
  if(dependency==0x20a){
   if(program==0x10||v->part_index!=-1||!v->model->work||root->size<0x16c8||
      root->bytes[0x2f6]!=1)return 0;
   const uint8_t *actor=root->bytes;
   const unsigned state=h(actor+0x2f4);
   const int16_t room=v->model->work->room;
   const int gate=room==0x102&&h(actor+0x640)==2&&actor[0x1650]==5&&
      (state==0x6b00||state==0x6c00)&&!w(actor+0x16c4);
   const int west=room==0x100&&!h(actor+0x640)&&actor[0x1650]==3&&state==0x6c00&&
      !actor[0x2f7]&&w(actor+0x16c4)==9;
   if(!gate&&!west)return 0;
   /* 2744E0's EM0A entry takes the common 0A/0B cues from EM0A.SES. */
   const unsigned alternate=program==0x0b;
   return h(p+4)==8u+alternate&&w(p+6)==0x0255022bu&&w(p+12)==0x00200000u&&
      w(p+16)==(alternate?0x00780078u:0x007d007du)&&w(p+20)==0x005a0040u&&
      !w(p+24)&&w(p+28)==0x00030000u&&w(p+32)==0x00010085u&&
      w(p+36)==0x400&&!h(p+42)&&h(p+40)==0xffffu&&!w(p+44)&&
      (!sample||(w(sample)==(alternate?0x1bfd0u:0x19130u)&&w(sample+4)==11025&&
                w(sample+8)==0x5fcb80ffu&&!w(sample+12)));
  }
  if(dependency==0x227){
   const int r102=program!=0x10&&(r102_east_em27_voice(v,0x6b00,1)||
      r102_east_em27_voice(v,0x6c00,1));
   const int r101=program!=0x10&&v->model->work&&v->model->work->room==0x101&&
       root->size>0x1650&&(h(root->bytes+0x640)==10||h(root->bytes+0x640)==11)&&
       root->bytes[0x1650]==4&&h(root->bytes+0x2f4)==0x6b00&&root->bytes[0x2f6]==1;
   if(!r105_em27_voice_source(v)&&!r102&&!r101)return 0;
  }else if(dependency!=0x200&&dependency!=0x202&&dependency!=0x203)return 0;
  if(program==0x10&&dependency!=0x227){
   if(root->size<0x642||!v->model->work||v->part_index!=-1)return 0;
   const unsigned stamp=h(root->bytes+0x640),room=v->model->work->room;
   const int r100=room==0x100&&((stamp==19&&dependency==0x203)||(stamp==20&&dependency==0x200));
   /* Armed EM02's original 3E4 motion directly requests local row 10. */
   const int r100_armed_em02=room==0x100&&dependency==0x202&&stamp==18&&
       root->size>0x1650&&root->bytes[0x1650]==9&&h(root->bytes+0x2f4)==0x6c00&&
       root->bytes[0x2f6]==1;
   /* R101's held weapon overrides the EM02 motion to SEQ2092C0,
    * which selects the existing EM03 program10 at sample7. */
   const int r101=room==0x101&&root->size>0x1650&&
       ((stamp==13&&dependency==0x203&&!root->bytes[0x1650])||
        (stamp==12&&dependency==0x200&&root->bytes[0x1650]==4&&
         h(root->bytes+0x2f4)==0x6c00&&root->bytes[0x2f6]==1&&!root->bytes[0x2f7]));
   /* Both observed R102 EM00 state 6C motions use the same SES slot 12. */
   const int r102=room==0x102&&dependency==0x200&&root->size>0x1650&&
       ((stamp==11&&root->bytes[0x1650]==5)||(stamp==3&&root->bytes[0x1650]==1));
   /* AT0B stamp 7 uses local program 10 during the original 6C move. */
   const int r102_at0b=room==0x102&&stamp==7&&dependency==0x200&&root->size>=0x16c8&&
       !root->bytes[0x1650]&&w(root->bytes+0x16c4)==2&&h(root->bytes+0x2f4)==0x6c00&&
       root->bytes[0x2f6]==1;
   const int west_wave=room==0x100&&root->size>=0x16c8&&!root->bytes[0x1650]&&
       ((dependency==0x203&&stamp==30&&w(root->bytes+0x16c4)==2)||
        (dependency==0x202&&stamp==32&&!w(root->bytes+0x16c4)))&&
       h(root->bytes+0x2f4)==0x6c00&&root->bytes[0x2f6]==1;
   if(!r100&&!r100_armed_em02&&!west_wave&&!r101&&!r102&&!r102_at0b&&!r103_em00_voice_actor(v)&&!arena_em03)return 0;
  }
 }else if(!v->combat_profile||program==0x10)return 0;
 const unsigned alternate=program==0x0b;
 if(w(p+6)!=(dependency==0x227?0x0255022bu:0x0255022du)||h(p+0xe)!=0x20||
    w(p+0x14)!=0x005a0040||h(p+0x1a)!=100||
    w(p+0x1c)!=0x00030000||w(p+0x20)!=0x00010085||
    w(p+0x24)!=0x400||w(p+0x28)!=0xffff||w(p+0x2c))return 0;
 if(sample&&(w(sample+4)!=11025||w(sample+8)!=0x5fcb80ff||w(sample+12)))return 0;
 if(program==0x10){
  const uint32_t offset=dependency==0x227?0x153a0u:dependency==0x200?0x131a0u:
      dependency==0x202?0x11010u:0x12d20u;
  const unsigned slot=dependency==0x202?11:12;
  return h(p+4)==slot&&w(p+0x10)==0x007d007du&&h(p+0x18)==0xff38&&(!sample||w(sample)==offset);
 }
 if(dependency==0x227)
    return h(p+4)==9u+alternate&&w(p+0x10)==0x00780078u&&h(p+0x18)==0xff38&&
           (!sample||w(sample)==(alternate?0x14330u:0x11e80u));
 if(h(p+4)==8u+alternate&&w(p+0x10)==0x00730073u)
    return (!dependency||dependency==0x202)&&h(p+0x18)==0xff38&&
           (!sample||w(sample)==(alternate?0xe360u:0xc9f0u));
 if(h(p+4)!=9u+alternate||w(p+0x10)!=0x007d007du||dependency==0x202)return 0;
 const int em00=dependency!=0x203&&h(p+0x18)==(alternate?0xff38:0)&&
    (!sample||w(sample)==(alternate?0x11bc0u:0xff80u));
 const int em03=dependency!=0x200&&h(p+0x18)==0xff38&&
    (!sample||w(sample)==(alternate?0x10be0u:0xfc20u));
 return em00||em03;
}

/* Actual 288F00 cEma2 impact selects one of three CORE.SES recordings. */
static int core_ema2_impact_profile(const struct voice *v,int16_t bank,int16_t program,const uint8_t *p)
{
 if(!v->source_owner||bank||v->requested_program!=0x96||program<0x96||program>0x98)return 0;
 if(!v->combat_profile&&(!v->model||v->model->root.size<0x568||
    v->model->root.original_vtable214!=0x448ca0||h(v->model->root.bytes+0x2fe)!=0x2a2||
    w(v->model->root.bytes+0x564)<0x2a2||w(v->model->root.bytes+0x564)>0x2a4))return 0;
 return h(p+4)==(unsigned)(program-0x37)&&w(p+6)==0x02500406&&
        h(p+0xe)==0x20&&w(p+0x10)==0x006e006e&&w(p+0x14)==0x005a0040&&
        h(p+0x18)==(program==0x96?0xff80:0x80)&&h(p+0x1a)==0x80&&
        w(p+0x1c)==0x00070000&&w(p+0x20)==(program==0x96?0x205u:5u)&&w(p+0x24)==1;
}
/* Actual 28696C requests EMA2 program 90 and one MT-selected alternate. */
static int ema2_timer_cue_profile(const struct voice *v,int16_t bank,int16_t program,
 const uint8_t *p,const uint8_t *sample)
{
 if(!v->source_owner||bank<1||bank==2||bank==3||v->requested_program!=0x5a||
    (program!=0x5a&&program!=0x5b)||h(p)!=(unsigned)bank||h(p+2)!=(unsigned)program||
    v->part_index!=-1||!v->model)return 0;
 const gh_title_player_view *root=&v->model->root;
 if(root->size<0x568||root->original_vtable214!=0x448ca0)return 0;
 const unsigned kind=h(root->bytes+0x2fe);
 const uint32_t dependency=w(root->bytes+0x564);
 const int ema7=root->size>0x1561&&v->model->work&&v->model->work->room==0x101&&kind==0x2a3&&dependency==0x2a7&&
    (root->bytes[0x1561]==16||root->bytes[0x1561]==17);
 if(!ema7&&(kind!=0x2a2||dependency!=0x2a2))return 0;
 const int alternate=program==0x5b;
 if(h(p+4)!=(unsigned)alternate||w(p+6)!=0x0250022d||h(p+0xe)!=0x20||
    w(p+0x10)!=0x00780078||w(p+0x14)!=0x005a0040||w(p+0x18)||
    w(p+0x1c)!=0x00030000||w(p+0x20)!=((ema7?0x30000u:0)|(alternate?5u:0x105u))||
    w(p+0x24)!=0x401||w(p+0x28)!=0xffff||w(p+0x2c))return 0;
 return !sample||(w(sample)==(ema7?(alternate?0x3940u:0x2120u):(alternate?6368u:2864u))&&
    w(sample+4)==11025&&w(sample+8)==0x5fcb80ff&&!w(sample+12));
}

/* The original EMA2 walk-home and recovery cues share their bank envelope. */
static int ema2_home_cue_profile(const struct voice *v,int16_t bank,int16_t program,
 const uint8_t *p,const uint8_t *sample)
{
 const gh_actor_model *model=v->model;
 if(!v->source_owner||bank<1||bank==2||bank==3||v->requested_bank!=bank||
    v->requested_program!=program||(program!=0x5c&&program!=0x5d)||v->part_index!=-1||!model||
    !model->work||model->work->room!=0x100||!model->root.bytes||model->root.size<0x568||
    model->root.original_vtable214!=0x448ca0)return 0;
 const uint8_t *actor=model->root.bytes;
 const int recovery=program==0x5d;
 if(h(actor+0x2fe)!=0x2a2||w(actor+0x564)!=0x2a2)return 0;
 if(recovery){
  if(model->root.size<0x642||h(actor+0x640)!=6||h(actor+0x2f4)||
     actor[0x2f6]!=6||actor[0x2f7])return 0;
 }else if(h(actor+0x2f4)!=0x0400||actor[0x2f6]!=2)return 0;
 return h(p)==(unsigned)bank&&h(p+2)==(unsigned)program&&h(p+4)==(recovery?3u:2u)&&
    w(p+6)==0x0250022du&&!w(p+0xa)&&h(p+0xe)==0x20&&w(p+0x10)==(recovery?0x00640064u:0x00780078u)&&
    w(p+0x14)==0x005a0040u&&!w(p+0x18)&&w(p+0x1c)==0x00030000u&&
    w(p+0x20)==(recovery?0x85u:0x45u)&&w(p+0x24)==0x401u&&w(p+0x28)==0xffffu&&!w(p+0x2c)&&
    w(sample)==(recovery?0x27a0u:0)&&w(sample+4)==11025&&w(sample+8)==0x5fcb80ffu&&!w(sample+12);
}

/* 287090 uses the R102 room bank for EMA2's two poisoned cries.
 * The descriptor and sample slots are from the original se/r102.ses. */
static int r102_ema2_poison_profile(const struct voice *v,int16_t bank,int16_t program,
 const uint8_t *p,const uint8_t *sample)
{
 const gh_actor_model *model=v->model;
 if(!v->source_owner||bank!=2||v->requested_bank!=2||v->requested_program!=0x60||
    (program!=0x60&&program!=0x61)||v->part_index!=-1||!model||!model->work||
    model->work->room!=0x102||!model->root.bytes||model->root.size<0x642||
    model->root.original_vtable214!=0x448ca0||h(model->root.bytes+0x2fe)!=0x2a2||
    w(model->root.bytes+0x564)!=0x2a2||h(model->root.bytes+0x640)!=0x11)return 0;
 const unsigned alternate=program==0x61;
 if(h(p)!=2||h(p+2)!=(unsigned)program||w(p+4)!=(alternate?0x02190008u:0x02190007u)||
    w(p+8)!=0x250||w(p+12)!=0x00200000||w(p+16)!=0x006e006e||w(p+20)!=0x005a0040||
    w(p+24)||w(p+28)!=0x00030000||w(p+32)!=(alternate?0x84u:0x184u)||
    w(p+36)!=0x401||w(p+40)!=0xffff||w(p+44))return 0;
 return w(sample)==(alternate?0xac90u:0x80f0u)&&w(sample+4)==11025&&
    w(sample+8)==0x5fc080ff&&!w(sample+12);
}

static const uint8_t *r102_voice_actor(const struct voice *v)
{
 const gh_actor_model *model=v->model;
 if(v->part_index!=-1||!model||!model->work||model->work->room!=0x102||
    !model->root.bytes||model->root.size<=0x1650||model->root.original_vtable214!=0x4479c8)return NULL;
 const uint8_t *actor=model->root.bytes;
 return h(actor+0x2fe)==0x202?actor:NULL;
}

static int r102_east_em27_route_voice(const struct voice *v,uint16_t state,unsigned variant)
{
 const uint8_t *actor=r102_voice_actor(v);
 if(!actor||v->model->root.size<0x16c8||w(actor+0x564)!=0x227||h(actor+0x640)!=4||
    actor[0x1650]||h(actor+0x2f4)!=state||actor[0x2f6]!=1||actor[0x2f7]!=variant)return 0;
 const uint32_t mode=w(actor+0x16c4);
 return mode==6||mode==8;
}

static const uint8_t *r102_em0a_voice_actor(const struct voice *v)
{
 const uint8_t *actor=r102_voice_actor(v);
 return actor&&v->model->root.size>=0x16c8&&w(actor+0x564)==0x20a&&h(actor+0x640)==2&&
    actor[0x1650]==5&&!w(actor+0x16c4)?actor:NULL;
}

static int r102_em0f_voice_source(const struct voice *v)
{
 const uint8_t *actor=r102_voice_actor(v);
 return actor&&w(actor+0x564)==0x20f&&h(actor+0x640)==0x12&&!actor[0x1650];
}

static int r102_obstacle_voice_actor(const struct voice *v)
{
 const uint8_t *actor=r102_voice_actor(v);
 return actor&&h(actor+0x2f4)==0x7500&&!actor[0x1650]&&h(actor+0x640)==7&&w(actor+0x564)==0x200;
}

static int r102_em02_route_voice(const struct voice *v,uint16_t state)
{
 const uint8_t *actor=r102_voice_actor(v);
 return actor&&v->model->root.size>=0x16c8&&
    h(actor+0x2f4)==state&&
    !actor[0x1650]&&
    (h(actor+0x640)==0x17||((state==0x1500||state==0x4b00)&&h(actor+0x640)==0x19))&&
    w(actor+0x564)==0x202&&w(actor+0x16c4)==2;
}

static int r100_wave_em02_action_voice(const struct voice *v,uint16_t state)
{
 const gh_actor_model *model=v->model;
 const uint8_t *actor=model?model->root.bytes:NULL;
 return actor&&model->work&&model->work->room==0x100&&model->root.size>=0x16c8&&
    model->root.original_vtable214==0x4479c8&&h(actor+0x2fe)==0x202&&w(actor+0x564)==0x202&&
    !actor[0x1650]&&h(actor+0x2f4)==state&&!actor[0x2f7]&&
    ((h(actor+0x640)==29&&w(actor+0x16c4)==2)||(h(actor+0x640)==32&&!w(actor+0x16c4)));
}

static int r100_at3_em02_combo_voice(const struct voice *v)
{
 return r100_wave_em02_action_voice(v,0x1600)&&
    h(v->model->root.bytes+0x640)==32&&v->model->root.bytes[0x2f6]==1;
}

static int r102_mount_voice_actor(const struct voice *v)
{
 const uint8_t *actor=r102_voice_actor(v);
 if(!actor||h(actor+0x2f4)!=0x7500||actor[0x1650]!=6)return -1;
 const unsigned stamp=h(actor+0x640);
 if(stamp<0x20||stamp>0x22)return -1;
 static const uint16_t dependency[]={0x202,0x200,0x203};
 const int index=(int)(stamp-0x20);
 return w(actor+0x564)==dependency[index]?index:-1;
}

/* The original AA, AC and hold AE footsteps keep the CORE bank through 2C7FD0.
 * SDE 3751B8 attenuates the first level and sets both levels to that
 * result in this mode, before the 37D558 pan split. */
static int r103_pincer_flank_audio(const uint8_t *actor)
{
 return actor&&h(actor+0x2f4)==0x3c00&&actor[0x2f6]==7&&!actor[0x2f7];
}

static int r103_pincer_flank_withdrawal(const uint8_t *actor)
{
 return actor&&h(actor+0x2f4)==0x3c00&&actor[0x2f6]==17&&!actor[0x2f7];
}

static int core_enemy_footstep_profile(const struct voice *v,int16_t bank,int16_t program,
 const uint8_t *p,const uint8_t *sample)
{
 const int hold_step=v->requested_program==0xae&&program==0xae;
 const int ground_step=v->requested_program==0xaa&&(program==0xaa||program==0xab);
 const int hard_step=v->requested_program==0xac&&(program==0xac||program==0xad);
 const int other_surface=v->requested_program==0xb6&&(program==0xb6||program==0xb7);
 const int r101_step=other_surface||(v->requested_program==0xb0&&(program==0xb0||program==0xb1));
 const int gate_death_step=v->requested_program==0xb4&&program==0xb4;
 if(!v->source_owner||bank||
    (!hold_step&&!ground_step&&!hard_step&&!r101_step&&!gate_death_step))
  return 0;
 if(!v->model||!v->model->root.bytes||v->model->root.size<0x568)return 0;
 const uint8_t *actor=v->model->root.bytes;
 uint32_t dependency=w(actor+0x564);
 /* The third home-walk phase also uses CORE B4 for its heavy ground step. */
 const uint16_t state=h(actor+0x2f4);
 const unsigned phase=actor[0x2f6];
 const int ema2_step=dependency==0x2a2&&v->part_index==-1&&
     v->model->root.original_vtable214==0x448ca0&&h(actor+0x2fe)==0x2a2&&
     v->model->work&&v->model->work->room==0x100&&
     ((ground_step&&((state==0x0200&&phase==3)||(state==0x0400&&(phase==1||phase==3))||
                     (!state&&phase==5&&v->model->root.size>=0x642&&h(actor+0x640)==6)))||
      (gate_death_step&&state==0x0400&&phase==3));
 const int paired_death=gate_death_step&&(r103_hold_death_player_voice(v)||
    (v->model->work&&v->model->work->room==0x100&&
     v->model->root.original_vtable214==0x41d220&&h(actor+0x2fe)==0x100&&
     state==0x0b01&&phase==5&&!h(actor+0x54a)&&v->part_index==-1));
 if(v->model->root.original_vtable214!=0x4479c8&&!ema2_step&&!paired_death)return 0;
 const int arena_foot=r103_em00_voice_actor(v)!=NULL;
 const int arena_em03=r103_em03_voice_actor(v)!=NULL;
 const uint8_t *boss=r103_boss_voice_actor(v);
 /* Original EM15 rows 560 and 568 contain CORE B4 during heavy recoil. */
 const int recoil_step=gate_death_step&&boss&&!boss[0x2f7]&&v->part_index==-1&&
    ((state==0x0301&&boss[0x2f6]==3)||(state==0x0401&&boss[0x2f6]==5));
 const int arena_boss_step=ground_step&&boss&&v->part_index==-1&&
    v->model->root.size>=0x16c8&&
    ((boss[0x2f6]==1&&(state==0x0500||state==0x0600||state==0x0700||state==0x0800||
      (state==0x1600&&w(boss+0x564)==0x214)||(state==0x1500&&w(boss+0x564)==0x215)||
      (state==0x1300&&w(boss+0x16c4)==3)||
      state==0x0900||(state==0x1600&&dependency==0x214)||state==0x1500||
      r103_boss_grab_attempt(boss)||r103_em15_throw_in_motion(boss)||
      ((state==0x6c00||state==0x6f00||state==0x0101||state==0x0d01)&&!boss[0x2f7])||
      (state==0x1101&&!boss[0x2f7]&&dependency==0x214&&v->model->root.size>=0x17b8&&
       w(boss+0x17b4)==10)||
      (state==0x0a00&&boss[0x2f7]==1)||r103_boss_paired_attack(boss)||
      state==0x3b00||state==0x3c00))||
     /* Original EM15 turning row 10B0 plays CORE AA in phase 3. */
     (!state&&(phase==2||(phase==3&&dependency==0x215&&!boss[0x2f7])))||
     (state==0x0101&&boss[0x2f6]==3&&!boss[0x2f7])||
     (state==0x3b00&&(boss[0x2f6]==3||boss[0x2f6]==5))||r103_pincer_flank_withdrawal(boss)||
     r103_em14_hold_voice(v,boss,5));
 const int arena_hold=(arena_foot&&(state==0x7500||state==0x4d00))||
     (arena_em03&&state==0x7500)||(boss&&state==0x7500&&boss[0x2f6]==3)||r103_boss_paired_attack(boss)||
     (v->part_index==-1&&(r103_pincer_flank_audio(boss)||r103_pincer_flank_withdrawal(boss)))||
     (boss&&dependency==0x214&&state==0x5200&&boss[0x2f6]==3&&!boss[0x2f7]&&v->part_index==-1);
 if(h(actor+0x2fe)!=0x202&&!ema2_step&&!paired_death&&!recoil_step&&
    !(((arena_foot||arena_em03)&&(ground_step||hard_step))||
      (arena_hold&&hold_step)||arena_boss_step))return 0;
 if(gate_death_step){
  const uint8_t *gate_actor=r102_voice_actor(v);
  const unsigned west_stamp=h(actor+0x640);
  const int west_formation=v->model->root.size>=0x16c8&&w(actor+0x16c4)==6&&actor[0x1650]==2&&
     ((dependency==0x200&&west_stamp==3)||(dependency==0x202&&west_stamp==4)||(dependency==0x203&&west_stamp==5));
  const int west_wave=v->model->root.size>=0x16c8&&w(actor+0x16c4)==2&&!actor[0x1650]&&
     ((dependency==0x202&&west_stamp==29)||(dependency==0x203&&west_stamp==30));
  const int west_death=v->model->work&&v->model->work->room==0x100&&
     (west_formation||west_wave)&&
     (state==0x0701||state==0x0102)&&phase==1&&!h(actor+0x54a)&&v->part_index==-1;
  /* The original 25E790 and 25BCC0 motion SEQs play CORE B4. */
  if(!ema2_step&&!west_death&&!paired_death&&!recoil_step&&(!gate_actor||dependency!=0x200||gate_actor[0x2f6]!=1))return 0;
  const unsigned stamp=gate_actor?h(gate_actor+0x640):0,state=gate_actor?h(gate_actor+0x2f4):0;
  const int fallen=gate_actor&&state==2&&!h(gate_actor+0x54a)&&
      ((stamp==0x10&&!gate_actor[0x1650])||(stamp==11&&gate_actor[0x1650]==5));
  const int west=gate_actor&&stamp==7&&v->model->root.size>=0x16c8&&!gate_actor[0x1650]&&w(gate_actor+0x16c4)==2;
  const int counter=gate_actor&&((stamp==11&&gate_actor[0x1650]==5)||west)&&state==0x1d01&&gate_actor[0x2f7]==1;
  return (fallen||counter||paired_death||west_death||ema2_step||recoil_step)&&v->part_index==-1&&
    w(p)==0x00b40000u&&w(p+4)==0x0420006cu&&w(p+8)==0x25au&&w(p+12)==0x00200000u&&
    w(p+16)==0x00640064u&&w(p+20)==0x005a0040u&&w(p+24)==0x0080ff80u&&
    w(p+28)==0x00010000u&&w(p+32)==0x00060005u&&w(p+36)==0x301u&&
    w(p+40)==0xffffu&&!w(p+44)&&(!sample||(w(sample)==0xe6d0u&&w(sample+4)==16000&&
       w(sample+8)==0x5fc080ffu&&!w(sample+12)));
 }
 if(r101_step){
  const gh_title_player_view *root=&v->model->root;
  const unsigned alt=program==0xb1||program==0xb7;
  if(root->size<=0x1650||!v->model->work||v->model->work->room!=0x101||v->part_index!=-1)return 0;
  const unsigned stamp=h(root->bytes+0x640);
  const unsigned appearance=root->bytes[0x1650];
  const int armed=dependency==0x203&&stamp==13&&appearance==0;
  const int scenery=dependency==0x227&&stamp==9&&appearance==4&&other_surface;
  const int r101_gate_step=dependency==0x227&&(stamp==10||stamp==11)&&appearance==4&&
    actor[0x2f6]==1&&(((state==0x0600||state==0x1600)&&!actor[0x2f7])||
     (state==0x0900&&actor[0x2f7]==1));
  const int r101_em00=dependency==0x200&&stamp==12&&appearance==4&&
    (state==0x0500||state==0x0600||state==0x0700||state==0x0800||state==0x0900||
     state==0x1500||state==0x4b00||
     (state==0x1300&&v->model->root.size>=0x16c8&&w(actor+0x16c4)==6))&&actor[0x2f6]==1&&
    (!actor[0x2f7]||((state==0x0800||state==0x0900)&&actor[0x2f7]==1));
  const int r101_home=v->model->root.size>=0x16c8&&w(actor+0x16c4)==6;
  const int r101_walk=dependency==0x200&&stamp==15&&!appearance&&other_surface&&
    ((actor[0x2f6]==1&&
      (((state==0x0500||state==0x0600||state==0x0900||state==0x1500)&&!actor[0x2f7])||
       ((state==0x0800||state==0x0900)&&actor[0x2f7]==1)||
       (state==0x1300&&r101_home&&!actor[0x2f7])))||
     (state==0x0800&&r101_home&&!actor[0x2f6]&&actor[0x2f7]==1));
  const int r101_em02=dependency==0x202&&stamp==5&&appearance==4&&
    other_surface&&actor[0x2f6]==1&&
    ((!actor[0x2f7]&&(state==0x0500||state==0x0600||state==0x0900||state==0x1500||
     (state==0x1300&&r101_home)))||
     (((state==0x0800&&r101_home)||state==0x0900)&&actor[0x2f7]==1));
  const int r101_em03=dependency==0x203&&!stamp&&appearance==1&&!other_surface&&
    actor[0x2f6]==1&&(((state==0x0500||state==0x1500)&&!actor[0x2f7])||(state==0x0900&&actor[0x2f7]==1));
  const int side_run=appearance==6&&actor[0x2f6]==1&&
    (((state==0x0600||state==0x0700)&&!actor[0x2f7])||
     (state==0x0900&&actor[0x2f7]==1))&&
    ((stamp==0x16&&dependency==0x200)||(stamp==0x20&&dependency==0x203));
  const int side_step=appearance==6&&actor[0x2f6]==1&&!actor[0x2f7]&&!other_surface&&
    ((stamp==0x20&&dependency==0x203&&(state==0x0800||state==0x1500))||
     (stamp==0x16&&dependency==0x200&&state==0x1500));
  if(!armed&&!scenery&&!r101_gate_step&&!r101_em00&&!r101_walk&&!r101_em02&&!r101_em03&&
     !side_run&&!side_step)return 0;
  return !h(p)&&h(p+2)==(unsigned)program&&h(p+4)==(other_surface?109u:105u)+alt&&
    w(p+6)==0x023c021c&&!w(p+0xa)&&h(p+0xe)==0x10&&w(p+0x10)==(other_surface?0x00500032u:0x00500028u)&&
    w(p+0x14)==0x005a0040&&w(p+0x18)==0x0100ff00&&w(p+0x1c)==0x00010000&&
    w(p+0x20)==(alt?0x30005u:0x30105u)&&w(p+0x24)==0x401&&w(p+0x28)==0xffff&&!w(p+0x2c)&&
    (!sample||(w(sample)==(other_surface?(alt?0x9d9a0u:0x9d3f0u):0x9cf00u)&&w(sample+4)==14000&&w(sample+8)==0x5fc080ff&&!w(sample+12)));
 }
 if(hold_step){
  const gh_title_player_view *root=&v->model->root;
  const int r100=dependency==0x200&&root->size>=0x642&&h(root->bytes+0x640)==20&&
     v->model->work&&v->model->work->room==0x100&&v->part_index==-1;
  const uint8_t *r102=r102_voice_actor(v);
  const int r102_em00=r102&&dependency==0x200&&h(r102+0x640)==11&&r102[0x1650]==5&&
    h(r102+0x2f4)==0x4d00&&r102[0x2f6]==1;
  if(!r100&&!r102_em00&&!arena_hold&&!r105_em27_voice_source(v)&&!r102_obstacle_voice_actor(v)&&
     r102_mount_voice_actor(v)<0)return 0;
  return h(p+4)==103&&w(p+6)==0x023c021c&&h(p+0xe)==0x10&&w(p+0x10)==0x00500050&&
    w(p+0x14)==0x005a0040&&w(p+0x18)==0x0100ff00&&w(p+0x1c)==0x00030000&&
    w(p+0x20)==0x00030005&&w(p+0x24)==0x401&&w(p+0x28)==0xffff&&!w(p+0x2c)&&
    (!sample||(w(sample)==0x9a790&&w(sample+4)==14000&&w(sample+8)==0x5fc080ff&&!w(sample+12)));
 }
 /* CORE AC/AD is the same authored floor-surface pair on these cEm00
  * dependencies. Keep the same attached root and descriptor checks. */
 if(hard_step){
  if(v->part_index!=-1||(dependency!=0x200&&dependency!=0x202&&dependency!=0x203&&
     dependency!=0x227&&!r102_em0f_voice_source(v)))return 0;
 }else if(dependency==0x227){
  const uint8_t *east=r102_voice_actor(v);
  const int east_step=east&&v->part_index==-1&&h(east+0x640)==4&&!east[0x1650]&&
     !east[0x2f4]&&east[0x2f6]==1&&
     ((east[0x2f5]>=5&&east[0x2f5]<=8&&!east[0x2f7])||
      ((east[0x2f5]==7||east[0x2f5]==9)&&east[0x2f7]<=1));
  const int east_route=ground_step&&(r102_east_em27_route_voice(v,0x1300,0)||
      r102_east_em27_route_voice(v,0x0800,1));
  if(!r105_em27_voice_source(v)&&!east_step&&!east_route&&!r102_east_em27_voice(v,0x1600,1))return 0;
 }else{
  const int r104_recovery=ground_step&&v->part_index==-1&&r104_em03_voice_actor(v)&&
      !actor[0x2f7]&&((state==0x1a00&&actor[0x2f6]==5)||(state==0x4b00&&actor[0x2f6]==1)||
                      (state==0x0101&&(actor[0x2f6]==1||actor[0x2f6]==3)));
  if(dependency!=0x200&&dependency!=0x202&&dependency!=0x203&&
     !r102_em0f_voice_source(v)&&!ema2_step&&!arena_boss_step&&!r104_recovery)return 0;
 }
 unsigned alternate=program!=v->requested_program;
 unsigned first_slot=hard_step?101u:99u;
 uint32_t level_pair=hard_step?0x004b002du:0x00370019u;
 if(h(p+4)!=first_slot+alternate||w(p+6)!=0x023c021c||h(p+0xe)!=0x10||
    w(p+0x10)!=level_pair||w(p+0x14)!=0x005a0040||w(p+0x18)!=0x0100ff00||
    w(p+0x1c)!=0x00010000||w(p+0x20)!=(alternate?0x30005u:0x30105u)||
    w(p+0x24)!=0x401||w(p+0x28)!=0xffff||w(p+0x2c))return 0;
 const uint32_t ground_offset=alternate?0x89af0u:0x89480u;
 const uint32_t sample_offset=hard_step?0x89df0u:ground_offset;
 return !sample||(w(sample)==sample_offset&&w(sample+4)==14000&&
    w(sample+8)==0x5fc080ff&&!w(sample+12));
}
static int r100_em0a_formation_source(const struct voice *v)
{
 const gh_actor_model *model=v->model;
 if(!model||!model->work||model->work->room!=0x100)return 0;
 const gh_title_player_view *root=&model->root;
 return root->bytes&&root->size>0x1650&&root->original_vtable214==0x4479c8&&
    h(root->bytes+0x2fe)==0x202&&w(root->bytes+0x564)==0x20a&&
    !h(root->bytes+0x640)&&root->bytes[0x1650]==3;
}

static int em0a_attack_voice_profile(const struct voice *v,int16_t bank,int16_t program,
 const uint8_t *p,const uint8_t *sample)
{
 const uint8_t *arena=r103_em0a_voice_actor(v);
 const unsigned arena_state=arena?h(arena+0x2f4):0;
 const int arena_cue=arena&&(arena_state==0x7500||
    ((arena_state==0x1900||arena_state==0x4500||arena_state==0x4b00||arena_state==0x6c00)&&
     arena[0x2f6]==1&&!arena[0x2f7]))&&
    v->requested_program==6&&program==6;
 const int arena_close=arena&&!arena[0x2f7]&&v->requested_program==4&&(program==4||program==5)&&
    ((arena_state==0x4c00&&(arena[0x2f6]==1||arena[0x2f6]==3))||
     ((arena_state==0x1500||arena_state==0x4400||arena_state==0x6c00)&&arena[0x2f6]==1));
 const int arena_strike_tail=arena&&(arena_state==0x1500||arena_state==0x4400||arena_state==0x4500)&&
    arena[0x2f6]==1&&!arena[0x2f7]&&
    v->requested_program==0x2c&&program==0x2c;
 if(arena&&arena_state==0x6c00&&arena[0x2f6]==1&&!arena[0x2f7]&&
    em0a_actor_bank(v,bank)&&v->part_index==-1&&v->requested_program==program&&
    (program==0xa||program==0xb))
  return h(p)==(unsigned)bank&&h(p+2)==(unsigned)program&&
    w(p+4)==(0x022b0000u|(unsigned)(program-2))&&
    w(p+8)==0x255&&w(p+12)==0x00200000u&&w(p+16)==(program==0xa?0x007d007du:0x00780078u)&&
    w(p+20)==0x005a0040u&&!w(p+24)&&w(p+28)==0x00030000u&&
    w(p+32)==0x00010085u&&w(p+36)==0x400&&w(p+40)==0xffff&&!w(p+44)&&
    (!sample||(w(sample)==(program==0xa?0x19130u:0x1bfd0u)&&w(sample+4)==11025&&
      w(sample+8)==0x5fcb80ffu&&!w(sample+12)));
 if(arena&&arena_state==0x1900&&arena[0x2f6]==1&&!arena[0x2f7]&&
    em0a_actor_bank(v,bank)&&v->part_index==-1&&v->requested_program==0x18&&program==0x18)
  return h(p)==(unsigned)bank&&h(p+2)==0x18&&w(p+4)==0x02230010u&&
    w(p+8)==0x23c&&w(p+12)==0x00200000u&&
    w(p+16)==0x007f007fu&&w(p+20)==0x005a0040u&&w(p+24)==0x0080ff80u&&
    !w(p+28)&&w(p+32)==5&&w(p+36)==0x300&&w(p+40)==0xffff&&!w(p+44)&&
    (!sample||(w(sample)==0x1ea40u&&w(sample+4)==16000u&&
      w(sample+8)==0x5fcb80ffu&&!w(sample+12)));
 const uint8_t *actor=r102_em0a_voice_actor(v);
 const int gate_attack=actor&&
    (((h(actor+0x2f4)==0x1500||h(actor+0x2f4)==0x4400)&&actor[0x2f6]==1&&
      ((v->requested_program==0x2c&&program==0x2c)||
       (v->requested_program==4&&(program==4||program==5))))||
     ((h(actor+0x2f4)==0x4500||h(actor+0x2f4)==0x4b00||h(actor+0x2f4)==0x6c00)&&
      actor[0x2f6]==1&&v->requested_program==6&&program==6)||
     (h(actor+0x2f4)==0x4500&&actor[0x2f6]==1&&v->requested_program==0x2c&&program==0x2c)||
     (h(actor+0x2f4)==0x6c00&&actor[0x2f6]==1&&v->requested_program==4&&(program==4||program==5))||
     (h(actor+0x2f4)==0x4c00&&(actor[0x2f6]==1||actor[0x2f6]==3)&&
      v->requested_program==4&&(program==4||program==5)));
 if(!em0a_actor_bank(v,bank)||v->part_index!=-1||
    (!r100_em0a_formation_source(v)&&!gate_attack&&!arena_cue&&!arena_close&&!arena_strike_tail)||
    h(p+2)!=(unsigned)program||w(p+12)!=0x00200000u||w(p+20)!=0x005a0040u||
    w(p+40)!=0xffff||w(p+44))return 0;
 if(v->requested_program==0x2c&&program==0x2c)
    return w(p+4)==0x020b0013u&&w(p+8)==0x246&&w(p+16)==0x005a005au&&
      w(p+24)==0x0080ff00u&&!w(p+28)&&w(p+32)==5&&w(p+36)==0x300&&
      (!sample||(w(sample)==0x237f0u&&w(sample+4)==16000&&
                w(sample+8)==0x5fcb80ffu&&!w(sample+12)));
 if(!((v->requested_program==4&&(program==4||program==5))||
      (v->requested_program==6&&program==6)))return 0;
 static const uint32_t offsets[]={0x9c80u,0xae40u,0xbbb0u};
 const uint32_t gain=program==5?0x00780078u:0x007d007du;
 return w(p+4)==(0x022a0000u|(unsigned)(program-1))&&w(p+8)==0x250&&
    w(p+16)==gain&&!w(p+24)&&w(p+28)==0x00060000u&&
    w(p+32)==(program==4?0x00010185u:0x00010085u)&&w(p+36)==0x400&&
    (!sample||(w(sample)==offsets[program-4]&&w(sample+4)==11025&&
              w(sample+8)==0x5fcb80ffu&&!w(sample+12)));
}

static unsigned r101_side_arena_landing_dependency(const struct voice *v)
{
 const gh_actor_model *model=v->model;
 if(!model||!model->work||model->work->room!=0x101||v->part_index!=-1)return 0;
 const uint8_t *actor=model->root.bytes;
 if(!actor||model->root.size<=0x1650||model->root.original_vtable214!=0x4479c8||
    h(actor+0x2fe)!=0x202||actor[0x1650]!=6||h(actor+0x2f4)!=0x7500||
    actor[0x2f7]||!actor[0x2f6]||actor[0x2f6]>3)return 0;
 const unsigned stamp=h(actor+0x640);
 return stamp>=0x16&&stamp<=0x28?w(actor+0x564):0;
}

/* Original R102 and R103 SEQs share enemy-bank program 6 across landings and the
 * area 0B EM02 attack. EM00 and EM03 have a second sample. CORE 8E is common. */
static uint32_t enemy_scene_motion_sample(const struct voice *v,int16_t bank,int16_t program,const uint8_t *p)
{
 static const struct {
  uint16_t gain;
  uint32_t sample[2];
 } actors[]={
  {125,{0x3b80,0}},
  {127,{0x3430,0x40f0}},
  {125,{0x5b60,0x7700}}
 };
 if(!v->source_owner||v->requested_bank!=bank)return 0;
 const uint8_t *boss=r103_boss_voice_actor(v);
 const int boss_landing=boss&&h(boss+0x2f4)==0x7500;
 const int boss_throw_partner=boss&&w(boss+0x564)==0x214&&
    h(boss+0x2f4)==0x5200&&boss[0x2f6]==3&&!boss[0x2f7];
 /* EM14 shares its original program 19 between melee and joining the throw. */
 if((boss_throw_partner||(boss&&w(boss+0x564)==0x214&&h(boss+0x2f4)==0x1600&&boss[0x2f6]==1))&&
    v->part_index==-1&&bank>=1&&bank!=2&&bank!=3&&v->requested_program==0x19&&program==0x19){
  return w(p)==(0x00190000u|(uint16_t)bank)&&w(p+4)==0x022b0013u&&w(p+8)==0x255u&&
    w(p+12)==0x00200000u&&w(p+16)==0x00780078u&&w(p+20)==0x005a0040u&&w(p+24)==0x0064ff38u&&
    w(p+28)==0x00080000u&&w(p+32)==0x00010085u&&w(p+36)==0x400u&&
    w(p+40)==0xffffu&&!w(p+44)?0x1c9b0u:0;
 }
 const int boss_grab_attempt=r103_boss_grab_attempt(boss);
 const int boss_hold_death=r103_em14_hold_voice(v,boss,5);
 const int boss_attack=boss&&h(boss+0x2f4)==0x1500&&v->part_index==-1&&boss[0x2f6]==1;
 if(boss_attack&&w(boss+0x564)==0x214&&bank>=1&&bank!=2&&bank!=3&&
    v->requested_program==0x1a&&program==0x1a){
  return w(p)==(0x001a0000u|(uint16_t)bank)&&w(p+4)==0x022b0014u&&w(p+8)==0x255u&&
    w(p+12)==0x00200000u&&w(p+16)==0x00780078u&&w(p+20)==0x005a0040u&&w(p+24)==0x0064ff38u&&
    w(p+28)==0x00080000u&&w(p+32)==0x00010085u&&w(p+36)==0x400u&&
    w(p+40)==0xffffu&&!w(p+44)?0x1b2c0u:0;
 }
 /* EM04 row 7D8 plays the boss's program 4 and CORE 85 in the hold. */
 if((r103_em15_throw_in_motion(boss)||boss_attack||r103_em14_hold_voice(v,boss,3)||boss_hold_death)&&bank>=1&&bank!=2&&bank!=3&&
    v->requested_program==4&&(program==4||program==5)){
  const unsigned alternate=program==5;
  return w(p)==(((uint32_t)program<<16)|(uint16_t)bank)&&w(p+4)==0x022a0003u+alternate&&
    w(p+8)==0x250u&&w(p+12)==0x00200000u&&w(p+16)==0x00800080u&&w(p+20)==0x005a0040u&&
    w(p+24)==0x00c8ff38u&&w(p+28)==0x00060000u&&w(p+32)==(alternate?0x00010085u:0x00010185u)&&
    w(p+36)==0x400u&&w(p+40)==0xffffu&&!w(p+44)?(alternate?0x10da0u:0x104d0u):0;
 }
 /* The 214/215 bank group resolves these authored program-6 cues to EM14.SES. */
 if((((boss_landing||boss_grab_attempt||boss_attack)&&boss[0x2f6]==1)||boss_hold_death)&&
    bank>=1&&bank!=2&&bank!=3&&v->requested_program==6&&program==6){
  return w(p)==(0x00060000u|(uint16_t)bank)&&w(p+4)==0x002a0005u&&w(p+8)==0x250u&&
    w(p+12)==0x00200000u&&w(p+16)==0x00800080u&&w(p+20)==0x005a0040u&&w(p+24)==0x00c8ff38u&&
    w(p+28)==0x00060000u&&w(p+32)==0x00010085u&&w(p+36)==0x400u&&
    w(p+40)==0xffffu&&!w(p+44)?0x11420u:0;
 }
 const int mount=r102_mount_voice_actor(v);
 const unsigned side_arena=r101_side_arena_landing_dependency(v);
 if(side_arena&&!bank&&v->requested_program==0xb8&&program==0xb8&&
    v->model->root.bytes[0x2f6]==3){
  return w(p)==0x00b80000u&&w(p+4)==0x021c006fu&&w(p+8)==0x23cu&&
    w(p+12)==0x00100000u&&w(p+16)==0x003c003cu&&w(p+20)==0x005a0040u&&
    w(p+24)==0x0100ff00u&&w(p+28)==0x00030000u&&w(p+32)==0x00030005u&&
    w(p+36)==0x401u&&w(p+40)==0xffffu&&!w(p+44)?0x9e000u:0;
 }
 const uint8_t *arena=r103_em00_voice_actor(v);
 const uint8_t *em03=r103_em03_voice_actor(v);
 const uint8_t *em0a=r102_em0a_voice_actor(v);
 const int arena_landing=arena&&h(arena+0x2f4)==0x7500;
 const int arena_em03_landing=em03&&h(em03+0x2f4)==0x7500;
 const uint8_t *arena_em0a=r103_em0a_voice_actor(v);
 const unsigned arena_em0a_state=arena_em0a?h(arena_em0a+0x2f4):0;
 const int arena_em0a_landing=arena_em0a&&(arena_em0a_state==0x7500||
    (arena_em0a_state==0x6c00&&arena_em0a[0x2f6]==1&&!arena_em0a[0x2f7]));
 const int arena_attack=arena&&h(arena+0x2f4)==0x1500;
 const int arena_em03_attack=em03&&(h(em03+0x2f4)==0x1500||h(em03+0x2f4)==0x4b00)&&
    em03[0x2f6]==1;
 const int retreat=em0a&&h(em0a+0x2f4)==0x6c00&&em0a[0x2f6]==1;
 if(!bank&&(mount>=0||r102_obstacle_voice_actor(v)||side_arena||retreat||arena_landing||arena_em03_landing||
    arena_em0a_landing||boss_landing||boss_throw_partner||r103_boss_paired_attack(boss)||
    (v->part_index==-1&&r103_pincer_flank_audio(boss)))&&
    v->requested_program==0x8e&&program==0x8e)
  return w(p)==0x008e0000u&&w(p+4)==0x020b005du&&w(p+8)==0x246&&
    w(p+12)==0x00200000&&w(p+16)==0x005a005au&&w(p+20)==0x005a0040&&
    w(p+24)==0x00800080u&&w(p+28)==0x00060000&&w(p+32)==0x00030005u&&
    w(p+36)==0x300&&w(p+40)==0xffff&&!w(p+44)?0x57420u:0;
 /* The AT0B EM02 uses the same authored voice for the 4E motion and
  * the late selector 15 attack. */
 if(bank>=4&&v->requested_program==4&&(program==4||program==5)&&
    (r100_wave_em02_action_voice(v,0x4e00)||r100_at3_em02_combo_voice(v)||r102_em02_route_voice(v,0x4e00)||
     (r102_em02_route_voice(v,0x1500)&&v->model->root.bytes[0x2f6]==1))){
  const unsigned alternate=(unsigned)(program-4);
  return w(p)==((uint32_t)program<<16|(uint16_t)bank)&&
    w(p+4)==0x022a0003u+alternate&&w(p+8)==0x250&&w(p+12)==0x00200000&&
    w(p+16)==(alternate?0x007d007d:0x00780078)&&w(p+20)==0x005a0040&&
    w(p+24)==0x00c8ff38&&w(p+28)==0x00060000&&
    w(p+32)==(alternate?0x00010085:0x00010185)&&w(p+36)==0x400&&
    w(p+40)==0xffff&&!w(p+44)?(alternate?0x32a0u:0x2c00u):0;
 }
 const int wave_throw=r100_wave_em02_action_voice(v,0x4e00)&&v->model->root.bytes[0x2f6]==5;
 if(wave_throw&&bank>=4&&v->requested_program==0xb&&program==0xb)
  return h(p)==(unsigned)bank&&h(p+2)==0xb&&w(p+4)==0x022d0009u&&
    w(p+8)==0x255u&&w(p+12)==0x00200000u&&w(p+16)==0x00730073u&&
    w(p+20)==0x005a0040u&&w(p+24)==0x0064ff38u&&w(p+28)==0x00030000u&&
    w(p+32)==0x00010085u&&w(p+36)==0x400u&&w(p+40)==0xffffu&&!w(p+44)?58208u:0;
 const int wave_em02=wave_throw||r100_at3_em02_combo_voice(v)||
    (r100_wave_em02_action_voice(v,0x4b00)&&v->model->root.bytes[0x2f6]==1);
 const int index=wave_em02||r102_em02_route_voice(v,0x1500)||r102_em02_route_voice(v,0x4b00)||side_arena==0x202?0:
     (arena_landing||arena_attack||side_arena==0x200)?1:
     (arena_em03_landing||arena_em03_attack||side_arena==0x203)?2:mount;
 if(index<0||bank<1||bank==2||bank==3||v->requested_program!=6||(program!=6&&program!=7))return 0;
 const unsigned alternate=(unsigned)(program-6);
 const uint32_t variants=program==6&&actors[index].sample[1]?0x10185u:0x10085u;
 if(!actors[index].sample[alternate]||
    w(p)!=((uint32_t)program<<16|(uint16_t)bank)||w(p+4)!=(0x022a0005u+alternate)||
    w(p+8)!=0x250||w(p+12)!=0x00200000||w(p+16)!=actors[index].gain*0x10001u||
    w(p+20)!=0x005a0040||w(p+24)!=0x00c8ff38||w(p+28)!=0x00060000||
    w(p+32)!=variants||w(p+36)!=0x400||w(p+40)!=0xffff||w(p+44))return 0;
 return actors[index].sample[alternate];
}

/* Original CORE 8D turn cue, also in EM0A's R100 formation SEQ. */
static int core_r101_weapon_player_impact_profile(const struct voice *v,int16_t bank,
 int16_t program,const uint8_t *p,const uint8_t *sample)
{
 if(!v->source_owner||v->requested_bank||bank||v->requested_program!=program||
    (program!=0x140&&program!=0x111)||v->part_index!=-1)return 0;
 if(v->model){
  const gh_title_player_view *actor=&v->model->root;
  if(!v->model->work||v->model->work->room!=0x101||!actor->bytes||actor->size<0x300)
   return 0;
  const int gene=actor->original_vtable214==0x41d220&&h(actor->bytes+0x2fe)==0x100;
  const int armed_sequence=program==0x111&&actor->original_vtable214==0x4479c8&&
     actor->size>0x1650&&h(actor->bytes+0x2fe)==0x202&&w(actor->bytes+0x564)==0x203&&
     h(actor->bytes+0x640)==13&&!actor->bytes[0x1650];
  if(!gene&&!armed_sequence)return 0;
 }else if(!v->combat_profile)return 0;
 const int follow_up=program==0x111;
 return w(p)==((uint32_t)program<<16)&&w(p+4)==(follow_up?0x04060084u:0x040600a5u)&&
        w(p+8)==0x50&&w(p+12)==0x00200000&&
        w(p+16)==(follow_up?0x006e006eu:0x007f007fu)&&w(p+20)==0x005a0040&&
        !w(p+24)&&w(p+28)==0x00060000&&w(p+32)==0x00030005&&
        w(p+36)==1&&w(p+40)==0xffff&&!w(p+44)&&
        (!sample||(w(sample)==0x4e6a0&&w(sample+4)==16000&&
                  w(sample+8)==0x5fc080ff&&!w(sample+12)));
}

/* Original 26ECD0 plays CORE F7, then the attached enemy 14/15 variant. */
static int enemy_pressure_cue_profile(const struct voice *v,int16_t bank,int16_t program,
 const uint8_t *p,const uint8_t *sample)
{
 if(!v->source_owner||v->part_index!=-1||!v->model||!v->model->work||
    v->requested_bank!=bank)return 0;
 const gh_title_player_view *actor=&v->model->root;
 if(!actor->bytes||actor->size<=0x1650||actor->original_vtable214!=0x4479c8||
    h(actor->bytes+0x2fe)!=0x202)return 0;
 const uint32_t dependency=w(actor->bytes+0x564);
 const unsigned stamp=h(actor->bytes+0x640);
 const unsigned appearance=actor->bytes[0x1650];
 const int16_t room=v->model->work->room;
 const int em00=room==0x100&&dependency==0x200&&stamp==10&&appearance==5;
 const int em03=dependency==0x203&&
    ((room==0x101&&stamp==13&&!appearance)||(room==0x100&&stamp==11&&appearance==5));
 const int em27=room==0x101&&dependency==0x227&&stamp==9&&appearance==4;
 if(!em00&&!em03&&!em27)return 0;
 if(h(p)!=(unsigned)bank||h(p+2)!=(unsigned)program||w(p+12)!=0x00200000||
    w(p+20)!=0x005a0040||w(p+40)!=0xffff||w(p+44))return 0;
 if(!bank&&v->requested_program==0xf7&&program==0xf7)
  return w(p+4)==0x04060079&&w(p+8)==0x250&&w(p+16)==0x005a005a&&
         !w(p+24)&&!w(p+28)&&w(p+32)==0x00030005&&!w(p+36)&&
         (!sample||(w(sample)==0x49c0&&w(sample+4)==16000&&
                    w(sample+8)==0x5fc080ff&&!w(sample+12)));
 if(bank<1||bank==2||bank==3||v->requested_program!=0x14||(program!=0x14&&program!=0x15))return 0;
 const int variant=program==0x15;
 static const uint32_t em03_offsets[]={0x23e0,0x38d0};
 static const uint32_t em27_offsets[]={0x5680,0x6300};
 static const uint32_t em00_offsets[]={0,0xc80};
 const uint32_t *offsets=em00?em00_offsets:em03?em03_offsets:em27_offsets;
 const uint32_t base_gain=em00||em03?0x007d007d:0x00780078;
 const uint32_t alternate_gain=em00?0x00780078:0x00730073;
 const uint32_t tone=em00?0x02280000u:0x022d0000u;
 return w(p+4)==(tone|(variant?0xeu:0xdu))&&w(p+8)==0x264&&
        w(p+16)==(variant?alternate_gain:base_gain)&&w(p+24)==0x0064ff9c&&
        w(p+28)==0x00050000&&w(p+32)==(variant?0x00010045:0x00010145)&&
        w(p+36)==0x400&&
        (!sample||(w(sample)==offsets[variant]&&w(sample+4)==11025&&
                   w(sample+8)==0x5fcb80ff&&!w(sample+12)));
}

static int weapon_swing_profile(const struct voice *v,int16_t bank,int16_t program,
 const uint8_t *p,const uint8_t *sample)
{
 if(!v->source_owner||v->requested_bank||bank||v->requested_program!=program||
    (program!=0x113&&program!=0x11d)||v->part_index!=-1)return 0;
 if(v->model){
  const gh_title_player_view *actor=&v->model->root;
  if(!v->model->work||!actor->bytes||actor->size<=0x1650||
     actor->original_vtable214!=0x4479c8||h(actor->bytes+0x2fe)!=0x202)return 0;
  const uint32_t dependency=w(actor->bytes+0x564);
  const unsigned stamp=h(actor->bytes+0x640);
  const int r101=v->model->work->room==0x101&&dependency==0x203&&stamp==13&&
     !actor->bytes[0x1650]&&program==0x113;
  const int r102=v->model->work->room==0x102&&dependency==0x200&&stamp==3&&actor->bytes[0x1650]==1;
  const int r100=v->model->work->room==0x100&&dependency==0x202&&stamp==18&&actor->bytes[0x1650]==9;
  if(!r100&&!r101&&!r102)return 0;
 }else if(!v->combat_profile)return 0;
 if(w(p)!=((uint32_t)program<<16)||w(p+12)!=0x00200000u||w(p+20)!=0x005a0040u||
    w(p+28)!=0x00060000u||w(p+40)!=0xffff||w(p+44))return 0;
 if(program==0x11d)
  return w(p+4)==0x0406008f&&w(p+8)==0x50&&w(p+16)==0x006e006e&&
         w(p+24)==0x0100ff00&&w(p+32)==0x00030005&&w(p+36)==1&&
         (!sample||(w(sample)==0xcbcc0&&w(sample+4)==20000&&
                   w(sample+8)==0x5fc080ff&&!w(sample+12)));
 return w(p+4)==0x020b0086&&w(p+8)==0x246&&w(p+16)==0x007f007f&&
        w(p+24)==0x0080ff80&&w(p+32)==5&&w(p+36)==0x300&&
        (!sample||(w(sample)==0x51310&&w(sample+4)==16000&&
                  w(sample+8)==0x5fc080ff&&!w(sample+12)));
}

static int core_enemy_turn_profile(const struct voice *v,int16_t bank,int16_t program,
 const uint8_t *p,const uint8_t *sample)
{
 if(!v->source_owner||bank||v->requested_program!=program||h(p)||h(p+2)!=(unsigned)program)
  return 0;
 const uint8_t *r104=r104_em03_voice_actor(v);
 const int r104_rear=r104&&h(r104+0x2f4)==0x0a00&&r104[0x2f6]==1&&r104[0x2f7]==1&&v->part_index==-1;
 if(program==0x29){
  if(!r104_rear)return 0;
  /* Original rear SEQ 2C9900 frame7 plays CORE29 alongside CORE8E. */
  return w(p+4)==0x020b001eu&&w(p+8)==0x46u&&w(p+12)==0x00100000u&&
    w(p+16)==0x00500050u&&w(p+20)==0x005a0040u&&w(p+24)==0x00800080u&&
    w(p+28)==0x00010000u&&w(p+32)==5u&&w(p+36)==0x300u&&w(p+40)==0xffffu&&!w(p+44)&&
    (!sample||(w(sample)==0x95340u&&w(sample+4)==16000&&w(sample+8)==0x5fc080ffu&&!w(sample+12)));
 }
 if(program!=0x8d&&program!=0x8e)return 0;
 const unsigned alternate=program==0x8e;
 if(alternate&&v->model){
  if(!r104_rear&&(!r104||h(r104+0x2f4)!=0x1500||r104[0x2f6]!=1||r104[0x2f7]||v->part_index!=-1))return 0;
 }else if(v->model){
  const gh_title_player_view *root=&v->model->root;
  if(!root->bytes||root->size<0x568)return 0;
  const uint8_t *actor=root->bytes;
  const int ema2_home=v->part_index==-1&&v->model->work&&v->model->work->room==0x100&&
     root->original_vtable214==0x448ca0&&h(actor+0x2fe)==0x2a2&&w(actor+0x564)==0x2a2&&
     h(actor+0x2f4)==0x0400&&(actor[0x2f6]==1||actor[0x2f6]==3);
  const uint8_t *arena=r103_em0a_voice_actor(v);
  const int arena_landing=arena&&h(arena+0x2f4)==0x7500&&arena[0x2f6]==3;
  const unsigned arena_state=arena?h(arena+0x2f4):0;
  const uint8_t *boss=r103_boss_voice_actor(v);
  const int boss_rear=boss&&h(boss+0x2f4)==0x0a00&&boss[0x2f6]==1&&boss[0x2f7]==1;
  /* EM04 row 1B88 plays CORE 8D during the shared recovery exit. */
  const int boss_recover=boss&&h(boss+0x2f4)==0x6900&&boss[0x2f6]==3&&boss[0x2f7]<=1;
  const int boss_getup=boss&&h(boss+0x2f4)==0x0d01&&
    boss[0x2f6]==1&&!boss[0x2f7]&&v->part_index==-1;
  const int boss_turn=boss_rear||boss_recover||boss_getup||r103_boss_grab_attempt(boss)||r103_em15_throw_in_motion(boss)||
    r103_boss_paired_attack(boss);
  const int arena_turn=arena&&arena[0x2f6]==1&&v->part_index==-1&&
    ((arena_state==0x0800&&arena[0x2f7]==1)||(arena_state==0x1900&&!arena[0x2f7]));
  const int flank_turn=v->part_index==-1&&r103_pincer_flank_withdrawal(r103_boss_voice_actor(v));
  const int r104_turn=r104&&v->part_index==-1&&
      r104[0x2f6]==1&&
      (((h(r104+0x2f4)==0x0800||h(r104+0x2f4)==0x0b00)&&r104[0x2f7]<=1)||
       (h(r104+0x2f4)==0x2100&&!r104[0x2f7]));
  if(!ema2_home&&(root->original_vtable214!=0x4479c8||
     (h(root->bytes+0x2fe)!=0x202&&!r103_em00_voice_actor(v)&&!r103_em03_voice_actor(v)&&
      !arena_landing&&!arena_turn&&!boss_turn&&!flank_turn&&!r104_turn)))return 0;
  const uint32_t dependency=w(root->bytes+0x564);
  const uint8_t *em0a=r102_em0a_voice_actor(v);
  const int gate_em0a=em0a&&em0a[0x2f6]==1&&
      (h(em0a+0x2f4)==0x4b00||(h(em0a+0x2f4)==0x0800&&em0a[0x2f7]==1));
  const int east_em27=r102_east_em27_voice(v,0x6b00,1)||r102_east_em27_voice(v,0x6c00,1)||
      r102_east_em27_voice(v,0x0800,1)||r102_east_em27_route_voice(v,0x0800,1);
  const int r101_gate=dependency==0x227&&v->model->work&&v->model->work->room==0x101&&
      root->size>0x1650&&v->part_index==-1&&(h(root->bytes+0x640)==10||h(root->bytes+0x640)==11)&&
      root->bytes[0x1650]==4&&h(root->bytes+0x2f4)==0x6b00&&root->bytes[0x2f6]==1;
  const int r101_key_carrier=r101_key_carrier_attack_voice(v,0x4b00);
  if(!ema2_home&&dependency!=0x200&&dependency!=0x202&&dependency!=0x203&&!gate_em0a&&!east_em27&&!r101_gate&&
     !r101_key_carrier&&
     !r102_em0f_voice_source(v)&&!r105_em27_voice_source(v)&&!r100_em0a_formation_source(v)&&
     !arena_landing&&!arena_turn&&!boss_turn&&!flank_turn&&!r104_turn)return 0;
 }else if(!v->combat_profile)return 0;
 if(h(p+4)!=92u+alternate||w(p+6)!=0x0246020bu||w(p+0xa)||h(p+0xe)!=0x20||
    w(p+0x10)!=(alternate?0x005a005au:0x00500050u)||w(p+0x14)!=0x005a0040||w(p+0x18)!=0x00800080||
    w(p+0x1c)!=0x00060000||w(p+0x20)!=0x00030005||w(p+0x24)!=0x300||
    w(p+0x28)!=0xffff||w(p+0x2c))return 0;
 return !sample||(w(sample)==(alternate?0x57420u:0x95340u)&&w(sample+4)==16000&&
     w(sample+8)==0x5fc080ff&&!w(sample+12));
}

/* Authored centered cues can use 2CE588's pending occlusion check. */
static int centered_enemy_motion_profile(const struct voice *v,int16_t bank,int16_t program,
 const uint8_t *p,const uint8_t *sample)
{
 const gh_actor_model *model=v->model;
 if(!bank&&program==0x6f&&v->requested_program==0x6f&&v->part_index==-1){
  const uint8_t *actor=r104_em03_voice_actor(v);
  /* EM03 row 1A40, SEQ 2F5AC0 frame 24. */
  return actor&&h(actor+0x2f4)==0x6c00&&actor[0x2f6]==1&&!actor[0x2f7]&&
     w(p)==0x006f0000u&&w(p+4)==0x04060049u&&w(p+8)==0x50u&&w(p+12)==0x00210000u&&
     w(p+16)==0x007f007fu&&w(p+20)==0x005a0040u&&w(p+24)==0x00800000u&&w(p+28)==0x00ff0000u&&
     w(p+32)==4u&&w(p+36)==1u&&w(p+40)==0xffffu&&!w(p+44)&&
     (!sample||(w(sample)==0x56490u&&w(sample+4)==22050u&&w(sample+8)==0x5fc080ffu&&!w(sample+12)));
 }
 if(!v->source_owner||bank||(program!=0x70&&program!=0x7c&&program!=0xae)||
    v->requested_program!=program||v->part_index!=-1||
    !model||!model->work||model->work->room!=0x100||!model->root.bytes||model->root.size<0x16c8||
    model->root.original_vtable214!=0x4479c8)return 0;
 const uint8_t *actor=model->root.bytes;
 if(program==0x7c){
  const uint32_t dep=w(actor+0x564);
  const unsigned stamp=h(actor+0x640);
  return h(actor+0x2fe)==0x202&&((dep==0x202&&stamp==4)||(dep==0x203&&stamp==5))&&
     actor[0x1650]==2&&w(actor+0x16c4)==6&&w(actor+0x2f4)==0x01006900u&&
     w(p)==0x007c0000u&&w(p+4)==0x04060054u&&w(p+8)==0x50u&&w(p+12)==0x00210000u&&
     w(p+0x10)==0x007f007fu&&w(p+0x14)==0x005a0040u&&w(p+0x18)==0x0100ff00u&&
     w(p+0x1c)==0x00ff0000u&&w(p+0x20)==4&&!w(p+0x24)&&w(p+0x28)==0xffffu&&!w(p+0x2c)&&
     (!sample||(w(sample)==0x38280u&&w(sample+4)==20000&&w(sample+8)==0x5fc080ffu&&!w(sample+12)));
 }
 if(h(actor+0x2fe)!=0x202||w(actor+0x564)!=0x200||h(actor+0x640)!=3||
    actor[0x1650]!=2||w(actor+0x16c4)!=6||h(actor+0x2f4)!=0x4200||actor[0x2f6]!=1)return 0;
 if(program==0xae){
  if(w(p)!=0x00ae0000u||w(p+4)!=0x021c0067u||w(p+8)!=0x23cu||w(p+12)!=0x00100000u||
     w(p+0x10)!=0x00500050u||w(p+0x14)!=0x005a0040u||w(p+0x18)!=0x0100ff00u||
     w(p+0x1c)!=0x00030000u||w(p+0x20)!=0x00030005u||w(p+0x24)!=0x401u||w(p+0x28)!=0xffffu||w(p+0x2c))
   return 0;
  return !sample||(w(sample)==0x9a790u&&w(sample+4)==14000&&
     w(sample+8)==0x5fc080ffu&&!w(sample+12));
 }
 if(w(p)!=0x00700000u||w(p+4)!=0x0406004au||w(p+8)!=0x50u||w(p+12)!=0x00210000u||
    w(p+0x10)!=0x007f007fu||w(p+0x14)!=0x005a0040u||w(p+0x18)!=0x00800000u||
    w(p+0x1c)!=0x00ff0000u||w(p+0x20)!=4||w(p+0x24)!=1||w(p+0x28)!=0xffffu||w(p+0x2c))
  return 0;
 return !sample||(w(sample)==0x78450u&&w(sample+4)==22050&&
    w(sample+8)==0x5fc080ffu&&!w(sample+12));
}

/* The selector15 CORE cues use the same spatial owner as the local bank. */
/* Original attached EM0A attacks use the same CORE positional cues as EM00. */
static int em0a_core_attack_source(const struct voice *v,int16_t program)
{
 if((program!=0x84&&program!=0x85&&program!=0x87)||v->part_index!=-1)return 0;
 if(r100_em0a_formation_source(v))return program==0x87;
 if(r103_em0a_voice_actor(v))return 1;
 const uint8_t *actor=r102_em0a_voice_actor(v);
 if(!actor)return 0;
 const unsigned state=h(actor+0x2f4);
 if(program==0x84)return state==0x1500&&actor[0x2f6]==1&&!actor[0x2f7];
 return (program==0x87&&state==0x4c00&&actor[0x2f6]==3)||
    ((state==0x1500||state==0x6c00)&&actor[0x2f6]==1&&!actor[0x2f7]);
}

static int r103_heavy_recoil_source(const struct voice *v)
{
 const uint8_t *boss=r103_boss_voice_actor(v);
 /* The original voice allocation is queued at phase 0. Deferred PCM setup
  * can see phase 1 after this same frame advances the recoil motion. */
 return boss&&h(boss+0x2f4)==0x0301&&boss[0x2f6]<=1&&!boss[0x2f7]&&
    (w(boss+0x16d0)&0x1000u)&&v->part_index==-1;
}

static int core_enemy_attack_profile(const struct voice *v,int16_t bank,int16_t program,
 const uint8_t *p,const uint8_t *sample)
{
 if(!v->source_owner||bank||v->requested_program!=program||
    h(p)||h(p+2)!=(unsigned)program)
  return 0;
 if(program==0x2b)
  return ((r103_heavy_recoil_source(v)&&!v->requested_bank)||(!v->model&&v->combat_profile))&&
     w(p+4)==0x020e0020u&&w(p+8)==0x266u&&w(p+12)==0x00210000u&&
     w(p+16)==0x00780078u&&w(p+20)==0x005a0040u&&w(p+24)==0x01000100u&&
     w(p+28)==0x00ff0000u&&w(p+32)==0x00060004u&&!w(p+36)&&w(p+40)==0xffff&&!w(p+44)&&
     (!sample||(w(sample)==0xbddd0u&&w(sample+4)==22050&&w(sample+8)==0x5fc080ffu&&!w(sample+12)));
 if(program==0x7c){
  const uint8_t *actor=r102_voice_actor(v);
  const int em03=actor&&w(actor+0x564)==0x203&&h(actor+0x640)==8&&!actor[0x1650];
  const int east_em00=actor&&w(actor+0x564)==0x200&&h(actor+0x640)==3&&actor[0x1650]==1;
  const int at0b_em02=actor&&v->model->root.size>=0x16c8&&w(actor+0x564)==0x202&&
      h(actor+0x640)==0x17&&!actor[0x1650]&&w(actor+0x16c4)==2;
  const int source=(em03||east_em00||r102_em0a_voice_actor(v)||at0b_em02)&&
      h(actor+0x2f4)==0x6900&&actor[0x2f7]==1;
  const uint8_t *boss=r103_boss_voice_actor(v);
  const int em15=boss&&w(boss+0x564)==0x215&&h(boss+0x2f4)==0x6900&&
    !boss[0x2f6]&&boss[0x2f7]==1&&v->part_index==-1;
  return (source||em15||(!v->model&&v->combat_profile))&&
     w(p+4)==0x04060054u&&w(p+8)==0x50u&&w(p+12)==0x00210000u&&
     w(p+16)==0x007f007fu&&w(p+20)==0x005a0040u&&w(p+24)==0x0100ff00u&&
     w(p+28)==0x00ff0000u&&w(p+32)==4u&&!w(p+36)&&w(p+40)==0xffff&&!w(p+44)&&
     (!sample||(w(sample)==0x38280u&&w(sample+4)==20000&&w(sample+8)==0x5fc080ffu&&!w(sample+12)));
 }
 const int program84=program==0x84;
 const int aged=program==0xa0;
 if(program!=0x83&&!program84&&program!=0x85&&program!=0x87&&!aged)return 0;
 if(v->model){
  const gh_title_player_view *root=&v->model->root;
  if(!root->bytes||root->size<0x568||root->original_vtable214!=0x4479c8)return 0;
  const uint32_t dependency=w(root->bytes+0x564);
  const uint8_t *boss=r103_boss_voice_actor(v);
  const int boss_attack=boss&&v->part_index==-1&&
      ((boss[0x2f6]==1&&
        ((dependency==0x214&&h(boss+0x2f4)==0x1600&&(program==0x83||program84))||
         (h(boss+0x2f4)==0x1500&&(program==0x83||program==0x85||program==0x87))||
         (r103_boss_paired_attack(boss)&&program==0x83)))||
       (program==0x85&&r103_em14_hold_voice(v,boss,3))||
       ((program==0x85||program==0x87)&&r103_em14_hold_voice(v,boss,5))||
       (program84&&h(boss+0x2f4)==0x3b00&&(boss[0x2f6]==9||boss[0x2f6]==11)&&!boss[0x2f7]));
  const uint8_t *r104=r104_em03_voice_actor(v);
  const int r104_grab=program84&&r104&&v->part_index==-1&&
      h(r104+0x2f4)==0x4b00&&r104[0x2f6]==1&&!r104[0x2f7];
  const int r104_melee=program==0x85&&r104&&v->part_index==-1&&
      h(r104+0x2f4)==0x1500&&r104[0x2f6]==1&&!r104[0x2f7];
  if(h(root->bytes+0x2fe)!=0x202&&!boss_attack&&!r104_grab&&!r104_melee){
   const uint8_t *em00=r103_em00_voice_actor(v);
   const int arena_em00=em00&&(program==0x83||program==0x87||
       (program==0x85&&h(em00+0x2f4)==0x1500&&em00[0x2f6]==1));
   const int arena_em03=r103_em03_voice_actor(v)&&(program==0x83||program==0x85||program==0x87||
       (aged&&h(root->bytes+0x2f4)==0x1500&&root->bytes[0x2f6]==1));
   if(!arena_em00&&!arena_em03&&!boss_attack&&!em0a_core_attack_source(v,program))return 0;
  }
  const int r101_gate=program==0x85&&dependency==0x227&&v->model->work&&
      v->model->work->room==0x101&&v->part_index==-1&&root->size>0x1650&&
      (h(root->bytes+0x640)==10||h(root->bytes+0x640)==11)&&root->bytes[0x1650]==4&&
      h(root->bytes+0x2f4)==0x1600&&root->bytes[0x2f6]==1&&!root->bytes[0x2f7];
  const int em27=r101_gate||(program==0x87&&r101_key_carrier_attack_voice(v,0x1500))||((program==0x83||program84||program==0x85||program==0x87)&&r105_em27_voice_source(v))||
      ((program84||program==0x85)&&r102_east_em27_voice(v,0x1600,1))||
      (program==0x87&&(r102_east_em27_voice(v,0x1500,1)||r102_east_em27_voice(v,0x1600,1)));
  const int area11=((aged||program==0x83||program==0x85||program==0x87)&&r102_em02_route_voice(v,0x1500))||
      (program==0x85&&r102_em02_route_voice(v,0x4e00))||
      ((program==0x85||program==0x87)&&r100_wave_em02_action_voice(v,0x4e00))||
      ((program84||program==0x85)&&r100_at3_em02_combo_voice(v));
  const uint8_t *actor=root->bytes;
  const int west_em02=program==0x83&&dependency==0x202&&root->size>=0x16c8&&
      v->model->work&&v->model->work->room==0x100&&h(actor+0x640)==4&&
      actor[0x1650]==2&&w(actor+0x16c4)==6&&h(actor+0x2f4)==0x1500;
  /* EM02's selector-15 sequence rows carry CORE 83, 85, 87 and A0. */
  const uint32_t home_mode=root->size>=0x16c8?w(actor+0x16c4):UINT32_MAX;
  const unsigned stamp=root->size>=0x16c8?h(actor+0x640):UINT16_MAX;
  const int wave_stamp=(stamp==29&&home_mode==2)||(stamp==32&&!home_mode);
  const int wave_em02=wave_stamp&&(program==0x83||program==0x85||program==0x87||aged)&&
      dependency==0x202&&
      v->model->work&&v->model->work->room==0x100&&
      !actor[0x1650]&&h(actor+0x2f4)==0x1500&&actor[0x2f6]==1;
  const int r101_em02=(program==0x83||program==0x87)&&v->part_index==-1&&v->model->work&&
      v->model->work->room==0x101&&root->size>0x1650&&dependency==0x202&&
      h(actor+0x640)==5&&actor[0x1650]==4&&h(actor+0x2f4)==0x1500&&
      actor[0x2f6]==1&&!actor[0x2f7];
  if(!r101_em02&&!boss_attack&&!r104_grab&&!r104_melee&&!area11&&!em27&&!west_em02&&!wave_em02&&
     !em0a_core_attack_source(v,program)&&
     (dependency!=0x200||aged)&&(dependency!=0x203||program84))return 0;
 }else if(!v->combat_profile)return 0;
 const unsigned slot=aged?98u:(unsigned)(program-0x2d);
 const unsigned gain=aged?120u:program==0x83?60u:program==0x85?70u:90u;
 const uint32_t pitch=program==0x85||aged?0x0080ff80u:0x00800080u;
 if(h(p+4)!=slot||w(p+6)!=(aged?0x025a020bu:0x0246020bu)||
    h(p+0xe)!=0x20||w(p+0x10)!=gain*0x10001u||w(p+0x14)!=0x005a0040u||
    w(p+0x18)!=pitch||w(p+0x1c)!=(aged?0x60000u:0)||
    w(p+0x20)!=(aged?0x30005u:4u)||w(p+0x24)!=(aged?0x300u:0x301u)||
    w(p+0x28)!=0xffffu||w(p+0x2c))return 0;
 const uint32_t offset=aged?0x6f7d0u:program==0x83?657040u:program==0x84?674208u:
     program==0x85?0xa2f60u:0xa1ed0u;
 return !sample||(w(sample)==offset&&w(sample+4)==16000&&
    w(sample+8)==0x5fc080ffu&&!w(sample+12));
}

/* CreateItem 2923E8 requests the real CORE FA cue on the new ticket. */
static int r102_ticket_spawn_profile(const struct voice *v,const uint8_t *p)
{
 const gh_actor_model *model=v->model;
 if(!v->source_owner||v->requested_bank||v->requested_program!=0xfa||v->part_index!=-1||
    !model||!model->work||model->work->room!=0x102||!model->root.bytes||model->root.size<0x690||
    model->root.original_vtable214!=0x429b88||h(model->root.bytes+0x2fe)!=0x3da||
    w(model->root.bytes+0x560)!=0x905)return 0;
 return w(p)==0x00fa0000u&&w(p+4)==0x0104007au&&w(p+8)==0x00000278u&&
    w(p+12)==0x00230000u&&w(p+16)==0x00780078u&&w(p+20)==0x005a0040u&&
    !w(p+24)&&w(p+28)==0x00ff0000u&&w(p+32)==0x00030004u&&!w(p+36)&&
    w(p+40)==0xffff&&!w(p+44);
}

/* CORE B4 is the attached impact of the original knockdown motions. */
static int core_knockdown_profile(const struct voice *v,int16_t bank,int16_t program,
 const uint8_t *p,const uint8_t *sample)
{
 if(!v->source_owner||bank||program!=180||v->requested_bank||v->requested_program!=180||
    v->part_index!=-1||w(p)!=0x00b40000u)return 0;
 if(v->model){
  const gh_title_player_view *root=&v->model->root;
  if(!root->bytes||root->size<0x642||root->original_vtable214!=0x4479c8)return 0;
  const int arena_counter=r103_em00_voice_actor(v)&&h(root->bytes+0x2f4)==0x1d01;
  const int arena_knockdown=((r103_em00_voice_actor(v)||r103_em03_voice_actor(v)||
      r103_em0a_voice_actor(v))&&
      h(root->bytes+0x2f4)==0x0701)||(r103_em0a_voice_actor(v)&&
      ((h(root->bytes+0x2f4)==0x7500&&root->bytes[0x2f6]==3)||
       (h(root->bytes+0x2f4)==0x1900&&root->bytes[0x2f6]==7&&!root->bytes[0x2f7])||
       (h(root->bytes+0x2f4)==0x0102&&root->bytes[0x2f6]==1&&!h(root->bytes+0x54a))));
  const unsigned state=h(root->bytes+0x2f4);
  const int boss_knockdown=r103_boss_voice_actor(v)&&!h(root->bytes+0x54a)&&
      (state==0x0701||(state==0x0102&&root->bytes[0x2f6]==1));
  if(!arena_counter&&!arena_knockdown&&!boss_knockdown&&h(root->bytes+0x2fe)!=0x202)return 0;
  const uint32_t dep=w(root->bytes+0x564);
  const unsigned stamp=h(root->bytes+0x640);
  const int stamp8_death=dep==0x200&&stamp==8&&v->model->work&&v->model->work->room==0x100&&
      h(root->bytes+0x2f4)==0x0701&&!h(root->bytes+0x54a);
  const uint8_t *em0a=r102_em0a_voice_actor(v);
  const int gate_blowaway=em0a&&
     ((h(em0a+0x2f4)==0x0801&&em0a[0x2f6]==3&&em0a[0x2f7]==2)||
      (h(em0a+0x2f4)==0x0102&&em0a[0x2f6]==1&&!em0a[0x2f7]&&!h(em0a+0x54a)));
  const uint8_t *em03=r102_voice_actor(v);
  const int east_counter=em03&&dep==0x200&&stamp==3&&em03[0x1650]==1&&
     h(em03+0x2f4)==0x1d01&&em03[0x2f6]==1&&em03[0x2f7]==1;
  const int gate_em03=em03&&dep==0x203&&stamp==8&&!em03[0x1650]&&
     h(em03+0x2f4)==2&&em03[0x2f6]==1&&!em03[0x2f7]&&!h(em03+0x54a);
  const int at3_death=((dep==0x202&&stamp==32)||(dep==0x200&&stamp==31))&&
      v->model->work&&v->model->work->room==0x100&&
      root->size>=0x16c8&&!root->bytes[0x1650]&&!w(root->bytes+0x16c4)&&
      h(root->bytes+0x2f4)==0x0701&&root->bytes[0x2f6]==1&&!h(root->bytes+0x54a);
  const int north_death=dep==0x203&&stamp==11&&v->model->work&&v->model->work->room==0x100&&
      root->size>=0x16c8&&root->bytes[0x1650]==5&&w(root->bytes+0x16c4)==6&&
      state==0x0701&&root->bytes[0x2f6]==1&&!root->bytes[0x2f7]&&!h(root->bytes+0x54a);
  if(!arena_counter&&!arena_knockdown&&!boss_knockdown&&!((dep==0x203&&stamp==19)||(dep==0x200&&stamp==20)||
       r100_em0a_formation_source(v)||stamp8_death||at3_death||north_death||gate_blowaway||gate_em03||east_counter))return 0;
 }else if(!v->combat_profile)return 0;
 if(w(p+4)!=0x0420006cu||w(p+8)!=0x25a||w(p+12)!=0x00200000u||
    w(p+16)!=0x00640064u||w(p+20)!=0x005a0040u||w(p+24)!=0x0080ff80u||
    w(p+28)!=0x00010000u||w(p+32)!=0x00060005u||w(p+36)!=0x301||
    w(p+40)!=0xffff||w(p+44))return 0;
 return !sample||(w(sample)==0xe6d0&&w(sample+4)==16000&&
    w(sample+8)==0x5fc080ffu&&!w(sample+12));
}

static int player_fall_voice_model(const struct voice *v)
{
 return v->source_owner&&v->model&&v->model->root.bytes&&v->model->root.size>=0x300&&
    v->model->root.original_vtable214==0x41d220&&h(v->model->root.bytes+0x2fe)==0x100;
}

/* R100 Devil Init calls CORE 05 with the canonical EM20 root. The original
 * descriptor starts in positional mode 2, without a SEH/reverb request. */
static int r100_devil_spawn_cue(const struct voice *v,const uint8_t *p)
{
 const gh_actor_model *model=v->model;
 if(!v->source_owner||v->part_index!=-1||v->requested_bank||v->requested_program!=5||
    v->info.bank||v->info.program!=5||v->info.position_mode!=2||!model||!model->work||
    model->work->room!=0x100||!model->root.bytes||model->root.size<=0x1651||
    model->root.original_vtable214!=0x4479c8)return 0;
 const uint8_t *actor=model->root.bytes;
 return h(actor+0x2fe)==0x220&&w(actor+0x564)==0x220&&w(actor+0x1644)==0x400000&&
    actor[0x1651]==0xff&&w(p)==0x00050000&&w(p+4)==0x00310000&&w(p+8)==0x272&&
    w(p+0xc)==0x00220000&&w(p+0x10)==0x007f007f&&w(p+0x14)==0x005a0040&&
    !w(p+0x18)&&!w(p+0x1c)&&w(p+0x20)==4&&!w(p+0x24)&&!w(p+0x28)&&!w(p+0x2c);
}

/* EM20's ranged sequence requests the authored CORE 3D cue on frame 22. */
static int r100_devil_ranged_cue(const struct voice *v,int16_t bank,int16_t program,
 const uint8_t *p)
{
 const gh_actor_model *model=v->model;
 if(!v->source_owner||v->part_index!=-1||bank||program!=0x3d||v->requested_bank||
    v->requested_program!=0x3d||v->info.position_mode!=2||!model||!model->work||
    model->work->room!=0x100||!model->root.bytes||model->root.size<=0x1651||
    model->root.original_vtable214!=0x4479c8)return 0;
 const uint8_t *actor=model->root.bytes;
 return h(actor+0x2fe)==0x220&&w(actor+0x564)==0x220&&w(actor+0x1644)==0x400000&&
    actor[0x1651]==0xff&&!actor[0x2f4]&&actor[0x2f5]==0x23&&actor[0x2f7]==2&&
    w(p)==0x003d0000u&&w(p+4)==0x01140029u&&w(p+8)==0x25a&&w(p+0xc)==0x00230000u&&
    w(p+0x10)==0x00780078u&&w(p+0x14)==0x005a0040u&&w(p+0x18)==0x00800080u&&
    w(p+0x1c)==0x00ff0000u&&w(p+0x20)==4&&!w(p+0x24)&&w(p+0x28)==0xffff&&!w(p+0x2c);
}

/* EM20's rise and reaction use its authored dynamic SES bank. */
static int r100_devil_rise_cue(const struct voice *v,int16_t bank,int16_t program,
 const uint8_t *p,const uint8_t *sample)
{
 const gh_actor_model *model=v->model;
 const int request=v->requested_program;
 if(!v->source_owner||v->part_index!=-1||bank<4||bank!=v->requested_bank||
    !model||!model->work||model->work->room!=0x100||
    !model->root.bytes||model->root.size<=0x1651||
    model->root.original_vtable214!=0x4479c8)return 0;
 const uint8_t *actor=model->root.bytes;
 const unsigned state=h(actor+0x2f4);
 if(program>=0&&program<=2&&((state==0x0101&&!request)||
    (state==0x1101&&(!request||(request==1&&program==1))))){
  static const uint32_t samples[]={54848,64032,69984};
  return h(actor+0x2fe)==0x220&&w(actor+0x564)==0x220&&w(actor+0x1644)==0x400000u&&
     actor[0x1651]==0xff&&actor[0x2f6]<=1&&
     h(p)==(unsigned)bank&&h(p+2)==(unsigned)program&&h(p+4)==(unsigned)program&&
     w(p+6)==0x02550228u&&h(p+0xe)==0x20&&w(p+0x10)==0x00780078u&&
     w(p+0x14)==0x005a0040u&&!w(p+0x18)&&!w(p+0x1c)&&
     w(p+0x20)==(program?5u:0x205u)&&w(p+0x24)==0x400u&&
     w(p+0x28)==0xffffu&&!w(p+0x2c)&&(!sample||
        (w(sample)==samples[program]&&w(sample+4)==11025&&w(sample+8)==0x5fc080ffu&&!w(sample+12)));
 }
 unsigned slot,offset,variants;
 if((request==4&&program>=4&&program<=6)||(request==6&&program==6&&
    h(model->root.bytes+0x2f4)==0x1600&&model->root.bytes[0x2f6]==3)){
  static const unsigned offsets[]={32160,40000,47776};
  slot=(unsigned)program-1;
  offset=offsets[program-4];
  variants=program==4?2:0;
 }else if((request==0x0a&&(program==0x0a||program==0x0b))||
    (request==0x0b&&program==0x0b&&h(model->root.bytes+0x2f4)==0x6b00&&
     model->root.bytes[0x2f6]==1)){
  slot=(unsigned)program-3;
  offset=program==0x0a?76576:85808;
  variants=program==0x0a?1:0;
 }else return 0;
 if(h(actor+0x2fe)!=0x220||w(actor+0x564)!=0x220||w(actor+0x1644)!=0x400000||
    actor[0x1651]!=0xff||h(p)!=(unsigned)bank||h(p+2)!=(unsigned)program||
    h(p+4)!=slot||w(p+6)!=0x02500228||h(p+0xe)!=0x10||w(p+0x10)!=0x007f007f||
    w(p+0x14)!=0x005a0040||w(p+0x18)||w(p+0x1c)||w(p+0x20)!=(0x00030005|(variants<<8))||
    w(p+0x24)!=0x400||w(p+0x28)!=0xffff||w(p+0x2c))return 0;
 return !sample||(w(sample)==offset&&w(sample+4)==11025&&
    w(sample+8)==0x5fc080ff&&!w(sample+12));
}

/* Authored EM20 idle-motion cue 0x11 uses two slots with one sample. */
static int r100_devil_idle_cue(const struct voice *v,int16_t bank,int16_t program,
 const uint8_t *p,const uint8_t *sample)
{
 const gh_actor_model *model=v->model;
 if(!v->source_owner||v->part_index!=-1||bank<4||bank!=v->requested_bank||
    v->requested_program!=0x11||(program!=0x11&&program!=0x12)||
    !model||!model->work||model->work->room!=0x100||!model->root.bytes||
    model->root.size<=0x1651||model->root.original_vtable214!=0x4479c8)return 0;
 const uint8_t *actor=model->root.bytes;
 const uint32_t cue_flags=program==0x11?0x00050105:0x00050005;
 if(h(actor+0x2fe)!=0x220||w(actor+0x564)!=0x220||w(actor+0x1644)!=0x400000||
    actor[0x1651]!=0xff||h(p)!=(unsigned)bank||h(p+2)!=(unsigned)program||
    h(p+4)!=(unsigned)program-7||w(p+6)!=0x023c021e||w(p+0xa)||h(p+0xe)!=0x10||
    w(p+0x10)!=0x005a005a||w(p+0x14)!=0x005a0040||w(p+0x18)!=0x0080ff80||
    w(p+0x1c)!=0x00070000||w(p+0x20)!=cue_flags||
    w(p+0x24)!=0x300||w(p+0x28)!=0xffff||w(p+0x2c))return 0;
 return !sample||(w(sample)==94144&&w(sample+4)==14000&&
    w(sample+8)==0x5fc080ff&&!w(sample+12));
}

/* EM20 retreat and its long strike use distinct authored motion cues. */
static int r100_devil_retreat_cue(const struct voice *v,int16_t bank,int16_t program,
 const uint8_t *p,const uint8_t *sample)
{
 const gh_actor_model *model=v->model;
 if(!v->source_owner||v->part_index!=-1||bank<4||bank!=v->requested_bank||
    v->requested_program!=program||(program!=0x21&&program!=0x20&&program!=0x1e)||
    !model||!model->work||model->work->room!=0x100||!model->root.bytes||
    model->root.size<=0x1651||model->root.original_vtable214!=0x4479c8)return 0;
 const uint8_t *actor=model->root.bytes;
 if(h(actor+0x2fe)!=0x220||w(actor+0x564)!=0x220||w(actor+0x1644)!=0x400000||
    actor[0x1651]!=0xff||h(p)!=(unsigned)bank||h(p+2)!=(unsigned)program)return 0;
 if(program==0x1e)
  return h(actor+0x2f4)==0x1500&&actor[0x2f6]==1&&!actor[0x2f7]&&
     w(p+4)==0x0206000eu&&w(p+8)==0x250u&&w(p+0xc)==0x00210000u&&
     w(p+0x10)==0x00500050u&&w(p+0x14)==0x005a0040u&&w(p+0x18)==0x0080ff80u&&
     w(p+0x1c)==0x00ff0000u&&w(p+0x20)==5&&w(p+0x24)==0x300u&&w(p+0x28)==0xffffu&&!w(p+0x2c)&&
     (!sample||(w(sample)==0x40e0u&&w(sample+4)==16000&&w(sample+8)==0x5fc080ffu&&!w(sample+12)));
 if(program==0x20)
  return ((h(actor+0x2f4)==0x1500&&actor[0x2f6]==1&&!actor[0x2f7])||
     (h(actor+0x2f4)==0x1600&&(actor[0x2f6]==3||actor[0x2f6]==5||actor[0x2f6]==7)))&&
     w(p+4)==0x020b0010u&&w(p+8)==0x246u&&w(p+0xc)==0x00200000u&&
     w(p+0x10)==0x00500050u&&w(p+0x14)==0x005a0040u&&w(p+0x18)==0x02fa02fau&&
     w(p+0x1c)==0x00010000u&&w(p+0x20)==0x00060005u&&w(p+0x24)==0x300u&&
     w(p+0x28)==0xffffu&&!w(p+0x2c)&&
     (!sample||(!w(sample)&&w(sample+4)==16000&&w(sample+8)==0x5fc080ffu&&!w(sample+12)));
 if(w(p+4)!=0x020b0011u||w(p+8)!=0x246u||w(p+0xc)!=0x00200000u||
    w(p+0x10)!=0x006e006eu||w(p+0x14)!=0x005a0040u||w(p+0x18)!=0x0080ff80u||
    w(p+0x1c)!=0x00010000u||w(p+0x20)!=0x00060005u||w(p+0x24)!=0x300u||
    w(p+0x28)!=0xffffu||w(p+0x2c))return 0;
 return !sample||(w(sample)==0x6380u&&w(sample+4)==16000&&w(sample+8)==0x5fc080ffu&&!w(sample+12));
}

/* PL00 recovery and the R102 gate guard's 26F120 use spatial CORE 8C. */
static int recovery_footing_profile(const struct voice *voice,const uint8_t *row,
 const uint8_t *sample)
{
 const gh_actor_model *model=voice->model;
 if(!voice->source_owner||voice->part_index!=-1||voice->requested_bank||
    voice->requested_program!=0x8c||voice->info.bank||voice->info.program!=0x8c||
    voice->info.position_mode!=2||!model||!model->work||!model->root.bytes||
    model->root.size<0x300)return 0;
 const uint8_t *actor=model->root.bytes;
 const int room=model->work->room;
 const uint16_t state=h(actor+0x2f4);
 const int player=player_fall_voice_model(voice)&&
    (room==0x100||room==0x101||room==0x102||room==0x103||room==0x105)&&state==0x0601;
 const int gate_guard=room==0x102&&model->root.size>=0x17d0&&
    model->root.original_vtable214==0x4479c8&&h(actor+0x2fe)==0x202&&w(actor+0x564)==0x200&&
    h(actor+0x640)==0x10&&!actor[0x1650]&&state==1&&!actor[0x2f6]&&
    w(actor+0x17cc)==0x43160000u;
 const int arena_stun=model->root.size>=0x17d0&&r103_em0a_voice_actor(voice)&&
    state==0x0101&&!actor[0x2f6]&&!actor[0x2f7]&&!w(actor+0x5b4)&&
    w(actor+0x17cc)==0x43160000u;
 const int east_rear=room==0x102&&model->root.size>=0x17d0&&
    model->root.original_vtable214==0x4479c8&&h(actor+0x2fe)==0x202&&w(actor+0x564)==0x200&&
    h(actor+0x640)==3&&actor[0x1650]==1&&state==1&&actor[0x2f6]==2&&!actor[0x2f7]&&
    w(actor+0x17cc)==0x43160000u;
 if(!player&&!gate_guard&&!arena_stun&&!east_rear)return 0;
 return w(row)==0x008c0000u&&h(row+4)==91&&w(row+6)==0x024b020eu&&
    h(row+0xa)==0&&h(row+0xc)==0&&h(row+0xe)==0x20&&w(row+0x10)==0x00460046u&&
    w(row+0x14)==0x005a0040u&&!w(row+0x18)&&w(row+0x1c)==0x00010000u&&w(row+0x20)==5&&
    w(row+0x24)==1&&w(row+0x28)==0xffffu&&!w(row+0x2c)&&
    w(sample)==0x80ad0u&&w(sample+4)==14000&&w(sample+8)==0x5fc080ffu&&!w(sample+12);
}

static int player_fall_grunt_profile(const struct voice *v,int16_t bank,int16_t program,
 const uint8_t *p,const uint8_t *sample)
{
 if(!player_fall_voice_model(v)||v->requested_bank||v->requested_program!=0xd7||
    bank!=3||program<15||program>17||h(p+4)!=(unsigned)(program-1)||w(p+6)!=0x0055000fu||
    h(p+0xe)!=0x21||w(p+0x10)!=0x007f007fu||w(p+0x14)!=0x005a0040u||
    w(p+0x18)||w(p+0x1c)!=0x00ff0000u||w(p+0x20)!=(program==15?0x205u:5u)||
    w(p+0x24)||w(p+0x28)!=0xffffu||w(p+0x2c))return 0;
 static const uint32_t offsets[]={0xf4f0,0x103b0,0x11900};
 return w(sample)==offsets[program-15]&&w(sample+4)==11025&&
    w(sample+8)==0x5fcb80ffu&&!w(sample+12);
}

static int em00_counter_voice_model(const struct voice *v)
{
 const gh_actor_model *model=v->model;
 if(!v->source_owner||v->part_index!=-1||!model||!model->work||
    !model->root.bytes||model->root.size<0x642||model->root.original_vtable214!=0x4479c8)
  return 0;
 const uint8_t *actor=model->root.bytes;
 const unsigned stamp=h(actor+0x640);
 const int opening=model->work->room==0x100&&stamp==20;
 const int story=model->work->room==0x102&&model->root.size>0x1650&&
    ((stamp==11&&actor[0x1650]==5)||(stamp==3&&actor[0x1650]==1)||
     (stamp==7&&model->root.size>=0x16c8&&!actor[0x1650]&&w(actor+0x16c4)==2))&&
    actor[0x2f6]==1&&actor[0x2f7]==1;
 const int ordinary=(opening||story)&&h(actor+0x2fe)==0x202&&w(actor+0x564)==0x200;
 return (ordinary||r103_em00_voice_actor(v))&&h(actor+0x2f4)==0x1d01;
}

static int paired_grab_player_voice_model(const struct voice *v)
{
 if(!player_fall_voice_model(v)||v->part_index!=-1||!v->model->work||
    h(v->model->root.bytes+0x2f4)!=0x0a01)return 0;
 const uint16_t room=v->model->work->room;
 return room==0x100||room==0x102||room==0x103||room==0x105;
}

static int r102_player_weapon_impact_source(const struct voice *v)
{
 if(!v->source_owner||v->part_index!=-1)return 0;
 if(!v->model)return v->combat_profile;
 const gh_title_player_view *root=&v->model->root;
 return v->model->work&&
    (v->model->work->room==0x102||(v->model->work->room==0x100&&
       (v->requested_program==0x142||v->requested_program==0x137)))&&
    root->bytes&&root->size>=0x300&&root->original_vtable214==0x41d220&&
    h(root->bytes+0x2fe)==0x100;
}

/* CORE 11D and 142 are original OM363 impact rows for the same ADPCM. */
static int core_r102_weapon_contact(const struct voice *v,int16_t bank,int16_t program,
 const uint8_t *p,const uint8_t *sample)
{
 if(bank||v->requested_bank||v->requested_program!=program||
    (program!=0x11d&&program!=0x142)||!r102_player_weapon_impact_source(v))return 0;
 const uint32_t row=program==0x11d?0x0406008fu:0x040600a7u;
 const uint32_t gain=program==0x11d?0x006e006eu:0x007f007fu;
 return w(p)==((uint32_t)program<<16)&&w(p+4)==row&&
        w(p+8)==0x50u&&w(p+12)==0x00200000u&&w(p+16)==gain&&
        w(p+20)==0x005a0040u&&w(p+24)==0x0100ff00u&&w(p+28)==0x00060000u&&
        w(p+32)==0x00030005u&&w(p+36)==1&&w(p+40)==0xffffu&&!w(p+44)&&
        (!sample||(w(sample)==0xcbcc0u&&w(sample+4)==20000&&
                   w(sample+8)==0x5fc080ffu&&!w(sample+12)));
}

static int core_close_impact_profile(const struct voice *v,const uint8_t *p)
{
 if(v->part_index!=-1||v->requested_bank||v->info.bank||
    v->requested_program!=v->info.program)return 0;
 const int player=player_fall_voice_model(v)&&v->model->work;
 const uint8_t *boss=r103_boss_voice_actor(v);
 /* Original 23D7D0 plays CORE F6 from the actor that deflected the hit. */
 const int boss_deflect=boss&&h(boss+0x2f4)==0x6f00&&!boss[0x2f6]&&!boss[0x2f7];
 if(!player&&!boss_deflect)return 0;
 if(player&&v->model->work->room==0x103&&v->info.program==0xf5)
  return w(p)==0x00f50000u&&w(p+4)==0x04060077u&&w(p+8)==0x50&&
    w(p+12)==0x00210000u&&w(p+16)==0x006e006eu&&w(p+20)==0x005a0040u&&
    w(p+24)==0x0000ff00u&&w(p+28)==0x00ff0000u&&w(p+32)==5&&
    w(p+36)==0x300&&w(p+40)==0xffff&&!w(p+44);
 return v->info.program==0xf6&&
    (boss_deflect||(player&&(v->model->work->room==0x100||v->model->work->room==0x102||
                            v->model->work->room==0x103)))&&
    h(p+4)==120&&w(p+6)==0x00500406u&&
    h(p+0xe)==0x21&&w(p+0x10)==0x007f0078u&&w(p+0x14)==0x005a0040u&&
    !w(p+0x18)&&!h(p+0x1c)&&w(p+0x20)==0x45u&&
    (w(p+0x24)&0x00ffffffu)==0x00000300u&&w(p+0x28)==0xffffu&&!w(p+0x2c);
}

static int r103_boss_throw_player_voice_model(const struct voice *v)
{
 if(!player_fall_voice_model(v)||v->part_index!=-1||!v->model->work||
    v->model->work->room!=0x103)return 0;
 const uint8_t *actor=v->model->root.bytes;
 return h(actor+0x2f4)==0x0f01&&actor[0x2f6]==1&&actor[0x2f7]==1;
}

static int core_fall_impact_profile(const struct voice *v,const uint8_t *p)
{
 const int16_t program=v->info.program;
 const int throw_impact=program==0x121&&r103_boss_throw_player_voice_model(v);
 return (throw_impact||(program==0x123&&(player_fall_voice_model(v)||em00_counter_voice_model(v))))&&
    !v->requested_bank&&v->requested_program==program&&!v->info.bank&&
    h(p+4)==(throw_impact?147u:149u)&&w(p+6)==0x00640100u&&
    h(p+0xe)==0x23&&w(p+0x10)==0x00780078u&&w(p+0x14)==0x005a0040u&&
    w(p+0x18)==0x0080ff80u&&w(p+0x1c)==0x00ff0000u&&w(p+0x20)==0x00060005u&&
    !w(p+0x24)&&w(p+0x28)==0xffffu&&!w(p+0x2c);
}

static int r103_hurt5_impact_profile(const struct voice *v,const uint8_t *p,const uint8_t *sample)
{
 const int16_t program=v->info.program;
 if(!player_fall_voice_model(v)||v->part_index!=-1||!v->model->work||
    v->model->work->room!=0x103||h(v->model->root.bytes+0x2f4)!=0x0501||
    v->model->root.bytes[0x2f6]||v->model->root.bytes[0x2f7]||
    v->requested_bank||v->info.bank)return 0;
 const int second=v->requested_program==0x32;
 if(second?(program!=0x32&&program!=0x33):(v->requested_program!=0x2d||program!=0x2d))
  return 0;
 const unsigned alternate=program==0x33;
 return w(p)==((uint32_t)program<<16)&&w(p+4)==(second?0x01260023u+alternate:0x010f0022u)&&
    w(p+8)==(second?0x68u:0x268u)&&w(p+12)==0x00210000u&&
    w(p+16)==(second?0x006e006eu:0x005a005au)&&w(p+20)==0x005a0040u&&
    w(p+24)==(second?0x0080ff80u:0)&&w(p+28)==0x00ff0000u&&
    w(p+32)==(program==0x32?0x00030105u:0x00030005u)&&!w(p+36)&&w(p+40)==0xffffu&&!w(p+44)&&
    (!sample||(w(sample)==(second?0x6a360u:0x62fc0u)&&w(sample+4)==18000&&
     w(sample+8)==0x5fc080ffu&&!w(sample+12)));
}

static int r103_grab_impact_profile(const struct voice *v,const uint8_t *p)
{
 if(!player_fall_voice_model(v)||v->part_index!=-1||!v->model->work||
    v->model->work->room!=0x103||v->requested_bank||v->requested_program!=0x11c||
    v->info.bank||v->info.program!=0x11c)return 0;
 const uint8_t *actor=v->model->root.bytes;
 const int em0a=h(actor+0x2f4)==0x0801&&actor[0x2f6]==3&&!actor[0x2f7];
 return (em0a||r103_boss_throw_player_voice_model(v))&&
    w(p)==0x011c0000u&&w(p+4)==0x0100008eu&&w(p+8)==0x264&&
    w(p+12)==0x00220000u&&w(p+16)==0x006e006eu&&w(p+20)==0x005a0040u&&
    !w(p+24)&&!w(p+28)&&w(p+32)==0x00060005u&&!w(p+36)&&
    w(p+40)==0xffff&&!w(p+44);
}

static int core_grab_impact_profile(const struct voice *v,const uint8_t *p)
{
 return (em00_counter_voice_model(v)||paired_grab_player_voice_model(v)||
    r103_boss_throw_player_voice_model(v))&&
    !v->requested_bank&&v->requested_program==0x134&&
    !v->info.bank&&v->info.program==0x134&&h(p+4)==163&&w(p+6)==0x01640100u&&
    p[0x9]==1&&h(p+0xe)==0x23&&w(p+0x10)==0x00780078u&&w(p+0x14)==0x010e0040u&&
    w(p+0x18)==0x01000100u&&w(p+0x1c)==0x00ff0000u&&w(p+0x20)==0x00060005u&&
    !w(p+0x24)&&w(p+0x28)==0xffffu&&!w(p+0x2c);
}
/* The player's original request is CORE 6E. The original RNG selects one of nine
 * program rows before the pending 308DA8 line test, even in mode zero. */
static int player_hurt_profile(const struct voice *v,int16_t bank,int16_t program,
 const uint8_t *p,const uint8_t *sample)
{
 if(!player_fall_voice_model(v)||v->part_index!=-1||v->requested_bank||
    v->requested_program!=0x6e||v->variants!=8||bank||program<0x6e||program>0x76)return 0;
 static const uint16_t pitch_min[]={0xff80,0,0,0,0xff80,0xff80,0,0xff80,0};
 static const uint32_t sample_offset[]={0x54300,0x56490,0x78450,0x82bc0,0x841f0,0x854b0,0x86830,0x878b0,0x88600};
 const unsigned variant=(unsigned)(program-0x6e);
 return !h(p)&&h(p+2)==(unsigned)program&&h(p+4)==0x48+variant&&
    w(p+6)==0x00500406u&&h(p+0xe)==0x21&&w(p+0x10)==0x007f007fu&&
    w(p+0x14)==0x005a0040u&&w(p+0x18)==(0x00800000u|pitch_min[variant])&&
    w(p+0x1c)==0x00ff0000u&&w(p+0x20)==(variant?4u:0x804u)&&w(p+0x24)==1&&
    w(p+0x28)==0xffffu&&!w(p+0x2c)&&w(sample)==sample_offset[variant]&&
    w(sample+4)==22050&&w(sample+8)==0x5fc080ffu&&!w(sample+12);
}

/* Original 25E790 voice for the two R100 opening enemies. Actor bank1
 * resolves to EM03.SES (stamp19) or EM00.SES (stamp20). */
static int enemy_death_cue_profile(const struct voice *v,int16_t bank,int16_t program,
 const uint8_t *p,const uint8_t *sample)
{
 if(!v->source_owner||v->requested_program!=8||bank<1||bank==2||bank==3||
    (program!=8&&program!=9)||v->part_index!=-1)return 0;
 uint32_t dependency=0;
 int gate_em0a=0;
 if(v->model){
  const gh_title_player_view *root=&v->model->root;
  if(!root->bytes||root->size<0x642||root->original_vtable214!=0x4479c8||!v->model->work)return 0;
  const unsigned stamp=h(root->bytes+0x640);
  dependency=w(root->bytes+0x564);
  const int room=v->model->work->room;
  const int em202=h(root->bytes+0x2fe)==0x202;
  const int opening=room==0x100&&em202&&
      ((stamp==19&&dependency==0x203)||(stamp==20&&dependency==0x200));
  const int r102=room==0x102&&em202&&dependency==0x200&&root->size>0x1650&&
      h(root->bytes+0x2f4)==2&&!h(root->bytes+0x54a)&&
      ((stamp==0x10&&!root->bytes[0x1650])||(stamp==11&&root->bytes[0x1650]==5));
  const int arena=(r103_em00_voice_actor(v)||r103_em03_voice_actor(v)||r103_em0a_voice_actor(v))&&
      !h(root->bytes+0x54a)&&
      (v->combat_profile||(h(root->bytes+0x2f4)==0x0102&&root->bytes[0x2f6]==0));
  const uint8_t *em0a=r102_em0a_voice_actor(v);
  gate_em0a=em0a&&h(em0a+0x2f4)==0x0102&&em0a[0x2f6]<=1&&!em0a[0x2f7]&&!h(em0a+0x54a);
  const uint8_t *em03=r102_voice_actor(v);
  const int r102_em03=em03&&dependency==0x203&&stamp==8&&!em03[0x1650]&&h(em03+0x2f4)==2&&!h(em03+0x54a);
  const int boss=r103_boss_voice_actor(v)&&!h(root->bytes+0x54a)&&
      (v->combat_profile||(h(root->bytes+0x2f4)==0x0102&&!root->bytes[0x2f6]));
  const uint8_t *actor=root->bytes;
  const int west_formation=root->size>=0x16c8&&w(actor+0x16c4)==6&&actor[0x1650]==2&&
      ((stamp==3&&dependency==0x200)||(stamp==4&&dependency==0x202)||(stamp==5&&dependency==0x203));
  const int west_wave=root->size>=0x16c8&&w(actor+0x16c4)==2&&!actor[0x1650]&&
      ((stamp==29&&dependency==0x202)||(stamp==30&&dependency==0x203));
  const int at3=root->size>=0x16c8&&!w(actor+0x16c4)&&!actor[0x1650]&&
      ((stamp==32&&dependency==0x202)||(stamp==31&&dependency==0x200));
  const int north=root->size>=0x16c8&&w(actor+0x16c4)==6&&actor[0x1650]==5&&
      stamp==11&&dependency==0x203;
  const int west=room==0x100&&em202&&(west_formation||west_wave||at3||north)&&
      h(actor+0x2f4)==0x0102&&!h(actor+0x54a);
  if(!opening&&!r102&&!arena&&!gate_em0a&&!r102_em03&&!boss&&!west)return 0;
 }else if(!v->combat_profile)return 0;
 if((dependency==0x202||!v->model)&&w(p+24)==0x0064ff38&&h(p+4)==(unsigned)(program-2))
  return w(p+4)==(0x012e0000u|(unsigned)(program-2))&&w(p+8)==0x25f&&
     w(p+12)==0x00200000u&&w(p+16)==(program==8?0x007d007du:0x00730073u)&&
     w(p+20)==0x005a0040u&&w(p+24)==0x0064ff38u&&!w(p+28)&&
     w(p+32)==(program==8?0x00010145u:0x00010045u)&&w(p+36)==0x400&&w(p+40)==0xffff&&!w(p+44)&&
     (!sample||(w(sample)==(program==8?0x6cf0u:0x8ae0u)&&w(sample+4)==11025&&
               w(sample+8)==0x5fcb80ffu&&!w(sample+12)));
 if(gate_em0a||dependency==0x20a||(!v->model&&!w(p+24)))
  return w(p+4)==(program==8?0x012e0006u:0x012e0007u)&&w(p+8)==0x25f&&
     w(p+12)==0x00200000u&&w(p+16)==0x007d007du&&w(p+20)==0x005a0040u&&
     !w(p+24)&&!w(p+28)&&w(p+32)==(program==8?0x00010145u:0x00010045u)&&
     w(p+36)==0x400&&w(p+40)==0xffff&&!w(p+44)&&
     (!sample||(w(sample)==(program==8?0x10160u:0x122f0u)&&
               w(sample+4)==11025&&w(sample+8)==0x5fcb80ffu&&!w(sample+12)));
 if(w(p+4)!=(program==8?0x012e0007u:0x012e0008u)||w(p+8)!=0x25f||
    h(p+12)||h(p+14)!=0x20||w(p+20)!=0x005a0040||w(p+24)!=0x0064ff38||w(p+28)||
    w(p+32)!=(program==8?0x00010145u:0x00010045u)||w(p+36)!=0x400||
    w(p+40)!=0xffff||w(p+44))return 0;
 if(dependency==0x20a||(!dependency&&!w(p+24)))
  return w(p+4)==(program==8?0x012e0006u:0x012e0007u)&&w(p+16)==0x007d007du&&!w(p+24)&&
    (!sample||(w(sample)==(program==8?0x10160u:0x122f0u)&&w(sample+4)==11025&&
      w(sample+8)==0x5fcb80ffu&&!w(sample+12)));
 if(w(p+4)!=(program==8?0x012e0007u:0x012e0008u)||w(p+24)!=0x0064ff38)return 0;
 const uint32_t gain=w(p+16);
 const uint32_t em03=program==8?0xa550u:0xbd40u;
 const uint32_t em00=program==8?0x7b40u:0xa4c0u;
 const int profile03=dependency!=0x200&&gain==(program==8?0x005f005fu:0x007d007du)&&
    (!sample||w(sample)==em03);
 const int profile00=dependency!=0x203&&gain==0x00780078u&&(!sample||w(sample)==em00);
 /* EM14.SES supplies both arena bosses, with its own death recordings. */
 const int profile14=(!dependency||dependency==0x214||dependency==0x215)&&
     gain==(program==8?0x00780078u:0x007d007du)&&
     (!sample||w(sample)==(program==8?0x147b0u:0x17120u));
 return (profile03||profile00||profile14)&&(!sample||
    (w(sample+4)==11025&&w(sample+8)==0x5fcb80ff&&!w(sample+12)));
}

/* Original EMAD.SES cue from cEmAD Move in R102 and R105. */
static int emad_room_loop(const struct voice *v,int16_t bank,int16_t program,const uint8_t *p)
{
 const gh_actor_model *model=v->model;
 if(!v->source_owner||bank<1||bank==2||bank==3||(program!=0x12&&program!=0)||v->requested_program!=program||
    v->part_index!=-1||!model||!model->work||
    (model->work->room!=0x105&&(model->work->room!=0x102||program!=0x12))||
    model->root.original_vtable214!=0x44a528||!model->root.bytes||model->root.size<0x1554)
  return 0;
 const uint8_t *actor=model->root.bytes;
 if(h(actor+0x2fe)!=0x2ad||w(actor+0x564)!=0x2ad||h(actor+0x640)!=63||
    h(p)!=(unsigned)bank||h(p+2)!=(unsigned)program)return 0;
 if(program==0)
  return w(p+4)==0x02300000&&w(p+8)==0x264&&w(p+0xc)==0x00220000&&
    w(p+0x10)==0x007f007f&&w(p+0x14)==0x005a0040&&!w(p+0x18)&&!w(p+0x1c)&&
    w(p+0x20)==4&&!w(p+0x24)&&!w(p+0x28)&&!w(p+0x2c);
 return w(p+4)==0x04140000&&w(p+8)==0x278&&
    w(p+0xc)==0x00200000&&w(p+0x10)==0x00370037&&w(p+0x14)==0x005a0040&&
    !w(p+0x18)&&w(p+0x1c)==0x00050000&&w(p+0x20)==0x00030004&&w(p+0x24)==0x302&&
    w(p+0x28)==0xffff&&!w(p+0x2c);
}

/* Original 2923E8 attaches CORE FA to the item created by 316540. */
static int item_drop_profile(const struct voice *v)
{
 const gh_actor_model *model=v->model;
 const uint8_t *p=v->descriptor;
 if(!v->source_owner||!model||!model->work||!model->root.bytes||model->root.size<0x690||
    model->root.original_vtable214!=0x429b88||v->part_index!=-1||
    v->requested_bank||v->info.bank||v->info.program!=v->requested_program)return 0;
 const unsigned kind=h(model->root.bytes+0x2fe);
 const int16_t room=model->work->room;
 const unsigned program=(unsigned)v->requested_program;
 const int drop=program==0xfa&&(((room==0x101||room==0x102||room==0x103)&&kind==0x3d6)||
    ((room==0x102||room==0x103)&&kind==0x3c4)||
    (room==0x103&&(kind==0x3d0||kind==0x3c8||kind==0x3d1||kind==0x3d7))||(room==0x102&&kind==0x3df)||(room==0x101&&kind==0x3e3)||
    (room==0x100&&(kind==0x3c4||kind==0x3c8||kind==0x3d6||kind==0x3e0||kind==0x3e1)));
 const int collect=program==0xfc&&room==0x102&&kind==0x3df&&model->root.bytes[0x2f6]==4;
 if(!drop&&!collect)return 0;
 return !h(p)&&h(p+2)==program&&h(p+4)==(collect?0x7cu:0x7au)&&
    w(p+6)==(collect?0x00780104u:0x02780104u)&&!w(p+0xa)&&
    h(p+0xe)==0x23&&w(p+0x10)==0x00780078&&w(p+0x14)==0x005a0040&&
    !w(p+0x18)&&w(p+0x1c)==0x00ff0000&&w(p+0x20)==0x00030004&&
    !w(p+0x24)&&w(p+0x28)==0xffff&&!w(p+0x2c);
}

static int scene_door_open_profile(const struct voice *v);
static int r103_roulette_windup_cue(const struct voice *voice);
static unsigned room_prop_break_profile(const struct voice *v);

static int r105_gate_loop_descriptor(const uint8_t *p)
{
 return w(p)==0x00100002u&&w(p+4)==0x00270005u&&w(p+8)==0x0000027cu&&
    w(p+12)==0x00200000u&&w(p+16)==0x00410041u&&w(p+20)==0x005a0040u&&
    !w(p+24)&&!w(p+28)&&w(p+32)==0x00030084u&&w(p+36)==0x300u&&
    w(p+40)==0xffffu&&!w(p+44);
}

static int r105_gate_close_descriptor(const uint8_t *p)
{
 return w(p)==0x00110002u&&w(p+4)==0x00170006u&&w(p+8)==0x0000025au&&
    w(p+12)==0x00200000u&&w(p+16)==0x00640078u&&w(p+20)==0x005a0040u&&
    !w(p+24)&&!w(p+28)&&w(p+32)==0x00030044u&&w(p+36)==0x300u&&
    w(p+40)==0xffffu&&!w(p+44);
}

static int boost_impact_voice(const struct voice *v,int16_t bank,int16_t program,
 const uint8_t *p,const uint8_t *sample)
{
 if(!v->source_owner||v->requested_bank||v->requested_program!=0x121||bank||program!=0x121||v->part_index!=-1)
  return 0;
 if(v->model){
  const uint8_t *at0b=r102_voice_actor(v);
  const int local=at0b&&v->model->root.size>=0x16c8&&
      ((w(at0b+0x564)==0x200&&h(at0b+0x640)==7)||(w(at0b+0x564)==0x202&&h(at0b+0x640)==0x17))&&
      !at0b[0x1650]&&w(at0b+0x16c4)==2&&h(at0b+0x2f4)==1&&!at0b[0x2f7]&&
      (!at0b[0x2f6]||(v->combat_profile&&at0b[0x2f6]==1));

  const uint8_t *arena_boss=r103_boss_voice_actor(v);
  const uint8_t *actor=v->model->root.bytes;
  const uint32_t dependency=arena_boss?w(arena_boss+0x564):0;
  const int heat_receiver=r103_em00_voice_actor(v)||arena_boss;
  const uint32_t power=heat_receiver&&v->model->root.size>=0x17b8?w(actor+0x17b4):0;
  const int heat_contact=(power==12&&arena_boss)||power==5;
  const int standing_hit=heat_contact&&h(actor+0x2f4)==0x0101;
  const int em15_square=dependency==0x215&&power==3&&h(actor+0x2f4)==0x0001;
  const int active_impact=(standing_hit||em15_square)&&
      (!actor[0x2f6]||(v->combat_profile&&actor[0x2f6]==1))&&!actor[0x2f7];
  const int west_scenery=v->model->work&&v->model->work->room==0x100&&actor&&
      v->model->root.original_vtable214==0x4479c8&&v->model->root.size>=0x17b8&&
      h(actor+0x2fe)==0x202&&w(actor+0x564)==0x203&&h(actor+0x640)==11&&actor[0x1650]==5&&
      w(actor+0x16c4)==6&&h(actor+0x2f4)==0x1101&&!actor[0x2f6]&&!actor[0x2f7]&&!w(actor+0x17b4);
  if(!west_scenery&&!active_impact&&!local&&!r102_east_em27_voice(v,0x1101,0)&&
     !(v->combat_profile&&r102_east_em27_voice(v,0x1101,1)))return 0;
 }else if(!v->combat_profile)return 0;
 return w(p)==0x01210000u&&w(p+4)==0x01000093u&&w(p+8)==0x64u&&
    w(p+12)==0x00230000u&&w(p+16)==0x00780078u&&w(p+20)==0x005a0040u&&
    w(p+24)==0x0080ff80u&&w(p+28)==0x00ff0000u&&w(p+32)==0x00060005u&&
    !w(p+36)&&w(p+40)==0xffffu&&!w(p+44)&&
    (!sample||(w(sample)==0x176d0u&&w(sample+4)==20000&&w(sample+8)==0x5fc080ffu&&!w(sample+12)));
}

/* The active PL00 aura emits CORE 3D and the six-tick CORE 11F cue. */
static int player_active_heat_voice(const struct voice *v,int16_t bank,int16_t program,
 const uint8_t *p,const uint8_t *sample)
{
 const gh_actor_model *model=v->model;
 if(!v->source_owner||!model||!model->work||!model->root.bytes||model->root.size<0x15f8||
    (model->work->room!=0x100&&model->work->room!=0x102&&model->work->room!=0x103)||
    model->root.original_vtable214!=0x41d220||
    h(model->root.bytes+0x2fe)!=0x100||h(model->root.bytes+0x5f2)!=3||
    (w(model->root.bytes+0x15f4)&0x20u)||bank||
    v->requested_bank||v->requested_program!=program||v->part_index!=-1)return 0;
 if(program==0x3d)
  return w(p)==0x003d0000u&&w(p+4)==0x01140029u&&w(p+8)==0x25au&&w(p+12)==0x00230000u&&
    w(p+16)==0x00780078u&&w(p+20)==0x005a0040u&&w(p+24)==0x00800080u&&
    w(p+28)==0x00ff0000u&&w(p+32)==4&&!w(p+36)&&w(p+40)==0xffffu&&!w(p+44)&&
    (!sample||(!w(sample)&&w(sample+4)==22050&&w(sample+8)==0x5fc080ffu&&!w(sample+12)));
 if(program!=0x11f)return 0;
 return w(p)==0x011f0000u&&w(p+4)==0x02040091u&&w(p+8)==0x6eu&&w(p+12)==0x00230000u&&
    w(p+16)==0x00780078u&&w(p+20)==0x005a0040u&&w(p+24)==0x0080ff80u&&
    w(p+28)==0x00ff0000u&&w(p+32)==0x00060005u&&!w(p+36)&&w(p+40)==0xffffu&&!w(p+44)&&
    (!sample||(w(sample)==0x3beb0u&&w(sample+4)==16000&&w(sample+8)==0x5fc080ffu&&!w(sample+12)));
}

/* PL00 finish clip 78460 submits its original attached CORE29 and 12D. */
static int player_heat_finish_voice(const struct voice *v,int16_t bank,int16_t program,
 const uint8_t *p,const uint8_t *sample)
{
 const gh_actor_model *m=v->model;
 if(!v->source_owner||!m||!m->work||!m->root.bytes||m->root.size<=0x1684||
    (m->work->room!=0x100&&m->work->room!=0x102&&m->work->room!=0x103)||
    m->root.original_vtable214!=0x41d220||
    h(m->root.bytes+0x2fe)!=0x100||w(m->root.bytes+0x2f4)!=0x00010900u||
    m->root.bytes[0x1684]!=1||bank||v->requested_bank||v->requested_program!=program||
    v->part_index!=-1)return 0;
 if(w(p+20)!=0x005a0040u||w(p+32)!=5||w(p+40)!=0xffffu||w(p+44))return 0;
 if(program==0x29)
  return w(p)==0x00290000u&&w(p+4)==0x020b001eu&&w(p+8)==0x46u&&
    w(p+12)==0x00100000u&&w(p+16)==0x00500050u&&w(p+24)==0x00800080u&&
    w(p+28)==0x00010000u&&w(p+36)==0x300u&&
    (!sample||(w(sample)==0x95340u&&w(sample+4)==16000&&w(sample+8)==0x5fc080ffu&&!w(sample+12)));
 if(program==0x12d)
  return w(p)==0x012d0000u&&w(p+4)==0x020b009cu&&w(p+8)==0x78u&&
    w(p+12)==0x00130000u&&w(p+16)==0x007f007fu&&w(p+24)==0x00c800c8u&&
    w(p+28)==0x00ff0000u&&!w(p+36)&&
    (!sample||(w(sample)==0x49880u&&w(sample+4)==22050&&w(sample+8)==0x5fc080ffu&&!w(sample+12)));
 return 0;
}

static int r103_roulette_motion_voice_profile(const struct voice *voice,
    int16_t bank,int16_t program,const uint8_t *row,const uint8_t *sample)
{
 const gh_actor_model *model=voice->model;
 if(!voice->source_owner||bank!=3||program<0x1a||program>0x1c||
    voice->requested_bank||voice->requested_program!=program+200||voice->part_index!=-1||
    !model||!model->work||model->work->room!=0x103||!model->root.bytes||
    model->root.size<0x300||model->root.original_vtable214!=0x41d220||
    h(model->root.bytes+0x2fe)!=0x100||h(model->root.bytes+0x2f4)!=0x2300)
  return 0;
 /* PL00 motion 0450 asks for CORE E2/E3/E4. Original audio remapping
  * resolves these to the WAVE table in PL00 rows 1A/1B/1C. */
 static const uint32_t offsets[]={0x12c80,0x13da0,0x18730};
 const unsigned i=(unsigned)(program-0x1a);
 return w(row)==((uint32_t)program<<16|3u)&&w(row+4)==0x000f0017u+i&&
    w(row+8)==(i==2?0x28u:0x50u)&&w(row+12)==0x00210000u&&
    w(row+16)==(i==2?0x007d007du:0x007f007fu)&&w(row+20)==0x005a0040u&&
    !w(row+24)&&w(row+28)==0x00ff0000u&&w(row+32)==5&&!w(row+36)&&
    w(row+40)==0xffffu&&!w(row+44)&&w(sample)==offsets[i]&&w(sample+4)==11025&&
    w(sample+8)==0x5fcb80ffu&&!w(sample+12);
}

static int player_evade_award_voice(const struct voice *voice,int16_t bank,int16_t program,
 const uint8_t *row,const uint8_t *sample)
{
 const gh_actor_model *model=voice->model;
 if(!voice->source_owner||voice->requested_bank||voice->part_index!=-1||
    !model||!model->root.bytes||model->root.size<0x300||model->root.original_vtable214!=0x41d220||
    h(model->root.bytes+0x2fe)!=0x100||w(row+20)!=0x005a0040u||w(row+28)!=0x00ff0000u||
    w(row+36)||w(row+40)!=0xffffu||w(row+44)||w(sample+12))return 0;
 /* 129430 requests CORE EE. The original bank remap and one MT draw select
  * PL00 row 0x26 or 0x27, separate samples with the same voice policy. */
 if(bank==3&&voice->requested_program==0xee&&(program==0x26||program==0x27)){
  const int alternate=program==0x27;
  return w(row+4)==(alternate?0x000f001fu:0x000f001eu)&&w(row+8)==0x41&&
      w(row+12)==0x00210000u&&w(row+16)==0x00730073u&&!w(row+24)&&
      w(row+32)==(alternate?5u:0x105u)&&w(sample)==(alternate?0x1c890u:0x1baf0u)&&
      w(sample+4)==11025&&w(sample+8)==0x5fcb80ffu;
 }
 /* The fifth consecutive evade also requests the original CORE 36 cue. */
 return bank==0&&program==0x36&&voice->requested_program==0x36&&
     w(row+4)==0x01260026u&&w(row+8)==0x68&&w(row+12)==0x00210000u&&
     w(row+16)==0x006e006eu&&w(row+24)==0x0080ff80u&&w(row+32)==0x00030005u&&
     w(sample)==0x58c80u&&w(sample+4)==16000&&w(sample+8)==0x5fc080ffu;
}

static gh_voice_status prepare_pcm_with_pitch(gh_se_voice_engine*s,int16_t bank,int16_t program,const uint8_t*p,struct voice*candidate,uint16_t selected_pitch){
 gh_audio_pcm_release(&candidate->pcm);
 unsigned slot=h(p+4),rate,level;gh_sound_span descriptor,tail;gh_voice_status result;
 slot=h(p+4);if(gh_audio_sample_at(s->registry,bank,slot,&descriptor,&tail)!=GH_AUDIO_OK)return unsupported(s,0x35fc);
 if(descriptor.size!=16)return unsupported(s,0x35fc);
 uint16_t adsr1=(uint16_t)h(descriptor.data+8),adsr2=(uint16_t)h(descriptor.data+10);
 int square_voice=candidate->source_owner&&bank==3&&program>=1&&program<=3&&adsr2==0x5fcb;
 const gh_actor_model *heat_model=candidate->model;
 const uint8_t *heat_player=heat_model&&heat_model->root.size>0x1684?heat_model->root.bytes:NULL;
 const int heat_activation=heat_player&&heat_model->work&&
    (heat_model->work->room==0x100||heat_model->work->room==0x102||heat_model->work->room==0x103)&&
    (h(heat_player+0x2f4)==0x0800||h(heat_player+0x2f4)==0x0900)&&heat_player[0x2f6]==1&&!heat_player[0x2f7]&&heat_player[0x1684]==1;
 const int boss_throw=candidate->model&&candidate->model->work&&
    candidate->model->work->room==0x103&&candidate->model->root.bytes&&candidate->model->root.size>=0x300&&
    h(candidate->model->root.bytes+0x2f4)==0x0f01&&candidate->model->root.bytes[0x2f6]==1&&
    candidate->part_index==-1;
 const int boss_deflect_reaction=candidate->part_index==-1&&player_fall_voice_model(candidate)&&
    candidate->model->work&&candidate->model->work->room==0x103&&
    h(candidate->model->root.bytes+0x2f4)==0x0201&&candidate->model->root.bytes[0x2f6]==1&&
    !candidate->model->root.bytes[0x2f7];
 const int hurt_program_nine=program==9&&(boss_throw||boss_deflect_reaction);
 const int hurt_program_six=program==6&&candidate->part_index==-1&&candidate->model&&
    candidate->model->root.bytes&&candidate->model->root.size>=0x300&&
    (heat_activation||boss_throw||h(candidate->model->root.bytes+0x2f4)==0x0b01||
     (h(candidate->model->root.bytes+0x2f4)==0x0001&&!candidate->model->root.bytes[0x2f6]))&&
    w(descriptor.data+8)==0x5fcb80ffu&&
    !w(descriptor.data+12);
 int hurt_voice=adsr2==0x5fcb&&candidate->source_owner&&bank==3&&(hurt_program_six||program==7||program==8||hurt_program_nine)&&
    candidate->requested_bank==0&&candidate->requested_program==program+200&&
    candidate->model&&candidate->model->root.size>=0x300&&
    candidate->model->root.original_vtable214==0x41d220&&
    h(candidate->model->root.bytes+0x2fe)==0x100&&h(p+4)==(unsigned)program&&
    w(p+6)==0x0055000f&&h(p+0xe)==0x21&&w(p+0x10)==0x007f007f&&
    w(p+0x14)==0x005a0040&&!w(p+0x18)&&w(p+0x1c)==0x00ff0000&&
    w(p+0x20)==5&&!w(p+0x24)&&
    w(descriptor.data)==(hurt_program_six?52464u:hurt_program_nine?0xeaf0u:program==7?54128u:57568u)&&w(descriptor.data+4)==11025;
 /* Original PL00 Heat activation and the borrowed EM02 escape both use CORE C8. */
 const int escape_voice=adsr2==0x5fcb&&candidate->source_owner&&bank==3&&!program&&
    !candidate->requested_bank&&candidate->requested_program==0xc8&&candidate->part_index==-1&&
    candidate->model&&candidate->model->work&&
    (candidate->model->work->room==0x100||candidate->model->work->room==0x102||candidate->model->work->room==0x103)&&
    candidate->model->root.size>=0x300&&candidate->model->root.original_vtable214==0x41d220&&
    h(candidate->model->root.bytes+0x2fe)==0x100&&
    (heat_activation||h(candidate->model->root.bytes+0x2f4)==0x3900)&&
    w(p+4)==0x000f0000u&&w(p+8)==0x50u&&w(p+12)==0x00210000u&&
    w(p+16)==0x007f007fu&&w(p+20)==0x005a0040u&&!w(p+24)&&
    w(p+28)==0x00ff0000u&&w(p+32)==5&&!w(p+36)&&w(p+40)==0xffffu&&!w(p+44)&&
    w(descriptor.data)==18992u&&w(descriptor.data+4)==11025u;
 /* 122D18 and 122E90 share the attached CORE D6 to original PL00 row14. */
 const int player_death=adsr2==0x5fcb&&candidate->source_owner&&bank==3&&program==14&&
    candidate->requested_bank==0&&candidate->requested_program==0xd6&&candidate->part_index==-1&&
    candidate->model&&candidate->model->work&&
    (candidate->model->work->room==0x100||candidate->model->work->room==0x102||
     candidate->model->work->room==0x103||candidate->model->work->room==0x104||
     candidate->model->work->room==0x105)&&
    candidate->model->root.bytes&&candidate->model->root.size>=0x54c&&
    candidate->model->root.original_vtable214==0x41d220&&
    h(candidate->model->root.bytes+0x2fe)==0x100&&
    (h(candidate->model->root.bytes+0x2f4)==0x0102||
     h(candidate->model->root.bytes+0x2f4)==0x0002||
     r103_boss_throw_player_voice_model(candidate)||r103_hold_death_player_voice(candidate)||
     (candidate->model->work->room==0x100&&h(candidate->model->root.bytes+0x2f4)==0x0b01&&
      candidate->model->root.bytes[0x2f6]==5))&&
    !h(candidate->model->root.bytes+0x54a)&&
    w(p+4)==0x000f000d&&w(p+8)==0x55&&w(p+0xc)==0x00210000&&
    w(p+0x10)==0x00780078&&w(p+0x14)==0x005a0040&&!w(p+0x18)&&
    w(p+0x1c)==0x00ff0000&&w(p+0x20)==5&&!w(p+0x24)&&
    w(p+0x28)==0xffff&&!w(p+0x2c)&&
    w(descriptor.data)==36224&&w(descriptor.data+4)==11025;
 const uint8_t *player=candidate->model&&candidate->model->root.size>=0x5ec?
    candidate->model->root.bytes:NULL;
 const uint32_t skill=player?w(player+0x5e8):UINT32_MAX;
 /* The original player Square combo cues use CORE CC mapped to PL00 row 4. */
 const int player_cc=adsr2==0x5fcb&&candidate->source_owner&&bank==3&&program==4&&
    candidate->requested_bank==0&&candidate->requested_program==0xcc&&candidate->part_index==-1&&
    ((candidate->model&&candidate->model->work&&candidate->model->root.bytes&&
    candidate->model->root.size>=0x5ec&&candidate->model->root.original_vtable214==0x41d220&&
    h(candidate->model->root.bytes+0x2fe)==0x100&&h(candidate->model->root.bytes+0x2f4)==0x0b00&&
    ((candidate->model->work->room==0x100&&w(candidate->model->root.bytes+0x5e8)==29)||
     (candidate->model->work->room==0x101&&skill==0&&h(player+0x2f6)==1)||
     (candidate->model->work->room==0x102&&(skill==0||skill==4||skill==10||skill==13)&&
      h(candidate->model->root.bytes+0x2f6)==1)||
     (candidate->model->work->room==0x103&&(skill==0||skill==29))))||
    (player&&candidate->model->work&&candidate->model->work->room==0x103&&
     candidate->model->root.original_vtable214==0x41d220&&h(player+0x2fe)==0x100&&
     h(player+0x2f4)==0x2b00&&player[0x2f6]==1&&!player[0x2f7])||
    (candidate->model&&candidate->model->work&&candidate->model->work->room==0x101&&
    candidate->model->root.bytes&&candidate->model->root.size>=0x300&&
    candidate->model->root.original_vtable214==0x41d220&&
    h(candidate->model->root.bytes+0x2fe)==0x100&&h(candidate->model->root.bytes+0x2f4)==0x1600))&&
    w(p+4)==0x000f0004&&w(p+8)==0x50&&w(p+12)==0x00210000&&
    w(p+16)==0x007f007f&&w(p+20)==0x005a0040&&w(p+24)==0xff80ff80u&&
    w(p+28)==0x00ff0000&&w(p+32)==5&&!w(p+36)&&w(p+40)==0xffff&&!w(p+44)&&
    w(descriptor.data)==0x2440&&w(descriptor.data+4)==11025;
 /* Logical CORE CD maps to the original PL00 voice bank row 5. */
 int player_cd=adsr2==0x5fcb&&candidate->source_owner&&bank==3&&program==5&&
    candidate->requested_bank==0&&candidate->requested_program==0xcd&&
    candidate->part_index==-1&&candidate->model&&candidate->model->root.size>=0x300&&
    candidate->model->root.original_vtable214==0x41d220&&
    h(candidate->model->root.bytes+0x2fe)==0x100&&
    w(p+4)==0x000f0005&&w(p+8)==0x50&&w(p+12)==0x00210000&&
    w(p+16)==0x007f007f&&w(p+20)==0x005a0040&&w(p+24)==0xff00ff00u&&
    w(p+28)==0x00ff0000&&w(p+32)==5&&!w(p+36)&&w(p+40)==0xffff&&!w(p+44)&&
    w(descriptor.data)==0x3540&&w(descriptor.data+4)==11025;
 /* Skill29 SEQ9AD20 logical CORE F3 maps to PL00 row43. */
 /* Skill 52 SEQ140920 requests CORE E2, which maps to PL00 row 26. */
 const int back_circle_voice=adsr2==0x5fcb&&candidate->source_owner&&bank==3&&program==26&&
    candidate->requested_bank==0&&candidate->requested_program==0xe2&&candidate->part_index==-1&&
    candidate->model&&candidate->model->work&&candidate->model->work->room==0x103&&player&&
    candidate->model->root.original_vtable214==0x41d220&&h(player+0x2fe)==0x100&&
    h(player+0x2f4)==0x0b00&&skill==52&&
    w(p+4)==0x000f0017&&w(p+8)==0x50&&w(p+12)==0x00210000&&
    w(p+16)==0x007f007f&&w(p+20)==0x005a0040&&!w(p+24)&&
    w(p+28)==0x00ff0000&&w(p+32)==5&&!w(p+36)&&w(p+40)==0xffff&&!w(p+44)&&
    w(descriptor.data)==0x12c80&&w(descriptor.data+4)==11025;
 const int triangle_voice=adsr2==0x5fcb&&candidate->source_owner&&bank==3&&program==43&&
    candidate->requested_bank==0&&candidate->requested_program==243&&candidate->part_index==-1&&
    candidate->model&&candidate->model->root.size>=0x300&&
    candidate->model->root.original_vtable214==0x41d220&&
    h(candidate->model->root.bytes+0x2fe)==0x100&&
    w(p+4)==0x020b0021&&w(p+8)==0x64&&w(p+12)==0x00210000&&
    w(p+16)==0x00550055&&w(p+20)==0x005a0040&&w(p+24)==0x00800080&&
    w(p+28)==0x00ff0000&&w(p+32)==5&&!w(p+36)&&w(p+40)==0xffff&&!w(p+44)&&
    w(descriptor.data)==0x23fa0&&w(descriptor.data+4)==16000;
 const gh_actor_model *pickup_model=candidate->model;
 const unsigned pickup_room=pickup_model&&pickup_model->work?pickup_model->work->room:0;
 const int boost_or_heal_cue=program==0x17&&candidate->requested_program==0xdf&&player&&
    ((pickup_room==0x102&&pickup_model->root.size>=0x600&&w(player+0x5fc)==600)||pickup_room==0x103);
 const int boss_currency_cue=program==0x16&&candidate->requested_program==0xde&&pickup_room==0x103;
 /* R101 cOmItem 03E3 ordinary acquisition maps CORE D3 to PL00 row 11. */
 const int key_card_cue=program==11&&candidate->requested_program==0xd3&&pickup_room==0x101;
 const int currency_cue=program==12&&candidate->requested_program==0xd4&&
     (pickup_room==0x100||pickup_room==0x101||pickup_room==0x102||pickup_room==0x103);
 const int full_heal_cue=program==20&&candidate->requested_program==0xdc&&pickup_room==0x103;
 const int partial_heal_cue=program==10&&candidate->requested_program==0xd2&&
     (pickup_room==0x100||pickup_room==0x103);
 const int pickup_voice=adsr2==0x5fcb&&candidate->source_owner&&bank==3&&
    (boost_or_heal_cue||boss_currency_cue||currency_cue||full_heal_cue||partial_heal_cue||key_card_cue)&&candidate->requested_bank==0&&candidate->part_index==-1&&
    pickup_model->root.bytes&&
    pickup_model->root.size>=0x300&&pickup_model->root.original_vtable214==0x41d220&&
    h(pickup_model->root.bytes+0x2fe)==0x100&&w(p)==(((uint32_t)program<<16)|3u)&&
    h(p+4)==(boost_or_heal_cue?21u:boss_currency_cue?20u:full_heal_cue?18u:(unsigned)program)&&
    w(p+6)==0x0055000fu&&!w(p+0xa)&&h(p+0xe)==0x21&&w(p+0x10)==0x00730073u&&
    w(p+0x14)==0x005a0040u&&!w(p+0x18)&&w(p+0x1c)==0x00ff0000u&&w(p+0x20)==5&&
    !w(p+0x24)&&w(p+0x28)==0xffff&&!w(p+0x2c)&&
    w(descriptor.data)==(boost_or_heal_cue?0x21d30u:boss_currency_cue?0x20b50u:full_heal_cue?0x1ef30u:
        partial_heal_cue?0x1dc60u:key_card_cue?0x1c890u:0x1baf0u)&&
    w(descriptor.data+4)==11025;
 int fall_grunt=adsr2==0x5fcb&&player_fall_grunt_profile(candidate,bank,program,p,descriptor.data);
 if((core_fall_impact_profile(candidate,p)||core_grab_impact_profile(candidate,p))&&
    (w(descriptor.data)!=(program==0x121?0x176d0u:0x35460u)||
    w(descriptor.data+4)!=(program==0x121?20000u:18000u)||
    w(descriptor.data+8)!=0x5fc080ffu||w(descriptor.data+12)))
  return unsupported(s,0x35fc);
 const int hurt5_impact=r103_hurt5_impact_profile(candidate,p,NULL);
 if(hurt5_impact&&!r103_hurt5_impact_profile(candidate,p,descriptor.data))
  return unsupported(s,0x35fc);
 const int r103_grab_impact=r103_grab_impact_profile(candidate,p);
 if(r103_grab_impact&&(w(descriptor.data)!=0xcde10u||w(descriptor.data+4)!=16000||
    w(descriptor.data+8)!=0x5fc080ffu||w(descriptor.data+12)))return unsupported(s,0x35fc);
 const uint32_t mount_fall_sample=enemy_scene_motion_sample(candidate,bank,program,p);
 if(mount_fall_sample&&(w(descriptor.data)!=mount_fall_sample||
    w(descriptor.data+4)!=(bank?11025u:program==0xb8?14000u:16000u)||
    w(descriptor.data+8)!=(bank?0x5fcb80ffu:0x5fc080ffu)||w(descriptor.data+12)))return unsupported(s,0x35fc);
 const int close_impact=core_close_impact_profile(candidate,p);
 if(close_impact&&(w(descriptor.data)!=(program==0xf5?0x20540u:485136u)||
    w(descriptor.data+4)!=(program==0xf5?20000u:18000u)||
    w(descriptor.data+8)!=0x5fc080ffu||w(descriptor.data+12)))
  return unsupported(s,0x35fc);
 const int player_hurt=player_hurt_profile(candidate,bank,program,p,descriptor.data);
 const int recovery=recovery_footing_profile(candidate,p,descriptor.data);
 const int door=scene_door_open_profile(candidate);
 const unsigned prop_break_room=room_prop_break_profile(candidate);
 const int item_drop=item_drop_profile(candidate);
 if(item_drop&&(w(descriptor.data)!=(program==0xfc?0x8a7c0u:0x7ad70u)||w(descriptor.data+4)!=16000||
    w(descriptor.data+8)!=0x5fc080ffu||w(descriptor.data+12)))return unsupported(s,0x35fc);
 const uint32_t prop_sample=prop_break_room==0x100?(candidate->requested_program==4?0x12bc0u:0xf630u):prop_break_room==0x102?0x113c0u:
    prop_break_room==0x103?0x5570u:0xe710u;
 if(prop_break_room&&(w(descriptor.data)!=prop_sample||w(descriptor.data+4)!=16000||
    w(descriptor.data+8)!=0x5fc080ffu||w(descriptor.data+12)))return unsupported(s,0x35fc);
 if(door){
  const unsigned kind=h(candidate->model->root.bytes+0x2fe);
  uint32_t sample_offset=0x19750u;
  if(kind==0x313)sample_offset=0x1e00u;
  if(kind==0x333)sample_offset=0x1b4e0u;
  if(w(descriptor.data)!=sample_offset||w(descriptor.data+4)!=14000||
     w(descriptor.data+8)!=0x5fc080ffu||w(descriptor.data+12))return unsupported(s,0x35fc);
 }
 const int devil_rise=r100_devil_rise_cue(candidate,bank,program,p,NULL);
 if(devil_rise&&!r100_devil_rise_cue(candidate,bank,program,p,descriptor.data))
  return unsupported(s,0x35fc);
 const int devil_idle=r100_devil_idle_cue(candidate,bank,program,p,NULL);
 if(devil_idle&&!r100_devil_idle_cue(candidate,bank,program,p,descriptor.data))
  return unsupported(s,0x35fc);
 const int devil_retreat=r100_devil_retreat_cue(candidate,bank,program,p,NULL);
 if(devil_retreat&&!r100_devil_retreat_cue(candidate,bank,program,p,descriptor.data))
  return unsupported(s,0x35fc);
 const int devil_ranged=r100_devil_ranged_cue(candidate,bank,program,p);
 if(devil_ranged&&(w(descriptor.data)||w(descriptor.data+4)!=22050||
    w(descriptor.data+8)!=0x5fc080ffu||w(descriptor.data+12)))return unsupported(s,0x35fc);
 const int devil_spawn=r100_devil_spawn_cue(candidate,p);
 if(devil_spawn&&(w(descriptor.data)!=0x40b70||w(descriptor.data+4)!=18000||
    w(descriptor.data+8)!=0x5fc080ffu||w(descriptor.data+12)))return unsupported(s,0x35fc);
 int impact_voice=adsr2==0x5fcb&&em0a_impact_profile(candidate,bank,program,p)&&
                  w(descriptor.data)==(program==0x2a?0u:0x2b80u)&&w(descriptor.data+4)==16000;
 int reaction_voice=adsr2==0x5fcb&&enemy_reaction_profile(candidate,bank,program,p,descriptor.data);
 const int em0a_attack_voice=em0a_attack_voice_profile(candidate,bank,program,p,descriptor.data);
 int sequence_voice=adsr2==0x5fcb&&enemy_state71_sequence_profile(candidate,bank,program,p,descriptor.data);
 int local_cue_voice=adsr2==0x5fcb&&(enemy_local_cue_profile(candidate,bank,program,p,descriptor.data)||
    r103_paired_attack_voice_profile(candidate,bank,program,p,descriptor.data));
 int attack_voice=adsr2==0x5fcb&&enemy_attack_sequence_profile(candidate,bank,program,p,descriptor.data);
 int death_voice=adsr2==0x5fcb&&enemy_death_cue_profile(candidate,bank,program,p,descriptor.data);
 int timed_voice=adsr2==0x5fcb&&em0a_timed_profile(candidate,bank,program,p)&&
                w(descriptor.data)==0x5be0&&w(descriptor.data+4)==16000;
 const int ema2_timer_voice=(adsr2==0x5fcb&&
    (ema2_timer_cue_profile(candidate,bank,program,p,descriptor.data)||
     ema2_home_cue_profile(candidate,bank,program,p,descriptor.data)))||
    (adsr2==0x5fc0&&r102_ema2_poison_profile(candidate,bank,program,p,descriptor.data));
 /* The loop bit also requires the physical ADPCM repeat markers and stable
  * predictor history validated by the decoder. */
 const int emad_loop=emad_room_loop(candidate,bank,program,p);
 const int gate_loop=!candidate->source_owner&&bank==2&&program==0x10&&r105_gate_loop_descriptor(p);
 const int ts_loop=candidate->ts_environment!=NULL;
 if(emad_loop||gate_loop||ts_loop){
  const size_t extent=ts_loop?13440u:emad_loop?12736u:13040u;
  if(w(descriptor.data)!=(ts_loop?0xdf40u:emad_loop?0:0x2b80u)||w(descriptor.data+4)!=16000||
     adsr1!=0x80ff||adsr2!=(emad_loop?0x5fcb:0x5fc0)||w(descriptor.data+12)!=1||
     (emad_loop?tail.size!=extent:tail.size<extent))return unsupported(s,0x35fc);
  result=gh_audio_decode_ps_adpcm_loop(tail.data,extent,&candidate->pcm);
  if(result){s->frontier=0x35fc;return result;}
  if(gate_loop&&(candidate->pcm.source_bytes!=extent||candidate->pcm.loop_frames!=814u*28u)){
   gh_audio_pcm_release(&candidate->pcm);return unsupported(s,0x35fc);
  }
  /* The stable PCM repeat is appended after the full first pass.
   * Authored ADPCM block 197 marks its source, not its host PCM position. */
  if(ts_loop&&(candidate->pcm.source_bytes!=extent||candidate->pcm.loop_first!=840u*28u||
               candidate->pcm.loop_frames!=(840u-197u)*28u)){
   gh_audio_pcm_release(&candidate->pcm);return unsupported(s,0x35fc);
  }
  /* Bake key-on in source sample time. The repeating tail uses the
   * sustain level at loop entry in this native DSP reference. */
  const uint16_t pitch=selected_pitch?selected_pitch:1365;
  keyon_envelope env={.phase=1};
  uint64_t next_sample=0;
  for(uint64_t i=0;next_sample<candidate->pcm.loop_first;i++){
   const uint32_t level=keyon_envelope_step(&env);
   if(((i*pitch)>>12)>=next_sample){
    int16_t *value=&candidate->pcm.samples[next_sample++];
    *value=(int16_t)floor_shift((int64_t)*value*level,15);
   }
  }
  for(size_t i=candidate->pcm.loop_first;i<candidate->pcm.frames;i++)
   candidate->pcm.samples[i]=(int16_t)floor_shift((int64_t)candidate->pcm.samples[i]*env.level,15);
  candidate->emad_loop=(uint8_t)emad_loop;
  candidate->info.pitch_q12=pitch;
  candidate->info.stop_current=0x7f00;
  candidate->base_gain=gate_loop?(uint16_t)((uint64_t)s->mix.se_level_q8*(h(p+0x10)<<8)/0x7f00u):0;
  candidate->info.level_q14=candidate->base_gain/2;
  candidate->info.rate=16000;candidate->info.sample_slot=slot;
  candidate->info.frames=(uint32_t)candidate->pcm.frames;
  candidate->info.adsr1=adsr1;candidate->info.adsr2=adsr2;
  candidate->info.program=program;candidate->info.bank=bank;
  candidate->info.pending=1;candidate->info.channel=NONE;
  return GH_VOICE_OK;
 }
 const int pressure_cue=enemy_pressure_cue_profile(candidate,bank,program,p,NULL);
 if(pressure_cue&&!enemy_pressure_cue_profile(candidate,bank,program,p,descriptor.data))
  return unsupported(s,0x35fc);
 const int roulette_motion_voice=r103_roulette_motion_voice_profile(candidate,bank,program,p,descriptor.data);
 const int evade_voice=player_evade_award_voice(candidate,bank,program,p,descriptor.data);
 if(adsr1!=0x80ff||(adsr2!=0x5fc0&&!evade_voice&&!roulette_motion_voice&&!pressure_cue&&!ema2_timer_voice&&!square_voice&&!back_circle_voice&&!triangle_voice&&!escape_voice&&!player_cc&&!player_cd&&!hurt_voice&&!player_death&&!fall_grunt&&!pickup_voice&&!impact_voice&&!em0a_attack_voice&&!reaction_voice&&!timed_voice&&!sequence_voice&&!local_cue_voice&&!attack_voice&&!death_voice&&!mount_fall_sample)||w(descriptor.data+12))return unsupported(s,0x35fc);
 static const uint32_t ema2_impact_offsets[]={0x553c0,0x56490,0x54300};
 int ema2_voice=adsr2==0x5fc0&&core_ema2_impact_profile(candidate,bank,program,p)&&
                w(descriptor.data)==ema2_impact_offsets[program-0x96]&&w(descriptor.data+4)==22050;
 const int footstep=core_enemy_footstep_profile(candidate,bank,program,p,NULL);
 if(footstep&&!core_enemy_footstep_profile(candidate,bank,program,p,descriptor.data))
  return unsupported(s,0x35fc);
 const int player_weapon_impact=core_r101_weapon_player_impact_profile(candidate,bank,program,p,NULL);
 if(player_weapon_impact&&!core_r101_weapon_player_impact_profile(candidate,bank,program,p,descriptor.data))
  return unsupported(s,0x35fc);
 const int weapon_swing=weapon_swing_profile(candidate,bank,program,p,NULL);
 if(weapon_swing&&!weapon_swing_profile(candidate,bank,program,p,descriptor.data))
  return unsupported(s,0x35fc);
 const int weapon_contact=core_r102_weapon_contact(candidate,bank,program,p,NULL);
 if(weapon_contact&&!core_r102_weapon_contact(candidate,bank,program,p,descriptor.data))
  return unsupported(s,0x35fc);
 const int turn_core=core_enemy_turn_profile(candidate,bank,program,p,NULL);
 if(turn_core&&!core_enemy_turn_profile(candidate,bank,program,p,descriptor.data))
  return unsupported(s,0x35fc);
 const int knockdown_core=core_knockdown_profile(candidate,bank,program,p,NULL);
 if(knockdown_core&&!core_knockdown_profile(candidate,bank,program,p,descriptor.data))
  return unsupported(s,0x35fc);
 const int ticket=r102_ticket_spawn_profile(candidate,p);
 if(ticket&&(w(descriptor.data)!=0x7ad70u||w(descriptor.data+4)!=16000||
    w(descriptor.data+8)!=0x5fc080ffu||w(descriptor.data+12)))return unsupported(s,0x35fc);
 const int attack_core=core_enemy_attack_profile(candidate,bank,program,p,NULL);
 if(attack_core&&!core_enemy_attack_profile(candidate,bank,program,p,descriptor.data))
  return unsupported(s,0x35fc);
 const int boosted_impact=boost_impact_voice(candidate,bank,program,p,NULL);
 if(boosted_impact&&!boost_impact_voice(candidate,bank,program,p,descriptor.data))
  return unsupported(s,0x35fc);
 const int finish_voice=player_heat_finish_voice(candidate,bank,program,p,NULL);
 if(finish_voice&&!player_heat_finish_voice(candidate,bank,program,p,descriptor.data))
  return unsupported(s,0x35fc);
 const int heat_aura=player_active_heat_voice(candidate,bank,program,p,NULL);
 if(heat_aura&&!player_active_heat_voice(candidate,bank,program,p,descriptor.data))
  return unsupported(s,0x35fc);
 const int roulette_windup=r103_roulette_windup_cue(candidate);
 if(roulette_windup&&(w(descriptor.data)!=(program==0x120?0x8d840u:0x1aab0u)||
    w(descriptor.data+4)!=(program==0x120?16000u:22050u)||
    w(descriptor.data+8)!=0x5fc080ffu||w(descriptor.data+12)))return unsupported(s,0x35fc);
 const int centered_motion=centered_enemy_motion_profile(candidate,bank,program,p,NULL);
 if(centered_motion&&!centered_enemy_motion_profile(candidate,bank,program,p,descriptor.data))
  return unsupported(s,0x35fc);
 if(roulette_windup||centered_motion||finish_voice||heat_aura||boosted_impact||impact_voice||reaction_voice||ema2_voice||ema2_timer_voice||timed_voice||footstep||sequence_voice||attack_core||evade_voice||
    local_cue_voice||weapon_swing||weapon_contact||turn_core||attack_voice||death_voice||knockdown_core||player_hurt||recovery||
    devil_spawn||devil_rise||devil_idle||devil_ranged||devil_retreat||em0a_attack_voice||
    close_impact||hurt5_impact||r103_grab_impact||item_drop||player_weapon_impact||pressure_cue||ticket||mount_fall_sample)candidate->combat_profile=1;
 /* The admitted cue keeps its spatial mix when 2CE488 detaches the actor. */
 candidate->door_spatial=(uint8_t)(door||prop_break_room);
 rate=w(descriptor.data+4);if(rate<1000 || rate>48000)return unsupported(s,0x38e4);
 candidate->info.pitch_q12=selected_pitch?selected_pitch:(uint16_t)((uint64_t)rate*4096/48000);
 result=gh_audio_decode_ps_adpcm(tail.data,tail.size,&candidate->pcm);if(result){s->frontier=0x35fc;return result;}
 result=render(&candidate->pcm,candidate->info.pitch_q12);if(result){gh_audio_pcm_release(&candidate->pcm);return result;}
 level=(unsigned)((uint64_t)s->mix.se_level_q8*(h(p+0x10)<<8)/0x7f00);
 candidate->base_gain=(uint16_t)level;level/=2;
 candidate->info.stop_current=0x7f00;
 candidate->info.level_q14=(uint16_t)level;candidate->info.rate=rate;candidate->info.sample_slot=slot;candidate->info.frames=(uint32_t)candidate->pcm.frames;
 candidate->info.adsr1=adsr1;candidate->info.adsr2=adsr2;candidate->info.program=program;candidate->info.bank=bank;candidate->info.pending=1;candidate->info.channel=NONE;
 return GH_VOICE_OK;
}
static gh_voice_status prepare_pcm(gh_se_voice_engine*s,int16_t bank,int16_t program,const uint8_t*p,struct voice*candidate){return prepare_pcm_with_pitch(s,bank,program,p,candidate,0);}
gh_voice_status gh_se_voice_create(const gh_audio_registry *r,gh_audio_output *output,const gh_se_mix *mix,gh_se_voice_engine **out)
{
 gh_se_voice_engine *s;if(!r || !output || !mix || !out || mix->se_level_q8>0x7f00 || mix->output_mode>1)return GH_VOICE_ARGUMENT;
 if(mix->dsp_policy!=GH_SE_DSP_NATIVE_LINEAR_REFERENCE)return GH_VOICE_UNSUPPORTED;
 s=calloc(1,sizeof(*s));if(!s)return GH_VOICE_MEMORY;s->registry=r;s->output=output;s->mix=*mix;s->head=-1;for(unsigned i=0;i<51;++i)s->channels[i]=-1;*out=s;return GH_VOICE_OK;
}
gh_voice_status gh_se_voice_bind_origin_seh(gh_se_voice_engine *s,const void *bytes,size_t size)
{
 if(!s || (!bytes && size))return GH_VOICE_ARGUMENT;
 s->origin_seh=bytes;s->origin_seh_size=size;s->origin_scene_bound=1;return GH_VOICE_OK;
}
gh_voice_status gh_se_voice_bind_external_adx(gh_se_voice_engine *s,gh_disc *disc,
 const gh_sound_control *control,const void *elf,size_t elf_bytes)
{
 if(!s||!disc||!control||!elf||!control->initialized||
    control->room_heaps[1].registry!=s->registry||!control->room_heaps[1].identity_verified)
  return GH_VOICE_ARGUMENT;
 s->frontier=0;
 if(s->sequence.disc)
  return s->sequence.disc==disc&&s->sequence.control==control?GH_VOICE_OK:GH_VOICE_ARGUMENT;
 sequence_audio config={.disc=disc,.control=control};
 if(gh_adx_key_from_elf(elf,elf_bytes,&config.key))return unsupported(s,0x2ca34c);
 if(gh_adx_volume_table_from_elf(elf,elf_bytes,config.decibels))return unsupported(s,0x373ff0);
 s->sequence=config;
 return GH_VOICE_OK;
}

typedef struct {
 int16_t program;
 uint8_t row,member,channels,first_channel,priority;
 uint8_t group_size_minus_one;
 uint32_t bytes,frames,format,level;
} sequence_clip;

/* Original EM00 counter and the paired PL00 grab select the same CORE stereo clip. */
static int core_grab_sequence(const struct voice *v)
{
 const uint8_t *p=v->descriptor;
 const uint8_t *boss=r103_boss_voice_actor(v);
 const int wall_recoil=v->source_owner&&v->part_index==-1&&boss&&
    h(boss+0x2f4)==0x0401&&boss[0x2f6]==3&&!boss[0x2f7];
 return (em00_counter_voice_model(v)||paired_grab_player_voice_model(v)||wall_recoil)&&
    !v->requested_bank&&v->requested_program==7&&
    !v->info.bank&&v->info.program==7&& w(p)==0x00070000u&&
    w(p+4)==0x01310000u&&w(p+8)==0x270u&&w(p+12)==0x00220000u&&
    w(p+16)==0x007f007fu&&w(p+20)==0x005a0040u&&!w(p+24)&&!w(p+28)&&
    w(p+32)==0x00060004u&&!w(p+36)&&w(p+40)==20&&!w(p+44);
}
/* Original 271D5C queues CORE 01 on the recipient of the amplified Take. */
static int core_critical_sequence(const struct voice *v)
{
 const uint8_t *p=v->descriptor;
 const gh_actor_model *model=v->model;
 if(!v->source_owner||!model||!model->work||!model->root.bytes||
    model->root.size<0x642||model->root.original_vtable214!=0x4479c8)return 0;
 const uint8_t *actor=model->root.bytes;
 const uint16_t state=h(actor+0x2f4),stamp=h(actor+0x640);
 const int r100=model->work->room==0x100&&stamp==8&&state==0x0701;
 const int r102=model->work->room==0x102&&model->root.size>0x1650&&
    ((stamp==0x10&&!actor[0x1650])||(stamp==0x0b&&actor[0x1650]==5))&&
    (state==0x0001||(state==0x0002&&!h(actor+0x54a)));
 const int r102_em0a=model->work->room==0x102&&stamp==2&&state==0x0101&&
    model->root.size>0x1650&&actor[0x1650]==5&&w(actor+0x564)==0x20a;
 const int arena_em00=r103_em00_voice_actor(v)!=NULL;
 const int arena_boss=r103_boss_voice_actor(v)&&!actor[0x2f7];
 const int arena=actor[0x2f6]==0&&
    ((state==0x0101&&(arena_em00||r103_em0a_voice_actor(v)||arena_boss))||(state==0x0701&&arena_em00));
 const unsigned dependency=w(actor+0x564);
 const int r101=model->work->room==0x101&&model->root.size>0x1650&&
    state==0x0001&&!actor[0x2f6]&&!actor[0x2f7]&&
    ((stamp==9&&actor[0x1650]==4&&dependency==0x227&&w(actor+0x560)==0x3e3)||
     (actor[0x1650]==6&&((stamp==0x16&&dependency==0x200)||(stamp==0x20&&dependency==0x203))));
 const int room_actor=(((r100||r102)&&w(actor+0x564)==0x200)||r102_em0a||r101)&&
    h(actor+0x2fe)==0x202;
 const int west_em02=model->work->room==0x100&&model->root.size>=0x16c8&&
    dependency==0x202&&stamp==4&&actor[0x1650]==2&&w(actor+0x16c4)==6&&
    (state==0x0101||(state==0x0701&&!h(actor+0x54a)));
 const int west_em03=model->work->room==0x100&&model->root.size>=0x16c8&&
    dependency==0x203&&stamp==19&&!actor[0x1650]&&w(actor+0x16c4)==6&&
    state==0x0101&&!actor[0x2f6]&&!actor[0x2f7];
 const int r104=r104_em03_voice_actor(v)&&state==0x0101&&!actor[0x2f6]&&!actor[0x2f7];
 return (room_actor||arena||r104||((west_em02||west_em03)&&h(actor+0x2fe)==0x202))&&
    (h(actor+0x3ac)&0x100u)&&v->part_index==-1&&
    !v->requested_bank&&v->requested_program==1&&!v->info.bank&&v->info.program==1&&
    w(p)==0x00010000u&&w(p+4)==0x01310000u&&w(p+8)==0x272u&&w(p+12)==0x00220000u&&
    w(p+16)==0x007f007fu&&w(p+20)==0x005a0040u&&!w(p+24)&&!w(p+28)&&
    w(p+32)==4&&!w(p+36)&&w(p+40)==26&&!w(p+44);
}
/* The cEm00 blade impact in 109928 queues the original mono CORE AFD row 40. */
static int core_r102_sword_sequence(const struct voice *v)
{
 const uint8_t *p=v->descriptor;
 return r102_player_weapon_impact_source(v)&&!v->requested_bank&&v->requested_program==0x137&&
    !v->info.bank&&v->info.program==0x137&&w(p)==0x01370000u&&w(p+4)==0x01300000u&&
    w(p+8)==0x64u&&w(p+12)==0x00220000u&&w(p+16)==0x007f007fu&&
    w(p+20)==0x005a0040u&&!w(p+24)&&!w(p+28)&&w(p+32)==4u&&
    !w(p+36)&&w(p+40)==40u&&!w(p+44);
}
/* PL00 114608 uses original CORE04 AFD row16 while Roulette is open. */
static int core_roulette_sequence(const struct voice *v)
{
 const gh_actor_model *model=v->model;
 if(!v->source_owner||v->requested_bank||v->requested_program!=4||
    v->info.bank||v->info.program!=4||v->part_index!=-1||
    !model||!model->work||model->work->room!=0x103||
    !model->root.bytes||model->root.size<0x300||model->root.original_vtable214!=0x41d220)
  return 0;
 const uint8_t *actor=model->root.bytes,*p=v->descriptor;
 return h(actor+0x2f4)==0x2b00&&actor[0x2f6]==1&&!actor[0x2f7]&&
    h(actor+0x2fe)==0x100&&(h(actor+0x3ac)&0x100u)&&
    w(p)==0x00040000u&&w(p+4)==0x00300000u&&w(p+8)==0x273u&&w(p+12)==0x00220000u&&
    w(p+16)==0x007f007fu&&w(p+20)==0x005a0040u&&!w(p+24)&&!w(p+28)&&
    w(p+32)==4&&!w(p+36)&&w(p+40)==16&&!w(p+44);
}

/* 24F8F0 starts the stereo CORE recoil cue on the standing receiver. */
static int core_heavy_recoil_sequence(const struct voice *v)
{
 const uint8_t *p=v->descriptor;
 return r103_heavy_recoil_source(v)&&!v->requested_bank&&v->requested_program==0x2e&&
    !v->info.bank&&v->info.program==0x2e&&w(p)==0x002e0000u&&
    w(p+4)==0x01300000u&&w(p+8)==0x264u&&w(p+12)==0x00220000u&&w(p+16)==0x007f007fu&&
    w(p+20)==0x005a0040u&&!w(p+24)&&!w(p+28)&&w(p+32)==0x00060004u&&
    !w(p+36)&&w(p+40)==10&&!w(p+44);
}

static const sequence_clip *find_sequence_clip(const char *archive,int16_t bank,int16_t program)
{
 static const sequence_clip clips[]={
  {0x0d,2,1,2,49,100,1,108166,96079,0x0901bb80,0x0200717f},
  {0x32,4,2,1,48,100,1,45454,80692,0x0501bb80,0x02005a71},
  {0x33,6,3,1,48,100,1,30082,53375,0x0501bb80,0x02005a71},
  {0x35,10,5,1,48,100,1,56218,99818,0x0501bb80,0x02005a71},
  {0x36,12,6,1,49,102,1,44248,78542,0x0501bb80,0x02005a71}
 };
 static const sequence_clip roulette={4,16,2,2,48,115,0,253952,225044,0x0901bb80,0x02007171};
 static const sequence_clip counter={7,20,2,2,49,112,1,253952,225044,0x05015dc0,0x02007171};
 static const sequence_clip r105_gate={0x32,2,1,1,48,100,1,38992,69209,0x0501bb80,0x02005a71};
 static const sequence_clip critical={1,26,3,2,49,114,1,146902,130514,0x09015dc0,0x0200717f};
 static const sequence_clip r101_gate[]={
  {0x28,2,1,1,48,100,1,42034,74615,0x0501bb80,0x02007f7f},
  {0x29,4,2,2,49,100,1,104530,92838,0x0901bb80,0x0200657f}
 };
 static const sequence_clip r102_gate[]={
  {0x0e,2,1,2,48,100,2,131386,116736,0x0901bb80,0x02007f7f},
  {0x32,6,3,1,49,100,1,42178,74861,0x0501bb80,0x02007f7f},
  {0x33,4,2,1,49,100,1,21136,37443,0x0501bb80,0x02005065}
 };
 /* Scenario REL0670 uses the original R103 room bank's arena calls. */
 static const sequence_clip r103_arena[]={
  {0x0c,0,0,2,48,100,1,156406,138968,0x0901bb80,0x02004865},
  {0x32,2,1,1,49,100,1,32152,57029,0x0501bb80,0x0200657f},
  {0x33,4,2,1,49,100,1,25618,45413,0x0501bb80,0x02007f7f},
  {0x34,6,3,1,49,100,1,19768,35017,0x0501bb80,0x0200657f},
  {0x35,8,4,1,49,100,1,18850,33402,0x0501bb80,0x02007f7f},
  {0x36,10,5,1,50,100,0,46516,82591,0x0501bb80,0x02007f7f},
  {0x37,12,6,2,48,100,1,147982,131484,0x0901bb80,0x02007f7f},
  {0x38,14,7,2,48,100,1,41350,36698,0x0901bb80,0x0200717f},
  {0x39,16,8,1,48,100,1,62266,110561,0x0501bb80,0x02007f7f},
  {0x3a,18,9,1,49,100,1,52474,93153,0x0501bb80,0x02007f7f}
 };
 static const sequence_clip sword={0x137,40,10,1,48,100,1,24484,43408,0x0501bb80,0x0200657f};
 static const sequence_clip recoil={0x2e,10,1,2,48,100,1,141322,125551,0x0901bb80,0x02007f7f};
 if(!bank&&program==0x2e&&!strcmp(archive,"se/core.afs"))return &recoil;
 if(!bank&&program==0x137&&!strcmp(archive,"se/core.afs"))return &sword;
 if(!bank&&program==1&&!strcmp(archive,"se/core.afs"))return &critical;
 if(!bank&&program==4&&!strcmp(archive,"se/core.afs"))return &roulette;
 if(!bank&&program==7&&!strcmp(archive,"se/core.afs"))return &counter;
 if(bank!=2)return NULL;
 if(!strcmp(archive,"se/r103.afs")){
  for(unsigned i=0;i<sizeof(r103_arena)/sizeof(r103_arena[0]);++i)
   if(r103_arena[i].program==program)return r103_arena+i;
  return NULL;
 }
 if(!strcmp(archive,"se/r101.afs"))
  return program>=0x28&&program<=0x29?r101_gate+(program-0x28):NULL;
 if(!strcmp(archive,"se/r105.afs"))return program==0x32?&r105_gate:NULL;
 if(!strcmp(archive,"se/r102.afs")){
  if(program==0x0e)return r102_gate;
  if(program==0x32)return r102_gate+1;
  if(program==0x33)return r102_gate+2;
  return NULL;
 }
 if(strcmp(archive,"se/r100.afs"))return NULL;
 for(unsigned i=0;i<sizeof(clips)/sizeof(clips[0]);i++)if(clips[i].program==program)return clips+i;
 return NULL;
}

static int sequence_profile(const sequence_clip *clip,const uint8_t p[48])
{
 return w(p+4)==(((uint32_t)clip->group_size_minus_one<<24)|((uint32_t)clip->first_channel<<16))&&
  w(p+8)==(0x200u|clip->priority)&&w(p+0xc)==0x00220000&&
  w(p+0x10)==0x007f007f&&w(p+0x14)==0x005a0040&&!w(p+0x18)&&!w(p+0x1c)&&
  w(p+0x20)==0x00030004&&!w(p+0x24)&&w(p+0x28)==clip->row&&!w(p+0x2c);
}

/* R103 REL0448 attaches each original line to its authored arena stamp. */
static const sequence_clip *r103_dialogue_clip(const gh_se_voice_engine *s,const struct voice *v)
{
 const int program=v->info.program;
 const uint8_t *actor=r103_voice_actor(v,15);
 if(!actor||v->requested_bank!=2||
    v->info.bank!=2||(program!=0x39&&program!=0x3a)||v->requested_program!=program)
  return NULL;
 const unsigned index=(unsigned)program-0x39;
 if(h(actor+0x640)!=index||w(actor+0x564)!=0x214u+index)
  return NULL;
 const char *path=NULL;
 if(gh_audio_external_path(s->registry,2,&path)||strcmp(path,"se/r103.afs"))return NULL;
 const sequence_clip *clip=find_sequence_clip(path,2,(int16_t)program);
 const uint32_t descriptor_key=((uint32_t)program<<16)|2u;
 return clip&&w(v->descriptor)==descriptor_key&&sequence_profile(clip,v->descriptor)?clip:NULL;
}

static gh_voice_status sequence_prepare(gh_se_voice_engine *s,struct voice *v)
{
 const sequence_audio *config=&s->sequence;
 gh_sound_span afd;
 uint64_t offset,bytes;
 const char *path=NULL;
 if(gh_audio_external_path(s->registry,v->info.bank,&path))return unsupported(s,0x373d90);
 const sequence_clip *clip=find_sequence_clip(path,v->info.bank,v->info.program);
 if(!clip||v->info.sample_slot!=clip->row||v->external_channels!=clip->channels||
    gh_audio_external_sample(s->registry,v->info.bank,clip->row,&afd,&offset,&bytes))
  return unsupported(s,0x373d90);
 const uint8_t *source=afd.data;
 /* CORE04 row 16 and CORE07 row 20 use the same original ADX
  * member but distinct AFD control words. */
 const int roulette=v->info.bank==0&&v->info.program==4;
 if(afd.size!=32||h(source)!=clip->row||h(source+2)!=clip->member||
    w(source+4)!=(roulette?0x00020000u:0u)||
    w(source+8)!=clip->format||w(source+12)!=clip->level||
    w(source+16)!=(!v->info.bank&&clip->channels==2&&v->info.program!=4&&clip->program!=0x2e?1u:0u)||w(source+20)||w(source+24)||w(source+28)||
    bytes!=clip->bytes)
  return unsupported(s,0x373298);

 size_t index;
 if(gh_disc_find(config->disc,path,&index))return unsupported(s,0x32bb48);
 uint8_t *encoded=malloc((size_t)bytes);
 if(!encoded)return GH_VOICE_MEMORY;
 if(gh_disc_read(config->disc,index,offset,encoded,(size_t)bytes)){
  free(encoded);
  return unsupported(s,0x32bb48);
 }
 gh_adx_decoder *decoder=NULL;
 gh_adx_info info;
 gh_adx_error error=gh_adx_open(encoded,(size_t)bytes,&config->key,&decoder);
 if(!error)error=gh_adx_inspect(decoder,&info);
 const int looped_core=v->info.bank==0&&(v->info.program==4||v->info.program==7);
 if(error||info.channels!=clip->channels||info.sample_rate!=48000||info.sample_count!=clip->frames||
    (looped_core?(!info.loop_enabled||info.loop_start!=8448||info.loop_end!=222256||
              info.data_offset!=736):info.loop_enabled)){
  gh_adx_close(decoder);
  free(encoded);
  return unsupported(s,0x32bb48);
 }
 int16_t *samples=malloc((size_t)clip->frames*clip->channels*sizeof(*samples));
 if(!samples){
  gh_adx_close(decoder);
  free(encoded);
  return GH_VOICE_MEMORY;
 }
 size_t decoded;
 error=clip->channels==2?gh_adx_decode(decoder,samples,clip->frames,&decoded):
       gh_adx_decode_mono(decoder,samples,clip->frames,&decoded);
 gh_adx_close(decoder);
 free(encoded);
 if(error||decoded!=clip->frames){
  free(samples);
  return unsupported(s,0x32bb48);
 }
 /* 373D90 enables the verified ADX repeat with 32B188 before memory start. */
 v->pcm=(gh_adpcm_pcm){.samples=samples,.frames=clip->frames,.source_bytes=(size_t)bytes,
    .loop_first=looped_core?info.loop_start:0,.loop_frames=looped_core?info.loop_end-info.loop_start:0};
 v->info.frames=clip->frames;
 /* 37C6A0 scales descriptor level 127 by the authored AFD weight.
  * 37D3C8 uses the existing SE mixer before the separate 373FF0 gain. */
 const unsigned level=h(v->descriptor+0x10)*source[12]/127u;
 v->base_gain=(uint16_t)((uint32_t)s->mix.se_level_q8*(level<<8)/0x7f00u);
 v->info.level_q14=v->base_gain/2u;
 return GH_VOICE_OK;
}

static gh_voice_status sequence_gain(gh_se_voice_engine *s,const struct voice *v,float *gain)
{
 const gh_sound_control *control=s->sequence.control;
 if(!control||!control->initialized||control->output_mode!=1)return unsupported(s,0x3756f0);
 const float master=sound_float(control->gain_bits[1]), category=sound_float(control->gain_bits[4]);
 if(!isfinite(master)||!isfinite(category)||master<0||master>127||category<0||category>127)
  return unsupported(s,0x373ff0);
 unsigned base=v->base_gain;
 if(v->source_owner&&v->external_channels&&v->info.position_mode==2){
  const char *path=NULL;
  if(gh_audio_external_path(s->registry,v->info.bank,&path))return unsupported(s,0x373d90);
  const sequence_clip *clip=find_sequence_clip(path,v->info.bank,v->info.program);
  if(!clip)return unsupported(s,0x373d90);
  /* 37C6A0 scales the position-adjusted 7-bit command by the authored AFD level. */
  const unsigned level=v->spatial_gain*(clip->level&0xffu)/127u;
  base=(unsigned)s->mix.se_level_q8*(level<<8)/0x7f00u;
 }
 const unsigned weight=(base*(unsigned)v->info.stop_current/0x7f00u)>>8;
 volatile float a=(float)weight*master,b=a/127.0f,c=b*category,d=c/127.0f;
 const unsigned index=(unsigned)(d<127.0f?d:127.0f);
 volatile float exponent=(float)s->sequence.decibels[index]/200.0f;
 *gain=powf(10.0f,exponent);
 return GH_VOICE_OK;
}

void gh_se_voice_destroy(gh_se_voice_engine *s){if(s){for(int i=0;i<64;++i)if(s->voices[i].used)drop(s,i,1);free(s);}}
static void origin_publish(gh_se_voice_engine *s,unsigned slot,const uint8_t *p,
 struct voice *candidate,uint32_t *handle)
{
 if(!++s->serial)++s->serial;
 candidate->info.original_handle=s->serial;
 candidate->next=s->head;
 candidate->used=1;
 candidate->priority=p[8];
 candidate->group_first=p[6];
 candidate->group_count=p[7]+1;
 candidate->replace_equal=(p[0xe]&0x20u)!=0;
 s->voices[slot]=*candidate;
 s->head=(int)slot;
 *handle=s->serial;
}

static int origin_duplicate(const gh_se_voice_engine *s,int16_t bank,int16_t program,unsigned age)
{
 if(!age)return 0;
 for(int j=s->head;j!=-1;j=s->voices[j].next){
  const struct voice *v=&s->voices[j];
  if(v->info.bank==bank&&v->info.program==program&&
     (v->ee_age<=age||v->ee_age>=UINT32_C(0x80000000)))return 1;
 }
 return 0;
}

static int rank_origin_descriptor(unsigned program,const uint8_t *p)
{
 const unsigned sample=program==0x38?39u:program==0x36?38u:37u;
 const uint32_t behavior=program==0x35?0x00030105:0x00030005;
 return h(p+4)==sample&&w(p+6)==0x00680126u&&h(p+0xe)==0x21&&
   w(p+0x10)==0x006e006eu&&w(p+0x14)==0x005a0040u&&w(p+0x18)==0x0080ff80u&&
   !h(p+0x1c)&&w(p+0x20)==behavior&&!w(p+0x24)&&h(p+0x28)==0xffffu&&
   !h(p+0x2a)&&!w(p+0x2c);
}

gh_voice_status gh_se_voice_request_rank_origin(gh_se_voice_engine *s,unsigned program,
 gh_game_mt *random,uint32_t *handle)
{
 if(!s||!random||!handle)return GH_VOICE_ARGUMENT;
 *handle=0;
 s->frontier=0;
 if(program!=0x35&&program!=0x38)return unsupported(s,0x2cb8a0);
 if(!s->origin_scene_bound)return unsupported(s,0x2cae10);
 if(s->mix.system_flags&0x08000040)return unsupported(s,0x2ce588);
 unsigned slot=0;
 for(;slot<64&&s->voices[slot].used;++slot){}
 if(slot==64)return GH_VOICE_OK;

 uint8_t p[48];
 if(gh_audio_get_program(s->registry,0,(int16_t)program,0,p)!=GH_AUDIO_OK)
  return unsupported(s,0x374cb8);
 if(!rank_origin_descriptor(program,p))return unsupported(s,0x2cdff0);
 gh_game_mt next=*random;
 if(program==0x35){
  uint32_t draw;
  if(gh_game_mt_next_0030f348(&next,&draw))return unsupported(s,0x31cfe0);
  program+=draw&1u;
  if(gh_audio_get_program(s->registry,0,(int16_t)program,0,p)!=GH_AUDIO_OK)
   return unsupported(s,0x374cb8);
  if(!rank_origin_descriptor(program,p))return unsupported(s,0x2cdff0);
 }
 if(origin_duplicate(s,0,(int16_t)program,p[0x22])){
  *random=next;
  return GH_VOICE_OK;
 }
 gh_sound_span descriptor,tail;
 if(gh_audio_sample_at(s->registry,0,h(p+4),&descriptor,&tail)!=GH_AUDIO_OK||
    descriptor.size!=16||w(descriptor.data+4)!=(program==0x38?18000u:16000u)||
    w(descriptor.data+8)!=0x5fc080ffu||w(descriptor.data+12))
  return unsupported(s,0x35fc);
 struct voice candidate={0};
 gh_voice_status status=prepare_pcm(s,0,(int16_t)program,p,&candidate);
 if(status)return status;
 memcpy(candidate.descriptor,p,sizeof candidate.descriptor);
 candidate.info.pitch_q12=0;
 *random=next;
 origin_publish(s,slot,p,&candidate,handle);
 return GH_VOICE_OK;
}

gh_voice_status gh_se_voice_request_origin(gh_se_voice_engine *s,int16_t bank,int16_t program,int16_t arg3,int16_t arg4,uint32_t flags,int32_t delay,uint32_t *handle)
{
 uint8_t p[48];gh_voice_status result;unsigned i;struct voice candidate={0};
 if(!s || !handle)return GH_VOICE_ARGUMENT;*handle=0;s->frontier=0;
 int title=bank==2&&(program==0||(program>=10&&program<=12)||program==15);
 const int gate_loop=bank==2&&program==0x10;
 const int gate_close=bank==2&&program==0x11;
 const char *archive=NULL;
 const sequence_clip *sequence=NULL;
 if(s->sequence.disc&&bank==2&&!gh_audio_external_path(s->registry,bank,&archive))
  sequence=find_sequence_clip(archive,bank,program);
 const int empty_load=bank==0&&program==0x122;
 const int r101_side_door=bank==2&&program==3&&archive&&!strcmp(archive,"se/r101.afs");
 if((!title&&!sequence&&!empty_load&&!gate_loop&&!gate_close&&!r101_side_door&&(bank || (program!=0x2d && program!=0x15e && program!=0x15f && program!=0x160 && program!=0x161))) || arg3!=-1 || arg4!=-1 || flags || delay)return unsupported(s,0x2cb8a0);
 if(s->mix.system_flags&0x08000040)return unsupported(s,0x2ce588);
 if(gh_audio_get_program(s->registry,bank,program,0,p)!=GH_AUDIO_OK)return unsupported(s,0x374cb8);
 /* Exact audited subsets; +21 controls random variants, +22 duplicate age
  * window. +09 is origin/output selection, not variant count. */
 if(sequence){
  if(!s->origin_scene_bound||s->origin_seh||s->origin_seh_size)return unsupported(s,0x2cae10);
  if(!sequence_profile(sequence,p))return unsupported(s,0x2cdff0);
 }else if(r101_side_door){
  if(!s->origin_scene_bound||s->origin_seh||s->origin_seh_size)return unsupported(s,0x2cae10);
  if(w(p)!=0x00030002u||w(p+4)!=0x02170001u||w(p+8)!=0x250u||
     w(p+12)!=0x00200000u||w(p+16)!=0x007f007fu||w(p+20)!=0x005a0040u||
     w(p+24)!=0x0080ff00u||w(p+28)!=0x00070000u||w(p+32)!=0x000a0004u||
     w(p+36)!=0x300u||w(p+40)!=0xffffu||w(p+44))return unsupported(s,0x2cdff0);
  gh_sound_span descriptor,tail;
  if(gh_audio_sample_at(s->registry,bank,1,&descriptor,&tail)!=GH_AUDIO_OK||
     descriptor.size!=16||w(descriptor.data)!=0x1e00||w(descriptor.data+4)!=14000||
     w(descriptor.data+8)!=0x5fc080ffu||w(descriptor.data+12))
   return unsupported(s,0x35fc);
 }else if(gate_loop){
  if(!s->origin_scene_bound||s->origin_seh||s->origin_seh_size)return unsupported(s,0x2cae10);
  if(!r105_gate_loop_descriptor(p))return unsupported(s,0x2cdff0);
 }else if(gate_close){
  if(!s->origin_scene_bound||s->origin_seh||s->origin_seh_size)return unsupported(s,0x2cae10);
  if(!r105_gate_close_descriptor(p))return unsupported(s,0x2cdff0);
  gh_sound_span descriptor,tail;
  if(gh_audio_sample_at(s->registry,bank,6,&descriptor,&tail)!=GH_AUDIO_OK||
     descriptor.size!=16||w(descriptor.data)!=0x5e70||w(descriptor.data+4)!=14000||
     w(descriptor.data+8)!=0x5fc080ff||w(descriptor.data+12))
   return unsupported(s,0x35fc);
 }else if(title){
  if(!s->origin_scene_bound||s->origin_seh||s->origin_seh_size)return unsupported(s,0x2cae10);
  if(program==0){if(p[6]!=10||p[7]!=4||p[8]!=120||p[9]||h(p+0xe)!=0x20||h(p+0x10)!=90||w(p+0x20)!=0x30000||w(p+0x24)!=0x300||h(p+0x1e))return unsupported(s,0x2cdff0);}
  else if(p[6]!=20||p[7]!=4||p[8]!=122||p[9]||h(p+0xe)!=0x23||h(p+0x10)!=120||w(p+0x20)||w(p+0x24)||h(p+0x1e)!=255)return unsupported(s,0x2cdff0);
  if(program==15&&h(p+4)!=4)return unsupported(s,0x2cdff0);
 }else if(empty_load){
  if(!s->origin_scene_bound||s->origin_seh||s->origin_seh_size)return unsupported(s,0x2cae10);
  if(h(p+4)!=148||w(p+6)!=0x00640100||w(p+0xa)||h(p+0xe)!=0x23||
     w(p+0x10)!=0x00780078||w(p+0x18)!=0x0080ff80||w(p+0x20)!=0x00060005)
   return unsupported(s,0x2cdff0);
  gh_sound_span descriptor,tail;
  if(gh_audio_sample_at(s->registry,bank,148,&descriptor,&tail)!=GH_AUDIO_OK||
     descriptor.size!=16||w(descriptor.data)!=0x176d0||w(descriptor.data+4)!=20000||
     w(descriptor.data+8)!=0x5fc080ff||w(descriptor.data+12))
   return unsupported(s,0x35fc);
 }else if(program==0x2d){
  if(p[6]!=15||p[7]!=1||p[8]!=104||p[9]!=2||h(p+0xe)!=0x21||w(p+0x20)!=0x00030005||h(p+0x10)!=90)return unsupported(s,0x2cdff0);
  if(!s->origin_scene_bound||s->origin_seh||s->origin_seh_size)return unsupported(s,0x2cae10);
 }else if(p[6]||p[7]!=4||p[8]!=120||p[9]||h(p+0xe)!=0x23||w(p+0x20))return unsupported(s,0x2cdff0);
 if(!sequence&&!gate_close&&!r101_side_door&&(h(p+0x10)!=h(p+0x12)||h(p+0x14)!=64||h(p+0x16)!=90||(!empty_load&&w(p+0x18))||h(p+0x1c)||h(p+0x28)!=65535||h(p+0x2a)||w(p+0x2c)))return unsupported(s,0x2cdff0);
 if(!title&&!sequence&&!gate_loop&&!gate_close&&!r101_side_door&&(h(p+0x1e)!=255||w(p+0x24)))return unsupported(s,0x2cdff0);
 if(bank==0&&program==0x15f&&(h(p+4)!=171||h(p+0x10)!=75))
  return unsupported(s,0x2cdff0);
 /* Actual2CBF78 compares bank AND program; duplicate-age suppression for
  * equal-origin zero-delay is shared by logo and room confirmation0. */
 if(origin_duplicate(s,bank,program,p[0x22]))return GH_VOICE_OK;
 for(i=0;i<64 && s->voices[i].used;++i){}if(i==64)return GH_VOICE_OK;
 if(sequence){
  memcpy(candidate.descriptor,p,sizeof(candidate.descriptor));
  candidate.external_channels=sequence->channels;
  candidate.info=(gh_se_voice_info){.bank=bank,.program=program,
   .sample_slot=h(p+0x28),.rate=48000,.pitch_q12=4096,.pending=1,.channel=NONE,.stop_current=0x7f00};
 }else{
  result=prepare_pcm(s,bank,program,p,&candidate);if(result)return result;
  if(gate_close||r101_side_door){
   memcpy(candidate.descriptor,p,sizeof(candidate.descriptor));
   candidate.origin_right_gain=(uint16_t)((uint64_t)s->mix.se_level_q8*(h(p+0x12)<<8)/0x7f00u);
  }
  if(empty_load){
   memcpy(candidate.descriptor,p,sizeof(candidate.descriptor));
   candidate.info.pitch_q12=0;
  }
 }
 origin_publish(s,i,p,&candidate,handle);return GH_VOICE_OK;
}
/* Actor-attached EE construction; existingPCM/DSP/channel/output owner above. */
/* R102's authored bank 2 program 34 uses the original mode-3 world loop. */
static int ts_voice_program(const uint8_t *p)
{
 return w(p+4)==0x02100006u&&w(p+8)==0x0000027au&&w(p+12)==0x00200000u&&
  w(p+16)==0x007f007fu&&w(p+20)==0x005a0040u&&!w(p+24)&&w(p+28)==0x00040000u&&
  w(p+32)==4&&w(p+36)==0x300u&&w(p+40)==0xffffu&&!w(p+44);
}

gh_voice_status gh_se_voice_request_ts_002cb968(gh_se_voice_engine *s,int16_t bank,int16_t program,
 const float position[4],const gh_se_attached_environment *environment,uint32_t *handle)
{
 if(!s||!position||!environment||!handle)return GH_VOICE_ARGUMENT;
 *handle=0;s->frontier=0;
 unsigned slot=0;
 while(slot<64&&s->voices[slot].used)++slot;
 if(slot==64)return GH_VOICE_OK;
 struct voice candidate={.part_index=-1,.ts_environment=environment,.flags18=0x20004u,.flags1c=3};
 gh_audio_status result=gh_audio_get_program(s->registry,bank,program,0,candidate.descriptor);
 if(result==GH_AUDIO_NOT_FOUND||result==GH_AUDIO_DISABLED||result==GH_AUDIO_BOUNDS)return GH_VOICE_OK;
 if(result)return unsupported(s,0x374cb8);
 if(bank!=2||program!=0x22||!ts_voice_program(candidate.descriptor))return unsupported(s,0x2cdff0);
 for(unsigned axis=0;axis<4;++axis)if(!isfinite(position[axis]))return unsupported(s,0x2cde28);
 memcpy(candidate.info.source_position,position,16);
 candidate.info.source_position[1]+=1.3f;
 candidate.info.position_mode=3;
 gh_voice_status status=prepare_pcm(s,bank,program,candidate.descriptor,&candidate);
 if(status)return status;
 candidate.used=1;
 candidate.priority=candidate.descriptor[8];
 candidate.group_first=candidate.descriptor[6];
 candidate.group_count=(uint8_t)(candidate.descriptor[7]+1);
 candidate.replace_equal=1;
 if(!++s->serial)++s->serial;
 candidate.info.original_handle=s->serial;
 candidate.next=s->head;
 s->voices[slot]=candidate;
 s->head=(int)slot;
 *handle=s->serial;
 return GH_VOICE_OK;
}

gh_voice_status gh_se_voice_update_ts_002cbf48(gh_se_voice_engine *s,uint32_t handle,
 const float position[4],uint16_t distance,int *found)
{
 if(!s||!position||!found)return GH_VOICE_ARGUMENT;
 s->frontier=0;*found=0;
 if(!handle)return GH_VOICE_OK;
 for(int index=s->head;index!=-1;index=s->voices[index].next){
  struct voice *voice=&s->voices[index];
  if(voice->info.original_handle!=handle)continue;
  if(!voice->ts_environment)return GH_VOICE_ARGUMENT;
  for(unsigned axis=0;axis<4;++axis)if(!isfinite(position[axis]))return unsupported(s,0x2c8b68);
  memcpy(voice->info.source_position,position,16);
  voice->ts_distance=distance;
  *found=1;
  break;
 }
 return GH_VOICE_OK;
}

static int source_inside(const uint8_t*base,size_t bytes,const uint8_t*ptr,size_t n){return base&&ptr&&(uintptr_t)ptr>=(uintptr_t)base&&(uintptr_t)ptr-(uintptr_t)base<=bytes&&n<=bytes-((uintptr_t)ptr-(uintptr_t)base);}
static gh_voice_status source_position(struct voice*v,int offset){const uint8_t*p;if(!v->model)return GH_VOICE_ARGUMENT;if(v->model->native_identity!=v->model||!v->model->bound)return GH_VOICE_ARGUMENT;if(v->part_index<0){p=v->model->root.alias_f0;if(!source_inside(v->model->root.bytes,v->model->root.size,p,16))return GH_VOICE_BOUNDS;}else{if((unsigned)v->part_index>=v->model->part_count)return GH_VOICE_BOUNDS;gh_actor_part*part=v->model->parts+v->part_index;p=part->alias_f0;if(!source_inside(part->bytes,0x220,p,16))return GH_VOICE_BOUNDS;}float position[4];memcpy(position,p,16);for(unsigned i=0;i<4;i++)if(!isfinite(position[i]))return GH_VOICE_UNSUPPORTED;if(offset){volatile float y=position[1]+1.3f;position[1]=y;if(!isfinite(position[1]))return GH_VOICE_UNSUPPORTED;}memcpy(v->info.source_position,position,16);return GH_VOICE_OK;}
static int remap_bank_program(int16_t program){int p=program;return (p>=200&&p<=232)||(p>=0xee&&p<=0xef)||(p>=0xf2&&p<=0xf3)||(p>=0xf8&&p<=0xf9)||p==0xea||p==0x109||p==0x10b;}
static int scene_door_open_profile(const struct voice *v)
{
 const uint8_t *p=v->descriptor;
 if(!v->source_owner||!v->model||!v->model->root.bytes||v->model->root.size<0x300)
  return 0;
 const uint16_t kind=h(v->model->root.bytes+0x2fe);
 const int r101=kind==0x313&&v->model->work&&v->model->work->room==0x101;
 /* 19D538 uses the common door cue with the exact R102.SES gain. */
 const int r102=kind==0x333&&v->model->work&&v->model->work->room==0x102&&
    v->model->root.size>=0x8f0&&v->model->root.original_vtable214==0x425d48;
 const uint32_t gain=r102?0x00640064u:r101?0x007f007fu:0x005a005au;
 return (kind==0x300||r101||r102)&&v->part_index==-1&&
    v->requested_bank==2&&v->requested_program==3&&v->info.bank==2&&v->info.program==3&&
    w(p)==0x00030002u&&w(p+4)==0x02170001u&&w(p+8)==0x00000250u&&
    w(p+12)==0x00200000u&&w(p+16)==gain&&w(p+20)==0x005a0040u&&
    w(p+24)==0x0080ff00u&&w(p+28)==0x00070000u&&w(p+32)==0x000a0004u&&
    w(p+36)==0x00000300u&&w(p+40)==0xffffu&&!w(p+44);
}

/* 1BC748, 1C2D60, 1881F0 and 18C580 use the room's own SES program 0. */
static unsigned room_prop_break_profile(const struct voice *v)
{
 const uint8_t *p=v->descriptor;
 if(!v->source_owner||!v->model||!v->model->work||!v->model->root.bytes||
    v->model->root.size<0x300||v->part_index!=-1||v->requested_bank!=2||v->info.bank!=2)return 0;
 const gh_title_player_view *root=&v->model->root;
 const unsigned room=v->model->work->room,kind=h(root->bytes+0x2fe);
 const uint32_t vtable=root->original_vtable214;
 const int om307=kind==0x307&&vtable==0x423608;
 const int om346=kind==0x346&&vtable==0x426e78&&root->size>=0xda0&&root->bytes[0x2f4]==2;
 if(room==0x100&&(om307||om346)&&v->requested_program==4&&v->info.program==4){
  /* 189818 and 1A17C8 select the same original R100.SES program. */
  return w(p)==0x00040002u&&w(p+4)==0x02170002u&&w(p+8)==0x254u&&w(p+12)==0x00200000u&&
    w(p+16)==0x007f007fu&&w(p+20)==0x005a0040u&&w(p+24)==0x0080fe80u&&
    !w(p+28)&&w(p+32)==0x00030004u&&w(p+36)==0x300u&&w(p+40)==0xffffu&&!w(p+44)?room:0;
 }
 if(v->requested_program||v->info.program)return 0;
 const int box=kind==0x30a&&vtable==0x4238d8;
 if(room==0x100){
  const int door=kind==0x300&&vtable==0x422f90;
  const int barrel=kind==0x305&&vtable==0x423400&&root->size>=0x5f8&&!h(root->bytes+0x5f6);
  if(!door&&!barrel&&!box)return 0;
 }else if(room==0x101){
  const int door=kind==0x313&&vtable==0x424128;
  if(!door&&!box&&!(kind==0x304&&vtable==0x423338))return 0;
 }else if(room==0x102){
  if(kind!=0x309||vtable!=0x4237e8)return 0;
 }else if(room==0x103){
  const int fixture=kind==0x33f&&vtable==0x4267f8;
  if(!box&&!fixture)return 0;
 }else return 0;
 const unsigned gain=room==0x100?120:127;
 if(w(p)!=2||w(p+4)!=0x02170000u||w(p+8)!=0x254u||w(p+12)!=0x00200000u||
    w(p+16)!=gain*0x10001u||w(p+20)!=0x005a0040u||w(p+24)!=0x0080ff00u||
    w(p+28)||w(p+32)!=0x00030004u||w(p+36)!=0x300u||w(p+40)!=0xffffu||w(p+44))
  return 0;
 return room;
}

static gh_voice_status attached_profile(gh_se_voice_engine*s,const uint8_t*p,int spatial_levels,
 int reverb_index,int alternate_pan)
{
 if(p[6]>=128||p[7]>=128||p[8]>=128||p[6]+(unsigned)p[7]+1>51)return unsupported(s,0x37cf00);
 if((!spatial_levels&&h(p+0x10)!=h(p+0x12))||h(p+0x10)>128||h(p+0x12)>128||
    h(p+0x14)!=64||h(p+0x16)!=(unsigned)alternate_pan||h(p+0x1c)||h(p+0x28)!=(unsigned)reverb_index||
    h(p+0x2a)||w(p+0x2c))return unsupported(s,0x2ce588);
 return GH_VOICE_OK;
}
/* 2CBF78 mode 2 compares signed EE ages and keeps the first equal-age node. */
static int youngest_attached_duplicate(const gh_se_voice_engine *s,
 const struct voice *candidate)
{
 int youngest=-1;
 uint32_t youngest_order=0;
 const uint32_t age_limit=UINT32_C(0x80000000)+candidate->descriptor[0x22];
 for(int i=s->head;i!=-1;i=s->voices[i].next){
  const struct voice *old=s->voices+i;
  uint32_t order=old->ee_age^UINT32_C(0x80000000);
  if(old->info.bank!=candidate->info.bank||old->info.program!=candidate->info.program||
     order>age_limit||(youngest>=0&&order>=youngest_order))continue;
  youngest=i;
  youngest_order=order;
 }
 return youngest;
}

/* 1F7E48 evaluates X²+Y², then adds Z² and takes the VU square root.
 * Keep each step binary32 under the surrounding request's numeric policy. */
static float duplicate_distance(const float listener[4],const float source[4])
{
 volatile float x=listener[0]-source[0];
 volatile float y=listener[1]-source[1];
 volatile float z=listener[2]-source[2];
 volatile float xx=x*x,yy=y*y,zz=z*z;
 volatile float xy=xx+yy;
 volatile float xyz=xy+zz;
 return sqrtf(xyz);
}

static int r103_roulette_windup_cue(const struct voice *voice)
{
 const gh_actor_model *model=voice->model;
 const uint8_t *row=voice->descriptor;
 const unsigned program=(unsigned)voice->info.program;
 const int heavy=program==0x120;
 return voice->source_owner&&!voice->requested_bank&&(heavy||program==0x3f)&&
    voice->requested_program==voice->info.program&&!voice->info.bank&&voice->part_index==-1&&
    model&&model->work&&model->work->room==0x103&&model->root.bytes&&
    model->root.size>=0x300&&model->root.original_vtable214==0x41d220&&
    h(model->root.bytes+0x2fe)==0x100&&h(model->root.bytes+0x2f4)==0x2300&&
    w(row)==program<<16&&w(row+4)==(heavy?0x01000092u:0x0114002bu)&&
    w(row+8)==(heavy?0x264u:0x25au)&&w(row+12)==0x00230000u&&
    w(row+16)==(heavy?0x00780078u:0x00640064u)&&w(row+20)==0x005a0040u&&
    !w(row+24)&&w(row+28)==0x00ff0000u&&w(row+32)==(heavy?0x00060005u:4u)&&
    !w(row+36)&&w(row+40)==0xffffu&&!w(row+44);
}

static gh_voice_status attached_duplicate(gh_se_voice_engine *s,struct voice *candidate,
 int *suppressed)
{
 *suppressed=0;
 if(!candidate->descriptor[0x22])return GH_VOICE_OK;
 if((candidate->info.bank!=2||candidate->info.program!=10||candidate->descriptor[0x22]!=3)&&
    !scene_door_open_profile(candidate)&&!room_prop_break_profile(candidate)&&!item_drop_profile(candidate)&&
    !core_grab_sequence(candidate)&&!core_heavy_recoil_sequence(candidate)&&!r103_dialogue_clip(s,candidate)&&!r103_roulette_windup_cue(candidate)&&
    !emad_room_loop(candidate,candidate->info.bank,candidate->info.program,candidate->descriptor)){
  if(!enemy_reaction_profile(candidate,candidate->info.bank,candidate->info.program,candidate->descriptor,NULL)&&
     !enemy_state71_sequence_profile(candidate,candidate->info.bank,candidate->info.program,candidate->descriptor,NULL)&&
     !enemy_local_cue_profile(candidate,candidate->info.bank,candidate->info.program,candidate->descriptor,NULL)&&
     !r103_paired_attack_voice_profile(candidate,candidate->info.bank,candidate->info.program,candidate->descriptor,NULL)&&
     !r100_devil_rise_cue(candidate,candidate->info.bank,candidate->info.program,candidate->descriptor,NULL)&&
     !r100_devil_idle_cue(candidate,candidate->info.bank,candidate->info.program,candidate->descriptor,NULL)&&
     !r100_devil_retreat_cue(candidate,candidate->info.bank,candidate->info.program,candidate->descriptor,NULL)&&
     !enemy_attack_sequence_profile(candidate,candidate->info.bank,candidate->info.program,candidate->descriptor,NULL)&&
     !em0a_attack_voice_profile(candidate,candidate->info.bank,candidate->info.program,candidate->descriptor,NULL)&&
     !enemy_death_cue_profile(candidate,candidate->info.bank,candidate->info.program,candidate->descriptor,NULL)&&
     !core_knockdown_profile(candidate,candidate->info.bank,candidate->info.program,candidate->descriptor,NULL)&&
     !core_enemy_attack_profile(candidate,candidate->info.bank,candidate->info.program,candidate->descriptor,NULL)&&
     !core_enemy_turn_profile(candidate,candidate->info.bank,candidate->info.program,candidate->descriptor,NULL)&&
     !centered_enemy_motion_profile(candidate,candidate->info.bank,candidate->info.program,candidate->descriptor,NULL)&&
     !em0a_timed_profile(candidate,candidate->info.bank,candidate->info.program,candidate->descriptor)&&
     !ema2_timer_cue_profile(candidate,candidate->info.bank,candidate->info.program,candidate->descriptor,NULL)&&
     !core_r101_weapon_player_impact_profile(candidate,candidate->info.bank,candidate->info.program,candidate->descriptor,NULL)&&
     !enemy_pressure_cue_profile(candidate,candidate->info.bank,candidate->info.program,candidate->descriptor,NULL)&&
     !core_fall_impact_profile(candidate,candidate->descriptor)&&
     !core_grab_impact_profile(candidate,candidate->descriptor)&&
     !r103_grab_impact_profile(candidate,candidate->descriptor)&&
     !r103_hurt5_impact_profile(candidate,candidate->descriptor,NULL)&&
     !r102_ticket_spawn_profile(candidate,candidate->descriptor)&&
     !enemy_scene_motion_sample(candidate,candidate->info.bank,candidate->info.program,candidate->descriptor)&&
     !boost_impact_voice(candidate,candidate->info.bank,candidate->info.program,candidate->descriptor,NULL)&&
     !player_active_heat_voice(candidate,candidate->info.bank,candidate->info.program,candidate->descriptor,NULL)&&
     !core_enemy_footstep_profile(candidate,candidate->info.bank,candidate->info.program,candidate->descriptor,NULL)&&
     !core_r102_weapon_contact(candidate,candidate->info.bank,candidate->info.program,candidate->descriptor,NULL)&&
     !weapon_swing_profile(candidate,candidate->info.bank,candidate->info.program,candidate->descriptor,NULL))
   return unsupported(s,0x2cbf78);
 }
 int index=youngest_attached_duplicate(s,candidate);
 if(index<0)return GH_VOICE_OK;
 const struct voice *old=s->voices+index;
 /* Original 2CDA80 sets bit0 after successful 2CE588. All currently
  * admitted producers use delay zero, including origin requests. */
 if(!old->info.pending||
    (old->info.source_position[0]==candidate->info.source_position[0]&&
     old->info.source_position[1]==candidate->info.source_position[1]&&
     old->info.source_position[2]==candidate->info.source_position[2])){
  *suppressed=1;
  return GH_VOICE_OK;
 }
 gh_se_attached_environment env={0};
 gh_voice_status status=candidate->probe(candidate->probe_context,
     candidate->info.source_position,0,&env,&s->frontier);
 if(status)return status;
 float listener[4];
 memcpy(listener,env.listener_position_c0,sizeof(listener));
 float old_distance=duplicate_distance(listener,old->info.source_position);
 float candidate_distance=duplicate_distance(listener,candidate->info.source_position);
 if(!isfinite(old_distance)||!isfinite(candidate_distance)||old->info.native_voice)
  return unsupported(s,0x2cbf78);
 if(old_distance<=candidate_distance)*suppressed=1;
 else drop(s,index,0); /* 2CE3E8 unlinks the older pending voice. */
 return GH_VOICE_OK;
}

static gh_voice_status attached_request_body(gh_se_voice_engine*s,const void*owner,const void*key,gh_actor_model*model,int32_t part,int16_t bank,int16_t program,uint32_t flags,int32_t delay,gh_se_attached_probe probe,void*probe_context,gh_se_attached_random random,void*random_context,uint32_t*handle){
 if(!owner||!key||!model||!probe||model->native_identity!=model||!model->bound||!model->parts_ready||!model->root.bytes||model->root.size<0x5ac||part < -1||(part>=0&&(uint32_t)part>=model->part_count))return GH_VOICE_ARGUMENT;
 if(flags||delay)return unsupported(s,0x2cdff0);
 unsigned index=64;for(unsigned i=0;i<64;i++)if(s->voices[i].used==2&&s->voices[i].source_owner==owner&&s->voices[i].request_key==key){index=i;break;}
 if(index==64){gh_sound_span a,b,c,d;gh_audio_status ready=gh_audio_bank_views(s->registry,bank,&a,&b,&c,&d);if(ready==GH_AUDIO_NOT_FOUND)return GH_VOICE_OK;if(ready)return unsupported(s,0x2cfc68);for(index=0;index<64&&s->voices[index].used;index++){}if(index==64)return GH_VOICE_OK;/* GenuineEEpoolfull BEFOREMT. */
  if(gh_actor_model_sound_borrow(model))return GH_VOICE_ARGUMENT;struct voice*v=s->voices+index;memset(v,0,sizeof(*v));v->used=2;v->info.channel=NONE;v->model=model;v->part_index=part;v->source_owner=owner;v->request_key=key;v->requested_bank=bank;v->requested_program=program;v->probe=probe;v->probe_context=probe_context;v->random=random;v->random_context=random_context;
  gh_voice_status positioned=source_position(v,1);if(positioned){drop(s,(int)index,0);s->frontier=0x2cdd18;return positioned;}
 }
 struct voice*v=s->voices+index;if(v->model!=model||v->part_index!=part||v->requested_bank!=bank||v->requested_program!=program||v->probe!=probe||v->probe_context!=probe_context||v->random!=random||v->random_context!=random_context)return GH_VOICE_ARGUMENT;
 if(!v->setup_stage){v->info.bank=bank;v->info.program=program;if(bank==0&&remap_bank_program(program)){v->info.bank=3;v->info.program=(int16_t)(program-200);}gh_audio_status got=gh_audio_get_program(s->registry,v->info.bank,v->info.program,0,v->descriptor);if(got==GH_AUDIO_BOUNDS||got==GH_AUDIO_DISABLED||got==GH_AUDIO_NOT_FOUND){drop(s,(int)index,0);return GH_VOICE_OK;}if(got)return unsupported(s,0x374cb8);v->variants=v->descriptor[0x21];v->setup_stage=1;}
 if(v->setup_stage==1){if(v->variants){if(!v->random)return unsupported(s,0x31cfe0);uint32_t word;gh_voice_status e=v->random(v->random_context,&word,&s->frontier);if(e){if(!s->frontier)s->frontier=0x31cfe0;return e;}v->info.program=(int16_t)(v->info.program+(int32_t)(word%((uint32_t)v->variants+1)));v->setup_stage=2;/* DRAWcommitted BEFORElookup/retry. */}else v->setup_stage=3;}
 if(v->setup_stage==2){gh_audio_status got=gh_audio_get_program(s->registry,v->info.bank,v->info.program,0,v->descriptor);if(got==GH_AUDIO_BOUNDS||got==GH_AUDIO_DISABLED||got==GH_AUDIO_NOT_FOUND){drop(s,(int)index,0);return GH_VOICE_OK;}if(got)return unsupported(s,0x374cb8);v->setup_stage=3;}
 if(v->setup_stage==3){
  if(v->variants&&v->descriptor[0x23])return unsupported(s,0x2cbd10);
  int suppressed;
  gh_voice_status duplicate=attached_duplicate(s,v,&suppressed);
  if(duplicate)return duplicate;
  if(suppressed){drop(s,(int)index,0);return GH_VOICE_OK;}
  v->setup_stage=4;
 }
 if(v->setup_stage==4){uint8_t*p=v->descriptor;gh_se_attached_environment env={0};gh_voice_status e=v->probe(v->probe_context,v->info.source_position,0,&env,&s->frontier);if(e)return e;v->info.position_mode=2;if(env.output_mode&&p[9]!=2){if(p[9]==3)v->info.position_mode=1;else if(p[9]<2)v->info.position_mode=0;}v->flags18=flags|p[0x20]|((uint32_t)p[0x24]<<8);v->flags1c=(unsigned)p[0x25]|(unsigned)p[0x26]<<8;if(v->flags1c&6)v->flags1c|=1;v->setup_stage=5;}
 const sequence_clip *clip=NULL;
 if(v->setup_stage==5){
  clip=r103_dialogue_clip(s,v);
  if(!clip&&(core_grab_sequence(v)||core_critical_sequence(v)||core_r102_sword_sequence(v)||core_roulette_sequence(v)||core_heavy_recoil_sequence(v)))
   clip=find_sequence_clip("se/core.afs",v->info.bank,v->info.program);
 }
 if(clip){
  if(!s->sequence.disc)return unsupported(s,0x2ca34c);
  if(v->flags18!=4||v->flags1c||v->info.position_mode!=(v->info.program==0x137?0u:2u))
   return unsupported(s,0x2ce588);
  v->external_channels=clip->channels;
  v->combat_profile=1;
  v->info.sample_slot=clip->row;
  v->info.rate=48000;
  v->info.pitch_q12=4096;
  v->info.pending=1;
  v->info.stop_current=0x7f00;
  v->priority=v->descriptor[8];
  v->group_first=v->descriptor[6];
  v->group_count=v->descriptor[7]+1;
  v->replace_equal=1;
  v->setup_stage=6;
 }
 if(v->setup_stage==5){gh_voice_status e=attached_profile(s,v->descriptor,core_enemy_footstep_profile(v,
    v->info.bank,v->info.program,v->descriptor,NULL)||core_close_impact_profile(v,v->descriptor),(r100_devil_spawn_cue(v,v->descriptor)||(!v->info.program&&emad_room_loop(v,v->info.bank,0,v->descriptor)))?0:65535,
    core_grab_impact_profile(v,v->descriptor)?270:90);
 if(e)return e;e=prepare_pcm(s,v->info.bank,v->info.program,v->descriptor,v);if(e)return e;v->priority=v->descriptor[8];v->group_first=v->descriptor[6];v->group_count=v->descriptor[7]+1;v->replace_equal=(v->descriptor[0xe]&0x20)!=0;/* Range is sampled later by actual37C2E0, never expose this intermediate PCM as finalpitch. */if(h(v->descriptor+0x18)||h(v->descriptor+0x1a))v->info.pitch_q12=0;v->setup_stage=6;}
 if(v->setup_stage==6){if(!++s->serial)++s->serial;v->info.original_handle=s->serial;v->request_key=NULL;v->setup_stage=0;v->random=NULL;v->random_context=NULL;v->used=1;v->next=s->head;s->head=(int)index;*handle=v->info.original_handle;}
 return GH_VOICE_OK;
}
static void note_attached_failure(gh_se_voice_engine *s,gh_se_attached_failure failure,
 const gh_actor_model *model)
{
 if(!s->frontier)return;
 failure.frontier=s->frontier;
 if(model&&model->root.bytes&&model->root.size>=0x300){
  failure.actor_kind=(uint16_t)h(model->root.bytes+0x2fe);
  if(failure.actor_kind>=0x200&&failure.actor_kind<0x300&&model->root.size>=0x568)
   failure.actor_dependency=w(model->root.bytes+0x564);
 }
 s->attached_failure=failure;
}

gh_voice_status gh_se_voice_request_attached(gh_se_voice_engine *s,const void *owner,
 const void *key,gh_actor_model *model,int32_t part,int16_t bank,int16_t program,
 uint32_t flags,int32_t delay,gh_se_attached_probe probe,void *probe_context,
 gh_se_attached_random random,void *random_context,uint32_t *handle)
{
 if(!s||!handle)return GH_VOICE_ARGUMENT;
 *handle=0;
 s->frontier=0;
 fenv_t env;
 if(feholdexcept(&env))return GH_VOICE_PLATFORM;
 if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return GH_VOICE_PLATFORM;}
 gh_voice_status status=attached_request_body(s,owner,key,model,part,bank,program,
     flags,delay,probe,probe_context,random,random_context,handle);
 if(fesetenv(&env))return GH_VOICE_PLATFORM;
 if(status)
  note_attached_failure(s,(gh_se_attached_failure){
      .flags=flags,.part_index=part,.delay=delay,
      .bank=bank,.program=program,.status=status
  },model);
 return status;
}

void gh_se_voice_last_attached_failure(const gh_se_voice_engine *s,gh_se_attached_failure *out)
{
 if(out)*out=s?s->attached_failure:(gh_se_attached_failure){0};
}
static int r100_spatial_profile(const struct voice *v)
{
 return v->source_owner&&v->info.position_mode==2&&
        ((v->info.bank==2&&v->info.program==10&&v->descriptor[0x22]==3)||v->door_spatial);
}
static int combat_voice_profile(const struct voice *v)
{
 return v->combat_profile;
}
static int voice_uses_ee_mix(const struct voice *v)
{
 return r100_spatial_profile(v)||combat_voice_profile(v)||v->emad_loop||v->ts_environment;
}
static int signed_half(const uint8_t *p){unsigned u=h(p);return u<0x8000?(int)u:(int)u-0x10000;}

/* Actual 2CF058 finite binary32 reference, with the existing 1F8180 leaf.
 * The listener producer supplies the same cSnd+C0/D0/110 fields. */
static gh_voice_status spatial_position(gh_se_voice_engine *s,const struct voice *v,
 const gh_se_attached_environment *env,unsigned *distance,unsigned *azimuth)
{
 float position[4],matrix[16];
 memcpy(position,env->listener_position_c0,sizeof(position));
 memcpy(matrix,env->listener_matrix_d0,sizeof(matrix));
 if(env->listener_mode110!=0&&env->listener_mode110!=1)return unsupported(s,0x2cf058);
 for(unsigned i=0;i<4;i++)if(!isfinite(position[i]))return unsupported(s,0x2cf058);
 for(unsigned i=0;i<16;i++)if(!isfinite(matrix[i]))return unsupported(s,0x2cf058);
 float local[2];
 const unsigned rows[]={0,2};
 for(unsigned i=0;i<2;i++){
  unsigned row=rows[i];
  local[i]=((matrix[row]*v->info.source_position[0]+matrix[row+4]*v->info.source_position[1])+
           matrix[row+8]*v->info.source_position[2])+matrix[row+12]*v->info.source_position[3];
 }
 float angle;
 uint32_t frontier;
 if(gh_game_angle_001f8180(local[1],local[0],&angle,GH_CAMERA_NATIVE_BINARY32_REFERENCE,&frontier))
  return unsupported(s,frontier?frontier:0x1f8180);
 if(angle<0)angle+=6.2831855f;
 unsigned degrees=(unsigned)(angle*57.295776f+.5f)%360;
 float scale=1;
 if(!env->listener_mode110){
  unsigned side=(degrees+270)%360;
  if(side>=51&&side<=309)scale=1+.008f*(float)(side<=180?side-50:310-side);
 }
 float x=position[0]-v->info.source_position[0],y=position[1]-v->info.source_position[1],
       z=position[2]-v->info.source_position[2];
 float length=sqrtf((x*x+y*y)+z*z)*scale;
 if(!isfinite(length))return unsupported(s,0x2cf058);
 *distance=length>=32000?32000:(unsigned)length;
 *azimuth=degrees;
 return GH_VOICE_OK;
}

static int span_fits(size_t size,size_t offset,size_t count)
{return offset<=size&&count<=size-offset;}

/* 3781F8/378398 follow the real 374B70 table pointer and preset selectors.
 * The current stereo path only reads the first two bytes of each preset. */
static gh_voice_status spatial_curve(gh_se_voice_engine *s,const struct voice *v,
 unsigned distance,unsigned pitch,int *out)
{
 uint32_t entry=pitch?0x378398:0x3781f8;
 gh_sound_span programs,sys,samples,afd;
 if(gh_audio_bank_views(s->registry,v->info.bank,&programs,&sys,&samples,&afd)||programs.size<32)
  return unsupported(s,entry);
 size_t offset=w(programs.data+0x10);
 if(!offset){*out=pitch?0:127;return GH_VOICE_OK;}
 if(!span_fits(programs.size,offset,16))return unsupported(s,entry);
 const uint8_t *curve=programs.data+offset;
 size_t size=programs.size-offset;
 unsigned preset=v->descriptor[0x1e];
 if(preset>=128||preset>=curve[0])return unsupported(s,entry);
 size_t preset_offset=(size_t)w(curve+4)+preset*4+pitch;
 if(!span_fits(size,preset_offset,1))return unsupported(s,entry);
 unsigned index=curve[preset_offset];
 if(index>=128){*out=pitch?0:127;return GH_VOICE_OK;}
 if(index>=curve[2+pitch])return unsupported(s,entry);
 size_t table=(size_t)w(curve+8+pitch*4)+index*8;
 if(!span_fits(size,table,8))return unsupported(s,entry);
 size_t points=w(curve+table);
 unsigned count=h(curve+table+6);
 if(!count||!span_fits(size,points,(size_t)count*4))return unsupported(s,entry);
 int previous_x=-1,previous_y=0;
 int found=0,value=0;
 for(unsigned i=0;i<count;i++){
  const uint8_t *point=curve+points+i*4;
  int x=signed_half(point),y=signed_half(point+2);
  if(x<=previous_x||(!i&&x)||(!pitch&&(y<0||y>127)))return unsupported(s,entry);
  if(!found&&(int)distance==x){found=1;value=y;}
  if(!found&&(int)distance<x){
   float slope=(float)(previous_y-y)/(float)(x-previous_x);
   value=previous_y-(int)(slope*(float)((int)distance-previous_x));
   found=1;
  }
  previous_x=x;
  previous_y=y;
 }
 *out=found?value:previous_y;
 return GH_VOICE_OK;
}
static unsigned approach(unsigned current,unsigned target,unsigned step)
{
 if(current<target)return target-current>step?current+step:target;
 return current-target>step?current-step:target;
}

/* Same integer master scaling and stereo split as 37D558. The output
 * still uses the documented native DSP envelope/resampler. */
static void apply_spatial_mix(const gh_se_voice_engine *s,struct voice *v)
{
 unsigned base=(unsigned)s->mix.se_level_q8*v->spatial_gain/127;
 v->base_gain=(uint16_t)base;
 unsigned level=base*(unsigned)v->info.stop_current/0x7f00/2;
 v->info.level_q14=(uint16_t)level;
 unsigned pan=v->info.pan;
 v->info.left_q14=(uint16_t)(pan>64?level*(127-pan)/64:level);
 v->info.right_q14=(uint16_t)(pan<64?level*pan/64:level);
 /* Original flag0200 applies the 0.3 factor after left/right calculation. */
 if(v->emad_occluded){
  v->info.left_q14=(uint16_t)(v->info.left_q14*3u/10u);
  v->info.right_q14=(uint16_t)(v->info.right_q14*3u/10u);
 }
}

/* 37C2E0 preserves the SDK random offset. 37D728 adds the current
 * command offset. A slow actor contributes -0xF00, not another draw. */
static gh_voice_status resolve_ee_pitch(gh_se_voice_engine *s,const struct voice *v,
 int slow,uint16_t *out)
{
 uint16_t bits=(uint16_t)((uint16_t)v->selected_offset+h(v->descriptor+0x1c)-(slow?0xf00u:0u));
 int16_t offset;
 memcpy(&offset,&bits,2);
 gh_voice_status status=gh_se_pitch_q12_000038e4(&s->pitch,offset,v->info.rate,out);
 if(status)s->frontier=0x38e4;
 return status;
}

/* 308D68 compares the pointer word with the same 747B20 list built
 * by scene visibility. Typed scroll handles preserve that identity. */
static int contains_scroll(const gh_game_scene_tables *tables,uint32_t handle)
{
 for(unsigned i=0;i<32&&tables->scroll_link_words[i];i++)
  if(tables->scroll_link_words[i]==handle)return 1;
 return 0;
}

/* Original 308DA8 static segment, with its own mask, flags and 8 reports. */
static gh_voice_status sound_occluded(gh_se_voice_engine *s,const struct voice *v,
 const gh_se_attached_environment *env,int *blocked)
{
 gh_game_scene_tables *tables=env->scene_tables;
 /* 12ED60 tests manager+0 before accessing geometry. The original
  * 12E800 room reset leaves detached voices with a valid empty owner. */
 if(!tables||(tables->akg.bytes&&!tables->actors_ready))return unsupported(s,0x308da8);
 gh_scene_ray_report reports[8]={0};
 gh_scene_ray_request request={.mask=7,.flags=0x12c,.report_limit=8,.native_reports=reports};
 memcpy(request.from,v->info.source_position,sizeof(request.from));
 memcpy(request.to,env->listener_position_c0,sizeof(request.to));
 int hit;
 uint32_t frontier;
 gh_scene_ray_status status=gh_scene_ray_query_direct_0012ed60(tables,&request,0,&hit,&frontier);
 if(tables->ray.reports18c==reports)tables->ray.reports18c=NULL;
 if(status)return unsupported(s,frontier?frontier:0x12ed60);
 *blocked=0;
 if(!hit)return GH_VOICE_OK;
 unsigned count=tables->ray.triangle17c;
 if(count>8)return unsupported(s,0x308da8);
 for(unsigned i=0;i<count;i++){
  if(!reports[i].attributes||reports[i].attribute_bytes<12)return unsupported(s,0x308da8);
  const uint8_t *attributes=reports[i].attributes;
  unsigned first=h(attributes+8),links=attributes[10];
  if(!links){*blocked=1;return GH_VOICE_OK;}
  for(unsigned j=0;j<links;j++){
   gh_actor_scroll_entry entry;
   if(first+j>UINT16_MAX||gh_scene_actor_scroll_entry_001333b8(tables,(uint16_t)(first+j),&entry))
    return unsupported(s,0x1333b8);
   if((uint32_t)entry.current_key)continue;
   if(!entry.resolved){
    if(entry.current_key)return unsupported(s,0x308d68);
    continue;
   }
   if(entry.scroll_handle&&!contains_scroll(tables,entry.scroll_handle)){
    *blocked=1;
    return GH_VOICE_OK;
   }
  }
 }
 return GH_VOICE_OK;
}

static gh_voice_status spatial_update(gh_se_voice_engine *s,struct voice *v,
 const gh_se_attached_environment *env)
{
 uint32_t entry=v->info.pending?0x2ce588:0x2cea00;
 if(env->output_mode!=1||env->se_level_q8!=s->mix.se_level_q8)return unsupported(s,0x375630);
 int combat=combat_voice_profile(v);
 /* 2CEA00 keeps updating live voices during the SaveMap fade. Only
  * pending 2CE588 switches its gain and records the 80000 voice flag. */
 if((!combat&&!v->emad_loop&&!v->ts_environment&&(v->flags18!=4||v->flags1c!=3))||env->reverb||
    (v->info.pending&&(env->system_flags_5e4&0x08000000u))||(env->game_flags_5d8&0x40u))
  return unsupported(s,entry);
 if(v->flags18&0x40u){
  if(!env->sound_flags_ac||!env->sound_flags_b0)return unsupported(s,0x2cda80);
  *env->sound_flags_ac|=0x04000000u;
  *env->sound_flags_b0|=0x04000000u;
 }
 if(v->info.pending&&(v->flags18&0x100u)){
  int blocked;
  gh_voice_status occlusion=sound_occluded(s,v,env,&blocked);
  if(occlusion)return occlusion;
  if(blocked){v->info.pending=0;return GH_VOICE_OK;}
 }
 int occluded=0;
 if(v->emad_loop&&(v->flags18&0x200u)){
  gh_voice_status status=sound_occluded(s,v,env,&occluded);
  if(status)return status;
 }
 unsigned distance=0,angle=0;
 int gain=(int)h(v->descriptor+0x10),pitch=0;
 gh_voice_status e;
 if(v->info.position_mode){
  e=spatial_position(s,v,env,&distance,&angle);
  if(e)return e;
  if(v->ts_environment&&!v->info.pending)distance=v->ts_distance;
  /* Original 3751B8 leaves the authored volume and pitch intact when
   * descriptor flags bit 0 is set. Position still controls stereo pan. */
  if(!(h(v->descriptor+0xe)&1u)){
   int attenuation;
   e=spatial_curve(s,v,distance,0,&attenuation);
   if(e)return e;
   gain=gain*attenuation/127;
   e=spatial_curve(s,v,distance,1,&pitch);
   if(e)return e;
  }
 }
 /* Both audited distance-pitch curves are identically zero. */
 if(pitch)return unsupported(s,0x378398);
 int slow=0;
 if(v->model&&(v->flags1c&1)){
  float speed;
  memcpy(&speed,v->model->root.bytes+0x5a8,4);
  if(!isfinite(speed))return unsupported(s,entry);
  /* 2CDA80 refreshes the bit at .5 every visit; pending 2CE588 can
   * additionally set it below 1 when Work58C is nonnegative. */
  slow=speed<.5f||(v->info.pending&&env->debug58c>=0&&speed<1);
 }
 if(!v->info.pending){
  float ratio=sound_float(env->gain64_bits);
  if(!(ratio>=0&&ratio<=1))return unsupported(s,entry);
  gain=(int)((float)gain*ratio);
 }
 if(slow&&(v->flags1c&2)){gain=(int)((float)gain*1.4f);if(gain>128)gain=128;}
 if(slow&&(v->flags1c&4))gain=0;
 unsigned folded=angle>=180?360-angle:angle;
 unsigned pan=v->info.position_mode?(unsigned)((float)folded*127.0f/180.0f+.5f):h(v->descriptor+0x14);
 if(!v->info.pending){
  gain=(int)approach(v->spatial_gain,(unsigned)gain,6);
  pan=approach(v->info.pan,pan,3);
 }
 /* Keep all visible parameters transactional if the native endpoint fails. */
 struct voice mixed={.info=v->info,.spatial_gain=(uint16_t)gain,.emad_occluded=(uint8_t)occluded,
   .source_owner=v->source_owner,.external_channels=v->external_channels};
 mixed.info.pan=(uint16_t)pan;
 apply_spatial_mix(s,&mixed);
 if(!v->info.pending&&v->info.native_voice){
  /* Mode 2 sets command mask 0xF and can update pitch. Mode 0's
   * gain/pan-only mask 0xB leaves the initial pitch intact. */
  if(v->info.position_mode==2&&slow!=v->spatial_slow){
   uint16_t next_pitch;
   e=resolve_ee_pitch(s,v,slow,&next_pitch);
   if(e)return e;
   if(next_pitch!=v->info.pitch_q12){
    e=gh_audio_output_voice_pitch_ratio(s->output,v->info.native_voice,next_pitch,v->submitted_pitch);
    if(e){s->frontier=0x374f30;return e;}
    mixed.info.pitch_q12=next_pitch;
   }
  }
  float left=(float)mixed.info.left_q14/16384.0f,right=(float)mixed.info.right_q14/16384.0f;
  if(v->external_channels){
   e=sequence_gain(s,&mixed,&left);
   if(e)return e;
   right=left;
  }
  e=gh_audio_output_voice_gain(s->output,v->info.native_voice,left,right);
  if(e){s->frontier=0x374f30;return e;}
 }
 mixed.info.distance=(uint16_t)distance;
 mixed.info.azimuth=(uint16_t)angle;
 v->spatial_slow=(uint8_t)slow;
 v->spatial_gain=(uint16_t)gain;
 v->emad_occluded=(uint8_t)occluded;
 v->base_gain=mixed.base_gain;
 v->info=mixed.info;
 return GH_VOICE_OK;
}

/* The original 2CEA00 mode-zero command keeps active mono voices live
 * while the save-map fade changes +64. Authenticate the observed player
 * footstep and the authored nine recordings of the EM00 CORE000A cue. */
static int r102_live_mono_gain_cue(const struct voice *v)
{
 if(v->info.pending||!v->info.native_voice||v->info.bank||v->part_index!=-1||
    !v->model||!v->model->work||v->model->work->room!=0x102||
    v->model->root.size<0x5ac||h(v->descriptor+0x14)!=64)return 0;
 const uint8_t *actor=v->model->root.bytes;
 const unsigned kind=h(actor+0x2fe);
 if(kind==0x100)
  return v->info.program==0x1e&&v->flags18==5&&v->flags1c==3&&
         w(v->descriptor+0x10)==0x00190019u;

 const int program=v->info.program;
 if(kind!=0x202||v->model->root.size<=0x1650||w(actor+0x564)!=0x200||
    h(actor+0x640)!=11||actor[0x1650]!=5||v->requested_bank||
    v->requested_program!=0x0a||program<0x0a||program>0x12||
    v->flags18!=4||v->flags1c)return 0;
 const unsigned authored=program==0x0f?120u:program==0x12?115u:110u;
 return h(v->descriptor+4)==(unsigned)(program-6)&&h(v->descriptor+0xe)==0x21&&
        w(v->descriptor+0x10)==authored*0x10001u;
}

/* 2CEBA8 scales the authored level by +64. 2CEEAC limits the change
 * to six command units while the native endpoint keeps its pitch. */
static gh_voice_status update_live_mono_gain(gh_se_voice_engine *s,struct voice *v,
 const gh_se_attached_environment *env,unsigned roulette_slow_flags)
{
 const float ratio=sound_float(env->gain64_bits);
 if(!(ratio>=0&&ratio<=1))return unsupported(s,0x2ceba8);
 const unsigned master=s->mix.se_level_q8;
 if(!master)return GH_VOICE_OK;
 /* The verified master can represent each 7-bit level distinctly. Recover
  * the last command from the existing pre-envelope master-scaled gain. */
 const unsigned previous=((unsigned)v->base_gain*127u+master/2u)/master;
 const unsigned authored=h(v->descriptor+0x10);
 /* 2CEA00's flag4 clears its retained gains before the six-unit
  * approach, so mode-zero mute begins immediately. Mode zero keeps pitch. */
 const int muted=(roulette_slow_flags&4u)!=0;
 unsigned target=(unsigned)nearbyintf((float)authored*ratio);
 if(roulette_slow_flags&2u){
  target=(unsigned)((float)(unsigned)((float)authored*ratio)*1.4f);
  if(target>128)target=128;
 }
 const unsigned command=muted?0:approach(previous,target,6);
 const uint16_t base=(uint16_t)(master*command/127u);
 if(base==v->base_gain)return GH_VOICE_OK;
 const uint16_t level=(uint16_t)((unsigned)base*(unsigned)v->info.stop_current/0x7f00u/2u);
 const float gain=(float)level/16384.0f;
 const gh_voice_status status=gh_audio_output_voice_gain(s->output,v->info.native_voice,gain,gain);
 if(status){s->frontier=0x374f30;return status;}
 v->base_gain=base;
 v->info.left_q14=v->info.right_q14=v->info.level_q14=level;
 return GH_VOICE_OK;
}

static gh_voice_status attached_update(gh_se_voice_engine *s,struct voice *v)
{
 if(v->ts_environment)return spatial_update(s,v,v->ts_environment);
 if(!v->source_owner)return GH_VOICE_OK;
 gh_voice_status status;
 if(!v->info.pending&&v->model){
  status=source_position(v,1);
  if(status){s->frontier=0x2cea00;return status;}
 }
 gh_se_attached_environment env={0};
 status=v->probe(v->probe_context,v->info.source_position,v->info.pending&&(v->flags18&1)!=0,&env,&s->frontier);
 if(status)return status;
 if(voice_uses_ee_mix(v))return spatial_update(s,v,&env);
 if(v->info.position_mode)return unsupported(s,0x2cf058);
 if(env.se_level_q8!=s->mix.se_level_q8)return unsupported(s,0x375630);
 if(env.output_mode!=1)return unsupported(s,0x37d680);
 /* Original Roulette sets the two global sound bits before attached
  * update. Existing active mode-zero mono voices keep their authored
  * pan and follow their own +1C gain flags, including boss footsteps. */
 const int roulette_cue=!v->info.pending&&v->info.native_voice&&
    !v->external_channels&&(v->flags18==4||v->flags18==5)&&(v->flags1c==3||v->flags1c==5)&&
    v->part_index==-1&&v->model&&v->model->work&&v->model->work->room==0x103&&
    h(v->descriptor+0x14)==64&&h(v->descriptor+0x10)==h(v->descriptor+0x12)&&
    ((env.sound_flags_ac&&env.sound_flags_b0&&
      (*env.sound_flags_ac&*env.sound_flags_b0&0x400000u))||v->spatial_slow);
 unsigned roulette_slow=0;
 const int live_gain=roulette_cue||(v->model?r102_live_mono_gain_cue(v):v->verified_live_gain);
 if((env.system_flags_5e4&0x08000000u)&&!live_gain)return unsupported(s,0x2ce588);
 if(env.game_flags_5d8&0x40u)return unsupported(s,0x2cda80);
 if(v->flags18&~5u)return unsupported(s,v->info.pending?0x2ce588:0x2cea00);
 if(v->info.pending&&env.reverb)return unsupported(s,0x2ce588);
 if(!v->info.pending&&(v->flags18&4)&&env.gain64_bits!=0x3f800000u&&!live_gain)
  return unsupported(s,0x2cea00);
 if(v->model){
  float speed;
  memcpy(&speed,v->model->root.bytes+0x5a8,4);
  if(!isfinite(speed)||(!roulette_cue&&(v->flags1c&1)&&
                       (speed<.5f||(v->info.pending&&env.debug58c>=0&&speed<1))))
   return unsupported(s,v->info.pending?0x2ce588:0x2cea00);
  roulette_slow=roulette_cue&&speed<.5f?v->flags1c:0;
 }
 if(!live_gain)return GH_VOICE_OK;
 status=update_live_mono_gain(s,v,&env,roulette_slow);
 if(!status){
  if(roulette_cue)v->spatial_slow=(uint8_t)(roulette_slow!=0);
  else v->verified_live_gain=1;
 }
 return status;
}
gh_voice_status gh_se_voice_detach_actor_002ce488(gh_se_voice_engine*s,gh_actor_model*model){if(!s||!model)return GH_VOICE_ARGUMENT;s->frontier=0;for(unsigned i=0;i<64;i++){struct voice*v=s->voices+i;if(v->used!=1||v->model!=model)continue;if(v->info.pending){gh_voice_status e=source_position(v,0);if(e){s->frontier=0x2ce488;return e;}}const int verified=v->verified_live_gain||r102_live_mono_gain_cue(v);
  if(gh_actor_model_sound_return(model))return GH_VOICE_ARGUMENT;
  /* 2CE488 clears the model reference. Active 2CEA00 mode zero keeps
   * using the same owner, descriptor and +64 gain until the voice ends. */
  v->verified_live_gain=(uint8_t)verified;
  v->model=NULL;v->part_index=-1;}return GH_VOICE_OK;}
gh_voice_status gh_se_voice_abort_attached_owner(gh_se_voice_engine *s,const void *owner)
{
 if(!s||!owner)return GH_VOICE_ARGUMENT;
 for(unsigned i=0;i<64;++i){
  struct voice *v=s->voices+i;
  if(!v->used||v->source_owner!=owner)continue;
  if(v->info.native_voice){
   gh_voice_status error=gh_audio_output_stop(s->output,v->info.native_voice);
   if(error&&error!=GH_VOICE_BOUNDS){s->frontier=0x2ce488;return error;}
  }
  drop(s,(int)i,0);
 }
 return GH_VOICE_OK;
}
unsigned gh_se_voice_attached_reservations(const gh_se_voice_engine*s){unsigned n=0;if(s)for(unsigned i=0;i<64;i++)n+=s->voices[i].used==2;return n;}
static gh_voice_status stop_command(gh_se_voice_engine *s,int index,int ticks)
{
 struct voice *v=s->voices+index;
 int active=0;
 uint64_t played=0;
 gh_voice_status error=gh_audio_output_active(s->output,v->info.native_voice,&active,&played);
 if(error&&error!=GH_VOICE_BOUNDS)return error;
 if(error==GH_VOICE_BOUNDS||!active){drop(s,index,error==GH_VOICE_OK);return GH_VOICE_OK;}
 if(ticks<=0||!played){
  error=gh_audio_output_stop(s->output,v->info.native_voice);
  if(error&&error!=GH_VOICE_BOUNDS)return error;
  if(v->info.channel<51&&s->channels[v->info.channel]==index)s->channels[v->info.channel]=-1;
  v->info.channel=NONE;
  v->info.native_voice=0;
  v->info.retiring=1;
  v->info.stopping=0;
 }else if(v->info.stop_current){
  const int step=-(int)v->info.stop_current/ticks;
  v->info.stop_step=(int16_t)(step?step:-1);
  v->info.stop_count=0;
  v->info.stopping=1;
 }
 return GH_VOICE_OK;
}

static gh_voice_status sequence_queued_stops(gh_se_voice_engine *s)
{
 /* Original 375050 reserves its shared command before this frame's EE
  * start commands. 37BE98 observes that order for the ADX link allocator. */
 for(unsigned q=0;q<s->stop_count;q++){
  for(int j=s->head;j!=-1;j=s->voices[j].next){
   struct voice *v=s->voices+j;
   if(!v->external_channels||v->info.original_handle!=s->stops[q].handle)continue;
   if(v->info.native_voice){
    gh_voice_status error=stop_command(s,j,s->stops[q].ticks);
    if(error){s->frontier=0x37d820;return error;}
   }
   s->stops[q].handle=0;
   break;
  }
 }
 return GH_VOICE_OK;
}

static int gate_close_voice(const struct voice *v)
{
 return !v->source_owner&&v->info.bank==2&&v->info.program==0x11&&
        r105_gate_close_descriptor(v->descriptor);
}

static gh_voice_status voice_update_body(gh_se_voice_engine *s,uint32_t *flags_ac,uint32_t *flags_b0)
{
 int i;unsigned issued=s?s->stop_count:0;if(!s)return GH_VOICE_ARGUMENT;s->frontier=0;i=s->head;
 /* 375380 ages existing links before37BE98 consumes this frame's queue.
  * Newly constructed37C160 links start at0 and must stay0 this visit. */
 for(unsigned c=0;c<51;++c)if(s->channels[c]!=-1){struct voice *old=s->voices+s->channels[c];if(old->age!=UINT32_MAX)++old->age;}
 gh_voice_status queued=sequence_queued_stops(s);
 if(queued)return queued;
 while(i!=-1){struct voice *v=s->voices+i;int next=v->next;
  if(!v->used){i=next;continue;}
  /* 2CDA80 publishes the descriptor's bit before checking the IOP reply. */
  if(!v->info.retiring&&gate_close_voice(v)){
   if(!flags_ac||!flags_b0)return unsupported(s,0x2cda80);
   *flags_b0|=UINT32_C(0x04000000);
   *flags_ac|=UINT32_C(0x04000000);
  }
  /* Submitting a newer voice reaps finished XAudio2 sources. Resolve the
   * endpoint lifetime before updating the older voice's attached gains. */
  if(!v->info.pending&&v->info.native_voice){
   int active;uint64_t played;
   gh_voice_status error=gh_audio_output_active(s->output,v->info.native_voice,&active,&played);
   if(error&&error!=GH_VOICE_BOUNDS)return error;
   if(error==GH_VOICE_BOUNDS||!active){drop(s,i,error==GH_VOICE_OK);i=next;continue;}
  }
  if(!v->info.retiring){
   gh_voice_status attached=attached_update(s,v);
   if(attached){
    note_attached_failure(s,(gh_se_attached_failure){.flags=v->flags18,
        .part_index=v->part_index,.bank=v->info.bank,.program=v->info.program,
        .during_update=1,.position_mode=v->info.position_mode,.status=attached},v->model);
    return attached;
   }
  }
  if(v->info.pending){
   /* Original shared SDE queue has48 entries. Overflowed EE voices observe
    * no SDE handle on the next update; don't invent extra queue capacity. */
   if(issued>=48){v->info.pending=0;i=next;continue;}++issued;
   unsigned first=v->group_first,end=first+v->group_count,channel;
   int replaced=-1;
   for(channel=first;channel<end && s->channels[channel]!=-1;++channel){}
   if(channel==end){
    unsigned minimum=255,picked=first;uint32_t age=0;
    for(unsigned c=first;c<end;++c){struct voice *old=s->voices+s->channels[c];if(old->priority<minimum)minimum=old->priority;}
    if(v->priority<minimum || (v->priority==minimum && !v->replace_equal)){v->info.pending=0;++v->ee_age;i=next;continue;}
    for(unsigned c=first;c<end;++c){struct voice *old=s->voices+s->channels[c];if(old->priority==minimum && age<=old->age){age=old->age;picked=c;}}
    channel=picked;
    replaced=s->channels[channel];
    if(!v->external_channels){drop(s,replaced,1);replaced=-1;}
   }
   if((h(v->descriptor+0x18)||h(v->descriptor+0x1a)||v->spatial_slow)&&v->pitch_setup<2){
    if(!s->pitch.loaded)return unsupported(s,0x37d760);
    if(!v->pitch_setup){
     int16_t low,high;
     uint16_t a=(uint16_t)h(v->descriptor+0x18),b=(uint16_t)h(v->descriptor+0x1a);
     memcpy(&low,&a,2);memcpy(&high,&b,2);
     gh_voice_status e=gh_se_pitch_choose_0037d760(&s->pitch,low,high,&v->selected_offset);
     if(e){s->frontier=0x37d760;return e;}
     v->pitch_setup=1;
    }
    gh_voice_status pitch=resolve_ee_pitch(s,v,v->spatial_slow,&v->selected_pitch);
    if(pitch)return pitch;
    if(!v->selected_pitch)return unsupported(s,0x38e4);gh_voice_status e=prepare_pcm_with_pitch(s,v->info.bank,v->info.program,v->descriptor,v,v->selected_pitch);if(e)return e;v->pitch_setup=2;
   }
   uint16_t left=v->info.level_q14,right=left;
   if(gate_close_voice(v))right=(uint16_t)(((uint32_t)v->origin_right_gain*(unsigned)v->info.stop_current/0x7f00u)/2u);
   if(voice_uses_ee_mix(v)){apply_spatial_mix(s,v);left=v->info.left_q14;right=v->info.right_q14;}
   float left_gain=(float)left/16384.0f,right_gain=(float)right/16384.0f;
   if(v->external_channels){
    /* 37CF00 uses the existing link table. 37BF10 interprets its final
     * three links as ADX sequence owners, separate from SPU voice DSP. */
    if(channel<48||channel>=51)return unsupported(s,0x37bf10);
    gh_voice_status error=GH_VOICE_OK;
    if(!v->pcm.samples)error=sequence_prepare(s,v);
    if(!error)error=sequence_gain(s,v,&left_gain);
    if(error)return error;
    right_gain=left_gain;
    if(replaced>=0){
     error=gh_audio_output_stop(s->output,s->voices[replaced].info.native_voice);
     if(error&&error!=GH_VOICE_BOUNDS){s->frontier=0x37d820;return error;}
     drop(s,replaced,0);
    }
   }
   gh_voice_status error=v->pcm.loop_frames&&!v->external_channels?
      gh_audio_output_play_q12(s->output,&v->pcm,v->info.pitch_q12,left_gain,right_gain,&v->info.native_voice):
      v->external_channels==2?
      gh_audio_output_play_stereo(s->output,&v->pcm,48000,left_gain,right_gain,&v->info.native_voice):
      gh_audio_output_play(s->output,&v->pcm,48000,left_gain,right_gain,&v->info.native_voice);
   if(error){s->frontier=0x374df8;return error;}
   gh_audio_pcm_release(&v->pcm);v->submitted_pitch=v->info.pitch_q12;v->info.pending=0;v->info.channel=(uint8_t)channel;s->channels[channel]=i;
  }else if(!v->info.native_voice)drop(s,i,0);
  if(v->used)++v->ee_age; /* actual EE+24 ADDIU/SW, modulo2^32 */
  i=next;
 }
 /* QueueStop is consumed before the per-node ramp scan, matching
  *375F68/37BE98 before37CAE0. SamplesPlayed is the explicit native busy
  * acknowledgement replacing asynchronous IOP feedback, not an EE oracle. */
 for(unsigned q=0;q<s->stop_count;q++)for(int j=s->head;j!=-1;j=s->voices[j].next){struct voice*v=s->voices+j;
  if(v->info.original_handle!=s->stops[q].handle||!v->info.native_voice)continue;
  gh_voice_status e=stop_command(s,j,s->stops[q].ticks);if(e)return e;
  break;
 }
 s->stop_count=0;
 for(int j=s->head;j!=-1;j=s->voices[j].next){struct voice*v=s->voices+j;if(!v->info.stopping||!v->info.native_voice)continue;
  if(!v->info.stop_current){gh_voice_status e=gh_audio_output_stop(s->output,v->info.native_voice);if(e&&e!=GH_VOICE_BOUNDS)return e;
   if(v->info.channel<51&&s->channels[v->info.channel]==j)s->channels[v->info.channel]=-1;
   v->info.channel=NONE;v->info.native_voice=0;v->info.retiring=1;v->info.stopping=0;
  }else{int next=v->info.stop_current+v->info.stop_step;if(next<0)next=0;v->info.stop_current=(int16_t)next;v->info.stop_count++;
   v->info.level_q14=(uint16_t)(((uint32_t)v->base_gain*(unsigned)next/0x7f00)/2);
   uint16_t left=v->info.level_q14,right=left;
   if(gate_close_voice(v))right=(uint16_t)(((uint32_t)v->origin_right_gain*(unsigned)v->info.stop_current/0x7f00u)/2u);
   if(voice_uses_ee_mix(v)){apply_spatial_mix(s,v);left=v->info.left_q14;right=v->info.right_q14;}
   float left_gain=(float)left/16384.0f,right_gain=(float)right/16384.0f;
   if(v->external_channels){
    gh_voice_status e=sequence_gain(s,v,&left_gain);if(e)return e;
    right_gain=left_gain;
   }
   gh_voice_status e=gh_audio_output_voice_gain(s->output,v->info.native_voice,left_gain,right_gain);
   if(e==GH_VOICE_BOUNDS)drop(s,j,0);else if(e)return e;
  }
 }
 return GH_VOICE_OK;
}
gh_voice_status gh_se_voice_update_with_sound_flags(gh_se_voice_engine*s,uint32_t *flags_ac,uint32_t *flags_b0)
{
 if(!s)return GH_VOICE_ARGUMENT;
 fenv_t env;
 if(feholdexcept(&env))return GH_VOICE_PLATFORM;
 if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return GH_VOICE_PLATFORM;}
 gh_voice_status status=voice_update_body(s,flags_ac,flags_b0);
 if(fesetenv(&env))return GH_VOICE_PLATFORM;
 if(status)return status;
 /* 2CA4D0 sends another stop after 375380/375500 consume the
  * current commands. The new command is therefore for the next update. */
 for(unsigned i=0;i<8;i++){
  uint32_t handle=s->actor_stop_retry[i];
  if(!handle)continue;
  int accepted;
  status=gh_se_voice_request_stop_00375050(s,handle,5,&accepted);
  if(status)return status;
  s->actor_stop_retry[i]=0;
 }
 return GH_VOICE_OK;
}
gh_voice_status gh_se_voice_update(gh_se_voice_engine*s)
{
 return gh_se_voice_update_with_sound_flags(s,NULL,NULL);
}
gh_voice_status gh_se_voice_query(const gh_se_voice_engine *s,uint32_t handle,gh_se_voice_info *out)
{if(!s || !handle || !out)return GH_VOICE_ARGUMENT;for(unsigned i=0;i<64;++i)if(s->voices[i].used && s->voices[i].info.original_handle==handle){*out=s->voices[i].info;out->attached_model=s->voices[i].model;out->attached=s->voices[i].model!=NULL;out->attached_part=s->voices[i].model?s->voices[i].part_index:-1;return GH_VOICE_OK;}return GH_VOICE_BOUNDS;}
uint32_t gh_se_voice_frontier(const gh_se_voice_engine *s){return s?s->frontier:0;}
gh_voice_status gh_se_voice_pcm_view(const gh_se_voice_engine *s,uint32_t handle,const int16_t **samples,size_t *frames)
{
 if(!s || !handle || !samples || !frames)return GH_VOICE_ARGUMENT;
 for(unsigned i=0;i<64;++i)if(s->voices[i].used && s->voices[i].info.original_handle==handle){
  if(!s->voices[i].info.pending)return GH_VOICE_BOUNDS;
  if(!s->voices[i].info.pitch_q12||s->voices[i].external_channels==2)return GH_VOICE_UNSUPPORTED;
  if(s->voices[i].external_channels&&!s->voices[i].pcm.samples)return GH_VOICE_BOUNDS;
  *samples=s->voices[i].pcm.samples;*frames=s->voices[i].pcm.frames;return GH_VOICE_OK;
 }
 return GH_VOICE_BOUNDS;
}

unsigned gh_se_voice_external_adx_nodes(const gh_se_voice_engine *s)
{
 unsigned count=0;
 if(s)for(unsigned i=0;i<64;i++)
  count+=s->voices[i].used&&s->voices[i].external_channels;
 return count;
}
unsigned gh_se_voice_active_nodes(const gh_se_voice_engine*s){unsigned n=0;if(s)for(unsigned i=0;i<64;i++)n+=s->voices[i].used==1;return n;}
gh_voice_status gh_se_voice_request_stop_00375050(gh_se_voice_engine*s,uint32_t handle,int16_t ticks,int*accepted){
 if(!s||!handle||!accepted)return GH_VOICE_ARGUMENT;*accepted=0;s->frontier=0;
 if(ticks < -1)return unsupported(s,0x378020);
 /* Zero/finished ramp sets the IOP 3998 fast-release override. A -1
  * key-off keeps the sample's release rate and needs a native tail. */
 if(ticks==-1)for(int i=s->head;i!=-1;i=s->voices[i].next){
  const struct voice *voice=s->voices+i;
  if(voice->info.original_handle==handle&&voice->info.adsr2==0x5fcb&&!voice->info.retiring)
   return unsupported(s,0x37d820);
 }
 if(s->stop_count==48)return GH_VOICE_OK;
 s->stops[s->stop_count].handle=handle;s->stops[s->stop_count].ticks=ticks;s->stop_count++;*accepted=1;return GH_VOICE_OK;
}
gh_voice_status gh_se_voice_stop_actor_002cbba0(gh_se_voice_engine *s,uint32_t handle)
{
 if(!s)return GH_VOICE_ARGUMENT;
 if(!handle)return GH_VOICE_OK;
 int accepted;
 gh_voice_status status=gh_se_voice_request_stop_00375050(s,handle,5,&accepted);
 if(status)return status;
 /* Original retries even if the SDK command slot was exhausted. */
 for(unsigned i=0;i<8;i++){
  if(s->actor_stop_retry[i])continue;
  s->actor_stop_retry[i]=handle;
  break;
 }
 return GH_VOICE_OK;
}

gh_voice_status gh_se_voice_stop_bank_002cbc58(gh_se_voice_engine*s,int16_t bank,int16_t ticks){
 if(!s||bank<0||bank>=52)return GH_VOICE_ARGUMENT;if(ticks < -1)return unsupported(s,0x378020);s->frontier=0;
 /* Check before dropping pending nodes or publishing any stop commands. */
 if(ticks==-1)for(int i=s->head;i!=-1;i=s->voices[i].next){
  const struct voice *voice=s->voices+i;
  if(voice->info.bank==bank&&voice->info.native_voice&&voice->info.adsr2==0x5fcb)
   return unsupported(s,0x37d820);
 }
 for(int i=s->head;i!=-1;){struct voice*v=s->voices+i;int next=v->next;
  if(v->info.bank==bank){if(v->info.native_voice||v->info.retiring){int accepted;gh_voice_status e=gh_se_voice_request_stop_00375050(s,v->info.original_handle,ticks,&accepted);if(e)return e;/* Originalvoid ignores queuefull. */}else drop(s,i,0);}i=next;
 }return GH_VOICE_OK;
}
gh_voice_status gh_se_voice_scene_exit_begin(gh_se_voice_engine*s){
 if(!s)return GH_VOICE_ARGUMENT;s->frontier=0;
 for(int bank=0;bank<52;bank++){gh_voice_status e=gh_se_voice_stop_bank_002cbc58(s,(int16_t)bank,10);if(e)return e;}
 for(int i=s->head;i!=-1;){struct voice*v=s->voices+i;int next=v->next;if(!v->info.native_voice&&!v->info.retiring)drop(s,i,0);i=next;}
 return GH_VOICE_OK;
}

int gh_se_voice_uses_registry(const gh_se_voice_engine*s,const gh_audio_registry*r){return s&&r&&s->registry==r;}

unsigned gh_se_voice_bank_nodes(const gh_se_voice_engine*s,int16_t bank){unsigned n=0;if(s)for(unsigned i=0;i<64;i++)n+=s->voices[i].used==1&&s->voices[i].info.bank==bank;return n;}

/* Native sound SDK setup, once before voices: own tables and original RNG,
 * loaded through the SAME disc; no module execution or DSP emulation. */
gh_voice_status gh_se_voice_bind_pitch_reference(gh_se_voice_engine*s,gh_disc*d){if(!s||!d)return GH_VOICE_ARGUMENT;if(s->pitch.loaded||s->head!=-1||gh_se_voice_attached_reservations(s))return GH_VOICE_UNSUPPORTED;return gh_se_pitch_load_reference(&s->pitch,d);}
gh_voice_status gh_se_voice_pitch_state(const gh_se_voice_engine*s,uint16_t*out){if(!s||!out)return GH_VOICE_ARGUMENT;if(!s->pitch.loaded)return GH_VOICE_UNSUPPORTED;*out=s->pitch.random_state;return GH_VOICE_OK;}
