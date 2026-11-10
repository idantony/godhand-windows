#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>
#include <vector>
#include <memory>
#include <new>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <algorithm>
#include "model_renderer.h"
template<class T>struct Owned{T*p=nullptr;~Owned(){if(p)p->Release();}T**out(){return &p;}Owned()=default;Owned(const Owned&)=delete;};
struct Material{uint64_t identity;unsigned width,height;Owned<ID3D11Texture2D> image;Owned<ID3D11ShaderResourceView> view;};
struct Vertex{int32_t xyzd[4];float stq[4];uint32_t rgba[4];};
static_assert(sizeof(Vertex)==48,"structured shader input");
struct Parameters{uint32_t surface[4],state[4],blend[4],texture[4];};
struct gh_model_renderer{
 unsigned width,height;DWORD thread;bool begun=false;
 Owned<ID3D11Device> device;Owned<ID3D11DeviceContext> context;Owned<ID3D11ComputeShader> shader;
 Owned<ID3D11Buffer> parameters;Owned<ID3D11Texture2D> color,depth,color_readback,depth_readback;
 Owned<ID3D11UnorderedAccessView> color_target,depth_target;
 std::vector<std::unique_ptr<Material>> materials;
};
#define GPU(call) do{HRESULT model_gpu_hr=(call);if(FAILED(model_gpu_hr)){std::fprintf(stderr,"model GPU failure %d %08lx\n",__LINE__,(unsigned long)model_gpu_hr);return GH_MODEL_GPU_FAILURE;}}while(0)
/* Authored bounded reference raster, independently tested against CPU math.
 * Integer packed format/GS register facts are documented in private evidence.
 * No PCSX2 renderer source or shader is included or linked. */
static const char *shader=R"(
struct Vertex{int4 xyzd;float4 stq;uint4 rgba;};
Texture2D<uint4> image:register(t0);StructuredBuffer<Vertex> vertices:register(t1);StructuredBuffer<uint> indices:register(t2);
RWTexture2D<uint4> colorTarget:register(u0);RWTexture2D<uint> depthTarget:register(u1);
cbuffer Parameters:register(b0){uint4 surface;uint4 state;uint4 blend;uint4 dimensions;};
// PackedXY-offset is at most16 bits in magnitude; pixel differences are
// below73728 on this512x448 surface. The16-bit partial products therefore
// have no unsigned intermediate overflow. Keep exact signed64 coverage using
// two32-bit words, without requiring shader-model6/int64 or doubles.
uint2 wide_negate(uint2 v){uint low=0u-v.x;return uint2(low,~v.y+(low==0?1u:0u));}
uint2 wide_product(int a,int b){
 uint ua=uint(abs(a)),ub=uint(abs(b));uint low=(ua&65535u)*(ub&65535u);
 uint middle=(ua>>16)*(ub&65535u)+(ub>>16)*(ua&65535u),extra=middle<<16;
 uint result=low+extra,high=(ua>>16)*(ub>>16)+(middle>>16)+(result<low?1u:0u);
 uint2 value=uint2(result,high);return (a<0)!=(b<0)?wide_negate(value):value;
}
uint2 edge(int2 a,int2 b,int2 p){int2 d=b-a,v=p-a;uint2 x=wide_product(d.x,v.y),y=wide_product(d.y,v.x);return uint2(x.x-y.x,x.y-y.y-(x.x<y.x?1u:0u));}
bool wide_zero(uint2 v){return v.x==0&&v.y==0;}
bool wide_negative(uint2 v){return (v.y&0x80000000u)!=0;}
bool inclusive(int2 a,int2 b){int2 d=b-a;return d.y<0||(d.y==0&&d.x>0);}
float interpolate(float a,float b,float c,float3 w){precise float x=(b-a)*w.y;precise float y=(c-a)*w.z;precise float v=a+x;v=v+y;return v;}
int4 texel(int x,int y){return int4(image.Load(int3(x&int(dimensions.x-1),y&int(dimensions.y-1),0)));}
int4 linear16(int4 a,int4 b,int f){return a+(((b-a)*f)>>4);}
int4 filtered_texel(float s,float t){
 int u=int(s*float(dimensions.x)*65536.0),v=int(t*float(dimensions.y)*65536.0);
 if(dimensions.z==0)return texel(u>>16,v>>16);
 u-=32768;v-=32768;int x=u>>16,y=v>>16,fu=(u>>12)&15,fv=(v>>12)&15;
 return linear16(linear16(texel(x,y),texel(x+1,y),fu),linear16(texel(x,y+1),texel(x+1,y+1),fu),fv);
}
bool alpha_pass(uint a){uint test=state.x;if((test&1)==0)return true;uint ref=(test>>4)&255,mode=(test>>1)&7;
 if(mode==0)return false;if(mode==1)return true;if(mode==2)return a<ref;if(mode==3)return a<=ref;
 if(mode==4)return a==ref;if(mode==5)return a>=ref;if(mode==6)return a>ref;return a!=ref;
}
int3 select_color(uint selector,int3 source,int3 destination){return selector==0?source:(selector==1?destination:int3(0,0,0));}
[numthreads(8,8,1)]void main(uint3 tid:SV_DispatchThreadID){
 uint2 p=tid.xy;if(p.x>=surface.x||p.y>=surface.y)return;int2 pixelPosition=int2(p)*16;
 uint4 destination=colorTarget[p];uint oldZ=depthTarget[p];
 [loop]for(uint i=0;i<surface.z;i+=3){Vertex a=vertices[indices[i]],b=vertices[indices[i+1]],c=vertices[indices[i+2]];
  uint2 area=edge(a.xyzd.xy,b.xyzd.xy,c.xyzd.xy);if(wide_zero(area))continue;
  if(wide_negative(area)){Vertex tmp=b;b=c;c=tmp;area=wide_negate(area);}
  uint2 e0=edge(b.xyzd.xy,c.xyzd.xy,pixelPosition),e1=edge(c.xyzd.xy,a.xyzd.xy,pixelPosition),e2=edge(a.xyzd.xy,b.xyzd.xy,pixelPosition);
  if(wide_negative(e0)||wide_negative(e1)||wide_negative(e2)||(wide_zero(e0)&&!inclusive(b.xyzd.xy,c.xyzd.xy))||(wide_zero(e1)&&!inclusive(c.xyzd.xy,a.xyzd.xy))||(wide_zero(e2)&&!inclusive(a.xyzd.xy,b.xyzd.xy)))continue;
  // A triangle in a65535x65535 box has area below2^32. For an inside
  // sample each edge is in[0,area], so its high word is also zero. Convert
  // the exact unsigned low words once, preserving the existing float profile.
  precise float3 w=float3(e0.x,e1.x,e2.x)/float(area.x);
  uint z=uint(interpolate(float(a.xyzd.z),float(b.xyzd.z),float(c.xyzd.z),w));
  uint zmode=(state.x>>17)&3;
  if((state.x&65536)!=0&&(zmode==0||(zmode==2&&z<oldZ)||(zmode==3&&z<=oldZ)))continue;
  if((state.x&16384)!=0&&((destination.a>>7)&1)!=((state.x>>15)&1))continue;
  precise float q=interpolate(a.stq.z,b.stq.z,c.stq.z,w);
  precise float s=interpolate(a.stq.x,b.stq.x,c.stq.x,w)/q;
  precise float t=interpolate(a.stq.y,b.stq.y,c.stq.y,w)/q;
  int4 tex=filtered_texel(s,t),tint;
  [unroll]for(uint channel=0;channel<4;channel++)tint[channel]=int(interpolate(float(a.rgba[channel]),float(b.rgba[channel]),float(c.rgba[channel]),w));
  int4 source;
  if(((state.w>>16)&3)==3){source.rgb=min(int3(255,255,255),((tex.rgb*tint.rgb)>>7)+tint.a);source.a=tex.a;}
  else source=min(int4(255,255,255,255),(tex*tint)>>7);
  bool alpha_ok=alpha_pass(uint(source.a));uint afail=(state.x>>12)&3;
  if(!alpha_ok&&afail==0)continue;bool writeColor=alpha_ok||afail!=2,writeZ=(alpha_ok||afail==2)&&state.y!=0;
  if(writeColor){
   if((surface.w&32)!=0){int fog=clamp(int(interpolate(float(a.xyzd.w),float(b.xyzd.w),float(c.xyzd.w),w)),0,255);
    int3 fc=int3(state.z&255,(state.z>>8)&255,(state.z>>16)&255);
    source.rgb=(fc*(256-fog)+source.rgb*fog)>>8;}
   int3 rgb=source.rgb;
   if((surface.w&64)!=0&&((state.w&1)==0||(source.a&128)!=0)){
    int3 ca=select_color(blend.x,source.rgb,int3(destination.rgb)),cb=select_color(blend.y,source.rgb,int3(destination.rgb));
    int factor=blend.z==0?source.a:(blend.z==1?int(destination.a):int(dimensions.w));
    rgb=(((ca-cb)*factor)>>7)+select_color(blend.w,source.rgb,int3(destination.rgb));}
   destination.rgb=uint3(clamp(rgb,0,255));
   if(alpha_ok||afail!=3)destination.a=uint(source.a)|((state.w>>1)&128);
  }
  if(writeZ)oldZ=z;
 }
 colorTarget[p]=destination;depthTarget[p]=oldZ;
})";
extern "C" void gh_model_renderer_destroy(gh_model_renderer*r){delete r;}
extern "C" gh_model_gpu_error gh_model_renderer_create(unsigned width,unsigned height,gh_model_renderer**out){
 if(!out)return GH_MODEL_GPU_ARGUMENT;*out=nullptr;if(width!=512||height!=448)return GH_MODEL_GPU_UNSUPPORTED;
 auto r=std::unique_ptr<gh_model_renderer>(new(std::nothrow)gh_model_renderer);if(!r)return GH_MODEL_GPU_MEMORY;
 r->width=width;r->height=height;r->thread=GetCurrentThreadId();Owned<IDXGIFactory1> factory;GPU(CreateDXGIFactory1(__uuidof(IDXGIFactory1),(void**)factory.out()));
 Owned<IDXGIAdapter1> adapter;for(UINT i=0;;i++){IDXGIAdapter1*candidate=nullptr;HRESULT h=factory.p->EnumAdapters1(i,&candidate);if(h==DXGI_ERROR_NOT_FOUND)break;GPU(h);
  DXGI_ADAPTER_DESC1 desc={};candidate->GetDesc1(&desc);if(!(desc.Flags&DXGI_ADAPTER_FLAG_SOFTWARE)){adapter.p=candidate;break;}candidate->Release();}
 if(!adapter.p)return GH_MODEL_GPU_FAILURE;
 GPU(D3D11CreateDevice(adapter.p,D3D_DRIVER_TYPE_UNKNOWN,nullptr,0,nullptr,0,D3D11_SDK_VERSION,r->device.out(),nullptr,r->context.out()));
 Owned<ID3DBlob> code,errors;HRESULT compiled=D3DCompile(shader,std::strlen(shader),nullptr,nullptr,nullptr,"main","cs_5_0",D3DCOMPILE_ENABLE_STRICTNESS|D3DCOMPILE_IEEE_STRICTNESS,0,code.out(),errors.out());
 if(FAILED(compiled)){if(errors.p)std::fprintf(stderr,"%s",(const char*)errors.p->GetBufferPointer());return GH_MODEL_GPU_FAILURE;}
 GPU(r->device.p->CreateComputeShader(code.p->GetBufferPointer(),code.p->GetBufferSize(),nullptr,r->shader.out()));
 D3D11_BUFFER_DESC bd={};bd.ByteWidth=sizeof(Parameters);bd.Usage=D3D11_USAGE_DEFAULT;bd.BindFlags=D3D11_BIND_CONSTANT_BUFFER;GPU(r->device.p->CreateBuffer(&bd,nullptr,r->parameters.out()));
 D3D11_TEXTURE2D_DESC td={};td.Width=width;td.Height=height;td.MipLevels=1;td.ArraySize=1;td.SampleDesc.Count=1;td.Usage=D3D11_USAGE_DEFAULT;td.BindFlags=D3D11_BIND_UNORDERED_ACCESS;
 td.Format=DXGI_FORMAT_R8G8B8A8_UINT;GPU(r->device.p->CreateTexture2D(&td,nullptr,r->color.out()));GPU(r->device.p->CreateUnorderedAccessView(r->color.p,nullptr,r->color_target.out()));
 td.Format=DXGI_FORMAT_R32_UINT;GPU(r->device.p->CreateTexture2D(&td,nullptr,r->depth.out()));GPU(r->device.p->CreateUnorderedAccessView(r->depth.p,nullptr,r->depth_target.out()));
 td.Usage=D3D11_USAGE_STAGING;td.BindFlags=0;td.CPUAccessFlags=D3D11_CPU_ACCESS_READ;GPU(r->device.p->CreateTexture2D(&td,nullptr,r->depth_readback.out()));
 td.Format=DXGI_FORMAT_R8G8B8A8_UINT;GPU(r->device.p->CreateTexture2D(&td,nullptr,r->color_readback.out()));*out=r.release();return GH_MODEL_GPU_OK;
}
extern "C" void gh_model_renderer_reset_resources(gh_model_renderer*r){if(r&&r->thread==GetCurrentThreadId())r->materials.clear();}
extern "C" gh_model_gpu_error gh_model_renderer_upload(gh_model_renderer*r,const gh_stage_material*m,uint32_t*out){
 if(!r||!m||!out||r->thread!=GetCurrentThreadId()||!m->source||!m->rgba_gs||!m->cache_id)return GH_MODEL_GPU_ARGUMENT;
 unsigned w=m->width,h=m->height;if(!w||!h||w>512||h>512||(w&(w-1))||(h&(h-1))||m->rgba_bytes!=(size_t)w*h*4||m->draw_tcc!=1||(m->draw_tfx!=0&&m->draw_tfx!=3)||(m->psm!=19&&m->psm!=20))return GH_MODEL_GPU_UNSUPPORTED;
 for(size_t i=0;i<r->materials.size();i++)if(r->materials[i]->identity==m->cache_id){*out=(uint32_t)i+1;return GH_MODEL_GPU_OK;}
 if(r->materials.size()>=UINT32_MAX)return GH_MODEL_GPU_MEMORY;
 auto material=std::unique_ptr<Material>(new(std::nothrow)Material);if(!material)return GH_MODEL_GPU_MEMORY;material->identity=m->cache_id;material->width=w;material->height=h;
 D3D11_TEXTURE2D_DESC td={};td.Width=w;td.Height=h;td.MipLevels=1;td.ArraySize=1;td.SampleDesc.Count=1;td.Format=DXGI_FORMAT_R8G8B8A8_UINT;td.Usage=D3D11_USAGE_IMMUTABLE;td.BindFlags=D3D11_BIND_SHADER_RESOURCE;
 D3D11_SUBRESOURCE_DATA initial={m->rgba_gs,w*4,0};GPU(r->device.p->CreateTexture2D(&td,&initial,material->image.out()));GPU(r->device.p->CreateShaderResourceView(material->image.p,nullptr,material->view.out()));
 try{r->materials.push_back(std::move(material));}catch(const std::bad_alloc&){return GH_MODEL_GPU_MEMORY;}*out=(uint32_t)r->materials.size();return GH_MODEL_GPU_OK;
}
extern "C" gh_model_gpu_error gh_model_renderer_begin(gh_model_renderer*r,uint32_t rgb,uint32_t z){
 if(!r||r->thread!=GetCurrentThreadId())return GH_MODEL_GPU_ARGUMENT;if(z>65535)return GH_MODEL_GPU_UNSUPPORTED;
 UINT color[4]={rgb&255,(rgb>>8)&255,(rgb>>16)&255,0},depth[4]={z,0,0,0};
 r->context.p->ClearUnorderedAccessViewUint(r->color_target.p,color);r->context.p->ClearUnorderedAccessViewUint(r->depth_target.p,depth);r->begun=true;return GH_MODEL_GPU_OK;
}
static gh_model_gpu_error structured(gh_model_renderer*r,const void*data,UINT stride,UINT count,Owned<ID3D11Buffer>&buffer,Owned<ID3D11ShaderResourceView>&view){
 D3D11_BUFFER_DESC bd={};bd.ByteWidth=stride*count;bd.Usage=D3D11_USAGE_IMMUTABLE;bd.BindFlags=D3D11_BIND_SHADER_RESOURCE;bd.MiscFlags=D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;bd.StructureByteStride=stride;
 D3D11_SUBRESOURCE_DATA initial={data,0,0};GPU(r->device.p->CreateBuffer(&bd,&initial,buffer.out()));D3D11_SHADER_RESOURCE_VIEW_DESC vd={};vd.Format=DXGI_FORMAT_UNKNOWN;vd.ViewDimension=D3D11_SRV_DIMENSION_BUFFER;vd.Buffer.NumElements=count;GPU(r->device.p->CreateShaderResourceView(buffer.p,&vd,view.out()));return GH_MODEL_GPU_OK;
}
extern "C" gh_model_gpu_error gh_model_renderer_draw(gh_model_renderer*r,const gh_model_draw_state*s,uint32_t texture,const gh_static_gs_vertex*input,size_t count,const uint32_t*indices,size_t n){
 if(!r||!s||!input||!indices||!r->begun||r->thread!=GetCurrentThreadId()||!texture||texture>r->materials.size()||count>1024||n>4096||!count||n%3)return GH_MODEL_GPU_ARGUMENT;
 unsigned prim=s->primitive&7;if((prim!=3&&prim!=4&&prim!=5)||!(s->primitive&8)||!(s->primitive&16)||(s->primitive&~0x7fu)||!s->native_float_reference||s->frame_psm||s->zbuf_psm!=58||s->dthe||s->colclamp!=1||s->clamp||s->fba>1||s->pabe>1||s->z_write>1||s->xy_offset_x16>65535||s->xy_offset_y16>65535||(s->texture_function!=0&&s->texture_function!=3))return GH_MODEL_GPU_UNSUPPORTED;
 unsigned mag=(unsigned)((s->tex1>>5)&1),min=(unsigned)((s->tex1>>6)&7);
 bool same_filter=mag?(min==1||min==4||min==5):(min==0||min==2||min==3);
 /* MXL0 clamps all mip choices to level0. Only equal base min/mag filters
  * are supported, so no missing LOD-dependent filter switch is concealed. */
 if(((s->tex1>>2)&7)!=0||!same_filter||(s->test&~UINT64_C(0x7ffff))||((s->alpha>>0)&3)>2||((s->alpha>>2)&3)>2||((s->alpha>>4)&3)>2||((s->alpha>>6)&3)>2)return GH_MODEL_GPU_UNSUPPORTED;
 std::vector<uint8_t> referenced;try{referenced.resize(count,0);}catch(const std::bad_alloc&){return GH_MODEL_GPU_MEMORY;}
 for(size_t i=0;i<n;i++){if(indices[i]>=count)return GH_MODEL_GPU_ARGUMENT;referenced[indices[i]]=1;}if(!n)return GH_MODEL_GPU_OK;
 std::vector<Vertex> vertices;try{vertices.resize(count);}catch(const std::bad_alloc&){return GH_MODEL_GPU_MEMORY;}
 for(size_t i=0;i<count;i++){const auto&v=input[i];auto&out=vertices[i];
  // ADC/CLIP can leave unused carry vertices in a real batch. Their Q/XYZ
  // values do not enter any raster primitive, so do not consume them.
  if(!referenced[i])continue;
  out.xyzd[0]=(int32_t)((uint32_t)v.xyz_fixed4[0]&65535)-(int32_t)s->xy_offset_x16;out.xyzd[1]=(int32_t)((uint32_t)v.xyz_fixed4[1]&65535)-(int32_t)s->xy_offset_y16;
  uint32_t z=((uint32_t)v.xyz_fixed4[2]>>4)&0xffffff;if(z>65535)return GH_MODEL_GPU_UNSUPPORTED;
  out.xyzd[2]=(int32_t)z;out.xyzd[3]=(v.fog_fixed4>>4)&255;
  if(!std::isfinite(v.stq[2])||v.stq[2]<0.000001f||v.stq[2]>1000000)return GH_MODEL_GPU_UNSUPPORTED;
  for(unsigned c=0;c<3;c++){if(!std::isfinite(v.stq[c])||(c<2&&std::fabs((double)v.stq[c]/v.stq[2])>16))return GH_MODEL_GPU_UNSUPPORTED;out.stq[c]=v.stq[c];}
  for(unsigned c=0;c<4;c++){if(v.rgba[c]>255)return GH_MODEL_GPU_UNSUPPORTED;out.rgba[c]=v.rgba[c];}
 }
 Owned<ID3D11Buffer> vb,ib;Owned<ID3D11ShaderResourceView> vv,iv;gh_model_gpu_error e=structured(r,vertices.data(),sizeof(Vertex),(UINT)count,vb,vv);if(e)return e;e=structured(r,indices,4,(UINT)n,ib,iv);if(e)return e;
 const Material&m=*r->materials[texture-1];Parameters p={{r->width,r->height,(uint32_t)n,s->primitive},{(uint32_t)s->test,s->z_write,s->fog_rgb,(uint32_t)s->pabe|((uint32_t)s->fba<<8)|((uint32_t)s->texture_function<<16)},
  {(uint32_t)s->alpha&3,((uint32_t)s->alpha>>2)&3,((uint32_t)s->alpha>>4)&3,((uint32_t)s->alpha>>6)&3},{m.width,m.height,(uint32_t)((s->tex1>>5)&1),(uint32_t)(s->alpha>>32)&255}};
 r->context.p->UpdateSubresource(r->parameters.p,0,nullptr,&p,0,0);ID3D11Buffer*constant=r->parameters.p;ID3D11ShaderResourceView*views[]={m.view.p,vv.p,iv.p};ID3D11UnorderedAccessView*targets[]={r->color_target.p,r->depth_target.p};
 r->context.p->CSSetShader(r->shader.p,nullptr,0);r->context.p->CSSetConstantBuffers(0,1,&constant);r->context.p->CSSetShaderResources(0,3,views);r->context.p->CSSetUnorderedAccessViews(0,2,targets,nullptr);r->context.p->Dispatch((r->width+7)/8,(r->height+7)/8,1);
 ID3D11ShaderResourceView*emptyViews[3]={};ID3D11UnorderedAccessView*emptyTargets[2]={};r->context.p->CSSetShaderResources(0,3,emptyViews);r->context.p->CSSetUnorderedAccessViews(0,2,emptyTargets,nullptr);return GH_MODEL_GPU_OK;
}
extern "C" gh_model_gpu_error gh_model_renderer_readback(gh_model_renderer*r,uint8_t*rgba,size_t bytes,uint32_t*z,size_t count){
 if(!r||!rgba||!r->begun||r->thread!=GetCurrentThreadId()||bytes!=(size_t)r->width*r->height*4||(z&&count!=(size_t)r->width*r->height))return GH_MODEL_GPU_ARGUMENT;
 r->context.p->CopyResource(r->color_readback.p,r->color.p);D3D11_MAPPED_SUBRESOURCE map={};GPU(r->context.p->Map(r->color_readback.p,0,D3D11_MAP_READ,0,&map));
 for(unsigned y=0;y<r->height;y++)std::memcpy(rgba+(size_t)y*r->width*4,(const uint8_t*)map.pData+(size_t)y*map.RowPitch,r->width*4);r->context.p->Unmap(r->color_readback.p,0);
 if(z){r->context.p->CopyResource(r->depth_readback.p,r->depth.p);GPU(r->context.p->Map(r->depth_readback.p,0,D3D11_MAP_READ,0,&map));
  for(unsigned y=0;y<r->height;y++)std::memcpy(z+(size_t)y*r->width,(const uint8_t*)map.pData+(size_t)y*map.RowPitch,r->width*4);r->context.p->Unmap(r->depth_readback.p,0);}
 return GH_MODEL_GPU_OK;
}
