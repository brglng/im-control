#include <clocale>
#include <conio.h>
#include <cstdarg>
#include <cstdio>
#include <cwchar>
#include <mutex>
#include <shlobj.h>
#include <windows.h>
#include "log.hpp"

class LogFile {
    FILE* m_file;
public:
    LogFile() : m_file(nullptr) {}

    void init(const char* name) {
        std::string filename;
        filename.resize(65536);
        HRESULT hr = SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA | CSIDL_FLAG_CREATE, NULL, SHGFP_TYPE_CURRENT, &filename[0]);
        if (SUCCEEDED(hr)) {
            filename.resize(strlen(filename.c_str()));
            filename += "\\im-control";
            if (CreateDirectoryA(filename.c_str(), NULL) || GetLastError() == ERROR_ALREADY_EXISTS) {
                filename += '\\';
                filename += name;
                filename += ".log";
                m_file = fopen(filename.c_str(), "w");
            }
        }
    }

    LogFile(const LogFile&) = delete;
    LogFile& operator=(const LogFile&) = delete;

    LogFile(LogFile&& other) noexcept : m_file(other.m_file) {
        other.m_file = nullptr;
    }

    LogFile& operator=(LogFile&& other) noexcept {
        if (this != &other) {
            if (m_file) {
                fclose(m_file);
            }
            m_file = other.m_file;
            other.m_file = nullptr;
        }
        return *this;
    }

    ~LogFile() {
        if (m_file) {
            fclose(m_file);
            m_file = nullptr;
        }
    }

    FILE* get() const {
        return m_file;
    }
};

static std::once_flag log_init_flag{};
static LogFile log_file;

static bool g_consoleActive = false;

#ifdef IM_CONTROL_GUI
static HWND g_consoleWindow = nullptr;
static bool g_consoleOwned = false;
static bool g_consoleVisible = false;
#endif

void logInit(const char* name) {
    std::call_once(log_init_flag, [name]() {
        log_file.init(name);
    });
}

static bool ensureConsole() {
    if (g_consoleActive) {
        return true;
    }

#ifdef IM_CONTROL_GUI
    BOOL allocated = AllocConsole();
    HWND consoleWindow = GetConsoleWindow();
    if (!allocated && consoleWindow == nullptr) {
        return false;
    }

    g_consoleActive = true;
    g_consoleOwned = allocated == TRUE;
    g_consoleWindow = consoleWindow;
    g_consoleVisible = !g_consoleOwned;
    if (g_consoleOwned) {
        SetConsoleOutputCP(CP_UTF8);
        if (g_consoleWindow) {
            ShowWindow(g_consoleWindow, SW_HIDE);
        }
    }

    FILE* stream = nullptr;
    freopen_s(&stream, "CONIN$", "r", stdin);
    freopen_s(&stream, "CONOUT$", "w", stdout);
    freopen_s(&stream, "CONOUT$", "w", stderr);
    setvbuf(stdout, nullptr, _IONBF, 0);
    setvbuf(stderr, nullptr, _IONBF, 0);
#else
    g_consoleActive = true;
#endif

    if (GetConsoleOutputCP() == CP_UTF8) {
        setlocale(LC_CTYPE, ".UTF8");
    } else {
        setlocale(LC_CTYPE, "");
    }
    return true;
}

void consoleShow() {
    if (ensureConsole()) {
#ifdef IM_CONTROL_GUI
        if (g_consoleOwned && g_consoleWindow && !g_consoleVisible) {
            ShowWindow(g_consoleWindow, SW_SHOWNOACTIVATE);
            g_consoleVisible = true;
        }
#endif
    }
}

void consoleFree() {
#ifdef IM_CONTROL_GUI
    if (g_consoleActive && g_consoleOwned) {
        printf("\nPress any key to exit...\n");
        getch();
        fflush(stdout);
        fflush(stderr);
        fclose(stdout);
        fclose(stderr);
        fclose(stdin);
        FreeConsole();
    }
    g_consoleActive = false;
    g_consoleWindow = nullptr;
    g_consoleOwned = false;
    g_consoleVisible = false;
#endif
}

static void vconsolePrint(FILE* file, const char* format, va_list args) {
    if (file == stdout || file == stderr) {
        consoleShow();
    }
    vfprintf(file, format, args);
    fflush(file);
}

void consolePrint(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vconsolePrint(stdout, format, args);
    va_end(args);
}

void consolePrintW(const wchar_t* format, ...) {
    consoleShow();

    va_list args;
    va_start(args, format);
    vwprintf(format, args);
    va_end(args);
    fflush(stdout);
}

void consoleError(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vconsolePrint(stderr, format, args);
    va_end(args);
}

void consoleFilePrint(FILE* file, const char* format, ...) {
    va_list args;
    va_start(args, format);
    vconsolePrint(file, format, args);
    va_end(args);
}

static void vlog(LogLevel level, const char* format, va_list args) {
    vfprintf(log_file.get(), format, args);
    fputc('\n', log_file.get());
    fflush(log_file.get());
}

void log(LogLevel level, const char* format, ...) {
    if (log_file.get()) {
        va_list args;
        va_start(args, format);
        vlog(level, format, args);
        va_end(args);
    }
}
