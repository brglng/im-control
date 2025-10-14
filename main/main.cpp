#include <cstdio>
#include <cstring>
#include <string>
#include <windows.h>
#include "argparse.hpp"
#include "err.hpp"
#include "log.hpp"
#include "version.hpp"

int print_usage(const char* exeName) {
    fprintf(stderr, "Usage: %s [-langid LANGID] [-guidProfile GUID] [-keyboardOpen|-keyboardClose] [-conversionMode <AlphaNumeric|Native[,...]>]\n", exeName);
    fprintf(stderr, "       %s -l\n", exeName);
    fprintf(stderr, "       %s\n", exeName);
    return ERR_INVALID_ARGUMENTS;
}

int main(int argc, const char *argv[]) {
    int err = 0;

    CliArgs args;
    if (args.parse(argc - 1, argv + 1) != 0) {
        return print_usage(argv[0]);
    }

    if (args.verb == VERB_PRINT_VERSION) {
        println("%s", VERSION_STRING);
        return 0;
    }

    SetLastError(0);

    log_init("main");

    HWND hForegroundWindow = GetForegroundWindow();
    if (hForegroundWindow == nullptr) {
        LOG_ERROR("GetForegroundWindow() failed with 0x%lx\n", GetLastError());
        err = ERR_GET_FOREGROUND_WINDOW;
    }

    DWORD dwThreadId = 0;
    DWORD dwProcessId = 0;
    if (!err) {
        // Get the thread ID of the foreground window.
        dwThreadId = GetWindowThreadProcessId(hForegroundWindow, &dwProcessId);
        if (dwThreadId == 0) {
	        LOG_ERROR("GetWindowThreadProcessId() failed with 0x%lx\n", GetLastError());
            err = ERR_GET_WINDOW_THREAD_PROCESS_ID;
        }
    }

    HANDLE hForegroundProcess = nullptr;
    if (!err) {
        hForegroundProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, dwProcessId);
        if (!hForegroundProcess) {
	        LOG_ERROR("OpenProcess() failed with 0x%lx\n", GetLastError());
            err = ERR_OPEN_PROCESS;
        }
    }

    BOOL isWow64 = FALSE;
    if (!err) {
        if (!IsWow64Process(hForegroundProcess, &isWow64)) {
            LOG_ERROR("IsWow64Process() failed with 0x%lx\n", GetLastError());
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
            LOG_ERROR("GetModuleFileName() failed with 0x%lx\n", GetLastError());
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
        if (args.langid) {
            commandLine += " -langid ";
            commandLine += args.langid;
        }
        if (args.guidProfile) {
            commandLine += " -guidProfile ";
            commandLine += args.guidProfile;
        }
        if (args.keyboardOpenClose) {
            if (*args.keyboardOpenClose) {
                commandLine += " -keyboardOpen";
            } else {
                commandLine += " -keyboardClose";
            }
        }
        if (args.conversionMode) {
            commandLine += " -conversionMode ";
            commandLine += args.conversionMode;
        }
        commandLine.reserve(65536);

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
            LOG_ERROR("CreateProcessA(\"%s\") failed with 0x%lx\n", injectorPath.c_str(), GetLastError());
            err = ERR_CREATE_PROCESS;
        }
    }

    if (!err) {
        if (WaitForSingleObject(pi.hProcess, INFINITE) == WAIT_FAILED) {
            LOG_ERROR("WaitForSingleObject() failed with 0x%lx\n", GetLastError());
            err = ERR_WAIT_FOR_SINGLE_OBJECT;
        }
    }

    if (!err) {
        DWORD exitCode = 0;
        if (GetExitCodeProcess(pi.hProcess, &exitCode)) {
            if (exitCode != 0) {
                LOG_ERROR("injector exited with code %lu\n", exitCode);
                err = (int)exitCode;
            }
        }
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hForegroundProcess);

    return err;
}
