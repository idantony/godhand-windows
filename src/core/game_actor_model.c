/* Actual149738->14B798/149E60->149C20 and1500C0/150538/14FF80.
 * Evidence analysis/game-actor-model-evidence.json. Readable native logic,
 * explicitbinary32 reference, no VU/guestcode execution or modelreadyack. */
#include "game_actor_model.h"
#include "game_actor_model_nodes.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
static uint32_t word(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
static unsigned half(const uint8_t*p){return (unsigned)p[0]|(unsigned)p[1]<<8;}
static int shalf(const uint8_t*p){unsigned n=half(p);return n<32768?(int)n:(int)n-65536;}
static float value(const uint8_t*p){float f;memcpy(&f,p,4);return f;}
static void put(uint8_t*p,size_t o,uint32_t n){memcpy(p+o,&n,4);}
static void scalar(uint8_t*p,size_t o,float n){memcpy(p+o,&n,4);}
static void vf0(uint8_t*p,size_t o){memset(p+o,0,12);put(p,o+12,0x3f800000);}
static void identity(uint8_t*p,size_t o){memset(p+o,0,64);for(unsigned j=0;j<4;j++)put(p,o+j*20,0x3f800000);}
static float mul(float a,float b){volatile float n=a*b;return n;}
static float add(float a,float b){volatile float n=a+b;return n;}
static gh_actor_model_status fail(gh_actor_model*s,uint32_t f,unsigned detail,gh_actor_model_status e){s->frontier=f;s->detail=detail;return e;}
/* 150538 memset220 precedes14FF80, so unspecifiedbytes areoriginalzeros. */
static void part_construct(uint8_t*p){
 memset(p,0,0x220);put(p,0x214,0x41eca0);identity(p,0);identity(p,0x80);
 vf0(p,0xc0);vf0(p,0xe0);vf0(p,0x100);vf0(p,0x130);vf0(p,0x1a0);
 for(unsigned j=0;j<4;j++){put(p,0x110+j*4,0x3f800000);put(p,0x120+j*4,0x3f800000);}
}
static int alias_valid(const gh_title_player_view*r,const uint8_t*p){uintptr_t b=(uintptr_t)r->bytes,a=(uintptr_t)p;return a>=b&&a-b<=r->size&&12<=r->size-(a-b);}
static int root_domain(const gh_title_player_view*r){
 const uint8_t*p=r->bytes;for(unsigned j=0;j<3;j++)if((word(p+0x100+j*4)&0x7fffffffu)||word(p+0x110+j*4)!=0x3f800000u)return 0;
 if((word(p+0x27c)&0x7fffffffu)||(word(p+0x250)&0x40004)==0x40000)return 0;
 if(word(p+0x250)&0x800){for(unsigned c=0;c<3;c++)for(unsigned row=0;row<4;row++)if(c==row?word(p+0x80+(c*4+row)*4)!=0x3f800000u:(word(p+0x80+(c*4+row)*4)&0x7fffffffu)!=0)return 0;}
 return 1;
}
static gh_actor_model_status source_parts(gh_actor_model*s,gh_model_mesh*mesh,size_t*offset){
 if(gh_model_mesh_at(&s->script,0,mesh)!=GH_MODEL_OK)return fail(s,0x149c20,0,GH_ACTOR_MODEL_BOUNDS);
 size_t base=mesh->data_offset;if(base>s->script.size||s->script.size-base<12)return GH_ACTOR_MODEL_BOUNDS;
 const uint8_t*p=s->script.bytes+base;unsigned count=half(p+8)&255u;uint32_t rel=word(p+4);
 if(!count||rel>s->script.size-base||(size_t)count*16>s->script.size-base-rel)return fail(s,0x149c20,count,GH_ACTOR_MODEL_BOUNDS);
 *offset=base+rel;s->part_count=count;
 for(unsigned i=0;i<count;i++){const uint8_t*q=s->script.bytes+*offset+(size_t)i*16;
  int mirror=shalf(q+12),parent=shalf(q+14);
  if((mirror>=0&&(unsigned)mirror>=count)||(parent>=0&&(unsigned)parent>=i))return fail(s,0x149be8,i,GH_ACTOR_MODEL_BOUNDS);
  for(unsigned j=0;j<3;j++)if(!isfinite(value(q+j*4)))return fail(s,0x149c20,i,GH_ACTOR_MODEL_UNSUPPORTED);
 }
 return GH_ACTOR_MODEL_OK;
}
static gh_actor_model_status bind_pose(gh_actor_model*s){
 uint8_t*root=s->root.bytes;float local_saved[3];memcpy(local_saved,s->root.alias_d0,12);
 memset(s->root.alias_d0,0,12);
 /*149EB8 initialdomain147A48: zeroEuler/unitScale => originalidentity.
  * Original149738 alreadyzeroedF0XYZ, so rebuiltrootworldtranslationzero. */
 if(!(word(root+0x250)&0x800))identity(root,0x80);
 put(root,0x250,word(root+0x250)&~4u); // original27C==0 branch
 for(unsigned i=0;i<s->part_count;i++){
  gh_actor_part*part=s->parts+i;uint8_t*p=part->bytes;
  const uint8_t*parent=part->parent_is_model?root:s->parts[part->parent].bytes;
  float local[16],pm[16],world[16];memcpy(local,p,64);memcpy(pm,parent+0x80,64);
  for(unsigned col=0;col<4;col++)for(unsigned row=0;row<4;row++){
   float v=mul(pm[row],local[col*4]);v=add(v,mul(pm[4+row],local[col*4+1]));v=add(v,mul(pm[8+row],local[col*4+2]));world[col*4+row]=add(v,mul(pm[12+row],local[col*4+3]));
   if(!isfinite(world[col*4+row])){memcpy(s->root.alias_d0,local_saved,12);return fail(s,0x1501d0,i,GH_ACTOR_MODEL_UNSUPPORTED);}
  }
  memcpy(p+0x80,world,64);
  /*147568 inverse(identity+translation), then149C20 storesinverseXYZ C0.
   * No genericinverse or animationmatrix is guessed. */
  for(unsigned j=0;j<3;j++)scalar(p,0xc0+j*4,-world[12+j]);
 }
 memcpy(s->root.alias_d0,local_saved,12);s->parts_ready=1;return GH_ACTOR_MODEL_OK;
}
gh_actor_model_status gh_actor_model_initialize_00149738(gh_actor_model*s,const gh_title_player_view*r,gh_boot_work*w,
 gh_room_pools*pools,const gh_bundle_entry*md,const gh_bundle_entry*texture,uint32_t arg,uint32_t mode,gh_static_transform_policy policy,int*out){
 if(!s||!r||!r->bytes||r->size<0x2d0||!w||!pools||!pools->initialized||!out||!alias_valid(r,r->alias_d0)||!alias_valid(r,r->alias_f0))return GH_ACTOR_MODEL_ARGUMENT;
 *out=0;s->frontier=0;s->detail=0;
 if(s->bound&&(s->root.bytes!=r->bytes||s->work!=w||s->pools!=pools||s->script.bytes!=(md?md->bytes:NULL)||s->packet_argument!=arg||s->texture_mode!=mode))return GH_ACTOR_MODEL_BUSY;
 if(!s->bound){
  memset(r->alias_f0,0,12);w->flags&=0xff1fffffu;put(r->bytes,0x2c4,1);
  if(!md||!md->bytes)return GH_ACTOR_MODEL_OK;
  if(policy!=GH_STATIC_BINARY32_REFERENCE)return fail(s,0x149c20,0,GH_ACTOR_MODEL_UNSUPPORTED);
  if(gh_model_open_scr(md->bytes,md->size,&s->script)!=GH_MODEL_OK)return fail(s,0x149738,0,GH_ACTOR_MODEL_BOUNDS);
  if(mode!=1&&(!texture||!texture->bytes||texture->size<16))return fail(s,0x149738,0,GH_ACTOR_MODEL_BOUNDS);
  s->root=*r;s->work=w;s->pools=pools;s->packet_argument=arg;s->texture_mode=mode;s->texture=texture?*texture:(gh_bundle_entry){0};
  s->texture_flags=mode==1?0x2000000u:0;if(!s->texture_flags&&texture->bytes[0]=='T'&&texture->bytes[2]=='3')s->texture_flags=word(texture->bytes+12)?0x4000000u:0x8000000u;
  if(word(r->bytes+0x25c)||r->bytes[0x2b1])return fail(s,0x149350,0,GH_ACTOR_MODEL_UNSUPPORTED);
  if(word(r->bytes+0x144)||r->bytes[0x2b4])return fail(s,0x150100,0,GH_ACTOR_MODEL_UNSUPPORTED);
  memset(r->bytes+0x260,0,24);put(r->bytes,0x144,0);s->bound=1;s->native_identity=s;s->stage=1;
 }
 if(policy!=GH_STATIC_BINARY32_REFERENCE)return fail(s,0x149c20,0,GH_ACTOR_MODEL_UNSUPPORTED);
 gh_model_mesh mesh;size_t positions;gh_actor_model_status status=source_parts(s,&mesh,&positions);if(status)return status;
 if(!root_domain(&s->root))return fail(s,0x149eb8,0,GH_ACTOR_MODEL_UNSUPPORTED);
 if(s->stage==1){
  s->root.bytes[0x2b4]=(uint8_t)s->part_count;
  while(s->parts_allocated<s->part_count){
   if(word((uint8_t*)w+0x594)&0x20000){w->flags|=0x800000;return fail(s,0x1500c0,s->parts_allocated,GH_ACTOR_MODEL_MEMORY);}
   gh_actor_part*part=s->parts+s->parts_allocated;gh_room_block_pool*pool=&pools->blocks[0];void*bytes=NULL;
   gh_room_pool_status pe=gh_room_block_take_002a97a0(pool,word((uint8_t*)w+0x594),&part->handle,&bytes);
   if(pe||!bytes){w->flags|=0x800000;return fail(s,0x1500c0,s->parts_allocated,GH_ACTOR_MODEL_MEMORY);}
   pool->remaining1c--;if(pool->remaining1c<pool->limit24)pool->limit24=pool->remaining1c;
   part->bytes=bytes;part->alias_d0=part->bytes+0x30;part->alias_f0=part->bytes+0xb0;part_construct(part->bytes);
   const uint8_t*q=s->script.bytes+positions+(size_t)s->parts_allocated*16;memcpy(part->alias_d0,q,12);
   part->parent=(int16_t)shalf(q+14);part->parent_is_model=part->parent<0;part->mirror=(int16_t)shalf(q+12);part->mirror_is_self=part->mirror<0;
   s->parts_allocated++;
  }
  fenv_t saved;if(fegetenv(&saved)||fesetround(FE_TONEAREST))return fail(s,0x149eb8,0,GH_ACTOR_MODEL_UNSUPPORTED);
  status=bind_pose(s);int restored=fesetenv(&saved);if(restored)return fail(s,0x149eb8,0,GH_ACTOR_MODEL_UNSUPPORTED);if(status)return status;s->stage=2;
 }
 if(s->stage==2){
  if(word((uint8_t*)w+0x594)&0x40000){w->flags|=0x400000;return fail(s,0x149310,0,GH_ACTOR_MODEL_MEMORY);}
  gh_room_block_pool*pool=&pools->blocks[1];void*bytes=NULL;gh_room_pool_status pe=gh_room_block_take_002a97a0(pool,word((uint8_t*)w+0x594),&s->first_node_handle,&bytes);
  if(pe||!bytes){w->flags|=0x400000;return fail(s,0x149310,0,GH_ACTOR_MODEL_MEMORY);}
  pool->remaining1c--;if(pool->remaining1c<pool->limit24)pool->limit24=pool->remaining1c;s->first_node=bytes;memset(bytes,0,0x480);
  const unsigned ones[]={0x2a8,0x1b0,0x1c4,0x1d8,0x280,0x294};for(unsigned i=0;i<sizeof(ones)/sizeof(*ones);i++)put(bytes,ones[i],0x3f800000);s->stage=3;
 }
 if(!s->packet_heap)return fail(s,0x148420,0,GH_ACTOR_MODEL_UNSUPPORTED);
 return gh_actor_model_build_nodes(s,policy,out);
}
gh_actor_model_status gh_actor_model_part_at(const gh_actor_model*s,unsigned i,const gh_actor_part**out){if(!s||!s->bound||!out)return GH_ACTOR_MODEL_ARGUMENT;if(i>=s->parts_allocated)return GH_ACTOR_MODEL_BOUNDS;*out=s->parts+i;return GH_ACTOR_MODEL_OK;}
gh_actor_model_status gh_actor_model_release(gh_actor_model*s){
 if(!s)return GH_ACTOR_MODEL_ARGUMENT;if(s->external_owner||s->external_borrowers)return GH_ACTOR_MODEL_BUSY;if(s->bound&&s->native_identity!=s)return GH_ACTOR_MODEL_ARGUMENT;if(!s->bound){*s=(gh_actor_model){0};return GH_ACTOR_MODEL_OK;}
 gh_actor_model_status released=gh_actor_model_release_nodes(s);if(released)return released;
 if(s->first_node_handle){if(gh_room_block_put_002a9800(&s->pools->blocks[1],s->first_node_handle))return GH_ACTOR_MODEL_BUSY;s->pools->blocks[1].remaining1c++;s->first_node_handle=0;}
 for(unsigned i=s->parts_allocated;i;i--){gh_actor_part*p=s->parts+i-1;if(gh_room_block_put_002a9800(&s->pools->blocks[0],p->handle))return GH_ACTOR_MODEL_BUSY;s->pools->blocks[0].remaining1c++;s->parts_allocated--;}
 s->root.bytes[0x2b4]=0;put(s->root.bytes,0x144,0);*s=(gh_actor_model){0};return GH_ACTOR_MODEL_OK;
}

gh_actor_model_status gh_actor_model_bind_packet_inputs(gh_actor_model*s,gh_heap*h,const void*fog){if(!s||!h)return GH_ACTOR_MODEL_ARGUMENT;if(s->packet_heap==h&&s->borrowed_fog_rgb==fog)return GH_ACTOR_MODEL_OK;if(s->bound||s->nodes){if(s->packet_heap!=h||s->borrowed_fog_rgb||s->ready||!fog)return GH_ACTOR_MODEL_BUSY;s->borrowed_fog_rgb=fog;return GH_ACTOR_MODEL_OK;}s->packet_heap=h;s->borrowed_fog_rgb=fog;return GH_ACTOR_MODEL_OK;}
