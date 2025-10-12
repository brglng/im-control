#include <cstdio>
#include <cstdlib>
#include <windows.h>
#include <msctf.h>
#include "shared_data.hpp"
#include "log.hpp"

static HANDLE g_hMapFile = NULL;
static SharedData* g_pSharedData = NULL;

extern "C" __declspec(dllexport) LRESULT CALLBACK IMControl_WndProcHook(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        CWPSTRUCT* cwp = (CWPSTRUCT*)lParam;
        if (cwp != NULL && g_pSharedData && cwp->hwnd == g_pSharedData->hForegroundWindow && cwp->message == g_pSharedData->uMsg) {
            LOG_INFO("WndProcHook: nCode=0x%x, hwnd=%p, message=0x%x\n", nCode, cwp->hwnd, cwp->message);
            HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
            if (FAILED(hr)) {
	            LOG_ERROR("ERROR: CoInitialize() failed with 0x%0lx", hr);
                return FALSE;
            }

            bool bCOMInitializedByMe = (hr == S_OK);

            ITfInputProcessorProfileMgr* pProfileMgr = NULL;
            hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles,
                                  NULL,
                                  CLSCTX_INPROC_SERVER,
                                  IID_ITfInputProcessorProfileMgr,
                                  (void**)&pProfileMgr);
            if (SUCCEEDED(hr)) {
                LOG_INFO("WndProcHook: pProfileMgr=%p\n", pProfileMgr);
                IEnumTfInputProcessorProfiles* pEnum = nullptr;
                hr = pProfileMgr->EnumProfiles(0, &pEnum);
                if (SUCCEEDED(hr)) {
                    TF_INPUTPROCESSORPROFILE profile;
                    ULONG fetched = 0;
                    while (pEnum->Next(1, &profile, &fetched) == S_OK) {
                        if (profile.guidProfile != GUID_NULL &&
                            IsEqualGUID(profile.guidProfile, g_pSharedData->guidProfile) &&
                            profile.langid != 0) {
                            hr = pProfileMgr->ActivateProfile(profile.dwProfileType,
                                                              profile.langid,
                                                              profile.clsid,
                                                              profile.guidProfile,
                                                              profile.hkl,
                                                              TF_IPPMF_FORPROCESS | TF_IPPMF_DONTCARECURRENTINPUTLANGUAGE);
                            break;
                        }
                    }
                    pEnum->Release();
                }
                pProfileMgr->Release();
            }

            if (bCOMInitializedByMe) {
                CoUninitialize();
            }

            return hr;
        }
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

INT APIENTRY DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    HRESULT hr = S_OK;
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
#ifdef _WIN64
            log_init("hook64");
#else
            log_init("hook32");
#endif
            g_hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, SHARED_DATA_NAME);
            if (g_hMapFile == NULL) {
                LOG_INFO("OpenFileMapping() failed with 0x%0lx\n", GetLastError());
                return FALSE;
            }
            g_pSharedData = (SharedData*)MapViewOfFile(g_hMapFile,
                                                       FILE_MAP_ALL_ACCESS,
                                                       0,
                                                       0,
                                                       sizeof(SharedData));
            if (g_pSharedData == NULL) {
                LOG_INFO("MapViewOfFile() failed with 0x%0lx\n", GetLastError());
                CloseHandle(g_hMapFile);
                g_hMapFile = NULL;
                return FALSE;
            }
            break;
        case DLL_PROCESS_DETACH:
            if (g_pSharedData) {
                UnmapViewOfFile(g_pSharedData);
                g_pSharedData = NULL;
            }
            if (g_hMapFile) {
                CloseHandle(g_hMapFile);
                g_hMapFile = NULL;
            }
            break;
    }
    return TRUE;
}
