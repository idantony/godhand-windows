#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>
#include <vector>
#include <memory>
#include <new>
#include <cstring>
#include <cstdio>
#include <climits>
#include "ui_text_renderer.h"
#include "../assets/font_texture.h"
template<class T>struct TPtr{T*p=nullptr;~TPtr(){if(p)p->Release();}T**out(){return&p;}TPtr()=default;TPtr(const TPtr&)=delete;};
struct TextGlyph{int rectangle[4];unsigned uv[4],info[4];};
struct gh_ui_text_renderer{
 unsigned width,height;DWORD thread;TPtr<ID3D11Device> device;TPtr<ID3D11DeviceContext> context;TPtr<ID3D11ComputeShader> shader;
 TPtr<ID3D11Texture2D> atlas,background,target,readback;TPtr<ID3D11ShaderResourceView> atlasview,backview,glyphview;
 TPtr<ID3D11UnorderedAccessView> output;TPtr<ID3D11Buffer> glyphs,constants;
};
#define TGPU(x) do{HRESULT status=(x);if(FAILED(status)){std::fprintf(stderr,"textGPU %d %08lx\n",__LINE__,(unsigned long)status);return GH_UI_GPU;}}while(0)
static const char *text_shader=R"(
Texture2DArray<uint4> atlas:register(t0);Texture2D<uint4> background:register(t1);
struct Glyph{int4 rectangle;uint4 uv;uint4 info;};StructuredBuffer<Glyph> glyphs:register(t2);
RWTexture2D<uint4> target:register(u0);cbuffer P:register(b0){uint4 dimensions;};
uint rational(uint a,uint b,uint d){uint p0=(a&65535)*(b&65535),p1=(a&65535)*(b>>16),p2=(a>>16)*(b&65535),p3=(a>>16)*(b>>16);
 uint mid=(p0>>16)+(p1&65535)+(p2&65535),lo=(p0&65535)|(mid<<16),hi=p3+(p1>>16)+(p2>>16)+(mid>>16),r=0,q=0;
 [loop]for(int bit=63;bit>=0;bit--){r=(r<<1)|(bit>=32?((hi>>(bit-32))&1):((lo>>bit)&1));if(r>=d){r-=d;if(bit<32)q|=1u<<bit;}}return q;}
[numthreads(8,8,1)]void main(uint3 tid:SV_DispatchThreadID){
 uint2 p=tid.xy;if(p.x>=dimensions.x||p.y>=dimensions.y)return;uint4 dest=background.Load(int3(p,0));int2 pixel=int2(p)*16;
 [loop]for(uint i=0;i<dimensions.z;i++){
  Glyph g=glyphs[i];if(any(pixel<g.rectangle.xy)||any(pixel>=g.rectangle.zw))continue;
  uint u=g.uv.x+rational(g.uv.z-g.uv.x,uint(pixel.x-g.rectangle.x),uint(g.rectangle.z-g.rectangle.x));
  uint v=g.uv.y+rational(g.uv.w-g.uv.y,uint(pixel.y-g.rectangle.y),uint(g.rectangle.w-g.rectangle.y));
  uint4 tex=atlas.Load(int4((u>>16)&511,(v>>16)&255,g.info.y,0));
  uint4 tint=uint4(g.info.x&255,(g.info.x>>8)&255,(g.info.x>>16)&255,g.info.x>>24);
  int alpha=min(255,int((tex.a*tint.a)>>7));if(alpha<=1)continue;
  int3 src=int3(min(uint3(255,255,255),(tex.rgb*tint.rgb)>>7));
  int3 rgb=int3(dest.rgb)+(((src-int3(dest.rgb))*alpha)>>7);dest=uint4(clamp(rgb,0,255),alpha);
 }target[p]=dest;
})";
extern "C" void gh_ui_text_destroy(gh_ui_text_renderer *r){delete r;}
extern "C" gh_ui_error gh_ui_text_create(const gh_font_slot *font,unsigned w,unsigned h,gh_ui_text_renderer **out){
 if(!font||!out)return GH_UI_ARGUMENT;*out=nullptr;if(!w||!h||w>2048||h>2048)return GH_UI_ARGUMENT;
 gh_font_transfer clut;if(gh_font_transfer_at(font,1,0,&clut)!=GH_FONT_OK)return GH_UI_UNSUPPORTED;
 for(size_t i=3;i<clut.pixel_bytes;i+=4)if(clut.pixels[i]>128)return GH_UI_UNSUPPORTED;
 std::vector<unsigned char> banks[2];try{for(auto &b:banks)b.resize(512*256*4);}catch(const std::bad_alloc&){return GH_UI_MEMORY;}
 for(unsigned i=0;i<2;i++){if(gh_font_atlas_bank_rgba(font,i,banks[i].data(),banks[i].size())!=GH_FONT_OK)return GH_UI_UNSUPPORTED;for(size_t a=3;a<banks[i].size();a+=4){if((banks[i][a]&1)&&banks[i][a]!=255)return GH_UI_UNSUPPORTED;banks[i][a]=banks[i][a]==255?128:(unsigned char)(banks[i][a]/2);}}
 std::unique_ptr<gh_ui_text_renderer> r(new(std::nothrow)gh_ui_text_renderer);if(!r)return GH_UI_MEMORY;r->width=w;r->height=h;r->thread=GetCurrentThreadId();
 TPtr<IDXGIFactory1> factory;TPtr<IDXGIAdapter1> adapter;TGPU(CreateDXGIFactory1(__uuidof(IDXGIFactory1),(void**)factory.out()));
 for(UINT i=0;;i++){IDXGIAdapter1*a=nullptr;HRESULT hr=factory.p->EnumAdapters1(i,&a);if(hr==DXGI_ERROR_NOT_FOUND)break;TGPU(hr);DXGI_ADAPTER_DESC1 d={};a->GetDesc1(&d);if(d.VendorId==0x10de&&!(d.Flags&DXGI_ADAPTER_FLAG_SOFTWARE)){adapter.p=a;break;}a->Release();}if(!adapter.p)return GH_UI_GPU;
 TGPU(D3D11CreateDevice(adapter.p,D3D_DRIVER_TYPE_UNKNOWN,nullptr,0,nullptr,0,D3D11_SDK_VERSION,r->device.out(),nullptr,r->context.out()));
 TPtr<ID3DBlob> code,errors;HRESULT compiled=D3DCompile(text_shader,strlen(text_shader),nullptr,nullptr,nullptr,"main","cs_5_0",D3DCOMPILE_ENABLE_STRICTNESS,0,code.out(),errors.out());if(FAILED(compiled)){if(errors.p)std::fprintf(stderr,"%s",(char*)errors.p->GetBufferPointer());return GH_UI_GPU;}
 TGPU(r->device.p->CreateComputeShader(code.p->GetBufferPointer(),code.p->GetBufferSize(),nullptr,r->shader.out()));
 D3D11_TEXTURE2D_DESC td={};td.Width=512;td.Height=256;td.MipLevels=1;td.ArraySize=2;td.Format=DXGI_FORMAT_R8G8B8A8_UINT;td.SampleDesc.Count=1;td.Usage=D3D11_USAGE_IMMUTABLE;td.BindFlags=D3D11_BIND_SHADER_RESOURCE;
 D3D11_SUBRESOURCE_DATA data[2]={{banks[0].data(),512*4,0},{banks[1].data(),512*4,0}};TGPU(r->device.p->CreateTexture2D(&td,data,r->atlas.out()));TGPU(r->device.p->CreateShaderResourceView(r->atlas.p,nullptr,r->atlasview.out()));
 td.Width=w;td.Height=h;td.ArraySize=1;td.Usage=D3D11_USAGE_DEFAULT;TGPU(r->device.p->CreateTexture2D(&td,nullptr,r->background.out()));TGPU(r->device.p->CreateShaderResourceView(r->background.p,nullptr,r->backview.out()));td.BindFlags=D3D11_BIND_UNORDERED_ACCESS;TGPU(r->device.p->CreateTexture2D(&td,nullptr,r->target.out()));TGPU(r->device.p->CreateUnorderedAccessView(r->target.p,nullptr,r->output.out()));td.Usage=D3D11_USAGE_STAGING;td.BindFlags=0;td.CPUAccessFlags=D3D11_CPU_ACCESS_READ;TGPU(r->device.p->CreateTexture2D(&td,nullptr,r->readback.out()));
 D3D11_BUFFER_DESC bd={};bd.ByteWidth=8192*sizeof(TextGlyph);bd.Usage=D3D11_USAGE_DEFAULT;bd.BindFlags=D3D11_BIND_SHADER_RESOURCE;bd.MiscFlags=D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;bd.StructureByteStride=sizeof(TextGlyph);TGPU(r->device.p->CreateBuffer(&bd,nullptr,r->glyphs.out()));
 D3D11_SHADER_RESOURCE_VIEW_DESC vd={};vd.Format=DXGI_FORMAT_UNKNOWN;vd.ViewDimension=D3D11_SRV_DIMENSION_BUFFER;vd.Buffer.NumElements=8192;TGPU(r->device.p->CreateShaderResourceView(r->glyphs.p,&vd,r->glyphview.out()));bd.ByteWidth=16;bd.BindFlags=D3D11_BIND_CONSTANT_BUFFER;bd.MiscFlags=0;bd.StructureByteStride=0;TGPU(r->device.p->CreateBuffer(&bd,nullptr,r->constants.out()));*out=r.release();return GH_UI_OK;
}
extern "C" gh_ui_error gh_ui_text_batch(gh_ui_text_renderer *r,const gh_ui_gs_state *s,int32_t ox,int32_t oy,const gh_id_text_glyph *input,size_t count,const unsigned char *back,unsigned char *output,size_t bytes){
 if(!r||!s||(!input&&count)||!back||!output||count>8192||GetCurrentThreadId()!=r->thread||bytes<(size_t)r->width*r->height*4)return GH_UI_ARGUMENT;
 if(s->alpha!=UINT64_C(0x8000000044)||s->test!=0x3001d||s->prim!=0x156||s->tex1||s->clamp||s->colclamp!=1||s->prmodecont!=1||s->pabe||s->fba||s->frame_mask||((s->tex0>>20)&63)!=20||((s->tex0>>26)&15)!=9||((s->tex0>>30)&15)!=8||((s->tex0>>34)&7)!=1)return GH_UI_UNSUPPORTED;
 std::vector<TextGlyph> glyphs;try{glyphs.resize(count);}catch(const std::bad_alloc&){return GH_UI_MEMORY;}
 for(size_t i=0;i<count;i++){
  const gh_font_quad &q=input[i].quad;TextGlyph &g=glyphs[i];int64_t rectangle[]={(int64_t)q.x0-ox,(int64_t)q.y0-oy,(int64_t)q.x1-ox,(int64_t)q.y1-oy};
  if(q.slot||q.palette_parity!=(input[i].code&1)||q.u1<q.u0||q.v1<q.v0||q.u1>512*16+8||q.v1>256*16+8)return GH_UI_UNSUPPORTED;
  for(unsigned j=0;j<4;j++){if(rectangle[j]<-65536||rectangle[j]>65536)return GH_UI_UNSUPPORTED;g.rectangle[j]=(int)rectangle[j];}
  if(g.rectangle[2]<=g.rectangle[0]||g.rectangle[3]<=g.rectangle[1])return GH_UI_UNSUPPORTED;
  g.uv[0]=(unsigned)q.u0<<12;g.uv[1]=(unsigned)q.v0<<12;g.uv[2]=(unsigned)q.u1<<12;g.uv[3]=(unsigned)q.v1<<12;g.info[0]=q.rgba;g.info[1]=q.palette_parity;g.info[2]=g.info[3]=0;
 }
 auto *ctx=r->context.p;if(count){D3D11_BOX box={0,0,0,(UINT)(count*sizeof(TextGlyph)),1,1};ctx->UpdateSubresource(r->glyphs.p,0,&box,glyphs.data(),0,0);}unsigned dims[]={r->width,r->height,(unsigned)count,0};ctx->UpdateSubresource(r->constants.p,0,nullptr,dims,0,0);ctx->UpdateSubresource(r->background.p,0,nullptr,back,r->width*4,0);
 ID3D11ShaderResourceView *views[]={r->atlasview.p,r->backview.p,r->glyphview.p};ctx->CSSetShader(r->shader.p,nullptr,0);ctx->CSSetShaderResources(0,3,views);ctx->CSSetConstantBuffers(0,1,&r->constants.p);ctx->CSSetUnorderedAccessViews(0,1,&r->output.p,nullptr);ctx->Dispatch((r->width+7)/8,(r->height+7)/8,1);
 ID3D11ShaderResourceView *empty[3]={};ID3D11UnorderedAccessView *nullview=nullptr;ctx->CSSetShaderResources(0,3,empty);ctx->CSSetUnorderedAccessViews(0,1,&nullview,nullptr);ctx->CopyResource(r->readback.p,r->target.p);D3D11_MAPPED_SUBRESOURCE mapped={};TGPU(ctx->Map(r->readback.p,0,D3D11_MAP_READ,0,&mapped));for(unsigned y=0;y<r->height;y++)memcpy(output+(size_t)y*r->width*4,(unsigned char*)mapped.pData+(size_t)y*mapped.RowPitch,(size_t)r->width*4);ctx->Unmap(r->readback.p,0);return GH_UI_OK;
}
