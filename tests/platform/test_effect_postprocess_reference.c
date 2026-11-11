#include "effect_postprocess_reference.h"
#include "game_effect_node.h"
#include "game_esp_create.h"
#include "game_camera_envelope.h"
#include "disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fenv.h>
#define C(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint8_t*load(gh_disc*d,const char*path,size_t*n){size_t i;gh_disc_entry e;C(!gh_disc_find(d,path,&i)&&!gh_disc_entry_at(d,i,&e));*n=(size_t)e.size;uint8_t*p=malloc(*n);C(p&&!gh_disc_read(d,i,0,p,*n));return p;}
typedef struct {gh_effect_trans_packet packets[7];unsigned count;} capture;
static gh_effect_status emit(void*context,const gh_effect_trans_packet*p){capture*c=context;C(c->count<7);c->packets[c->count++]=*p;return GH_EFFECT_OK;}
static unsigned weighted(unsigned color,const gh_effect_trans_packet*p){uint64_t n=0;for(unsigned q=0;q<p->quad_count;q++)n+=(uint64_t)color*(p->quads[q].vertices[0].rgba&255)/128;return n>255?255:(unsigned)n;}
static void check_scene_capture(const gh_effect_post_sampler *nearest,const gh_effect_post_sampler *linear)
{
 gh_effect_trans_packet packet={
  .original_callee=0x2fa620,.kind=GH_EFFECT_PASS_SCENE_CAPTURE,
  .source=GH_EFFECT_SURFACE_SCENE_COLOR,.destination=GH_EFFECT_SURFACE_3C00,
  .frame_mask=0xff000000u,.prim=0x116,.alpha=UINT64_C(0x8000000044),
  .test=0x30000,.zbuf=UINT64_C(0x13a0000f8),.tex0=UINT64_C(0x2000000664020e00),
  .writes_tex0=1,.inherited_sampler=1,.quad_count=8,
  .restores_scene_target=1,.texture_flush_after=1,.restore_zbuf=0x3a0000f8
 };
 for(unsigned strip=0;strip<8;strip++){
  gh_effect_trans_quad *quad=packet.quads+strip;
  quad->vertex_count=2;
  for(unsigned end=0;end<2;end++){
   quad->vertices[end]=(gh_effect_trans_vertex){
    .x12_4=28672+(int32_t)(strip+end)*512,.y12_4=29184+(int32_t)end*7168,
    .z=65535,.u12_4=(uint16_t)((strip+end)*1024+8),.v12_4=(uint16_t)(end*7168+4),
    .rgba=0x80808080u,.q=1
   };
  }
 }
 gh_effect_post_surface source={malloc(512*448*4),512*448*4,512*4,512,448};
 gh_effect_post_surface target={malloc(256*448*4),256*448*4,256*4,256,448};
 C(source.rgba&&target.rgba);
 for(unsigned y=0;y<448;y++)for(unsigned x=0;x<512;x++){
  uint8_t *p=source.rgba+(size_t)y*source.stride+x*4;
  p[0]=(uint8_t)x;p[1]=(uint8_t)y;p[2]=73;p[3]=44;
 }
 int row=-1;
 const gh_effect_post_sampler *states[]={nearest,linear};
 for(unsigned filter=0;filter<2;filter++){
  memset(target.rgba,0xa5,target.bytes);
  C(!gh_effect_postprocess_capture_scene(&packet,states[filter],&source,&target,28672,29184,&row));
  C(row==(int)!filter);
  for(unsigned y=0;y<448;y++)for(unsigned x=0;x<256;x++){
   const uint8_t *pixel=target.rgba+(size_t)y*target.stride+x*4;
   C(pixel[3]==0xa5);
   if(filter&&!y)C(pixel[0]==0xa5&&pixel[1]==0xa5&&pixel[2]==0xa5);
   else{
    unsigned expected=filter?((uint8_t)(y-1)+3u*(uint8_t)y)/4:(uint8_t)y;
    C(pixel[0]==(uint8_t)(x*2)&&pixel[1]==expected&&pixel[2]==73);
   }
  }
 }
 packet.quads[7].vertices[1].v12_4++;
 memset(target.rgba,0xa5,target.bytes);
 row=-1;
 C(gh_effect_postprocess_capture_scene(&packet,linear,&source,&target,28672,29184,&row)==GH_EFFECT_POST_UNSUPPORTED);
 C(row==-1&&target.rgba[256*4]==0xa5);
 free(target.rgba);free(source.rgba);
}
static void check_impact_warp(const gh_effect_post_sampler *nearest,const gh_effect_post_sampler *linear)
{
 gh_effect_trans_packet packet={
  .original_callee=0x2e9d48,.kind=GH_EFFECT_PASS_SCENE_WARP,
  .source=GH_EFFECT_SURFACE_DISPLAY_BASE0,.destination=GH_EFFECT_SURFACE_SCENE_COLOR,
  .frame_mask=0xff000000u,.prim=0x5d,.alpha=UINT64_C(0x8000000044),
  .test=0x5000c,.zbuf=UINT64_C(0x13a0000f8),.tex0=UINT64_C(0x2000000664120000),
  .writes_texa=1,.texa=0x80,.quad_count=1
 };
 const int x[]={2,0,4,4,0,0},y[]={2,0,0,4,4,0};
 packet.quads[0].vertex_count=6;
 for(unsigned i=0;i<6;i++){
  packet.quads[0].vertices[i]=(gh_effect_trans_vertex){
   .x12_4=x[i]*16,.y12_4=y[i]*16,.q=1,.t=447.0f/512.0f,.rgba=0x80808080u
  };
 }
 C(!gh_effect_postprocess_warp_source_known(&packet,linear,4,4,0,0,0));
 for(unsigned i=0;i<6;i++)packet.quads[0].vertices[i].t=448.0f/512.0f;
 C(gh_effect_postprocess_warp_source_known(&packet,nearest,4,4,0,0,1)==GH_EFFECT_POST_BOUNDS);
 C(gh_effect_postprocess_warp_source_known(&packet,linear,4,4,0,0,1)==GH_EFFECT_POST_BOUNDS);
 for(unsigned i=0;i<6;i++)packet.quads[0].vertices[i].rgba=0x00808080u;
 C(!gh_effect_postprocess_warp_source_known(&packet,linear,4,4,0,0,1));
 packet.source=GH_EFFECT_SURFACE_3C00;
 packet.tex0=UINT64_C(0x2000000620113c00);
 for(unsigned i=0;i<6;i++){
  packet.quads[0].vertices[i].t=.5f/256.0f;
  packet.quads[0].vertices[i].rgba=0x80808080u;
 }
 C(gh_effect_postprocess_warp_source_known(&packet,linear,4,4,0,0,0)==GH_EFFECT_POST_BOUNDS);
 C(!gh_effect_postprocess_warp_source_known(&packet,linear,4,4,0,0,1));
}
int main(int argc,char**argv){
 C(argc==2||argc==3);uint8_t one[4]={9,7,3,255},sample[4]={99,99,99,99};gh_effect_post_surface tiny={one,4,4,1,1};gh_effect_post_sampler linear={.tex1=0x60,.known=1,.policy=GH_EFFECT_SAMPLE_NATIVE_SEPARABLE_4BIT},nearest={.known=1,.policy=GH_EFFECT_SAMPLE_NATIVE_SEPARABLE_4BIT};
 C(!gh_effect_post_sample(&tiny,512,512,&linear,8,8,sample)&&!memcmp(one,sample,4));memset(sample,99,4);C(gh_effect_post_sample(&tiny,512,512,&linear,9,8,sample)==GH_EFFECT_POST_BOUNDS&&sample[0]==99);C(gh_effect_post_sample(&tiny,512,512,&linear,8,9,sample)==GH_EFFECT_POST_BOUNDS&&sample[0]==99);
 uint8_t four[16]={0,0,0,255,1,0,0,255,0,0,0,255,3,0,0,255};gh_effect_post_surface bilinear={four,16,8,2,2};C(!gh_effect_post_sample(&bilinear,2,2,&linear,16,16,sample)&&sample[0]==0&&sample[3]==255); /* separablefloors: floor((floor(.5)+floor(1.5))/2)=0, notsingle-round1 */
 C(!gh_effect_post_sample(&bilinear,2,2,&nearest,-1,-1,sample)&&sample[0]==3);linear.clamp=1;C(gh_effect_post_sample(&tiny,512,512,&linear,8,8,sample)==GH_EFFECT_POST_UNSUPPORTED);linear.clamp=0;
 check_scene_capture(&nearest,&linear);
 check_impact_warp(&nearest,&linear);
 gh_disc*d;C(!gh_disc_open(argv[1],&d));size_t en=(size_t)gh_disc_boot_elf_size(d);uint8_t*elf=malloc(en);C(elf&&!gh_disc_read_boot_elf(d,0,elf,en));size_t cn,rn;uint8_t*core=load(d,"etc/core.dat",&cn),*roombytes=load(d,"st1/r100.dat",&rn);gh_bundle room;C(!gh_bundle_open(roombytes,rn,&room));gh_effects_frame_globals globals={0};gh_camera_envelope envelope={0};uint32_t f;C(!gh_effects_frame_globals_from_elf(&globals,elf,en)&&!gh_game_camera_envelope_from_elf(&envelope,elf,en,&f));
 gh_game_effects*effects=calloc(1,sizeof(*effects));uint8_t*esp=calloc(1,0x100180),*emitters=calloc(1,0x26060);C(effects&&esp&&emitters&&!gh_game_effects_initialize_00305a68(effects,esp,0x100180,emitters,0x26060,core,cn,&room));gh_boot_work work={0};gh_game_mt random;gh_game_mt_seed_0030f2b0(&random,123);gh_effect_release_bindings storage={.effects=effects,.esp=esp,.esp_bytes=0x100180,.emitters=emitters,.emitter_bytes=0x26060};gh_effect_create_bindings producer={.storage=storage,.work=&work,.random=&random};gh_effect_create_call create={0};C(!gh_effect_create_ambient_begin_00305e88(&create,0)&&!gh_game_effect_create_00306528(&create,&producer)&&create.complete);
 gh_effect_emitter_view source;C(!gh_effect_create_emitter_at(effects,create.slot,&source));gh_esp_create_bindings eb={.storage=storage,.random=&random,.work=&work,.policy=GH_STATIC_BINARY32_REFERENCE,.globals=&globals,.atan_coeff=envelope.atan_coeff};gh_esp_create_call ec={0};C(!gh_esp_create_begin_00300e30(&ec,create.slot,source.serial)&&!gh_game_esp_create_00300e30(&ec,&eb)&&ec.complete);
 gh_display_state display={0};C(!gh_display_state_from_elf(&display,elf,en)&&!gh_display_static_initialize_002a48a8(&display,1,0xffff));capture packets={0};gh_effects_trans_bindings tb={.storage=storage,.work=&work,.globals=&globals,.display=&display,.policy=GH_STATIC_BINARY32_REFERENCE,.cvt_policy=GH_FRAME_CHOP_REFERENCE,.emit=emit,.context=&packets};gh_game_effects_trans trans={0};C(!gh_effects_trans_begin_002fb310(&trans)&&!gh_game_effects_trans_002fb310(&trans,&tb)&&trans.complete&&packets.count==7);
 gh_ui_order_group groups[7];size_t order[7],written,omitted;for(unsigned i=0;i<7;i++)groups[i]=packets.packets[i].order;C(!gh_ui_order_indices(groups,7,0,0,order,7,&written,&omitted)&&written==7&&!omitted);const size_t original_order[7]={2,1,0,3,5,4,6};C(!memcmp(order,original_order,sizeof(order)));
 gh_effect_post_context context={.xy_offset_x16=28672,.xy_offset_y16=29184,.visible_width=512,.visible_height=448,.colclamp=1};
 for(unsigned i=0;i<4;i++){gh_effect_post_surface*s=context.surfaces+i;s->width=i?128:512;s->height=i?128:448;s->stride=s->width*4+8;s->bytes=s->stride*s->height;s->rgba=malloc(s->bytes);C(s->rgba);}
 const int modes[4]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};
 for(unsigned mode=0;mode<4;mode++){
  C(!fesetround(modes[mode])&&!feclearexcept(FE_ALL_EXCEPT)&&!feraiseexcept(FE_INVALID));
  for(unsigned i=0;i<4;i++)memset(context.surfaces[i].rgba,0xa5,context.surfaces[i].bytes);
  gh_effect_post_surface*scene=context.surfaces;for(unsigned y=0;y<448;y++)for(unsigned x=0;x<512;x++){uint8_t*p=scene->rgba+(size_t)y*scene->stride+x*4;p[0]=32;p[1]=64;p[2]=96;p[3]=255;}
  gh_effect_post_receipt receipt;
  for(unsigned j=0;j<6;j++){const gh_effect_trans_packet*p=packets.packets+order[j];C(!gh_effect_postprocess_apply(p,&linear,&context,&receipt,&f)&&!f&&receipt.quads==p->quad_count&&receipt.restores_scene_target&&receipt.texture_flush_after&&receipt.restore_zbuf==0x3a0000f8&&fegetround()==modes[mode]&&fetestexcept(FE_ALL_EXCEPT)==FE_INVALID);}
  /* Closed-form constantinterior, independent of the raster/sampler loops:
   * threshold255>128; then source-alpha blend,25integerweights twice. */
  unsigned base[3]={32u*255/128,64u*255/128,96u*255/128},expected[3];for(unsigned k=0;k<3;k++){unsigned horizontal=weighted(base[k],packets.packets+5);expected[k]=weighted(horizontal,packets.packets+4);}
  const gh_effect_post_surface*blur=context.surfaces+GH_EFFECT_SURFACE_3B00;const uint8_t*middle=blur->rgba+56*blur->stride+64*4;for(unsigned k=0;k<3;k++)C(middle[k]==expected[k]);
  for(unsigned y=0;y<448;y++)for(unsigned x=0;x<512;x++)scene->rgba[(size_t)y*scene->stride+x*4+3]=(uint8_t)(x+y);
  C(!gh_effect_postprocess_apply(packets.packets+6,&linear,&context,&receipt,&f));
  middle=scene->rgba+224*scene->stride+256*4;for(unsigned k=0;k<3;k++){unsigned sum=expected[k]+(k+1)*32;C(middle[k]==(sum>255?255:sum));}
  for(unsigned y=0;y<448;y++)for(unsigned x=0;x<512;x++)C(scene->rgba[(size_t)y*scene->stride+x*4+3]==(uint8_t)(x+y));
  for(unsigned i=0;i<4;i++)for(unsigned y=0;y<context.surfaces[i].height;y++)for(unsigned k=0;k<8;k++)C(context.surfaces[i].rgba[(size_t)y*context.surfaces[i].stride+context.surfaces[i].width*4+k]==0xa5);
 }
 C(!fesetround(FE_TONEAREST));gh_effect_post_surface*dest=context.surfaces+GH_EFFECT_SURFACE_3900;uint8_t*saved=malloc(dest->bytes);C(saved);memcpy(saved,dest->rgba,dest->bytes);gh_effect_post_receipt receipt={.quads=99};gh_effect_post_sampler unknown={0};C(gh_effect_postprocess_apply(packets.packets+3,&unknown,&context,&receipt,&f)==GH_EFFECT_POST_UNSUPPORTED&&f==0x2e8b38&&receipt.quads==99&&!memcmp(saved,dest->rgba,dest->bytes));
 {gh_effect_trans_packet bad=packets.packets[order[0]];uint32_t snan=0x7f800001u;memcpy(&bad.quads[0].vertices[0].q,&snan,4);C(!fesetround(FE_UPWARD)&&!feclearexcept(FE_ALL_EXCEPT)&&!feraiseexcept(FE_DIVBYZERO));C(gh_effect_postprocess_apply(&bad,&linear,&context,&receipt,&f)==GH_EFFECT_POST_UNSUPPORTED&&f==0x2e9808&&!memcmp(saved,dest->rgba,dest->bytes)&&fegetround()==FE_UPWARD&&fetestexcept(FE_ALL_EXCEPT)==FE_DIVBYZERO);C(!fesetround(FE_TONEAREST));}
 context.surfaces[0].height=1;C(gh_effect_postprocess_apply(packets.packets+3,&linear,&context,&receipt,&f)==GH_EFFECT_POST_BOUNDS&&f==0x2e8b38&&receipt.quads==99&&!memcmp(saved,dest->rgba,dest->bytes));context.surfaces[0].height=448;
 gh_effect_post_surface old=context.surfaces[GH_EFFECT_SURFACE_3A00];context.surfaces[GH_EFFECT_SURFACE_3A00]=*dest;C(gh_effect_postprocess_apply(packets.packets+5,&linear,&context,&receipt,&f)==GH_EFFECT_POST_UNSUPPORTED&&f==0x2e83d8);context.surfaces[GH_EFFECT_SURFACE_3A00]=old;
 if(argc==3){
  for(unsigned i=0;i<4;i++)memset(context.surfaces[i].rgba,0xa5,context.surfaces[i].bytes);
  gh_effect_post_surface*scene=context.surfaces;const unsigned alpha[4]={100,128,129,255};for(unsigned y=0;y<448;y++)for(unsigned x=0;x<512;x++){uint8_t*p=scene->rgba+(size_t)y*scene->stride+x*4;p[0]=(uint8_t)(3*x+y);p[1]=(uint8_t)(7*y);p[2]=(uint8_t)(x^y);p[3]=(uint8_t)alpha[x&3];}
  for(unsigned j=0;j<7;j++)C(!gh_effect_postprocess_apply(packets.packets+order[j],&linear,&context,&receipt,&f));
  char path[1024];int count=snprintf(path,sizeof(path),"%s-packets.txt",argv[2]);C(count>0&&(size_t)count<sizeof(path));FILE*meta=fopen(path,"wbx");C(meta);
  for(unsigned j=0;j<7;j++){const gh_effect_trans_packet*p=packets.packets+order[j];fprintf(meta,"packet %u %u %u %u %u %llX %llX %llX %X %u\n",j,p->kind,p->source,p->destination,p->prim,(unsigned long long)p->tex0,(unsigned long long)p->test,(unsigned long long)p->alpha,p->frame_mask,p->quad_count);for(unsigned q=0;q<p->quad_count;q++){const gh_effect_trans_vertex*a=p->quads[q].vertices,*b=a+(p->quads[q].vertex_count==2?1:3);fprintf(meta,"quad %u %u %d %d %d %d %u %u %u %u %X\n",j,q,a->x12_4,a->y12_4,b->x12_4,b->y12_4,a->u12_4,a->v12_4,b->u12_4,b->v12_4,a->rgba);}}
  C(!fclose(meta));for(unsigned i=0;i<4;i++){count=snprintf(path,sizeof(path),"%s-%u.rgba",argv[2],i);C(count>0&&(size_t)count<sizeof(path));FILE*out=fopen(path,"wbx");C(out);const gh_effect_post_surface*p=context.surfaces+i;for(unsigned y=0;y<p->height;y++)C(fwrite(p->rgba+(size_t)y*p->stride,1,p->width*4,out)==p->width*4);C(!fclose(out));}
 }
 free(saved);for(unsigned i=0;i<4;i++)free(context.surfaces[i].rgba);C(!gh_effect_create_esp_abort_native(effects,ec.slot,ec.serial));gh_effect_create_forget_on_owner_destroy(effects);gh_disc_close(d);free(effects);free(esp);free(emitters);free(core);free(roombytes);free(elf);puts("PASS actualISO/ELF FX13 sevenOTpasses, constantinterior integeroracle, RGBmask/padding, inheritedsampler/positivebounds atomic, zero-weightmissingneighbors and 4FEnv; native reference notGS hardwaretrace");return 0;
}
