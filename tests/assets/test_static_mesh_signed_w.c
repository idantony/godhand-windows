#include "static_mesh_program10_packets.h"
#include "static_mesh_vu_numeric.h"
#include "disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fenv.h>
#include <math.h>
#define C(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
int main(int argc,char**argv){
 C(argc==2);gh_disc*d;size_t index;gh_disc_entry file;C(!gh_disc_open(argv[1],&d)&&!gh_disc_find(d,"st1/r100.dat",&index)&&!gh_disc_entry_at(d,index,&file));
 uint8_t*room=malloc((size_t)file.size);C(room&&!gh_disc_read(d,index,0,room,(size_t)file.size));gh_bundle rb,scp;gh_bundle_entry entry,scr;C(!gh_bundle_open(room,(size_t)file.size,&rb)&&!gh_bundle_find(&rb,"SCP",0,&entry)&&!gh_bundle_open(entry.bytes,entry.size,&scp)&&!gh_bundle_find(&scp,"SCR",0,&scr));
 uint8_t*bytes=malloc(scr.size);C(bytes);memcpy(bytes,scr.bytes,scr.size);gh_scene_mesh_stream stream={.bytes=bytes,.size=scr.size,.header_offset=128,.position_offset=160,.uv_offset=544,.color_offset=736,.vertices=3,.material=37};
 const int16_t positions[3][4]={{-50,-50,0,(int16_t)0x8000},{50,-50,0,(int16_t)0x8000},{-50,50,0,1}};
 for(unsigned k=0;k<3;k++){memcpy(bytes+160+k*8,positions[k],8);memset(bytes+544+k*4,0,4);memset(bytes+736+k*4,100,4);}
 gh_static_transform_inputs in={.screen_matrix={100,0,0,0,0,100,0,0,0,0,1,0,2048,2048,1000,-2},.clip_matrix={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},.color_scale={1,1,1,1},.fog={100,-1}};
 const float projection[16]={100,0,0,0,0,100,0,0,0,0,1,0,2048,2048,1000,1};
 const int modes[4]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};
 gh_static_gs_vertex vertices[362];uint32_t indices[864];gh_static_packet_range ranges[37];gh_static_packet_result result;gh_static_transform_frontier f;
 for(unsigned mode=0;mode<4;mode++){
  C(!fesetround(modes[mode])&&!feclearexcept(FE_ALL_EXCEPT)&&!feraiseexcept(FE_INVALID));
  C(!gh_static_mesh_program10_packets(&stream,0x1001000,0,&in,projection,GH_STATIC_BINARY32_REFERENCE,vertices,362,indices,864,ranges,37,&result,&f));
  C(result.packets==1&&result.vertices==3&&result.triangles==1&&vertices[0].stq[2]==-.5f&&indices[0]==0&&indices[1]==1&&indices[2]==2);
  gh_static_gs_vertex legacy[38];uint32_t li[108];gh_static_geometry_result lr;
  C(!gh_static_mesh_transform_batch(&stream,0x1001000,0,&in,GH_STATIC_BINARY32_REFERENCE,legacy,38,li,108,&lr,&f)&&lr.triangles==1&&!memcmp(legacy,vertices,3*sizeof(*legacy))&&!memcmp(li,indices,12));
  C(!gh_static_mesh_transform_program10_batch(&stream,0x1001000,0,&in,GH_STATIC_BINARY32_REFERENCE,legacy,38,li,108,&lr,&f)&&lr.triangles==1&&!memcmp(legacy,vertices,3*sizeof(*legacy))&&!memcmp(li,indices,12));
  for(unsigned i=0;i<3;i++)for(unsigned k=0;k<3;k++){
   long double original=(k<2?100.0L*positions[i][k]/100.0L+2048.0L:1000.0L)/-2.0L;
   C(vertices[i].xyz_fixed4[k]==(int32_t)(original*16));
  }
  C(fegetround()==modes[mode]&&fetestexcept(FE_ALL_EXCEPT)==FE_INVALID);
 }
 C(!fesetround(FE_TONEAREST));
 /* Original DIV is signed, not a positive-depth predicate. */
 float q;C(gh_static_vu_reciprocal(0,&q)&&q==FLT_MAX&&!signbit(q));C(gh_static_vu_reciprocal(-0.0f,&q)&&q==-FLT_MAX&&signbit(q));C(!gh_static_vu_reciprocal(0x1p-149f,&q));
 /* All-three common CLIP bits disable strip; zero W still produces the
  * original packed values, including FTOI saturation of overflowed XYZ. */
 memset(in.screen_matrix,0,64);in.screen_matrix[12]=in.screen_matrix[13]=in.screen_matrix[14]=2;in.clip_matrix[12]=2;in.clip_matrix[0]=0;
 C(!gh_static_mesh_program10_packets(&stream,0x1001000,0,&in,projection,GH_STATIC_BINARY32_REFERENCE,vertices,362,indices,864,ranges,37,&result,&f)&&result.packets==1&&!result.triangles&&vertices[0].xyz_fixed4[0]==INT32_MAX&&vertices[0].stq[2]==FLT_MAX);
 C(vertices[2].adc&&vertices[0].clip_flags==1&&vertices[1].clip_flags==1&&vertices[2].clip_flags==1);
 /* Negative carry Q can coexist with a genuine independently projected
  * clipped fan; no indices of the fan reference the original strip. */
 memcpy(in.screen_matrix,projection,64);in.screen_matrix[15]=-1;memset(in.clip_matrix,0,64);in.clip_matrix[0]=in.clip_matrix[5]=in.clip_matrix[10]=in.clip_matrix[15]=1;
 int16_t outside=200;memcpy(bytes+160+8,&outside,2);
 C(!gh_static_mesh_program10_packets(&stream,0x1001000,0,&in,projection,GH_STATIC_BINARY32_REFERENCE,vertices,362,indices,864,ranges,37,&result,&f)&&result.packets==2&&ranges[0].triangles==2&&!ranges[1].triangles);
 for(size_t i=0;i<ranges[0].vertices;i++){C(vertices[i].stq[2]>0);}
 for(size_t i=0;i<ranges[1].vertices;i++){C(vertices[ranges[1].vertex_offset+i].stq[2]<0);}
 for(size_t i=0;i<ranges[0].triangles*3;i++)C(indices[i]<ranges[0].vertices);
 /* Unknown overflowed UV remains strict even for a clipped-away strip. */
 in.screen_matrix[15]=0;int16_t uv=32767;memcpy(bytes+544,&uv,2);gh_static_packet_result before=result;uint8_t previous[sizeof(vertices)];memcpy(previous,vertices,sizeof(vertices));
 C(gh_static_mesh_program10_packets(&stream,0x1001000,0,&in,projection,GH_STATIC_BINARY32_REFERENCE,vertices,362,indices,864,ranges,37,&result,&f)==GH_STATIC_UNSUPPORTED&&f.vu_byte_pc==0x260&&!memcmp(&before,&result,sizeof(result))&&!memcmp(previous,vertices,sizeof(vertices)));
 free(bytes);free(room);gh_disc_close(d);puts("PASS actualSCR bounded VIF fixtures: signedW, zeroDIV/FTOIsaturation, commonCLIP, fan-before-negativeQcarry, independentprojection, FEnv/atomic; notliveworldframe");return 0;
}
