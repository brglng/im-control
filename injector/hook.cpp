#include <cstdio>
#include <cstdlib>
#include <windows.h>
#include <msctf.h>
#include "shared_data.hpp"
#include "log.hpp"

// TF_GetThreadMgr is exported by msctf.dll as a per-thread ThreadMgr singleton.
// On Windows 11, CoCreateInstance(CLSID_TF_ThreadMgr) returns a NEW instance
// instead of the per-thread singleton, so compartment writes don't trigger
// OnChange notifications on sinks registered on the framework-provided instance.
// TF_GetThreadMgr reliably returns the per-thread singleton on both Win10/Win11.
typedef HRESULT(WINAPI* PFN_TF_GetThreadMgr)(ITfThreadMgr**);

static ITfThreadMgr* GetThreadMgrSingleton() {
    HMODULE hMsctf = GetModuleHandleW(L"msctf.dll");
    if (!hMsctf) {
        hMsctf = LoadLibraryW(L"msctf.dll");
    }
    if (!hMsctf) {
        return nullptr;
    }
    auto pfn = (PFN_TF_GetThreadMgr)GetProcAddress(hMsctf, "TF_GetThreadMgr");
    if (!pfn) {
        return nullptr;
    }
    ITfThreadMgr* pThreadMgr = nullptr;
    HRESULT hr = pfn(&pThreadMgr);
    if (FAILED(hr) || !pThreadMgr) {
        return nullptr;
    }
    return pThreadMgr;
}

static HANDLE g_hMapFile = NULL;
static HANDLE g_hEvent = NULL;
static SharedData* g_pSharedData = NULL;
static bool g_isToOpenClose = false;

static bool ReadToggleImeOnOpenClose() {
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Rime\\weasel", 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        return false;
    char value[8] = {0};
    DWORD valueSize = sizeof(value);
    DWORD type = 0;
    bool result = false;
    if (RegQueryValueExA(hKey, "ToggleImeOnOpenClose", NULL, &type, (LPBYTE)value, &valueSize) == ERROR_SUCCESS
        && type == REG_SZ) {
        result = (_stricmp(value, "yes") == 0);
    }
    RegCloseKey(hKey);
    return result;
}

extern "C" __declspec(dllexport) LRESULT CALLBACK IMControl_WndProcHook(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        CWPSTRUCT* cwp = (CWPSTRUCT*)lParam;
        if (cwp != NULL && g_pSharedData && cwp->hwnd == g_pSharedData->hForegroundWindow && cwp->message == g_pSharedData->uMsg) {
            LOG_INFO("WndProcHook: nCode=0x%x, hwnd=%p, message=0x%x", nCode, cwp->hwnd, cwp->message);
            HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
            if (FAILED(hr)) {
	            LOG_ERROR("ERROR: CoInitialize() failed with 0x%0lx", hr);
                return FALSE;
            }

            bool bCOMInitializedByMe = (hr == S_OK);
            ITfInputProcessorProfileMgr* pProfileMgr = NULL;
            ITfThreadMgr* pThreadMgr = NULL;
            ITfCompartmentMgr* pCompartmentMgr = nullptr;
            TfClientId clientId = TF_CLIENTID_NULL;

            if (SUCCEEDED(hr)) {
                LOG_INFO("COM initialized");
                hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles,
                                      NULL,
                                      CLSCTX_ALL,
                                      IID_ITfInputProcessorProfileMgr,
                                      (void**)&pProfileMgr);
            }

            TF_INPUTPROCESSORPROFILE prevProfile;
            if (SUCCEEDED(hr)) {
                hr = pProfileMgr->GetActiveProfile(GUID_TFCAT_TIP_KEYBOARD, &prevProfile);
            }

            if (g_pSharedData->verb == VERB_SWITCH) {
                if (SUCCEEDED(hr)) {
                    LANGID targetLangId = 0;
                    const GUID* targetGuidProfile = nullptr;
                    if (g_pSharedData->ifLangId && g_pSharedData->ifGuidProfile) {
                        if (*g_pSharedData->ifLangId == prevProfile.langid &&
                            IsEqualGUID(*g_pSharedData->ifGuidProfile, prevProfile.guidProfile)) {
                            targetLangId = g_pSharedData->langid ? *g_pSharedData->langid : 0;
                            targetGuidProfile = g_pSharedData->guidProfile ? &(*g_pSharedData->guidProfile) : nullptr;
                        } else if (g_pSharedData->elseLangId && g_pSharedData->elseGuidProfile) {
                            targetLangId = *g_pSharedData->elseLangId;
                            targetGuidProfile = &(*g_pSharedData->elseGuidProfile);
                        } else {
                            LOG_INFO("Condition not met, skipping profile switch");
                            hr = S_OK;
                        }
                    } else {
                        if (g_pSharedData->langid && g_pSharedData->guidProfile) {
                            targetLangId = *g_pSharedData->langid;
                            targetGuidProfile = &(*g_pSharedData->guidProfile);
                        } else {
                            LOG_INFO("No target profile specified, skipping profile switch");
                        }
                    }

                    LOG_INFO("targetLangId=0x%04X", targetLangId);
                    if (targetGuidProfile) {
                        LOG_INFO("targetGuidProfile = {%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
                                 targetGuidProfile->Data1,
                                 targetGuidProfile->Data2,
                                 targetGuidProfile->Data3,
                                 targetGuidProfile->Data4[0], targetGuidProfile->Data4[1],
                                 targetGuidProfile->Data4[2], targetGuidProfile->Data4[3],
                                 targetGuidProfile->Data4[4], targetGuidProfile->Data4[5],
                                 targetGuidProfile->Data4[6], targetGuidProfile->Data4[7]);
                    } else {
                        LOG_INFO("targetGuidProfile = nullptr");
                    }

                    LOG_INFO("WndProcHook: pProfileMgr=%p", pProfileMgr);
                    IEnumTfInputProcessorProfiles* pEnum = nullptr;
                    hr = pProfileMgr->EnumProfiles(0, &pEnum);
                    if (SUCCEEDED(hr)) {
                        TF_INPUTPROCESSORPROFILE profile;
                        ULONG fetched = 0;
                        while (pEnum->Next(1, &profile, &fetched) == S_OK) {
                            if (IsEqualGUID(profile.catid, GUID_TFCAT_TIP_KEYBOARD) && (profile.dwFlags & TF_IPP_FLAG_ENABLED)) {
                                if (targetLangId != 0 && targetGuidProfile != nullptr &&
                                    profile.langid == targetLangId && IsEqualGUID(profile.guidProfile, *targetGuidProfile))
                                {
                                    LOG_INFO("guidProfile = {%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
                                             profile.guidProfile.Data1,
                                             profile.guidProfile.Data2,
                                             profile.guidProfile.Data3,
                                             profile.guidProfile.Data4[0], profile.guidProfile.Data4[1],
                                             profile.guidProfile.Data4[2], profile.guidProfile.Data4[3],
                                             profile.guidProfile.Data4[4], profile.guidProfile.Data4[5],
                                             profile.guidProfile.Data4[6], profile.guidProfile.Data4[7]);
                                    hr = pProfileMgr->ActivateProfile(profile.dwProfileType,
                                                                      profile.langid,
                                                                      profile.clsid,
                                                                      profile.guidProfile,
                                                                      profile.hkl,
                                                                      TF_IPPMF_FORPROCESS | TF_IPPMF_DONTCARECURRENTINPUTLANGUAGE);
                                    if (SUCCEEDED(hr)) {
                                        LOG_INFO("Profile switched successfully");
                                    } else {
                                        LOG_ERROR("ERROR: ActivateProfile() failed with 0x%0lx", hr);
                                    }
                                    break;
                                }
                            }
                        }
                        pEnum->Release();
                    } else {
                        LOG_ERROR("ERROR: EnumProfiles() failed with 0x%0lx", hr);
                    }
                } else {
                    LOG_ERROR("ERROR: CoCreateInstance(CLSID_TF_InputProcessorProfiles) failed with 0x%0lx", hr);
                }
            }

            if (SUCCEEDED(hr)) {
                g_pSharedData->langid = prevProfile.langid;
                g_pSharedData->guidProfile = prevProfile.guidProfile;
            }

            if (g_pSharedData->getKeyboardState) {
                ITfThreadMgr* pThreadMgrGet = NULL;
                ITfCompartmentMgr* pCompartmentMgrGet = nullptr;

                pThreadMgrGet = GetThreadMgrSingleton();
                HRESULT hrGet = pThreadMgrGet ? S_OK : E_FAIL;
                if (SUCCEEDED(hrGet)) {
                    hrGet = pThreadMgrGet->QueryInterface(IID_ITfCompartmentMgr, (void**)&pCompartmentMgrGet);
                }

                if (SUCCEEDED(hrGet)) {
                    ITfCompartment* keyboardOpenCloseCompartment = nullptr;
                    hrGet = pCompartmentMgrGet->GetCompartment(GUID_COMPARTMENT_KEYBOARD_OPENCLOSE, &keyboardOpenCloseCompartment);
                    if (SUCCEEDED(hrGet)) {
                        VARIANT varKeyboardOpenClose;
                        VariantInit(&varKeyboardOpenClose);
                        hrGet = keyboardOpenCloseCompartment->GetValue(&varKeyboardOpenClose);
                        if (SUCCEEDED(hrGet) && (varKeyboardOpenClose.vt == VT_I4 || varKeyboardOpenClose.vt == VT_UI4)) {
                            g_pSharedData->keyboardOpenClose = (varKeyboardOpenClose.lVal != 0);
                        }
                        VariantClear(&varKeyboardOpenClose);
                        keyboardOpenCloseCompartment->Release();
                    }
                }

                if (SUCCEEDED(hrGet)) {
                    ITfCompartment* conversionCompartment = nullptr;
                    hrGet = pCompartmentMgrGet->GetCompartment(GUID_COMPARTMENT_KEYBOARD_INPUTMODE_CONVERSION, &conversionCompartment);
                    if (SUCCEEDED(hrGet)) {
                        VARIANT varConv;
                        VariantInit(&varConv);
                        hrGet = conversionCompartment->GetValue(&varConv);
                        if (SUCCEEDED(hrGet) && (varConv.vt == VT_I4 || varConv.vt == VT_UI4)) {
                            g_pSharedData->conversionModeNative = (varConv.lVal & TF_CONVERSIONMODE_NATIVE) != 0;
                        }
                        VariantClear(&varConv);
                        conversionCompartment->Release();
                    }
                }

                if (pCompartmentMgrGet) {
                    pCompartmentMgrGet->Release();
                }
                if (pThreadMgrGet) {
                    pThreadMgrGet->Release();
                }
                // Intentionally do not modify `hr` here: get-keyboard failures should not override prior errors.
            }

            if ((g_pSharedData->verb == VERB_SWITCH) && !g_pSharedData->getKeyboardState && (g_pSharedData->keyboardOpenClose || g_pSharedData->conversionModeNative)) {
                pThreadMgr = GetThreadMgrSingleton();
                hr = pThreadMgr ? S_OK : E_FAIL;
                if (SUCCEEDED(hr)) {
                    hr = pThreadMgr->QueryInterface(IID_ITfCompartmentMgr, (void**)&pCompartmentMgr);
                } else {
                    LOG_ERROR("ERROR: GetThreadMgrSingleton() failed");
                }

                if (SUCCEEDED(hr)) {
                    hr = pThreadMgr->Activate(&clientId);
                    if (FAILED(hr)) {
                        LOG_ERROR("ERROR: ITfThreadMgr::Activate() failed with 0x%0lx", hr);
                    }
                }

                if (SUCCEEDED(hr)) {
                    if (g_pSharedData->keyboardOpenClose) {
                        LOG_INFO("keyboardOpenClose = %d", *g_pSharedData->keyboardOpenClose);
                        ITfCompartment* keyboardOpenCloseCompartment = nullptr;
                        if (SUCCEEDED(hr)) {
                            hr = pCompartmentMgr->GetCompartment(GUID_COMPARTMENT_KEYBOARD_OPENCLOSE, &keyboardOpenCloseCompartment);
                        } else {
                            LOG_ERROR("ERROR: QueryInterface(IID_ITfCompartmentMgr) failed with 0x%0lx", hr);
                        }
                        VARIANT varKeyboardOpenClose;
                        VariantInit(&varKeyboardOpenClose);
                        if (SUCCEEDED(hr)) {
                            VARIANT varCurrent;
                            VariantInit(&varCurrent);
                            bool needWrite = true;
                            if (SUCCEEDED(keyboardOpenCloseCompartment->GetValue(&varCurrent)) &&
                                (varCurrent.vt == VT_I4 || varCurrent.vt == VT_UI4)) {
                                bool currentOpen = (varCurrent.lVal != 0);
                                needWrite = (currentOpen != *g_pSharedData->keyboardOpenClose);
                                if (!needWrite) {
                                    LOG_INFO("OPENCLOSE already %d, skipping SetValue", currentOpen ? 1 : 0);
                                }
                            }
                            VariantClear(&varCurrent);
                            if (needWrite) {
                                varKeyboardOpenClose.vt = VT_I4;
                                if (*g_pSharedData->keyboardOpenClose) {
                                    varKeyboardOpenClose.lVal = 1;
                                } else {
                                    varKeyboardOpenClose.lVal = 0;
                                }
                                hr = keyboardOpenCloseCompartment->SetValue(clientId, &varKeyboardOpenClose);
                            }
                        } else {
                            LOG_ERROR("ERROR: GetCompartment(GUID_COMPARTMENT_KEYBOARD_OPENCLOSE) failed with 0x%0lx", hr);
                        }
                        if (FAILED(hr)) {
                            LOG_ERROR("ERROR: SetValue() failed with 0x%0lx", hr);
                        }
                        VariantClear(&varKeyboardOpenClose);
                        if (keyboardOpenCloseCompartment) {
                            keyboardOpenCloseCompartment->Release();
                        }
                    }

                    if (g_pSharedData->conversionModeNative) {
                        LOG_INFO("conversionModeNative = %d", *g_pSharedData->conversionModeNative);
                        ITfCompartment* keyboardInputModeConversionCompartment = nullptr;
                        if (SUCCEEDED(hr)) {
                            hr = pCompartmentMgr->GetCompartment(GUID_COMPARTMENT_KEYBOARD_INPUTMODE_CONVERSION, &keyboardInputModeConversionCompartment);
                        } else {
                            LOG_ERROR("ERROR: QueryInterface(IID_ITfCompartmentMgr) failed with 0x%0lx", hr);
                        }
                        VARIANT varKeyboardInputModeConversion;
                        VariantInit(&varKeyboardInputModeConversion);
                        varKeyboardInputModeConversion.vt = VT_EMPTY;
                        if (SUCCEEDED(hr)) {
                            hr = keyboardInputModeConversionCompartment->GetValue(&varKeyboardInputModeConversion);
                        } else {
                            LOG_ERROR("ERROR: GetCompartment(GUID_COMPARTMENT_KEYBOARD_INPUTMODE_CONVERSION) failed with 0x%0lx", hr);
                        }
                        if (SUCCEEDED(hr)) {
                            DWORD oldMode = 0;
                            if (varKeyboardInputModeConversion.vt == VT_I4 || varKeyboardInputModeConversion.vt == VT_UI4) {
                                oldMode = varKeyboardInputModeConversion.lVal;
                            }
                            DWORD newMode = oldMode;
                            if  (*g_pSharedData->conversionModeNative) {
                                newMode |= TF_CONVERSIONMODE_NATIVE;
                            } else {
                                newMode &= ~TF_CONVERSIONMODE_NATIVE;
                            }
                            if (newMode != oldMode) {
                                varKeyboardInputModeConversion.vt = VT_I4;
                                varKeyboardInputModeConversion.lVal = newMode;
                                hr = keyboardInputModeConversionCompartment->SetValue(clientId, &varKeyboardInputModeConversion);
                            }
                        } else {
                            LOG_ERROR("ERROR: GetValue() failed with 0x%0lx", hr);
                        }
                        if (FAILED(hr)) {
                            LOG_ERROR("ERROR: SetValue() failed with 0x%0lx", hr);
                        }
                        VariantClear(&varKeyboardInputModeConversion);
                        if (keyboardInputModeConversionCompartment) {
                            keyboardInputModeConversionCompartment->Release();
                        }
                    }

                    if (g_isToOpenClose && g_pSharedData->conversionModeNative && !g_pSharedData->keyboardOpenClose) {
                        ITfCompartment* ocCompartment = nullptr;
                        HRESULT hrOC = pCompartmentMgr->GetCompartment(GUID_COMPARTMENT_KEYBOARD_OPENCLOSE, &ocCompartment);
                        if (SUCCEEDED(hrOC)) {
                            VARIANT varOC;
                            VariantInit(&varOC);
                            hrOC = ocCompartment->GetValue(&varOC);
                            if (SUCCEEDED(hrOC) && (varOC.vt == VT_I4 || varOC.vt == VT_UI4) && varOC.lVal == 0) {
                                VARIANT varSet;
                                VariantInit(&varSet);
                                varSet.vt = VT_I4;
                                varSet.lVal = 1;
                                ocCompartment->SetValue(clientId, &varSet);
                            }
                            VariantClear(&varOC);
                            ocCompartment->Release();
                        }
                    }
                } else {
                    LOG_ERROR("ERROR: QueryInterface(IID_ITfCompartmentMgr) failed with 0x%0lx", hr);
                }
            }

            if (SUCCEEDED(hr)) {
                if (!SetEvent(g_hEvent)) {
                    LOG_ERROR("SetEvent() failed with 0x%lx", GetLastError());
                    g_pSharedData->err = ERR_SET_EVENT;
                } else {
                    LOG_INFO("SetEvent() succeeded");
                }
            }

            if (pCompartmentMgr) {
                pCompartmentMgr->Release();
            }
            if (pThreadMgr) {
                if (clientId != TF_CLIENTID_NULL) {
                    pThreadMgr->Deactivate();
                }
                pThreadMgr->Release();
            }
            if (pProfileMgr) {
                pProfileMgr->Release();
            }
            if (bCOMInitializedByMe) {
                CoUninitialize();
            }

            LOG_INFO("Done");

            return hr;
        }
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

INT APIENTRY DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
#ifdef _WIN64
            logInit("hook64");
#else
            logInit("hook32");
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

            g_hEvent = OpenEventA(EVENT_MODIFY_STATE, FALSE, "Local\\IMControlDoneEvent");
            if (g_hEvent == NULL) {
                LOG_ERROR("OpenEventA() failed with 0x%lx", GetLastError());
                UnmapViewOfFile(g_pSharedData);
                g_pSharedData = NULL;
                CloseHandle(g_hMapFile);
                g_hMapFile = NULL;
                return FALSE;
            }

            g_isToOpenClose = ReadToggleImeOnOpenClose();

            break;
        case DLL_PROCESS_DETACH:
            if (g_hEvent) {
                CloseHandle(g_hEvent);
                g_hEvent = NULL;
            }
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
