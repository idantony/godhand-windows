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
#include "ui_polygon_renderer.h"
template<class T>struct SPtr{T*p=nullptr;~SPtr(){if(p)p->Release();}T**out(){return &p;}SPtr()=default;SPtr(const SPtr&)=delete;};
struct PolygonTexture{SPtr<ID3D11Texture2D> image;SPtr<ID3D11ShaderResourceView> view;unsigned width,height;};
struct gh_ui_polygon_renderer{
 unsigned width,height;DWORD thread;
 SPtr<ID3D11Device> device;SPtr<ID3D11DeviceContext> context;SPtr<ID3D11ComputeShader> shader;
 SPtr<ID3D11Buffer> constants;SPtr<ID3D11Texture2D> frame[2],readback;
 SPtr<ID3D11ShaderResourceView> frameview[2];SPtr<ID3D11UnorderedAccessView> target[2];
 std::vector<std::unique_ptr<PolygonTexture>> textures;
};
struct PolygonParams{int xy[8];unsigned su[4],sv[4],info[4],dimensions[4];};
#define SGPU(x) do{HRESULT result=(x);if(FAILED(result)){std::fprintf(stderr,"polygon GPU failure %d %08lx\n",__LINE__,(unsigned long)result);return GH_UI_GPU;}}while(0)
static const char *polygon_shader=R"(
Texture2D<uint4> sourceImage:register(t0);Texture2D<uint4> previous:register(t1);RWTexture2D<uint4> outputImage:register(u0);
cbuffer Parameters:register(b0){int4 xy01;int4 xy23;uint4 su;uint4 sv;uint4 info;uint4 dimensions;};
uint2 multiply64(uint a,uint b){uint p0=(a&65535)*(b&65535),p1=(a&65535)*(b>>16),p2=(a>>16)*(b&65535),p3=(a>>16)*(b>>16);uint mid=(p0>>16)+(p1&65535)+(p2&65535);return uint2((p0&65535)|(mid<<16),p3+(p1>>16)+(p2>>16)+(mid>>16));}
uint2 add64(uint2 a,uint2 b){uint lo=a.x+b.x;return uint2(lo,a.y+b.y+(lo<a.x?1:0));}
uint divide64(uint2 p,uint divisor){uint r=0,q=0;[loop]for(int bit=63;bit>=0;bit--){uint v=bit>=32?((p.y>>(bit-32))&1):((p.x>>bit)&1);r=(r<<1)|v;if(r>=divisor){r-=divisor;if(bit<32)q|=1u<<bit;}}return q;}
int edge(int2 a,int2 b,int2 p){int2 d=b-a,v=p-a;return d.x*v.y-d.y*v.x;}
bool inclusive(int2 a,int2 b){int2 d=b-a;return d.y<0||(d.y==0&&d.x>0);}
bool covered_triangle(int2 p,int2 a,int2 b,int2 c,uint3 us,uint3 vs,out uint u,out uint v){
 int area=edge(a,b,c);if(area<0){int2 z=b;b=c;c=z;uint q=us.y;us.y=us.z;us.z=q;q=vs.y;vs.y=vs.z;vs.z=q;area=-area;}
 int3 w=int3(edge(b,c,p),edge(c,a,p),edge(a,b,p));u=0;v=0;
 if(any(w<0)||(w.x==0&&!inclusive(b,c))||(w.y==0&&!inclusive(c,a))||(w.z==0&&!inclusive(a,b)))return false;
 u=divide64(add64(add64(multiply64(us.x,uint(w.x)),multiply64(us.y,uint(w.y))),multiply64(us.z,uint(w.z))),uint(area));
 v=divide64(add64(add64(multiply64(vs.x,uint(w.x)),multiply64(vs.y,uint(w.y))),multiply64(vs.z,uint(w.z))),uint(area));return true;
}
int4 texel(int x,int y){return int4(sourceImage.Load(int3(x&int(info.y-1),y&int(info.z-1),0)));}
int4 lerp4(int4 a,int4 b,int f){return a+(((b-a)*f)>>4);}
[numthreads(8,8,1)]void main(uint3 tid:SV_DispatchThreadID){uint2 p=tid.xy;if(p.x>=info.w||p.y>=dimensions.x)return;uint4 dst=previous.Load(int3(p,0));int2 pixel_fixed=int2(p)*16;uint u,v;
 bool hit=covered_triangle(pixel_fixed,xy01.xy,xy01.zw,xy23.xy,su.xyz,sv.xyz,u,v);
 if(!hit)hit=covered_triangle(pixel_fixed,xy01.zw,xy23.zw,xy23.xy,su.ywz,sv.ywz,u,v);if(!hit){outputImage[p]=dst;return;}
 int bu=int(u)-32768,bv=int(v)-32768,x=bu>>16,y=bv>>16,fu=(bu>>12)&15,fv=(bv>>12)&15;
 int4 filtered=lerp4(lerp4(texel(x,y),texel(x+1,y),fu),lerp4(texel(x,y+1),texel(x+1,y+1),fu),fv);
 uint4 tint=uint4(info.x&255,(info.x>>8)&255,(info.x>>16)&255,info.x>>24);int alpha=min(255,(filtered.a*int(tint.a))>>7);
 if(alpha<=1){outputImage[p]=dst;return;}int3 src=min(int3(255,255,255),(filtered.rgb*int3(tint.rgb))>>7);int3 rgb=int3(dst.rgb)+(((src-int3(dst.rgb))*alpha)>>7);outputImage[p]=uint4(clamp(rgb,0,255),alpha);
})";
extern "C" void gh_ui_polygon_destroy(gh_ui_polygon_renderer *r){delete r;}
extern "C" gh_ui_error gh_ui_polygon_create(unsigned w,unsigned h,gh_ui_polygon_renderer **out){
 if(!out)return GH_UI_ARGUMENT;*out=nullptr;if(w!=512||h!=448)return GH_UI_UNSUPPORTED;
 std::unique_ptr<gh_ui_polygon_renderer> r(new(std::nothrow)gh_ui_polygon_renderer);if(!r)return GH_UI_MEMORY;
 r->width=w;r->height=h;r->thread=GetCurrentThreadId();SPtr<IDXGIFactory1> factory;SGPU(CreateDXGIFactory1(__uuidof(IDXGIFactory1),(void**)factory.out()));SPtr<IDXGIAdapter1> adapter;
 for(UINT i=0;;i++){IDXGIAdapter1 *candidate=nullptr;HRESULT hr=factory.p->EnumAdapters1(i,&candidate);if(hr==DXGI_ERROR_NOT_FOUND)break;SGPU(hr);DXGI_ADAPTER_DESC1 desc={};candidate->GetDesc1(&desc);if(desc.VendorId==0x10de&&!(desc.Flags&DXGI_ADAPTER_FLAG_SOFTWARE)){adapter.p=candidate;break;}candidate->Release();}
 if(!adapter.p)return GH_UI_GPU;
 SGPU(D3D11CreateDevice(adapter.p,D3D_DRIVER_TYPE_UNKNOWN,nullptr,0,nullptr,0,D3D11_SDK_VERSION,r->device.out(),nullptr,r->context.out()));
 SPtr<ID3DBlob> code,errors;HRESULT compiled=D3DCompile(polygon_shader,strlen(polygon_shader),nullptr,nullptr,nullptr,"main","cs_5_0",D3DCOMPILE_ENABLE_STRICTNESS,0,code.out(),errors.out());
 if(FAILED(compiled)){if(errors.p)std::fprintf(stderr,"%s",(char*)errors.p->GetBufferPointer());return GH_UI_GPU;}
 SGPU(r->device.p->CreateComputeShader(code.p->GetBufferPointer(),code.p->GetBufferSize(),nullptr,r->shader.out()));
 D3D11_BUFFER_DESC bd={};bd.ByteWidth=sizeof(PolygonParams);bd.Usage=D3D11_USAGE_DEFAULT;bd.BindFlags=D3D11_BIND_CONSTANT_BUFFER;SGPU(r->device.p->CreateBuffer(&bd,nullptr,r->constants.out()));
 D3D11_TEXTURE2D_DESC td={};td.Width=w;td.Height=h;td.MipLevels=1;td.ArraySize=1;td.Format=DXGI_FORMAT_R8G8B8A8_UINT;td.SampleDesc.Count=1;td.Usage=D3D11_USAGE_DEFAULT;td.BindFlags=D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_UNORDERED_ACCESS;
 for(unsigned i=0;i<2;i++){SGPU(r->device.p->CreateTexture2D(&td,nullptr,r->frame[i].out()));SGPU(r->device.p->CreateShaderResourceView(r->frame[i].p,nullptr,r->frameview[i].out()));SGPU(r->device.p->CreateUnorderedAccessView(r->frame[i].p,nullptr,r->target[i].out()));}
 td.Usage=D3D11_USAGE_STAGING;td.BindFlags=0;td.CPUAccessFlags=D3D11_CPU_ACCESS_READ;SGPU(r->device.p->CreateTexture2D(&td,nullptr,r->readback.out()));*out=r.release();return GH_UI_OK;
}
extern "C" gh_ui_error gh_ui_polygon_upload(gh_ui_polygon_renderer *r,const gh_id_texture *t,const unsigned char *rgba,size_t bytes,uint32_t *handle){
 if(!r||!t||!rgba||!handle||GetCurrentThreadId()!=r->thread)return GH_UI_ARGUMENT;
 // Actual64x64 indexed4 HUD icon; templateTCC overridden by15BCF0.
 if(t->width!=64||t->height!=64||bytes!=64*64*4||((t->tex0>>20)&63)!=20||((t->tex0>>26)&15)!=6||((t->tex0>>30)&15)!=6||((t->tex0>>35)&3)||((t->tex0>>51)&63)||((t->tex0>>56)&31))return GH_UI_UNSUPPORTED;
 std::vector<unsigned char> raw;try{raw.assign(rgba,rgba+bytes);}catch(const std::bad_alloc&){return GH_UI_MEMORY;}
 for(size_t i=3;i<bytes;i+=4){if((raw[i]&1)&&raw[i]!=255)return GH_UI_UNSUPPORTED;raw[i]=raw[i]==255?128:(unsigned char)(raw[i]/2);}
 std::unique_ptr<PolygonTexture> texture(new(std::nothrow)PolygonTexture);if(!texture)return GH_UI_MEMORY;texture->width=t->width;texture->height=t->height;
 D3D11_TEXTURE2D_DESC td={};td.Width=t->width;td.Height=t->height;td.MipLevels=1;td.ArraySize=1;td.Format=DXGI_FORMAT_R8G8B8A8_UINT;td.SampleDesc.Count=1;td.Usage=D3D11_USAGE_IMMUTABLE;td.BindFlags=D3D11_BIND_SHADER_RESOURCE;
 D3D11_SUBRESOURCE_DATA data={raw.data(),t->width*4U,0};SGPU(r->device.p->CreateTexture2D(&td,&data,texture->image.out()));SGPU(r->device.p->CreateShaderResourceView(texture->image.p,nullptr,texture->view.out()));
 if(r->textures.size()>=1024)return GH_UI_MEMORY;
 try{r->textures.push_back(std::move(texture));}catch(const std::bad_alloc&){return GH_UI_MEMORY;}*handle=(uint32_t)r->textures.size();return GH_UI_OK;
}
static gh_ui_error parameters(const gh_ui_polygon_renderer*r,const gh_ui_polygon_item&item,PolygonParams*p){
 const auto&q=item.polygon;if(!item.texture||item.texture>r->textures.size())return GH_UI_ARGUMENT;const auto&t=*r->textures[item.texture-1];
 if(q.color_mode!=6||q.font_mode!=5||q.clip||(q.alignment!=0&&q.alignment!=3))return GH_UI_UNSUPPORTED;
 for(unsigned i=0;i<4;i++){float x=q.x[i],y=q.y[i];if(!std::isfinite(x)||!std::isfinite(y)||x< -512||x>1024||y< -512||y>1024)return GH_UI_UNSUPPORTED;p->xy[i*2]=(int)(x*16);p->xy[i*2+1]=(int)(y*16);
  if(!std::isfinite(q.u[i])||!std::isfinite(q.v[i])||q.u[i]<0||q.u[i]>1||q.v[i]<0||q.v[i]>1)return GH_UI_UNSUPPORTED;p->su[i]=(unsigned)(q.u[i]*t.width*65536.0f);p->sv[i]=(unsigned)(q.v[i]*t.height*65536.0f);
 }
 const unsigned order[]={0,1,3,2};int sign=0;
 for(unsigned i=0;i<4;i++){unsigned a=order[i],b=order[(i+1)%4],c=order[(i+2)%4];int64_t dx=p->xy[b*2]-p->xy[a*2],dy=p->xy[b*2+1]-p->xy[a*2+1];int64_t cross=dx*(p->xy[c*2+1]-p->xy[a*2+1])-dy*(p->xy[c*2]-p->xy[a*2]);if(!cross||cross>INT32_MAX||cross< -INT32_MAX)return GH_UI_UNSUPPORTED;int next=cross<0?-1:1;if(sign&&sign!=next)return GH_UI_UNSUPPORTED;sign=next;}
 p->info[0]=q.color;p->info[1]=t.width;p->info[2]=t.height;p->info[3]=r->width;p->dimensions[0]=r->height;p->dimensions[1]=p->dimensions[2]=p->dimensions[3]=0;return GH_UI_OK;
}
extern "C" gh_ui_error gh_ui_polygon_batch(gh_ui_polygon_renderer *r,const gh_ui_sprite_state *s,const gh_ui_polygon_item *items,size_t count,const unsigned char *back,unsigned char *output,size_t bytes){
 if(!r||!s||(!items&&count)||!back||!output||GetCurrentThreadId()!=r->thread||bytes<(size_t)r->width*r->height*4||count>1024)return GH_UI_ARGUMENT;
 if(s->alpha!=UINT64_C(0x8000000044)||s->test!=0x3001d||s->prim!=0x54||s->tex1!=0x60||s->clamp||s->colclamp!=1||s->prmodecont!=1||s->pabe||s->fba||s->frame_mask)return GH_UI_UNSUPPORTED;
 for(size_t i=0;i<count;i++){PolygonParams p;gh_ui_error e=parameters(r,items[i],&p);if(e)return e;}
 auto *ctx=r->context.p;ctx->UpdateSubresource(r->frame[0].p,0,nullptr,back,r->width*4,0);unsigned previous=0;
 ctx->CSSetShader(r->shader.p,nullptr,0);ctx->CSSetConstantBuffers(0,1,&r->constants.p);
 for(size_t i=0;i<count;i++){
  PolygonParams p;parameters(r,items[i],&p);unsigned next=previous^1;ctx->UpdateSubresource(r->constants.p,0,nullptr,&p,0,0);
  ID3D11ShaderResourceView *views[]={r->textures[items[i].texture-1]->view.p,r->frameview[previous].p};ctx->CSSetShaderResources(0,2,views);ctx->CSSetUnorderedAccessViews(0,1,&r->target[next].p,nullptr);ctx->Dispatch((r->width+7)/8,(r->height+7)/8,1);
  ID3D11ShaderResourceView *nullviews[2]={};ID3D11UnorderedAccessView *nulltarget=nullptr;ctx->CSSetShaderResources(0,2,nullviews);ctx->CSSetUnorderedAccessViews(0,1,&nulltarget,nullptr);previous=next;
 }
 ctx->CopyResource(r->readback.p,r->frame[previous].p);D3D11_MAPPED_SUBRESOURCE map={};SGPU(ctx->Map(r->readback.p,0,D3D11_MAP_READ,0,&map));for(unsigned y=0;y<r->height;y++)memcpy(output+(size_t)y*r->width*4,(unsigned char*)map.pData+(size_t)y*map.RowPitch,(size_t)r->width*4);ctx->Unmap(r->readback.p,0);return GH_UI_OK;
}
