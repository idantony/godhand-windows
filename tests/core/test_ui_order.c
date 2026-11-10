#include "../../src/core/ui_order.h"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
int main(void){
 gh_ui_order_group g[]={{6,2,100},{6,5,200},{6,2,300},{0,1,400},{9,1,500},{2,5000,600}};
 size_t out[6]={99,99,99,99,99,99},n=88,skip=77;
 static const uint32_t expected[]={2,16,1024,6,13,1024,9,2,3,2};
 size_t i;for(i=0;i<10;i++)assert(gh_ui_order_bucket_count((uint32_t)i)==expected[i]);
 assert(gh_ui_order_bucket_count(10)==0);
 assert(gh_ui_order_indices(g,6,0,0,out,6,&n,&skip)==GH_UI_ORDER_OK);
 assert(n==5&&skip==1&&out[0]==3&&out[1]==5&&out[2]==1&&out[3]==2&&out[4]==0);
 /* Model independent linked-list head insertion into the two font buckets:
  * depth5 texture precedes depth2 message; newest whole message comes first. */
 assert(g[out[3]].identity==300&&g[out[4]].identity==100);
 assert(gh_ui_order_indices(g,6,0x02000000,1,out,6,&n,&skip)==GH_UI_ORDER_OK);
 assert(n==5&&skip==1&&out[0]==3&&out[1]==1&&out[4]==4);
 out[0]=99;n=88;skip=77;
 assert(gh_ui_order_indices(g,6,0,0,out,1,&n,&skip)==GH_UI_ORDER_CAPACITY);
 assert(out[0]==99&&n==88&&skip==77);
 g[0].depth=0;assert(gh_ui_order_indices(g,6,0,0,out,6,&n,&skip)==GH_UI_ORDER_UNSUPPORTED);
 assert(out[0]==99&&n==88);g[0].depth=2;g[0].bank=10;
 assert(gh_ui_order_indices(g,6,0,0,out,6,&n,&skip)==GH_UI_ORDER_UNSUPPORTED);
 assert(gh_ui_order_submit_parity(0)==0&&gh_ui_order_build_parity(0)==1);
 assert(gh_ui_order_submit_parity(UINT32_MAX)==1&&gh_ui_order_build_parity(UINT32_MAX)==0);
 puts("PASS semantic OT bank/bucket/LIFO/group/parity");return 0;
}
