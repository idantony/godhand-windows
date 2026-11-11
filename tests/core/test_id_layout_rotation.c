#include "../../src/assets/tim2.h"
#include "../../src/core/game_enemy_marker_quad.h"
#include "../../src/core/id_layout_rotation.h"
#include "../../src/platform/disc.h"
#include <fenv.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL%d %s\n",__LINE__,#x);exit(1);}}while(0)
static float f(uint32_t n){float v;memcpy(&v,&n,4);return v;}
static uint32_t u(float v){uint32_t n;memcpy(&n,&v,4);return n;}
int main(int argc,char**argv){
 /* Independently walked actual VU lane/broadcast instructions and actual
  * ELF coefficient bytes, using a separate offline register-vector oracle. */
 static const uint32_t vectors[][3]={{0,0,0x3f800000},{0x80000000,0,0x3f800000},{0x3e800000,0x3e7d5767,0x3f780aa6},{0xbe800000,0xbe7d5767,0x3f780aa6},{0x3f800000,0x3f576aa4,0x3f0a5141},{0xbf800000,0xbf576aa4,0x3f0a5141},{0x3fc90fdb,0x3f800000,0},{0xbfc90fdb,0xbf800000,0},{0x40169a95,0x3f359035,0xbf347946},{0xc0169a95,0xbf359035,0xbf347946},{0x40490fdb,0,0xbf800000},{0xc0490fdb,0x80000000,0xbf800000}};
 static const int modes[]={FE_TONEAREST,FE_TOWARDZERO,FE_UPWARD,FE_DOWNWARD};fenv_t original;float s,c,a;gh_id_frontier frontier;
 C(argc==2&&fegetenv(&original)==0);
 for(unsigned mode=0;mode<4;mode++){C(fesetround(modes[mode])==0);C(feclearexcept(FE_ALL_EXCEPT)==0&&feraiseexcept(FE_INVALID|FE_DIVBYZERO)==0);int flags=fetestexcept(FE_ALL_EXCEPT);
  for(unsigned i=0;i<sizeof(vectors)/sizeof(*vectors);i++){C(gh_id_sincos_reference(f(vectors[i][0]),&s,&c)==GH_ID_OK);C(u(s)==vectors[i][1]&&u(c)==vectors[i][2]);C(fegetround()==modes[mode]&&fetestexcept(FE_ALL_EXCEPT)==flags);}
  C(gh_id_angle_reference_001f7d48(2.35318f,&a)==GH_ID_OK&&u(a)==u(2.35318f));
  C(gh_id_angle_reference_001f7d48(f(0x40c90fdb),&a)==GH_ID_OK&&a==0);
  C(gh_id_angle_reference_001f7d48(f(0x40490fdc),&a)==GH_ID_OK&&a<0);C(gh_id_angle_reference_001f7d48(f(0xc0490fdc),&a)==GH_ID_OK&&a>0);
  C(gh_id_angle_reference_001f7d48(f(0x4fc90fda),&a)==GH_ID_OK&&a==0);
  C(gh_id_angle_reference_001f7d48(f(0xcfc90fdb),&a)==GH_ID_OK&&a==0);
  a=17;
  C(gh_id_angle_reference_001f7d48(f(0x4fc90fdb),&a)==GH_ID_UNSUPPORTED&&a==17);
  C(gh_id_angle_reference_001f7d48(f(0xcfc90fdc),&a)==GH_ID_UNSUPPORTED&&a==17);
  C(gh_id_angle_reference_001f7d48(NAN,&a)==GH_ID_UNSUPPORTED&&a==17);
  C(gh_id_sincos_reference(NAN,&s,&c)==GH_ID_UNSUPPORTED&&gh_id_sincos_reference(f(1),&s,&c)==GH_ID_UNSUPPORTED);
  C(fegetround()==modes[mode]&&fetestexcept(FE_ALL_EXCEPT)==flags);
 }C(fesetenv(&original)==0);
 {gh_id_entry e[2]={{0}};gh_id_base b={0};b.entries=e;b.entry_count=2;e[0].parent_ref=-1;e[0].scale_x=e[0].scale_y=1;e[0].rotation_speed=f(0x3fc90fdb);e[0].color_base=0x80808080;e[0].pos_x=10;
  e[1].parent_ref=0;e[1].parent=e;e[1].scale_x=e[1].scale_y=1;e[1].pos_x=5;e[1].pos_y=6;e[1].color_base=0x80808080;
  C(gh_id_jobs_move(&b,NULL,NULL,&frontier)==GH_ID_UNSUPPORTED&&frontier.address==0x2aa6e0);
  C(gh_id_jobs_move_reference_rotation(&b,NULL,NULL,&frontier)==GH_ID_OK);C(e[0].rotation_x==10&&e[0].rotation_y==0&&e[0].draw_rotation==f(0x3fc90fdb));C(e[1].rotation_x==-6&&e[1].rotation_y==5&&e[1].draw_x==4&&e[1].draw_y==5);
  e[1].flags=0x80;C(gh_id_jobs_move_reference_rotation(&b,NULL,NULL,&frontier)==GH_ID_OK&&e[1].rotation_x==5&&e[1].rotation_y==6&&e[1].draw_rotation==0);
  e[0].parent_ref=0;e[0].parent=e;C(gh_id_jobs_move_reference_rotation(&b,NULL,NULL,&frontier)==GH_ID_BOUNDS);
 }
 /* Real original resource1 IDs14/21, not a synthetic widget/base replacement. */
 {gh_disc*d;size_t index;gh_disc_entry entry;unsigned char*data;gh_bundle bundle;gh_bundle_entry idd;gh_id_state*m=calloc(1,sizeof(*m));gh_id_runtime*r;
  C(m&&gh_disc_open(argv[1],&d)==GH_DISC_OK&&gh_disc_find(d,"etc/core_usa.bin",&index)==GH_DISC_OK&&gh_disc_entry_at(d,index,&entry)==GH_DISC_OK);data=malloc((size_t)entry.size);C(data&&gh_disc_read(d,index,0,data,(size_t)entry.size)==GH_DISC_OK);
  {unsigned char coeff[16];static const unsigned char expected[]={0x14,0x9c,0x2e,0x36,0x1f,0xb2,0x4f,0xb9,0x3e,0x87,0x08,0x3c,0xa4,0xaa,0x2a,0xbe};C(gh_disc_read_boot_elf(d,0x2c0140,coeff,16)==GH_DISC_OK&&!memcmp(coeff,expected,16));}gh_disc_close(d);
  C(gh_bundle_open(data,(size_t)entry.size,&bundle)==GH_BUNDLE_OK&&gh_id_find_language_data(&bundle,&idd)==GH_MENU_OK&&gh_id_system_init_002acc70(m,&idd)==GH_MENU_OK);C(gh_id_runtime_create(m,1,&r)==GH_ID_OK&&gh_id_runtime_set_path_policy(r,GH_ID_PATH_REFERENCE_NEAREST)==GH_ID_OK);
  {
   gh_id_base marker={0};int returned,reclaimed;
   C(!gh_id_base_initialize_002a9ea0(r,&marker,1,8,NULL,NULL,&returned)&&returned==1);
   C(marker.entry_count==5&&!gh_id_jobs_move_reference_rotation(&marker,NULL,NULL,&frontier));
   gh_id_entry *entry=&marker.entries[2];
   C(entry->kind==3&&entry->id==1&&entry->b89==3&&entry->b8a==1&&entry->b8b==37);
   gh_game_pl_camera camera={.constructed=1};
   gh_player_shared_vector pivot={.initialized=1};
   const float screen[16]={64,0,0,0,0,64,0,0,0,0,0,0,2048,2048,1000,1};
   const float identity[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
   memcpy(camera.fields,screen,64);
   memcpy(camera.fields+0x140,identity,64);
   memcpy(camera.fields+0x208,&(float){8},4);
   marker.vector_a[2]=4;
   marker.f40=1.0f/128;
   marker.h44=27;
   entry->color=0x80808080;
   gh_enemy_marker_quad packet;
   uint32_t pc=0;
   C(!gh_enemy_marker_quad_002ab360(&camera,&pivot,m,&marker,entry,&packet,&pc)&&!pc);
   C(packet.visible&&packet.quad.vertex_count==4&&packet.order.bank==6&&packet.order.depth==27);
   C(packet.prim==0x54&&packet.test==0x5000d&&packet.alpha==UINT64_C(0x8000000044)&&
     packet.tex1==0x60&&!packet.clamp&&packet.texture.width==64&&packet.texture.height==64&&
     packet.texture.tex0==UINT64_C(0x199308000)&&entry->color==0x20808080);
   for(unsigned i=0;i<4;i++){
    const gh_effect_trans_vertex *vertex=&packet.quad.vertices[i];
    const double x=(256.0-entry->draw_x-((i&1)?0:(double)entry->f68*entry->draw_scale_x))/128;
    const double y=(224.0-entry->draw_y+((i&2)?(double)entry->f6c*entry->draw_scale_y:0))/128;
    C(vertex->x12_4==(int32_t)((float)(2048.0+x*64)*16));
    C(vertex->y12_4==(int32_t)((float)(2048.0+y*64)*16));
    C(vertex->z==1000&&vertex->rgba==0x20808080&&vertex->q==1);
    C(vertex->s==entry->f70[(i&1)?0:2]&&vertex->t==entry->f70[(i&2)?1:3]);
   }
   gh_stage_textures *textures;
   const gh_stage_material *material,*again;
   C(!gh_stage_textures_create(&textures));
   C(!gh_stage_tim2_marker_resolve_0015e0a0(textures,packet.texture.source.bytes,
       packet.texture.source.size,&material));
   C(!gh_stage_tim2_marker_resolve_0015e0a0(textures,packet.texture.source.bytes,
       packet.texture.source.size,&again)&&again==material);
   C(material->source_tex0==UINT64_C(0x2000000599308000)&&material->psm==19&&
     material->draw_tcc==1&&material->rgba_bytes==64*64*4);
   gh_tim2_picture raw;
   C(!gh_tim2_first(packet.texture.source.bytes,packet.texture.source.size,&raw));
   unsigned clut_seen=0;
   for(unsigned i=0;i<64*64;i++){
    unsigned index=raw.image[i];
    unsigned palette=(index&0xe7)|((index&8)<<1)|((index&16)>>1);
    clut_seen+=(palette!=index);
    C(!memcmp(material->rgba_gs+i*4,raw.clut+palette*4,4));
   }
   C(clut_seen);
   gh_stage_textures_destroy(textures);
   C(!gh_id_base_release_002aa078(r,&marker,&returned,&reclaimed)&&reclaimed);
   puts("PASS ISO ID8 marker projection, actual atlas and entry, affine ST/Q, GS TEST/TEX1/ALPHA, bank6 depth27");
  }
  for(unsigned id=14;id<=21;id+=7){gh_id_base b={0};int ret,reclaimed;C(gh_id_base_initialize_002a9ea0(r,&b,1,(int)id,NULL,NULL,&ret)==GH_ID_OK&&ret==1);C(b.entry_count>2&&b.entries[2].rotation_speed>2.35f&&b.entries[2].rotation_speed<2.36f);b.playing=0;
   C(gh_id_jobs_move_reference_rotation(&b,NULL,NULL,&frontier)==GH_ID_OK);C(b.entries[2].draw_rotation==b.entries[2].rotation_speed);C(b.entries[2].rotation_x==b.entries[2].pos_x&&b.entries[2].rotation_y==b.entries[2].pos_y);
   printf("realID%u entry2 angle=%08x flags=%08x position unchanged, native layout PASS\n",id,u(b.entries[2].rotation_speed),b.entries[2].flags);C(gh_id_base_release_002aa078(r,&b,&ret,&reclaimed)==GH_ID_OK&&reclaimed);
  }C(gh_id_runtime_destroy(r)==GH_ID_OK);free(m);free(data);
 }
 puts("PASS explicit original rotation reference: VU vector goldens, angle edges, FEnv, hierarchy and real ISO IDs14/21; exact default remains strict");return 0;
}
