#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <windows.h>
#include <msctf.h>
#include "err.hpp"
#include "log.hpp"
#include "shared_data.hpp"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    int err = OK;

    SetLastError(0);

#ifdef _WIN64
    logInit("injector64");
#else
    logInit("injector32");
#endif

    HANDLE hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, SHARED_DATA_NAME);
    if (hMapFile == NULL) {
        LOG_ERROR("OpenFileMapping() failed with 0x%lx", GetLastError());
        err = ERR_OPEN_FILE_MAPPING;
    }

    SharedData* pSharedData = NULL;
    if (!err) {
        // Map the shared memory object to the process's address space.
        pSharedData = (SharedData*)MapViewOfFile(hMapFile,
                                                 FILE_MAP_ALL_ACCESS,
                                                 0,
                                                 0,
                                                 sizeof(SharedData));
        if (pSharedData == NULL) {
            LOG_ERROR("MapViewOfFile() failed with 0x%lx", GetLastError());
            err = ERR_MAP_VIEW_OF_FILE;
        }
    }

    HWND hForegroundWindow = pSharedData->hForegroundWindow;
    DWORD dwThreadId = pSharedData->dwThreadId;
    UINT uMsg = pSharedData->uMsg;

    std::string dllPath;
    DWORD dllPathBytes = 0;
    if (!err) {
        dllPath.resize(65536);
        dllPathBytes = GetModuleFileNameA(NULL, &dllPath[0], (DWORD)dllPath.size());
        if (dllPathBytes == 0) {
            LOG_ERROR("GetModuleFileName() failed with 0x%lx", GetLastError());
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
            LOG_ERROR("LoadLibrary(\"im-control-hook.dll\") failed with 0x%lx", GetLastError());
            err = ERR_LOAD_LIBRARY;
        }
    }

    HOOKPROC hHookProc = NULL;
    if (!err) {
        hHookProc = (HOOKPROC)GetProcAddress(hDll, "IMControl_WndProcHook");
        if (hHookProc == NULL) {
            LOG_ERROR("GetProcAddress(\"IMControl_WndProcHook\") failed with 0x%lx", GetLastError());
            err = ERR_GET_PROC_ADDRESS;
        }
    }

    HHOOK hHook = NULL;
    if (!err) {
        // Set a hook to intercept the window message.
        hHook = SetWindowsHookEx(WH_CALLWNDPROC, hHookProc, hDll, dwThreadId);
        if (hHook == NULL) {
            LOG_ERROR("SetWindowsHookEx() failed with 0x%lx", GetLastError());
            err = ERR_SET_WINDOWS_HOOK_EX;
        }
    }

    if (!err) {
        // Send the input language change request message to the foreground window, and wait for the message to be processed.
        LOG_INFO("Sending message to foreground window: hwnd=%p, message=0x%x", hForegroundWindow, uMsg);
        DWORD_PTR dwResult = 0;
        LRESULT lSendResult = SendMessageTimeout(hForegroundWindow,
                                                 uMsg,
                                                 0,
                                                 0,
                                                 SMTO_ABORTIFHUNG,
                                                 10000,
                                                 &dwResult);
        LOG_INFO("SendMessageTimeout() returned 0x%lx, result=0x%llx", lSendResult, (uint64_t)dwResult);
        if (lSendResult == 0) {
            DWORD dwError = GetLastError();
            if (dwError == WAIT_TIMEOUT) {
                LOG_WARN("SendMessageTimeout() timed out");
                err = ERR_SEND_MESSAGE_TIMEOUT_TIMED_OUT;
            } else {
                LOG_ERROR("SendMessageTimeout() failed with 0x%lx", dwError);
                err = ERR_SEND_MESSAGE_TIMEOUT_FAIL_AFTER_WAIT;
            }
        } else {
            if (dwResult != 0) {
                LOG_ERROR("SendMessageTimeout() returned 0x%llx", (uint64_t)dwResult);
                err = ERR_SEND_MESSAGE_TIMEOUT;
            }
        }
    }

    LOG_INFO("Cleaning up...");

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

    LOG_INFO("Exiting with code %d", err);

    return err;
}
