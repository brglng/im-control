#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <windows.h>
#include <msctf.h>
#include "argparse.hpp"
#include "err.hpp"
#include "log.hpp"
#include "shared_data.hpp"

int print_usage(const char* exeName) {
    fprintf(stderr, "Usage: %s <hWnd> <dwThreadId> [-langid LANGID] [-guidProfile GUID] [-keyboardOpen|-keyboardClose] [-conversionMode <Alphanumeric|Native[,...]>]\n", exeName);
    return ERR_INVALID_ARGUMENTS;
}

int main(int argc, const char* argv[]) {
    int err = OK;

    if (argc < 3) {
        return print_usage(argv[0]);
    }

    CliArgs args;
    if (parse_args(argc - 3, argv + 3, &args) != 0) {
        return print_usage(argv[0]);
    }

    SetLastError(0);

#ifdef _WIN64
    log_init("injector64");
#else
    log_init("injector32");
#endif

    // Open shared memory for data sharing, and also for ensuring only one instance is running.
    HANDLE hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE,
                                        NULL,
                                        PAGE_READWRITE,
                                        0,
                                        sizeof(SharedData),
                                        SHARED_DATA_NAME);
    if (hMapFile == NULL) {
        LOG_ERROR("CreateFileMapping failed with 0x%lx\n", GetLastError());
        err = ERR_CREATE_FILE_MAPPING;
    }
    if (!err) {
        if (GetLastError() == ERROR_ALREADY_EXISTS) {
            CloseHandle(hMapFile);
            return 0;
        }
    }

    HWND hForegroundWindow = (HWND)std::atoll(argv[1]);
    DWORD dwThreadId = (DWORD)std::atoll(argv[2]);

    UINT uMsg = 0;
    if (!err) {
        // Register a message to be used for the hook.
        uMsg = RegisterWindowMessage("IMControlWndMsg");
        if (uMsg == 0) {
            LOG_ERROR("RegisterWindowMessage(\"IMControlWndMsg\") failed with 0x%lx\n", GetLastError());
            err = ERR_REGISTER_WINDOW_MESSAGE;
        }
    }

    SharedData* pSharedData = NULL;
    if (!err) {
        // Create a shared memory object to store the IME profile data.
        pSharedData = (SharedData*)MapViewOfFile(hMapFile,
                                                 FILE_MAP_ALL_ACCESS,
                                                 0,
                                                 0,
                                                 sizeof(SharedData));
        if (pSharedData == NULL) {
            LOG_ERROR("MapViewOfFile() failed with 0x%lx\n", GetLastError());
            err = ERR_MAP_VIEW_OF_FILE;
        } else {
            new (pSharedData) SharedData();
        }
    }

    std::string dllPath;
    DWORD dllPathBytes = 0;
    if (!err) {
        dllPath.resize(65536);
        dllPathBytes = GetModuleFileNameA(NULL, &dllPath[0], (DWORD)dllPath.size());
        if (dllPathBytes == 0) {
            LOG_ERROR("GetModuleFileName() failed with 0x%lx\n", GetLastError());
            err = ERR_GET_MODULE_FILE_NAME;
        }
    }

    HMODULE hDll = NULL;
    if (!err) {
        dllPath.resize(dllPathBytes);
        size_t pos = dllPath.rfind('\\');
        if (pos != std::string::npos) {
            dllPath.resize(pos + 1);
        }
        dllPath += "im-control-hook-";
#ifdef _WIN64
        dllPath += "64.dll";
        hDll = LoadLibrary(dllPath.c_str());
#else
        dllPath += "32.dll";
        hDll = LoadLibrary(dllPath.c_str());
#endif
        if (hDll == NULL) {
            LOG_ERROR("LoadLibrary(\"im-control-hook.dll\") failed with 0x%lx\n", GetLastError());
            err = ERR_LOAD_LIBRARY;
        }
    }

    HOOKPROC hHookProc = NULL;
    if (!err) {
        hHookProc = (HOOKPROC)GetProcAddress(hDll, "IMControl_WndProcHook");
        if (hHookProc == NULL) {
            LOG_ERROR("GetProcAddress(\"IMControl_WndProcHook\") failed with 0x%lx\n", GetLastError());
            err = ERR_GET_PROC_ADDRESS;
        }
    }

    LPOLESTR wszguidProfile = nullptr;
    int wideSize = 0;
    if (!err) {
        // Initialize the shared data structure.
        pSharedData->hForegroundWindow = hForegroundWindow;
        pSharedData->dwThreadId = dwThreadId;
        pSharedData->uMsg = uMsg;

        if (args.langid) {
            if (args.langid[0] == '0' && (args.langid[1] == 'x' || args.langid[1] == 'X')) {
                pSharedData->langid = (LANGID)std::strtoul(args.langid + 2, NULL, 16);
            } else {
                pSharedData->langid = (LANGID)std::strtoul(args.langid, NULL, 10);
            }
        }
    }

    if (args.guidProfile) {
        wideSize = MultiByteToWideChar(CP_ACP, 0, args.guidProfile, -1, NULL, 0);
        if (wideSize > 0) {
            wszguidProfile = (LPOLESTR)CoTaskMemAlloc(wideSize * sizeof(WCHAR));
            if (!wszguidProfile)
                err = ERR_OUT_OF_MEMORY;
        }
        if (!err) {
            MultiByteToWideChar(CP_ACP, 0, args.guidProfile, -1, wszguidProfile, wideSize);
            pSharedData->guidProfile.emplace();
            HRESULT hr = CLSIDFromString(wszguidProfile, &*pSharedData->guidProfile);
            if (FAILED(hr)) {
                LOG_ERROR("CLSIDFromString(\"%s\") failed with 0x%lx\n", args.guidProfile, hr);
                err = ERR_CLSID_FROM_STRING;
            }
        }
    }

    if (args.keyboardOpenClose) {
        pSharedData->keyboardOpenClose = *args.keyboardOpenClose;
    }

    if (args.conversionMode) {
        const char* mode = strtok((char*)args.conversionMode, ",");
        while (mode != NULL && !err) {
            if (strcmp(mode, "AlphaNumeric") == 0) {
                pSharedData->conversionModeNative = false;
            } else if (strcmp(mode, "Native") == 0) {
                pSharedData->conversionModeNative = true;
            } else {
                pSharedData->conversionModeNative = std::nullopt;
                LOG_ERROR("Invalid conversion mode: %s\n", mode);
                err = ERR_INVALID_ARGUMENTS;
            }
            mode = strtok(NULL, ",");
        }
    }

    HHOOK hHook = NULL;
    if (!err) {
        // Set a hook to intercept the window message.
        hHook = SetWindowsHookEx(WH_CALLWNDPROC, hHookProc, hDll, dwThreadId);
        if (hHook == NULL) {
            LOG_ERROR("SetWindowsHookEx() failed with 0x%lx\n", GetLastError());
            err = ERR_SET_WINDOWS_HOOK_EX;
        }
    }

    if (!err) {
        // Send the input language change request message to the foreground window, and wait for the message to be processed.
        LOG_INFO("Sending message to foreground window: hwnd=%p, message=0x%x\n", hForegroundWindow, uMsg);
        DWORD_PTR dwResult = 0;
        LRESULT lSendResult = SendMessageTimeout(hForegroundWindow,
                                                 uMsg,
                                                 0,
                                                 0,
                                                 SMTO_ABORTIFHUNG,
                                                 10000,
                                                 &dwResult);
        if (lSendResult == 0) {
            DWORD dwError = GetLastError();
            if (dwError == WAIT_TIMEOUT) {
                LOG_ERROR("SendMessageTimeout() timed out\n");
                err = ERR_SEND_MESSAGE_TIMEOUT_TIMED_OUT;
            } else {
                LOG_ERROR("SendMessageTimeout() failed with 0x%lx\n", dwError);
                err = ERR_SEND_MESSAGE_TIMEOUT_FAIL_AFTER_WAIT;
            }
        } else {
            if (dwResult != 0) {
                LOG_ERROR("SendMessageTimeout() returned 0x%llx\n", dwResult);
                err = ERR_SEND_MESSAGE_TIMEOUT;
            }
        }
    }

    if (wszguidProfile != nullptr) {
        CoTaskMemFree(wszguidProfile);
    }
    if (hHook != NULL) {
        UnhookWindowsHookEx(hHook);
    }
    if (hDll != NULL) {
        FreeLibrary(hDll);
    }
    if (pSharedData != NULL) {
        UnmapViewOfFile(pSharedData);
    }
    if (hMapFile != NULL) {
        CloseHandle(hMapFile);
    }

    return err;
}
