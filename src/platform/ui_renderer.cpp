// Independent restricted D3D11 execution of recovered font GS fragment math.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>
#include <cstring>
#include <cstdio>
#include <climits>
#include "ui_renderer.h"
template<class T> struct Ptr{T*p=nullptr;~Ptr(){if(p)p->Release();}T**out(){return &p;}};
#define GPU(x) do{HRESULT gpu_result=(x);if(FAILED(gpu_result)){std::fprintf(stderr,"ui GPU failure %d: %08lx\n",__LINE__,(unsigned long)gpu_result);return GH_UI_GPU;}}while(0)
extern "C" gh_ui_error gh_ui_font_quad(const gh_font_quad *q,const gh_ui_gs_state *s,
 int32_t ox,int32_t oy,unsigned w,unsigned h,const unsigned char *atlas,
 const unsigned char *back,unsigned char *output,size_t capacity){
 if(!q||!s||!atlas||!back||!output||!w||!h||w>2048||h>2048||capacity<(size_t)w*h*4)return GH_UI_ARGUMENT;
 // GS state actually emitted by2A4FD8/2A53A8. COLCLAMP must be confirmed by
 // caller's global environment; font setup does not write it locally.
 if(s->alpha!=UINT64_C(0x8000000044)||s->test!=0x3001d||s->prim!=0x156||s->tex1||s->clamp||s->colclamp!=1||
    s->prmodecont!=1||s->pabe||s->fba||s->frame_mask||
    ((s->tex0>>20)&63)!=20||((s->tex0>>26)&15)!=9||((s->tex0>>30)&15)!=8||((s->tex0>>34)&7)!=1)return GH_UI_UNSUPPORTED;
 int64_t ax0=(int64_t)q->x0-ox,ay0=(int64_t)q->y0-oy,ax1=(int64_t)q->x1-ox,ay1=(int64_t)q->y1-oy;
 if(ax0<INT_MIN||ax0>INT_MAX||ay0<INT_MIN||ay0>INT_MAX||ax1<INT_MIN||ax1>INT_MAX||ay1<INT_MIN||ay1>INT_MAX)return GH_UI_UNSUPPORTED;
 int x0=(int)ax0,y0=(int)ay0,x1=(int)ax1,y1=(int)ay1;
 for(size_t i=3;i<512U*256U*4U;i+=4)if((atlas[i]&1)&&atlas[i]!=255)return GH_UI_UNSUPPORTED;
 if(x0%16||y0%16||x1%16||y1%16||q->u0%16!=8||q->v0%16!=8||q->u1%16!=8||q->v1%16!=8||
    x1<=x0||y1<=y0||x1-x0!=(int)q->u1-q->u0||y1-y0!=(int)q->v1-q->v0)return GH_UI_UNSUPPORTED;
 if(q->u1/16>512||q->v1/16>256)return GH_UI_ARGUMENT;
 Ptr<IDXGIFactory1> factory;GPU(CreateDXGIFactory1(__uuidof(IDXGIFactory1),(void**)factory.out()));
 Ptr<IDXGIAdapter1> adapter;
 for(UINT i=0;;i++){IDXGIAdapter1 *a=nullptr;HRESULT hr=factory.p->EnumAdapters1(i,&a);if(hr==DXGI_ERROR_NOT_FOUND)break;GPU(hr);DXGI_ADAPTER_DESC1 d={};a->GetDesc1(&d);if(d.VendorId==0x10de&&!(d.Flags&DXGI_ADAPTER_FLAG_SOFTWARE)){adapter.p=a;break;}a->Release();}
 if(!adapter.p)return GH_UI_GPU;
 Ptr<ID3D11Device> device;Ptr<ID3D11DeviceContext> ctx;
 GPU(D3D11CreateDevice(adapter.p,D3D_DRIVER_TYPE_UNKNOWN,nullptr,0,nullptr,0,D3D11_SDK_VERSION,device.out(),nullptr,ctx.out()));
 const char *source=R"(
Texture2D<uint4> atlas:register(t0);Texture2D<uint4> background:register(t1);
RWTexture2D<uint4> target:register(u0);
cbuffer P:register(b0){int4 rectangle;uint4 info;uint4 dimensions;};
[numthreads(8,8,1)]void main(uint3 thread:SV_DispatchThreadID){
 uint2 p=thread.xy;if(p.x>=dimensions.x||p.y>=dimensions.y)return;
 uint4 dest=background.Load(int3(p,0));int2 local=int2(p)-rectangle.xy;
 if(any(local<0)||any(int2(p)>=rectangle.zw)){target[p]=dest;return;}
 uint4 sampleColor=atlas.Load(int3((int2(info.xy)+local)&int2(511,255),0));
 uint4 color=uint4(info.z&255,(info.z>>8)&255,(info.z>>16)&255,info.z>>24);
 uint alphaTexture=sampleColor.a==255?128:sampleColor.a/2;
 uint alpha=min(255u,(alphaTexture*color.a)>>7);
 if(alpha<=1){target[p]=dest;return;}
 int3 src=int3(min(uint3(255,255,255),(sampleColor.rgb*color.rgb)>>7));
 int3 rgb=((src-int3(dest.rgb))*int(alpha))>>7;rgb+=int3(dest.rgb);
 target[p]=uint4(clamp(rgb,0,255),alpha);
})";
 Ptr<ID3DBlob> code;GPU(D3DCompile(source,strlen(source),nullptr,nullptr,nullptr,"main","cs_5_0",D3DCOMPILE_ENABLE_STRICTNESS,0,code.out(),nullptr));
 Ptr<ID3D11ComputeShader> shader;GPU(device.p->CreateComputeShader(code.p->GetBufferPointer(),code.p->GetBufferSize(),nullptr,shader.out()));
 D3D11_TEXTURE2D_DESC td={};td.Width=512;td.Height=256;td.MipLevels=1;td.ArraySize=1;td.Format=DXGI_FORMAT_R8G8B8A8_UINT;td.SampleDesc.Count=1;td.Usage=D3D11_USAGE_IMMUTABLE;td.BindFlags=D3D11_BIND_SHADER_RESOURCE;
 D3D11_SUBRESOURCE_DATA data={atlas,512*4,0};Ptr<ID3D11Texture2D> tex;GPU(device.p->CreateTexture2D(&td,&data,tex.out()));
 Ptr<ID3D11ShaderResourceView> srv;GPU(device.p->CreateShaderResourceView(tex.p,nullptr,srv.out()));
 td.Width=w;td.Height=h;data.pSysMem=back;data.SysMemPitch=w*4;Ptr<ID3D11Texture2D> bg;GPU(device.p->CreateTexture2D(&td,&data,bg.out()));
 Ptr<ID3D11ShaderResourceView> bgsrv;GPU(device.p->CreateShaderResourceView(bg.p,nullptr,bgsrv.out()));
 td.Usage=D3D11_USAGE_DEFAULT;td.BindFlags=D3D11_BIND_UNORDERED_ACCESS;Ptr<ID3D11Texture2D> target;GPU(device.p->CreateTexture2D(&td,nullptr,target.out()));
 Ptr<ID3D11UnorderedAccessView> uav;GPU(device.p->CreateUnorderedAccessView(target.p,nullptr,uav.out()));
 struct Params{int rect[4];unsigned info[4],dimensions[4];}params={{x0/16,y0/16,x1/16,y1/16},{(unsigned)q->u0/16,(unsigned)q->v0/16,q->rgba,0},{w,h,0,0}};
 D3D11_BUFFER_DESC bd={};bd.ByteWidth=sizeof(params);bd.Usage=D3D11_USAGE_IMMUTABLE;bd.BindFlags=D3D11_BIND_CONSTANT_BUFFER;D3D11_SUBRESOURCE_DATA initial={&params,0,0};Ptr<ID3D11Buffer> constants;GPU(device.p->CreateBuffer(&bd,&initial,constants.out()));
 ID3D11ShaderResourceView *views[]={srv.p,bgsrv.p};ctx.p->CSSetShader(shader.p,nullptr,0);ctx.p->CSSetShaderResources(0,2,views);ctx.p->CSSetConstantBuffers(0,1,&constants.p);ctx.p->CSSetUnorderedAccessViews(0,1,&uav.p,nullptr);ctx.p->Dispatch((w+7)/8,(h+7)/8,1);
 td.Usage=D3D11_USAGE_STAGING;td.BindFlags=0;td.CPUAccessFlags=D3D11_CPU_ACCESS_READ;Ptr<ID3D11Texture2D> readback;GPU(device.p->CreateTexture2D(&td,nullptr,readback.out()));ctx.p->CopyResource(readback.p,target.p);
 D3D11_MAPPED_SUBRESOURCE mapped={};GPU(ctx.p->Map(readback.p,0,D3D11_MAP_READ,0,&mapped));
 for(unsigned y=0;y<h;y++)memcpy(output+(size_t)y*w*4,(unsigned char*)mapped.pData+(size_t)y*mapped.RowPitch,(size_t)w*4);
 ctx.p->Unmap(readback.p,0);return GH_UI_OK;
}
