#include "game_children.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t u32(const uint8_t *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static uint16_t u16(const uint8_t *p){return (uint16_t)((unsigned)p[0]|(unsigned)p[1]<<8);}
static void w32(uint8_t *p,uint32_t v){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(v>>(i*8));}
static const uint8_t *elf_at(const uint8_t *b,size_t size,uint32_t a,size_t n){
 uint32_t ph=u32(b+28);uint16_t step=u16(b+42),count=u16(b+44);
 for(unsigned i=0;i<count;i++){
  CHECK((uint64_t)ph+(uint64_t)i*step+32<=size);const uint8_t *p=b+ph+(size_t)i*step;
  uint32_t v=u32(p+8),len=u32(p+16),off=u32(p+4);
  if(u32(p)==1 && a>=v && a-v<=len && n<=len-(a-v)){
   CHECK((uint64_t)off+a-v+n<=size);return b+off+a-v;
  }
 }CHECK(0);return 0;
}
int main(int argc,char **argv){
 gh_game_children *s;gh_game_child_view child;const uint8_t *bytes;size_t size,elf_size;uint8_t *elf;
 FILE *f;long len;int room_jump=0;
 CHECK(argc==2);f=fopen(argv[1],"rb");CHECK(f && !fseek(f,0,SEEK_END));len=ftell(f);CHECK(len>0);rewind(f);
 elf_size=(size_t)len;elf=malloc(elf_size);CHECK(elf && fread(elf,1,elf_size,f)==elf_size);fclose(f);
 CHECK(gh_game_children_create(&room_jump,&s)==GH_CHILD_OK && gh_game_children_room_jump(s)==&room_jump);
 CHECK(gh_game_children_manager_view(s,&bytes,&size)==GH_CHILD_OK && size>=0xf16);
 CHECK(gh_game_child_at(s,0,&child)==GH_CHILD_BOUNDS);
 CHECK(u32(bytes+0x580+12)==0x3f800000 && u32(bytes+0x830+12)==0x3f800000);
 /* Simulate selected mutable prior-state fields, never the vtableidentity. */
 {static const unsigned offsets[]={0x60,0x310,0x620,0x730,0x880,0x9c0,0xba0,0xc40,0xae0};
  uint8_t *mutable_bytes=(uint8_t *)(void *)bytes;
  for(unsigned i=0;i<9;i++){uint8_t *p=mutable_bytes+offsets[i];memset(p+0x58,0xa5,6);w32(p+0x60,123);p[0x64]=9;p[0x65]=0xa5;w32(p+0x50,42);}
  w32(mutable_bytes+0x60+0x298,77);w32(mutable_bytes+0x310+0x264,88);mutable_bytes[0xba0+0x90]=99;
 }
 CHECK(gh_game_children_initialize_0015fd98(s)==GH_CHILD_OK);
 for(unsigned i=0;i<9;i++){
  const uint8_t *vt;CHECK(gh_game_child_at(s,i,&child)==GH_CHILD_OK && child.initialized);
  vt=elf_at(elf,elf_size,child.vtable_address+8,8);
  CHECK(u16(vt)==0 && u32(vt+4)==child.init_entry);
  CHECK(u32(child.bytes+0x70)==child.vtable_address);
  {gh_id_base *base;CHECK(gh_game_children_id_base(s,child.original_offset,&base)==GH_CHILD_OK && base==child.native_base);
   CHECK(base->vector_a[3]==1.0f && base->vector_b[3]==1.0f && base->f40==0.01f && base->h44==128 && base->mode==6);}

  CHECK(u32(child.bytes+0x50)==0 && u32(child.bytes+0x60)==0xffffffff && child.bytes[0x64]==0 && child.bytes[0x65]==0xa5);
  for(unsigned j=0;j<6;j++)CHECK(child.bytes[0x58+j]==0);
  CHECK(u32(child.bytes+0x2c)==0x3f800000 && u32(child.bytes+0x3c)==0x3f800000);
  CHECK(u16(child.bytes+0x44)==128 && child.bytes[0x46]==6 && u32(child.bytes+0x40)==0x3c23d70a);
  if(child.kind==0)CHECK(u32(child.bytes+0x298)==0);
  if(child.kind==1)CHECK(u32(child.bytes+0x264)==0);
  if(child.kind==8)CHECK(child.bytes[0x90]==0);
 }
 CHECK(gh_game_child_at(s,6,&child)==GH_CHILD_OK && child.kind==8);
 CHECK(gh_game_child_at(s,8,&child)==GH_CHILD_OK && child.kind==6);
 CHECK(u32(bytes+0x48)==0xffffffff && u16(bytes+0x50)==0 && u32(bytes+0xed0)==0 && bytes[0xed4]==0 && bytes[0xf15]==0);
 for(unsigned i=0;i<9;i++)CHECK(u32(bytes+0xf08-i*4)==0xffffffff);
 CHECK(gh_game_children_initialize_0015fd98(s)==GH_CHILD_OK && gh_game_children_room_jump(s)==&room_jump);
 CHECK(gh_game_child_at(s,9,&child)==GH_CHILD_BOUNDS);
 gh_game_children_destroy(s);free(elf);
 puts("PASS original9child vtable mappings from actualELF, construction/common/derived resets and native registry order");return 0;
}
