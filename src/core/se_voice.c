/* Recovered MC origin SE slice; upstream eaae18d0dfab1f9a0df276ae5ca1581ad326faf3
 * MIT LICENSE.upstream. Actual ELF and user's snddrv.irx evidence:
 * analysis/se-voice-semantics.json. Native DSP adaptation is explicit. */
#include "se_voice.h"
#include "game_actor_model.h"
#include "../assets/se_pitch.h"
#include <math.h>
#include <fenv.h>
#include <stdlib.h>
#include <string.h>
#define NONE 255
struct voice {gh_se_voice_info info;gh_adpcm_pcm pcm;uint32_t age,ee_age;uint16_t base_gain;int next;uint8_t used,priority,group_first,group_count,replace_equal;
 gh_actor_model *model;int32_t part_index;const void *source_owner,*request_key;
 gh_se_attached_probe probe;void *probe_context;gh_se_attached_random random;void *random_context;
 uint8_t descriptor[48],setup_stage,variants;int16_t requested_bank,requested_program;
 uint32_t flags18,flags1c;uint16_t selected_pitch;uint8_t pitch_setup;};
struct gh_se_voice_engine {const gh_audio_registry *registry;gh_audio_output *output;gh_se_mix mix;struct voice voices[64];int channels[51],head;uint32_t serial,frontier;const void *origin_seh;size_t origin_seh_size;uint8_t origin_scene_bound;struct {uint32_t handle;int16_t ticks;} stops[48];unsigned stop_count;const void *attached_bound_owner;gh_se_pitch_reference pitch;};
static unsigned h(const uint8_t *p){return (unsigned)p[0]|(unsigned)p[1]<<8;}
static uint32_t w(const uint8_t *p){return (uint32_t)h(p)|(uint32_t)h(p+2)<<16;}
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
/* Fixed actual80FF/5FC0 envelope, in48000Hz ticks. Native linear resampling
 * replaces SPU interpolation. Independent arithmetic implementation; reference
 * envelope timing is pinned PCSX2 mathematical model, not hardware capture. */
static gh_voice_status render(gh_adpcm_pcm *pcm,uint16_t pitch)
{
 uint64_t count;int16_t *out;
 uint32_t env=0,counter=0,phase=1;
 if(pcm->frames>(UINT64_MAX-pitch+1)/4096)return GH_VOICE_BOUNDS;
 count=((uint64_t)pcm->frames*4096+pitch-1)/pitch;
 if(count>SIZE_MAX/sizeof(*out))return GH_VOICE_BOUNDS;
 out=malloc((size_t)count*sizeof(*out));if(!out)return GH_VOICE_MEMORY;
 for(uint64_t i=0;i<count;++i){
  uint64_t pos=i*pitch;size_t index=(size_t)(pos>>12);unsigned fraction=(unsigned)(pos&4095);int32_t a=pcm->samples[index],b=index+1<pcm->frames?pcm->samples[index+1]:0,value;
  if(phase==1){counter+=env>0x6000?8192:32768;if(counter>=32768){counter=0;env+=14336;if(env>=32767){env=32767;phase=2;}}}
  else if(phase==2){counter+=2048;if(counter>=32768){counter=0;env-=(env*8+32767)/32768;}phase=3;}
  else {++counter;if(counter>=32768){counter=0;env=env>=5?env-5:0;}}
  value=floor_shift((int64_t)a*(4096-fraction)+(int64_t)b*fraction,12);
  out[i]=(int16_t)floor_shift((int64_t)value*env,15);
 }
 free(pcm->samples);pcm->samples=out;pcm->frames=(size_t)count;return GH_VOICE_OK;
}
static gh_voice_status prepare_pcm_with_pitch(gh_se_voice_engine*s,int16_t bank,int16_t program,const uint8_t*p,struct voice*candidate,uint16_t selected_pitch){
 gh_audio_pcm_release(&candidate->pcm);
 unsigned slot=h(p+4),rate,level;gh_sound_span descriptor,tail;gh_voice_status result;
 slot=h(p+4);if(gh_audio_sample_at(s->registry,bank,slot,&descriptor,&tail)!=GH_AUDIO_OK)return unsupported(s,0x35fc);
 if(descriptor.size!=16 || h(descriptor.data+8)!=0x80ff || h(descriptor.data+10)!=0x5fc0 || w(descriptor.data+12))return unsupported(s,0x35fc);
 rate=w(descriptor.data+4);if(rate<1000 || rate>48000)return unsupported(s,0x38e4);
 candidate->info.pitch_q12=selected_pitch?selected_pitch:(uint16_t)((uint64_t)rate*4096/48000);
 result=gh_audio_decode_ps_adpcm(tail.data,tail.size,&candidate->pcm);if(result){s->frontier=0x35fc;return result;}
 result=render(&candidate->pcm,candidate->info.pitch_q12);if(result){gh_audio_pcm_release(&candidate->pcm);return result;}
 level=(unsigned)((uint64_t)s->mix.se_level_q8*(h(p+0x10)<<8)/0x7f00);
 candidate->base_gain=(uint16_t)level;level/=2;
 candidate->info.stop_current=0x7f00;
 candidate->info.level_q14=(uint16_t)level;candidate->info.rate=rate;candidate->info.sample_slot=slot;candidate->info.frames=(uint32_t)candidate->pcm.frames;
 candidate->info.adsr1=0x80ff;candidate->info.adsr2=0x5fc0;candidate->info.program=program;candidate->info.bank=bank;candidate->info.pending=1;candidate->info.channel=NONE;
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
void gh_se_voice_destroy(gh_se_voice_engine *s){if(s){for(int i=0;i<64;++i)if(s->voices[i].used)drop(s,i,1);free(s);}}
gh_voice_status gh_se_voice_request_origin(gh_se_voice_engine *s,int16_t bank,int16_t program,int16_t arg3,int16_t arg4,uint32_t flags,int32_t delay,uint32_t *handle)
{
 uint8_t p[48];gh_voice_status result;unsigned i;struct voice candidate={0};
 if(!s || !handle)return GH_VOICE_ARGUMENT;*handle=0;s->frontier=0;
 int title=bank==2&&(program==0||(program>=10&&program<=12));
 if((!title&&(bank || (program!=0x2d && program!=0x15e && program!=0x160 && program!=0x161))) || arg3!=-1 || arg4!=-1 || flags || delay)return unsupported(s,0x2cb8a0);
 if(s->mix.system_flags&0x08000040)return unsupported(s,0x2ce588);
 if(gh_audio_get_program(s->registry,bank,program,0,p)!=GH_AUDIO_OK)return unsupported(s,0x374cb8);
 /* Exact audited subsets; +21 controls random variants, +22 duplicate age
  * window. +09 is origin/output selection, not variant count. */
 if(title){
  if(!s->origin_scene_bound||s->origin_seh||s->origin_seh_size)return unsupported(s,0x2cae10);
  if(program==0){if(p[6]!=10||p[7]!=4||p[8]!=120||p[9]||h(p+0xe)!=0x20||h(p+0x10)!=90||w(p+0x20)!=0x30000||w(p+0x24)!=0x300||h(p+0x1e))return unsupported(s,0x2cdff0);}
  else if(p[6]!=20||p[7]!=4||p[8]!=122||p[9]||h(p+0xe)!=0x23||h(p+0x10)!=120||w(p+0x20)||w(p+0x24)||h(p+0x1e)!=255)return unsupported(s,0x2cdff0);
 }else if(program==0x2d){
  if(p[6]!=15||p[7]!=1||p[8]!=104||p[9]!=2||h(p+0xe)!=0x21||w(p+0x20)!=0x00030005||h(p+0x10)!=90)return unsupported(s,0x2cdff0);
  if(!s->origin_scene_bound||s->origin_seh||s->origin_seh_size)return unsupported(s,0x2cae10);
 }else if(p[6]||p[7]!=4||p[8]!=120||p[9]||h(p+0xe)!=0x23||w(p+0x20))return unsupported(s,0x2cdff0);
 if(h(p+0x10)!=h(p+0x12)||h(p+0x14)!=64||h(p+0x16)!=90||w(p+0x18)||h(p+0x1c)||h(p+0x28)!=65535||h(p+0x2a)||w(p+0x2c))return unsupported(s,0x2cdff0);
 if(!title&&(h(p+0x1e)!=255||w(p+0x24)))return unsupported(s,0x2cdff0);
 /* Actual2CBF78 compares bank AND program; duplicate-age suppression for
  * equal-origin zero-delay is shared by logo and room confirmation0. */
 if(p[0x22])for(int j=s->head;j!=-1;j=s->voices[j].next)if(s->voices[j].info.bank==bank&&s->voices[j].info.program==program&&(s->voices[j].ee_age<=p[0x22]||s->voices[j].ee_age>=UINT32_C(0x80000000)))return GH_VOICE_OK;
 for(i=0;i<64 && s->voices[i].used;++i){}if(i==64)return GH_VOICE_OK;
 result=prepare_pcm(s,bank,program,p,&candidate);if(result)return result;
 if(!++s->serial)++s->serial;candidate.info.original_handle=s->serial;candidate.next=s->head;candidate.used=1;candidate.priority=p[8];candidate.group_first=p[6];candidate.group_count=p[7]+1;candidate.replace_equal=(p[0xe]&0x20)!=0;s->voices[i]=candidate;s->head=(int)i;*handle=s->serial;return GH_VOICE_OK;
}
/* Actor-attached EE construction; existingPCM/DSP/channel/output owner above. */
static int source_inside(const uint8_t*base,size_t bytes,const uint8_t*ptr,size_t n){return base&&ptr&&(uintptr_t)ptr>=(uintptr_t)base&&(uintptr_t)ptr-(uintptr_t)base<=bytes&&n<=bytes-((uintptr_t)ptr-(uintptr_t)base);}
static gh_voice_status source_position(struct voice*v,int offset){const uint8_t*p;if(!v->model)return GH_VOICE_ARGUMENT;if(v->model->native_identity!=v->model||!v->model->bound)return GH_VOICE_ARGUMENT;if(v->part_index<0){p=v->model->root.alias_f0;if(!source_inside(v->model->root.bytes,v->model->root.size,p,16))return GH_VOICE_BOUNDS;}else{if((unsigned)v->part_index>=v->model->part_count)return GH_VOICE_BOUNDS;gh_actor_part*part=v->model->parts+v->part_index;p=part->alias_f0;if(!source_inside(part->bytes,0x220,p,16))return GH_VOICE_BOUNDS;}float position[4];memcpy(position,p,16);for(unsigned i=0;i<4;i++)if(!isfinite(position[i]))return GH_VOICE_UNSUPPORTED;if(offset){volatile float y=position[1]+1.3f;position[1]=y;if(!isfinite(position[1]))return GH_VOICE_UNSUPPORTED;}memcpy(v->info.source_position,position,16);return GH_VOICE_OK;}
static int remap_bank_program(int16_t program){int p=program;return (p>=200&&p<=232)||(p>=0xee&&p<=0xef)||(p>=0xf2&&p<=0xf3)||(p>=0xf8&&p<=0xf9)||p==0xea||p==0x109||p==0x10b;}
static gh_voice_status attached_profile(gh_se_voice_engine*s,const uint8_t*p){if(p[6]>=128||p[7]>=128||p[8]>=128||p[6]+(unsigned)p[7]+1>51)return unsupported(s,0x37cf00);if(h(p+0x10)!=h(p+0x12)||h(p+0x10)>128||h(p+0x14)!=64||h(p+0x16)!=90||h(p+0x1c)||h(p+0x28)!=65535||h(p+0x2a)||w(p+0x2c))return unsupported(s,0x2ce588);return GH_VOICE_OK;}
static gh_voice_status attached_request_body(gh_se_voice_engine*s,const void*owner,const void*key,gh_actor_model*model,int32_t part,int16_t bank,int16_t program,uint32_t flags,int32_t delay,gh_se_attached_probe probe,void*probe_context,gh_se_attached_random random,void*random_context,uint32_t*handle){
 if(!owner||!key||!model||!probe||model->native_identity!=model||!model->bound||!model->parts_ready||!model->root.bytes||model->root.size<0x5ac||part < -1||(part>=0&&(uint32_t)part>=model->part_count))return GH_VOICE_ARGUMENT;
 if(flags||delay)return unsupported(s,0x2cdff0);if(s->attached_bound_owner&&s->attached_bound_owner!=owner)return GH_VOICE_ARGUMENT;
 unsigned index=64;for(unsigned i=0;i<64;i++)if(s->voices[i].used==2&&s->voices[i].source_owner==owner&&s->voices[i].request_key==key){index=i;break;}
 if(index==64){gh_sound_span a,b,c,d;gh_audio_status ready=gh_audio_bank_views(s->registry,bank,&a,&b,&c,&d);if(ready==GH_AUDIO_NOT_FOUND)return GH_VOICE_OK;if(ready)return unsupported(s,0x2cfc68);for(index=0;index<64&&s->voices[index].used;index++){}if(index==64)return GH_VOICE_OK;/* GenuineEEpoolfull BEFOREMT. */
  if(gh_actor_model_sound_borrow(model))return GH_VOICE_ARGUMENT;struct voice*v=s->voices+index;memset(v,0,sizeof(*v));v->used=2;v->info.channel=NONE;v->model=model;v->part_index=part;v->source_owner=owner;v->request_key=key;v->requested_bank=bank;v->requested_program=program;v->probe=probe;v->probe_context=probe_context;v->random=random;v->random_context=random_context;s->attached_bound_owner=owner;
  gh_voice_status positioned=source_position(v,1);if(positioned){drop(s,(int)index,0);s->frontier=0x2cdd18;return positioned;}
 }
 struct voice*v=s->voices+index;if(v->model!=model||v->part_index!=part||v->requested_bank!=bank||v->requested_program!=program||v->probe!=probe||v->probe_context!=probe_context||v->random!=random||v->random_context!=random_context)return GH_VOICE_ARGUMENT;
 if(!v->setup_stage){v->info.bank=bank;v->info.program=program;if(bank==0&&remap_bank_program(program)){v->info.bank=3;v->info.program=(int16_t)(program-200);}gh_audio_status got=gh_audio_get_program(s->registry,v->info.bank,v->info.program,0,v->descriptor);if(got==GH_AUDIO_BOUNDS||got==GH_AUDIO_DISABLED||got==GH_AUDIO_NOT_FOUND){drop(s,(int)index,0);return GH_VOICE_OK;}if(got)return unsupported(s,0x374cb8);v->variants=v->descriptor[0x21];v->setup_stage=1;}
 if(v->setup_stage==1){if(v->variants){if(!v->random)return unsupported(s,0x31cfe0);uint32_t word;gh_voice_status e=v->random(v->random_context,&word,&s->frontier);if(e){if(!s->frontier)s->frontier=0x31cfe0;return e;}v->info.program=(int16_t)(v->info.program+(int32_t)(word%((uint32_t)v->variants+1)));v->setup_stage=2;/* DRAWcommitted BEFORElookup/retry. */}else v->setup_stage=3;}
 if(v->setup_stage==2){gh_audio_status got=gh_audio_get_program(s->registry,v->info.bank,v->info.program,0,v->descriptor);if(got==GH_AUDIO_BOUNDS||got==GH_AUDIO_DISABLED||got==GH_AUDIO_NOT_FOUND){drop(s,(int)index,0);return GH_VOICE_OK;}if(got)return unsupported(s,0x374cb8);v->setup_stage=3;}
 if(v->setup_stage==3){uint8_t*p=v->descriptor;if(v->variants&&p[0x23])return unsupported(s,0x2cbd10);if(p[0x22])return unsupported(s,0x2cbf78);gh_se_attached_environment env={0};gh_voice_status e=v->probe(v->probe_context,v->info.source_position,0,&env,&s->frontier);if(e)return e;v->info.position_mode=2;if(env.output_mode&&p[9]!=2){if(p[9]==3)v->info.position_mode=1;else if(p[9]<2)v->info.position_mode=0;}v->flags18=flags|p[0x20]|((uint32_t)p[0x24]<<8);v->flags1c=(unsigned)p[0x25]|(unsigned)p[0x26]<<8;if(v->flags1c&6)v->flags1c|=1;v->setup_stage=4;}
 if(v->setup_stage==4){gh_voice_status e=attached_profile(s,v->descriptor);if(e)return e;e=prepare_pcm(s,v->info.bank,v->info.program,v->descriptor,v);if(e)return e;v->priority=v->descriptor[8];v->group_first=v->descriptor[6];v->group_count=v->descriptor[7]+1;v->replace_equal=(v->descriptor[0xe]&0x20)!=0;/* Range is sampled later by actual37C2E0, never expose this intermediate PCM as finalpitch. */if(h(v->descriptor+0x18)||h(v->descriptor+0x1a))v->info.pitch_q12=0;v->setup_stage=5;}
 if(v->setup_stage==5){if(!++s->serial)++s->serial;v->info.original_handle=s->serial;v->request_key=NULL;v->setup_stage=0;v->random=NULL;v->random_context=NULL;v->used=1;v->next=s->head;s->head=(int)index;*handle=v->info.original_handle;}
 return GH_VOICE_OK;
}
gh_voice_status gh_se_voice_request_attached(gh_se_voice_engine*s,const void*owner,const void*key,gh_actor_model*model,int32_t part,int16_t bank,int16_t program,uint32_t flags,int32_t delay,gh_se_attached_probe probe,void*probe_context,gh_se_attached_random random,void*random_context,uint32_t*handle){if(!s||!handle)return GH_VOICE_ARGUMENT;*handle=0;s->frontier=0;fenv_t env;if(feholdexcept(&env))return GH_VOICE_PLATFORM;if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return GH_VOICE_PLATFORM;}gh_voice_status e=attached_request_body(s,owner,key,model,part,bank,program,flags,delay,probe,probe_context,random,random_context,handle);if(fesetenv(&env))return GH_VOICE_PLATFORM;return e;}
static gh_voice_status attached_update(gh_se_voice_engine*s,struct voice*v){if(!v->source_owner)return GH_VOICE_OK;gh_voice_status e;if(!v->info.pending&&v->model){e=source_position(v,1);if(e){s->frontier=0x2cea00;return e;}}gh_se_attached_environment env={0};e=v->probe(v->probe_context,v->info.source_position,v->info.pending&&(v->flags18&1)!=0,&env,&s->frontier);if(e)return e;if(v->info.position_mode)return unsupported(s,0x2cf058);if(env.se_level_q8!=s->mix.se_level_q8)return unsupported(s,0x375630);if(env.output_mode!=1)return unsupported(s,0x37d680);if(env.system_flags_5e4&0x08000000u)return unsupported(s,0x2ce588);if(env.game_flags_5d8&0x40u)return unsupported(s,0x2cda80);if(v->flags18&~5u)return unsupported(s,v->info.pending?0x2ce588:0x2cea00);if(v->info.pending&&env.reverb)return unsupported(s,0x2ce588);if(!v->info.pending&&(v->flags18&4)&&env.gain64_bits!=0x3f800000u)return unsupported(s,0x2cea00);
 if(v->model){float speed;memcpy(&speed,v->model->root.bytes+0x5a8,4);if(!isfinite(speed)||((v->flags1c&1)&&(speed<.5f||(v->info.pending&&env.debug58c>=0&&speed<1))))return unsupported(s,v->info.pending?0x2ce588:0x2cea00);}return GH_VOICE_OK;}
gh_voice_status gh_se_voice_detach_actor_002ce488(gh_se_voice_engine*s,gh_actor_model*model){if(!s||!model)return GH_VOICE_ARGUMENT;s->frontier=0;for(unsigned i=0;i<64;i++){struct voice*v=s->voices+i;if(v->used!=1||v->model!=model)continue;if(v->info.pending){gh_voice_status e=source_position(v,0);if(e){s->frontier=0x2ce488;return e;}}if(gh_actor_model_sound_return(model))return GH_VOICE_ARGUMENT;v->model=NULL;v->part_index=-1;}return GH_VOICE_OK;}
gh_voice_status gh_se_voice_abort_attached_owner(gh_se_voice_engine*s,const void*owner){if(!s||!owner)return GH_VOICE_ARGUMENT;if(s->attached_bound_owner&&s->attached_bound_owner!=owner)return GH_VOICE_ARGUMENT;for(unsigned i=0;i<64;i++)if(s->voices[i].used&&s->voices[i].source_owner==owner){struct voice*v=s->voices+i;if(v->info.native_voice){gh_voice_status e=gh_audio_output_stop(s->output,v->info.native_voice);if(e){s->frontier=0x2ce488;return e;}}drop(s,(int)i,0);}s->attached_bound_owner=NULL;return GH_VOICE_OK;}
unsigned gh_se_voice_attached_reservations(const gh_se_voice_engine*s){unsigned n=0;if(s)for(unsigned i=0;i<64;i++)n+=s->voices[i].used==2;return n;}
static gh_voice_status voice_update_body(gh_se_voice_engine *s)
{
 int i;unsigned issued=s?s->stop_count:0;if(!s)return GH_VOICE_ARGUMENT;s->frontier=0;i=s->head;
 /* 375380 ages existing links before37BE98 consumes this frame's queue.
  * Newly constructed37C160 links start at0 and must stay0 this visit. */
 for(unsigned c=0;c<51;++c)if(s->channels[c]!=-1){struct voice *old=s->voices+s->channels[c];if(old->age!=UINT32_MAX)++old->age;}
 while(i!=-1){struct voice *v=s->voices+i;int next=v->next;
  if(!v->used){i=next;continue;}
  gh_voice_status attached=attached_update(s,v);if(attached)return attached;
  if(v->info.pending){
   /* Original shared SDE queue has48 entries. Overflowed EE voices observe
    * no SDE handle on the next update; don't invent extra queue capacity. */
   if(issued>=48){v->info.pending=0;i=next;continue;}++issued;
   unsigned first=v->group_first,end=first+v->group_count,channel;
   for(channel=first;channel<end && s->channels[channel]!=-1;++channel){}
   if(channel==end){
    unsigned minimum=255,picked=first;uint32_t age=0;
    for(unsigned c=first;c<end;++c){struct voice *old=s->voices+s->channels[c];if(old->priority<minimum)minimum=old->priority;}
    if(v->priority<minimum || (v->priority==minimum && !v->replace_equal)){v->info.pending=0;++v->ee_age;i=next;continue;}
    for(unsigned c=first;c<end;++c){struct voice *old=s->voices+s->channels[c];if(old->priority==minimum && age<=old->age){age=old->age;picked=c;}}
    channel=picked;drop(s,s->channels[channel],1);
   }
   if(v->source_owner&&(h(v->descriptor+0x18)||h(v->descriptor+0x1a))&&v->pitch_setup<2){
    if(!s->pitch.loaded)return unsupported(s,0x37d760);
    if(!v->pitch_setup){int16_t low,high,chosen;uint16_t a=(uint16_t)h(v->descriptor+0x18),b=(uint16_t)h(v->descriptor+0x1a);memcpy(&low,&a,2);memcpy(&high,&b,2);gh_voice_status e=gh_se_pitch_choose_0037d760(&s->pitch,low,high,&chosen);if(e){s->frontier=0x37d760;return e;}uint16_t bits=(uint16_t)((uint16_t)chosen+h(v->descriptor+0x1c));int16_t offset;memcpy(&offset,&bits,2);v->pitch_setup=1;e=gh_se_pitch_q12_000038e4(&s->pitch,offset,v->info.rate,&v->selected_pitch);if(e){s->frontier=0x38e4;return e;}}
    if(!v->selected_pitch)return unsupported(s,0x38e4);gh_voice_status e=prepare_pcm_with_pitch(s,v->info.bank,v->info.program,v->descriptor,v,v->selected_pitch);if(e)return e;v->pitch_setup=2;
   }
   gh_voice_status error=gh_audio_output_play(s->output,&v->pcm,48000,(float)v->info.level_q14/16384.0f,(float)v->info.level_q14/16384.0f,&v->info.native_voice);
   if(error){s->frontier=0x374df8;return error;}
   gh_audio_pcm_release(&v->pcm);v->info.pending=0;v->info.channel=(uint8_t)channel;s->channels[channel]=i;
  }else if(!v->info.native_voice)drop(s,i,0);
  else {int active;uint64_t played;gh_voice_status error=gh_audio_output_active(s->output,v->info.native_voice,&active,&played);if(error==GH_VOICE_BOUNDS)active=0;else if(error)return error;if(!active)drop(s,i,1);}
  if(v->used)++v->ee_age; /* actual EE+24 ADDIU/SW, modulo2^32 */
  i=next;
 }
 /* QueueStop is consumed before the per-node ramp scan, matching
  *375F68/37BE98 before37CAE0. SamplesPlayed is the explicit native busy
  * acknowledgement replacing asynchronous IOP feedback, not an EE oracle. */
 for(unsigned q=0;q<s->stop_count;q++)for(int j=s->head;j!=-1;j=s->voices[j].next){struct voice*v=s->voices+j;
  if(v->info.original_handle!=s->stops[q].handle||!v->info.native_voice)continue;
  int active;uint64_t played;gh_voice_status e=gh_audio_output_active(s->output,v->info.native_voice,&active,&played);if(e)return e;
  int ticks=s->stops[q].ticks;
  if(ticks<=0||!played){e=gh_audio_output_stop(s->output,v->info.native_voice);if(e&&e!=GH_VOICE_BOUNDS)return e;
   if(v->info.channel<51&&s->channels[v->info.channel]==j)s->channels[v->info.channel]=-1;
   v->info.channel=NONE;v->info.native_voice=0;v->info.retiring=1;v->info.stopping=0;
  }else if(v->info.stop_current){int step=-(int)v->info.stop_current/ticks;v->info.stop_step=(int16_t)(step?step:-1);v->info.stop_count=0;v->info.stopping=1;}
  break;
 }
 s->stop_count=0;
 for(int j=s->head;j!=-1;j=s->voices[j].next){struct voice*v=s->voices+j;if(!v->info.stopping||!v->info.native_voice)continue;
  if(!v->info.stop_current){gh_voice_status e=gh_audio_output_stop(s->output,v->info.native_voice);if(e&&e!=GH_VOICE_BOUNDS)return e;
   if(v->info.channel<51&&s->channels[v->info.channel]==j)s->channels[v->info.channel]=-1;
   v->info.channel=NONE;v->info.native_voice=0;v->info.retiring=1;v->info.stopping=0;
  }else{int next=v->info.stop_current+v->info.stop_step;if(next<0)next=0;v->info.stop_current=(int16_t)next;v->info.stop_count++;
   v->info.level_q14=(uint16_t)(((uint32_t)v->base_gain*(unsigned)next/0x7f00)/2);
   float gain=(float)v->info.level_q14/16384.0f;gh_voice_status e=gh_audio_output_voice_gain(s->output,v->info.native_voice,gain,gain);if(e)return e;
  }
 }
 return GH_VOICE_OK;
}
gh_voice_status gh_se_voice_update(gh_se_voice_engine*s){if(!s)return GH_VOICE_ARGUMENT;fenv_t env;if(feholdexcept(&env))return GH_VOICE_PLATFORM;if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return GH_VOICE_PLATFORM;}gh_voice_status e=voice_update_body(s);if(fesetenv(&env))return GH_VOICE_PLATFORM;return e;}
gh_voice_status gh_se_voice_query(const gh_se_voice_engine *s,uint32_t handle,gh_se_voice_info *out)
{if(!s || !handle || !out)return GH_VOICE_ARGUMENT;for(unsigned i=0;i<64;++i)if(s->voices[i].used && s->voices[i].info.original_handle==handle){*out=s->voices[i].info;out->attached_model=s->voices[i].model;out->attached=s->voices[i].model!=NULL;out->attached_part=s->voices[i].model?s->voices[i].part_index:-1;return GH_VOICE_OK;}return GH_VOICE_BOUNDS;}
uint32_t gh_se_voice_frontier(const gh_se_voice_engine *s){return s?s->frontier:0;}
gh_voice_status gh_se_voice_pcm_view(const gh_se_voice_engine *s,uint32_t handle,const int16_t **samples,size_t *frames)
{
 if(!s || !handle || !samples || !frames)return GH_VOICE_ARGUMENT;
 for(unsigned i=0;i<64;++i)if(s->voices[i].used && s->voices[i].info.original_handle==handle){
  if(!s->voices[i].info.pending)return GH_VOICE_BOUNDS;if(s->voices[i].source_owner&&!s->voices[i].info.pitch_q12)return GH_VOICE_UNSUPPORTED;
  *samples=s->voices[i].pcm.samples;*frames=s->voices[i].pcm.frames;return GH_VOICE_OK;
 }
 return GH_VOICE_BOUNDS;
}

unsigned gh_se_voice_active_nodes(const gh_se_voice_engine*s){unsigned n=0;if(s)for(unsigned i=0;i<64;i++)n+=s->voices[i].used==1;return n;}
gh_voice_status gh_se_voice_request_stop_00375050(gh_se_voice_engine*s,uint32_t handle,int16_t ticks,int*accepted){
 if(!s||!handle||!accepted)return GH_VOICE_ARGUMENT;*accepted=0;s->frontier=0;
 if(ticks < -1)return unsupported(s,0x378020);
 if(s->stop_count==48)return GH_VOICE_OK;
 s->stops[s->stop_count].handle=handle;s->stops[s->stop_count].ticks=ticks;s->stop_count++;*accepted=1;return GH_VOICE_OK;
}
gh_voice_status gh_se_voice_stop_bank_002cbc58(gh_se_voice_engine*s,int16_t bank,int16_t ticks){
 if(!s||bank<0||bank>=52)return GH_VOICE_ARGUMENT;if(ticks < -1)return unsupported(s,0x378020);s->frontier=0;
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