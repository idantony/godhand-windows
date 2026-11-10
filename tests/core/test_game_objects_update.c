#include "game_objects.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static uint32_t w(const uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
static void put(uint8_t*p,uint32_t n){memcpy(p,&n,4);}
int main(void){gh_stage_slots slots={0};gh_game_objects*s;uint32_t frontier;
 C(gh_game_objects_create(&slots,0,&s)==GH_OBJECT_OK&&gh_stage_slots_reset(&slots,NULL)==GH_STAGE_OK);uint8_t before[80*0x5c];
 /* Explicitcanonicalholder-scalarfixtures; no model/init callback orfakeIO. */
 put(slots.records+79*0x5c,5);put(slots.records+78*0x5c,6);
 C(gh_game_objects_update_001fe748(s,&frontier)==GH_OBJECT_OK&&!frontier&&w(slots.records+79*0x5c)==6&&w(slots.records+78*0x5c)==7);
 memcpy(before,slots.records,sizeof(before));C(gh_game_objects_update_001fe748(s,&frontier)==GH_OBJECT_UNSUPPORTED&&frontier==0x1ff338&&!memcmp(before,slots.records,sizeof(before)));
 C(gh_stage_slots_reset(&slots,NULL)==GH_STAGE_OK);put(slots.records+79*0x5c,5);put(slots.records,1);memcpy(before,slots.records,sizeof(before));C(gh_game_objects_update_001fe748(s,&frontier)==GH_OBJECT_UNSUPPORTED&&frontier==0x1ff238&&!memcmp(before,slots.records,sizeof(before)));
 C(gh_stage_slots_reset(&slots,NULL)==GH_STAGE_OK);uint8_t*a=slots.records,*b=a+0x5c;put(a,8);put(a+0x20,0x1234);put(a+0x4c,0xfffe);
 C(gh_game_objects_update_001fe748(s,&frontier)==GH_OBJECT_OK&&w(a)==8&&w(a+0x4c)==0xffff);
 /* Realdependencycascade needs1FF2E0 resourceRelease, cannotfakeit. */
 put(a+0x4c,0xfffe);put(b+4,1);put(b+0x4c,0x1234);memcpy(before,slots.records,sizeof(before));
 C(gh_game_objects_update_001fe748(s,&frontier)==GH_OBJECT_UNSUPPORTED&&frontier==0x1ff2e0&&!memcmp(before,slots.records,sizeof(before)));
 /* Original1FF710 quirk: twomatchingtags+twoalreadyempty slots doesNOT
  * reportlastreference, thoughbothmatchingtagsareremoved. Preserveusecount. */
 put(b+0x50,0x1234);C(gh_game_objects_update_001fe748(s,&frontier)==GH_OBJECT_OK&&w(b+0x4c)==0xffff&&w(b+0x50)==0xffff&&w(b+4)==1);
 C(gh_stage_slots_reset(&slots,NULL)==GH_STAGE_OK);put(a,8);put(a+0x4c,0xfffffffe);memcpy(before,slots.records,sizeof(before));C(gh_game_objects_update_001fe748(s,&frontier)==GH_OBJECT_OK&&!memcmp(before,slots.records,sizeof(before)));
 gh_game_objects_destroy(s);puts("PASS holderstate5/6/dependencyscalars, exactFFFEkey/1FF710quirk, atomicunportedIO/releasepreflight; nomodelreadyclaim");return 0;}
