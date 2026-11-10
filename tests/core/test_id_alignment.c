#include "id_jobs.h"
#include "disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
int main(int argc,char**argv){gh_disc*d;C(argc==2&&!gh_disc_open(argv[1],&d));size_t i;gh_disc_entry file;
 C(!gh_disc_find(d,"etc/core_usa.bin",&i)&&!gh_disc_entry_at(d,i,&file));uint8_t*data=malloc((size_t)file.size);gh_id_state*m=calloc(1,sizeof(*m));
 C(data&&m&&!gh_disc_read(d,i,0,data,(size_t)file.size));gh_bundle b;gh_bundle_entry idd;
 C(!gh_bundle_open(data,(size_t)file.size,&b)&&!gh_id_find_language_data(&b,&idd)&&!gh_id_system_init_002acc70(m,&idd));
 gh_id_base base={0};gh_id_entry e={0};e.kind=0;e.b8a=1;e.b8b=30;e.f80=1;e.draw_scale_x=e.draw_scale_y=1;
 e.f68=256;e.f6c=128;e.draw_x=-7.25f;e.draw_y=10.5f;e.f70[2]=e.f70[3]=1;e.color=0x80808080;
 /* Constants from actual2C5EA8's five zero-angle corner lists, including
  * drawInit(+1,+1). Rectangles retain offscreen coordinates for GS scissor. */
 const float expected[5][4]={{-134.25f,-52.5f,121.75f,75.5f},{-6.25f,11.5f,249.75f,139.5f},
  {-262.25f,11.5f,-6.25f,139.5f},{-6.25f,-116.5f,249.75f,11.5f},{-262.25f,-116.5f,-6.25f,11.5f}};
 gh_id_texture_quad q;
 for(unsigned a=0;a<5;a++){e.b89=(uint8_t)a;C(!gh_id_texture_quad_prepare(m,&base,&e,&q));
  C(q.x0==expected[a][0]&&q.y0==expected[a][1]&&q.x1==expected[a][2]&&q.y1==expected[a][3]);
  C(q.texture.width==32&&q.texture.height==16&&q.color==e.color&&q.u0==0&&q.v0==0&&q.u1==1&&q.v1==1);
 }
 gh_id_texture_quad before=q;e.b89=5;C(gh_id_texture_quad_prepare(m,&base,&e,&q)==GH_ID_UNSUPPORTED&&!memcmp(&q,&before,sizeof(q)));
 e.b89=1;e.draw_rotation=1;C(gh_id_texture_quad_prepare(m,&base,&e,&q)==GH_ID_UNSUPPORTED&&!memcmp(&q,&before,sizeof(q)));
 free(m);free(data);gh_disc_close(d);puts("PASS actualT4 loading texture / all5 zero-angle alignments / offscreen coordinates / strict unsupported branches");return 0;
}
