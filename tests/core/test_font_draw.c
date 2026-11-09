#include "../../src/core/font_draw.h"
#include "../../src/platform/disc.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);exit(1);}}while(0)
int main(int argc,char **argv){gh_disc *d;gh_disc_entry e;size_t i;unsigned char *b;gh_bundle pack;gh_font_state font={0};gh_font_quad q;
 gh_font_draw_state s={1,1,9,512,448,1824,0x80808080U};
 C(argc==2);C(gh_disc_open(argv[1],&d)==GH_DISC_OK);C(gh_disc_find(d,"etc/core_usa.bin",&i)==GH_DISC_OK);C(gh_disc_entry_at(d,i,&e)==GH_DISC_OK);b=malloc((size_t)e.size);C(b);C(gh_disc_read(d,i,0,b,(size_t)e.size)==GH_DISC_OK);C(gh_bundle_open(b,(size_t)e.size,&pack)==GH_BUNDLE_OK);C(gh_font_initialize(&font,&pack,1)==GH_FONT_OK);
 C(gh_font_draw_quad(&font,&s,1,10,20,3,&q)==GH_FONT_DRAW_OK);
 C(q.x0==28832&&q.x1==29248&&q.y0==29504&&q.y1==30016);
 C(q.u0==56&&q.u1==472&&q.v0==8&&q.v1==520&&q.z==3&&q.advance==26&&q.palette_parity==1&&q.rgba==0x80808080U);
 s.alignment_flags=18;C(gh_font_draw_quad(&font,&s,1,100,100,0,&q)==GH_FONT_DRAW_OK);
 C(q.x0==30064&&q.y0==30528); /* centered:87,84 then actual GS origins */
 s.scale_x=2;s.scale_y=0.5f;s.alignment_flags=9;s.screen_height=224;
 C(gh_font_draw_quad(&font,&s,1,10.07f,20,1,&q)==GH_FONT_DRAW_OK);C(q.x0==28833&&q.x1==29665&&q.y1-q.y0==128&&q.advance==52);
 C(gh_font_draw_quad(&font,&s,0,0,0,0,&q)==GH_FONT_DRAW_UNSUPPORTED);
 C(gh_font_draw_quad(&font,&s,1,NAN,0,0,&q)==GH_FONT_DRAW_UNSUPPORTED);
 free(b);gh_disc_close(d);puts("PASS real font glyph->original GS quad origins/scales/align/halftexel/raw tint");return 0;}
