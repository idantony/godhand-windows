#ifndef GH_D3D11_ADAPTER_H
#define GH_D3D11_ADAPTER_H

#include <d3d11.h>
#include <dxgi.h>

#include <climits>
#include <cstdio>
#include <memory>

namespace gh_d3d11_detail {

struct ReleaseAdapter {
    void operator()(IDXGIAdapter1 *adapter) const { adapter->Release(); }
};

struct AdapterSelection {
    HRESULT status = DXGI_ERROR_NOT_FOUND;
    LUID luid = {};
};

inline AdapterSelection select_hardware_adapter(IDXGIFactory1 *factory)
{
    wchar_t index_text[11] = {};
    const DWORD count = GetEnvironmentVariableW(
        L"GODHAND_D3D11_ADAPTER_INDEX", index_text, ARRAYSIZE(index_text));
    if (count >= ARRAYSIZE(index_text))
        return {E_INVALIDARG, {}};
    UINT requested = 0;
    for (DWORD i = 0; i < count; ++i) {
        if (index_text[i] < L'0' || index_text[i] > L'9')
            return {E_INVALIDARG, {}};
        const UINT digit = static_cast<UINT>(index_text[i] - L'0');
        if (requested > (UINT_MAX - digit) / 10)
            return {E_INVALIDARG, {}};
        requested = requested * 10 + digit;
    }

    constexpr UINT nvidia = 0x10DE;
    constexpr D3D_FEATURE_LEVEL feature_level = D3D_FEATURE_LEVEL_11_0;
    AdapterSelection fallback;
    UINT fallback_index = 0;
    DXGI_ADAPTER_DESC1 fallback_desc = {};
    for (UINT index = 0; ; ++index) {
        IDXGIAdapter1 *raw = nullptr;
        HRESULT status = factory->EnumAdapters1(index, &raw);
        if (status == DXGI_ERROR_NOT_FOUND)
            break;
        if (FAILED(status))
            return {status, {}};
        std::unique_ptr<IDXGIAdapter1, ReleaseAdapter> adapter(raw);
        if (count && index != requested)
            continue;

        DXGI_ADAPTER_DESC1 desc = {};
        status = adapter->GetDesc1(&desc);
        if (FAILED(status))
            return {status, {}};
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            continue;
        if (!count && desc.VendorId != nvidia && SUCCEEDED(fallback.status))
            continue;

        status = D3D11CreateDevice(adapter.get(), D3D_DRIVER_TYPE_UNKNOWN, nullptr, 0,
                                  &feature_level, 1, D3D11_SDK_VERSION, nullptr, nullptr, nullptr);
        if (FAILED(status)) {
            if (count)
                return {status, {}};
            continue;
        }
        fallback = {S_OK, desc.AdapterLuid};
        fallback_desc = desc;
        fallback_index = index;
        if (count || desc.VendorId == nvidia)
            break;
    }

    if (SUCCEEDED(fallback.status))
        std::fprintf(stderr, "D3D11 adapter %u vendor=%04X device=%04X luid=%08X:%08X\n",
                     fallback_index, fallback_desc.VendorId, fallback_desc.DeviceId,
                     static_cast<unsigned>(fallback.luid.HighPart),
                     static_cast<unsigned>(fallback.luid.LowPart));
    return fallback;
}

} // namespace gh_d3d11_detail

// All native renderers have separate devices but keep the same adapter for the process.
inline HRESULT gh_d3d11_hardware_adapter(IDXGIFactory1 *factory, IDXGIAdapter1 **output)
{
    *output = nullptr;
    static const auto selected = gh_d3d11_detail::select_hardware_adapter(factory);
    if (FAILED(selected.status))
        return selected.status;
    for (UINT index = 0; ; ++index) {
        IDXGIAdapter1 *raw = nullptr;
        HRESULT status = factory->EnumAdapters1(index, &raw);
        if (FAILED(status))
            return status;
        std::unique_ptr<IDXGIAdapter1, gh_d3d11_detail::ReleaseAdapter> adapter(raw);
        DXGI_ADAPTER_DESC1 desc = {};
        status = adapter->GetDesc1(&desc);
        if (FAILED(status))
            return status;
        if (desc.AdapterLuid.LowPart == selected.luid.LowPart &&
            desc.AdapterLuid.HighPart == selected.luid.HighPart) {
            *output = adapter.release();
            return S_OK;
        }
    }
}

#endif
