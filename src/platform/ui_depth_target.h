#ifndef GH_UI_DEPTH_TARGET_H
#define GH_UI_DEPTH_TARGET_H
#include "ui_renderer.h"
#include <d3d11.h>
#include <cstring>

/* This is the same OT depth in a format each existing native UI device
 * can update. No extra raster pass or inference from a changed color. */
class NativeUIDepthTarget {
    ID3D11Texture2D *texture=nullptr,*readback=nullptr;
    ID3D11UnorderedAccessView *uav=nullptr;
    void release(){
        if(uav)uav->Release();
        if(texture)texture->Release();
        if(readback)readback->Release();
        uav=nullptr;texture=readback=nullptr;
    }
public:
    NativeUIDepthTarget()=default;
    NativeUIDepthTarget(const NativeUIDepthTarget&)=delete;
    ~NativeUIDepthTarget(){release();}
    ID3D11UnorderedAccessView *view()const{return uav;}
    gh_ui_error begin(ID3D11Device *dev,ID3D11DeviceContext *ctx,unsigned w,unsigned h,const gh_ui_depth_write &plane){
        if(!plane.pixels||plane.count!=(size_t)w*h||plane.z>65535)return GH_UI_ARGUMENT;
        if(!uav){
            D3D11_TEXTURE2D_DESC d={};d.Width=w;d.Height=h;d.MipLevels=d.ArraySize=1;
            d.Format=DXGI_FORMAT_R32_UINT;d.SampleDesc.Count=1;
            d.Usage=D3D11_USAGE_DEFAULT;d.BindFlags=D3D11_BIND_UNORDERED_ACCESS;
            if(FAILED(dev->CreateTexture2D(&d,nullptr,&texture))||
               FAILED(dev->CreateUnorderedAccessView(texture,nullptr,&uav))){release();return GH_UI_GPU;}
            d.Usage=D3D11_USAGE_STAGING;d.BindFlags=0;d.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
            if(FAILED(dev->CreateTexture2D(&d,nullptr,&readback))){release();return GH_UI_GPU;}
        }
        ctx->UpdateSubresource(texture,0,nullptr,plane.pixels,w*sizeof(uint32_t),0);return GH_UI_OK;
    }
    gh_ui_error finish(ID3D11DeviceContext *ctx,unsigned w,unsigned h,const gh_ui_depth_write &plane){
        ctx->CopyResource(readback,texture);D3D11_MAPPED_SUBRESOURCE m={};
        if(FAILED(ctx->Map(readback,0,D3D11_MAP_READ,0,&m)))return GH_UI_GPU;
        for(unsigned y=0;y<h;y++)std::memcpy(plane.pixels+(size_t)y*w,
            (const uint8_t*)m.pData+(size_t)y*m.RowPitch,(size_t)w*sizeof(uint32_t));
        ctx->Unmap(readback,0);return GH_UI_OK;
    }
};
#endif
