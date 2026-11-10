#include "../../src/assets/static_mesh.h"
#include "../../src/platform/disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint64_t word_hash(uint64_t h,uint32_t n){for(unsigned i=0;i<4;i++)h=(h^((n>>(i*8))&255))*UINT64_C(1099511628211);return h;}
int main(int argc,char**argv){C(argc==2);gh_disc*d;size_t i;gh_disc_entry file;C(!gh_disc_open(argv[1],&d)&&!gh_disc_find(d,"st1/r100.dat",&i)&&!gh_disc_entry_at(d,i,&file));
 uint8_t*room=malloc((size_t)file.size);C(room&&!gh_disc_read(d,i,0,room,(size_t)file.size));gh_bundle bundle,scp;gh_bundle_entry child,scr;
 C(!gh_bundle_open(room,(size_t)file.size,&bundle)&&!gh_bundle_find(&bundle,"SCP",0,&child)&&!gh_bundle_open(child.bytes,child.size,&scp)&&!gh_bundle_find(&scp,"SCR",0,&scr));
 long n=(long)scr.size;C(n==12640);uint8_t*b=malloc((size_t)n);C(b);memcpy(b,scr.bytes,(size_t)n);free(room);gh_disc_close(d);
 /* Actual firstR100 descriptor borrowed from the separately tested canonical
  * SCR/MDB parser: this fixture does not replace that parser/room owner. */
 gh_scene_mesh_stream s={.bytes=b,.size=(size_t)n,.header_offset=128,.position_offset=160,.uv_offset=544,.color_offset=736,.vertices=48,.material=37};
 size_t count;uint32_t frontier;gh_static_mesh_batch batch;C(!gh_static_mesh_batch_count(&s,&count)&&count==2);
 C(!gh_static_mesh_batch_at(&s,0x1001000,0,&batch,&frontier)&&!frontier&&batch.first_vertex==0&&batch.logical_vertices==38&&batch.fetched_vertices==40&&batch.position_ref_qwords==20&&batch.uv_ref_qwords==10&&batch.color_ref_qwords==10&&batch.position_unpack==0x6d288002&&batch.uv_unpack==0x65288004&&batch.color_unpack==0x6e28c003);
 C(batch.gif_strip[0]==UINT64_C(0x303e400000008026)&&batch.gif_fan[0]==UINT64_C(0x303ec00000008026)&&batch.gif_strip[1]==0x412&&batch.gif_fan[1]==0x412);
 C(!gh_static_mesh_batch_at(&s,0x1001000,1,&batch,&frontier)&&batch.first_vertex==36&&batch.logical_vertices==12&&batch.fetched_vertices==16&&batch.position_offset==448&&batch.uv_offset==688&&batch.color_offset==880);
 C(batch.position_offset+batch.fetched_vertices*8==576&&576>s.uv_offset); /* real fetch crosses logicalpositiontail, no zero padding */
 gh_static_mesh_batch saved=batch;C(gh_static_mesh_batch_at(&s,0x1001000,2,&batch,&frontier)==GH_STATIC_BOUNDS&&!memcmp(&batch,&saved,sizeof(batch)));
 gh_static_mesh_vertex v;C(!gh_static_mesh_vertex_at(&s,0x1001000,0,&v,&frontier)&&v.position[0]==73&&v.position[1]==33&&v.position[2]==63&&v.position[3]==-32768&&v.raw_w==0x8000&&v.uv[0]==4057&&v.uv[1]==-731&&v.color[0]==57&&v.color[3]==128);
 uint64_t hash=UINT64_C(14695981039346656037);unsigned controls[3]={0};
 for(unsigned i=0;i<48;i++){C(!gh_static_mesh_vertex_at(&s,0x1001000,i,&v,&frontier));for(unsigned j=0;j<4;j++)hash=word_hash(hash,(uint32_t)v.position[j]);for(unsigned j=0;j<2;j++)hash=word_hash(hash,(uint32_t)v.uv[j]);for(unsigned j=0;j<4;j++)hash=word_hash(hash,v.color[j]);if(v.raw_w==0x8000)controls[0]++;else if(v.raw_w==0)controls[1]++;else if(v.raw_w==1)controls[2]++;else C(0);}
 C(hash==UINT64_C(0x7dde78e32fcc0347)&&controls[0]==24&&controls[1]==12&&controls[2]==12); /* independentPython struct-unpack oracle */
 C(!gh_static_mesh_batch_at(&s,0x1001010,0,&batch,&frontier)&&batch.gif_strip[0]==UINT64_C(0x302e400000008026));
 saved=batch;C(gh_static_mesh_batch_at(&s,0x1000,0,&batch,&frontier)==GH_STATIC_UNSUPPORTED&&frontier==0x155800&&!memcmp(&batch,&saved,sizeof(batch)));
 s.size=920;C(gh_static_mesh_batch_at(&s,0x1001000,1,&batch,&frontier)==GH_STATIC_BOUNDS);s.size=(size_t)n;
 s.vertices=38;C(!gh_static_mesh_batch_count(&s,&count)&&count==1);s.vertices=39;C(!gh_static_mesh_batch_count(&s,&count)&&count==2&&!gh_static_mesh_batch_at(&s,0x1001000,1,&batch,&frontier)&&batch.logical_vertices==3&&batch.fetched_vertices==8);
 free(b);puts("PASS actual firstR100 typed VIF input vectors, independent48vertex oracle,38/36 overlap/rounded physical fetch/GIF headers; no VU transform claimed");return 0;
}
