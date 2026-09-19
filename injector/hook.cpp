#include <cstdio>
#include <cstdlib>
#include <exception>
#include <windows.h>
#include <msctf.h>
#include "com/com_initializer.hpp"
#include "com/enum_tf_input_processor_profiles.hpp"
#include "com/scoped_variant.hpp"
#include "com/tf_compartment.hpp"
#include "com/tf_compartment_mgr.hpp"
#include "com/tf_input_processor_profile_mgr.hpp"
#include "com/tf_thread_mgr.hpp"
#include "com_error.hpp"
#include "log.hpp"
#include "shared_data.hpp"
#include "winapi_error.hpp"

static HANDLE g_hMapFile = NULL;
static HANDLE g_hEvent = NULL;
static SharedData* g_pSharedData = NULL;
static bool g_isWeaselToggleImeOnOpenClose = false;

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

// Applies the requested input method state inside the hooked thread.
// COM and Win32 failures are reported by throwing; the hook callback catches
// them at the boundary and returns a failure code without signalling the
// done event, preserving the original protocol.
static void applyInputMethodState() {
    ComInitializer comInitializer;
    LOG_INFO("COM initialized");

    TfInputProcessorProfileMgr profileMgr;
    TF_INPUTPROCESSORPROFILE prevProfile = profileMgr.getActiveProfile(GUID_TFCAT_TIP_KEYBOARD);

    if (g_pSharedData->verb == VERB_SWITCH) {
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

        if (targetGuidProfile) {
            EnumTfInputProcessorProfiles enumerator = profileMgr.enumProfiles(0);
            TF_INPUTPROCESSORPROFILE profile = {};
            while (enumerator.next(profile)) {
                if (!IsEqualGUID(profile.catid, GUID_TFCAT_TIP_KEYBOARD) || !(profile.dwFlags & TF_IPP_FLAG_ENABLED)) {
                    continue;
                }
                if (targetLangId == 0 || profile.langid != targetLangId ||
                    !IsEqualGUID(profile.guidProfile, *targetGuidProfile)) {
                    continue;
                }
                LOG_INFO("guidProfile = {%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
                         profile.guidProfile.Data1,
                         profile.guidProfile.Data2,
                         profile.guidProfile.Data3,
                         profile.guidProfile.Data4[0], profile.guidProfile.Data4[1],
                         profile.guidProfile.Data4[2], profile.guidProfile.Data4[3],
                         profile.guidProfile.Data4[4], profile.guidProfile.Data4[5],
                         profile.guidProfile.Data4[6], profile.guidProfile.Data4[7]);
                profileMgr.activateProfile(profile.dwProfileType,
                                           profile.langid,
                                           profile.clsid,
                                           profile.guidProfile,
                                           profile.hkl,
                                           TF_IPPMF_FORPROCESS | TF_IPPMF_DONTCARECURRENTINPUTLANGUAGE);
                LOG_INFO("Profile switched successfully");
                break;
            }
        }
    }

    g_pSharedData->langid = prevProfile.langid;
    g_pSharedData->guidProfile = prevProfile.guidProfile;

    if (g_pSharedData->getKeyboardState) {
        // Failures while querying the compartments are logged but do not fail the hook.
        try {
            TfThreadMgr threadMgr = TfThreadMgr::getThreadMgrSingleton();
            TfCompartmentMgr compartmentMgr = threadMgr.compartmentMgr();

            TfCompartment keyboardOpenCloseCompartment =
                compartmentMgr.getCompartment(GUID_COMPARTMENT_KEYBOARD_OPENCLOSE);
            ScopedVariant keyboardOpenClose = keyboardOpenCloseCompartment.getValue();
            if (keyboardOpenClose.isInt()) {
                g_pSharedData->keyboardOpenClose = (keyboardOpenClose.asInt() != 0);
            }

            TfCompartment conversionCompartment =
                compartmentMgr.getCompartment(GUID_COMPARTMENT_KEYBOARD_INPUTMODE_CONVERSION);
            ScopedVariant conversionMode = conversionCompartment.getValue();
            if (conversionMode.isInt()) {
                g_pSharedData->conversionModeNative = (conversionMode.asInt() & TF_CONVERSIONMODE_NATIVE) != 0;
            }
        } catch (const std::exception& e) {
            LOG_ERROR("ERROR: Failed to query keyboard state: %s", e.what());
        }
    }

    if ((g_pSharedData->verb == VERB_SWITCH) && !g_pSharedData->getKeyboardState &&
        (g_pSharedData->keyboardOpenClose || g_pSharedData->conversionModeNative)) {
        TfThreadMgr threadMgr = TfThreadMgr::getThreadMgrSingleton();
        TfCompartmentMgr compartmentMgr = threadMgr.compartmentMgr();
        TfClientId clientId = threadMgr.activate();

        if (g_pSharedData->keyboardOpenClose) {
            LOG_INFO("keyboardOpenClose = %d", *g_pSharedData->keyboardOpenClose);
            TfCompartment keyboardOpenCloseCompartment =
                compartmentMgr.getCompartment(GUID_COMPARTMENT_KEYBOARD_OPENCLOSE);
            bool needWrite = true;
            ScopedVariant currentValue;
            try {
                currentValue = keyboardOpenCloseCompartment.getValue();
            } catch (const COMError& e) {
                LOG_ERROR("ERROR: %s", e.what());
            }
            if (currentValue.isInt()) {
                bool currentOpen = (currentValue.asInt() != 0);
                needWrite = (currentOpen != *g_pSharedData->keyboardOpenClose);
                if (!needWrite) {
                    LOG_INFO("OPENCLOSE already %d, skipping SetValue", currentOpen ? 1 : 0);
                }
            }
            if (needWrite) {
                keyboardOpenCloseCompartment.setIntValue(clientId, *g_pSharedData->keyboardOpenClose ? 1 : 0);
            }
        }

        if (g_pSharedData->conversionModeNative) {
            LOG_INFO("conversionModeNative = %d", *g_pSharedData->conversionModeNative);
            TfCompartment conversionCompartment =
                compartmentMgr.getCompartment(GUID_COMPARTMENT_KEYBOARD_INPUTMODE_CONVERSION);
            ScopedVariant currentValue = conversionCompartment.getValue();
            LONG oldMode = currentValue.isInt() ? currentValue.asInt() : 0;
            LONG newMode = oldMode;
            if (*g_pSharedData->conversionModeNative) {
                newMode |= TF_CONVERSIONMODE_NATIVE;
            } else {
                newMode &= ~TF_CONVERSIONMODE_NATIVE;
            }
            if (newMode != oldMode) {
                conversionCompartment.setIntValue(clientId, newMode);
            }
        }
    }

    if (!SetEvent(g_hEvent)) {
        LOG_ERROR("SetEvent() failed with 0x%lx", GetLastError());
        g_pSharedData->err = ERR_SET_EVENT;
    } else {
        LOG_INFO("SetEvent() succeeded");
    }
}

extern "C" __declspec(dllexport) LRESULT CALLBACK IMControl_WndProcHook(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        CWPSTRUCT* cwp = (CWPSTRUCT*)lParam;
        if (cwp != NULL && g_pSharedData && cwp->hwnd == g_pSharedData->hForegroundWindow && cwp->message == g_pSharedData->uMsg) {
            LOG_INFO("WndProcHook: nCode=0x%x, hwnd=%p, message=0x%x", nCode, cwp->hwnd, cwp->message);

            // Never let an exception escape the hook callback.
            HRESULT hr = S_OK;
            try {
                applyInputMethodState();
            } catch (const COMError& e) {
                LOG_ERROR("ERROR: %s", e.what());
                hr = e.code();
            } catch (const WinAPIError& e) {
                LOG_ERROR("ERROR: %s", e.what());
                hr = HRESULT_FROM_WIN32(e.code());
            } catch (const std::exception& e) {
                LOG_ERROR("ERROR: %s", e.what());
                hr = E_FAIL;
            } catch (...) {
                LOG_ERROR("ERROR: Unknown exception");
                hr = E_FAIL;
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

            g_isWeaselToggleImeOnOpenClose = ReadToggleImeOnOpenClose();

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
