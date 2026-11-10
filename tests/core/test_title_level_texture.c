#include "../../src/core/id_jobs.h"
#include "../../src/assets/font_texture.h"
#include "../../src/platform/disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL%d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t word(const unsigned char*p){return p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static unsigned bit(unsigned a,unsigned n){return a>>n&1;}
int main(int argc,char**argv){C(argc==2);gh_disc*d;size_t index;gh_disc_entry e;C(!gh_disc_open(argv[1],&d)&&!gh_disc_find(d,"id/local/us/idtitlecore_us.idd",&index)&&!gh_disc_entry_at(d,index,&e));unsigned char*b=malloc((size_t)e.size),*rgba=malloc(256*128*4),*expected=malloc(256*128*4),*seen=calloc(256*128,1);gh_id_state*m=calloc(1,sizeof(*m));C(b&&rgba&&expected&&seen&&m&&!gh_disc_read(d,index,0,b,(size_t)e.size));gh_disc_close(d);gh_bundle_entry idd={.bytes=b,.size=(size_t)e.size};gh_id_texture t;C(!gh_id_set_data_002accf8(m,2,&idd)&&!gh_id_texture_open(m,2,11,&t)&&t.width==256&&t.height==128&&t.tex0==UINT64_C(0x20000001e1310000)&&!gh_id_texture_rgba(&t,rgba,256*128*4));
 gh_font_slot packet={0};packet.bound=1;packet.mst=t.source.bytes;packet.mst_size=t.source.size;for(unsigned i=0;i<2;i++){packet.table_offset[i]=word(packet.mst+8+i*4);packet.table_count[i]=(uint16_t)(packet.mst[16+i*2]|(unsigned)packet.mst[17+i*2]<<8);}gh_font_transfer image,clut;C(!gh_font_transfer_at(&packet,0,0,&image)&&!gh_font_transfer_at(&packet,1,0,&clut)&&image.width==64&&image.height==128&&clut.width==16&&clut.height==16);
 /* Independent physical-byte inverse: pagebit13=Xhigh,pagebit14=Yhigh,
  * unlike the old512wide pagepitch. No production forward addr8/32 call. */
 for(unsigned a=0;a<32768;a++){unsigned x=bit(a,2)|(bit(a,4)<<1)|((bit(a,5)^bit(a,0)^bit(a,6))<<2)|(bit(a,1)<<3)|(bit(a,8)<<4)|(bit(a,10)<<5)|(bit(a,12)<<6)|(bit(a,13)<<7),y=bit(a,3)|(bit(a,0)<<1)|(bit(a,6)<<2)|(bit(a,7)<<3)|(bit(a,9)<<4)|(bit(a,11)<<5)|(bit(a,14)<<6),w=a/4;
  unsigned ux=bit(w,0)|(bit(w,2)<<1)|(bit(w,3)<<2)|(bit(w,6)<<3)|(bit(w,8)<<4)|(bit(w,10)<<5),uy=bit(w,1)|(bit(w,4)<<1)|(bit(w,5)<<2)|(bit(w,7)<<3)|(bit(w,9)<<4)|((w>>11)<<5);C(x<256&&y<128&&ux<64&&uy<128&&!seen[y*256+x]);seen[y*256+x]=1;
  unsigned index8=image.pixels[((size_t)uy*64+ux)*4+a%4],pal=(index8&~24u)|((index8&8)<<1)|((index8&16)>>1);const unsigned char*c=clut.pixels+pal*4;C(c[3]<=128);size_t o=((size_t)y*256+x)*4;expected[o]=c[0];expected[o+1]=c[1];expected[o+2]=c[2];expected[o+3]=c[3]==128?255:(unsigned char)(c[3]*2);
 }C(!memcmp(rgba,expected,256*128*4));free(m);free(seen);free(expected);free(rgba);free(b);puts("PASS actualTitleLevel256x128T32T8:32768pixels independentphysicalinverse/pagepitch2/CT32upload/CSM1 exact");return 0;}
