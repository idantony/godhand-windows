/* PatchedSLUS_215.03 actual31AF70/31B560 axes and31AFE0 copy/direction.
 * Byte-sized integer reconstruction; no FPU or emulated hardware runtime. */
#include "game_pad_analog.h"
#include <string.h>
static uint32_t word(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
static int signed_byte(unsigned v){v&=255;return v<128?(int)v:(int)v-256;}
int8_t gh_pad_center_deadzone_0031af70(uint8_t value){int n=(int)value-128;return (int8_t)(n>=-63&&n<=63?0:n);}
static void stick(uint8_t x,uint8_t y,int right,int8_t*out){
 /* Inputs here are the driver's already-inverted wire bytes. */
 int a=gh_pad_center_deadzone_0031af70((uint8_t)(255u-x)),b=gh_pad_center_deadzone_0031af70(y);
 if((a>64||a< -64)&&b==0)b=signed_byte((unsigned)y+128u);
 /* Actualrightstick31B6D4 is addiu30/andiFF/sltiu71, not the
  * leftstick's signed<-64/>64 tests. Preserve the asymmetry. */
 int outside=right?(((unsigned)(b+48)&255u)>=113u):(b>64||b< -64);
 if(outside&&a==0)a=signed_byte(127u-(unsigned)x);
 out[0]=(int8_t)a;out[1]=(int8_t)b;
}
gh_pad_analog_status gh_pad_analog_evaluate_0031b560(const gh_boot_work*w,const uint8_t wire[4],uint32_t digital,gh_pad_analog_result*out){
 if(!w||!wire||!out||(digital&0xffff0000u))return GH_PAD_ANALOG_ARGUMENT;const uint8_t*p=(const uint8_t*)w;gh_pad_analog_result r;memset(&r,0,sizeof(r));r.source=w;r.digital_raw=digital;r.raw_held=digital;r.mode_word10=word(p+0x10);r.old_directions=word(p+0x180);memcpy(r.wire,wire,4);memcpy(r.previous_inverted,p+0x60,4);for(unsigned i=0;i<4;i++)r.inverted[i]=(uint8_t)(255u-r.wire[i]);
 if(r.mode_word10){if(!(digital&0xa000u)){if(p[0x62]>=177)r.raw_held|=0x8000;if(p[0x62]<80)r.raw_held|=0x2000;}if(!(r.raw_held&0x5000u)){if(p[0x63]>=177)r.raw_held|=0x1000;if(p[0x63]<80)r.raw_held|=0x4000;}}
 stick(r.inverted[2],r.inverted[3],0,r.axes);stick(r.inverted[0],r.inverted[1],1,r.axes+2);
 static const uint32_t positive[]={0x800000,0x100000,0x80000,0x10000},negative[]={0x400000,0x200000,0x40000,0x20000};
 static const uint32_t dir_positive[]={0x80,0x10,8,1},dir_negative[]={0x40,0x20,4,2};
 for(unsigned i=0;i<4;i++){int n=r.axes[i];if(n>80)r.raw_held|=positive[i];if(n< -80)r.raw_held|=negative[i];if(n>0)r.directions|=dir_positive[i];if(n<0)r.directions|=dir_negative[i];}
 r.pressed_directions=(r.directions^r.old_directions)&r.directions;memcpy(out,&r,sizeof(r));return GH_PAD_ANALOG_OK;
}
gh_pad_analog_status gh_pad_analog_commit_0031afe0(gh_boot_work*w,const gh_pad_analog_result*r){
 if(!w||!r)return GH_PAD_ANALOG_ARGUMENT;if(r->source!=w)return GH_PAD_ANALOG_STALE;gh_pad_analog_result expected;gh_pad_analog_status e=gh_pad_analog_evaluate_0031b560(w,r->wire,r->digital_raw,&expected);if(e)return e;if(memcmp(r,&expected,sizeof(expected)))return GH_PAD_ANALOG_STALE;
 uint8_t*p=(uint8_t*)w;memcpy(p+0x60,r->inverted,4);memcpy(p+0x70,r->axes,4);memcpy(p+0x1e0,p+0x60,20);memcpy(p+0x184,&r->old_directions,4);memcpy(p+0x180,&r->directions,4);memcpy(p+0x188,&r->pressed_directions,4);return GH_PAD_ANALOG_OK;
}
