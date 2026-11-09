#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <bcrypt.h>
#include "../src/platform/video_player.h"
#include <cstdio>
#include <vector>
#include <cstring>
template<class T>struct Ptr{T*p=nullptr;~Ptr(){if(p)p->Release();}T**out(){return&p;}};
struct QA{Ptr<ID3D11Device>device;Ptr<ID3D11DeviceContext>context;Ptr<ID3D11Texture2D>upload,readback;BCRYPT_ALG_HANDLE algorithm=nullptr;BCRYPT_HASH_HANDLE hash=nullptr;std::vector<unsigned char>hash_object;uint64_t frames=0;FILE*capture=nullptr;bool captured=false;};
static int present(void*opaque,const unsigned char*rgba,size_t n,unsigned w,unsigned h,uint64_t pts){
 auto&q=*(QA*)opaque;if(w!=640||h!=448||n!=(size_t)w*h*4||pts!=q.frames*UINT64_C(1001000000000)/30000)return 0;
 q.context.p->UpdateSubresource(q.upload.p,0,nullptr,rgba,w*4,0);q.context.p->CopyResource(q.readback.p,q.upload.p);D3D11_MAPPED_SUBRESOURCE map={};if(FAILED(q.context.p->Map(q.readback.p,0,D3D11_MAP_READ,0,&map)))return 0;
 bool good=true;for(unsigned y=0;y<h;y++){auto*row=(unsigned char*)map.pData+(size_t)y*map.RowPitch;if(std::memcmp(row,rgba+(size_t)y*w*4,w*4)||BCryptHashData(q.hash,row,w*4,0)<0)good=false;if(q.frames==45&&q.capture&&std::fwrite(row,1,w*4,q.capture)!=w*4)good=false;}q.context.p->Unmap(q.readback.p,0);if(q.frames==45&&q.capture)q.captured=true;q.frames++;return good?1:0;
}
int main(int argc,char**argv){
 if(argc!=3&&argc!=4){std::fprintf(stderr,"usage: movie-qa ISO ffmpeg.exe [new-frame45.rgba]\n");return 2;}QA q;if(argc==4){q.capture=std::fopen(argv[3],"wbx");if(!q.capture)return 2;}
 Ptr<IDXGIFactory1>factory;Ptr<IDXGIAdapter1>adapter;if(FAILED(CreateDXGIFactory1(__uuidof(IDXGIFactory1),(void**)factory.out())))return 2;
 for(UINT i=0;;i++){IDXGIAdapter1*a=nullptr;if(factory.p->EnumAdapters1(i,&a)==DXGI_ERROR_NOT_FOUND)break;DXGI_ADAPTER_DESC1 d={};a->GetDesc1(&d);if(d.VendorId==0x10de&&!(d.Flags&DXGI_ADAPTER_FLAG_SOFTWARE)){adapter.p=a;break;}a->Release();}if(!adapter.p||FAILED(D3D11CreateDevice(adapter.p,D3D_DRIVER_TYPE_UNKNOWN,nullptr,0,nullptr,0,D3D11_SDK_VERSION,q.device.out(),nullptr,q.context.out())))return 2;
 D3D11_TEXTURE2D_DESC desc={};desc.Width=640;desc.Height=448;desc.MipLevels=desc.ArraySize=1;desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;desc.SampleDesc.Count=1;desc.Usage=D3D11_USAGE_DEFAULT;if(FAILED(q.device.p->CreateTexture2D(&desc,nullptr,q.upload.out())))return 2;desc.Usage=D3D11_USAGE_STAGING;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;if(FAILED(q.device.p->CreateTexture2D(&desc,nullptr,q.readback.out())))return 2;
 DWORD bytes=0,n=0;if(BCryptOpenAlgorithmProvider(&q.algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0)<0||BCryptGetProperty(q.algorithm,BCRYPT_OBJECT_LENGTH,(PUCHAR)&bytes,sizeof(bytes),&n,0)<0)return 2;q.hash_object.resize(bytes);if(BCryptCreateHash(q.algorithm,&q.hash,q.hash_object.data(),bytes,nullptr,0,0)<0)return 2;
 gh_video_player*p=nullptr;gh_video_error error=gh_video_open_movie0(argv[1],argv[2],present,&q,&p);if(error==GH_VIDEO_OK)error=gh_video_set_muted(p,1);gh_video_status status={};ULONGLONG start=GetTickCount64();while(error==GH_VIDEO_OK){error=gh_video_poll(p,&status);if(status.complete||GetTickCount64()-start>15000)break;Sleep(1);}gh_video_close(p);
 unsigned char digest[32];bool hashed=BCryptFinishHash(q.hash,digest,sizeof(digest),0)>=0;BCryptDestroyHash(q.hash);BCryptCloseAlgorithmProvider(q.algorithm,0);if(q.capture){if(std::fclose(q.capture)||!q.captured){std::remove(argv[3]);return 1;}}
 std::printf("{\"error\":%u,\"video_eof\":%d,\"audio_done\":%d,\"complete\":%d,\"frames\":%llu,\"pcm_sample_frames\":%llu,\"elapsed_ms\":%llu,\"gpu_sha256\":\"",(unsigned)error,status.video_eof,status.audio_done,status.complete,(unsigned long long)q.frames,(unsigned long long)status.audio_sample_frames,(unsigned long long)(GetTickCount64()-start));if(hashed)for(unsigned char b:digest)std::printf("%02x",b);std::puts("\",\"scope\":\"real ISO movie0 native codec/waveOut and NVIDIA GPU upload-readback; no scene or PS2 decoder equivalence claim\"}");return error==GH_VIDEO_OK&&status.complete&&hashed?0:1;
}
