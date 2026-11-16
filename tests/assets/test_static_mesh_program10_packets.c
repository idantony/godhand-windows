#include "../../src/assets/static_mesh_program10_packets.h"
#include "../../src/platform/disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fenv.h>
#include <math.h>
#include <float.h>
#define C(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
/* Exact matrices from the native renderer at R103 tick 2979. The stream is
 * the original room's SCR0 / mesh5 / stream0 from the supplied disc. */
static void r103_near_plane_strip(const char *iso)
{
    static const uint32_t screen_bits[]={
        0x4480fd9a,0x43a896cb,0xbfd900d8,0x3e2eedee,0xc33b6ca8,0xc477f02e,0x3f6880ec,0xbdbb6ca8,
        0xc4ec4cee,0xc4f20b30,0x411bc6ad,0xbf7b2587,0xc833d372,0xc82a69a5,0x45e86f52,0xc2b208ee};
    static const uint32_t clip_bits[]={
        0x3fd5254b,0xbcf87077,0x3e2efcdb,0x3e2eedee,0,0xbff63915,0xbdbb7ca6,0xbdbb6ca8,
        0x3e9475fe,0x3e3257ac,0xbf7b3af5,0xbf7b2587,0xc08f4940,0x4195553b,0xc2b27e8a,0xc2b208ee};
    static const uint32_t projection_bits[]={
        0x43ccccce,0,0,0,0,0x43d11110,0,0,0,0,0xc6fffe00,0,0x45000000,0x45000000,0x47000000,0x3f800000};
    static const uint32_t uv_bits[]={0,0x3f1eaa00};
    static const uint32_t color_bits[]={0x3f7eb852,0x3f7eb852,0x3f7eb852,0x3f800000};
    static const uint32_t fog_bits[]={0x43990000,0xbf028f5c};
    gh_disc *disc;
    size_t entry_index;
    gh_disc_entry entry;
    C(!gh_disc_open(iso,&disc));
    C(!gh_disc_find(disc,"st1/r103.dat",&entry_index)&&!gh_disc_entry_at(disc,entry_index,&entry));
    uint8_t *data=malloc((size_t)entry.size);
    C(data&&!gh_disc_read(disc,entry_index,0,data,(size_t)entry.size));
    gh_disc_close(disc);

    gh_bundle room,objects;
    gh_bundle_entry scp,scr;
    C(!gh_bundle_open(data,(size_t)entry.size,&room)&&!gh_bundle_find(&room,"SCP",0,&scp));
    C(!gh_bundle_open(scp.bytes,scp.size,&objects)&&!gh_bundle_find(&objects,"SCR",0,&scr));
    gh_scene_mesh_descriptor mesh={.node_flags=0x01001000};
    C(!gh_model_open_scr(scr.bytes,scr.size,&mesh.script));
    C(!gh_model_mesh_at(&mesh.script,5,&mesh.mesh));
    gh_scene_mesh_stream stream;
    C(!gh_scene_mesh_stream_at(&mesh,0,&stream));

    gh_static_transform_inputs input;
    float projection[16];
    memcpy(input.screen_matrix,screen_bits,sizeof(screen_bits));
    memcpy(input.clip_matrix,clip_bits,sizeof(clip_bits));
    memcpy(projection,projection_bits,sizeof(projection_bits));
    memcpy(input.uv_offset,uv_bits,sizeof(uv_bits));
    memcpy(input.color_scale,color_bits,sizeof(color_bits));
    memcpy(input.fog,fog_bits,sizeof(fog_bits));

    size_t batch_count=0,total_triangles=0,unused_overflows=0;
    C(gh_static_mesh_batch_count(&stream,&batch_count)==GH_STATIC_OK);
    for(size_t batch=0;batch<batch_count;batch++){
        gh_static_gs_vertex vertices[362];
        uint32_t indices[864];
        gh_static_packet_range ranges[37];
        gh_static_packet_result result;
        gh_static_transform_frontier frontier;
        const gh_static_mesh_status status=gh_static_mesh_program10_packets(&stream,mesh.node_flags,
            batch,&input,projection,GH_STATIC_BINARY32_REFERENCE,vertices,362,indices,864,ranges,37,&result,&frontier);
        if(status)fprintf(stderr,"R103 tick2979 SCR0/mesh5 batch%zu status%d EE%08X VU%08X\n",
            batch,status,frontier.ee_entry,frontier.vu_byte_pc);
        C(status==GH_STATIC_OK&&result.packets>0);
        total_triangles+=result.triangles;
        const gh_static_packet_range *strip=&ranges[result.packets-1];
        const gh_static_gs_vertex *strip_vertices=vertices+strip->vertex_offset;
        for(size_t i=0;i<strip->vertices;i++){
            const gh_static_gs_vertex *vertex=&strip_vertices[i];
            C(isfinite(vertex->stq[0])&&isfinite(vertex->stq[1])&&isfinite(vertex->stq[2]));
            unused_overflows+=vertex->clip_flags!=0&&
                (fabsf(vertex->stq[0])==FLT_MAX||fabsf(vertex->stq[1])==FLT_MAX);
        }
        for(size_t i=0;i<strip->triangles*3;i++){
            const uint32_t index=indices[strip->index_offset+i];
            C(index<strip->vertices&&strip_vertices[index].clip_flags==0);
        }
    }
    fprintf(stderr,"R103 tick2979 clipped strip: %zu unused overflowed vertices, %zu triangles\n",
        unused_overflows,total_triangles);
    C(unused_overflows>0);
    free(data);
}

int main(int argc,char**argv){C(argc==2);r103_near_plane_strip(argv[1]);gh_disc*d;size_t i;gh_disc_entry file;C(!gh_disc_open(argv[1],&d)&&!gh_disc_find(d,"st1/r100.dat",&i)&&!gh_disc_entry_at(d,i,&file));
 uint8_t*room=malloc((size_t)file.size);C(room&&!gh_disc_read(d,i,0,room,(size_t)file.size));gh_bundle bundle,scp;gh_bundle_entry child,scr;
 C(!gh_bundle_open(room,(size_t)file.size,&bundle)&&!gh_bundle_find(&bundle,"SCP",0,&child)&&!gh_bundle_open(child.bytes,child.size,&scp)&&!gh_bundle_find(&scp,"SCR",0,&scr));
 long n=(long)scr.size;C(n==12640);uint8_t*b=malloc((size_t)n);C(b);memcpy(b,scr.bytes,(size_t)n);free(room);gh_disc_close(d);
 gh_scene_mesh_stream s={.bytes=b,.size=(size_t)n,.header_offset=128,.position_offset=160,.uv_offset=544,.color_offset=736,.vertices=48,.material=37};

 gh_static_transform_inputs in={.screen_matrix={100,0,0,0,0,-100,0,0,0,0,1,.05f,2048,2048,1000,2},.clip_matrix={.25f,0,0,0,0,.25f,0,0,0,0,.25f,0,0,0,0,1},.uv_offset={.25f,-.125f},.color_scale={.75f,1,1.25f,.5f},.fog={100,-25}};
 const float projection[16]={100,0,0,0,0,100,0,0,0,0,1,0,2048,2048,1000,1};
 gh_static_gs_vertex vertices[362],expected[38];uint32_t indices[864],ei[108];gh_static_packet_range ranges[37];gh_static_packet_result result;gh_static_geometry_result er;gh_static_transform_frontier f;
 for(unsigned batch=0;batch<2;batch++){
  C(!gh_static_mesh_program10_packets(&s,0x1001000,batch,&in,projection,GH_STATIC_BINARY32_REFERENCE,vertices,362,indices,864,ranges,37,&result,&f));
  C(!gh_static_mesh_transform_program10_batch(&s,0x1001000,batch,&in,GH_STATIC_BINARY32_REFERENCE,expected,38,ei,108,&er,&f));
  C(result.packets==1&&result.vertices==er.vertices&&result.triangles==er.triangles&&ranges[0].primitive==0x7c);
  C(!memcmp(vertices,expected,er.vertices*sizeof(*vertices))&&!memcmp(indices,ei,er.triangles*3*sizeof(*indices)));
 }
 /* Clearly labeled VIF input fixture in a private writable copy, NOT a
  * fabricated runtime room. Exercises real batch clipping/XGKICK order. */
 s.vertices=3;const int16_t positions[3][4]={{-50,-50,0,(int16_t)0x8000},{200,-50,0,(int16_t)0x8000},{-50,50,0,1}};
 for(unsigned k=0;k<3;k++){memcpy(b+160+k*8,positions[k],8);memset(b+544+k*4,0,4);memset(b+736+k*4,100,4);}
 memset(&in,0,sizeof(in));memcpy(in.screen_matrix,projection,sizeof(projection));in.clip_matrix[0]=in.clip_matrix[5]=in.clip_matrix[10]=in.clip_matrix[15]=1;for(unsigned k=0;k<4;k++)in.color_scale[k]=1;in.fog[0]=100;in.fog[1]=-25;
 const int modes[4]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};
 for(unsigned mode=0;mode<4;mode++){
  C(!fesetround(modes[mode])&&!feclearexcept(FE_ALL_EXCEPT)&&!feraiseexcept(FE_DIVBYZERO));
  C(!gh_static_mesh_program10_packets(&s,0x1001000,0,&in,projection,GH_STATIC_BINARY32_REFERENCE,vertices,362,indices,864,ranges,37,&result,&f));
  C(fegetround()==modes[mode]&&fetestexcept(FE_ALL_EXCEPT)==FE_DIVBYZERO);
  C(result.packets==2&&result.vertices==7&&result.triangles==2);
  C(ranges[0].primitive==0x7d&&ranges[0].vertices==4&&ranges[0].triangles==2&&ranges[0].vertex_offset==0&&ranges[0].material==37);
  C(ranges[1].primitive==0x7c&&ranges[1].vertices==3&&!ranges[1].triangles&&ranges[1].vertex_offset==4&&ranges[1].index_offset==6);
  C(vertices[1].xyz_fixed4[0]==34368&&vertices[2].xyz_fixed4[1]==32608&&vertices[6].adc);
  const uint32_t ix[6]={0,1,2,0,2,3};C(!memcmp(indices,ix,sizeof(ix)));
 }
 C(!fesetround(FE_TONEAREST));gh_static_packet_result old_result=result;gh_static_gs_vertex saved[7];memcpy(saved,vertices,sizeof(saved));
 C(gh_static_mesh_program10_packets(&s,0x1001000,0,&in,projection,GH_STATIC_BINARY32_REFERENCE,vertices,6,indices,864,ranges,37,&result,&f)==GH_STATIC_BOUNDS&&!memcmp(&old_result,&result,sizeof(result))&&!memcmp(saved,vertices,sizeof(saved)));
 /* Original ADC bypasses clipping/fan; strip remains but no kick. */
 b[160+2*8+6]=0;b[160+2*8+7]=0x80;
 C(!gh_static_mesh_program10_packets(&s,0x1001000,0,&in,projection,GH_STATIC_BINARY32_REFERENCE,vertices,362,indices,864,ranges,37,&result,&f)&&result.packets==1&&result.vertices==3&&!result.triangles);
 free(b);puts("PASS program10 actualISO nonclipgoldens plus explicitclippedfixture fan-beforestrip/ADC/atomiccapacity/4FEnv; notliveframecapture");return 0;
}
