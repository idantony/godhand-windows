#include "display_state.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fenv.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);return 1;}}while(0)
int main(int argc,char**argv){
 CHECK(argc==2);FILE*f=fopen(argv[1],"rb");CHECK(f&&!fseek(f,0,SEEK_END));long n=ftell(f);CHECK(n>0);rewind(f);
 uint8_t*elf=malloc((size_t)n);CHECK(elf&&fread(elf,1,(size_t)n,f)==(size_t)n&&!fclose(f));
 gh_display_state s={0};CHECK(gh_display_state_from_elf(&s,elf,(size_t)n)==GH_DISPLAY_OK);
 CHECK(s.loaded&&!s.initialized&&s.screen_width==512&&s.screen_lines==448&&s.screen_height_bits==0x43e00000&&!s.pixel_aspect_bits&&!s.gs_y_origin);
 gh_display_state before=s;CHECK(!gh_display_static_initialize_002a48a8(&s,0,0xffff)&&!memcmp(&s,&before,sizeof(s)));
 CHECK(!gh_display_static_initialize_002a48a8(&s,1,0)&&!memcmp(&s,&before,sizeof(s)));
 fenv_t original;CHECK(!fegetenv(&original));const int modes[]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};
 for(unsigned i=0;i<4;++i){s=before;s.font_batch_count[0]=77;s.font_batch_count[1]=88;
  CHECK(!fesetround(modes[i])&&!feclearexcept(FE_ALL_EXCEPT)&&!feraiseexcept(FE_INVALID|FE_INEXACT));int flags=fetestexcept(FE_ALL_EXCEPT);
  CHECK(!gh_display_static_initialize_002a48a8(&s,1,0xffff)&&s.initialized&&s.gs_y_origin==1824&&s.gs_y_origin_bits==0x44e40000&&s.pixel_aspect_bits==0x3f6eeeef&&!s.font_batch_count[0]&&!s.font_batch_count[1]);
  CHECK(fegetround()==modes[i]&&fetestexcept(FE_ALL_EXCEPT)==flags&&s.screen_height_bits==before.screen_height_bits&&s.screen_width==before.screen_width);
 }
 CHECK(!fesetenv(&original));before=s;s.screen_lines=-1;CHECK(gh_display_static_initialize_002a48a8(&s,1,0xffff)==GH_DISPLAY_NUMERIC&&s.pixel_aspect_bits==before.pixel_aspect_bits);
 CHECK(gh_display_state_from_elf(&s,elf,51)==GH_DISPLAY_BOUNDS);free(elf);
 const uint8_t top[]={255,5,157},next[]={255,255,9};
 CHECK(gh_display_copy_base0_0030b870(top,next)==0x5041be);
 puts("PASS actual ELF display scalars/BSS plus conditional2A48A8 writes, both font counters and preserved caller FEnv");return 0;
}
