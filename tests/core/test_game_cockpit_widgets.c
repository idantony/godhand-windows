#include "game_cockpit.h"
#include "disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fenv.h>
#define C(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static void put(uint8_t*p,uint32_t v){memcpy(p,&v,4);}
static uint32_t word(const uint8_t*p){uint32_t v;memcpy(&v,p,4);return v;}
static gh_id_base*base(gh_game_cockpit*s,unsigned o){gh_id_base*b;C(!gh_game_cockpit_base(s,o,&b));return b;}
static gh_id_entry*cached(gh_game_cockpit*s,unsigned o){gh_id_entry*e;C(!gh_game_cockpit_entry_at(s,o,&e));return e;}
typedef struct {int ids[16];unsigned count,fans;size_t visible;} draw_fixture;
/* Explicit test consumer: validates source ordering and real ID jobs/fan,
 * not a platform renderer acknowledgement or a visible-GPU assertion. */
static gh_id_error trans_fixture(void*ctx,gh_id_base*b,uint32_t*f){draw_fixture*q=ctx;gh_id_draw_job jobs[256];size_t count=0;gh_id_frontier frontier={0};gh_id_error e=gh_id_jobs_collect_visible(b,jobs,256,&count,&frontier);if(e){*f=frontier.address;return e;}if(q->count>=16)return GH_ID_CAPACITY;q->ids[q->count++]=b->id;q->visible+=count;for(size_t i=0;i<count;i++)C(jobs[i].entry&&jobs[i].entry_index<b->entry_count);return GH_ID_OK;}
static gh_id_error fan_fixture(void*ctx,const gh_cockpit_fan*fan,uint32_t*f){draw_fixture*q=ctx;(void)f;C(fan->ready);q->fans++;return GH_ID_OK;}
int main(int argc,char**argv){
 C(argc==2);gh_disc*d;C(!gh_disc_open(argv[1],&d));size_t i;gh_disc_entry de;C(!gh_disc_find(d,"etc/core_usa.bin",&i)&&!gh_disc_entry_at(d,i,&de));size_t ln=(size_t)de.size;uint8_t*language=malloc(ln),*copy=malloc(ln);C(language&&copy&&!gh_disc_read(d,i,0,language,ln));memcpy(copy,language,ln);size_t en=(size_t)gh_disc_boot_elf_size(d);uint8_t*elf=malloc(en);C(elf&&!gh_disc_read_boot_elf(d,0,elf,en));C(!gh_disc_find(d,"st1/r100.dat",&i)&&!gh_disc_entry_at(d,i,&de));size_t rn=(size_t)de.size;uint8_t*room_bytes=malloc(rn);C(room_bytes&&!gh_disc_read(d,i,0,room_bytes,rn));gh_disc_close(d);gh_bundle room;gh_heap*heap;gh_room_pools pools={0};C(!gh_bundle_open(room_bytes,rn,&room)&&!gh_heap_create(16*1024*1024,&heap)&&!gh_game_room_pools_initialize_002a5d80(&pools,&room,heap));
 gh_bundle bundle;gh_bundle_entry idd;C(!gh_bundle_open(language,ln,&bundle)&&!gh_id_find_language_data(&bundle,&idd));const int modes[]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};
 for(unsigned mode=0;mode<4;mode++){
  gh_id_state*m=calloc(1,sizeof(*m));C(m&&!gh_id_system_init_002acc70(m,&idd));gh_id_runtime*r;C(!gh_id_runtime_create(m,1,&r)&&!gh_id_runtime_set_path_policy(r,GH_ID_PATH_REFERENCE_NEAREST));gh_game_init_state*init=malloc(sizeof(*init));C(init);gh_game_init_state_initialize(init);put(init->heat,0x42f00000);gh_game_mt_seed_0030f2b0(&init->random,1);gh_boot_work work={0};uint8_t bytes[GH_CORE_SAVE_BYTES];gh_core_save_view save={bytes,sizeof(bytes)};C(!gh_core_save_system_init(save)&&!gh_core_save_game_init(save,&work));put(bytes+0x84,1);put(bytes+0x68,2);put(bytes+0x6c,0xffffffff);put(bytes+0x20,12345);
  gh_game_cockpit*s;C(!gh_game_cockpit_create(r,m,&init->random,&s)&&!gh_game_cockpit_initialize_00140c88(s,save)&&!gh_game_cockpit_room_reset_00140de0(s,save,&work,init->heat)&&!gh_game_cockpit_bind_frame_elf(s,elf,en));
  /* Focused original-widget test: the SAME registry is explicitly empty;
   * knownNULL player is a fixture publisher, not an R100 gameplay claim. */
  gh_cockpit_gameplay_bindings gameplay={.init=init,.pools=&pools,.player_known=1};C(!gh_game_cockpit_bind_gameplay(s,&gameplay));gh_cockpit_frame_callbacks cb={.save=save};
  size_t n;const uint8_t*read=gh_game_cockpit_fields(s,&n);C(n==0x23c0);/* Test-only request fixture; production exposes no mutable byte span. */uint8_t*fields=(uint8_t*)read;
  put(fields+0x1470+0x94,0xa1234567);fields[0x1cb0+0x104]=0xa5;
  gh_id_base*circle[5],*four[5];unsigned four_offsets[]={0xcc0,0xe00,0xe50,0xd60,0xdb0};uint16_t cf[5],ff[5];int16_t ticks[5][5];for(unsigned j=0;j<5;j++){circle[j]=base(s,0xfa0+j*0x50);cf[j]=circle[j]->frame;circle[j]->entries[0].draw_x=1234567;four[j]=base(s,four_offsets[j]);ff[j]=four[j]->frame;memcpy(ticks[j],four[j]->entries[0].jitter,10);}
  uint16_t seven=base(s,0x1380)->frame,nine=base(s,0x1650)->frame,item=base(s,0x1cb0)->frame;
  uint16_t eight=base(s,0x1470)->frame,fourteen=base(s,0x1520)->frame,keys=base(s,0x1910)->frame,money=base(s,0x1ae0)->frame;
  unsigned layout_offsets[]={0x1470,0x1520,0x1910,0x1ae0};for(unsigned j=0;j<4;j++)base(s,layout_offsets[j])->entries[0].draw_x=1234567;uint8_t before[sizeof(bytes)];memcpy(before,bytes,sizeof(bytes));C(!fesetround(modes[mode])&&!feclearexcept(FE_ALL_EXCEPT)&&!feraiseexcept(FE_DIVBYZERO));
  gh_id_error result=gh_game_cockpit_move_00140e58(s,&work,&cb);if(result)fprintf(stderr,"frontier %08X status%d\n",gh_game_cockpit_frontier(s),result);C(!result);
  C(fegetround()==modes[mode]&&fetestexcept(FE_ALL_EXCEPT)==FE_DIVBYZERO&&!memcmp(before,bytes,sizeof(bytes)));
  for(unsigned j=0;j<5;j++){C(circle[j]->frame==(uint16_t)(cf[j]+(circle[j]->playing?1:0))&&circle[j]->entries[0].draw_x!=1234567&&four[j]->frame==ff[j]);for(unsigned k=0;k<5;k++)C((uint16_t)four[j]->entries[0].jitter[k]==(uint16_t)((uint16_t)ticks[j][k]+(four[j]->playing?1:0)));}
  C(base(s,0x1380)->frame==seven&&base(s,0x1650)->frame==nine&&base(s,0x1cb0)->frame==item&&fields[0x1cb0+0x104]==0xa4);
  C(base(s,0x1470)->frame==(uint16_t)(eight+(base(s,0x1470)->playing?1:0))&&base(s,0x1520)->frame==(uint16_t)(fourteen+(base(s,0x1520)->playing?1:0))&&base(s,0x1910)->frame==(uint16_t)(keys+(base(s,0x1910)->playing?1:0))&&base(s,0x1ae0)->frame==(uint16_t)(money+(base(s,0x1ae0)->playing?1:0)));
  for(unsigned j=0;j<4;j++)C(base(s,layout_offsets[j])->entries[0].draw_x!=1234567);
  for(unsigned j=0;j<4;j++)C(!!(cached(s,0x1470+0x98+j*4)->flags&0x8000000)==!(0xa0000000u&(0x80000000u>>j)));C(word(fields+0x1470+0x94)==0x01234567);
  C(!(cached(s,0x1910+0x9c)->flags&0x8000000)&&cached(s,0x1910+0x9c)->seed==2&&(cached(s,0x1910+0xa0)->flags&0x8000000));C(cached(s,0x1ae0+0x94)->seed==12345);
  draw_fixture draw={0};cb.context=&draw;cb.trans=trans_fixture;cb.fan=fan_fixture;C(!gh_game_cockpit_trans_001410a8(s,&work,&cb));static const int draw_order[]={13,15,1,0,12,20,10,19};C(draw.count==8&&draw.fans==1&&draw.visible);for(unsigned j=0;j<8;j++)C(draw.ids[j]==draw_order[j]);
  /* A real active Seven branch must stop, after earlierwidgets once. A
   * retry retains cursors and cannot increment Circle/Four again. */
  fields[0x1380+0x94]=1;C(gh_game_cockpit_move_00140e58(s,&work,&cb)==GH_ID_UNSUPPORTED&&gh_game_cockpit_frontier(s)==0x13bd40);uint16_t retained=circle[0]->frame;int16_t tick=four[0]->entries[0].jitter[0];C(gh_game_cockpit_move_00140e58(s,&work,&cb)==GH_ID_UNSUPPORTED&&gh_game_cockpit_frontier(s)==0x13bd40&&circle[0]->frame==retained&&four[0]->entries[0].jitter[0]==tick);fields[0x1380+0x94]=0;C(!gh_game_cockpit_move_00140e58(s,&work,&cb)&&circle[0]->frame==retained);
  C(fegetround()==modes[mode]&&fetestexcept(FE_ALL_EXCEPT)==FE_DIVBYZERO);C(!fesetround(FE_TONEAREST)&&!gh_game_cockpit_destroy(s)&&!gh_id_runtime_destroy(r));free(init);free(m);
 }
 C(!memcmp(copy,language,ln));C(!gh_game_room_pools_release(&pools)&&!gh_heap_used(heap)&&!gh_heap_destroy(heap));free(room_bytes);free(copy);free(elf);free(language);puts("PASS actualUSA405ID all11 cold registered HUDMoves; exact fiveMoves/fiveTicks/disabledgates/requestbits/keycounts/money; resumable activeSeven frontier; fourFEnv/immutable assets");return 0;
}
