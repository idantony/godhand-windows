#include "../../src/core/id_jobs.h"
#include "../../src/platform/disc.h"
#include "../../src/assets/tim2.h"
#include "../../src/assets/font_texture.h"
#include <stdio.h>
#include <stdlib.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);}}while(0)
static unsigned bit(unsigned value,unsigned n){return value>>n&1;}
static void radar_texture(gh_id_state *manager,unsigned char *rgba)
{
 gh_id_texture lookup,texture;
 C(!gh_id_texture_open(manager,1,0x36,&lookup));
 C(!gh_id_texture_open_view(&lookup.source,&texture));
 C(texture.source.bytes==lookup.source.bytes&&texture.source.offset==0x8db60&&texture.source.size==34272);
 C(texture.width==128&&texture.height==256&&texture.tex0==UINT64_C(0x200000021d308000));
 C(!gh_id_texture_rgba(&texture,rgba,128*256*4));
 /* Independently invert the original CT32 upload and PSMT8 addresses.
  * Both page bits advance Y at the actual radar's one-page buffer pitch. */
 gh_font_slot source={0};
 source.bound=1;source.mst=texture.source.bytes;source.mst_size=texture.source.size;
 source.table_offset[0]=0x30;source.table_offset[1]=0x8100;
 source.table_count[0]=source.table_count[1]=1;
 gh_font_transfer image,clut;
 C(!gh_font_transfer_at(&source,0,0,&image)&&!gh_font_transfer_at(&source,1,0,&clut));
 C(image.width==64&&image.height==128&&image.buffer_width==1&&clut.width==16&&clut.height==16);
 unsigned seen[128*256]={0},nonzero=0;
 for(unsigned a=0;a<32768;++a){
  unsigned x=bit(a,2)|(bit(a,4)<<1)|((bit(a,5)^bit(a,0)^bit(a,6))<<2)|
      (bit(a,1)<<3)|(bit(a,8)<<4)|(bit(a,10)<<5)|(bit(a,12)<<6);
  unsigned y=bit(a,3)|(bit(a,0)<<1)|(bit(a,6)<<2)|(bit(a,7)<<3)|
      (bit(a,9)<<4)|(bit(a,11)<<5)|((a>>13)<<6);
  unsigned w=a/4;
  unsigned ux=bit(w,0)|(bit(w,2)<<1)|(bit(w,3)<<2)|(bit(w,6)<<3)|(bit(w,8)<<4)|(bit(w,10)<<5);
  unsigned uy=bit(w,1)|(bit(w,4)<<1)|(bit(w,5)<<2)|(bit(w,7)<<3)|(bit(w,9)<<4)|((w>>11)<<5);
  C(x<128&&y<256&&ux<64&&uy<128&&!seen[y*128+x]);
  seen[y*128+x]=1;
  unsigned index=image.pixels[(uy*64+ux)*4+a%4];
  unsigned palette=(index&~24u)|((index&8)<<1)|((index&16)>>1);
  const unsigned char *expected=clut.pixels+palette*4,*pixel=rgba+(y*128+x)*4;
  C(pixel[0]==expected[0]&&pixel[1]==expected[1]&&pixel[2]==expected[2]&&expected[3]<=128);
  C(pixel[3]==(expected[3]==128?255:expected[3]*2));
  nonzero+=pixel[3]!=0;
 }
 C(nonzero>0&&nonzero<128*256);
 C(gh_id_texture_rgba(&texture,rgba,128*256*4-1)==GH_ID_CAPACITY);
 printf("actualRadar128x256 item54 independently mapped 32768 pixels, nontransparent %u\n",nonzero);
}
static uint32_t hud_random(void *context)
{
 (void)context;
 return 0;
}
static void hud_texture(gh_id_state *manager,unsigned char *rgba)
{
 gh_id_base base={0};
 gh_id_runtime *runtime;
 int original,reclaimed;
 C(!gh_id_runtime_create(manager,1,&runtime));
 C(!gh_id_runtime_set_path_policy(runtime,GH_ID_PATH_REFERENCE_NEAREST));
 gh_id_base_clear_002a9df8(&base);
 C(!gh_id_base_initialize_002a9ea0(runtime,&base,1,19,hud_random,NULL,&original)&&original==1);
 C(base.entry_count>2);
 gh_id_entry *entry=base.entries+2;
 C(entry->source_index==2&&entry->kind==0&&entry->b8a==1&&entry->b8b==12&&
   entry->f80==1&&entry->f68==26&&entry->f6c==30&&entry->f70[3]>1);
 gh_id_frontier frontier={0};
 C(!gh_id_jobs_move_reference_rotation(&base,NULL,NULL,&frontier));
 gh_id_texture_quad quad;
 C(!gh_id_texture_quad_prepare(manager,&base,entry,&quad));
 C(quad.x0==481&&quad.y0==108&&quad.x1==507&&quad.y1==138);
 C(quad.u0==entry->f70[0]&&quad.v0==entry->f70[1]&&
   quad.u1==entry->f70[2]&&quad.v1==entry->f70[3]&&quad.v1>1);
 C(quad.color_mode==6&&quad.font_mode==5&&!quad.clip);
 C(quad.texture.source.offset==0x19200&&quad.texture.source.size==9280);
 gh_id_texture t;
 C(gh_id_texture_open(manager,1,20,&t)==GH_ID_UNSUPPORTED);
 for(unsigned item=12;item<=13;++item){
  gh_tim2_picture picture;
  C(!gh_id_texture_open(manager,1,item,&t));
  C(t.width==128&&t.height==64&&t.tex0==UINT64_C(0x19d308000));
  C(!gh_tim2_first(t.source.bytes,t.source.size,&picture)&&picture.image_size==8192&&picture.clut_size==1024);
  C(!gh_id_texture_rgba(&t,rgba,8192*4));
  unsigned nonzero=0;
  static const unsigned physical_group[]={0,2,1,3};
  for(unsigned i=0;i<8192;++i){
   unsigned index=picture.image[i];
   unsigned palette=(index/32)*32+physical_group[(index/8)%4]*8+index%8;
   const unsigned char *expected=picture.clut+palette*4,*pixel=rgba+i*4;
   C(pixel[0]==expected[0]&&pixel[1]==expected[1]&&pixel[2]==expected[2]&&expected[3]<=128);
   C(pixel[3]==(expected[3]==128?255:expected[3]*2));
   nonzero+=pixel[3]!=0;
  }
  C(nonzero>0&&nonzero<8192);
  C(gh_tim2_rgba8(&picture,rgba,8192*4-1)==GH_TIM2_BOUNDS);
  C(gh_id_texture_rgba(&t,rgba,8192*4-1)==GH_ID_CAPACITY);
  printf("actual Cockpit TIM2/T8 item%u independently mapped 8192 pixels, nontransparent %u\n",item,nonzero);
 }
 C(!gh_id_base_release_002aa078(runtime,&base,&original,&reclaimed)&&reclaimed);
 C(!gh_id_runtime_destroy(runtime));
}
int main(int argc,char **argv){gh_disc *d;gh_disc_entry entry;size_t i;unsigned char *b,*rgba;gh_bundle_entry idd={0};gh_id_state *manager=calloc(1,sizeof(*manager));gh_id_texture t;unsigned changed=0;
 C(argc==2||argc==3);C(manager);C(gh_disc_open(argv[1],&d)==GH_DISC_OK);C(gh_disc_find(d,"id/local/US/idsavecore_US.idd",&i)==GH_DISC_OK);C(gh_disc_entry_at(d,i,&entry)==GH_DISC_OK);b=malloc((size_t)entry.size);rgba=malloc(512*512*4);C(b&&rgba);C(gh_disc_read(d,i,0,b,(size_t)entry.size)==GH_DISC_OK);idd.bytes=b;idd.size=(size_t)entry.size;C(gh_id_set_data_002accf8(manager,11,&idd)==GH_MENU_OK);
 C(gh_id_texture_open(manager,11,1,&t)==GH_ID_NOT_FOUND);C(gh_id_texture_open(manager,11,2,&t)==GH_ID_OK&&t.width==512&&t.height==512);
 C(gh_id_texture_rgba(&t,rgba,512*512*4)==GH_ID_OK);
 for(i=0;i<512*512;i++)changed+=rgba[i*4]!=0||rgba[i*4+1]!=0||rgba[i*4+2]!=0;C(changed>1000);
 if(argc==3){FILE *f=fopen(argv[2],"wbx");C(f);C(fwrite(rgba,1,512*512*4,f)==512*512*4);C(fclose(f)==0);}
 C(gh_id_texture_rgba(&t,rgba,12)==GH_ID_CAPACITY);C(gh_id_texture_open(manager,11,3,&t)==GH_ID_OK&&t.height==256);C(gh_id_texture_rgba(&t,rgba,512*512*4)==GH_ID_OK);C(gh_id_texture_open(manager,11,4,&t)==GH_ID_NOT_FOUND);
 /* ReallanguagecursorTIM2 isnotanopaqueT32 packet. NativegenericTBL1lookup
  * mustdispatchtheoriginalTIM2 branchandretainactual16coloralpha domain. */
 {unsigned char*lang;gh_bundle bundle;gh_tim2_picture picture;uint64_t hash=UINT64_C(14695981039346656037);unsigned nonzero=0;
  C(gh_disc_find(d,"etc/core_usa.bin",&i)==GH_DISC_OK&&gh_disc_entry_at(d,i,&entry)==GH_DISC_OK);lang=malloc((size_t)entry.size);C(lang&&gh_disc_read(d,i,0,lang,(size_t)entry.size)==GH_DISC_OK&&gh_bundle_open(lang,(size_t)entry.size,&bundle)==GH_BUNDLE_OK&&gh_id_find_language_data(&bundle,&idd)==GH_MENU_OK&&gh_id_system_init_002acc70(manager,&idd)==GH_MENU_OK);
  C(gh_id_texture_open(manager,1,47,&t)==GH_ID_OK&&t.width==32&&t.height==32&&gh_tim2_first(t.source.bytes,t.source.size,&picture)==GH_TIM2_OK&&picture.colors==16&&picture.image_size==512&&picture.clut_size==64);
  for(i=0;i<16;i++)C(picture.clut[i*4+3]<=128);
  C(gh_id_texture_rgba(&t,rgba,4096)==GH_ID_OK);for(i=0;i<4096;i++)hash=(hash^rgba[i])*UINT64_C(1099511628211);for(i=0;i<1024;i++)nonzero+=rgba[i*4+3]!=0;C(hash==UINT64_C(0x4950639901636b2f)&&nonzero==194);
  {unsigned char*alpha=(unsigned char*)picture.clut+3;unsigned char saved=*alpha;*alpha=129;C(gh_id_texture_rgba(&t,rgba,4096)==GH_ID_UNSUPPORTED);*alpha=saved;}
  C(gh_id_texture_open(manager,1,31,&t)==GH_ID_OK&&t.width==32&&t.height==16&&t.tex0==UINT64_C(0x115408000));
  C(gh_tim2_first(t.source.bytes,t.source.size,&picture)==GH_TIM2_OK&&picture.image_size==256&&picture.clut_size==64&&picture.colors==16);
  C(gh_id_texture_rgba(&t,rgba,2048)==GH_ID_OK);unsigned loading_nonzero=0;
  for(unsigned pixel=0;pixel<512;pixel++){unsigned shift=pixel%2?4:0,index4=picture.image[pixel/2]>>shift&15;const unsigned char*color=picture.clut+index4*4;
   C(color[3]<=128);for(unsigned channel=0;channel<3;channel++)C(rgba[pixel*4+channel]==color[channel]);C(rgba[pixel*4+3]==(color[3]==128?255:color[3]*2));loading_nonzero+=rgba[pixel*4+3]!=0;
  }C(loading_nonzero>0);C(gh_id_texture_rgba(&t,rgba,2047)==GH_ID_CAPACITY);printf("actualSystemMenu32x16 item31 independentlymapped512pixels nontransparent%u\n",loading_nonzero);
  printf("actualcursor32x32 RGBAfnv%016llx nontransparent%u\n",(unsigned long long)hash,nonzero);radar_texture(manager,rgba);hud_texture(manager,rgba);free(lang);
 }
 free(rgba);free(b);free(manager);gh_disc_close(d);puts("PASS actual MC TBL1 T32/PSMT8 512x512 and512x256 native decoding; source bounds and empty slots");return 0;}

