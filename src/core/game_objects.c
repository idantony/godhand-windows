#include "game_objects.h"
#include <stdlib.h>
#include <string.h>
struct gh_game_objects {
 gh_stage_slots *slots;uint8_t records[80][0x5c];
 struct {const uint8_t *bytes;size_t size;} source[80];
};
static uint32_t u32(const uint8_t *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static void w32(uint8_t *p,uint32_t n){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(n>>(8*i));}
static int manager_valid(const gh_game_objects *s){return s&&s->slots&&s->slots->configured&&s->slots->count<=80&&s->slots->records==&s->records[0][0]&&s->slots->bytes>=(size_t)s->slots->count*0x5c;}
static int valid(const gh_game_objects *s,unsigned i){const uint8_t *p=s->slots->records+(size_t)i*0x5c;
 /* Actual1FF0D8: nonzero useCount, else (!(flags&4) && data!=NULL). */
 return u32(p+4)!=0 || (!(u32(p+8)&4)&&u32(p+0x10)!=0);
}
static int find(const gh_game_objects *s,uint32_t kind){for(uint32_t n=s->slots->count;n;n--)if(valid(s,n-1)&&u32(s->slots->records+(size_t)(n-1)*0x5c+0x20)==kind)return (int)(n-1);return -1;}
static uint32_t file_kind(uint32_t kind){uint32_t prefix=kind&0xff00;return kind&&prefix!=0x500&&prefix<=0x600?kind|0x40000000:UINT32_MAX;}
gh_object_status gh_game_objects_create(gh_stage_slots *slots,int room,gh_game_objects **out){
 gh_game_objects *s;if(!slots||!out||(room!=0&&room!=1)||slots->configured)return GH_OBJECT_ARGUMENT;*out=NULL;
 s=calloc(1,sizeof(*s));if(!s)return GH_OBJECT_MEMORY;s->slots=slots;slots->records=&s->records[0][0];slots->count=room?32:80;
 slots->bytes=(size_t)slots->count*0x5c;slots->heap_handle=NULL;slots->configured=1;*out=s;return GH_OBJECT_OK;
}
void gh_game_objects_destroy(gh_game_objects *s){if(!s)return;if(s->slots->records==&s->records[0][0])memset(s->slots,0,sizeof(*s->slots));free(s);}
gh_object_status gh_game_objects_add_pinned_001fece0(gh_game_objects *s,uint32_t kind,const void *bytes,size_t size,int *original){
 int old,index=-1;uint8_t *p;
 if(original)*original=0;if(!manager_valid(s)||!bytes||!size)return GH_OBJECT_ARGUMENT;
 old=find(s,kind);if(old>=0){
  gh_stage_slots one={s->slots->records+(size_t)old*0x5c,0x5c,1,NULL,1};
  if(gh_stage_slots_reset(&one,NULL)!=GH_STAGE_OK)return GH_OBJECT_BOUNDS;
  s->source[old].bytes=NULL;s->source[old].size=0;
 }
 for(unsigned i=0;i<s->slots->count;i++)if(!valid(s,i)&&!(u32(s->slots->records+(size_t)i*0x5c+8)&4)){index=(int)i;break;}
 if(index<0)return GH_OBJECT_CAPACITY;p=s->slots->records+(size_t)index*0x5c;
 /*1FF470 order: useCount1; file-kind mapping; data; ID; file; state2.
  * Native+10 is an owner-local handle(index+1), never a truncated pointer.
  * A canonical1FF028 reset clears it, so stale borrowed views are inactive. */
 w32(p+4,1);w32(p+0x10,(uint32_t)index+1);s->source[index].bytes=bytes;s->source[index].size=size;
 w32(p+0x20,kind);w32(p+0xc,file_kind(kind));w32(p,2);w32(p+4,0xffffff9c);
 if(original)*original=1;return GH_OBJECT_OK;
}
gh_object_status gh_game_objects_find(const gh_game_objects *s,uint32_t kind,gh_object_view *out){
 if(!manager_valid(s)||!out)return GH_OBJECT_ARGUMENT;int n=find(s,kind);if(n<0)return GH_OBJECT_MISSING;
 const uint8_t *p=s->slots->records+(size_t)n*0x5c;
 if(u32(p+0x10)!=(uint32_t)n+1)return GH_OBJECT_BOUNDS;
 out->bytes=s->source[n].bytes;out->size=s->source[n].size;out->kind=u32(p+0x20);out->state=u32(p);out->flags=u32(p+8);out->file_kind=u32(p+0xc);
 uint32_t count=u32(p+4);out->use_count=count<=INT32_MAX?(int32_t)count:(int32_t)((int64_t)count-4294967296LL);return GH_OBJECT_OK;
}
gh_object_status gh_game_objects_register_startup(gh_game_objects *s,gh_game_assets *assets){
 static const uint16_t ids[]={0x361,0x362,0x363,0x364,0x365,0x366,0x368,0x376,0x377,0x3b7,
 0x379,0x37b,0x37c,0x380,0x36d,0x375,0x3be,0x3c1,0x3c6,0x3c7,0x3c8,0x3d0,0x3d1,
 0x3d2,0x3d6,0x3d7,0x3d8,0x3d9,0x3da,0x3db,0x3dc,0x3c2,0x3c3,0x3c4,0x3df,0x3e0,
 0x3e1,0x3de,0x3e2,0x3e3,0x23f};
 static const uint8_t slots[]={26,27,28,29,30,31,32,34,35,33,36,37,38,39,40,41,2,4,23,24,
 3,10,11,25,12,13,14,16,17,18,19,20,21,22,9,5,6,7,8,15,42};
 if(!manager_valid(s)||!assets||!assets->work||s->slots->count!=80)return GH_OBJECT_ARGUMENT;
 if(!(assets->work->flags&0x400))return GH_OBJECT_NOT_READY;
 for(unsigned i=0;i<43;i++){
  unsigned slot=i<2?i:slots[i-2];uint32_t id=i<2?0x100+i:ids[i-2];int original;
  gh_object_status e=gh_game_objects_add_pinned_001fece0(s,id,assets->objects[slot].pointer,assets->object_sizes[slot],&original);
  if(e)return e;
 }
 assets->work->flags|=0x80;return GH_OBJECT_OK;
}

static int has_dependency(const uint8_t*p,uint32_t key){for(unsigned i=0;i<4;i++)if(u32(p+0x4c+i*4)==key)return 1;return 0;}
/* Actual1FF710 counts pre-existingFFFF only; removedmatches donotcountasempty.
 * Multiplematchingdependencies thereforedoNOTreturnlast-reference true. */
static int remove_dependency(uint8_t*p,uint32_t key){unsigned empty=0;int found=0;for(unsigned i=0;i<4;i++){uint8_t*q=p+0x4c+i*4;uint32_t old=u32(q);if(old==key){w32(q,0xffff);found=1;}else if(old==0xffff)empty++;}return empty==3&&found;}
gh_object_status gh_game_objects_update_001fe748(gh_game_objects*s,uint32_t*f){
 if(f)*f=0;if(!manager_valid(s))return GH_OBJECT_ARGUMENT;
 /* Onlypure scalarpaths commit. AnunknownIO/free consumer isdetectedbefore
  * owner mutation; thisnativepreflight isnot anexecutedpartialoriginalcall. */
 uint8_t plan[80][0x5c];memcpy(plan,s->slots->records,(size_t)s->slots->count*0x5c);
 for(uint32_t n=s->slots->count;n;n--){uint8_t*p=plan[n-1];uint32_t state=u32(p);
  if(state==1||state==7){if(f)*f=state==1?0x1ff238:0x1ff338;return GH_OBJECT_UNSUPPORTED;}
  if(state==5)w32(p,6);else if(state==6)w32(p,7);
  if(u32(p)==8&&has_dependency(p,0xfffe)){
   uint32_t kind=u32(p+0x20);(void)remove_dependency(p,0xfffe);
   for(uint32_t j=s->slots->count;j;j--){uint8_t*q=plan[j-1];int is_valid=u32(q+4)!=0||(!(u32(q+8)&4)&&u32(q+0x10)!=0);
    if(is_valid&&has_dependency(q,kind)&&remove_dependency(q,kind)){if(f)*f=0x1ff2e0;return GH_OBJECT_UNSUPPORTED;}
   }
  }
 }
 memcpy(s->slots->records,plan,(size_t)s->slots->count*0x5c);return GH_OBJECT_OK;
}
