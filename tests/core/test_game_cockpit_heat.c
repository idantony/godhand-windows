#include "game_cockpit.h"
#include "disc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fenv.h>
#define C(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t word(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
static void put(uint8_t*p,uint32_t n){memcpy(p,&n,4);}
static gh_id_entry*entry(gh_game_cockpit*s,unsigned field){gh_id_entry*e;C(!gh_game_cockpit_entry_at(s,0x460+field,&e));return e;}
static uint8_t*va_file(uint8_t*e,size_t n,uint32_t va){uint32_t off=word(e+28);unsigned stride=e[42]|(unsigned)e[43]<<8,count=e[44]|(unsigned)e[45]<<8;for(unsigned i=0;i<count;i++){uint8_t*p=e+off+(size_t)i*stride;uint32_t v=word(p+8),f=word(p+16),o=word(p+4);if(word(p)==1&&va>=v&&(uint64_t)va+4<=(uint64_t)v+f){C(o<=n&&f<=n-o);return e+o+va-v;}}return NULL;}
int main(int argc,char**argv){
 C(argc==2);gh_disc*d;C(!gh_disc_open(argv[1],&d));size_t i;gh_disc_entry de;C(!gh_disc_find(d,"etc/core_usa.bin",&i)&&!gh_disc_entry_at(d,i,&de));size_t ln=(size_t)de.size;uint8_t*language=malloc(ln);C(language&&!gh_disc_read(d,i,0,language,ln));size_t en=(size_t)gh_disc_boot_elf_size(d);uint8_t*elf=malloc(en),*original=malloc(en);C(elf&&original&&!gh_disc_read_boot_elf(d,0,elf,en));memcpy(original,elf,en);gh_disc_close(d);
 gh_bundle bundle;gh_bundle_entry idd;C(!gh_bundle_open(language,ln,&bundle)&&!gh_id_find_language_data(&bundle,&idd));uint8_t*asset_copy=malloc(ln);C(asset_copy);memcpy(asset_copy,language,ln);
 const int modes[]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};
 for(unsigned mode=0;mode<4;mode++){
  gh_id_state*m=calloc(1,sizeof(*m));C(m&&!gh_id_system_init_002acc70(m,&idd));gh_id_runtime*r;C(!gh_id_runtime_create(m,1,&r)&&!gh_id_runtime_set_path_policy(r,GH_ID_PATH_REFERENCE_NEAREST));gh_game_init_state*init=malloc(sizeof(*init));C(init);gh_game_init_state_initialize(init);put(init->heat,0x42f00000);/* Actual cold maximum120/current0/mode0. */
  gh_boot_work work={0};uint8_t save_bytes[GH_CORE_SAVE_BYTES];gh_core_save_view save={save_bytes,sizeof(save_bytes)};C(!gh_core_save_system_init(save)&&!gh_core_save_game_init(save,&work));put(save_bytes+0x84,1);/* Actual firstplayer currentlyHP1; never replace it with100. */
  gh_game_cockpit*s;gh_game_mt_seed_0030f2b0(&init->random,1);C(!gh_game_cockpit_create(r,m,&init->random,&s)&&!gh_game_cockpit_initialize_00140c88(s,save)&&!gh_game_cockpit_room_reset_00140de0(s,save,&work,init->heat));
  gh_room_pools unused_pool={0};gh_cockpit_gameplay_bindings gameplay={.init=init,.pools=&unused_pool,.player_known=1};C(!gh_game_cockpit_bind_gameplay(s,&gameplay));gh_cockpit_frame_callbacks callbacks={.save=save};
  C(gh_game_cockpit_heat_move_00142060(s,&work,&callbacks)==GH_ID_UNSUPPORTED&&gh_game_cockpit_frontier(s)==0x142060);if(mode==3){uint8_t*shake=va_file(elf,en,0x3bd6ec);C(shake&&word(shake)==0);put(shake,0x3f800000);}C(!gh_game_cockpit_bind_frame_elf(s,elf,en));
  C(gh_game_cockpit_heat_move_00142060(s,&work,NULL)==GH_ID_UNSUPPORTED&&gh_game_cockpit_frontier(s)==0x1fac68);
  gh_id_base*b[4];const unsigned offsets[]={0x460,0x540,0x5f0,0x670};uint16_t frames[4];for(unsigned k=0;k<4;k++){C(!gh_game_cockpit_base(s,offsets[k],b+k));frames[k]=b[k]->frame;}
  float origin_y=entry(s,0xa8)->pos_y;uint8_t before[sizeof(save_bytes)],heat_before[sizeof(init->heat)];memcpy(before,save_bytes,sizeof(before));memcpy(heat_before,init->heat,sizeof(heat_before));
  C(!fesetround(modes[mode])&&!feclearexcept(FE_ALL_EXCEPT)&&!feraiseexcept(FE_DIVBYZERO));
  C(!gh_game_cockpit_heat_move_00142060(s,&work,&callbacks));
  for(unsigned k=0;k<4;k++)C(b[k]->frame==(uint16_t)(frames[k]+(b[k]->playing?1:0)));C(fegetround()==modes[mode]&&fetestexcept(FE_ALL_EXCEPT)==FE_DIVBYZERO);
  C(!memcmp(before,save_bytes,sizeof(before))&&!memcmp(heat_before,init->heat,sizeof(heat_before)));
  C(fabs((double)entry(s,0x9c)->scale_x-.01)<1e-8&&entry(s,0xa8)->pos_y==origin_y+(mode==3?5:0)&&entry(s,0xa8)->scale_x==1&&entry(s,0xd8)->scale_x==1);
  C(entry(s,0xbc)->color_base==0x80744042&&entry(s,0x1f8)->seed==1);for(unsigned k=0;k<3;k++){gh_id_entry*e=entry(s,0x1ec+k*4);C(!(e->flags&0x8000000)&&(e->color_base>>24)==128);}
  size_t size;const uint8_t*fields=gh_game_cockpit_fields(s,&size);C(size==0x23c0&&fields[0x460+0x26e]==0&&fields[0x460+0x26f]==1&&word(fields+0x460+0x94)==1);
  /* Same original global466440 remains captured despite another caller's
   * position mutation. Next actual pulse is3, no recapture from altered ID. */
  entry(s,0xa8)->pos_y=origin_y+9;C(!gh_game_cockpit_heat_move_00142060(s,&work,&callbacks));C(entry(s,0xa8)->pos_y==origin_y+(mode==3?-4:0)&&fields[0x460+0x26e]==3&&(entry(s,0xc4)->color_base>>24)==3);
  /* Genuine1FACB0 recomputes max from actual upgrade, preserves all other
   * Save bytes and never modifies playerHP/profile or Heat. */
  save_bytes[0x154]=3;memcpy(before,save_bytes,sizeof(before));C(!gh_game_cockpit_heat_move_00142060(s,&work,&callbacks));put(before+0x80,160);C(!memcmp(before,save_bytes,sizeof(before))&&entry(s,0xa8)->scale_x>1.59f&&entry(s,0xa8)->scale_x<1.61f);
  /* Original damage/heal paths really reset distinct families, retain
   * signedcurrentlife, cache94 and initiate global shake; no player write. */
  put(save_bytes+0x84,0);C(!gh_game_cockpit_heat_move_00142060(s,&work,&callbacks));C(entry(s,0xa8)->pos_y==origin_y+5&&word(fields+0x460+0x94)==0&&fields[0x460+0x1ac]==1&&entry(s,0x9c)->scale_x==0&&!(entry(s,0xb8)->flags&0x8000000));
  C(!gh_game_cockpit_heat_move_00142060(s,&work,&callbacks)&&entry(s,0xa8)->pos_y==origin_y-4);
  put(save_bytes+0x84,2);C(!gh_game_cockpit_heat_move_00142060(s,&work,&callbacks)&&entry(s,0xa8)->pos_y==origin_y+5&&word(fields+0x460+0x94)==2&&!(entry(s,0xdc)->flags&0x8000000));
  /* 0202's first real hit adds .8 to current and release baseline.
   * Compare the first smoothed bar sample against the binary64 equation. */
  uint16_t last=b[0]->frame;
  put(init->heat+4,0x3f4ccccd);put(init->heat+8,0x3f4ccccd);
  memcpy(heat_before,init->heat,sizeof(heat_before));
  C(!gh_game_cockpit_heat_move_00142060(s,&work,&callbacks)&&
    b[0]->frame==(uint16_t)(last+1)&&!memcmp(heat_before,init->heat,sizeof(heat_before)));
  C(!(entry(s,0xc4)->flags&0x8000000)&&(entry(s,0xc4)->color_base>>24)==0&&
    (entry(s,0xc0)->flags&0x8000000));
  fenv_t oracle;
  C(!feholdexcept(&oracle));
  C(fabs((double)entry(s,0xa4)->scale_x-.8/120.0*.1)<1e-9);
  C(!fesetenv(&oracle));
  C(!gh_game_cockpit_heat_move_00142060(s,&work,&callbacks)&&
    (entry(s,0xc4)->color_base>>24)==3);
  C(!feholdexcept(&oracle));
  C(fabs((double)entry(s,0xa4)->scale_x-.8/120.0*.19)<1e-9);
  C(!fesetenv(&oracle));

  /* The two ready-color branches and same owner's readiness word. */
  put(init->heat,0x43700000);put(init->heat+4,0x42f00000);put(init->heat+0xc,0x42f00000);
  put(init->heat+0x14,0x1234);
  C(!gh_game_cockpit_heat_move_00142060(s,&work,&callbacks)&&
    entry(s,0xbc)->color_base==0x804e6060&&word(init->heat+0x14)==1);
  put(init->heat+4,0x43700000);
  C(!gh_game_cockpit_heat_move_00142060(s,&work,&callbacks)&&
    entry(s,0xbc)->color_base==0x80808080);
  put(init->heat+0xc,0x43710000);
  C(!gh_game_cockpit_heat_move_00142060(s,&work,&callbacks)&&
    entry(s,0xbc)->color_base==0x80744042&&!word(init->heat+0x14));

  /* Released gauge bypasses display smoothing only; keeps canonical
   * target/smoothed fields, and casts alpha from (180-120)/(240-120). */
  put(init->heat+4,0x43340000);put(init->heat+8,0x42f00000);init->heat[0x10]=1;
  put(init->heat+0x14,0x4321);uint8_t pulse=fields[0x460+0x26e];
  C(!gh_game_cockpit_heat_move_00142060(s,&work,&callbacks)&&
    entry(s,0xa4)->scale_x==.75f&&entry(s,0xc4)->scale_x==.75f&&
    !(entry(s,0xc4)->flags&0x8000000)&&!(entry(s,0xc0)->flags&0x8000000)&&
    (entry(s,0xc4)->color_base>>24)==64&&fields[0x460+0x26e]==pulse&&
    word(fields+0x460+0x264)==0x3f400000&&word(fields+0x460+0x268)!=0x3f400000&&
    word(init->heat+0x14)==0x4321);
  uint8_t snapshot[0x23c0];memcpy(snapshot,fields,sizeof(snapshot));last=b[0]->frame;
  put(init->heat+8,0x43700000);C(gh_game_cockpit_heat_move_00142060(s,&work,&callbacks)==GH_ID_UNSUPPORTED&&
    gh_game_cockpit_frontier(s)==0x2a9d28&&b[0]->frame==last&&!memcmp(snapshot,fields,sizeof(snapshot)));
  init->heat[0x10]=0;

  C(fegetround()==modes[mode]&&fetestexcept(FE_ALL_EXCEPT)==FE_DIVBYZERO);
  /* Room reset seeds both positive ratios before the first frame. At
   * full with threshold met the original flash appears. */
  put(init->heat+4,0x43700000);put(init->heat+0xc,0x42f00000);
  C(!gh_game_cockpit_room_quit_00141628(s,save,init->heat)&&
    !gh_game_cockpit_room_reset_00140de0(s,save,&work,init->heat)&&!gh_game_cockpit_bind_gameplay(s,&gameplay));
  C(word(fields+0x460+0x264)==0x3f800000&&word(fields+0x460+0x268)==0x3f800000);
  C(!gh_game_cockpit_heat_move_00142060(s,&work,&callbacks)&&
    entry(s,0xa4)->scale_x==1&&!(entry(s,0xc0)->flags&0x8000000)&&word(init->heat+0x14)==1);
  C(fegetround()==modes[mode]);

  /* Exact max==zero fallback and signed-zero current are valid. */
  put(init->heat,0);put(init->heat+4,0x80000000);
  C(!gh_game_cockpit_room_quit_00141628(s,save,init->heat)&&
    !gh_game_cockpit_room_reset_00140de0(s,save,&work,init->heat)&&!gh_game_cockpit_bind_gameplay(s,&gameplay));
  C(!word(fields+0x460+0x264)&&!word(fields+0x460+0x268));
  C(fegetround()==modes[mode]&&!feclearexcept(FE_ALL_EXCEPT)&&!feraiseexcept(FE_DIVBYZERO));
  C(!gh_game_cockpit_heat_move_00142060(s,&work,&callbacks));
  C(fegetround()==modes[mode]&&fetestexcept(FE_ALL_EXCEPT)==FE_DIVBYZERO);C(!fesetround(FE_TONEAREST));C(!gh_game_cockpit_destroy(s)&&!gh_id_runtime_destroy(r));free(init);free(m);memcpy(elf,original,en);
 }
 C(!memcmp(language,asset_copy,ln));free(asset_copy);free(original);free(elf);free(language);puts("PASS actualUSA405ID Heat142060 stablelife1/zeroHeat real warnings/pulse/slots/4IDMove/SaveMaximum; fourFEnv/damage-heal resets+shake/positive Heat smoothing, readiness, release alpha and room reset; assets unchanged");return 0;
}
