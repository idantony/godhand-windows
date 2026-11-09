#include "../../src/core/id_jobs.h"
#include "../../src/platform/disc.h"
#include "../../src/assets/font_texture.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x)do{if(!(x)){fprintf(stderr,"FAIL%d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t word(const unsigned char*p){return p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static unsigned bit(unsigned a,unsigned n){return(a>>n)&1;}
int main(int argc,char**argv){gh_disc*d;gh_disc_entry entry;size_t slot;unsigned char*data,*rgba,*expected,*seen;gh_id_state*m=calloc(1,sizeof(*m));gh_bundle_entry idd={0};gh_id_texture t;gh_font_slot packet={0};gh_font_transfer image,clut;uint64_t hash=UINT64_C(14695981039346656037);unsigned alpha_max=0,nonzero=0;C(argc==2||argc==3);C(m&&gh_disc_open(argv[1],&d)==GH_DISC_OK&&gh_disc_find(d,"id/local/us/idtitlecore_us.idd",&slot)==GH_DISC_OK&&gh_disc_entry_at(d,slot,&entry)==GH_DISC_OK);data=malloc((size_t)entry.size);rgba=malloc(512*512*4);expected=malloc(512*512*4);seen=calloc(512*512,1);C(data&&rgba&&expected&&seen&&gh_disc_read(d,slot,0,data,(size_t)entry.size)==GH_DISC_OK);idd.bytes=data;idd.size=(size_t)entry.size;C(gh_id_set_data_002accf8(m,2,&idd)==GH_MENU_OK&&gh_id_texture_open(m,2,7,&t)==GH_ID_OK&&t.width==512&&t.height==512&&t.tex0==UINT64_C(0x2000000265420000));C(gh_id_texture_rgba(&t,rgba,512*512*4)==GH_ID_OK);
 packet.bound=1;packet.mst=t.source.bytes;packet.mst_size=t.source.size;for(unsigned i=0;i<2;i++){packet.table_offset[i]=word(packet.mst+8+i*4);packet.table_count[i]=(uint16_t)(packet.mst[16+i*2]|(unsigned)packet.mst[17+i*2]<<8);}C(gh_font_transfer_at(&packet,0,0,&image)==GH_FONT_OK&&gh_font_transfer_at(&packet,1,0,&clut)==GH_FONT_OK);C(image.width==64&&image.height==512&&image.pixel_bytes==131072&&clut.width==16&&clut.height==16);
 /* Independentinversewalk: visitphysicalnibbleaddresses exactlyonce, decode
  * logical(x,y) byinvertingGSbitplanes, theninvertCT32word->upload(x,y).
  * Thisdoesnotcall/copytheproductionforwardaddr4/addr32 converters. */
 for(unsigned a=0;a<512*512;a++){
  unsigned x=bit(a,3)|(bit(a,5)<<1)|((bit(a,6)^bit(a,0)^bit(a,7))<<2)|(bit(a,1)<<3)|(bit(a,2)<<4)|(bit(a,10)<<5)|(bit(a,12)<<6)|(bit(a,14)<<7)|(bit(a,15)<<8);
  unsigned y=bit(a,4)|(bit(a,0)<<1)|(bit(a,7)<<2)|(bit(a,8)<<3)|(bit(a,9)<<4)|(bit(a,11)<<5)|(bit(a,13)<<6)|(bit(a,16)<<7)|(bit(a,17)<<8),w=a>>3;
  unsigned ux=bit(w,0)|(bit(w,2)<<1)|(bit(w,3)<<2)|(bit(w,6)<<3)|(bit(w,8)<<4)|(bit(w,10)<<5);
  unsigned uy=bit(w,1)|(bit(w,4)<<1)|(bit(w,5)<<2)|(bit(w,7)<<3)|(bit(w,9)<<4)|((w>>11)<<5);C(x<512&&y<512&&ux<64&&uy<512&&!seen[y*512+x]);seen[y*512+x]=1;
  unsigned pixel=image.pixels[((size_t)uy*64+ux)*4+(a%8)/2],index=(pixel>>(bit(a,0)*4))&15;
  const unsigned char*color=clut.pixels+((index%8)+(index/8)*16)*4;unsigned char*out=expected+((size_t)y*512+x)*4;out[0]=color[0];out[1]=color[1];out[2]=color[2];C(color[3]<=128);out[3]=color[3]==128?255:(unsigned char)(color[3]*2);
 }
 C(!memcmp(rgba,expected,512*512*4));for(size_t i=0;i<512*512*4;i++)hash=(hash^rgba[i])*UINT64_C(1099511628211);for(unsigned i=0;i<256;i++)if(clut.pixels[i*4+3]>alpha_max)alpha_max=clut.pixels[i*4+3];for(unsigned i=0;i<512*512;i++)nonzero+=rgba[i*4+3]!=0;
 if(argc==3){FILE*f=fopen(argv[2],"wbx");C(f&&fwrite(rgba,1,512*512*4,f)==512*512*4&&!fclose(f));}
 printf("PASS actualTitleLogoT32PSMT4: 262144logicalpixels/physicalnibblesbijective, independentinverseCT32upload+CSM1pixelsallmatch; RGBAfnv%016llx alphaMax%u nontransparent%u\n",(unsigned long long)hash,alpha_max,nonzero);
 free(seen);free(expected);free(rgba);free(data);free(m);gh_disc_close(d);return 0;}
