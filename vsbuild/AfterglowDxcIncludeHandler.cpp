#include "AfterglowDxcIncludeHandler.h"
#include "AfterglowShaderAsset.h"
#include "AfterglowUtilities.h"
#include "Configurations.h"
#include "DebugUtilities.h"

STDMETHODIMP_(HRESULT __stdcall) AfterglowDxcIncludeHandler::QueryInterface(REFIID riid, void** ppvObject) {
    if (riid == IID_IUnknown || riid == __uuidof(IDxcIncludeHandler)) {
        *ppvObject = this;
        AddRef();
        return S_OK;
    }
    *ppvObject = nullptr;
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG __stdcall) AfterglowDxcIncludeHandler::AddRef() {
    return InterlockedIncrement(&_refCount);
}

STDMETHODIMP_(ULONG __stdcall) AfterglowDxcIncludeHandler::Release() {
    ULONG refCount = InterlockedDecrement(&_refCount);
    if (refCount == 0) {
        delete this;
    }
    return refCount;
}

STDMETHODIMP_(HRESULT __stdcall) AfterglowDxcIncludeHandler::LoadSource(LPCWSTR pFilename, IDxcBlob** ppIncludeSource) {
    Microsoft::WRL::ComPtr<IDxcUtils> pUtils;
    DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&pUtils));
    Microsoft::WRL::ComPtr<IDxcBlobEncoding> pBlob;
    try {
        AfterglowShaderAsset shaderAsset(util::ToString(pFilename));
        // IDxcBlobEncoding
        pUtils->CreateBlob(shaderAsset.code().data(), shaderAsset.code().size(), CP_UTF8, &pBlob);        
    }
    catch (const std::exception& error) {
        DEBUG_CLASS_ERROR(std::format("Failed to include shader: {}", error.what()));
        std::string emptyCode;
        pUtils->CreateBlob(emptyCode.data(), emptyCode.size(), CP_UTF8, &pBlob);
    }
    *ppIncludeSource = pBlob.Detach();
    return S_OK;
}
