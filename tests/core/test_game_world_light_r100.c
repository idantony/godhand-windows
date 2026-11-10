#include "game_world_light.h"
#include "disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t w(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
static void put(uint8_t*p,uint32_t n){memcpy(p,&n,4);}
int main(int argc,char**argv){gh_disc*d;C(argc==2&&!gh_disc_open(argv[1],&d));size_t index;gh_disc_entry e;
 C(!gh_disc_find(d,"st1/r100.dat",&index)&&!gh_disc_entry_at(d,index,&e));uint8_t*bytes=malloc((size_t)e.size);C(bytes&&!gh_disc_read(d,index,0,bytes,(size_t)e.size));gh_bundle room;gh_bundle_entry li;C(!gh_bundle_open(bytes,(size_t)e.size,&room)&&!gh_bundle_find(&room,"LI3",0,&li)&&li.size==5952);
 gh_game_world_light*s=calloc(1,sizeof(*s));gh_heap*heap;C(s&&!gh_heap_create(10000,&heap)&&!gh_game_world_light_construct_002d6d80(s));
 C(!gh_game_world_light_initialize_with_heap_002d6f40(s,&room,heap)&&s->ready&&gh_heap_used(heap)==2912&&s->record_heap==heap&&!w(s->scalar+0x78));
 size_t off=4;for(unsigned i=0;i<8;i++){
  size_t count;const uint8_t*records;C(!gh_game_world_light_preset_records(s,i,&records,&count)&&count==(i<2?13:0));
  C(!memcmp(s->scalar+0x15d00+i*0x80,li.bytes+off,0x70)&&!w(s->scalar+0x15d74+i*0x80)&&s->slots[i].bytes==li.bytes+off+0x70);
  if(count)C(records!=li.bytes+off+0x178&&!((uintptr_t)records&15)&&s->preset_records[i].owner==heap&&!memcmp(records,li.bytes+off+0x178,count*0x70));
  off+=0x178+count*0x70;
 }
 uint8_t*snapshot=malloc(sizeof(s->scalar));C(snapshot);memcpy(snapshot,s->scalar,sizeof(s->scalar));
 C(gh_game_world_light_select_002d8408(s,0,NULL)==GH_LIGHT_UNSUPPORTED&&s->frontier==0x2db6b0&&!memcmp(snapshot,s->scalar,sizeof(s->scalar)));
 /* Attachment fixture exercises real original boundary rules; it is NOT a
  * fabricated runtime player. Production must borrow its actual native parts. */
 int owner,part0,part1,extra;const void*parts[2]={&part0,&part1};gh_light_player_parts player={&owner,parts,2};
 uint8_t*owned=s->preset_records[0].pointer;put(owned+0x4c,0);put(owned+0x70+0x4c,1);put(owned+3*0x70+0x4c,3);put(owned+4*0x70+0x4c,2);put(owned+7*0x70+0x4c,0xffffffff);
 put(s->scalar+0x3c,0x7fc12345);C(!gh_game_world_light_select_002d8408(s,0,&player)&&w(s->scalar+0x78)==13&&w(s->scalar+0x16288)==0&&w(s->scalar+0x3c)==0x7fc12345);
 C(!memcmp(s->scalar+0x30,li.bytes+4,12)&&!memcmp(s->scalar+0x40,li.bytes+4+0x10,12));
 C(s->native_light_owners[0]==&part0&&s->native_light_owners[1]==&part1&&s->native_light_owners[3]==&part0&&!s->native_light_owners[4]&&s->native_light_owners[7]==&part0);
 for(unsigned i=0;i<13;i++)C(!w(s->scalar+0x80+i*0x70+0x54)); /* no hostpointer truncation */
 /* Original keeps live nonzero-ID lights, compacts them before newpreset. */
 uint8_t*old=s->scalar+0x80+2*0x70;old[2]=9;old[3]=0;s->native_light_owners[2]=&extra;uint8_t preserved[0x70];memcpy(preserved,old,sizeof(preserved));
 C(!gh_game_world_light_select_002d8408(s,1,&player)&&w(s->scalar+0x78)==14&&s->native_light_owners[0]==&extra&&!memcmp(s->scalar+0x80,preserved,sizeof(preserved)));
 C(!gh_game_world_light_select_002d8408(s,1,&player)&&w(s->scalar+0x78)==14); /* no duplicatepreservedlight */
 C(!gh_game_world_light_select_002d8408(s,2,NULL)&&w(s->scalar+0x78)==14); /* nullpresetarray skips list reset, original quirk */
 memcpy(snapshot,s->scalar,sizeof(s->scalar));C(!gh_game_world_light_release_presets(s)&&!gh_heap_used(heap)&&!memcmp(snapshot,s->scalar,sizeof(s->scalar)));
 gh_heap*small;C(!gh_heap_create(1500,&small));C(gh_game_world_light_initialize_with_heap_002d6f40(s,&room,small)==GH_LIGHT_MEMORY&&!s->ready&&s->frontier==0x30ee08&&gh_heap_used(small)==1456);
 C(!gh_game_world_light_release_presets(s)&&!gh_heap_used(small)&&!gh_heap_destroy(small));
 C(!gh_game_world_light_initialize_with_heap_002d6f40(s,&room,heap)&&gh_heap_used(heap)==2912);C(!gh_game_world_light_release_presets(s)&&!gh_heap_destroy(heap));
 free(snapshot);free(s);free(bytes);gh_disc_close(d);puts("PASS actualR100 LI33 13+13 realheap records, variablecursor/copies, nativeattachment/compaction/nullpreset rules, allocationfailure/reload/cleanup");return 0;}
