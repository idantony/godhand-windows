#include "ui_order.h"
static const uint16_t counts[10]={2,16,1024,6,13,1024,9,2,3,2};
uint32_t gh_ui_order_bucket_count(uint32_t b){return b<10?counts[b]:0;}
uint32_t gh_ui_order_submit_parity(uint32_t f){return f&1u;}
uint32_t gh_ui_order_build_parity(uint32_t f){return (f+1u)&1u;}
static int linked(uint32_t b,uint32_t flags,uint32_t mode){
 return !(b==9&&mode!=1)&&!((flags&0x02000000u)&&b>=1&&b<=3);
}
static uint32_t bucket(const gh_ui_order_group *g){
 uint32_t n=counts[g->bank];return n-(g->depth<n?g->depth:n);
}
static int before(const gh_ui_order_group *g,size_t a,size_t b){
 uint32_t x,y;
 if(g[a].bank!=g[b].bank)return g[a].bank<g[b].bank;
 x=bucket(g+a);y=bucket(g+b);
 return x!=y?x<y:a>b;
}
gh_ui_order_status gh_ui_order_indices(const gh_ui_order_group *g,size_t n,
 uint32_t flags,uint32_t mode,size_t *out,size_t cap,size_t *written,size_t *omitted){
 size_t i,j,k=0,skip=0,t;
 if(!written||!omitted||(n&&!g))return GH_UI_ORDER_ARGUMENT;
 for(i=0;i<n;i++){
  if(g[i].bank>=10||g[i].depth==0)return GH_UI_ORDER_UNSUPPORTED;
  if(linked(g[i].bank,flags,mode))k++;else skip++;
 }
 if(k>cap)return GH_UI_ORDER_CAPACITY;
 if(k&&!out)return GH_UI_ORDER_ARGUMENT;
 k=0;
 for(i=0;i<n;i++)if(linked(g[i].bank,flags,mode)){
  j=k;while(j&&before(g,i,out[j-1])){out[j]=out[j-1];j--;}
  t=i;out[j]=t;k++;
 }
 *written=k;*omitted=skip;return GH_UI_ORDER_OK;
}
