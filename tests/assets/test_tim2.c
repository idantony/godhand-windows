#include "../../src/assets/tim2.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);return 1;}}while(0)
int main(int argc,char **argv){
 unsigned char b[2177],bad[2176],rgba[64*64*4];FILE *f;gh_tim2_picture p;
 C(argc==2);f=fopen(argv[1],"rb");C(f);C(fread(b,1,sizeof(b),f)==2176);fclose(f);
 C(gh_tim2_first(b,2176,&p)==GH_TIM2_OK);C(p.width==64&&p.height==64&&p.colors==16);
 C(p.image_type==4&&p.image_size==2048&&p.clut_size==64);
 C(p.image==b+64&&p.clut==b+2112);C(p.clut+p.clut_size==b+2176);
 C(gh_tim2_rgba4(&p,rgba,sizeof(rgba))==GH_TIM2_OK);
 {unsigned i,transparent=0,opaque=0;for(i=0;i<4096;i++){transparent+=rgba[i*4+3]==0;opaque+=rgba[i*4+3]==255;}
 printf("Actual asset alpha: transparent=%u opaque=%u total=4096\n",transparent,opaque);}
 {unsigned char indices[]={0x21,0xf0},clut[64]={0},out[16];gh_tim2_picture q={0};
 clut[4]=255;clut[7]=128;clut[9]=255;clut[11]=64;clut[60+2]=255;clut[63]=128;
 q.width=4;q.height=1;q.colors=16;q.image_type=4;q.clut_type=3;q.tex0=(uint64_t)20<<20;
 q.image=indices;q.image_size=2;q.clut=clut;q.clut_size=64;
 C(gh_tim2_rgba4(&q,out,sizeof(out))==GH_TIM2_OK);
 C(out[0]==255&&out[3]==255);C(out[5]==255&&out[7]==128);
 C(out[8]==0&&out[11]==0);C(out[14]==255&&out[15]==255);
 C(gh_tim2_rgba4(&q,out,15)==GH_TIM2_BOUNDS);
 q.clut_type=0x83;C(gh_tim2_rgba4(&q,out,16)==GH_TIM2_UNSUPPORTED);}
 C(gh_tim2_first(b,63,&p)==GH_TIM2_BOUNDS);
 memcpy(bad,b,2176);bad[16]=255;bad[17]=255;C(gh_tim2_first(bad,2176,&p)==GH_TIM2_BOUNDS);
 memcpy(bad,b,2176);bad[35]=255;C(gh_tim2_first(bad,2176,&p)==GH_TIM2_UNSUPPORTED);
 memcpy(bad,b,2176);bad[0]='X';C(gh_tim2_first(bad,2176,&p)==GH_TIM2_FORMAT);
 puts("PASS actual TIM2 texture planes and malformed/unsupported rejection");return 0;
}
