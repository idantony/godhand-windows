#include "bgm_requests.h"
#include <stdlib.h>
#include <string.h>
#include <fenv.h>
#include <math.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
typedef struct {uint8_t scalar[0xdc],row[32];gh_r207_allocation encoded,work;
 uint64_t offset,size;uint32_t work_bytes,rate;uint16_t member;
 uint8_t occupied,source_loaded,adx_channels,adx_version,adx_flags,linked,start_pending;
 int previous,next;gh_adx_stream *player;gh_adx_stream_status transport;} request;
struct gh_bgm_requests {gh_audio_registry *registry;gh_sound_control *control;gh_disc *disc;gh_heap *heap;
 request requests[16];uint32_t frontier;DWORD owner;gh_adx_key key;int16_t decibels[128];
 int native_enabled,muted,head;int32_t current128;uint32_t field138,field44,field48;
 uint8_t fallback_map[24],scene_event_valid;int32_t scene_event;const gh_game_children *scene_children;const gh_game_init_state *scene_init;uint32_t field134;};
static uint16_t u16(const uint8_t*p){return (uint16_t)((unsigned)p[0]|(unsigned)p[1]<<8);}
static uint32_t u32(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static uint32_t be32(const uint8_t*p){return (uint32_t)p[3]|(uint32_t)p[2]<<8|(uint32_t)p[1]<<16|(uint32_t)p[0]<<24;}
static void w32(uint8_t*p,uint32_t n){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(n>>(8*i));}
static void wf(uint8_t*p,unsigned n){float f=(float)n;uint32_t bits;memcpy(&bits,&f,4);w32(p,bits);}
static float rf(const uint8_t*p){uint32_t bits=u32(p);float f;memcpy(&f,&bits,4);return f;}
static void putf(uint8_t*p,float f){uint32_t bits;memcpy(&bits,&f,4);w32(p,bits);}
static int own(const gh_bgm_requests*s){return s&&s->owner==GetCurrentThreadId();}
static gh_bgm_status fail(gh_bgm_requests*s,uint32_t frontier,gh_bgm_status e){s->frontier=frontier;return e;}
gh_bgm_status gh_bgm_requests_create(gh_audio_registry*r,gh_sound_control*c,gh_disc*d,gh_heap*h,gh_bgm_requests**out){
 if(!out)return GH_BGM_ARGUMENT;*out=NULL;if(!r||!c||!c->initialized||!d||!h)return GH_BGM_ARGUMENT;
 gh_bgm_requests*s=calloc(1,sizeof(*s));if(!s)return GH_BGM_MEMORY;s->registry=r;s->control=c;s->disc=d;s->heap=h;s->owner=GetCurrentThreadId();s->head=-1;s->current128=-2;*out=s;return GH_BGM_OK;
}
unsigned gh_bgm_requests_pending(const gh_bgm_requests*s){unsigned n=0;if(own(s))for(unsigned i=0;i<16;i++)n+=s->requests[i].occupied&&!s->requests[i].linked;return n;}
unsigned gh_bgm_requests_active(const gh_bgm_requests*s){unsigned n=0;if(own(s))for(unsigned i=0;i<16;i++)n+=s->requests[i].linked!=0;return n;}
uint32_t gh_bgm_requests_frontier(const gh_bgm_requests*s){return s?s->frontier:0;}
gh_bgm_status gh_bgm_event_set_prefix_002d15a8(gh_bgm_requests*s,uint32_t event,int16_t arg3,int16_t arg4){
 request*q=NULL;gh_sound_span table,row;uint64_t off,size;size_t fi;gh_disc_entry de;gh_audio_status ae;
 if(!own(s))return GH_BGM_ARGUMENT;
 /* A stopped native C continuation must not be restarted at the function
  * entrance. Caller first supplies the missingprimitive, or explicitlyaborts. */
 if(gh_bgm_requests_pending(s))return GH_BGM_BUSY;s->frontier=0;
 for(unsigned i=0;i<16;i++)if(!u32(s->requests[i].scalar+0x94)){q=&s->requests[i];break;}
 if(!q)return GH_BGM_OK; /* Actualvoid no-op on exhaustedoriginalpool. */
 memset(q,0,sizeof(*q));w32(q->scalar+0x8c,UINT32_MAX);w32(q->scalar+0x90,UINT32_MAX); /*2CC578*/
 if(gh_audio_bgm0_table(s->registry,&table)!=GH_AUDIO_OK)return GH_BGM_NOT_READY; /*2CFF68*/
 w32(q->scalar+0x8c,0x80);w32(q->scalar+0x90,event);w32(q->scalar+0x94,5);w32(q->scalar+0x98,1);w32(q->scalar+0x9c,UINT32_MAX);
 q->scalar[0xa0]=(uint8_t)arg3;q->scalar[0xa1]=(uint8_t)((uint16_t)arg3>>8);q->scalar[0xa2]=(uint8_t)arg4;q->scalar[0xa3]=(uint8_t)((uint16_t)arg4>>8);q->occupied=1;
 /* Native table/row views replace originalpointerfieldsB8/6C/70/7C; no
  * raw guest pointer or truncated hostpointer is written into scalarstorage. */
 w32(q->scalar+0x48,1);w32(q->scalar+0x74,0x80);w32(q->scalar+0x4c,1); /*373118*/
 ae=gh_audio_external_sample(s->registry,-1,event,&row,&off,&size);if(ae)return fail(s,0x373158,GH_BGM_BOUNDS);
 memcpy(q->row,row.data,32);q->member=u16(q->row+2);q->offset=off;q->size=size;
 w32(q->scalar+0x58,q->row[10]);w32(q->scalar+0x54,q->row[11]&3);w32(q->scalar+0x78,event);
 memcpy(q->scalar+0x5c,q->row+0x11,8);memcpy(q->scalar+0x64,q->row+0x19,4);memcpy(q->scalar+0x68,q->row+0x1d,3);
 /* The verifiedtitle row is single ADX stream (AFDchannels1), whose ADX
  * payload is stereo. They are distinct fields; do not treat1 as monoaudio. */
 if(q->row[10]!=1||q->row[0x10])return fail(s,q->row[10]>1?0x374360:0x373d90,GH_BGM_UNSUPPORTED);
 q->rate=u16(q->row+8);unsigned kind=(q->row[11]>>2)&3;if(!kind)kind=2;
 wf(q->scalar+0x20,q->row[12]);wf(q->scalar+0x24,u16(q->row+4));wf(q->scalar+0x28,u16(q->row+6));
 if(!q->row[12])return fail(s,0x373d90,GH_BGM_UNSUPPORTED);
 /* Explicit orderednativebinary32 reference, not EEdivisionexactness. Keep
  * callerFEnv intact. Real row0 yields originalinterpolationfactor0.8. */
 {fenv_t saved;if(fegetenv(&saved)||fesetround(FE_TONEAREST))return fail(s,0x373d90,GH_BGM_UNSUPPORTED);
  volatile float a=(float)q->row[13],b=(float)q->row[12],ratio=a/b,scaled=ratio*100.0f;
  volatile float step=(float)((int)scaled+1),factor=step/100.0f;uint32_t bits;float value=factor;memcpy(&bits,&value,4);
  if(fesetenv(&saved))return fail(s,0x373d90,GH_BGM_UNSUPPORTED);w32(q->scalar+0x34,bits);
 }
 w32(q->scalar+0xc,q->rate);w32(q->scalar+0x10,kind);w32(q->scalar+0x2c,0x3f800000);w32(q->scalar+0x30,0x3f800000);
 /*373BA8 type>=2 nonmemory branch, originalinteger arithmetic. Other
  * paths remainexplicit instead of guessing SDK buffer/decoder contracts. */
 if(kind<2)return fail(s,0x373ba8,GH_BGM_UNSUPPORTED);
 uint64_t n=(uint64_t)kind*(q->rate/1000u)*25000u/44u*(q->row[15]+1u);
 n=((n+0x800u)/0x800u)*0x800u+(uint64_t)kind*0x40c0u+0x24u+0x40u;
 if(n>UINT32_MAX)return fail(s,0x373ba8,GH_BGM_BOUNDS);q->work_bytes=(uint32_t)n;
 if(gh_heap_allocate(s->heap,q->work_bytes,64,&q->work)!=GH_MEM_OK)return fail(s,0x31c8e0,GH_BGM_MEMORY);
 /* Actual sourceassociation32BB48 moved before the missingcodecconstructor
  * only as explicitnativepreparation. It does not publish constructor success. */
 if(gh_disc_find(s->disc,"bgm/bgm00.afs",&fi)||gh_disc_entry_at(s->disc,fi,&de)||off>de.size||size>de.size-off||size<20||size>SIZE_MAX)return fail(s,0x32bb48,GH_BGM_DISC);
 if(gh_heap_allocate(s->heap,(size_t)size,64,&q->encoded)!=GH_MEM_OK)return fail(s,0x31c8e0,GH_BGM_MEMORY);
 if(gh_disc_read(s->disc,fi,off,q->encoded.pointer,(size_t)size))return fail(s,0x32bb48,GH_BGM_DISC);
 const uint8_t*b=q->encoded.pointer;
 if(b[0]!=0x80||b[1]!=0||b[4]!=3||be32(b+8)!=q->rate)return fail(s,0x329438,GH_BGM_UNSUPPORTED);
 q->adx_channels=b[7];q->adx_version=b[18];q->adx_flags=b[19];q->source_loaded=1;
 return fail(s,0x329438,GH_BGM_UNSUPPORTED);
}
gh_bgm_status gh_bgm_requests_inspect(const gh_bgm_requests*s,unsigned slot,gh_bgm_request_view*v){
 if(!own(s)||!v||slot>=16)return GH_BGM_ARGUMENT;const request*q=&s->requests[slot];if(!q->occupied)return GH_BGM_NOT_READY;
 *v=(gh_bgm_request_view){.slot=slot,.event=u32(q->scalar+0x90),.logical_id=u32(q->scalar+0x8c),.mode=u32(q->scalar+0x94),.request_flags=u32(q->scalar+0x98),.stream_flags=u32(q->scalar+0x4c),.rate=q->rate,.original_buffer_bytes=q->work_bytes,.member=q->member,.afd_channels=q->row[10],.adx_channels=q->adx_channels,.adx_version=q->adx_version,.adx_flags=q->adx_flags,.member_offset=q->offset,.member_bytes=q->size,.original_scalars=q->scalar,.original_scalar_bytes=sizeof(q->scalar),.afd_row={q->row,32},.encoded={q->source_loaded?q->encoded.pointer:NULL,q->source_loaded?(size_t)q->size:0},.frontier=s->frontier,.linked=q->linked,.prepared=q->player!=NULL,.transport=q->transport};return GH_BGM_OK;
}
static int elf_span(gh_disc*d,uint32_t address,uint8_t*out,size_t n){
 uint8_t h[52],p[32];if(gh_disc_read_boot_elf(d,0,h,52)||memcmp(h,"\177ELF\1\1\1",7)||u16(h+42)!=32)return 0;
 uint32_t po=u32(h+28);unsigned count=u16(h+44);uint64_t fs=gh_disc_boot_elf_size(d);if(count>128||(uint64_t)po+count*32>fs)return 0;
 for(unsigned i=0;i<count;i++){if(gh_disc_read_boot_elf(d,(uint64_t)po+i*32,p,32))return 0;
  uint32_t base=u32(p+8),bytes=u32(p+16),off=u32(p+4);if(u32(p)!=1||address<base||(uint64_t)address+n>(uint64_t)base+bytes)continue;
  uint64_t at=(uint64_t)off+address-base;if(at>fs||n>fs-at)return 0;return gh_disc_read_boot_elf(d,at,out,n)==GH_DISC_OK;
 }return 0;
}
gh_bgm_status gh_bgm_requests_enable_native(gh_bgm_requests*s,const gh_adx_key*k,int muted,gh_bgm_gain_policy policy){
 uint8_t table[256];if(!own(s)||!k||(muted!=0&&muted!=1)||policy!=GH_BGM_GAIN_NATIVE_DB_REFERENCE)return GH_BGM_ARGUMENT;
 if(s->native_enabled||gh_bgm_requests_pending(s)||gh_bgm_requests_active(s))return GH_BGM_BUSY;
 if(!elf_span(s->disc,0x3f2060,table,sizeof(table)))return fail(s,0x373ff0,GH_BGM_DISC);
 for(unsigned i=0;i<128;i++){uint16_t n=u16(table+i*2);s->decibels[i]=(int16_t)(n<32768?(int)n:(int)n-65536);if(s->decibels[i]>0||s->decibels[i]<-999)return fail(s,0x373ff0,GH_BGM_UNSUPPORTED);}
 if(s->decibels[0]!=-960||s->decibels[127]!=0)return fail(s,0x373ff0,GH_BGM_UNSUPPORTED);
 s->key=*k;s->muted=muted;s->native_enabled=1;s->frontier=0;return GH_BGM_OK;
}
static gh_bgm_status gain(gh_bgm_requests*s,request*q,float volume){
 unsigned group=u32(q->scalar+0x54);if(group>=3||!isfinite(volume)||volume<0)return fail(s,0x373ff0,GH_BGM_UNSUPPORTED);
 float master,category;memcpy(&master,&s->control->gain_bits[group],4);memcpy(&category,&s->control->gain_bits[group+3],4);
 volatile float a=volume*master,b=a/127.0f,c=b*category,d=c/127.0f;
 float scaled=d;if(s->control->output_mode==2){volatile float mono=scaled*rf(q->scalar+0x34);scaled=mono;}
 else if(s->control->output_mode!=1)return fail(s,0x3756f0,GH_BGM_UNSUPPORTED);
 volatile float result=scaled*rf(q->scalar+0x2c);float limited=result;if(limited>=127)limited=127;
 if(!isfinite(limited)||limited<0)return fail(s,0x373ff0,GH_BGM_UNSUPPORTED);
 int index=(int)limited;volatile float exponent=(float)s->decibels[index]/200.0f;float linear=powf(10.0f,exponent);
 if(gh_adx_stream_set_gain(q->player,linear)!=GH_ADX_OK)return fail(s,0x32a9f0,GH_BGM_NATIVE);
 putf(q->scalar+0x14,volume);return GH_BGM_OK;
}
static void fade(request*q,float target,float frames){
 if(frames==0)putf(q->scalar+0x1c,target<=rf(q->scalar+0x18)?rf(q->scalar+0x28):rf(q->scalar+0x24));
 else putf(q->scalar+0x1c,frames);putf(q->scalar+0x18,target);
}
static gh_bgm_status clear_stream(gh_bgm_requests*s,request*q){
 q->start_pending=0;if(q->player){gh_adx_stream_close(q->player);q->player=NULL;}memset(&q->transport,0,sizeof(q->transport));
 if(q->work.pointer){if(gh_heap_release(s->heap,q->work)!=GH_MEM_OK)return fail(s,0x373d48,GH_BGM_MEMORY);q->work=(gh_r207_allocation){0};}
 if(q->encoded.pointer){if(gh_heap_release(s->heap,q->encoded)!=GH_MEM_OK)return fail(s,0x373d48,GH_BGM_MEMORY);q->encoded=(gh_r207_allocation){0};q->source_loaded=0;}
 memset(q->scalar,0,0x84);return GH_BGM_OK;
}
static void unlink_request(gh_bgm_requests*s,request*q){
 if(q->previous<0)s->head=q->next;else s->requests[q->previous].next=q->next;
 if(q->next>=0)s->requests[q->next].previous=q->previous;q->linked=0;q->occupied=0;w32(q->scalar+0x94,0);
}
static gh_bgm_status stream_update(gh_bgm_requests*s,request*q){
 if(!q->player)return GH_BGM_OK;uint32_t flags=u32(q->scalar+0x4c);float delay=rf(q->scalar+0x50);
 if(delay!=0)return fail(s,0x372cb0,GH_BGM_UNSUPPORTED); /*R020row0 delay0*/
 if(flags&4){
  gh_adx_error de=q->transport.started?gh_adx_stream_pause(q->player,0):gh_adx_stream_start(q->player);
  if(de)return fail(s,0x3734f0,GH_BGM_NATIVE);flags&=~2u;
 }
 float multiplier=rf(q->scalar+0x2c),target=rf(q->scalar+0x30);volatile float difference=multiplier-target;
 if(difference<-.1f){volatile float next=multiplier+.1f;putf(q->scalar+0x2c,next);}
 else if(difference>.1f){volatile float next=multiplier-.1f;putf(q->scalar+0x2c,next);}else putf(q->scalar+0x2c,target);
 float time=rf(q->scalar+0x1c),volume;
 if(time>0){volatile float next=time-1.0f,desired=rf(q->scalar+0x20)*rf(q->scalar+0x18),delta=rf(q->scalar+0x14)-desired,part=delta/time,result=rf(q->scalar+0x14)-part;putf(q->scalar+0x1c,next);volume=result;}
 else {volatile float result=rf(q->scalar+0x20)*rf(q->scalar+0x18);volume=result;}
 gh_bgm_status e=gain(s,q,volume);if(e)return e;
 if(!(flags&8)&&volume<=0){if(gh_adx_stream_stop(q->player)!=GH_ADX_OK)return fail(s,0x373430,GH_BGM_NATIVE);return clear_stream(s,q);}
 if(flags&8){
  if(volume<=0&&!(flags&2)){if(q->transport.started&&gh_adx_stream_pause(q->player,1)!=GH_ADX_OK)return fail(s,0x373488,GH_BGM_NATIVE);flags|=2;}
  else if(volume>0&&(flags&2)){gh_adx_error de=q->transport.started?gh_adx_stream_pause(q->player,0):gh_adx_stream_start(q->player);if(de)return fail(s,0x3734f0,GH_BGM_NATIVE);flags&=~2u;}
 }
 /* Native asynchronous-start timing adaptation: queue first PCM only after
  * the original immediate volume update, rather than at default gain. */
 if(q->start_pending){if(gh_adx_stream_start(q->player)!=GH_ADX_OK)return fail(s,0x373fc8,GH_BGM_NATIVE);q->start_pending=0;}
 w32(q->scalar+0x4c,flags);
 if(gh_adx_stream_poll(q->player,&q->transport)!=GH_ADX_OK)return fail(s,0x374170,GH_BGM_NATIVE);
 if(q->transport.complete)return clear_stream(s,q);return GH_BGM_OK;
}
static gh_bgm_status update_one(gh_bgm_requests*s,request*q){
 uint32_t mode=u32(q->scalar+0x94);if(!mode)return GH_BGM_OK;if(mode!=1&&mode!=5)return fail(s,0x2cc628,GH_BGM_UNSUPPORTED);
 if(!q->player){if(q->linked)unlink_request(s,q);return GH_BGM_OK;}
 uint32_t state=u32(q->scalar+0xb0);if(state>2)return fail(s,0x2cd1d0,GH_BGM_UNSUPPORTED);
 if(state==0){if(u16(q->scalar+0xa0)&1){w32(q->scalar+0x4c,u32(q->scalar+0x4c)|8);w32(q->scalar+0xb4,(uint32_t)(int32_t)(int16_t)u16(q->scalar+0xa2));}w32(q->scalar+0xb0,1);state=1;}
 if(state==1&&!(s->control->event_mask50&u32(q->scalar+0x9c))){w32(q->scalar+0x4c,u32(q->scalar+0x4c)&~8u);fade(q,0,0);w32(q->scalar+0xb0,2);}
 if(state!=2&&(u32(q->scalar+0x98)&0x180000)&&!(u32(q->scalar+0x4c)&2)){
  volatile float time=rf(q->scalar+0xc8)-1.0f;putf(q->scalar+0xc8,time);
  if(time<0){putf(q->scalar+0xc8,0);if(gh_adx_stream_pause(q->player,1)!=GH_ADX_OK)return fail(s,0x373488,GH_BGM_NATIVE);w32(q->scalar+0x4c,u32(q->scalar+0x4c)|2u);}
 }
 w32(q->scalar+0x98,u32(q->scalar+0x98)&~4u);return stream_update(s,q);
}
static gh_bgm_status create_native_request_mode(gh_bgm_requests*s,uint32_t event,int16_t a3,int16_t a4,int paused,uint32_t mode){
 if(!own(s))return GH_BGM_ARGUMENT;
 if(!s->native_enabled){gh_bgm_status prefix=gh_bgm_event_set_prefix_002d15a8(s,event,a3,a4);if(mode==1)for(unsigned i=0;i<16;i++)if(s->requests[i].occupied&&!s->requests[i].linked){w32(s->requests[i].scalar+0x94,1);w32(s->requests[i].scalar+0x98,0);break;}return prefix==GH_BGM_NOT_READY?GH_BGM_OK:prefix;}
 if((mode==5&&event!=0)||event>55||a3||a4)return fail(s,0x2cc728,GH_BGM_UNSUPPORTED);
 gh_bgm_status e=gh_bgm_event_set_prefix_002d15a8(s,event,a3,a4);if(e==GH_BGM_NOT_READY||e==GH_BGM_OK)return GH_BGM_OK; /*actualvoid ignoresnotready/full*/
 if(e!=GH_BGM_UNSUPPORTED||s->frontier!=0x329438)return e;
 request*q=NULL;for(unsigned i=0;i<16;i++)if(s->requests[i].occupied&&!s->requests[i].linked){q=&s->requests[i];break;}
 if(!q||!q->source_loaded)return fail(s,0x329438,GH_BGM_ARGUMENT);w32(q->scalar+0x94,mode);
 if(gh_adx_stream_open(q->encoded.pointer,(size_t)q->size,&s->key,&q->player)!=GH_ADX_OK)return fail(s,0x329438,GH_BGM_NATIVE);
 if(gh_adx_stream_set_muted(q->player,s->muted)!=GH_ADX_OK||gh_adx_stream_set_loop(q->player,1)!=GH_ADX_OK)return fail(s,0x32b188,GH_BGM_NATIVE);
 if(gh_adx_stream_poll(q->player,&q->transport)!=GH_ADX_OK)return fail(s,0x374170,GH_BGM_NATIVE);
 if(paused)w32(q->scalar+0x4c,u32(q->scalar+0x4c)|2u); /*373158*/
 else {w32(q->scalar+0x98,0);q->start_pending=(uint8_t)(mode==5);} /*373298 ->373F38 ->373FC8*/
 fenv_t saved;if(fegetenv(&saved)||fesetround(FE_TONEAREST))return fail(s,0x373ff0,GH_BGM_UNSUPPORTED);
 e=gain(s,q,0);if(!e){fade(q,1,0);w32(q->scalar+0xb4,0);e=update_one(s,q);}if(fesetenv(&saved))return fail(s,0x373ff0,GH_BGM_UNSUPPORTED);if(e)return e;
 /*2CC5C0 links onlyAFTERactualimmediateupdate. No request is played simply
  * because thisnativeconstructor and registryassociation succeeded. */
 int slot=(int)(q-s->requests);q->previous=-1;q->next=s->head;if(s->head>=0)s->requests[s->head].previous=slot;s->head=slot;q->linked=1;if(mode==1)q->start_pending=1;s->frontier=0;return GH_BGM_OK;
}
gh_bgm_status gh_bgm_event_set_002d15a8(gh_bgm_requests*s,uint32_t event,int16_t a3,int16_t a4){
 return create_native_request_mode(s,event,a3,a4,1,5);
}

static gh_bgm_status automatic_update(gh_bgm_requests *,const gh_boot_work *);
gh_bgm_status gh_bgm_requests_update_002d0938(gh_bgm_requests*s,const gh_boot_work*w){
 if(!own(s)||!w)return GH_BGM_ARGUMENT;if(gh_bgm_requests_pending(s))return fail(s,s->frontier?s->frontier:0x329438,GH_BGM_BUSY);
 fenv_t saved;if(fegetenv(&saved)||fesetround(FE_TONEAREST))return fail(s,0x373ff0,GH_BGM_UNSUPPORTED);
 gh_bgm_status e=GH_BGM_OK;for(int i=s->head;i>=0;){request*q=&s->requests[i];int next=q->next;e=update_one(s,q);if(e)break;i=next;}
 if(fesetenv(&saved))return fail(s,0x373ff0,GH_BGM_UNSUPPORTED);if(e)return e;
 if(s->control->flags_ac&0x10000000){int mode5=0;for(int i=s->head;i>=0;i=s->requests[i].next)if(u32(s->requests[i].scalar+0x94)==5){mode5=1;break;}if(!mode5){e=gh_bgm_event_stop_002d1800(s,0);if(e)return e;}}
 uint32_t wf=u32((const uint8_t*)w+0x5e4);
 if(wf&0x20080000){for(int i=s->head;i>=0;i=s->requests[i].next){request*q=&s->requests[i];if(u32(q->scalar+0xb0)!=2&&q->scalar[0x60])return fail(s,0x2cd4a8,GH_BGM_UNSUPPORTED);}s->current128=-2;s->field138=3;}
 if((s->control->flags_ac&0x200000)&&!(s->control->flags_ac&0x100000)&&!(wf&0x20080000))return automatic_update(s,w);
 s->frontier=0;return GH_BGM_OK;
}
gh_bgm_status gh_bgm_event_start_002d1610(gh_bgm_requests*s,uint32_t event,int16_t a3,int16_t a4){
 (void)a3;(void)a4; /* Actualexistingrequestbranch doesnotreadthem. */
 if(!own(s))return GH_BGM_ARGUMENT;if(gh_bgm_requests_pending(s))return GH_BGM_BUSY;
 request*q=NULL;for(int i=s->head;i>=0;i=s->requests[i].next){request*r=&s->requests[i];if(u32(r->scalar+0x8c)==0x80&&u32(r->scalar+0x90)==event){q=r;break;}}
 if(!q||!(u32(q->scalar+0x98)&1)){
  if(!s->native_enabled)return fail(s,0x2cc900,GH_BGM_UNSUPPORTED);
  int free_slot=0;for(unsigned i=0;i<16;i++)if(!u32(s->requests[i].scalar+0x94)){free_slot=1;break;}
  if(!free_slot)return gh_bgm_event_stop_002d1800(s,0); /*2D2228 failure*/
  gh_bgm_status e=create_native_request_mode(s,event,a3,a4,0,5);if(e)return e;
  q=s->head>=0?&s->requests[s->head]:NULL;
  if(!q)return GH_BGM_NOT_READY;
 }else {
 w32(q->scalar+0x98,u32(q->scalar+0x98)&~1u); /*2CCAC8*/
 if(q->player){
  fenv_t saved;if(fegetenv(&saved)||fesetround(FE_TONEAREST))return fail(s,0x3734f0,GH_BGM_UNSUPPORTED);
  gh_adx_error ae=q->transport.started?gh_adx_stream_pause(q->player,0):gh_adx_stream_start(q->player);
  if(!ae)ae=gh_adx_stream_poll(q->player,&q->transport);int restore=fesetenv(&saved);
  if(ae||restore)return fail(s,0x3734f0,GH_BGM_NATIVE);w32(q->scalar+0x4c,u32(q->scalar+0x4c)&~2u);
 }
 }
 s->control->flags_b0|=0x10000000;s->control->flags_ac|=0x10000000;
 if(!q->scalar[0x5e])for(int i=s->head;i>=0;i=s->requests[i].next){request*r=&s->requests[i];uint32_t flags=u32(r->scalar+0x98);
  if(!(flags&0x180000)&&r!=q&&!(flags&1)){w32(r->scalar+0x98,flags|0x1000);if(u32(r->scalar+0xb0)!=2)fade(r,0,10);}}
 s->frontier=0;return GH_BGM_OK;
}
gh_bgm_status gh_bgm_event_fade_002d1768(gh_bgm_requests*s,uint32_t event,float frames,float target,float initial_factor){
 if(!own(s)||!isfinite(frames)||!isfinite(target)||!isfinite(initial_factor)||frames<0)return GH_BGM_ARGUMENT;
 request*q=NULL;for(int i=s->head;i>=0;i=s->requests[i].next)if(u32(s->requests[i].scalar+0x90)==event){q=&s->requests[i];break;}
 if(!q){s->frontier=0;return GH_BGM_OK;}
 if(initial_factor>=0)return fail(s,0x373938,GH_BGM_UNSUPPORTED);
 fenv_t saved;if(fegetenv(&saved)||fesetround(FE_TONEAREST))return fail(s,0x3735e0,GH_BGM_UNSUPPORTED);
 if(target<.001f)target=0;if(q->player)fade(q,target,frames);int restore=fesetenv(&saved);if(restore)return fail(s,0x3735e0,GH_BGM_UNSUPPORTED);s->frontier=0;return GH_BGM_OK;
}

gh_bgm_status gh_bgm_event_stop_002d1800(gh_bgm_requests*s,int preserve_mode5){
 if(!own(s)||(preserve_mode5!=0&&preserve_mode5!=1))return GH_BGM_ARGUMENT;
 s->control->flags_b0&=~0x10000000u;s->control->flags_ac&=~0x10000000u;
 fenv_t saved;if(fegetenv(&saved)||fesetround(FE_TONEAREST))return fail(s,0x2cd4a8,GH_BGM_UNSUPPORTED);
 gh_bgm_status e=GH_BGM_OK;for(int i=s->head;i>=0;i=s->requests[i].next){request*q=&s->requests[i];
  if(u32(q->scalar+0x94)==5){if(!preserve_mode5)fade(q,0,0);}
  else {e=fail(s,0x2d1854,GH_BGM_UNSUPPORTED);break;}}
 if(fesetenv(&saved))return fail(s,0x2cd4a8,GH_BGM_UNSUPPORTED);if(!e)s->frontier=0;return e;
}
int gh_bgm_requests_scene_event(const gh_bgm_requests*s,int32_t*out){if(!own(s)||!out||!s->scene_event_valid)return 0;*out=s->scene_event;return 1;}
static gh_bgm_status select_scene_event(gh_bgm_requests*s,const gh_boot_work*w,const gh_game_children*children,const gh_game_init_state*init,int32_t*out){
 if(!own(s)||!w||!children||!init)return GH_BGM_ARGUMENT;
 const uint8_t*c;size_t bytes;if(gh_game_children_manager_view(children,&c,&bytes)||bytes<0xeec)return GH_BGM_ARGUMENT;
 uint32_t mode=u32(c+0x48);if((mode==3||mode==12)&&u32(c+0xee8))return fail(s,0x2d0d70,GH_BGM_UNSUPPORTED);
 int32_t selected=10;
 if(!init->heat[0x10]){
  uint8_t map[44*24];const uint8_t*row=NULL;uint16_t room=(uint16_t)w->room;
  if(!elf_span(s->disc,0x3c3138,map,sizeof(map)))return fail(s,0x2d0b88,GH_BGM_DISC);
  for(unsigned i=0;i<44;i++)if(u16(map+i*24)==room)row=map+i*24; /*LASTmatchoriginal*/
  if(!row){
   if(!elf_span(s->disc,0x3c3120,s->fallback_map,24))return fail(s,0x2d0b88,GH_BGM_DISC);
   s->fallback_map[0]=(uint8_t)room;s->fallback_map[1]=(uint8_t)(room>>8);
   uint32_t high=room>>8,number=2;if(((high-1)&255)<8)number=high+1;
   for(unsigned i=4;i<24;i+=4)w32(s->fallback_map+i,number);row=s->fallback_map;
  }
  unsigned column=4;int32_t state=s->control->bgm_state_13c;
  if(state==1)column=8;
  else {
   /*2D0CD0 calls2D0C10 forfiveenemykinds; canonicalemptylist genuinely
    * returns-2. Do not silentlyignore an actualenemyoverride. */
   if(init->enemy_top)return fail(s,0x2d0c10,GH_BGM_UNSUPPORTED);
   if(state==2)column=12;else if(state==3)column=16;else if(state==4)column=20;
  }
  uint32_t raw=u32(row+column);memcpy(&selected,&raw,4);
 }
 *out=selected;return GH_BGM_OK;
}
gh_bgm_status gh_bgm_requests_scene_begin_002d0850(gh_bgm_requests*s,const gh_boot_work*w,const gh_game_children*children,const gh_game_init_state*init){
 int32_t selected;gh_bgm_status status=select_scene_event(s,w,children,init,&selected);if(status)return status;s->scene_children=children;s->scene_init=init;
 for(int i=s->head;i>=0;i=s->requests[i].next){request*q=&s->requests[i];uint32_t raw=u32(q->scalar+0x90);int32_t event;memcpy(&event,&raw,4);
  if(u32(q->scalar+0xb0)!=2&&event!=selected&&q->player)fade(q,0,15);
 }
 if(w->room!=0x20&&w->room!=0x23){s->current128=-2;s->control->flags_b0|=0x200000;s->control->flags_ac|=0x200000;}
 s->field48=s->field44=0;s->field138=3;s->scene_event=selected;s->scene_event_valid=1;s->frontier=0;return GH_BGM_OK;
}
/*373A78's single-AFD-channel path tests target18, not current volume14. */
static int fully_faded(const request*q){return !q->player||!(u32(q->scalar+0x4c)&1)||!(rf(q->scalar+0x18)>0);}
static gh_bgm_status fade_out_request(gh_bgm_requests*s,request*q,float frames){if(u32(q->scalar+0x98)&4)frames=rf(q->scalar+0xc4);if(!isfinite(frames)||frames<0)return fail(s,0x3735e0,GH_BGM_UNSUPPORTED);fade(q,0,frames);return GH_BGM_OK;}
static gh_bgm_status resume_request(gh_bgm_requests*s,request*q,float frames,uint32_t set,uint32_t clear){putf(q->scalar+0xc8,frames);w32(q->scalar+0x98,(u32(q->scalar+0x98)|set)&~clear);if(q->player&&(u32(q->scalar+0x4c)&2)){gh_adx_error e=q->transport.started?gh_adx_stream_pause(q->player,0):gh_adx_stream_start(q->player);if(e||gh_adx_stream_poll(q->player,&q->transport))return fail(s,0x3734f0,GH_BGM_NATIVE);w32(q->scalar+0x4c,u32(q->scalar+0x4c)&~2u);}fade(q,1,frames);return GH_BGM_OK;}
static gh_bgm_status automatic_body(gh_bgm_requests*s,const gh_boot_work*w){
 if(!s->scene_children||!s->scene_init)return fail(s,0x2d0d70,GH_BGM_UNSUPPORTED);
 int32_t selected;gh_bgm_status e=select_scene_event(s,w,s->scene_children,s->scene_init,&selected);if(e)return e;s->scene_event=selected;s->scene_event_valid=1;
 if(selected!=-2&&selected!=s->current128){
  for(int i=s->head;i>=0;i=s->requests[i].next){request*q=&s->requests[i];uint32_t event=u32(q->scalar+0x90),flags=u32(q->scalar+0x98);if(u32(q->scalar+0xb0)==2||event==(uint32_t)selected)continue;float frames=30;
   if(((uint16_t)w->room>>8)==8&&selected!=10&&selected!=1&&event!=0x2e&&selected!=0x1b){e=fade_out_request(s,q,10);if(e)return e;continue;}
   if((flags&0x180000)||fully_faded(q))continue;
   if(event==1||event==0x2e){e=fade_out_request(s,q,30);if(e)return e;continue;}
   if(q->scalar[0x5f]){frames=70;s->field134=50;}else if(q->scalar[0x61]){s->field134=30;e=fade_out_request(s,q,40);if(e)return e;continue;}else s->field134=0;
   if(selected!=10&&selected!=1&&event!=0x2e&&selected!=0x1b){e=fade_out_request(s,q,frames);if(e)return e;}
   else{putf(q->scalar+0xc8,frames);w32(q->scalar+0x98,flags|0x80000);fade(q,.01f,frames);}
  }
  s->field134--;int32_t delay;memcpy(&delay,&s->field134,4);if(delay<1){s->field134=0;s->current128=selected;int reused=0;
   for(int i=s->head;i>=0;i=s->requests[i].next){request*q=&s->requests[i];uint32_t flags=u32(q->scalar+0x98);if(u32(q->scalar+0xb0)==2||u32(q->scalar+0x90)!=(uint32_t)selected||q->scalar[0x61]||!(flags&0x180000))continue;reused=1;if(flags&0x100000){e=resume_request(s,q,15,0x400000,0x100000);if(e)return e;}if(u32(q->scalar+0x98)&0x80000){e=resume_request(s,q,40,0x200000,0x80000);if(e)return e;}}
   if(!reused){int free_slot=0;for(unsigned i=0;i<16;i++)if(!u32(s->requests[i].scalar+0x94)){free_slot=1;break;}if(free_slot){e=create_native_request_mode(s,(uint32_t)selected,0,0,0,1);if(e)return e;request*q=s->head>=0?&s->requests[s->head]:NULL;if(!q)return GH_BGM_NOT_READY;e=gain(s,q,0);if(e)return e;if(!(u32(q->scalar+0x98)&0x100))fade(q,1,40);}}
  }
 }
 if(w->room){
  if(!s->scene_init->heat[0x10])for(int i=s->head;i>=0;i=s->requests[i].next){request*q=&s->requests[i];if(u32(q->scalar+0xb0)!=2&&u32(q->scalar+0x90)==10&&!fully_faded(q)){e=fade_out_request(s,q,60);if(e)return e;}}
  if(s->scene_init->enemy_top)return fail(s,0x2d0c10,GH_BGM_UNSUPPORTED); /*actual2D0CD0 nonempty predicate*/
  for(int i=s->head;i>=0;i=s->requests[i].next){request*q=&s->requests[i];uint32_t event=u32(q->scalar+0x90);if(u32(q->scalar+0xb0)!=2&&!fully_faded(q)&&(event==1||event==0x2e||event==0x1b)){e=fade_out_request(s,q,20);if(e)return e;}}
  if(s->control->bgm_state_13c!=1&&(w->room==0x604||w->room==0x606))for(int i=s->head;i>=0;i=s->requests[i].next){request*q=&s->requests[i];if(u32(q->scalar+0xb0)!=2&&u32(q->scalar+0x90)==0x26&&!fully_faded(q)){e=fade_out_request(s,q,20);if(e)return e;}}
 }
 for(int i=s->head;i>=0;i=s->requests[i].next){request*q=&s->requests[i];uint32_t flags=u32(q->scalar+0x98);if(u32(q->scalar+0xb0)==2||u32(q->scalar+0x90)!=(uint32_t)s->current128||(flags&0x80000))continue;if(s->control->flags_ac&0x400000){if(!(flags&0x20000)){fade(q,.2f,10);w32(q->scalar+0x98,(flags&~0x40000u)|0x20000);}}else if(!(flags&0x40000)){fade(q,1,10);w32(q->scalar+0x98,(flags&~0x20000u)|0x40000);}}
 s->frontier=0;return GH_BGM_OK;
}
static gh_bgm_status automatic_update(gh_bgm_requests*s,const gh_boot_work*w){
 s->field138--;int32_t counter;memcpy(&counter,&s->field138,4);if(counter>0){s->frontier=0;return GH_BGM_OK;}s->field138=UINT32_MAX;
 fenv_t env;if(feholdexcept(&env))return fail(s,0x2d0ea0,GH_BGM_UNSUPPORTED);if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return fail(s,0x2d0ea0,GH_BGM_UNSUPPORTED);}gh_bgm_status e=automatic_body(s,w);if(fesetenv(&env))return fail(s,0x2d0ea0,GH_BGM_UNSUPPORTED);return e;
}
int gh_bgm_requests_automatic_state(const gh_bgm_requests*s,gh_bgm_automatic_state*out){if(!own(s)||!out)return 0;*out=(gh_bgm_automatic_state){s->current128,s->field134,s->field138,s->scene_children,s->scene_init};return 1;}
gh_bgm_status gh_bgm_requests_abort_pending(gh_bgm_requests*s){
 if(!own(s))return GH_BGM_ARGUMENT;if(gh_bgm_requests_active(s))return GH_BGM_BUSY;for(unsigned i=0;i<16;i++){request*q=&s->requests[i];
  if(q->player){gh_adx_stream_close(q->player);q->player=NULL;}
  if(q->encoded.pointer){if(gh_heap_release(s->heap,q->encoded)!=GH_MEM_OK)return GH_BGM_MEMORY;q->encoded=(gh_r207_allocation){0};}
  if(q->work.pointer){if(gh_heap_release(s->heap,q->work)!=GH_MEM_OK)return GH_BGM_MEMORY;q->work=(gh_r207_allocation){0};}memset(q,0,sizeof(*q));
 }s->frontier=0;return GH_BGM_OK;
}
void gh_bgm_requests_destroy(gh_bgm_requests*s){if(!own(s))return;for(unsigned i=0;i<16;i++)s->requests[i].linked=0;if(gh_bgm_requests_abort_pending(s)==GH_BGM_OK)free(s);}

gh_bgm_status gh_bgm_requests_scene_exit_begin(gh_bgm_requests*s){
 if(!own(s))return GH_BGM_ARGUMENT;if(gh_bgm_requests_pending(s))return fail(s,0x2caa00,GH_BGM_BUSY);
 for(int i=s->head;i>=0;i=s->requests[i].next){request*q=&s->requests[i];uint32_t flags=u32(q->scalar+0x98);
  if(!q->scalar[0x5f]&&!q->scalar[0x61]&&!(flags&0x10000)){
   putf(q->scalar+0xc8,20);w32(q->scalar+0x98,flags|0x100000);if(q->player)fade(q,.01f,20);
  }else{float frames=(flags&4)?rf(q->scalar+0xc4):20;if(!isfinite(frames)||frames<0)return fail(s,0x2cd4a8,GH_BGM_UNSUPPORTED);if(q->player)fade(q,0,frames);w32(q->scalar+0x98,flags|0x100);}
 }s->current128=-2;s->frontier=0;return GH_BGM_OK;
}
gh_bgm_status gh_bgm_requests_scene_exit_ready(const gh_bgm_requests*s,int*out){
 if(!own(s)||!out)return GH_BGM_ARGUMENT;int ready=1;
 for(int i=s->head;i>=0;i=s->requests[i].next){const request*q=&s->requests[i];uint32_t flags=u32(q->scalar+0x98);
  if(q->player&&!(u32(q->scalar+0x4c)&2)&&(flags&0x100)&&!(flags&0x180000))ready=0;
 }*out=ready;return GH_BGM_OK;
}
gh_bgm_status gh_bgm_requests_scene_exit_finalize(gh_bgm_requests*s){
 if(!own(s))return GH_BGM_ARGUMENT;
 for(int i=s->head;i>=0;){request*q=&s->requests[i];int next=q->next;uint32_t flags=u32(q->scalar+0x98);
  if(flags&0x100){gh_bgm_status e=clear_stream(s,q);if(e)return e;unlink_request(s,q);}
  else if(flags&0x1ff00){w32(q->scalar+0x98,flags&0xfffe00ff);if(q->player)fade(q,1,30);}
  i=next;
 }s->frontier=0;return GH_BGM_OK;
}

int gh_bgm_requests_uses_owners(const gh_bgm_requests*s,const gh_audio_registry*r,const gh_sound_control*c){return own(s)&&r&&c&&s->registry==r&&s->control==c;}
