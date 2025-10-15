#include <cstdio>
#include <cstring>
#include <string>
#include <windows.h>
#include "argparse.hpp"
#include "err.hpp"
#include "log.hpp"
#include "shared_data.hpp"
#include "version.hpp"

int printUsage(const char* exeName) {
    println("Usage: %s [LANGID-{GUID}] [-k|--keyboard <open|close>] [-c|--conversion-mode <alphamumeric|native[,...]>]", exeName);
    println("       %s -l|--list", exeName);
    println("       %s", exeName);
    return ERR_INVALID_ARGUMENTS;
}

int main(int argc, const char *argv[]) {
    int err = 0;

    CliArgs args;
    if (args.parse(argc - 1, argv + 1) != 0) {
        return printUsage(argv[0]);
    }

    if (args.verb == VERB_VERSION) {
        println("%s", VERSION_STRING);
        return 0;
    }

    SetLastError(0);

    logInit("main");

    // Open shared memory for data sharing, and also for ensuring only one instance is running.
    HANDLE hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE,
                                        NULL,
                                        PAGE_READWRITE,
                                        0,
                                        sizeof(SharedData),
                                        SHARED_DATA_NAME);
    if (hMapFile == NULL) {
        LOG_ERROR("CreateFileMapping failed with 0x%lx", GetLastError());
        err = ERR_CREATE_FILE_MAPPING;
    }
    if (!err) {
        if (GetLastError() == ERROR_ALREADY_EXISTS) {
            CloseHandle(hMapFile);
            return ERR_ALREADY_RUNNING;
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
            LOG_ERROR("MapViewOfFile() failed with 0x%lx", GetLastError());
            err = ERR_MAP_VIEW_OF_FILE;
        } else {
            new (pSharedData) SharedData();
        }
    }

    HANDLE hEvent = CreateEventA(NULL, TRUE, FALSE, "Local\\IMControlDoneEvent");
    if (hEvent == NULL) {
        LOG_ERROR("CreateEventA() failed with 0x%lx", GetLastError());
        err = ERR_CREATE_EVENT;
    }

    HWND hForegroundWindow = NULL;
    if (!err) {
        hForegroundWindow = GetForegroundWindow();
        if (hForegroundWindow == nullptr) {
            eprintln("%s: GetForegroundWindow() failed with 0x%lx.", argv[0], GetLastError());
            LOG_ERROR("GetForegroundWindow() failed with 0x%lx", GetLastError());
            err = ERR_GET_FOREGROUND_WINDOW;
        }
    }

    DWORD dwThreadId = 0;
    DWORD dwProcessId = 0;
    if (!err) {
        // Get the thread ID of the foreground window.
        dwThreadId = GetWindowThreadProcessId(hForegroundWindow, &dwProcessId);
        if (dwThreadId == 0) {
            eprintln("%s: GetWindowThreadProcessId() failed with 0x%lx.", argv[0], GetLastError());
	        LOG_ERROR("GetWindowThreadProcessId() failed with 0x%lx", GetLastError());
            err = ERR_GET_WINDOW_THREAD_PROCESS_ID;
        }
    }

    HANDLE hForegroundProcess = nullptr;
    if (!err) {
        hForegroundProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, dwProcessId);
        if (!hForegroundProcess) {
            eprintln("%s: OpenProcess() failed with 0x%lx.", argv[0], GetLastError());
	        LOG_ERROR("OpenProcess() failed with 0x%lx", GetLastError());
            err = ERR_OPEN_PROCESS;
        }
    }

    BOOL isWow64 = FALSE;
    if (!err) {
        if (!IsWow64Process(hForegroundProcess, &isWow64)) {
            eprintln("%s: IsWow64Process() failed with 0x%lx.", argv[0], GetLastError());
            LOG_ERROR("IsWow64Process() failed with 0x%lx", GetLastError());
            err = ERR_IS_WOW64_PROCESS;
        }
    }

    const char* bitness = nullptr;
    std::string injectorPath;
    DWORD injectorPathBytes = 0;
    if (!err) {
        bitness = isWow64 ? "32" : "64";

        injectorPath.resize(65536);
        injectorPathBytes = GetModuleFileNameA(NULL, &injectorPath[0], (DWORD)injectorPath.size());
        if (injectorPathBytes == 0) {
            eprintln("%s: GetModuleFileName() failed with 0x%lx.", argv[0], GetLastError());
            LOG_ERROR("GetModuleFileName() failed with 0x%lx", GetLastError());
            err = ERR_GET_MODULE_FILE_NAME;
        }
    }

    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    if (!err) {
        injectorPath.resize(injectorPathBytes);
        size_t pos = injectorPath.rfind('\\');
        if (pos != std::string::npos) {
            injectorPath.resize(pos + 1);
        }
        injectorPath += "im-control-injector-";
        injectorPath += bitness;
        injectorPath += ".exe";

        std::string commandLine = "\"";
        commandLine += injectorPath;
        commandLine += "\" ";
        commandLine += std::to_string((uintptr_t)hForegroundWindow);
        commandLine += " ";
        commandLine += std::to_string(dwThreadId);
        if (args.id) {
            commandLine += " ";
            commandLine += args.id;
        }
        if (args.keyboardOpenClose) {
            commandLine += " -k ";
            if (*args.keyboardOpenClose) {
                commandLine += "open";
            } else {
                commandLine += "close";
            }
        }
        if (args.conversionMode) {
            commandLine += " -c ";
            commandLine += args.conversionMode;
        }
        commandLine.reserve(65536);

        LOG_INFO("Executing: %s", commandLine.c_str());

        if (!CreateProcessA(injectorPath.c_str(),
                            &commandLine[0],
                            NULL,
                            NULL,
                            FALSE,
                            0,
                            NULL,
                            NULL,
                            &si,
                            &pi)) {
            eprintln("%s: CreateProcessA(\"%s\") failed with 0x%lx.", argv[0], injectorPath.c_str(), GetLastError());
            LOG_ERROR("CreateProcessA(\"%s\") failed with 0x%lx", injectorPath.c_str(), GetLastError());
            err = ERR_CREATE_PROCESS;
        }
    }

    // if (!err) {
    //     LOG_INFO("Waiting for injector to finish...");
    //     while (true) {
    //         DWORD res = WaitForSingleObject(pi.hProcess, 50);
    //         if (res == WAIT_FAILED) {
    //             eprintln("%s: WaitForSingleObject() failed with 0x%lx.", argv[0], GetLastError());
    //             LOG_ERROR("WaitForSingleObject() failed with 0x%lx", GetLastError());
    //             err = ERR_WAIT_FOR_SINGLE_OBJECT;
    //             break;
    //         } else if (res == WAIT_TIMEOUT) {
    //             Sleep(50);
    //         } else {
    //             break;
    //         }
    //     }
    //     LOG_INFO("Injector finished.");
    // }
    //
    // if (!err) {
    //     DWORD exitCode = 0;
    //     if (GetExitCodeProcess(pi.hProcess, &exitCode)) {
    //         if (exitCode != 0) {
    //             eprintln("%s: injector exited with code %lu.", argv[0], exitCode);
    //             LOG_ERROR("injector exited with code %lu", exitCode);
    //             err = (int)exitCode;
    //         }
    //     }
    // }

    if (!err) {
        LOG_INFO("Waiting for injector to finish...");
        DWORD dwWaitResult = WaitForSingleObject(hEvent, INFINITE);
        switch (dwWaitResult) {
            case WAIT_OBJECT_0:
                LOG_INFO("Injector finished.");
                err = (int)pSharedData->err;
                if (err != 0) {
                    eprintln("%s: injector exited with code %d.", argv[0], err);
                    LOG_ERROR("injector exited with code %d", err);
                }
                break;
            case WAIT_FAILED:
                eprintln("%s: WaitForSingleObject() failed with 0x%lx.", argv[0], GetLastError());
                LOG_ERROR("WaitForSingleObject() failed with 0x%lx", GetLastError());
                err = ERR_WAIT_FOR_SINGLE_OBJECT;
                break;
            default:
                eprintln("%s: WaitForSingleObject() returned unexpected value 0x%lx.", argv[0], dwWaitResult);
                LOG_ERROR("WaitForSingleObject() returned unexpected value 0x%lx", dwWaitResult);
                err = ERR_WAIT_FOR_SINGLE_OBJECT;
                break;
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

    LOG_INFO("Cleaning up...");

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hForegroundProcess);

    if (hEvent != NULL) {
        CloseHandle(hEvent);
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

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    BOOL hasConsole = AttachConsole(ATTACH_PARENT_PROCESS);
    if (hasConsole) {
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
        freopen("CONIN$", "r", stdin);
    }

    int ret = main(__argc, const_cast<const char**>(__argv));

    if (hasConsole) {
        fclose(stdout);
        fclose(stderr);
        fclose(stdin);
        FreeConsole();
    }

    return ret;
}
