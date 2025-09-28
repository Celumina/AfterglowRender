#pragma once
#include <d3d12shader.h>
#include <dxc/dxcapi.h>
#include <wrl/client.h>

// Windows only.
class AfterglowDxcIncludeHandler :public IDxcIncludeHandler {
public:
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void** ppvObject) override;

    STDMETHODIMP_(ULONG) AddRef() override;

    STDMETHODIMP_(ULONG) Release() override;

    // IDxcIncludeHandler
    STDMETHODIMP LoadSource(LPCWSTR pFilename, IDxcBlob** ppIncludeSource) override;

private:
    ULONG _refCount = 1;
};

