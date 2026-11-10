#include "../../src/core/id_number.h"
#include "../../src/core/id_animation.h"
#include "../../src/assets/font_texture.h"
#include "../../src/platform/disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fenv.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL%d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t w(const uint8_t*p){uint32_t v;memcpy(&v,p,4);return v;}
static unsigned bit(unsigned n,unsigned b){return n>>b&1;}
int main(int argc,char**argv){
 C(argc==2);gh_disc*d;size_t i;gh_disc_entry de;C(!gh_disc_open(argv[1],&d)&&!gh_disc_find(d,"etc/core_usa.bin",&i)&&!gh_disc_entry_at(d,i,&de));uint8_t*lang=malloc((size_t)de.size);C(lang&&!gh_disc_read(d,i,0,lang,(size_t)de.size));gh_disc_close(d);
 gh_id_state*m=calloc(1,sizeof(*m));gh_bundle language;gh_bundle_entry idd;C(m&&!gh_bundle_open(lang,(size_t)de.size,&language)&&!gh_id_find_language_data(&language,&idd)&&!gh_id_system_init_002acc70(m,&idd));gh_id_runtime*r;gh_id_base base={0};int ret,freed;C(!gh_id_runtime_create(m,1,&r)&&!gh_id_runtime_set_path_policy(r,GH_ID_PATH_REFERENCE_NEAREST)&&!gh_id_base_initialize_002a9ea0(r,&base,1,19,NULL,NULL,&ret));gh_id_animation_profile profile={1,0};C(!gh_id_jobs_move_reference(&base,NULL,NULL,&profile,NULL)&&base.entry_count==6);
 gh_id_entry e=base.entries[3];C(e.kind==1&&e.b8c==6&&e.b89==4&&e.b8a==1&&e.b8b==3&&e.draw_scale_x>0);e.seed=0;gh_id_texture_quad q[10];size_t count=0;uint32_t f;C(!gh_id_number_prepare(m,&base,&e,q,10,&count,&f)&&count==1&&!f&&q[0].u0==0&&q[0].v0==0&&q[0].u1==.25f&&q[0].v1==.25f&&q[0].texture.tex0==UINT64_C(0x20000001dd308000));
 uint8_t*rgba=malloc(128*128*4),*expected=malloc(128*128*4),*seen=calloc(128*128,1);C(rgba&&expected&&seen&&!gh_id_number_texture_rgba(&q[0].texture,rgba,128*128*4));gh_font_slot packet={0};packet.bound=1;packet.mst=q[0].texture.source.bytes;packet.mst_size=q[0].texture.source.size;for(unsigned k=0;k<2;k++){packet.table_offset[k]=w(packet.mst+8+k*4);packet.table_count[k]=(uint16_t)(packet.mst[16+k*2]|(unsigned)packet.mst[17+k*2]<<8);}gh_font_transfer image,clut;C(!gh_font_transfer_at(&packet,0,0,&image)&&!gh_font_transfer_at(&packet,1,0,&clut)&&image.width==64&&image.height==64&&clut.width==16&&clut.height==16);
 /* Independent physical-address inverse, not production forwardaddr8/32.
  * Both128x64PSMT8pages and CT32upload word permutation are visited once. */
 for(unsigned a=0;a<16384;a++){
  unsigned x=bit(a,2)|(bit(a,4)<<1)|((bit(a,5)^bit(a,0)^bit(a,6))<<2)|(bit(a,1)<<3)|(bit(a,8)<<4)|(bit(a,10)<<5)|(bit(a,12)<<6);
  unsigned y=bit(a,3)|(bit(a,0)<<1)|(bit(a,6)<<2)|(bit(a,7)<<3)|(bit(a,9)<<4)|(bit(a,11)<<5)|(bit(a,13)<<6),word=a/4;
  unsigned ux=bit(word,0)|(bit(word,2)<<1)|(bit(word,3)<<2)|(bit(word,6)<<3)|(bit(word,8)<<4)|(bit(word,10)<<5);
  unsigned uy=bit(word,1)|(bit(word,4)<<1)|(bit(word,5)<<2)|(bit(word,7)<<3)|(bit(word,9)<<4)|(bit(word,11)<<5);C(x<128&&y<128&&ux<64&&uy<64&&!seen[y*128+x]);seen[y*128+x]=1;
  unsigned index=image.pixels[((size_t)uy*64+ux)*4+a%4],palette=(index&~24u)|((index&8)<<1)|((index&16)>>1);const uint8_t*c=clut.pixels+palette*4;C(c[3]<=128);size_t o=((size_t)y*128+x)*4;expected[o]=c[0];expected[o+1]=c[1];expected[o+2]=c[2];expected[o+3]=c[3]==128?255:(uint8_t)(c[3]*2);
 }C(!memcmp(rgba,expected,128*128*4));
 e.seed=103;C(fesetround(FE_UPWARD)==0&&!gh_id_number_prepare(m,&base,&e,q,10,&count,&f)&&count==3&&fegetround()==FE_UPWARD);C(fesetround(FE_TONEAREST)==0);C(q[0].u0==.25f&&q[0].v0==0&&q[1].u0==0&&q[2].u0==.75f&&q[2].v0==0);
 e.seed=(uint32_t)-12;C(!gh_id_number_prepare(m,&base,&e,q,10,&count,&f)&&count==3&&q[0].u0==.5f&&q[0].v0==.75f&&q[1].u0==.25f&&q[2].u0==.5f);
 e.seed=9999999;C(!gh_id_number_prepare(m,&base,&e,q,10,&count,&f)&&count==6);for(unsigned k=0;k<6;k++)C(q[k].u0==.25f&&q[k].v0==.5f);
 e.seed=0;e.flags|=0x100;C(!gh_id_number_prepare(m,&base,&e,q,10,&count,&f)&&count==6);gh_id_texture_quad before=q[0];count=99;C(gh_id_number_prepare(m,&base,&e,q,1,&count,&f)==GH_ID_CAPACITY&&count==99&&!memcmp(&before,q,sizeof(before)));
 e.seed=0x80000000;C(gh_id_number_prepare(m,&base,&e,q,10,&count,&f)==GH_ID_UNSUPPORTED&&f==0x2c70a8);
 free(rgba);free(expected);free(seen);C(!gh_id_base_release_002aa078(r,&base,&ret,&freed)&&freed&&!gh_id_runtime_destroy(r));free(m);free(lang);puts("PASS actualMoneyID19/128T32T8:16384pixels independentinverse+CSM1;0/103/minus/clamp/leadingzero per-digitgroups, capacity/FEnv/INTMIN");return 0;
}
