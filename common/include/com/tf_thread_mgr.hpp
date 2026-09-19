#ifndef COM_TF_THREAD_MGR_HPP
#define COM_TF_THREAD_MGR_HPP

#include <utility>
#include <windows.h>
#include <msctf.h>
#include "com/com_ptr.hpp"
#include "com/tf_compartment_mgr.hpp"
#include "com_error.hpp"
#include "winapi_error.hpp"

// Thin wrapper around ITfThreadMgr.
//
// TF_GetThreadMgr is exported by msctf.dll as a per-thread ThreadMgr singleton.
// On Windows 11, CoCreateInstance(CLSID_TF_ThreadMgr) returns a NEW instance
// instead of the per-thread singleton, so compartment writes don't trigger
// OnChange notifications on sinks registered on the framework-provided instance.
// TF_GetThreadMgr reliably returns the per-thread singleton on both Win10/Win11.
class TfThreadMgr {
public:
    // Returns the per-thread ThreadMgr singleton by looking up TF_GetThreadMgr
    // in msctf.dll dynamically. Throws WinAPIError when the module or the
    // export cannot be loaded, and COMError when TF_GetThreadMgr fails.
    static TfThreadMgr getThreadMgrSingleton() {
        HMODULE module = GetModuleHandleW(L"msctf.dll");
        bool ownsModule = false;
        if (!module) {
            module = LoadLibraryW(L"msctf.dll");
            if (!module) {
                throw WinAPIError(GetLastError(), "LoadLibraryW(msctf.dll)");
            }
            ownsModule = true;
        }

        try {
            auto getThreadMgr = reinterpret_cast<PFN_TF_GetThreadMgr>(GetProcAddress(module, "TF_GetThreadMgr"));
            if (!getThreadMgr) {
                throw WinAPIError(GetLastError(), "GetProcAddress(msctf.dll, TF_GetThreadMgr)");
            }
            ITfThreadMgr* threadMgr = nullptr;
            HRESULT hr = getThreadMgr(&threadMgr);
            ComPtr<ITfThreadMgr> acquiredThreadMgr(threadMgr);
            if (FAILED(hr)) {
                throw COMError(hr, "TF_GetThreadMgr");
            }
            if (!acquiredThreadMgr) {
                throw COMError(E_POINTER, "TF_GetThreadMgr");
            }
            return TfThreadMgr(acquiredThreadMgr.release(), module, ownsModule);
        } catch (...) {
            if (ownsModule) {
                FreeLibrary(module);
            }
            throw;
        }
    }

    TfThreadMgr(const TfThreadMgr&) = delete;
    TfThreadMgr& operator=(const TfThreadMgr&) = delete;

    TfThreadMgr(TfThreadMgr&& other) noexcept
        : m_threadMgr(std::move(other.m_threadMgr)),
          m_module(other.m_module),
          m_ownsModule(other.m_ownsModule),
          m_clientId(other.m_clientId) {
        other.m_module = nullptr;
        other.m_ownsModule = false;
        other.m_clientId = TF_CLIENTID_NULL;
    }

    TfThreadMgr& operator=(TfThreadMgr&& other) noexcept {
        if (this != &other) {
            release();
            m_threadMgr = std::move(other.m_threadMgr);
            m_module = other.m_module;
            m_ownsModule = other.m_ownsModule;
            m_clientId = other.m_clientId;
            other.m_module = nullptr;
            other.m_ownsModule = false;
            other.m_clientId = TF_CLIENTID_NULL;
        }
        return *this;
    }

    ~TfThreadMgr() {
        release();
    }

    // Activates the thread manager for this client and returns the client ID.
    // The manager is deactivated automatically in the destructor.
    TfClientId activate() {
        TfClientId clientId = TF_CLIENTID_NULL;
        HRESULT hr = m_threadMgr.get()->Activate(&clientId);
        if (FAILED(hr)) {
            throw COMError(hr, "ITfThreadMgr::Activate");
        }
        m_clientId = clientId;
        return clientId;
    }

    // Queries the compartment manager of this thread manager.
    // Throws COMError when the interface is not available.
    TfCompartmentMgr compartmentMgr() const {
        ITfCompartmentMgr* compartmentMgr = nullptr;
        HRESULT hr = m_threadMgr.get()->QueryInterface(IID_ITfCompartmentMgr, reinterpret_cast<void**>(&compartmentMgr));
        ComPtr<ITfCompartmentMgr> acquiredCompartmentMgr(compartmentMgr);
        if (FAILED(hr)) {
            throw COMError(hr, "ITfThreadMgr::QueryInterface(IID_ITfCompartmentMgr)");
        }
        if (!acquiredCompartmentMgr) {
            throw COMError(E_POINTER, "ITfThreadMgr::QueryInterface(IID_ITfCompartmentMgr)");
        }
        return TfCompartmentMgr(acquiredCompartmentMgr.release());
    }

private:
    typedef HRESULT(WINAPI* PFN_TF_GetThreadMgr)(ITfThreadMgr**);

    TfThreadMgr(ITfThreadMgr* threadMgr, HMODULE module, bool ownsModule) noexcept
        : m_threadMgr(threadMgr),
          m_module(module),
          m_ownsModule(ownsModule),
          m_clientId(TF_CLIENTID_NULL) {}

    void release() noexcept {
        if (m_clientId != TF_CLIENTID_NULL && m_threadMgr) {
            m_threadMgr.get()->Deactivate();
            m_clientId = TF_CLIENTID_NULL;
        }
        m_threadMgr.reset();
        if (m_ownsModule && m_module) {
            FreeLibrary(m_module);
            m_module = nullptr;
            m_ownsModule = false;
        }
    }

    ComPtr<ITfThreadMgr> m_threadMgr;
    HMODULE m_module;
    bool m_ownsModule;
    TfClientId m_clientId;
};

#endif /* end of include guard: COM_TF_THREAD_MGR_HPP */
