#include "game_objects.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define C(x) do{if(!(x)){fprintf(stderr,"line%d %s\n",__LINE__,#x);exit(1);}}while(0)
static void put(uint8_t*p,uint32_t n){memcpy(p,&n,4);}static uint32_t get(uint8_t*p){uint32_t n;memcpy(&n,p,4);return n;}
int main(void){gh_stage_slots slots={0};gh_game_objects*m;C(!gh_game_objects_create(&slots,1,&m));uint8_t bytes[4]={1,2,3,4};int original;C(!gh_game_objects_add_pinned_001fece0(m,0x300,bytes,4,&original));uint8_t*record=NULL;for(unsigned i=0;i<slots.count;i++)if(get(slots.records+i*0x5c+0x20)==0x300)record=slots.records+i*0x5c;C(record);
 gh_object_view out;uint8_t before[0x5c];memcpy(before,record,sizeof(before));C(gh_game_objects_acquire_001fea48(m,0x301,&out)==GH_OBJECT_MISSING&&!memcmp(before,record,sizeof(before)));
 C(!gh_game_objects_acquire_001fea48(m,0x300,&out)&&out.bytes==bytes&&out.use_count==-100&&out.state==3);C(!gh_game_objects_release_reference_001ff0b0(m,0x300)&&get(record)==3);
 put(record+4,0);C(!gh_game_objects_acquire_001fea48(m,0x300,&out)&&out.use_count==1&&out.state==3);C(!gh_game_objects_release_reference_001ff0b0(m,0x300)&&get(record+4)==0&&get(record)==4);
 put(record+4,0x7fffffff);C(!gh_game_objects_acquire_001fea48(m,0x300,&out)&&out.use_count==INT32_MIN&&out.state==3);C(!gh_game_objects_release_reference_001ff0b0(m,0x300)&&get(record+4)==0x80000000&&get(record)==3);
 gh_game_objects_destroy(m);puts("PASS sameholder actual1FEA48/1FF090 refcounts signedgate/mod32/state3 and1FF0B0 decrement/state4; missingatomic no modelreadyclaim");return 0;
}
