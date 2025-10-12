#ifndef SHARED_DATA_HPP
#define SHARED_DATA_HPP

#include <windows.h>
#include <msctf.h>

typedef struct {
    HWND    hForegroundWindow;
    DWORD   dwThreadId; 
    UINT    uMsg;
    GUID    guidProfile;
} SharedData;

#define SHARED_DATA_NAME "Local\\IMControlSharedData"

#endif /* SHARED_DATA_HPP */
