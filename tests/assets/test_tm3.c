#include "../../src/assets/tm3.h"
#include "../../src/core/game_stage.h"
#include "../../src/platform/disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL%d %s\n",__LINE__,#x);exit(1);}}while(0)
static unsigned bit(size_t a,unsigned n){return (unsigned)(a>>n)&1u;}
static unsigned char*load(gh_disc*d,const char*path,size_t*n){size_t i;gh_disc_entry e;C(!gh_disc_find(d,path,&i)&&!gh_disc_entry_at(d,i,&e));unsigned char*b=malloc((size_t)e.size);C(b&&!gh_disc_read(d,i,0,b,(size_t)e.size));*n=(size_t)e.size;return b;}
/* Independent transfer-forward / sample-inverse oracle. Production directly
 * combines the opposite transformations and never constructs this VRAM view. */
static void oracle(const gh_tm3_picture*p,unsigned char*out){
 size_t pixels=(size_t)p->width*p->height;unsigned char*vram=calloc(pixels/2,1),*seen=calloc(pixels,1);C(vram&&seen);
 unsigned pitch=p->width/128;
 if(p->transfer_kind==6){unsigned uw=p->width/2,uh=p->height/4;
  for(unsigned y=0;y<uh;y++)for(unsigned x=0;x<uw;x++){
   unsigned bx=(x%64)/8,by=(y%32)/8,cx=x%8,cy=y%8;
   unsigned block=bx%2+2*(by%2)+4*((bx/2)%2)+8*(by/2)+16*(bx/4);
   unsigned column=cx%2+2*(cy%2)+4*((cx/2)%2)+8*(cx/4)+16*((cy/2)%2)+32*(cy/4);
   size_t a=(((size_t)(y/32)*pitch+x/64)*2048+block*64+column)*4;C(a+4<=pixels/2);memcpy(vram+a,p->image+((size_t)y*uw+x)*4,4);
  }
  for(size_t a=0;a<pixels;a++){
   unsigned x=bit(a,3)|(bit(a,5)<<1)|((bit(a,6)^bit(a,0)^bit(a,7))<<2)|(bit(a,1)<<3)|(bit(a,2)<<4)|(bit(a,10)<<5)|(bit(a,12)<<6);
   unsigned y=bit(a,4)|(bit(a,0)<<1)|(bit(a,7)<<2)|(bit(a,8)<<3)|(bit(a,9)<<4)|(bit(a,11)<<5)|(bit(a,13)<<6);
   x+=(unsigned)((a/16384)%pitch)*128;y+=(unsigned)((a/16384)/pitch)*128;C(x<p->width&&y<p->height&&!seen[(size_t)y*p->width+x]);seen[(size_t)y*p->width+x]=1;
   unsigned index=vram[a/2]>>((a&1)*4)&15;memcpy(out+((size_t)y*p->width+x)*4,p->clut+index*4,4);
  }
 }else for(size_t i=0;i<pixels;i++){unsigned index=p->image[i/2]>>(i%2?4:0)&15;memcpy(out+i*4,p->clut+index*4,4);}
 free(seen);free(vram);
}
int main(int argc,char**argv){C(argc==2);gh_disc*d;C(!gh_disc_open(argv[1],&d));size_t room_bytes,core_bytes;unsigned char*raw=load(d,"st1/r100.dat",&room_bytes),*core=load(d,"etc/core.dat",&core_bytes);gh_disc_close(d);
 gh_bundle room,scp;gh_bundle_entry entry;C(!gh_bundle_open(raw,room_bytes,&room)&&!gh_bundle_find(&room,"SCP",0,&entry)&&!gh_bundle_open(entry.bytes,entry.size,&scp)&&!gh_bundle_find(&scp,"TM3",0,&entry));gh_tm3_bundle tm3;C(!gh_tm3_open(entry.bytes,entry.size,&tm3)&&tm3.count==69);
 unsigned normal=0,fast=0;size_t pixels=0;uint64_t hash=UINT64_C(14695981039346656037);
 for(unsigned i=0;i<69;i++){gh_tm3_picture p;C(!gh_tm3_member(&tm3,i,&p));size_t bytes=(size_t)p.width*p.height*4;unsigned char*rgba=malloc(bytes),*expected=malloc(bytes);C(rgba&&expected&&!gh_tm3_rgba_gs(&p,rgba,bytes));oracle(&p,expected);C(!memcmp(rgba,expected,bytes));for(size_t j=0;j<bytes;j++)hash=(hash^rgba[j])*UINT64_C(1099511628211);pixels+=bytes/4;if(p.transfer_kind==6)fast++;else normal++;C(gh_tm3_rgba_gs(&p,rgba,bytes-1)==GH_TM3_CAPACITY);free(expected);free(rgba);}
 C(normal==45&&fast==24);gh_tm3_picture p;C(!gh_tm3_member(&tm3,37,&p)&&p.width==32&&p.height==32&&p.image_bytes==512&&p.tex0==UINT64_C(0x155408000));
 unsigned char indices[512],palette[64]={0},rgba[4096];memset(indices,0x21,sizeof(indices));palette[4]=1;palette[5]=2;palette[6]=3;palette[7]=129;palette[8]=4;palette[9]=5;palette[10]=6;palette[11]=128;gh_tm3_picture known=p;known.image=indices;known.clut=palette;C(!gh_tm3_rgba_gs(&known,rgba,sizeof(rgba))&&!memcmp(rgba,palette+4,4)&&!memcmp(rgba+4,palette+8,4));
 C(gh_tm3_member(&tm3,69,&p)==GH_TM3_BOUNDS);unsigned char*flag=(unsigned char*)tm3.bytes+5;unsigned char saved=*flag;*flag=0xff;gh_tm3_bundle bad;C(gh_tm3_open(tm3.bytes,tm3.size,&bad)==GH_TM3_BOUNDS);*flag=saved;
 gh_stage_textures*manager;C(!gh_stage_textures_create(&manager)&&!gh_stage_core_textures(manager,core,core_bytes));gh_stage_texture_cursor before,after;gh_stage_channel global;uint32_t pixel_before,pixel_after;C(!gh_stage_texture_cursor_get(manager,&before)&&!gh_stage_texture_channel(manager,0,&global)&&!gh_stage_vram_ct32(manager,global.base,1,0,0,&pixel_before));
 const gh_stage_material*a,*b,*c;C(!gh_stage_tm3_resolve(manager,tm3.bytes,tm3.size,37,1,&a)&&a->width==32&&a->height==32&&a->record_mode==1&&a->cache_pool==1);C(!gh_stage_tm3_resolve(manager,tm3.bytes,tm3.size,37,1,&b)&&a==b);C(!gh_stage_tm3_resolve(manager,tm3.bytes,tm3.size,37,3,&c)&&c!=a&&c->cache_pool==0&&c->cache_id>a->cache_id);C(!gh_stage_tm3_resolve(manager,tm3.bytes,tm3.size,37,0,&b)&&b==c);uint64_t last=c->cache_id;unsigned char*copy=malloc(tm3.size);C(copy);memcpy(copy,tm3.bytes,tm3.size);const gh_stage_material*outside;C(!gh_stage_tm3_resolve(manager,copy,tm3.size,37,1,&outside));uint64_t outside_id=outside->cache_id;last=outside_id;
 C(gh_stage_texture_forget_source_range(manager,(void*)(UINTPTR_MAX-2),8)==GH_STAGE_BOUNDS);C(!gh_stage_texture_forget_source_range(manager,tm3.bytes,tm3.size));C(!gh_stage_texture_cursor_get(manager,&after)&&!memcmp(&before,&after,sizeof(before))&&!gh_stage_texture_channel(manager,0,&global)&&!gh_stage_vram_ct32(manager,global.base,1,0,0,&pixel_after)&&pixel_before==pixel_after);
 C(!gh_stage_tm3_resolve(manager,copy,tm3.size,37,1,&b)&&b==outside&&b->cache_id==outside_id);C(!gh_stage_tm3_resolve(manager,tm3.bytes,tm3.size,37,1,&a)&&a->cache_id>last);C(gh_stage_tm3_resolve(manager,tm3.bytes,tm3.size,37,4,&b)==GH_STAGE_UNSUPPORTED&&!b);
 C(!gh_stage_texture_forget_source_range(manager,core,core_bytes)&&gh_stage_texture_channel(manager,0,&global)==GH_STAGE_UNAVAILABLE);C(!gh_stage_texture_cursor_get(manager,&after)&&!memcmp(&before,&after,sizeof(before)));gh_stage_textures_destroy(manager);free(copy);free(core);free(raw);printf("PASS actualR100TM3 69 materials(%u direct/%u CT32-fast),%zu pixels independenttransfer/sampleinverse RGBA-GS hash%016llx; rawalpha129 retained, modecacheidentity/rangeforget/coreVRAM/lifecycle\n",normal,fast,pixels,(unsigned long long)hash);return 0;
}
