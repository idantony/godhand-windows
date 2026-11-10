#include "game_player_springs.h"
#include <string.h>
#include <math.h>
#include <fenv.h>
static uint32_t u32(const uint8_t*p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static unsigned u16(const uint8_t*p){return p[0]|(unsigned)p[1]<<8;}
static void ww(uint8_t*p,uint32_t v){memcpy(p,&v,4);}
static float f(const uint8_t*p){float v;memcpy(&v,p,4);return v;}
static void sf(uint8_t*p,float v){memcpy(p,&v,4);}
static float add(float a,float b){volatile float v=a+b;return v;}
static float mul(float a,float b){volatile float v=a*b;return v;}
static float divv(float a,float b){volatile float v=a/b;return v;}
static float root(float a){volatile float v=sqrtf(a);return v;}
static float norm(const float*v){return root(add(add(mul(v[0],v[0]),mul(v[1],v[1])),mul(v[2],v[2])));}
static int span(const uint8_t*elf,size_t bytes,uint32_t addr,size_t n,const uint8_t**out,size_t*available){
 if(bytes<52||memcmp(elf,"\177ELF\1\1\1",7)||u16(elf+18)!=8)return 0;uint32_t table=u32(elf+28);unsigned stride=u16(elf+42),count=u16(elf+44);
 if(stride<32||count>128||table>bytes||count>(bytes-table)/stride)return 0;
 for(unsigned i=0;i<count;i++){const uint8_t*p=elf+table+(size_t)i*stride;uint32_t offset=u32(p+4),base=u32(p+8),files=u32(p+16);
  if(u32(p)!=1||addr<base||(uint64_t)addr-base>files)continue;size_t delta=addr-base;if(offset>bytes||files>bytes-offset||n>files-delta)return 0;*out=elf+offset+delta;if(available)*available=files-delta;return 1;}
 return 0;
}
static gh_player_springs_status fail(gh_game_player_springs*s,uint32_t pc,gh_player_springs_status e){s->frontier=pc;return e;}
static int part(const gh_actor_model*m,unsigned i,const gh_actor_part**p){return gh_actor_model_part_at(m,i,p)==GH_ACTOR_MODEL_OK&&(*p)->bytes&&(*p)->alias_f0;}
static void xyz(uint8_t*p,const float*v){for(unsigned k=0;k<3;k++)sf(p+k*4,v[k]);}
static void transform(float*out,const uint8_t*m,const uint8_t*v,int point){for(unsigned k=0;k<4;k++){float a=mul(f(m+k*4),f(v));a=add(a,mul(f(m+16+k*4),f(v+4)));a=add(a,mul(f(m+32+k*4),f(v+8)));out[k]=point?add(a,mul(f(m+48+k*4),f(v+12))):a;}}
static gh_player_springs_status setup(gh_game_player_springs*s,uint8_t*records){
 const gh_actor_model*m=&s->child->model;const uint8_t*raw=s->child->object.view.bytes;
 for(unsigned i=0;i<s->count;i++){unsigned id=s->indices[0][i];if(id==255)continue;if(id>=44)return fail(s,0x1039b0,GH_SPRINGS_BOUNDS);const gh_actor_part*p;if(!part(m,id,&p))return fail(s,0x1039a8,GH_SPRINGS_BOUNDS);
  uint8_t*r=records+id*0xa0;memset(r+0x20,0,16);memset(r+0x60,0,16);ww(r+0x70,0);ww(r+0x74,0);ww(r+0x78,0);r[0x90]=r[0x91]=0;
  unsigned target=s->indices[6][i],copy=s->indices[5][i];float v[4];
  if(target==255){if(copy==255){ww(r,0);sf(r+4,-0.1f);ww(r+8,0);ww(r+0x10,0);sf(r+0x14,-1.0f);ww(r+0x18,0);sf(r+0x60,0.1f);transform(v,p->bytes+0x80,r,1);xyz(r+0x40,v);}
   else{if(copy>=44)return fail(s,0x103aa8,GH_SPRINGS_BOUNDS);uint8_t*src=records+copy*0xa0;memcpy(r+0x60,src+0x60,4);memcpy(r,src,12);memcpy(r+0x10,src+0x10,12);memcpy(r+0x40,p->alias_f0,12);transform(v,raw+0x80,r,0);for(unsigned k=0;k<4;k++)sf(r+0x40+k*4,add(f(r+0x40+k*4),v[k]));}
  }else{const gh_actor_part*q;if(!part(m,target,&q))return fail(s,0x103b78,GH_SPRINGS_BOUNDS);memcpy(r+0x40,q->alias_f0,12);float d[3];for(unsigned k=0;k<3;k++)d[k]=add(f(p->alias_f0+k*4),-f(q->alias_f0+k*4));sf(r+0x60,norm(d));for(unsigned k=0;k<3;k++)d[k]=add(f(q->alias_f0+k*4),-f(p->alias_f0+k*4));xyz(r,d);float len=norm(d);if(len>0){float inv=divv(1.0f,len);for(unsigned k=0;k<3;k++)d[k]=mul(d[k],inv);}else for(unsigned k=0;k<3;k++)d[k]=0;xyz(r+0x10,d);}
  memset(r+0x30,0,12);for(unsigned off=0;off<0x90;off+=4)if(!isfinite(f(r+off)))return fail(s,0x1038e0,GH_SPRINGS_NUMERIC);
 }
 for(unsigned i=0;i<s->count;i++){unsigned id=s->indices[0][i];if(id==255)continue;uint8_t*r=records+id*0xa0;for(unsigned j=1;j<=4;j++){unsigned neighbor=s->indices[j][i];if(!s->table_addresses[j]||neighbor==255)continue;if(neighbor>=44)return fail(s,0x103d04,GH_SPRINGS_BOUNDS);const uint8_t*q=records+neighbor*0xa0;float d[3];for(unsigned k=0;k<3;k++)d[k]=add(f(r+0x40+k*4),-f(q+0x40+k*4));float radius=mul(norm(d),0.5f);if(!isfinite(radius))return fail(s,0x103d04,GH_SPRINGS_NUMERIC);sf(r+0x20+(j-1)*4,radius);}}
 return GH_SPRINGS_OK;
}
gh_player_springs_status gh_game_player_springs_initialize_00100870(gh_game_player_springs*s,gh_game_player_child*c,gh_core_save_view save,const void*data,size_t bytes,gh_static_transform_policy policy){
 if(!s||!c||!c->complete||!c->object.view.bytes||c->object.view.size<GH_PLAYER_CHILD_BYTES||c->model.root.bytes!=c->object.view.bytes||!c->model.bound||!save.bytes||save.size<GH_CORE_SAVE_BYTES||!data)return GH_SPRINGS_ARGUMENT;
 if(policy!=GH_STATIC_BINARY32_REFERENCE)return fail(s,0x1038e0,GH_SPRINGS_NUMERIC);unsigned costume=gh_core_save_costume(save);if(s->configured&&(s->child!=c||s->costume!=costume))return GH_SPRINGS_ARGUMENT;if(s->initialized)return GH_SPRINGS_OK;
 const uint8_t*elf=data;
 if(!s->configured){static const uint32_t table[4][12]={
  {40,0x3bbf80,0x3bbff8,0,0,0,0x3bbfa8,0x3bbfd0,0x3bc020,0x3bc0c0,0x3bc160,0x460e00},
  {24,0x3bc200,0x3bc248,0,0,0,0x3bc218,0x3bc230,0x3bc260,0x3bc2c0,0x3bc320,0x461040},
  {33,0x3bc380,0x3bc3f8,0x3bc420,0,0,0x3bc3a8,0x3bc3d0,0x3bc448,0x3bc4d0,0x3bc558,0x461250},
  {20,0x3bc5e0,0x3bc628,0,0,0,0x3bc5f8,0x3bc610,0x3bc640,0x3bc690,0x3bc6e0,0x4613a0}};
  unsigned profile=costume==2||costume==3?1:costume==4||costume==5?2:costume==6||costume==7?3:0;const uint32_t*t=table[profile];gh_game_player_springs n={0};n.count=t[0];n.child=c;n.costume=costume;
  for(unsigned i=0;i<11;i++){n.table_addresses[i]=t[i+1];if(!t[i+1]){if(i<7)memset(n.indices[i],255,44);continue;}const uint8_t*p;
   if(i<7){if(!span(elf,bytes,t[i+1],n.count,&p,NULL))return fail(s,0x100870,GH_SPRINGS_BOUNDS);memcpy(n.indices[i],p,n.count);}
   else if(i<10){if(!span(elf,bytes,t[i+1],n.count*4,&p,NULL))return fail(s,0x100870,GH_SPRINGS_BOUNDS);for(unsigned j=0;j<n.count;j++)n.coefficients[i-7][j]=u32(p+j*4);}
   else if(!span(elf,bytes,t[i+1],1,&n.table2084,&n.table2084_available)){/* Original2084 is mutable BSS scratch, not an immutable ELF table. 1038E0 does not read it. */uint32_t ph=u32(elf+28);unsigned stride=u16(elf+42),cnt=u16(elf+44);int mapped=0;for(unsigned k=0;k<cnt;k++){const uint8_t*q=elf+ph+(size_t)k*stride;if(u32(q)==1&&t[i+1]>=u32(q+8)&&(uint64_t)t[i+1]<(uint64_t)u32(q+8)+u32(q+20))mapped=1;}if(!mapped)return fail(s,0x100870,GH_SPRINGS_BOUNDS);}
  }
  n.configured=1;*s=n;uint8_t*p=c->object.view.bytes;ww(p+0x2050,n.count);for(unsigned i=0;i<7;i++)ww(p+0x2054+i*4,0);ww(p+0x2070,0);ww(p+0x2074,0);ww(p+0x2078,0);ww(p+0x207c,0);ww(p+0x2080,0);ww(p+0x2084,0);ww(p+0x2088,0);ww(p+0x208c,0x3c75c28f);ww(p+0x2090,profile==2?0x3f000000:0x3f4ccccd);ww(p+0x2094,1);ww(p+0x2098,0);ww(p+0x209c,profile==2?0x3f800000:0x3f4ccccd);ww(p+0x20a0,0x3f000000);ww(p+0x20a4,0);ww(p+0x20a8,0);ww(p+0x20ac,0x100);ww(p+0x20b0,0);ww(p+0x20b4,0);ww(p+0x20b8,0);ww(p+0x20c0,0x3f860a92);
 }
 uint8_t records[44*0xa0];memcpy(records,c->object.view.bytes+0x4d0,sizeof(records));fenv_t env;if(feholdexcept(&env))return fail(s,0x1038e0,GH_SPRINGS_NUMERIC);if(fesetround(FE_TONEAREST)){(void)fesetenv(&env);return fail(s,0x1038e0,GH_SPRINGS_NUMERIC);}gh_player_springs_status e=setup(s,records);if(fesetenv(&env))return fail(s,0x1038e0,GH_SPRINGS_NUMERIC);if(e)return e;memcpy(c->object.view.bytes+0x4d0,records,sizeof(records));s->initialized=1;s->frontier=0;return GH_SPRINGS_OK;
}
