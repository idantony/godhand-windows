#include "game_player_opacity.h"
#include "disc.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <fenv.h>
#define C(x) do{if(!(x)){fprintf(stderr,"FAIL%d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t word(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
static float number(const uint8_t*p){float n;memcpy(&n,p,4);return n;}
static void put(uint8_t*p,uint32_t n){memcpy(p,&n,4);}
static void scalar(uint8_t*p,float n){memcpy(p,&n,4);}
static void fixture(uint8_t*p,uint8_t*a,gh_game_pl_camera*c){memset(p,0xa5,GH_TITLE_PLAYER_BYTES);memset(a,0x5a,0x254);put(p+0x15f4,0);put(p+0x68c,0);put(p+0x250,0x12345668);put(a+0x250,0x87654321);scalar(p+0x30,0);scalar(p+0x34,1234);scalar(p+0x38,0);scalar(c->fields+0x210,0);scalar(c->fields+0x214,-999);scalar(c->fields+0x218,0);put(c->fields+0x520,1);put(c->fields+0x528,0);}
static void only_fields(const uint8_t*before,const uint8_t*after,size_t n){for(size_t i=0;i<n;i++)if(!((i>=0x24c&&i<0x254)))C(before[i]==after[i]);}
int main(int argc,char**argv){
 C(argc==2);gh_disc*d;C(!gh_disc_open(argv[1],&d));size_t n=(size_t)gh_disc_boot_elf_size(d);uint8_t*elf=malloc(n),*backup=malloc(n);C(elf&&backup&&!gh_disc_read_boot_elf(d,0,elf,n));gh_disc_close(d);memcpy(backup,elf,n);
 gh_player_opacity_globals globals={0};C(!gh_player_opacity_globals_from_elf(&globals,elf,n));C(number(globals.bytes)==1&&number(globals.bytes+4)==6&&number(globals.bytes+8)==.3f&&number(globals.bytes+12)==2.1f);
 gh_player_opacity_globals malformed={0};C(gh_player_opacity_globals_from_elf(&malformed,elf,51)==GH_OPACITY_ARGUMENT&&!malformed.loaded);scalar(globals.bytes,2);C(gh_player_opacity_globals_from_elf(&globals,elf,n)==GH_OPACITY_BUSY&&number(globals.bytes)==2);scalar(globals.bytes,1);
 gh_game_init_state init={0};init.camera.mode=1;gh_game_camera alternate={0};init.active_camera=&alternate; /* Original138F88 reads manager mode, not active pointer. */
 gh_game_pl_camera camera={0};C(!gh_game_camera_construct_pl_00139bc8(&camera,&init.camera,GH_CAMERA_NATIVE_BINARY32_REFERENCE));
 uint8_t p[GH_TITLE_PLAYER_BYTES],a[0x254],before[GH_TITLE_PLAYER_BYTES],abefore[0x254];gh_title_player_view player={.bytes=p,.size=sizeof(p),.alias_f0=p+0x30},aux={.bytes=a,.size=sizeof(a)};
 gh_player_opacity_bindings b={.init=&init,.camera=&camera,.globals=&globals,.aux_68c=&aux,.auxiliary_binding_known=1,.policy=GH_CAMERA_NATIVE_BINARY32_REFERENCE};
 const int modes[]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};uint32_t golden[2][65]={0};unsigned cases=0;
 for(unsigned host=0;host<4;host++)for(unsigned cammode=0;cammode<2;cammode++)for(unsigned i=0;i<65;i++){
  C(!fesetround(FE_TONEAREST));fixture(p,a,&camera);put(camera.fields+0x520,cammode);put(camera.fields+0x528,2);float x=(float)i/8,z=(float)(i%5)/8;scalar(camera.fields+0x210,x);scalar(camera.fields+0x218,z);memcpy(before,p,sizeof(p));memcpy(abefore,a,sizeof(a));
  long double near=number(globals.bytes+(cammode?0:8)),far=number(globals.bytes+(cammode?4:12));long double expected=(sqrtl((long double)x*x+(long double)z*z)-near)/(far-near);if(expected<0)expected=0;if(expected>1)expected=1;
  C(!fesetround(modes[host]));C(!feclearexcept(FE_ALL_EXCEPT));C(!feraiseexcept(FE_INVALID));int raised=fetestexcept(FE_ALL_EXCEPT);gh_player_opacity_call call={0};C(!gh_player_opacity_00123160(&call,&player,&b)&&call.stage==2&&!call.frontier);C(fegetround()==modes[host]&&fetestexcept(FE_ALL_EXCEPT)==raised);
  C(!fesetround(FE_TONEAREST));C(fabsl((long double)number(p+0x24c)-expected)<.000002L);C(word(a+0x24c)==word(p+0x24c));int transparent=number(p+0x24c)<=.99f;C(word(p+0x250)==((word(before+0x250)&~16u)|(transparent?16u:0)));C(word(a+0x250)==((word(abefore+0x250)&~16u)|(transparent?16u:0)));only_fields(before,p,sizeof(p));only_fields(abefore,a,sizeof(a));
  if(!host)golden[cammode][i]=word(p+0x24c);else C(golden[cammode][i]==word(p+0x24c));cases++;
 }
 /* Explicit reference .99 test at exactly representable custom thresholds. */
 for(unsigned host=0;host<4;host++){
  fixture(p,a,&camera);gh_player_opacity_globals custom=globals;scalar(custom.bytes,0);scalar(custom.bytes+4,1);b.globals=&custom;scalar(camera.fields+0x210,.99f);C(!fesetround(modes[host]));gh_player_opacity_call call={0};C(!gh_player_opacity_00123160(&call,&player,&b));C(word(p+0x250)&16);C(!fesetround(FE_TONEAREST));
 }
 b.globals=&globals;
 /* Forced invisible does not require camera or globals, but still typed68C. */
 fixture(p,a,&camera);put(p+0x15f4,0x1000);b.init=NULL;b.camera=NULL;b.globals=NULL;gh_player_opacity_call forced={0};C(!gh_player_opacity_00123160(&forced,&player,&b)&&word(p+0x24c)==0&&(word(p+0x250)&16)&&word(a+0x24c)==0);
 b.init=&init;b.camera=&camera;b.globals=&globals;
 /* Genuine no-op mode bypasses unresolved auxiliary binding. */
 fixture(p,a,&camera);put(camera.fields+0x520,3);b.auxiliary_binding_known=0;memcpy(before,p,sizeof(p));memcpy(abefore,a,sizeof(a));gh_player_opacity_call noop={0};C(!gh_player_opacity_00123160(&noop,&player,&b)&&noop.stage==2&&!memcmp(before,p,sizeof(p))&&!memcmp(abefore,a,sizeof(a)));
 /* Unknown rawzero is not a NULL proof: preserve committed parent prefix. */
 fixture(p,a,&camera);scalar(camera.fields+0x210,3.5f);gh_player_opacity_call pending={0};C(gh_player_opacity_00123160(&pending,&player,&b)==GH_OPACITY_UNSUPPORTED&&pending.frontier==0x123194&&pending.stage==1&&number(p+0x24c)==.5f);scalar(camera.fields+0x210,999);b.auxiliary_binding_known=1;C(!gh_player_opacity_00123160(&pending,&player,&b)&&number(a+0x24c)==.5f&&number(p+0x24c)==.5f);
 /* Only explicit null may omit the mirror. Contradictory raw token rejects. */
 fixture(p,a,&camera);b.aux_68c=NULL;gh_player_opacity_call nullcall={0};C(!gh_player_opacity_00123160(&nullcall,&player,&b));put(p+0x68c,0x123400);gh_player_opacity_call conflict={0};C(gh_player_opacity_00123160(&conflict,&player,&b)==GH_OPACITY_UNSUPPORTED&&conflict.stage==1);
 b.aux_68c=&aux;
 /* mode mask false and camera0 flag2 false restore alpha1 without geometry. */
 fixture(p,a,&camera);init.camera.mode=4;b.camera=NULL;b.globals=NULL;gh_player_opacity_call inactive={0};C(!gh_player_opacity_00123160(&inactive,&player,&b)&&word(p+0x24c)==0x3f800000&&!(word(p+0x250)&16));init.camera.mode=1;b.camera=&camera;put(camera.fields+0x520,0);put(camera.fields+0x528,0);gh_player_opacity_call zero={0};C(!gh_player_opacity_00123160(&zero,&player,&b)&&word(p+0x24c)==0x3f800000);
 /* Numeric/reference/owner failures do not manufacture opacity writes. */
 fixture(p,a,&camera);b.globals=&globals;b.policy=GH_CAMERA_EXACT_UNAVAILABLE;memcpy(before,p,sizeof(p));gh_player_opacity_call exact={0};C(gh_player_opacity_00123160(&exact,&player,&b)==GH_OPACITY_UNSUPPORTED&&!memcmp(before,p,sizeof(p)));b.policy=GH_CAMERA_NATIVE_BINARY32_REFERENCE;
 scalar(camera.fields+0x210,NAN);gh_player_opacity_call nan={0};C(gh_player_opacity_00123160(&nan,&player,&b)==GH_OPACITY_NUMERIC&&!memcmp(before,p,sizeof(p)));scalar(camera.fields+0x210,2);gh_player_opacity_globals equal=globals;memcpy(equal.bytes+4,equal.bytes,4);b.globals=&equal;gh_player_opacity_call divzero={0};C(gh_player_opacity_00123160(&divzero,&player,&b)==GH_OPACITY_NUMERIC&&!memcmp(before,p,sizeof(p)));
 b.globals=&globals;camera.identity=&alternate;gh_player_opacity_call foreign={0};C(gh_player_opacity_00123160(&foreign,&player,&b)==GH_OPACITY_UNSUPPORTED&&!memcmp(before,p,sizeof(p)));C(!memcmp(backup,elf,n));C(!fesetround(FE_TONEAREST));free(backup);free(elf);
 printf("PASS actualELF opacity thresholds; %u distance/4FEnv cases; independent longdouble XZ oracle; mode/flag gates; typed68C mirror+strict/resume; poisoned neighbors/assets unchanged\n",cases);return 0;
}
