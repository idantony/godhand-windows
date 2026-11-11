/* Original effect setup; upstream MIT LICENSE.upstream. Evidence JSON. */
#include "game_effects.h"
#include <string.h>
static uint32_t u32(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static gh_effect_status fail(gh_game_effects*s,uint32_t f,int d,gh_effect_status e){s->frontier=f;s->detail=d;return e;}
void gh_game_effects_pool_reset_00306d18(gh_game_effects*s){
 if(!s)return;
 for(unsigned i=0;i<1000;i++)s->vectors[i][0]=s->vectors[i][1]=s->vectors[i][2]=0;
 for(unsigned i=0;i<32;i++){gh_effect_pool_node*n=s->nodes+i;n->previous=n->next=-1;n->in_use=n->flag9=0;n->flags_a=0;n->vector_start=0;n->units=n->field12=0;}
 memset(s->pool_words,0,sizeof(s->pool_words));
 s->nodes[0].in_use=1;s->nodes[0].vector_start=0;s->nodes[0].units=1000;s->nodes[0].previous=32;
 s->head.previous=-1;s->head.next=0;s->head.in_use=s->head.flag9=0;s->head.flags_a=0;s->head.vector_start=0;s->head.units=s->head.field12=0;
}
static gh_effect_status part(const uint8_t*p,size_t z,unsigned word,gh_bundle*out){
 if(z<4)return GH_EFFECT_BOUNDS;uint32_t n=u32(p);if(n>(z-4)/4||word<1||word>n)return GH_EFFECT_BOUNDS;
 uint32_t a=u32(p+4*word);size_t end=z;if(a<4+(size_t)n*4||a>=z)return GH_EFFECT_BOUNDS;
 for(unsigned i=1;i<=n;i++){uint32_t b=u32(p+4*i);if(b>a&&b<end)end=b;}
 return gh_bundle_open(p+a,end-a,out)==GH_BUNDLE_OK?GH_EFFECT_OK:GH_EFFECT_BOUNDS;
}
static gh_effect_status emd(gh_game_effects*s,unsigned id,gh_bundle_entry e){
 if(e.size<16||u32(e.bytes)!=0x444d45||id>=1250)return fail(s,0x306af0,0,GH_EFFECT_BOUNDS);
 if(s->emd_tables[id].bytes)return GH_EFFECT_OK; /* originalreturns0, callerignores */
 uint32_t n=u32(e.bytes+4);if(n>64||n>(e.size-16)/4)return fail(s,0x306a48,0,GH_EFFECT_BOUNDS);
 const size_t table_end=16+(size_t)n*4;
 for(unsigned i=0;i<n;i++){
  uint32_t off=u32(e.bytes+16+4*i);
  if(off&&(off<table_end||off>=e.size))return fail(s,0x306a48,0,GH_EFFECT_BOUNDS);
 }
 s->emd_tables[id]=(gh_effect_view){e.bytes,e.size};
 for(unsigned i=0;i<n;i++){
  uint32_t off=u32(e.bytes+16+4*i);if(!off)continue;
  size_t end=e.size;
  for(unsigned j=0;j<n;j++){
   uint32_t next=u32(e.bytes+16+4*j);
   if(next>off&&next<end)end=next;
  }
  s->emitter_entries[i]=(gh_effect_view){e.bytes+off,end-off};
 }
 return GH_EFFECT_OK;
}
static gh_effect_status reg(gh_game_effects*s,unsigned id,const gh_bundle*b){
 gh_effect_resource*r=0;gh_bundle_entry e;gh_bundle_error be;
 for(unsigned i=0;i<64;i++)if(s->resources[i].kind==(int32_t)id){s->resources[i].references++;return GH_EFFECT_OK;}
 for(unsigned i=0;i<64;i++)if(s->resources[i].kind==5){r=s->resources+i;break;}if(!r)return fail(s,0x305908,0,GH_EFFECT_UNSUPPORTED);
 r->kind=(int32_t)id;
 be=gh_bundle_find(b,"TBL",1,&e);if(be==GH_BUNDLE_OK)r->tbl1=(gh_effect_view){e.bytes,e.size};else if(be!=GH_BUNDLE_NOT_FOUND)return fail(s,0x305358,be,GH_EFFECT_BOUNDS);
 be=gh_bundle_find(b,"TBL",0,&e);if(be==GH_BUNDLE_OK)r->tbl0=(gh_effect_view){e.bytes,e.size};else if(be!=GH_BUNDLE_NOT_FOUND)return fail(s,0x305358,be,GH_EFFECT_BOUNDS);
 be=gh_bundle_find(b,"EMD",0,&e);if(be==GH_BUNDLE_OK){gh_effect_status x=emd(s,id,e);if(x)return x;}else if(be!=GH_BUNDLE_NOT_FOUND)return fail(s,0x3057d0,be,GH_EFFECT_BOUNDS);
 r->references++;return GH_EFFECT_OK;
}

gh_effect_status gh_game_effects_register_event_002fa0b0(gh_game_effects *s, const void *data, size_t bytes) {
 if (!s || !s->ready) return GH_EFFECT_ARGUMENT;
 s->frontier = 0;
 s->detail = 0;
 if (!data) return bytes ? fail(s, 0x2fa0b0, 0, GH_EFFECT_ARGUMENT) : GH_EFFECT_OK;

 if (s->event_registered_41c && (s->event_data.bytes != data || s->event_data.size != bytes)) {
  return fail(s, 0x2fa0e8, 0, GH_EFFECT_UNSUPPORTED);
 }
 gh_bundle bundle;
 gh_bundle_entry table0, table1, emitters;
 if (gh_bundle_open(data, bytes, &bundle) || gh_bundle_find(&bundle, "TBL", 0, &table0) ||
     gh_bundle_find(&bundle, "TBL", 1, &table1) || gh_bundle_find(&bundle, "EMD", 0, &emitters) ||
     table0.size < 4 || table1.size < 4 || emitters.size < 16 || u32(emitters.bytes) != 0x00444d45u) {
  return fail(s, 0x3057d0, 0, GH_EFFECT_BOUNDS);
 }
 const uint32_t count = u32(emitters.bytes + 4);
 if (count > 64 || count > (emitters.size - 16) / 4) return fail(s, 0x306af0, 0, GH_EFFECT_BOUNDS);
 // EV01's EMD contains an empty emitter slot. A positive slot changes
 // the shared emitter pointer array and requires its release path too.
 for (uint32_t i = 0; i < count; ++i) {
  if (u32(emitters.bytes + 16 + i * 4)) return fail(s, 0x306af0, (int)i, GH_EFFECT_UNSUPPORTED);
 }
 for (unsigned i = 0; i < 64; ++i) {
  const gh_effect_resource *r = s->resources + i;
  if (r->kind == 6 && (!s->event_registered_41c || r->references == UINT32_MAX ||
                      r->tbl0.bytes != table0.bytes || r->tbl1.bytes != table1.bytes ||
                      s->emd_tables[6].bytes != emitters.bytes)) {
   return fail(s, 0x305908, (int)i, GH_EFFECT_UNSUPPORTED);
  }
 }

 gh_effect_status status = reg(s, 6, &bundle);
 if (status != GH_EFFECT_OK) return status;
 s->event_data = (gh_effect_view){data, bytes};
 s->event_registered_41c = 1;
 return GH_EFFECT_OK;
}

gh_effect_status gh_game_effects_initialize_00305a68(gh_game_effects*s,
 uint8_t*a,size_t az,uint8_t*b,size_t bz,const void*core,size_t cz,const gh_bundle*room){
 gh_bundle c0,c58,rb;gh_bundle_entry e;gh_bundle_error be;gh_effect_status x;
 if(!s||!a||az<0x100180||!b||bz<0x26060||!core||!room)return GH_EFFECT_ARGUMENT;
 if(s->event_registered_41c)return fail(s,0x2fa0e8,0,GH_EFFECT_UNSUPPORTED);
 s->frontier=0;s->detail=0;s->active=0;s->ready=0;
 /* Read eachbit anew, matching original loops; no invented clearing/killing. */
 for(unsigned i=0;i<256;i++){unsigned off=(i>>5)*4;uint32_t mask=0x80000000u>>(i&31);if((u32(b+0x26000+off)&mask)&&!(u32(b+0x26020+off)&mask)&&!(u32(b+0x26040+off)&mask))return fail(s,0x302918,(int)i,GH_EFFECT_UNSUPPORTED);}
 for(unsigned i=0;i<1024;i++){unsigned off=(i>>5)*4;uint32_t mask=0x80000000u>>(i&31);if((u32(a+0x100000+off)&mask)&&!(u32(a+0x100080+off)&mask)&&!(u32(a+0x100100+off)&mask))return fail(s,0x2fbe50,(int)i,GH_EFFECT_UNSUPPORTED);}
 for(unsigned i=0;i<64;i++)s->resources[i]=(gh_effect_resource){.kind=5};
 s->ssd=(gh_effect_view){0};s->ear=(gh_effect_view){0};s->cached408=s->cached40c=0xffffffffu;s->field414=0;
 memset(s->emd_tables,0,sizeof(s->emd_tables));memset(s->emitter_entries,0,sizeof(s->emitter_entries));
 x=part(core,cz,5,&c0);if(x)return fail(s,0x305758,x,x);x=part(core,cz,7,&c58);if(x)return fail(s,0x305758,x,x);
 x=reg(s,0,&c0);if(x)return x;x=reg(s,0x58,&c58);if(x)return x;x=reg(s,4,&c0);if(x)return x;
 be=gh_bundle_find(room,"EFF",0,&e);if(be==GH_BUNDLE_OK){if(gh_bundle_open(e.bytes,e.size,&rb)!=GH_BUNDLE_OK)return fail(s,0x305758,0,GH_EFFECT_BOUNDS);x=reg(s,1,&rb);if(x)return x;}else if(be!=GH_BUNDLE_NOT_FOUND)return fail(s,0x31ccf0,be,GH_EFFECT_BOUNDS);
 be=gh_bundle_find(room,"SSD",0,&e);if(be==GH_BUNDLE_OK)s->ssd=(gh_effect_view){e.bytes,e.size};else if(be!=GH_BUNDLE_NOT_FOUND)return fail(s,0x31ccf0,be,GH_EFFECT_BOUNDS);
 be=gh_bundle_find(room,"EAR",0,&e);if(be==GH_BUNDLE_OK)s->ear=(gh_effect_view){e.bytes,e.size};else if(be!=GH_BUNDLE_NOT_FOUND)return fail(s,0x31ccf0,be,GH_EFFECT_BOUNDS);
 gh_game_effects_pool_reset_00306d18(s);s->ready=1;return GH_EFFECT_OK;
}
