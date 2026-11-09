/* Independent host view of cModelScript/cMeshInfo/cMeshData layout from MIT
 * god-hand-decomp include/godhand/cModel.h; no vertex meaning guessed. */
#include "model.h"
#include <string.h>
static uint32_t u32(const unsigned char *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static uint16_t u16(const unsigned char *p){return (uint16_t)(p[0]|p[1]<<8);}
gh_model_error gh_model_open_scr(const void *raw,size_t n,gh_model_view *out){
 const unsigned char *b=raw;gh_model_view v;size_t i;gh_model_mesh mesh;
 if(!raw||!out)return GH_MODEL_ARGUMENT;
 if(n<16)return GH_MODEL_BOUNDS;
 if(memcmp(b,"scr\0",4))return GH_MODEL_UNSUPPORTED;
 v.bytes=b;v.size=n;v.mesh_count=u16(b+8);
 if(!v.mesh_count||v.mesh_count>(n-16)/4)return GH_MODEL_BOUNDS;
 for(i=0;i<v.mesh_count;i++){gh_model_error e=gh_model_mesh_at(&v,i,&mesh);if(e)return e;}
 *out=v;return GH_MODEL_OK;
}
gh_model_error gh_model_mesh_at(const gh_model_view *v,size_t i,gh_model_mesh *out){
 uint32_t offset,word;int64_t delta,data;gh_model_mesh m;const unsigned char *b;
 if(!v||!out||!v->bytes)return GH_MODEL_ARGUMENT;
 if(i>=v->mesh_count||v->size<16||i>=(v->size-16)/4)return GH_MODEL_BOUNDS;
 offset=u32(v->bytes+16+i*4);
 if(offset>v->size||v->size-offset<0x59)return GH_MODEL_BOUNDS;
 b=v->bytes+offset;word=u32(b);delta=(word&0x80000000U)?(int64_t)word-INT64_C(4294967296):(int64_t)word;
 data=(int64_t)offset+delta;
 if(data<0||(uint64_t)data>v->size||v->size-(size_t)data<10)return GH_MODEL_BOUNDS;
 m.descriptor_offset=offset;m.data_offset=(size_t)data;memcpy(m.name,b+8,8);m.name[8]=0;
 m.flags=u32(b+0x34);m.layer=b[0x43];m.alpha=b[0x58];m.data_num=u16(v->bytes+m.data_offset+8);
 *out=m;return GH_MODEL_OK;
}
