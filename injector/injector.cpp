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

int printUsage(const char* exeName) {
    println("Usage: %s <hWnd> <dwThreadId> [LANGID,GUID] [-k|--keyboard <open|close>] [-c|--conversion-mode <alphanumeric|native[,...]>]\n", exeName);
    return ERR_INVALID_ARGUMENTS;
}

int main(int argc, const char* argv[]) {
    int err = OK;

    CliArgs args;
    if (args.parse(argc - 3, argv + 3) != 0) {
        return printUsage(argv[0]);
    }

    SetLastError(0);

#ifdef _WIN64
    logInit("injector64");
#else
    logInit("injector32");
#endif

    // Open shared memory for data sharing, and also for ensuring only one instance is running.
    HANDLE hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE,
                                        NULL,
                                        PAGE_READWRITE,
                                        0,
                                        sizeof(SharedData),
                                        SHARED_DATA_NAME);
    if (hMapFile == NULL) {
        eprintln("%s: CreateFileMapping failed with 0x%lx. Maybe another process is running.\n", argv[0], GetLastError());
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
            eprintln("%s: RegisterWindowMessage(\"IMControlWndMsg\") failed with 0x%lx.\n", argv[0], GetLastError());
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
            eprintln("%s: MapViewOfFile() failed with 0x%lx.\n", argv[0], GetLastError());
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
            eprintln("%s: GetModuleFileName() failed with 0x%lx.\n", argv[0], GetLastError());
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
            eprintln("%s: LoadLibrary(\"%s\") failed with 0x%lx.\n", argv[0], dllPath.c_str(), GetLastError());
            err = ERR_LOAD_LIBRARY;
        }
    }

    HOOKPROC hHookProc = NULL;
    if (!err) {
        hHookProc = (HOOKPROC)GetProcAddress(hDll, "IMControl_WndProcHook");
        if (hHookProc == NULL) {
            LOG_ERROR("GetProcAddress(\"IMControl_WndProcHook\") failed with 0x%lx\n", GetLastError());
            eprintln("%s: GetProcAddress(\"IMControl_WndProcHook\") failed with 0x%lx.\n", argv[0], GetLastError());
            err = ERR_GET_PROC_ADDRESS;
        }
    }

    if (!err) {
        // Initialize the shared data structure.
        pSharedData->hForegroundWindow = hForegroundWindow;
        pSharedData->dwThreadId = dwThreadId;
        pSharedData->uMsg = uMsg;
        pSharedData->verb = args.verb;

        if (args.id) {
            const char* langidStr = nullptr;
            const char* guidStr = nullptr;

            const char* dash = strchr(args.id, '-');
            if (!dash) {
                eprintln("%s: Invalid id format: %s\n", argv[0], args.id);
                LOG_ERROR("Invalid id format: %s\n", args.id);
                err = ERR_INVALID_ARGUMENTS;
            }

            if (!err) {
                langidStr = args.id;
                guidStr = dash + 1;
                pSharedData->langid = (LANGID)std::strtoul(langidStr, NULL, 16);
                if (pSharedData->langid == 0) {
                    LOG_ERROR("Invalid LANGID: %s\n", langidStr);
                    eprintln("%s: Invalid LANGID: %s\n", argv[0], langidStr);
                    err = ERR_INVALID_ARGUMENTS;
                }
            }

            LPOLESTR wszGuid = nullptr;
            int wideSize = 0;
            if (!err) {
                wideSize = MultiByteToWideChar(CP_ACP, 0, guidStr, -1, NULL, 0);
                if (wideSize > 0) {
                    wszGuid = (LPOLESTR)CoTaskMemAlloc(wideSize * sizeof(WCHAR));
                    if (!wszGuid)
                        err = ERR_OUT_OF_MEMORY;
                }
            }

            if (!err) {
                MultiByteToWideChar(CP_ACP, 0,  guidStr, -1, wszGuid, wideSize);
                pSharedData->guidProfile.emplace();
                HRESULT hr = CLSIDFromString(wszGuid, &*pSharedData->guidProfile);
                if (FAILED(hr)) {
                    LOG_ERROR("CLSIDFromString(\"%s\") failed with 0x%lx\n", guidStr, hr);
                    eprintln("%s: CLSIDFromString(\"%s\") failed with 0x%lx.\n", argv[0], guidStr, hr);
                    err = ERR_CLSID_FROM_STRING;
                }
            }

            if (wszGuid != nullptr) {
                CoTaskMemFree(wszGuid);
            }
        }
    }

    if (args.keyboardOpenClose) {
        pSharedData->keyboardOpenClose = *args.keyboardOpenClose;
    }

    if (args.conversionMode) {
        const char* mode = strtok((char*)args.conversionMode, ",");
        while (mode != NULL && !err) {
            if (strcmp(mode, "alphanumeric") == 0) {
                pSharedData->conversionModeNative = false;
            } else if (strcmp(mode, "native") == 0) {
                pSharedData->conversionModeNative = true;
            } else {
                pSharedData->conversionModeNative = std::nullopt;
                LOG_ERROR("Invalid conversion mode: %s\n", mode);
                eprintln("%s: Invalid conversion mode: %s\n", argv[0], mode);
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
            eprintln("%s: SetWindowsHookEx() failed with 0x%lx.\n", argv[0], GetLastError());
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
                eprintln("%s: SendMessageTimeout() timed out.\n", argv[0]);
                err = ERR_SEND_MESSAGE_TIMEOUT_TIMED_OUT;
            } else {
                LOG_ERROR("SendMessageTimeout() failed with 0x%lx\n", dwError);
                eprintln("%s: SendMessageTimeout() failed with 0x%lx.\n", argv[0], dwError);
                err = ERR_SEND_MESSAGE_TIMEOUT_FAIL_AFTER_WAIT;
            }
        } else {
            if (dwResult != 0) {
                LOG_ERROR("SendMessageTimeout() returned 0x%llx\n", (uint64_t)dwResult);
                eprintln("%s: SendMessageTimeout() returned 0x%llx.\n", argv[0], (uint64_t)dwResult);
                err = ERR_SEND_MESSAGE_TIMEOUT;
            }
        }
    }

    if (!err) {
        if (args.verb == VERB_CURRENT) {
            println("%04X-{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
                *pSharedData->langid,
                pSharedData->guidProfile->Data1,
                pSharedData->guidProfile->Data2,
                pSharedData->guidProfile->Data3,
                pSharedData->guidProfile->Data4[0], pSharedData->guidProfile->Data4[1],
                pSharedData->guidProfile->Data4[2], pSharedData->guidProfile->Data4[3],
                pSharedData->guidProfile->Data4[4], pSharedData->guidProfile->Data4[5],
                pSharedData->guidProfile->Data4[6], pSharedData->guidProfile->Data4[7]
            );
        }
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
