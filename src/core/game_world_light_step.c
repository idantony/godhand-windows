#include "game_world_light_step.h"
#include "game_scene_at_geometry.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <fenv.h>
static uint32_t word(const uint8_t*p){uint32_t v;memcpy(&v,p,4);return v;}
static void put(uint8_t*p,uint32_t v){memcpy(p,&v,4);}
static float scalar(const uint8_t*p){float v;memcpy(&v,p,4);return v;}
static float mul(float a,float b){volatile float r=a*b;return r;}
static float add(float a,float b){volatile float r=a+b;return r;}
static gh_light_status fail(gh_game_world_light*s,uint32_t pc,int detail,gh_light_status e){s->frontier=pc;s->detail=detail;return e;}
typedef struct {uint8_t *bytes;uint32_t flags;uint8_t write_flags,write_area,area;} model_plan;
typedef struct {uint32_t area,cached_area,cached_row;const uint8_t *fog;} fog_plan;

static gh_light_status fog_region(gh_game_world_light *s,const gh_light_move_bindings *b,fog_plan *plan)
{
 const uint8_t *p=s->scalar;
 *plan=(fog_plan){.cached_area=word(p+0x16128),.cached_row=word(p+0x1612c)};
 if(!s->area.bytes)return GH_LIGHT_OK;
 if(!b||!b->player||!b->player->bytes||b->player->size<0xbc)
  return fail(s,0x2db6b0,0,GH_LIGHT_UNSUPPORTED);
 const uint8_t *areas=s->area.bytes;
 if(s->area.size<16||word(areas)>(s->area.size-16)/0x50)
  return fail(s,0x1f83e8,0,GH_LIGHT_BOUNDS);
 float point[3];
 memcpy(point,b->player->bytes+0xb0,sizeof(point));
 point[1]=add(point[1],.5f);
 /* FAR uses the first matching volume in authored order. */
 uint32_t area_count=word(areas);
 for(uint32_t i=0;i<area_count;i++){
  const uint8_t *area=areas+16+(size_t)i*0x50;
  int inside;
  gh_scene_at_status status=gh_scene_at_region_contains_001f83e8(area+0x20,point,&inside);
  if(status)return fail(s,0x1f83e8,(int)i,status==GH_AT_BOUNDS?GH_LIGHT_BOUNDS:GH_LIGHT_UNSUPPORTED);
  if(inside){plan->area=area[2];break;}
 }

 /* 2D8EB0 takes the active LI33 preset's 16-byte fog rows, whose ID
  * is the last word. Guest +16100 pointers are borrowed native slots. */
 uint32_t preset=word(p+0x16288);
 if(preset>=8)return fail(s,0x2d8eb0,(int)preset,GH_LIGHT_BOUNDS);
 const gh_bundle_entry *fog=&s->slots[preset];
 if(!fog->bytes)return GH_LIGHT_OK;
 if(fog->size<4||word(fog->bytes)>(fog->size-4)/16)
  return fail(s,0x2d8eb0,(int)preset,GH_LIGHT_BOUNDS);
 uint32_t fog_count=word(fog->bytes);
 if(!fog_count)return GH_LIGHT_OK;
 if(!plan->area){plan->cached_area=0;return GH_LIGHT_OK;}
 const uint8_t *rows=fog->bytes+4;
 if(plan->cached_area==plan->area){
  if(plan->cached_row>=fog_count)return fail(s,0x2d8eb0,(int)plan->cached_row,GH_LIGHT_BOUNDS);
  plan->fog=rows+(size_t)plan->cached_row*16;
  return GH_LIGHT_OK;
 }
 plan->cached_area=plan->area;
 for(uint32_t i=0;i<fog_count;i++){
  const uint8_t *row=rows+(size_t)i*16;
  if(word(row+12)==plan->area){plan->cached_row=i;plan->fog=row;break;}
 }
 return GH_LIGHT_OK;
}
int gh_game_world_light_class_002d74b8(uint16_t kind){return (kind>=0x100&&kind<=0x600)||kind==0xff01||kind==0xff02;}
static gh_light_status models(gh_game_world_light*s,const gh_room_vector*v,const gh_light_move_bindings*b,model_plan*plans){
 for(uint32_t i=0;i<v->count;i++){
  if(!b||!b->actor_view)return fail(s,0x2d74b8,(int)i,GH_LIGHT_UNSUPPORTED);
  uint32_t pc=0;gh_title_player_view view={0};gh_light_status e=b->actor_view(b->context,word((const uint8_t*)v->allocation.pointer+(size_t)i*4),&view,&pc);
  if(e)return fail(s,pc?pc:0x2d74b8,(int)i,e);
  if(!view.bytes||view.size<0x300)return fail(s,0x2d74b8,(int)i,GH_LIGHT_BOUNDS);
  uint8_t*p=view.bytes;uint16_t kind;memcpy(&kind,p+0x2fe,2);uint32_t flags=word(p+0x250);
  if(!gh_game_world_light_class_002d74b8(kind)||(flags&0x80008000u))continue;
  model_plan*q=plans+i;q->bytes=p;q->flags=flags|0x200000u;q->write_flags=1;
  if(flags&0x80000u)continue;
  /* Actual 2C3190: absent pointer68 means every group is visible. A
     positive guest token requires a separately recovered typed mask owner. */
  if(!b->scene_at||b->scene_at->field68)return fail(s,0x2c3190,(int)i,GH_LIGHT_UNSUPPORTED);
  q->write_area=1;q->area=0;
  if(!s->table.bytes)continue;
  if(kind>=0x500&&kind<=0x5ff)continue;
  if(view.size<0x4ac)return fail(s,0x2d7560,(int)i,GH_LIGHT_BOUNDS);
  if(word(p+0x4a8)&0x10000000u)continue;
  const uint8_t*table=s->table.bytes;size_t n=s->table.size;
  if(n<16||word(table)>(n-16)/0x50)return fail(s,0x1f83e8,(int)i,GH_LIGHT_BOUNDS);
  float point[3];memcpy(point,p+0xb0,12);point[1]=add(point[1],.5f);
  for(uint32_t j=word(table);j;j--){const uint8_t*r=table+16+(size_t)(j-1)*0x50;int inside;
   gh_scene_at_status ae=gh_scene_at_region_contains_001f83e8(r+0x20,point,&inside);
   if(ae)return fail(s,0x1f83e8,(int)i,ae==GH_AT_BOUNDS?GH_LIGHT_BOUNDS:GH_LIGHT_UNSUPPORTED);
   /* Original reverse traversal does not break: lower-index match wins. */
   if(inside)q->area=r[2];
  }
 }
 return GH_LIGHT_OK;
}

gh_light_status gh_game_set_background_0030e5c0(gh_boot_work*work,unsigned mode,uint8_t r,uint8_t g,uint8_t b){
 if(!work)return GH_LIGHT_ARGUMENT;if(mode>1)return GH_LIGHT_OK;
 uint8_t*p=(uint8_t*)work;uint64_t a,c;memcpy(&a,p+0x3e0,8);memcpy(&c,p+0x270,8);
 uint64_t bits=mode?0x16u:0x10u;a=(a&UINT64_C(0xffffffffffff8000))|bits;c=(c&UINT64_C(0xffffffffffff8000))|bits;
 if(mode)put(p+0x60c,(uint32_t)r|(uint32_t)g<<8|(uint32_t)b<<16);
 memcpy(p+0x3e0,&a,8);memcpy(p+0x270,&c,8);return GH_LIGHT_OK;
}

gh_light_status gh_game_world_light_move_native_002d7560(gh_game_world_light*s,gh_boot_work*work,const gh_room_pools*pools,const gh_light_move_bindings*b,gh_camera_numeric_policy policy){
 if(!s||!s->constructed||!work)return GH_LIGHT_ARGUMENT;s->frontier=0;s->detail=0;
 uint8_t*p=s->scalar;const uint8_t*w=(const uint8_t*)work;
 if((word(w+0x5d8)&0x4000u)||(word(w+0x5e0)&0x80000u))return GH_LIGHT_OK;
 if(!s->ready)return GH_LIGHT_ARGUMENT;
 if(policy!=GH_CAMERA_NATIVE_BINARY32_REFERENCE)return fail(s,0x2d7560,0,GH_LIGHT_UNSUPPORTED);

 if(word(p+0x16284)&0x10u)return fail(s,0x2d8c60,0,GH_LIGHT_UNSUPPORTED);
 const gh_room_vector*objects;gh_room_pool_status ps=gh_room_registry_view(pools,0x754c58,&objects);
 if(ps)return fail(s,0x2d7560,ps,GH_LIGHT_ARGUMENT);
 if(objects->count&&(!b||!b->actor_view))return fail(s,0x2d74b8,0,GH_LIGHT_UNSUPPORTED);
 for(unsigned i=0;i<256;i++)if(word(p+0x80+i*0x70+4))return fail(s,0x2d7560,(int)i,GH_LIGHT_UNSUPPORTED);
 size_t count=word(p+0x78);if(count>256)return fail(s,0x2d8ff8,0,GH_LIGHT_BOUNDS);
 for(size_t i=0;i<count&&i<48;i++)if(word(p+0x80+i*0x70+0x58)&0x100000u)return fail(s,0x156488,(int)i,GH_LIGHT_UNSUPPORTED);
 /* Prepare fog and model writes before committing the supported invocation. */
 model_plan*plans=objects->count?calloc(objects->count,sizeof(*plans)):NULL;
 if(objects->count&&!plans)return fail(s,0x2d7560,0,GH_LIGHT_MEMORY);
 float target[5]={scalar(p+0x10),scalar(p+0x14),(float)p[4],(float)p[5],(float)p[6]},next[5];
 fenv_t saved;if(feholdexcept(&saved)){free(plans);return fail(s,0x2d7560,0,GH_LIGHT_UNSUPPORTED);}if(fesetround(FE_TONEAREST)){fesetenv(&saved);free(plans);return fail(s,0x2d7560,0,GH_LIGHT_UNSUPPORTED);}
 fog_plan fog;
 gh_light_status status=fog_region(s,b,&fog);
 if(!status)status=models(s,objects,b,plans);
 if(status){(void)fesetenv(&saved);free(plans);return status;}
 if(fog.fog){
  target[0]=scalar(fog.fog+4);target[1]=scalar(fog.fog+8);
  for(unsigned i=0;i<3;i++)target[2+i]=(float)fog.fog[i];
 }
 int valid=1;for(unsigned i=0;i<5;i++){next[i]=add(mul(scalar(p+0x16150+4*i),.96f),mul(target[i],.04f));if(!isfinite(next[i]))valid=0;}
 int restore=fesetenv(&saved);if(!valid||restore){free(plans);return fail(s,0x2d7560,0,GH_LIGHT_UNSUPPORTED);}
 put(p+0x16124,fog.area);put(p+0x16128,fog.cached_area);put(p+0x1612c,fog.cached_row);
 memcpy(p+0x16150,next,sizeof(next));
 put(p+0x16138,255);put(p+0x16134,0);put(p+0x16140,255);put(p+0x1613c,255);
 for(uint32_t i=0;i<objects->count;i++){model_plan*q=plans+i;if(q->write_flags)put(q->bytes+0x250,q->flags);if(q->write_area)q->bytes[0x2cc]=q->area;}free(plans);
 gh_game_set_background_0030e5c0(work,1,p[0],p[1],p[2]);
 put(p+0x16144,word(p+0x16138));put(p+0x16148,word(p+0x1613c));put(p+0x1614c,word(p+0x16140));
 return GH_LIGHT_OK;
}
gh_light_status gh_game_world_light_move_002d7560(gh_game_world_light*s,gh_boot_work*w,const gh_room_pools*p,gh_camera_numeric_policy policy){return gh_game_world_light_move_native_002d7560(s,w,p,NULL,policy);}
