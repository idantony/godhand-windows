#include "game_enemy_model.h"
#include "game_enemy_row2.h"
#include "../assets/tm3.h"
#include <string.h>
static uint32_t word(const uint8_t*p,size_t o){uint32_t v;memcpy(&v,p+o,4);return v;}
static uint16_t half(const uint8_t*p,size_t o){uint16_t v;memcpy(&v,p+o,2);return v;}
/* SearchData scans tags even when reserved directory holes have offset0.
 * Host extent ends at the next populated entry; no game bytes are relocated. */
static gh_bundle_error sparse_find(const uint8_t*b,size_t n,const char*tag,unsigned occurrence,gh_bundle_entry*out){
 if(n<4)return GH_BUNDLE_BOUNDS;uint32_t count=word(b,0);if(!count||count>(n-4)/8)return GH_BUNDLE_BOUNDS;
 size_t directory=4+(size_t)count*8,previous=directory;uint32_t selected=UINT32_MAX;
 for(uint32_t i=0;i<count;i++){
  size_t offset=word(b,4+4*(size_t)i);const uint8_t*t=b+4+4*(size_t)count+4*(size_t)i;
  if(!memchr(t,0,4))return GH_BUNDLE_UNSUPPORTED;
  if(!offset){if(t[0]||t[1]||t[2]||t[3])return GH_BUNDLE_BOUNDS;continue;}
  if(offset<previous||offset>n)return GH_BUNDLE_BOUNDS;previous=offset;
  if(!strcmp((const char*)t,tag)&&selected==UINT32_MAX){if(!occurrence)selected=i;else occurrence--;}
 }
 if(selected==UINT32_MAX){*out=(gh_bundle_entry){0};return GH_BUNDLE_NOT_FOUND;}
 size_t start=word(b,4+4*(size_t)selected),end=n;
 for(uint32_t i=selected+1;i<count;i++){size_t next=word(b,4+4*(size_t)i);if(next){end=next;break;}}
 *out=(gh_bundle_entry){b+start,start,end-start,{0}};memcpy(out->tag,b+4+4*(size_t)count+4*(size_t)selected,4);return GH_BUNDLE_OK;
}
/* 149AD8 resolves row zero TXE source names in the TM3 name table. */
static gh_actor_model_status texture_exchange_00149ad8(const gh_bundle_entry *texture,
 const gh_bundle_entry *exchange,uint8_t result[16])
{
 gh_tm3_bundle tm3;
 if(gh_tm3_open(texture->bytes,texture->size,&tm3))
  return GH_ACTOR_MODEL_UNSUPPORTED;
 if(exchange->size<8)return GH_ACTOR_MODEL_BOUNDS;
 const uint8_t *bytes=exchange->bytes;
 if(memcmp(bytes,"TXE\0",4)||half(bytes,4)!=1)return GH_ACTOR_MODEL_UNSUPPORTED;
 const unsigned name_count=half(bytes,6);
 size_t row_offset=8+(size_t)name_count*8;
 if(row_offset>=exchange->size)return GH_ACTOR_MODEL_BOUNDS;
 const uint8_t *row=bytes+row_offset;
 const unsigned count=row[0];
 if((size_t)count*2>exchange->size-row_offset-1)return GH_ACTOR_MODEL_BOUNDS;

 const size_t names_offset=(16+(size_t)tm3.count*4+7)&~(size_t)7;
 if(names_offset>tm3.size||tm3.count>(tm3.size-names_offset)/8)
  return GH_ACTOR_MODEL_BOUNDS;
 const uint8_t *names=tm3.bytes+names_offset;
 uint8_t mapped[16];
 memset(mapped,255,sizeof(mapped));
 for(unsigned i=0;i<count;++i){
  const unsigned replacement=row[1+i*2],name=row[2+i*2];
  if(name>=name_count||replacement>=tm3.count)return GH_ACTOR_MODEL_BOUNDS;
  const uint8_t *key=bytes+8+(size_t)name*8;
  for(unsigned member=0;member<tm3.count;++member){
   if(memcmp(names+(size_t)member*8,key,8))continue;
   if(member>=sizeof(mapped))return GH_ACTOR_MODEL_UNSUPPORTED;
   mapped[member]=(uint8_t)replacement;
   break;
  }
 }
 memcpy(result,mapped,sizeof(mapped));
 return GH_ACTOR_MODEL_OK;
}

gh_actor_model_status gh_enemy_model_initialize_00208268(gh_enemy_model*s,const gh_title_player_view*root,
 const gh_object_view*resource,gh_boot_work*work,gh_room_pools*pools,gh_heap*heap,const void*fog,
 gh_static_transform_policy policy,int*original){
 if(!s||!root||!root->bytes||root->size<0x1650||!resource||!work||!pools||!heap||!fog||!original)return GH_ACTOR_MODEL_ARGUMENT;
 *original=0;s->frontier=0;
 uint32_t dependency=word(root->bytes,0x564);
 const int r105_exchange=work->room==0x105&&root->bytes[0x1650]==0&&root->bytes[0x1651]==2;
 const int r101_exchange=work->room==0x101&&root->bytes[0x1650]==4&&root->bytes[0x1651]==9;
 const int exchange=(r105_exchange||r101_exchange)&&half(root->bytes,0x2fe)==0x202&&
  word(root->bytes,0x1648)==0x200&&word(root->bytes,0x164c)==1;
 if(exchange&&s->stage&&dependency==0x227)dependency=0x200;
 unsigned occurrence;
 const unsigned kind=half(root->bytes,0x2fe);
 if(kind==0x220&&dependency==0x220&&work->room==0x100)occurrence=0;
 else if(kind!=0x202||gh_enemy_model_occurrence_002ddea0(kind,dependency,&occurrence)){
  s->frontier=0x2ddea0;return GH_ACTOR_MODEL_UNSUPPORTED;
 }
 if(!exchange&&(dependency==0x200||dependency==0x202||dependency==0x203)&&(word(root->bytes,0x164c)&1)){
  s->frontier=0x149ad8;return GH_ACTOR_MODEL_UNSUPPORTED;
 }
 if(resource->kind!=kind||!resource->bytes||!resource->file_extent_known||resource->file_extent>resource->size)return GH_ACTOR_MODEL_ARGUMENT;
 if(s->stage&&(s->resource!=resource->bytes||s->occurrence!=occurrence))return GH_ACTOR_MODEL_BUSY;
 if(!s->stage){
  gh_bundle_error e=sparse_find(resource->bytes,resource->file_extent,"MD",occurrence,&s->geometry);
  if(e&&e!=GH_BUNDLE_NOT_FOUND){s->frontier=0x31ccf0;return GH_ACTOR_MODEL_BOUNDS;}
  e=sparse_find(resource->bytes,resource->file_extent,"TM3",occurrence,&s->texture);
  if(e&&e!=GH_BUNDLE_NOT_FOUND){s->frontier=0x31ccf0;return GH_ACTOR_MODEL_BOUNDS;}
  if(exchange){
   gh_bundle_entry txe;
   uint8_t replacement[16];
   e=sparse_find(resource->bytes,resource->file_extent,"TXE",0,&txe);
   if(e||resource->file_extent<0x48||txe.offset!=word(resource->bytes,0x44)){
    s->frontier=0x149ad8;return GH_ACTOR_MODEL_UNSUPPORTED;
   }
   const gh_actor_model_status status=texture_exchange_00149ad8(&s->texture,&txe,replacement);
   if(status){s->frontier=0x149ad8;return status;}
   uint8_t *actor=root->bytes;
   memcpy(actor+0x2d4,replacement,sizeof(replacement));
   uint32_t flags=word(actor,0x254)|0x10000000u;
   memcpy(actor+0x254,&flags,4);
   const uint32_t exchanged_dependency=0x227;
   memcpy(actor+0x564,&exchanged_dependency,4);
  }
  s->resource=resource->bytes;s->occurrence=occurrence;s->stage=1;
 }
 /* 20A default and unexchanged 200 both use the selected spans. */
 if(!s->texture.bytes){s->stage=2;return GH_ACTOR_MODEL_OK;}
 gh_actor_model_status e=gh_actor_model_bind_packet_inputs(&s->model,heap,fog);if(e)return e;
 e=gh_actor_model_initialize_00149738(&s->model,root,work,pools,s->geometry.bytes?&s->geometry:NULL,&s->texture,0,0,policy,original);
 s->frontier=s->model.frontier;if(!e)s->stage=2;return e;
}
gh_actor_model_status gh_enemy_model_release_prefix(gh_enemy_model*s){if(!s)return GH_ACTOR_MODEL_ARGUMENT;gh_actor_model_status e=gh_actor_model_release(&s->model);if(e)return e;*s=(gh_enemy_model){0};return GH_ACTOR_MODEL_OK;}
