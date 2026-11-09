/* Semantic recovery of actual ELF SearchData 0x31CCF0..0x31CDB8.
 * Added host bounds checks; original returns base+offset without relocation.
 * Tag comparison is case-sensitive with zero-based matching occurrence. */
#include "core_bundle.h"
#include <string.h>
static uint32_t word(const unsigned char *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
gh_bundle_error gh_bundle_open(const void *raw,size_t n,gh_bundle *out){
 const unsigned char *b=raw;uint32_t count,i;size_t directory,previous=0;
 if(!raw||!out)return GH_BUNDLE_ARGUMENT;
 if(n<4)return GH_BUNDLE_BOUNDS;
 count=word(b);if(!count||count>(n-4)/8)return GH_BUNDLE_BOUNDS;
 directory=4+(size_t)count*8;
 for(i=0;i<count;i++){
  size_t offset=word(b+4+(size_t)i*4);const unsigned char *tag=b+4+(size_t)count*4+(size_t)i*4;
  if(offset<directory||offset>n||offset<previous)return GH_BUNDLE_BOUNDS;
  if(!memchr(tag,0,4))return GH_BUNDLE_UNSUPPORTED;
  previous=offset;
 }
 out->bytes=b;out->size=n;out->count=count;return GH_BUNDLE_OK;
}
gh_bundle_error gh_bundle_at(const gh_bundle *v,uint32_t i,gh_bundle_entry *out){
 size_t start,end;if(!v||!out||!v->bytes)return GH_BUNDLE_ARGUMENT;
 if(v->size<4||v->count>(v->size-4)/8||i>=v->count)return GH_BUNDLE_BOUNDS;
 start=word(v->bytes+4+(size_t)i*4);end=i+1<v->count?word(v->bytes+8+(size_t)i*4):v->size;
 if(start>end||end>v->size)return GH_BUNDLE_BOUNDS;
 out->bytes=v->bytes+start;out->offset=start;out->size=end-start;
 memcpy(out->tag,v->bytes+4+(size_t)v->count*4+(size_t)i*4,4);return GH_BUNDLE_OK;
}
gh_bundle_error gh_bundle_find(const gh_bundle *v,const char *tag,uint32_t occurrence,gh_bundle_entry *out){
 uint32_t i;size_t n;
 if(!v||!tag||!out)return GH_BUNDLE_ARGUMENT;
 n=strlen(tag);if(n>3)return GH_BUNDLE_UNSUPPORTED;
 for(i=0;i<v->count;i++){
  gh_bundle_entry e;gh_bundle_error error=gh_bundle_at(v,i,&e);if(error)return error;
  if(!strcmp(e.tag,tag)){if(!occurrence){*out=e;return GH_BUNDLE_OK;}occurrence--;}
 }
 return GH_BUNDLE_NOT_FOUND;
}
