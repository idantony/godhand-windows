#include "game_objects.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t u32(const uint8_t *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static const uint8_t *at(const uint8_t *b,size_t size,uint32_t a,size_t n){
 unsigned count=b[44]|(unsigned)b[45]<<8;uint32_t ph=u32(b+28);
 for(unsigned i=0;i<count;i++){CHECK((uint64_t)ph+i*32+32<=size);const uint8_t *p=b+ph+i*32;
  uint32_t v=u32(p+8),sz=u32(p+16),off=u32(p+4);if(u32(p)==1&&a>=v&&a-v<=sz&&n<=sz-(a-v)){CHECK((uint64_t)off+a-v+n<=size);return b+off+a-v;}}
 CHECK(0);return NULL;
}
int main(int argc,char **argv){
 gh_stage_slots slots={0},room={0};gh_game_objects *m,*rm;gh_game_assets assets={0};gh_boot_work work={0};gh_object_view view;
 gh_disc *disc;CHECK(argc==3 && gh_disc_open(argv[1],&disc)==GH_DISC_OK);
 FILE *f=fopen(argv[2],"rb");CHECK(f&&!fseek(f,0,SEEK_END));long size=ftell(f);CHECK(size>0);rewind(f);
 uint8_t *elf=malloc((size_t)size);CHECK(elf&&fread(elf,1,(size_t)size,f)==(size_t)size);fclose(f);
 const uint8_t *table=at(elf,(size_t)size,u32(at(elf,(size_t)size,0x3c3cc4,4)),42*8);
 CHECK(gh_game_objects_create(&slots,0,&m)==GH_OBJECT_OK && slots.count==80 && slots.configured);
 CHECK(gh_game_objects_create(&room,1,&rm)==GH_OBJECT_OK && room.count==32);
 CHECK(gh_stage_slots_reset(&slots,NULL)==GH_STAGE_OK && gh_stage_slots_reset(&room,NULL)==GH_STAGE_OK);
 assets.work=&work;CHECK(gh_game_objects_register_startup(m,&assets)==GH_OBJECT_NOT_READY && !(work.flags&0x80));
 /* Read the real43 payloads, using the actual ELF's ID-to-source-slot table. */
 for(unsigned i=0;i<43;i++){
  uint32_t id=i<2?0x100+i:u32(table+(i-2)*8);unsigned slot=i<2?i:u32(table+(i-2)*8+4);char path[12];size_t index;gh_disc_entry e;
  CHECK(slot<43 && gh_game_object_path(id,path)==GH_GAME_OK);
  CHECK(gh_disc_find(disc,path,&index)==GH_DISC_OK&&gh_disc_entry_at(disc,index,&e)==GH_DISC_OK);
  assets.objects[slot].pointer=malloc((size_t)e.size);assets.object_sizes[slot]=(size_t)e.size;assets.object_file_sizes[slot]=(size_t)e.size;CHECK(assets.objects[slot].pointer);
  CHECK(gh_disc_read(disc,index,0,assets.objects[slot].pointer,(size_t)e.size)==GH_DISC_OK);
 }
 CHECK(u32(table+41*8)==0xffff);work.flags=0x400;
 void *last=assets.objects[42].pointer;assets.objects[42].pointer=NULL;
 CHECK(gh_game_objects_register_startup(m,&assets)==GH_OBJECT_ARGUMENT && !(work.flags&0x80));
 CHECK(gh_game_objects_find(m,0x100,&view)==GH_OBJECT_OK && view.bytes==assets.objects[0].pointer);
 CHECK(gh_game_objects_find(m,0x23f,&view)==GH_OBJECT_MISSING);assets.objects[42].pointer=last;
 CHECK(gh_game_objects_register_startup(m,&assets)==GH_OBJECT_OK && (work.flags&0x80));
 for(unsigned i=0;i<43;i++){
  uint32_t id=i<2?0x100+i:u32(table+(i-2)*8);unsigned slot=i<2?i:u32(table+(i-2)*8+4);
  CHECK(gh_game_objects_find(m,id,&view)==GH_OBJECT_OK && view.bytes==assets.objects[slot].pointer && view.size==assets.object_sizes[slot]&&view.file_extent_known&&view.file_extent==assets.object_file_sizes[slot]);
  CHECK(view.state==2 && view.use_count==-100 && view.kind==id && view.file_kind==(id|0x40000000));
  CHECK(u32(slots.records+i*0x5c+0x20)==id && u32(slots.records+i*0x5c+0x10)==i+1);
 }
 /* Actuallate1FE748 defaultstate2 mustnotpretendmodelinitialization.
  * All43actual pinnedpayload/holderbytes androomemptyrecords stayunchanged. */
 {uint8_t snapshot[80*0x5c],room_snapshot[32*0x5c];uint32_t frontier;
  memcpy(snapshot,slots.records,sizeof(snapshot));memcpy(room_snapshot,room.records,sizeof(room_snapshot));
  CHECK(gh_game_objects_update_001fe748(m,&frontier)==GH_OBJECT_OK&&!frontier&&!memcmp(snapshot,slots.records,sizeof(snapshot)));
  CHECK(gh_game_objects_update_001fe748(rm,&frontier)==GH_OBJECT_OK&&!frontier&&!memcmp(room_snapshot,room.records,sizeof(room_snapshot)));
 }
 /* Replacement frees the old kind then uses the actual first free slot. */
 int original=0;CHECK(gh_game_objects_add_pinned_001fece0(m,0x100,assets.objects[1].pointer,assets.object_sizes[1],&original)==GH_OBJECT_OK&&original==1);
 CHECK(gh_game_objects_find(m,0x100,&view)==GH_OBJECT_OK && view.bytes==assets.objects[1].pointer);
 for(unsigned i=0;i<32;i++)CHECK(gh_game_objects_add_pinned_001fece0(rm,0x500+i,assets.objects[0].pointer,assets.object_sizes[0],&original)==GH_OBJECT_OK);
 CHECK(gh_game_objects_find(rm,0x500,&view)==GH_OBJECT_OK && view.file_kind==UINT32_MAX);
 CHECK(gh_game_objects_add_pinned_001fece0(rm,0x100,assets.objects[0].pointer,assets.object_sizes[0],&original)==GH_OBJECT_CAPACITY && original==0);
 CHECK(gh_game_objects_add_pinned_001fece0(rm,0x500,NULL,1,&original)==GH_OBJECT_ARGUMENT);
 CHECK(gh_stage_slots_reset(&room,NULL)==GH_STAGE_OK);room.records[8]=4;
 CHECK(gh_game_objects_add_pinned_001fece0(rm,0x100,assets.objects[0].pointer,assets.object_sizes[0],&original)==GH_OBJECT_OK);
 CHECK(u32(room.records)==0 && u32(room.records+0x5c+0x20)==0x100);
 /* Canonical reset invalidates the pointer handles without stale-view alias. */
 CHECK(gh_stage_slots_reset(&slots,NULL)==GH_STAGE_OK && gh_game_objects_find(m,0x100,&view)==GH_OBJECT_MISSING);
 gh_game_objects_destroy(m);gh_game_objects_destroy(rm);CHECK(!slots.configured && !room.configured);
 for(unsigned i=0;i<43;i++)free(assets.objects[i].pointer);free(elf);gh_disc_close(disc);
 puts("PASS actual43 asset/ELF source associations, original80/32 slots, pinned replacement/full/reset handles");return 0;
}
