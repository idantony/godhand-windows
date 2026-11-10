#define main original_game_stage_test
#include "test_game_stage.c"
#undef main
int main(int argc,char **argv){
 CHECK(argc==2);gh_disc *disc;CHECK(!gh_disc_open(argv[1],&disc));size_t size;uint8_t *core=payload(disc,"etc/core.dat",&size);gh_disc_close(disc);
 gh_stage_textures *s;CHECK(!gh_stage_textures_create(&s)&&!gh_stage_core_textures(s,core,size));
 const uint64_t tex0=UINT64_C(0x2003800599422600);const gh_stage_material *m=NULL,*same=NULL;
 CHECK(!gh_stage_tex0_material(s,tex0,&m)&&m&&m->width==64&&m->height==64&&m->psm==20&&m->cache_pool==3&&m->source_tex0==tex0&&m->draw_tcc==1&&!m->draw_tfx);
 CHECK(!gh_stage_tex0_material(s,tex0,&same)&&m==same&&same->cache_id==m->cache_id);
 uint8_t rgba[64*64*4];CHECK(!gh_stage_texture_rgba(s,0,rgba,sizeof(rgba)));
 gh_font_slot slot={0};uint32_t offset=u32(core+4),end=u32(core+8);slot.bound=1;slot.mst=core+offset;slot.mst_size=end-offset;
 for(unsigned i=0;i<2;i++){slot.table_offset[i]=u32(slot.mst+8+i*4);slot.table_count[i]=u16(slot.mst+16+i*2);}gh_font_transfer palette;CHECK(!gh_font_transfer_at(&slot,1,0,&palette));CHECK(palette.width==16&&palette.height==16&&palette.pixel_bytes==1024);
 unsigned nontrivial=0;
 for(unsigned i=0;i<64*64;i++){
  const uint8_t *p=m->rgba_gs+i*4;CHECK(!memcmp(p,rgba+i*4,3)&&rgba[i*4+3]==(p[3]>=128?255:p[3]*2));
  /* PSMT4 selects16 indices, but CSM1 places them across the actual256-cell
   * CLUT upload. Its first16 linear source cells are not the sampled palette. */
  unsigned found=0;for(unsigned j=0;j<(unsigned)palette.width*palette.height;j++)if(!memcmp(p,palette.pixels+j*4,4))found++;CHECK(found);if(p[3]&&p[3]!=255)nontrivial++;
 }
 CHECK(nontrivial);CHECK(gh_stage_tex0_material(s,tex0^1,&same)==GH_STAGE_UNAVAILABLE&&!same);
 CHECK(gh_stage_tex0_material(s,tex0^(UINT64_C(1)<<56),&same)==GH_STAGE_UNAVAILABLE&&!same);
 /* Synthetic multi-page channel makes a changed TBW observably different.
  * Do not generalize the real64x64 shadow's harmless one-page change. */
 uint8_t *bank=malloc(end-offset);CHECK(bank);memcpy(bank,core+offset,end-offset);bank[6]=0;bank[7]=1;
 uint64_t raw;memcpy(&raw,bank+32,8);raw=(raw&~(UINT64_C(15)<<30))|(UINT64_C(8)<<30);memcpy(bank+32,&raw,8);
 uint32_t channel;CHECK(!gh_stage_texture_load(s,bank,end-offset,&channel));gh_stage_channel c;CHECK(!gh_stage_texture_channel(s,channel,&c));
 uint64_t changed=(c.tex0&~(UINT64_C(63)<<14))|(UINT64_C(8)<<14);CHECK(gh_stage_tex0_material(s,changed,&same)==GH_STAGE_UNAVAILABLE&&!same);
 CHECK(!gh_stage_texture_forget_source_range(s,core,size));CHECK(gh_stage_tex0_material(s,tex0,&same)==GH_STAGE_UNAVAILABLE&&!same);
 gh_stage_textures_destroy(s);free(bank);free(core);puts("PASS actualcore shadowTEX0 differentTBW equalalladdresses/rawGSalpha originalCLUT/cacheidentity,syntheticdifferentpagepitch reject/lifetime; no GPU proof");return 0;
}
