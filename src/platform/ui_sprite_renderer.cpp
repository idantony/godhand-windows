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
#include "ui_sprite_renderer.h"
#include "ui_depth_target.h"
template<class T>struct SPtr{T*p=nullptr;~SPtr(){if(p)p->Release();}T**out(){return &p;}SPtr()=default;SPtr(const SPtr&)=delete;};
struct SpriteTexture{SPtr<ID3D11Texture2D> image;SPtr<ID3D11ShaderResourceView> view;unsigned width,height;};
struct gh_ui_sprite_renderer{
 unsigned width,height;DWORD thread;
 SPtr<ID3D11Device> device;SPtr<ID3D11DeviceContext> context;SPtr<ID3D11ComputeShader> shader;
 SPtr<ID3D11Buffer> constants;SPtr<ID3D11Texture2D> frame[2],readback;
 SPtr<ID3D11ShaderResourceView> frameview[2];SPtr<ID3D11UnorderedAccessView> target[2];
 std::vector<std::unique_ptr<SpriteTexture>> textures;
 NativeUIDepthTarget depth;
};
struct SpriteParams{int rectangle[4];unsigned st[4],info[4],dimensions[4];};
#define SGPU(x) do{HRESULT result=(x);if(FAILED(result)){std::fprintf(stderr,"sprite GPU failure %d %08lx\n",__LINE__,(unsigned long)result);return GH_UI_GPU;}}while(0)
static const char *sprite_shader=R"(
Texture2D<uint4> sourceImage:register(t0);Texture2D<uint4> previous:register(t1);
RWTexture2D<uint4> outputImage:register(u0);RWTexture2D<uint> outputDepth:register(u1);
cbuffer Parameters:register(b0){int4 rectangle;uint4 st;uint4 info;uint4 dimensions;};
uint rational(uint a,uint b,uint divisor){
 uint p0=(a&65535)*(b&65535),p1=(a&65535)*(b>>16),p2=(a>>16)*(b&65535),p3=(a>>16)*(b>>16);
 uint middle=(p0>>16)+(p1&65535)+(p2&65535);
 uint lo=(p0&65535)|(middle<<16),hi=p3+(p1>>16)+(p2>>16)+(middle>>16);uint remainder=0,quotient=0;
 [loop]for(int bit=63;bit>=0;bit--){uint value=bit>=32?((hi>>(bit-32))&1):((lo>>bit)&1);remainder=(remainder<<1)|value;
 if(remainder>=divisor){remainder-=divisor;if(bit<32)quotient|=1u<<bit;}}
 return quotient;
}
int4 texel(int x,int y){return int4(sourceImage.Load(int3(x&int(info.y-1),y&int(info.z-1),0)));}
int4 lerp4(int4 a,int4 b,int f){return a+(((b-a)*f)>>4);}
int coordinate(int a,int b,uint numerator,uint denominator){
 // Floor signed linear interpolation. Reflect descending ranges about their
 // endpoint so the 64-bit unsigned multiply/divide stays exact.
 return b>=a?a+int(rational(uint(b-a),numerator,denominator)):
  b+int(rational(uint(a-b),denominator-numerator,denominator));
}
[numthreads(8,8,1)]void main(uint3 tid:SV_DispatchThreadID){
 uint2 p=tid.xy;if(p.x>=info.w||p.y>=dimensions.x)return;uint4 destination=previous.Load(int3(p,0));int2 fixedp=int2(p)*16;
 if(any(fixedp<rectangle.xy)||any(fixedp>=rectangle.zw)){outputImage[p]=destination;return;}
 int u=coordinate(int(st.x),int(st.z),uint(fixedp.x-rectangle.x),uint(rectangle.z-rectangle.x));
 int v=coordinate(int(st.y),int(st.w),uint(fixedp.y-rectangle.y),uint(rectangle.w-rectangle.y));
 int bu=u-32768,bv=v-32768;int x=bu>>16,y=bv>>16,fu=(bu>>12)&15,fv=(bv>>12)&15;
 int4 filtered=lerp4(lerp4(texel(x,y),texel(x+1,y),fu),lerp4(texel(x,y+1),texel(x+1,y+1),fu),fv);
 uint4 tint=uint4(info.x&255,(info.x>>8)&255,(info.x>>16)&255,info.x>>24);
 int alpha=min(255,(filtered.a*int(tint.a))>>7);if(alpha<=1){outputImage[p]=destination;return;}
 int3 source=min(int3(255,255,255),(filtered.rgb*int3(tint.rgb))>>7);
 int3 rgb=int3(destination.rgb)+(((source-int3(destination.rgb))*alpha)>>7);
 outputImage[p]=uint4(clamp(rgb,0,255),alpha);if(dimensions.z)outputDepth[p]=dimensions.w;
})";
extern "C" void gh_ui_sprite_destroy(gh_ui_sprite_renderer *r){delete r;}
extern "C" gh_ui_error gh_ui_sprite_create(unsigned w,unsigned h,gh_ui_sprite_renderer **out){
 if(!out)return GH_UI_ARGUMENT;*out=nullptr;if(!w||!h||w>2048||h>2048)return GH_UI_ARGUMENT;
 std::unique_ptr<gh_ui_sprite_renderer> r(new(std::nothrow)gh_ui_sprite_renderer);if(!r)return GH_UI_MEMORY;
 r->width=w;r->height=h;r->thread=GetCurrentThreadId();SPtr<IDXGIFactory1> factory;SGPU(CreateDXGIFactory1(__uuidof(IDXGIFactory1),(void**)factory.out()));SPtr<IDXGIAdapter1> adapter;
 for(UINT i=0;;i++){IDXGIAdapter1 *candidate=nullptr;HRESULT hr=factory.p->EnumAdapters1(i,&candidate);if(hr==DXGI_ERROR_NOT_FOUND)break;SGPU(hr);DXGI_ADAPTER_DESC1 desc={};candidate->GetDesc1(&desc);if(desc.VendorId==0x10de&&!(desc.Flags&DXGI_ADAPTER_FLAG_SOFTWARE)){adapter.p=candidate;break;}candidate->Release();}
 if(!adapter.p)return GH_UI_GPU;
 SGPU(D3D11CreateDevice(adapter.p,D3D_DRIVER_TYPE_UNKNOWN,nullptr,0,nullptr,0,D3D11_SDK_VERSION,r->device.out(),nullptr,r->context.out()));
 SPtr<ID3DBlob> code,errors;HRESULT compiled=D3DCompile(sprite_shader,strlen(sprite_shader),nullptr,nullptr,nullptr,"main","cs_5_0",D3DCOMPILE_ENABLE_STRICTNESS,0,code.out(),errors.out());
 if(FAILED(compiled)){if(errors.p)std::fprintf(stderr,"%s",(char*)errors.p->GetBufferPointer());return GH_UI_GPU;}
 SGPU(r->device.p->CreateComputeShader(code.p->GetBufferPointer(),code.p->GetBufferSize(),nullptr,r->shader.out()));
 D3D11_BUFFER_DESC bd={};bd.ByteWidth=sizeof(SpriteParams);bd.Usage=D3D11_USAGE_DEFAULT;bd.BindFlags=D3D11_BIND_CONSTANT_BUFFER;SGPU(r->device.p->CreateBuffer(&bd,nullptr,r->constants.out()));
 D3D11_TEXTURE2D_DESC td={};td.Width=w;td.Height=h;td.MipLevels=1;td.ArraySize=1;td.Format=DXGI_FORMAT_R8G8B8A8_UINT;td.SampleDesc.Count=1;td.Usage=D3D11_USAGE_DEFAULT;td.BindFlags=D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_UNORDERED_ACCESS;
 for(unsigned i=0;i<2;i++){SGPU(r->device.p->CreateTexture2D(&td,nullptr,r->frame[i].out()));SGPU(r->device.p->CreateShaderResourceView(r->frame[i].p,nullptr,r->frameview[i].out()));SGPU(r->device.p->CreateUnorderedAccessView(r->frame[i].p,nullptr,r->target[i].out()));}
 td.Usage=D3D11_USAGE_STAGING;td.BindFlags=0;td.CPUAccessFlags=D3D11_CPU_ACCESS_READ;SGPU(r->device.p->CreateTexture2D(&td,nullptr,r->readback.out()));*out=r.release();return GH_UI_OK;
}
extern "C" gh_ui_error gh_ui_sprite_upload(gh_ui_sprite_renderer *r,const gh_id_texture *t,const unsigned char *rgba,size_t bytes,uint32_t *handle){
 if(!r||!t||!rgba||!handle||GetCurrentThreadId()!=r->thread)return GH_UI_ARGUMENT;
 // T32 template may have TCC0. Actual15BE44..15BE54 forces TCC1 before draw.
 bool level=t->width==256&&t->height==128&&t->tex0==UINT64_C(0x20000001e1310000);
 bool radar=t->width==128&&t->height==256&&t->tex0==GH_ID_RADAR_TEX0;
 bool t32=(t->width==512&&(t->height==256||t->height==512)&&((t->tex0>>20)&63)==19)||level||radar;
 bool logo=t->width==512&&t->height==512&&((t->tex0>>20)&63)==20&&((t->tex0>>26)&15)==9&&((t->tex0>>30)&15)==9&&((t->tex0>>51)&63)==0&&((t->tex0>>56)&31)==0;
 bool cursor=t->width==32&&t->height==32&&((t->tex0>>20)&63)==20&&((t->tex0>>26)&15)==5&&((t->tex0>>30)&15)==5&&((t->tex0>>51)&63)==0&&((t->tex0>>56)&31)==0;
 bool loading=t->width==32&&t->height==16&&t->tex0==UINT64_C(0x115408000);
 bool hud8=t->width==128&&t->height==64&&t->tex0==GH_ID_TIM2_128X64_PSMT8_TEX0;
 if((!t32&&!cursor&&!logo&&!loading&&!hud8)||bytes!=(size_t)t->width*t->height*4||((t->tex0>>35)&3)!=0)return GH_UI_UNSUPPORTED;
 std::vector<unsigned char> raw;try{raw.assign(rgba,rgba+bytes);}catch(const std::bad_alloc&){return GH_UI_MEMORY;}
 for(size_t i=3;i<bytes;i+=4){if((raw[i]&1)&&raw[i]!=255)return GH_UI_UNSUPPORTED;raw[i]=raw[i]==255?128:(unsigned char)(raw[i]/2);}
 std::unique_ptr<SpriteTexture> texture(new(std::nothrow)SpriteTexture);if(!texture)return GH_UI_MEMORY;texture->width=t->width;texture->height=t->height;
 D3D11_TEXTURE2D_DESC td={};td.Width=t->width;td.Height=t->height;td.MipLevels=1;td.ArraySize=1;td.Format=DXGI_FORMAT_R8G8B8A8_UINT;td.SampleDesc.Count=1;td.Usage=D3D11_USAGE_IMMUTABLE;td.BindFlags=D3D11_BIND_SHADER_RESOURCE;
 D3D11_SUBRESOURCE_DATA data={raw.data(),t->width*4U,0};SGPU(r->device.p->CreateTexture2D(&td,&data,texture->image.out()));SGPU(r->device.p->CreateShaderResourceView(texture->image.p,nullptr,texture->view.out()));
 if(r->textures.size()>=1024)return GH_UI_MEMORY;
 try{r->textures.push_back(std::move(texture));}catch(const std::bad_alloc&){return GH_UI_MEMORY;}*handle=(uint32_t)r->textures.size();return GH_UI_OK;
}
static gh_ui_error parameters(const gh_ui_sprite_renderer *r,const gh_ui_sprite_item &item,SpriteParams *p){
 const gh_id_texture_quad &q=item.quad;if(!item.texture||item.texture>r->textures.size())return GH_UI_ARGUMENT;const SpriteTexture &t=*r->textures[item.texture-1];
 float coords[]={q.x0,q.y0,q.x1,q.y1};for(unsigned i=0;i<4;i++){if(!std::isfinite(coords[i])||coords[i]<-4096||coords[i]>4096)return GH_UI_UNSUPPORTED;p->rectangle[i]=(int)(coords[i]*16);}
 if(p->rectangle[2]<=p->rectangle[0]||p->rectangle[3]<=p->rectangle[1]||q.color_mode!=6||q.font_mode!=5||q.clip)return GH_UI_UNSUPPORTED;
 float uv[]={q.u0,q.v0,q.u1,q.v1};for(unsigned i=0;i<4;i++){if(!std::isfinite(uv[i])||uv[i]<-4||uv[i]>4)return GH_UI_UNSUPPORTED;
  int32_t fixed=(int32_t)(uv[i]*(float)(i&1?t.height:t.width)*65536.0f);p->st[i]=(uint32_t)fixed;}
 p->info[0]=q.color;p->info[1]=t.width;p->info[2]=t.height;p->info[3]=r->width;p->dimensions[0]=r->height;p->dimensions[1]=p->dimensions[2]=p->dimensions[3]=0;return GH_UI_OK;
}
extern "C" gh_ui_error gh_ui_sprite_batch_with_depth(gh_ui_sprite_renderer *r,const gh_ui_sprite_state *s,const gh_ui_sprite_item *items,size_t count,const unsigned char *back,unsigned char *output,size_t bytes,const gh_ui_depth_write *depth){
 if(!r||!s||(!items&&count)||!back||!output||GetCurrentThreadId()!=r->thread||bytes<(size_t)r->width*r->height*4||count>1024)return GH_UI_ARGUMENT;
 if(s->alpha!=UINT64_C(0x8000000044)||s->test!=0x3001d||s->prim!=0x54||s->tex1!=0x60||s->clamp||s->colclamp!=1||s->prmodecont!=1||s->pabe||s->fba||s->frame_mask)return GH_UI_UNSUPPORTED;
 for(size_t i=0;i<count;i++){SpriteParams p;gh_ui_error e=parameters(r,items[i],&p);if(e)return e;}
 auto *ctx=r->context.p;
 if(depth){gh_ui_error e=r->depth.begin(r->device.p,ctx,r->width,r->height,*depth);if(e)return e;}
 ctx->UpdateSubresource(r->frame[0].p,0,nullptr,back,r->width*4,0);unsigned previous=0;
 ctx->CSSetShader(r->shader.p,nullptr,0);ctx->CSSetConstantBuffers(0,1,&r->constants.p);
 for(size_t i=0;i<count;i++){
  SpriteParams p;parameters(r,items[i],&p);if(depth){p.dimensions[2]=1;p.dimensions[3]=depth->z;}unsigned next=previous^1;ctx->UpdateSubresource(r->constants.p,0,nullptr,&p,0,0);
  ID3D11ShaderResourceView *views[]={r->textures[items[i].texture-1]->view.p,r->frameview[previous].p};ctx->CSSetShaderResources(0,2,views);ID3D11UnorderedAccessView *targets[]={r->target[next].p,depth?r->depth.view():nullptr};ctx->CSSetUnorderedAccessViews(0,2,targets,nullptr);ctx->Dispatch((r->width+7)/8,(r->height+7)/8,1);
  ID3D11ShaderResourceView *nullviews[2]={};ID3D11UnorderedAccessView *nulltarget[2]={};ctx->CSSetShaderResources(0,2,nullviews);ctx->CSSetUnorderedAccessViews(0,2,nulltarget,nullptr);previous=next;
 }
 ctx->CopyResource(r->readback.p,r->frame[previous].p);D3D11_MAPPED_SUBRESOURCE map={};SGPU(ctx->Map(r->readback.p,0,D3D11_MAP_READ,0,&map));for(unsigned y=0;y<r->height;y++)memcpy(output+(size_t)y*r->width*4,(unsigned char*)map.pData+(size_t)y*map.RowPitch,(size_t)r->width*4);ctx->Unmap(r->readback.p,0);
 return depth?r->depth.finish(ctx,r->width,r->height,*depth):GH_UI_OK;
}
extern "C" gh_ui_error gh_ui_sprite_batch(gh_ui_sprite_renderer *r,const gh_ui_sprite_state *s,const gh_ui_sprite_item *items,
 size_t count,const unsigned char *back,unsigned char *output,size_t bytes){
 return gh_ui_sprite_batch_with_depth(r,s,items,count,back,output,bytes,nullptr);
}
