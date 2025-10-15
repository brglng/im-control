#ifndef SHARED_DATA_HPP
#define SHARED_DATA_HPP

#include <optional>
#include <windows.h>
#include <msctf.h>
#include "err.hpp"
#include "verb.hpp"

struct SharedData {
    HWND                    hForegroundWindow;
    DWORD                   dwThreadId; 
    UINT                    uMsg;
    Verb                    verb;
    std::optional<LANGID>   langid;
    std::optional<GUID>     guidProfile;
    std::optional<bool>     keyboardOpenClose;
    std::optional<bool>     conversionModeNative;
    Err                     err;

    SharedData() :
        hForegroundWindow(NULL),
        dwThreadId(0),
        uMsg(0),
        verb(VERB_CURRENT),
        langid(),
        guidProfile(),
        keyboardOpenClose(),
        conversionModeNative(),
        err(OK)
    {}
};

#define SHARED_DATA_NAME "Local\\IMControlSharedData"

#endif /* SHARED_DATA_HPP */
