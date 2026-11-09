/* Recovered 002ABC30; upstream eaae18d0dfab1f9a0df276ae5ca1581ad326faf3,
 * src/cod/001f8550.c, MIT LICENSE.upstream. Actual patched ELF is authoritative.
 * See analysis/id-path-semantics.json for the deliberate numeric scope. */
#include "id_path.h"
#include <string.h>
#include <float.h>
_Static_assert(sizeof(float)==4 && FLT_RADIX==2 && FLT_MANT_DIG==24,"binary32 path fields");
static unsigned word16(const unsigned char *p){return (unsigned)p[0]|(unsigned)p[1]<<8;}
static int half(const unsigned char *p){unsigned n=word16(p);return n<32768?(int)n:(int)n-65536;}
static uint32_t word(const unsigned char *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static int integer(const unsigned char *p,int32_t *out)
{
    uint32_t b=word(p),e=(b>>23)&255,m=(b&0x7fffff)|0x800000;unsigned shift;
    if(!(b&0x7fffffff)){*out=0;return 1;}
    if(e<127 || e>150)return 0;
    shift=150-e;if(m&((UINT32_C(1)<<shift)-1))return 0;
    *out=(int32_t)(m>>shift);if(b>>31)*out=-*out;return 1;
}
static uint32_t root(uint32_t n)
{
    uint32_t r=0,b=UINT32_C(1)<<30;
    while(b>n)b>>=2;
    while(b){if(n>=r+b){n-=r+b;r=(r>>1)+b;}else r>>=1;b>>=2;}
    return r;
}
static uint64_t root64(uint64_t n)
{
    uint64_t r=0,b=UINT64_C(1)<<62;
    while(b>n)b>>=2;
    while(b){if(n>=r+b){n-=r+b;r=(r>>1)+b;}else r>>=1;b>>=2;}
    return r;
}
static unsigned msb(uint64_t n){unsigned k=0;while(n>>1){n>>=1;++k;}return k;}
static uint32_t sqrt_nearest(uint32_t n)
{
    unsigned k,shift;uint64_t scaled,q,mid;
    if(!n)return 0;
    k=msb(n)/2;shift=23-k;scaled=(uint64_t)n<<(2*shift);q=root64(scaled);
    mid=2*q+1;
    if(4*scaled>mid*mid || (4*scaled==mid*mid && (q&1)))++q;
    if(q==UINT64_C(0x1000000)){q>>=1;++k;}
    return (uint32_t)(k+127)<<23|((uint32_t)q&0x7fffff);
}
static uint32_t add_chop(uint32_t a,uint32_t b)
{
    unsigned ea,eb,emin,top;uint64_t sum;
    if(!a)return b;if(!b)return a;
    ea=a>>23;eb=b>>23;emin=ea<eb?ea:eb;
    /* Inputs here are sums of at most14 roots of positive integers <=2^24:
     * exponent differences <=16, so exact alignment fits uint64. */
    sum=((uint64_t)((a&0x7fffff)|0x800000)<<(ea-emin))+
        ((uint64_t)((b&0x7fffff)|0x800000)<<(eb-emin));
    top=msb(sum);
    return (uint32_t)(emin+top-23)<<23|((uint32_t)(sum>>(top-23))&0x7fffff);
}
static gh_id_error reset(gh_id_entry *e,int reference)
{
    const unsigned char *s,*p;int n,count,i;uint32_t total=0,total_bits=0;
    if(!e || !e->source)return GH_ID_ARGUMENT;
    s=e->source;n=half(s+0x6e);e->jitter[1]=0;
    if(!s[0x5d]){if(half(s+0x6c)<2 || n<2)e->flags|=0x2000;else e->flags&=~UINT32_C(0x2000);}
    e->path_length=0;count=n-3;if(count<=0)return GH_ID_OK;
    if(n>17 || e->parts[0]!=s+0x130)return GH_ID_BOUNDS;
    p=e->parts[0];
    for(i=0;i<count;++i,p+=8){
        int32_t x0,y0,x1,y1;int64_t dx,dy;uint64_t squared;uint32_t length;
        if(!integer(p+8,&x0)||!integer(p+12,&y0)||!integer(p+16,&x1)||!integer(p+20,&y1))return GH_ID_UNSUPPORTED;
        dx=(int64_t)x0-x1;dy=(int64_t)y0-y1;squared=(uint64_t)(dx*dx)+(uint64_t)(dy*dy);
        /* <=2^24 ensures the original subtract/multiply/add are all exact. */
        if(squared>UINT32_C(0x1000000))return GH_ID_UNSUPPORTED;
        if(reference){
            total_bits=add_chop(total_bits,sqrt_nearest((uint32_t)squared));
            memcpy(&e->path_length,&total_bits,4);continue;
        }
        length=root((uint32_t)squared);if((uint64_t)length*length!=squared)return GH_ID_UNSUPPORTED;
        if(total>UINT32_C(0x1000000)-length)return GH_ID_UNSUPPORTED;
        total+=length;e->path_length=(float)total; /* exactly representable */
    }
    return GH_ID_OK;
}
gh_id_error gh_id_path_reset_002abc30(gh_id_entry *e){return reset(e,0);}
gh_id_error gh_id_path_reset_reference_nearest(gh_id_entry *e){return reset(e,1);}
