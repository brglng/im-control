#ifndef SHARED_DATA_HPP
#define SHARED_DATA_HPP

#include <optional>
#include <windows.h>
#include <msctf.h>

struct SharedData {
    HWND                    hForegroundWindow;
    DWORD                   dwThreadId; 
    UINT                    uMsg;
    std::optional<LANGID>   langid;
    std::optional<GUID>     guidProfile;
    std::optional<bool>     keyboardOpenClose;
    std::optional<bool>     conversionModeNative;

    SharedData() :
        hForegroundWindow(NULL),
        dwThreadId(0),
        uMsg(0),
        langid(),
        guidProfile(),
        keyboardOpenClose(),
        conversionModeNative()
    {}
};

#define SHARED_DATA_NAME "Local\\IMControlSharedData"

#endif /* SHARED_DATA_HPP */
