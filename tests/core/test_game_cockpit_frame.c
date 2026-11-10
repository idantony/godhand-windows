#include "../../src/core/game_cockpit.h"
#include "../../src/platform/disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fenv.h>
#include "../../src/assets/tim2.h"
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL%d %s\n",__LINE__,#x);exit(1);}}while(0)
typedef struct {gh_id_draw_job jobs[512];size_t count;int calls;int ids[8];gh_cockpit_fan fan;unsigned fan_count;} queue;
static gh_id_error enqueue(void*p,gh_id_base*b,uint32_t*f){queue*q=p;gh_id_frontier frontier={0};size_t n=0;if(q->calls>=8)return GH_ID_CAPACITY;
 gh_id_error e=gh_id_jobs_collect_visible(b,q->jobs+q->count,512-q->count,&n,&frontier);if(e){*f=frontier.address;return e;}q->count+=n;q->ids[q->calls++]=b->id;return GH_ID_OK;
}
static gh_id_error enqueue_fan(void*p,const gh_cockpit_fan*fan,uint32_t*f){queue*q=p;(void)f;if(q->fan_count)return GH_ID_CAPACITY;q->fan=*fan;q->fan_count++;return GH_ID_OK;}
static void put(uint8_t*p,uint32_t n){memcpy(p,&n,4);}
static uint32_t word(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
int main(int argc,char**argv){
 C(argc==2);gh_disc*d;C(!gh_disc_open(argv[1],&d));size_t i;gh_disc_entry de;C(!gh_disc_find(d,"etc/core_usa.bin",&i)&&!gh_disc_entry_at(d,i,&de));uint8_t*l=malloc((size_t)de.size);C(l&&!gh_disc_read(d,i,0,l,(size_t)de.size));
 size_t es=(size_t)gh_disc_boot_elf_size(d);uint8_t*elf=malloc(es);C(elf&&!gh_disc_read_boot_elf(d,0,elf,es));gh_disc_close(d);
 gh_bundle language;gh_bundle_entry idd;C(!gh_bundle_open(l,(size_t)de.size,&language)&&!gh_id_find_language_data(&language,&idd));gh_id_state*m=calloc(1,sizeof(*m));C(m&&!gh_id_system_init_002acc70(m,&idd));gh_id_runtime*r;C(!gh_id_runtime_create(m,1,&r)&&!gh_id_runtime_set_path_policy(r,GH_ID_PATH_REFERENCE_NEAREST));
 gh_id_texture fan_texture={0};C(gh_id_texture_open(m,1,0x21,&fan_texture)==GH_ID_UNSUPPORTED);
 {const gh_bundle_entry*t=&m->pair[1].texture_data;C(t->size>136);uint32_t o=word(t->bytes+132);C(o<t->size);gh_tim2_picture pic={0};C(!gh_tim2_first(t->bytes+o,t->size-o,&pic)&&pic.width==16&&pic.height==16&&pic.image_type==4&&pic.clut_type==3&&pic.colors==16&&pic.tex0==UINT64_C(0x111408000));}
 uint8_t save[GH_CORE_SAVE_BYTES];gh_core_save_view sv={save,sizeof(save)};gh_boot_work work={0};C(!gh_core_save_system_init(sv)&&!gh_core_save_game_init(sv,&work));gh_game_mt random;gh_game_mt_seed_0030f2b0(&random,1);gh_game_cockpit*s;C(!gh_game_cockpit_create(r,m,&random,&s)&&!gh_game_cockpit_initialize_00140c88(s,sv));uint8_t heat[0x18]={0};C(!gh_game_cockpit_room_reset_00140de0(s,sv,&work,heat));C(!gh_game_cockpit_bind_frame_elf(s,elf,es));free(elf);
 size_t n;const uint8_t*raw=gh_game_cockpit_fields(s,&n);C(n==0x23c0);gh_id_base*b17,*b14;C(!gh_game_cockpit_base(s,0x1520,&b17)&&!gh_game_cockpit_base(s,0xbe0,&b14));uint16_t f17=b17->frame,f14=b14->frame;C(gh_game_cockpit_move_00140e58(s,&work,NULL)==GH_ID_UNSUPPORTED&&gh_game_cockpit_frontier(s)==0x13c950&&b17->frame==f17);put((uint8_t*)&work+0x5d8,0xf00c6000);
 gh_id_error e=gh_game_cockpit_move_00140e58(s,&work,NULL);if(e)fprintf(stderr,"move %d %08X\n",e,gh_game_cockpit_frontier(s));C(!e&&b17->frame!=f17&&b14->frame==f14);C(!raw[0x1ca0]&&!raw[0x1db4]);
 queue q={0};gh_cockpit_frame_callbacks cb={.context=&q,.trans=enqueue};C(fesetround(FE_DOWNWARD)==0);
 C(gh_game_cockpit_trans_001410a8(s,&work,NULL)==GH_ID_UNSUPPORTED&&gh_game_cockpit_frontier(s)==0x2aac28);
 C(fegetround()==FE_DOWNWARD&&raw[0x6e0+0x5dc]==1&&word(raw+0x6e0+0x5d8)==0x00300330&&word(raw+0x6e0+0x5d4)==0);
 C(gh_game_cockpit_trans_001410a8(s,&work,&cb)==GH_ID_ARGUMENT&&gh_game_cockpit_frontier(s)==0x1fa080);
 C(q.calls==3&&q.ids[0]==13&&q.ids[1]==15&&q.ids[2]==1&&raw[0x6e0+0x5dc]==1); /* samefailedframe not replayed */
 size_t jobs=q.count;cb.save=sv;C(gh_game_cockpit_trans_001410a8(s,&work,&cb)==GH_ID_UNSUPPORTED&&gh_game_cockpit_frontier(s)==0x15c410&&q.count==jobs&&q.calls==3);
 cb.fan=enqueue_fan;C(!gh_game_cockpit_trans_001410a8(s,&work,&cb)&&q.fan_count==1&&q.fan.ready);
 static const int sequence[]={13,15,1,0,12,10,19};C(q.calls==7);for(unsigned k=0;k<7;k++)C(q.ids[k]==sequence[k]);jobs=q.count;
 C(fegetround()==FE_DOWNWARD&&fesetround(FE_TONEAREST)==0);
 C(!gh_game_cockpit_destroy(s));gh_id_runtime_destroy(r);free(m);free(l);
 printf("PASS real language39runs/roomreset R020140E58 singleID17Move; all11widget drawbranches realqueue %zu jobs +33vertexfan; missingbindings frontiers, no replay/FEnv\n",jobs);return 0;
}
