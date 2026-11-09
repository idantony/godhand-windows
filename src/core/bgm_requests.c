#include "bgm_requests.h"
#include <stdlib.h>
#include <string.h>
#include <fenv.h>
typedef struct {uint8_t scalar[0xdc],row[32];gh_r207_allocation encoded,work;
 uint64_t offset,size;uint32_t work_bytes,rate;uint16_t member;
 uint8_t occupied,source_loaded,adx_channels,adx_version,adx_flags;} request;
struct gh_bgm_requests {gh_audio_registry *registry;gh_sound_control *control;gh_disc *disc;gh_heap *heap;
 request requests[16];uint32_t frontier;};
static uint16_t u16(const uint8_t*p){return (uint16_t)((unsigned)p[0]|(unsigned)p[1]<<8);}
static uint32_t u32(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static uint32_t be32(const uint8_t*p){return (uint32_t)p[3]|(uint32_t)p[2]<<8|(uint32_t)p[1]<<16|(uint32_t)p[0]<<24;}
static void w32(uint8_t*p,uint32_t n){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(n>>(8*i));}
static void wf(uint8_t*p,unsigned n){float f=(float)n;uint32_t bits;memcpy(&bits,&f,4);w32(p,bits);}
static gh_bgm_status fail(gh_bgm_requests*s,uint32_t frontier,gh_bgm_status e){s->frontier=frontier;return e;}
gh_bgm_status gh_bgm_requests_create(gh_audio_registry*r,gh_sound_control*c,gh_disc*d,gh_heap*h,gh_bgm_requests**out){
 if(!out)return GH_BGM_ARGUMENT;*out=NULL;if(!r||!c||!c->initialized||!d||!h)return GH_BGM_ARGUMENT;
 gh_bgm_requests*s=calloc(1,sizeof(*s));if(!s)return GH_BGM_MEMORY;s->registry=r;s->control=c;s->disc=d;s->heap=h;*out=s;return GH_BGM_OK;
}
unsigned gh_bgm_requests_pending(const gh_bgm_requests*s){unsigned n=0;if(s)for(unsigned i=0;i<16;i++)n+=s->requests[i].occupied!=0;return n;}
unsigned gh_bgm_requests_active(const gh_bgm_requests*s){(void)s;/* No request has crossed unrecovered2CC628 yet. */return 0;}
uint32_t gh_bgm_requests_frontier(const gh_bgm_requests*s){return s?s->frontier:0;}
gh_bgm_status gh_bgm_event_set_prefix_002d15a8(gh_bgm_requests*s,uint32_t event,int16_t arg3,int16_t arg4){
 request*q=NULL;gh_sound_span table,row;uint64_t off,size;size_t fi;gh_disc_entry de;gh_audio_status ae;
 if(!s)return GH_BGM_ARGUMENT;
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
 if(!s||!v||slot>=16)return GH_BGM_ARGUMENT;const request*q=&s->requests[slot];if(!q->occupied)return GH_BGM_NOT_READY;
 *v=(gh_bgm_request_view){.slot=slot,.event=u32(q->scalar+0x90),.logical_id=u32(q->scalar+0x8c),.mode=u32(q->scalar+0x94),.request_flags=u32(q->scalar+0x98),.stream_flags=u32(q->scalar+0x4c),.rate=q->rate,.original_buffer_bytes=q->work_bytes,.member=q->member,.afd_channels=q->row[10],.adx_channels=q->adx_channels,.adx_version=q->adx_version,.adx_flags=q->adx_flags,.member_offset=q->offset,.member_bytes=q->size,.original_scalars=q->scalar,.original_scalar_bytes=sizeof(q->scalar),.afd_row={q->row,32},.encoded={q->source_loaded?q->encoded.pointer:NULL,q->source_loaded?(size_t)q->size:0},.frontier=s->frontier,.linked=0,.prepared=0};return GH_BGM_OK;
}
gh_bgm_status gh_bgm_requests_abort_pending(gh_bgm_requests*s){
 if(!s)return GH_BGM_ARGUMENT;for(unsigned i=0;i<16;i++){request*q=&s->requests[i];
  if(q->encoded.pointer){if(gh_heap_release(s->heap,q->encoded)!=GH_MEM_OK)return GH_BGM_MEMORY;q->encoded=(gh_r207_allocation){0};}
  if(q->work.pointer){if(gh_heap_release(s->heap,q->work)!=GH_MEM_OK)return GH_BGM_MEMORY;q->work=(gh_r207_allocation){0};}memset(q,0,sizeof(*q));
 }s->frontier=0;return GH_BGM_OK;
}
void gh_bgm_requests_destroy(gh_bgm_requests*s){if(s&&gh_bgm_requests_abort_pending(s)==GH_BGM_OK)free(s);}
