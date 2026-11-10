#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>
#include <memory>
#include <new>
#include <cmath>
#include <cstring>
#include "ui_fade_renderer.h"
template<class T>struct FP{T*p=nullptr;~FP(){if(p)p->Release();}T**out(){return&p;}};
struct gh_ui_fade_renderer{DWORD owner;FP<ID3D11Device>device;FP<ID3D11DeviceContext>context;FP<ID3D11ComputeShader>shader;FP<ID3D11Texture2D>back,target,readback,depth;FP<ID3D11ShaderResourceView>srv,depth_srv;FP<ID3D11UnorderedAccessView>uav;FP<ID3D11Buffer>constant;};
static int64_t cross(int ax,int ay,int bx,int by,int px,int py){return (int64_t)(bx-ax)*(py-ay)-(int64_t)(by-ay)*(px-ax);}
extern "C" gh_ui_error gh_ui_fade_geometry(const gh_fade_draw*d,const gh_ui_fade_context*c,uint16_t xy[8]){
 if(!d||!c||!xy)return GH_UI_ARGUMENT;if(c->width!=512||c->height!=448||c->gs_y_origin!=1824||c->gs_y_scale!=448.0f/480.0f||d->x!=28672||d->y!=29184||d->w!=8192||d->h!=7168)return GH_UI_UNSUPPORTED;
 // Source15C928 scalesitszero-angleverticesbeforetranslation;15B880 then
 // vftoi4/addorigin andORpacksXYZwithoutclearinghighXbits. Retainthatmath.
 float scale_x=((float)c->width/640.0f)/0.93333334f;float dx=(float)d->w*scale_x,dy=(float)d->h*c->gs_y_scale;
 for(unsigned i=0;i<4;i++){float x=((i&1)?dx:-dx)+(float)d->x,y=((i&2)?dy:-dy)+(float)d->y;int32_t fx=(int32_t)(x*16.0f)+28672,fy=(int32_t)(y*16.0f)+29184;uint64_t packed=(uint64_t)(int64_t)fx|((uint64_t)(int64_t)fy<<16);xy[i*2]=(uint16_t)packed;xy[i*2+1]=(uint16_t)(packed>>16);}
 const unsigned order[]={0,1,3,2};int direction=0;
 for(unsigned i=0;i<4;i++){unsigned a=order[i],b=order[(i+1)%4],e=order[(i+2)%4];int64_t value=cross(xy[a*2],xy[a*2+1],xy[b*2],xy[b*2+1],xy[e*2],xy[e*2+1]);int sign=value>0?1:value<0?-1:0;if(!sign||(direction&&direction!=sign))return GH_UI_UNSUPPORTED;direction=sign;}
 const int px[]={28672,28672+511*16},py[]={29184,29184+447*16};
 for(int x:px)for(int y:py)for(unsigned i=0;i<4;i++){unsigned a=order[i],b=order[(i+1)%4];int64_t value=cross(xy[a*2],xy[a*2+1],xy[b*2],xy[b*2+1],x,y);if((direction>0&&value<=0)||(direction<0&&value>=0))return GH_UI_UNSUPPORTED;}
 return GH_UI_OK;
}
#define FG(x) do{if(FAILED(x))return GH_UI_GPU;}while(0)
extern "C" void gh_ui_fade_destroy(gh_ui_fade_renderer*p){delete p;}
extern "C" gh_ui_error gh_ui_fade_create(gh_ui_fade_renderer**out){
 if(!out)return GH_UI_ARGUMENT;*out=nullptr;std::unique_ptr<gh_ui_fade_renderer>p(new(std::nothrow)gh_ui_fade_renderer);if(!p)return GH_UI_MEMORY;p->owner=GetCurrentThreadId();FP<IDXGIFactory1>factory;FP<IDXGIAdapter1>adapter;FG(CreateDXGIFactory1(__uuidof(IDXGIFactory1),(void**)factory.out()));
 for(UINT i=0;;i++){IDXGIAdapter1*a=nullptr;if(factory.p->EnumAdapters1(i,&a)==DXGI_ERROR_NOT_FOUND)break;DXGI_ADAPTER_DESC1 d={};a->GetDesc1(&d);if(d.VendorId==0x10de&&!(d.Flags&DXGI_ADAPTER_FLAG_SOFTWARE)){adapter.p=a;break;}a->Release();}if(!adapter.p)return GH_UI_GPU;FG(D3D11CreateDevice(adapter.p,D3D_DRIVER_TYPE_UNKNOWN,nullptr,0,nullptr,0,D3D11_SDK_VERSION,p->device.out(),nullptr,p->context.out()));
 static const char*code=R"(Texture2D<uint4> back:register(t0);Texture2D<uint> depth:register(t1);RWTexture2D<uint4> target:register(u0);cbuffer C:register(b0){uint4 color;uint4 settings;};[numthreads(8,8,1)]void main(uint3 id:SV_DispatchThreadID){uint2 p=id.xy;if(p.x>=512||p.y>=448)return;uint4 d=back.Load(int3(p,0));if(settings.x&&depth.Load(int3(p,0))>=65535){target[p]=d;return;}int3 rgb=int3(d.rgb)+(((int3(color.rgb)-int3(d.rgb))*int(color.a))>>7);target[p]=uint4(clamp(rgb,0,255),color.a);})";
 FP<ID3DBlob>binary,errors;FG(D3DCompile(code,strlen(code),nullptr,nullptr,nullptr,"main","cs_5_0",D3DCOMPILE_ENABLE_STRICTNESS,0,binary.out(),errors.out()));FG(p->device.p->CreateComputeShader(binary.p->GetBufferPointer(),binary.p->GetBufferSize(),nullptr,p->shader.out()));D3D11_BUFFER_DESC bd={};bd.ByteWidth=32;bd.Usage=D3D11_USAGE_DEFAULT;bd.BindFlags=D3D11_BIND_CONSTANT_BUFFER;FG(p->device.p->CreateBuffer(&bd,nullptr,p->constant.out()));
 D3D11_TEXTURE2D_DESC td={};td.Width=512;td.Height=448;td.MipLevels=td.ArraySize=1;td.SampleDesc.Count=1;td.Format=DXGI_FORMAT_R8G8B8A8_UINT;td.Usage=D3D11_USAGE_DEFAULT;td.BindFlags=D3D11_BIND_SHADER_RESOURCE;FG(p->device.p->CreateTexture2D(&td,nullptr,p->back.out()));FG(p->device.p->CreateShaderResourceView(p->back.p,nullptr,p->srv.out()));td.BindFlags=D3D11_BIND_UNORDERED_ACCESS;FG(p->device.p->CreateTexture2D(&td,nullptr,p->target.out()));FG(p->device.p->CreateUnorderedAccessView(p->target.p,nullptr,p->uav.out()));td.Usage=D3D11_USAGE_STAGING;td.BindFlags=0;td.CPUAccessFlags=D3D11_CPU_ACCESS_READ;FG(p->device.p->CreateTexture2D(&td,nullptr,p->readback.out()));*out=p.release();return GH_UI_OK;
}
extern "C" gh_ui_error gh_ui_fade_apply_with_depth(gh_ui_fade_renderer*p,const gh_fade_draw*d,const gh_ui_fade_context*c,
 const unsigned char*back,unsigned char*out,size_t bytes,const uint32_t*z,size_t count){
 if(!p||!d||!c||!back||!out||bytes!=512*448*4||p->owner!=GetCurrentThreadId()||(z?count!=512*448:count!=0))return GH_UI_ARGUMENT;if(c->width!=512||c->height!=448)return GH_UI_UNSUPPORTED;if(!d->visible){memmove(out,back,512*448*4);return GH_UI_OK;}uint16_t xy[8];gh_ui_error geometry=gh_ui_fade_geometry(d,c,xy);if(geometry)return geometry;
 bool compare=d->test==0x70000;
 if(d->kind||d->alpha!=UINT64_C(0x8000000044)||(!compare&&d->test!=0x30000)||(compare&&!z)||c->colclamp!=1||c->prmodecont!=1||c->pabe||c->fba||c->frame_mask)return GH_UI_UNSUPPORTED;
 if(compare){
  for(size_t i=0;i<count;i++)if(z[i]>65535)return GH_UI_UNSUPPORTED;
  if(!p->depth_srv.p){
   D3D11_TEXTURE2D_DESC td={};td.Width=512;td.Height=448;td.MipLevels=td.ArraySize=1;td.Format=DXGI_FORMAT_R32_UINT;
   td.SampleDesc.Count=1;td.Usage=D3D11_USAGE_DEFAULT;td.BindFlags=D3D11_BIND_SHADER_RESOURCE;
   FP<ID3D11Texture2D> image;FP<ID3D11ShaderResourceView> view;
   FG(p->device.p->CreateTexture2D(&td,nullptr,image.out()));
   FG(p->device.p->CreateShaderResourceView(image.p,nullptr,view.out()));
   p->depth.p=image.p;p->depth_srv.p=view.p;image.p=nullptr;view.p=nullptr;
  }
  p->context.p->UpdateSubresource(p->depth.p,0,nullptr,z,512*sizeof(*z),0);
 }
 unsigned color[]={d->rgba&255,(d->rgba>>8)&255,(d->rgba>>16)&255,d->rgba>>24,(unsigned)compare,0,0,0};auto*ctx=p->context.p;ctx->UpdateSubresource(p->back.p,0,nullptr,back,512*4,0);ctx->UpdateSubresource(p->constant.p,0,nullptr,color,0,0);ctx->CSSetShader(p->shader.p,nullptr,0);ID3D11ShaderResourceView*views[]={p->srv.p,compare?p->depth_srv.p:nullptr};ctx->CSSetShaderResources(0,2,views);ctx->CSSetConstantBuffers(0,1,&p->constant.p);ctx->CSSetUnorderedAccessViews(0,1,&p->uav.p,nullptr);ctx->Dispatch(64,56,1);ID3D11ShaderResourceView*nullsrv[2]={};ID3D11UnorderedAccessView*nulluav=nullptr;ctx->CSSetShaderResources(0,2,nullsrv);ctx->CSSetUnorderedAccessViews(0,1,&nulluav,nullptr);ctx->CopyResource(p->readback.p,p->target.p);D3D11_MAPPED_SUBRESOURCE map={};FG(ctx->Map(p->readback.p,0,D3D11_MAP_READ,0,&map));for(unsigned y=0;y<448;y++)memcpy(out+(size_t)y*512*4,(unsigned char*)map.pData+(size_t)y*map.RowPitch,512*4);ctx->Unmap(p->readback.p,0);return GH_UI_OK;
}
extern "C" gh_ui_error gh_ui_fade_apply(gh_ui_fade_renderer*p,const gh_fade_draw*d,const gh_ui_fade_context*c,
 const unsigned char*back,unsigned char*out,size_t bytes){
 return gh_ui_fade_apply_with_depth(p,d,c,back,out,bytes,nullptr,0);
}
