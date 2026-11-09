// SPDX-License-Identifier: MIT
// Offscreen asset QA only. This is not the God Hand renderer or a game launcher.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>
#include <wincodec.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

template<class T> struct Com {
    T *p=nullptr;
    ~Com(){if(p)p->Release();}
    T **out(){return &p;}
    T *operator->(){return p;}
};
static HRESULT failure(const char *stage,HRESULT hr){
    fprintf(stderr,"{\"status\":\"failed\",\"stage\":\"%s\",\"hresult\":\"0x%08lx\"}\n",stage,(unsigned long)hr);
    return hr;
}
#define CHECK(stage, expression) do {HRESULT result=(expression);if(FAILED(result)){failure(stage,result);return 1;}}while(0)
static bool integer(const wchar_t *s,UINT *out){
    wchar_t *end;unsigned long n=wcstoul(s,&end,10);
    if(!*s||*end||n==0||n>4096)return false;*out=(UINT)n;return true;
}

int wmain(int argc,wchar_t **argv){
    const wchar_t *input=nullptr,*output=nullptr;UINT width=0,height=0;
    for(int i=1;i<argc;i+=2){
        if(i+1>=argc)return 2;
        if(!wcscmp(argv[i],L"--rgba"))input=argv[i+1];
        else if(!wcscmp(argv[i],L"--output"))output=argv[i+1];
        else if(!wcscmp(argv[i],L"--width")){if(!integer(argv[i+1],&width))return 2;}
        else if(!wcscmp(argv[i],L"--height")){if(!integer(argv[i+1],&height))return 2;}
        else return 2;
    }
    if(!input||!output||!width||!height||(uint64_t)width*height>4194304){
        fprintf(stderr,"Usage: render-texture --rgba input.rgba --width N --height N --output output.png\n");return 2;
    }
    wchar_t inFull[32768],outFull[32768];
    DWORD inLength=GetFullPathNameW(input,32768,inFull,nullptr),outLength=GetFullPathNameW(output,32768,outFull,nullptr);
    if(!inLength||inLength>=32768||!outLength||outLength>=32768||!_wcsicmp(inFull,outFull)){
        failure("input-output-path",E_INVALIDARG);return 1;
    }
    const UINT ow=width*4,oh=height*4;
    const size_t bytes=(size_t)width*height*4;
    unsigned char *pixels=(unsigned char*)malloc(bytes);
    if(!pixels)return 1;
    struct Free {void *p;~Free(){free(p);}} freePixels{pixels};
    FILE *file=_wfopen(input,L"rb");
    if(!file){failure("input-open",HRESULT_FROM_WIN32(GetLastError()));return 1;}
    bool inputOk=_fseeki64(file,0,SEEK_END)==0&&_ftelli64(file)==(int64_t)bytes&&_fseeki64(file,0,SEEK_SET)==0&&fread(pixels,1,bytes,file)==bytes;
    fclose(file);if(!inputOk){failure("input-size-or-read",E_INVALIDARG);return 1;}
    CHECK("COM",CoInitializeEx(nullptr,COINIT_MULTITHREADED));
    struct Uninit {~Uninit(){CoUninitialize();}} uninit;
    Com<IDXGIFactory1> factory;CHECK("DXGI-factory",CreateDXGIFactory1(__uuidof(IDXGIFactory1),(void**)factory.out()));
    Com<IDXGIAdapter1> adapter;DXGI_ADAPTER_DESC1 desc={};
    for(UINT i=0;;++i){
        IDXGIAdapter1 *candidate=nullptr;HRESULT hr=factory->EnumAdapters1(i,&candidate);
        if(hr==DXGI_ERROR_NOT_FOUND)break;
        CHECK("adapter-enumeration",hr);
        DXGI_ADAPTER_DESC1 d={};hr=candidate->GetDesc1(&d);
        if(SUCCEEDED(hr)&&d.VendorId==0x10de&&!(d.Flags&DXGI_ADAPTER_FLAG_SOFTWARE)&&
           wcsstr(d.Description,L"RTX 4080 SUPER")){adapter.p=candidate;desc=d;break;}
        candidate->Release();
    }
    if(!adapter.p){failure("required-NVIDIA-RTX4080SUPER-not-found",DXGI_ERROR_NOT_FOUND);return 1;}
    Com<ID3D11Device> device;Com<ID3D11DeviceContext> context;
    D3D_FEATURE_LEVEL feature;
    CHECK("D3D11-device",D3D11CreateDevice(adapter.p,D3D_DRIVER_TYPE_UNKNOWN,nullptr,0,nullptr,0,D3D11_SDK_VERSION,device.out(),&feature,context.out()));
    const char *hlsl=R"(
// SPDX-License-Identifier: MIT
Texture2D image : register(t0); SamplerState nearestSampler : register(s0);
struct V {float4 position:SV_Position;float2 uv:TEXCOORD0;};
V vs(uint id:SV_VertexID){V o;o.uv=float2((id<<1)&2,id&2);o.position=float4(o.uv*float2(2,-2)+float2(-1,1),0,1);return o;}
float4 ps(V i):SV_Target{return image.SampleLevel(nearestSampler,i.uv,0);}
)";
    Com<ID3DBlob> vsBlob,psBlob;
    CHECK("compile-VS",D3DCompile(hlsl,strlen(hlsl),nullptr,nullptr,nullptr,"vs","vs_5_0",D3DCOMPILE_ENABLE_STRICTNESS,0,vsBlob.out(),nullptr));
    CHECK("compile-PS",D3DCompile(hlsl,strlen(hlsl),nullptr,nullptr,nullptr,"ps","ps_5_0",D3DCOMPILE_ENABLE_STRICTNESS,0,psBlob.out(),nullptr));
    Com<ID3D11VertexShader> vs;Com<ID3D11PixelShader> ps;
    CHECK("create-VS",device->CreateVertexShader(vsBlob->GetBufferPointer(),vsBlob->GetBufferSize(),nullptr,vs.out()));
    CHECK("create-PS",device->CreatePixelShader(psBlob->GetBufferPointer(),psBlob->GetBufferSize(),nullptr,ps.out()));
    D3D11_TEXTURE2D_DESC td={};td.Width=width;td.Height=height;td.MipLevels=1;td.ArraySize=1;td.Format=DXGI_FORMAT_R8G8B8A8_UNORM;td.SampleDesc.Count=1;td.Usage=D3D11_USAGE_IMMUTABLE;td.BindFlags=D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA initial={pixels,width*4,0};Com<ID3D11Texture2D> texture;
    CHECK("upload-RGBA",device->CreateTexture2D(&td,&initial,texture.out()));
    Com<ID3D11ShaderResourceView> view;CHECK("texture-view",device->CreateShaderResourceView(texture.p,nullptr,view.out()));
    td.Width=ow;td.Height=oh;td.Usage=D3D11_USAGE_DEFAULT;td.BindFlags=D3D11_BIND_RENDER_TARGET;
    Com<ID3D11Texture2D> target;CHECK("render-target",device->CreateTexture2D(&td,nullptr,target.out()));
    Com<ID3D11RenderTargetView> rtv;CHECK("target-view",device->CreateRenderTargetView(target.p,nullptr,rtv.out()));
    D3D11_SAMPLER_DESC sd={};sd.Filter=D3D11_FILTER_MIN_MAG_MIP_POINT;sd.AddressU=sd.AddressV=sd.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;sd.MaxLOD=D3D11_FLOAT32_MAX;
    Com<ID3D11SamplerState> sampler;CHECK("point-sampler",device->CreateSamplerState(&sd,sampler.out()));
    context->OMSetRenderTargets(1,&rtv.p,nullptr);D3D11_VIEWPORT viewport={0,0,(float)ow,(float)oh,0,1};context->RSSetViewports(1,&viewport);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);context->VSSetShader(vs.p,nullptr,0);context->PSSetShader(ps.p,nullptr,0);context->PSSetShaderResources(0,1,&view.p);context->PSSetSamplers(0,1,&sampler.p);context->Draw(3,0);
    td.Usage=D3D11_USAGE_STAGING;td.BindFlags=0;td.CPUAccessFlags=D3D11_CPU_ACCESS_READ;Com<ID3D11Texture2D> staging;
    CHECK("staging",device->CreateTexture2D(&td,nullptr,staging.out()));context->CopyResource(staging.p,target.p);
    D3D11_MAPPED_SUBRESOURCE mapped={};CHECK("GPU-readback",context->Map(staging.p,0,D3D11_MAP_READ,0,&mapped));
    struct Unmap {ID3D11DeviceContext*c;ID3D11Texture2D*t;~Unmap(){c->Unmap(t,0);}} unmap{context.p,staging.p};
    uint64_t mismatches=0;
    for(UINT y=0;y<oh;++y)for(UINT x=0;x<ow;++x)
        if(memcmp((unsigned char*)mapped.pData+(size_t)y*mapped.RowPitch+x*4,pixels+((size_t)(y/4)*width+x/4)*4,4))++mismatches;
    if(mismatches){fprintf(stderr,"{\"status\":\"failed\",\"stage\":\"pixel-compare\",\"mismatched_pixels\":%llu}\n",(unsigned long long)mismatches);return 1;}
    Com<IWICImagingFactory> wic;CHECK("WIC",CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,__uuidof(IWICImagingFactory),(void**)wic.out()));
    // Encode privately in memory, then atomically claim a new output name.
    // CREATE_NEW never truncates an existing input, output, link or other file.
    Com<IStream> stream;CHECK("PNG-memory-stream",CreateStreamOnHGlobal(nullptr,TRUE,stream.out()));
    Com<IWICBitmapEncoder> encoder;CHECK("PNG-encoder",wic->CreateEncoder(GUID_ContainerFormatPng,nullptr,encoder.out()));CHECK("encoder-init",encoder->Initialize(stream.p,WICBitmapEncoderNoCache));
    Com<IWICBitmapFrameEncode> frame;CHECK("PNG-frame",encoder->CreateNewFrame(frame.out(),nullptr));CHECK("frame-init",frame->Initialize(nullptr));CHECK("PNG-size",frame->SetSize(ow,oh));
    WICPixelFormatGUID format=GUID_WICPixelFormat32bppBGRA;CHECK("PNG-format",frame->SetPixelFormat(&format));
    if(!IsEqualGUID(format,GUID_WICPixelFormat32bppBGRA)){failure("unexpected-WIC-format",E_FAIL);return 1;}
    BYTE *bgra=(BYTE*)malloc((size_t)ow*oh*4);if(!bgra){failure("PNG-buffer",E_OUTOFMEMORY);return 1;}
    Free freeBgra{bgra};
    for(UINT y=0;y<oh;++y)for(UINT x=0;x<ow;++x){
        const BYTE *s=(BYTE*)mapped.pData+(size_t)y*mapped.RowPitch+x*4;
        BYTE *d=bgra+((size_t)y*ow+x)*4;d[0]=s[2];d[1]=s[1];d[2]=s[0];d[3]=s[3];
    }
    CHECK("PNG-pixels",frame->WritePixels(oh,ow*4,ow*oh*4,bgra));CHECK("frame-commit",frame->Commit());CHECK("PNG-commit",encoder->Commit());
    STATSTG stats={};CHECK("PNG-stream-size",stream->Stat(&stats,STATFLAG_NONAME));
    if(stats.cbSize.QuadPart==0||stats.cbSize.QuadPart>MAXDWORD){failure("PNG-size-bound",E_FAIL);return 1;}
    HGLOBAL memory=nullptr;CHECK("PNG-memory",GetHGlobalFromStream(stream.p,&memory));
    const void *encoded=GlobalLock(memory);if(!encoded){failure("PNG-memory-lock",E_FAIL);return 1;}
    struct Unlock {HGLOBAL h;~Unlock(){GlobalUnlock(h);}} unlock{memory};
    HANDLE outputFile=CreateFileW(output,GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(outputFile==INVALID_HANDLE_VALUE){failure("PNG-create-new",HRESULT_FROM_WIN32(GetLastError()));return 1;}
    DWORD written=0;BOOL saved=WriteFile(outputFile,encoded,(DWORD)stats.cbSize.QuadPart,&written,nullptr);
    DWORD saveError=saved?ERROR_WRITE_FAULT:GetLastError();
    BOOL closed=CloseHandle(outputFile);
    if(!saved||written!=stats.cbSize.QuadPart||!closed){failure("PNG-write-new",HRESULT_FROM_WIN32(saveError));return 1;}
    printf("{\"status\":\"passed\",\"gpu\":\"");
    for(const wchar_t *s=desc.Description;*s;++s){
        if(*s<32||*s>126||*s=='"'||*s=='\\')printf("\\u%04x",(unsigned)*s);
        else putchar((char)*s);
    }
    printf("\",\"vendor_id\":\"10de\",\"device_id\":%u,\"width\":%u,\"height\":%u,\"output_width\":%u,\"output_height\":%u,\"scale\":4,\"pixelmatch\":true,\"compared_pixels\":%llu,\"scope\":\"offscreen asset QA, not game renderer\"}\n",desc.DeviceId,width,height,ow,oh,(unsigned long long)ow*oh);
    return 0;
}
