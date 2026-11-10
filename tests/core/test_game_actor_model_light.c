#include "game_actor_model_light.h"
#include "disc.h"
#include "game_actor_light_attachment.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fenv.h>
#include <math.h>
#define C(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t word(const void*p){uint32_t n;memcpy(&n,p,4);return n;}
static void put(void*p,uint32_t n){memcpy(p,&n,4);}
static void putf(void*p,float n){memcpy(p,&n,4);}
static float value(const void*p){float n;memcpy(&n,p,4);return n;}
static unsigned char* load(gh_disc*d,const char*name,size_t*n){size_t i;gh_disc_entry e;C(!gh_disc_find(d,name,&i)&&!gh_disc_entry_at(d,i,&e));*n=(size_t)e.size;unsigned char*p=malloc(*n);C(p&&!gh_disc_read(d,i,0,p,*n));return p;}
int main(int argc,char**argv){
 C(argc==2);gh_disc*d;C(!gh_disc_open(argv[1],&d));size_t n;unsigned char*room=load(d,"st1/r100.dat",&n);size_t plsize;unsigned char*pl=load(d,"pl/pl00.dat",&plsize);gh_disc_close(d);
 gh_bundle bundle;C(!gh_bundle_open(room,n,&bundle));gh_heap*heap;C(!gh_heap_create(32*1024*1024,&heap));
 gh_game_world_light*s=calloc(1,sizeof(*s));C(s&&!gh_game_world_light_construct_002d6d80(s)&&!gh_game_world_light_initialize_with_heap_002d6f40(s,&bundle,heap));
 gh_light_player_parts no_parts={.owner=&bundle};C(!gh_game_world_light_select_002d8408(s,0,&no_parts));
 printf("actual R100 selected count=%u\n",word(s->scalar+0x78));
 /* Real PL00 model initialization with explicit numeric pose/camera inputs.
  * This is not an actual running player pose or a rendered-frame proof. */
 unsigned char bytes[GH_TITLE_PLAYER_BYTES]={0};gh_title_player_view v;C(!gh_title_player_construct_00107e40(bytes,sizeof(bytes),&v));
 gh_bundle pb;gh_bundle_entry md,tm;C(!gh_bundle_open(pl,plsize,&pb)&&!gh_bundle_find(&pb,"MD",0,&md)&&!gh_bundle_find(&pb,"TM3",0,&tm));
 gh_room_pools pools={0};C(!gh_game_room_pools_initialize_002a5d80(&pools,&bundle,heap));gh_boot_work work={0};gh_actor_model m={0};uint32_t fog=0;int original;C(!gh_actor_model_bind_packet_inputs(&m,heap,&fog)&&!gh_actor_model_initialize_00149738(&m,&v,&work,&pools,&md,&tm,0,0,GH_STATIC_BINARY32_REFERENCE,&original)&&original);gh_player_shared_vector pivot={.initialized=1};gh_game_pl_camera camera={.constructed=1};gh_actor_light_bindings b={.pivot_owner=&pivot,.camera=&camera};uint32_t pc;
 /* Actual part owner, explicit finite numeric matrix: 4D inputW must
  * multiply translation in30A2B0, unlike a fixedW1 point helper. */
 const gh_actor_part*attached_part;C(!gh_actor_model_part_at(&m,0,&attached_part));uint8_t saved_matrix[64];memcpy(saved_matrix,attached_part->bytes+0x80,64);
 float fixture_matrix[16]={2,0,0,0,0,3,0,0,0,0,4,0,10,20,30,1};memcpy(attached_part->bytes+0x80,fixture_matrix,64);
 const int attachment_modes[4]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};
 for(unsigned mode=0;mode<4;mode++){
  C(!fesetround(attachment_modes[mode]));
  for(unsigned w=0;w<3;w++){
   float point[4]={1,2,3,(float)w},out[4],scratch[16];memset(scratch,0x5a,sizeof(scratch));int written=99;
   C(!gh_actor_light_attachment_0014d9a4(&m,&b,0,point,out,scratch,&written,GH_STATIC_BINARY32_REFERENCE,&pc)&&written==0&&fegetround()==attachment_modes[mode]);
   C(out[0]==2+10*w&&out[1]==6+20*w&&out[2]==12+30*w&&out[3]==w);for(unsigned z=0;z<sizeof(scratch);z++)C(((uint8_t*)scratch)[z]==0x5a);
   C(!gh_actor_light_attachment_0014d9a4(&m,&b,1,point,out,scratch,&written,GH_STATIC_BINARY32_REFERENCE,&pc)&&written==1&&out[0]==11&&out[1]==22&&out[2]==33&&out[3]==w);C(scratch[0]==1&&scratch[5]==1&&scratch[10]==1&&scratch[15]==1);
  }
  putf(camera.fields+0x224,.25f);float point[4]={1,2,3,2},out[4],scratch[16];int written;
  C(!gh_actor_light_attachment_0014d9a4(&m,&b,1,point,out,scratch,&written,GH_STATIC_BINARY32_REFERENCE,&pc));
  C(fabsl(out[0]-(10+cosl(.25L)+3*sinl(.25L)))<3e-6L&&fabsl(out[2]-(30-sinl(.25L)+3*cosl(.25L)))<3e-6L&&out[1]==22&&out[3]==2);
  putf(camera.fields+0x224,0);
 }
 C(!fesetround(FE_TONEAREST));
 {float point[4]={1,2,3,2},out[4]={99,99,99,99},scratch[16],old[16];memset(scratch,0x5a,64);memcpy(old,scratch,64);int written=7;gh_actor_light_bindings missing={.pivot_owner=&pivot};C(gh_actor_light_attachment_0014d9a4(&m,&missing,1,point,out,scratch,&written,GH_STATIC_BINARY32_REFERENCE,&pc)==GH_ACTOR_MODEL_ARGUMENT&&pc==0x147728&&out[0]==99&&written==7&&!memcmp(old,scratch,64));
  uint8_t parts=bytes[0x2b4];bytes[0x2b4]=0;putf(pivot.bytes,7);putf(pivot.bytes+4,8);putf(pivot.bytes+8,9);C(!gh_actor_light_attachment_0014d9a4(&m,&b,1,point,out,scratch,&written,GH_STATIC_BINARY32_REFERENCE,&pc)&&out[0]==8&&out[1]==10&&out[2]==12&&out[3]==2);C(gh_actor_light_attachment_0014d9a4(&m,&b,0,point,out,scratch,&written,GH_STATIC_BINARY32_REFERENCE,&pc)==GH_ACTOR_MODEL_UNSUPPORTED&&pc==0x30a2b0);bytes[0x2b4]=parts;memset(pivot.bytes,0,12);
 }
 memcpy(attached_part->bytes+0x80,saved_matrix,64);
 gh_game_world_light*saved=malloc(sizeof(*s));C(saved);memcpy(saved,s,sizeof(*s));
 printf("actual model flags=%08x/%08x area=%u mask=%08x aliasKind=%u\n",word(bytes+0x250),word(bytes+0x254),bytes[0x2cc],word(bytes+0x2d0),(unsigned)s->scalar[0x10]|(unsigned)s->scalar[0x11]<<8);
 gh_actor_model_status actual=gh_actor_model_light_0014d828(&m,s,&b,GH_STATIC_BINARY32_REFERENCE,&pc);
 printf("actual R100 producer status=%d frontier=%08x\n",actual,pc);
 C(actual==GH_ACTOR_MODEL_OK&&!pc);memcpy(s,saved,sizeof(*s));
 put(bytes+0x2d0,0xffffffffu);put(bytes+0x254,0x20000000);
 actual=gh_actor_model_light_0014d828(&m,s,&b,GH_STATIC_BINARY32_REFERENCE,&pc);
 printf("actual R100 positive-mask status=%d frontier=%08x selectors=%u,%u,%u\n",actual,pc,bytes[0x2cd],bytes[0x2ce],bytes[0x2cf]);
 C(actual==GH_ACTOR_MODEL_OK&&bytes[0x2cd]==0&&bytes[0x2ce]==6&&bytes[0x2cf]==5);
 for(unsigned k=0;k<16;k++)C(value(s->scalar+0x16240+k*4)==(k%5==0?1:0));
 for(unsigned mode=0;mode<4;mode++){
  const int fm[4]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};C(!fesetround(fm[mode]));memcpy(s,saved,sizeof(*s));C(!gh_actor_model_light_0014d828(&m,s,&b,GH_STATIC_BINARY32_REFERENCE,&pc)&&fegetround()==fm[mode]&&bytes[0x2cd]==0&&bytes[0x2ce]==6&&bytes[0x2cf]==5);
 }
 C(!fesetround(FE_TONEAREST));memcpy(s,saved,sizeof(*s));
 gh_actor_light_bindings no_camera={.pivot_owner=&pivot};uint8_t previous_selectors[3];memcpy(previous_selectors,bytes+0x2cd,3);C(gh_actor_model_light_0014d828(&m,s,&no_camera,GH_STATIC_BINARY32_REFERENCE,&pc)==GH_ACTOR_MODEL_ARGUMENT&&pc==0x147728&&!memcmp(s,saved,sizeof(*s))&&!memcmp(previous_selectors,bytes+0x2cd,3));
 /* Actual unmodified kind1 record2 isolated in the SAME canonical owner
  * for a numeric consumer test. This is not the live13-record selection. */
 for(unsigned mode=0;mode<4;mode++){
  const int fm[4]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};C(!fesetround(fm[mode]));memcpy(s,saved,sizeof(*s));memcpy(s->scalar+0x80,saved->scalar+0x80+2*0x70,0x70);put(s->scalar+0x78,1);put(bytes+0x2d0,0xc0000000);bytes[0x2cc]=3;
  actual=gh_actor_model_light_0014d828(&m,s,&b,GH_STATIC_BINARY32_REFERENCE,&pc);printf("actual isolated record2 status=%d pc=%08x\n",actual,pc);C(actual==GH_ACTOR_MODEL_OK&&bytes[0x2cd]==0&&fegetround()==fm[mode]);
  const uint8_t*r=s->scalar+0x80;long double q=0;for(unsigned k=0;k<3;k++){long double delta=(long double)value(v.alias_f0+k*4)-value(r+0x10+k*4);q+=delta*delta;}
  long double len=sqrtl(q),factor=1/((long double)value(r+0x40)*q+(long double)value(r+0x44)*len)+value(r+0x48);if(factor<0)factor=0;if(factor>1)factor=1;
  for(unsigned k=0;k<3;k++){long double delta=(long double)value(v.alias_f0+k*4)-value(r+0x10+k*4);C(fabsl(value(s->scalar+0x161a0+k*4)-(len>0?delta/len:0))<2e-6L);C(fabsl(value(s->scalar+0x161d0+k*4)-(value(r+0x30+k*4)*factor))<2e-5L);}
 }
 C(!fesetround(FE_TONEAREST));bytes[0x2cc]=0;
 memcpy(s,saved,sizeof(*s));
 const int modes[4]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};
 for(unsigned mode=0;mode<4;mode++){
  memcpy(s,saved,sizeof(*s));put(s->scalar+0x78,0);put(bytes+0x254,0);memset(bytes+0x2cd,0xaa,3);
  unsigned char olddir[48];memcpy(olddir,s->scalar+0x161a0,48);put(s->scalar+0x1622c,0x7fc01234);
  C(!fesetround(modes[mode]));C(!gh_actor_model_light_0014d828(&m,s,NULL,GH_STATIC_BINARY32_REFERENCE,&pc)&&!pc&&fegetround()==modes[mode]);
  C(!memcmp(olddir,s->scalar+0x161a0,48)&&bytes[0x2cd]==255&&bytes[0x2ce]==255&&bytes[0x2cf]==255);
  for(unsigned i=0;i<3;i++)for(unsigned k=0;k<3;k++)C(!word(s->scalar+0x161d0+i*16+k*4));
  C(word(s->scalar+0x16220)==0xbe23d70a&&word(s->scalar+0x16224)==0xbf733333&&word(s->scalar+0x16228)==0xbe851eb8);
  C(word(s->scalar+0x1622c)==0x7fc01234&&word(s->scalar+0x16230)==0x3f000000&&word(s->scalar+0x1623c)==0x3f800000);
  /* Signed -1 fallback reads owner+10: kind0 uses strength0, preserving
   * actual direction while producing zero RGB, not a synthetic null record. */
  memset(s->scalar+0x10,0,0x70);putf(s->scalar+0x30,2);putf(s->scalar+0x34,3);putf(s->scalar+0x38,4);
  putf(s->scalar+0x40,8);putf(s->scalar+0x44,9);putf(s->scalar+0x48,10);put(bytes+0x254,0x20000000);
  C(!gh_actor_model_light_0014d828(&m,s,&b,GH_STATIC_BINARY32_REFERENCE,&pc));C(value(s->scalar+0x16200)==2&&value(s->scalar+0x16204)==3&&value(s->scalar+0x16208)==4&&!word(s->scalar+0x16210));
 }
 C(!fesetround(FE_TONEAREST));memcpy(s,saved,sizeof(*s));put(bytes+0x254,0);put(bytes+0x2d0,1);put(s->scalar+0x78,4);
 for(unsigned i=0;i<4;i++){uint8_t*r=s->scalar+0x80+i*0x70;memset(r,0,0x70);r[0]=2;put(r+0x58,1);for(unsigned k=0;k<3;k++){putf(r+0x20+k*4,(float)(i*3+k));putf(r+0x30+k*4,(float)(10+i*3+k));}}
 putf(pivot.bytes,1);putf(pivot.bytes+4,2);putf(pivot.bytes+8,3);C(!gh_actor_model_light_0014d828(&m,s,&b,GH_STATIC_BINARY32_REFERENCE,&pc));
 C(bytes[0x2cd]==0&&bytes[0x2ce]==2&&bytes[0x2cf]==1); /* original least-slot scan, no sorting */
 C(value(s->scalar+0x161a0)==0&&value(s->scalar+0x161b0)==6&&value(s->scalar+0x161c0)==3&&value(s->scalar+0x161d0)==9);
 /* 2D9360 uses same LAR records, first matching ID, and defaults1 for ID0. */
 uint8_t table[16+2*0x50]={0};put(table,2);table[0x12]=7;table[0x62]=7;putf(table+0x14,.25f);putf(table+0x64,.75f);
 gh_bundle_entry oldtable=s->table;s->table=(gh_bundle_entry){.bytes=table,.size=sizeof(table)};bytes[0x2cc]=7;
 for(unsigned i=0;i<4;i++){uint8_t*r=s->scalar+0x80+i*0x70;put(r+0x4c,7);put(r+0x58,0x2000001);}
 C(!gh_actor_model_light_0014d828(&m,s,&b,GH_STATIC_BINARY32_REFERENCE,&pc)&&value(s->scalar+0x161d0)==2.25f);
 s->table.size=15;memcpy(saved,s,sizeof(*s));C(gh_actor_model_light_0014d828(&m,s,&b,GH_STATIC_BINARY32_REFERENCE,&pc)==GH_ACTOR_MODEL_BOUNDS&&pc==0x2d9360&&!memcmp(saved,s,sizeof(*s)));
 s->table=oldtable;bytes[0x2cc]=0;for(unsigned i=0;i<4;i++)put(s->scalar+0x80+i*0x70+0x4c,0);
 memcpy(saved,s,sizeof(*s));uint8_t oldselectors[3];memcpy(oldselectors,bytes+0x2cd,3);s->scalar[0x80]=1;
 gh_game_world_light*before=malloc(sizeof(*s));C(before);memcpy(before,s,sizeof(*s));C(gh_actor_model_light_0014d828(&m,s,&b,GH_STATIC_BINARY32_REFERENCE,&pc)==GH_ACTOR_MODEL_UNSUPPORTED&&pc==0x14daf4&&!memcmp(s,before,sizeof(*s))&&!memcmp(bytes+0x2cd,oldselectors,3));
 /* Controlled point light proves quadratic+linear denominator, then
  * constant OUTSIDE reciprocal, full XYZ normalization and part0 source. */
 memcpy(s,saved,sizeof(*s));put(s->scalar+0x78,1);uint8_t*pr=s->scalar+0x80;memset(pr,0,0x70);pr[0]=1;put(pr+0x58,1);put(bytes+0x2d0,1);put(bytes+0x254,0);bytes[0x2cc]=0;memset(pivot.bytes,0,12);
 putf(v.alias_f0,3);putf(v.alias_f0+4,4);putf(v.alias_f0+8,0);putf(pr+0x40,2);putf(pr+0x44,3);putf(pr+0x48,.25f);for(unsigned k=0;k<3;k++)putf(pr+0x30+k*4,(float)(k+1));
 C(!gh_actor_model_light_0014d828(&m,s,&b,GH_STATIC_BINARY32_REFERENCE,&pc));long double factor=1.0L/65+.25L;
 C(fabsl(value(s->scalar+0x161a0)-.6L)<2e-7L&&fabsl(value(s->scalar+0x161a4)-.8L)<2e-7L);
 for(unsigned k=0;k<3;k++)C(fabsl(value(s->scalar+0x161d0+k*4)-(k+1)*factor)<2e-7L);
 const gh_actor_part*part;C(!gh_actor_model_part_at(&m,0,&part));putf(part->bytes+0xb0,0);putf(part->bytes+0xb4,0);putf(part->bytes+0xb8,2);put(pr+0x58,0x1000001);
 C(!gh_actor_model_light_0014d828(&m,s,&b,GH_STATIC_BINARY32_REFERENCE,&pc)&&value(s->scalar+0x161a0)==0&&value(s->scalar+0x161a4)==0&&value(s->scalar+0x161a8)==1);
 C(fabsl(value(s->scalar+0x161d0)-factor)<2e-7L); /* selection uses actor, not part, position */
 pr[0]=0;put(pr+0x58,1);putf(pr+0x20,-3);putf(pr+0x24,7);putf(pr+0x28,9);C(!gh_actor_model_light_0014d828(&m,s,&b,GH_STATIC_BINARY32_REFERENCE,&pc)&&value(s->scalar+0x161a0)==-3&&value(s->scalar+0x161a4)==7);
 /* Root bit2 genuinely returns before invalid records or bindings. */
 put(bytes+0x250,2);put(s->scalar+0x78,999);C(!gh_actor_model_light_0014d828(&m,s,NULL,GH_STATIC_BINARY32_REFERENCE,&pc));
 C(!gh_actor_model_release(&m)&&!gh_game_room_pools_release(&pools)&&!gh_game_world_light_release_presets(s)&&!gh_heap_destroy(heap));free(pl);free(before);free(saved);free(s);free(room);
 puts("PASS actual R100 LI3 input, signed alias fallback, constant lights, original selector order, XYZ/W boundaries, FEnv and atomic failures");return 0;
}
