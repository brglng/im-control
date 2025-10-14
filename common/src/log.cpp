#include <cstdarg>
#include <cstdio>
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
                filename += std::move(name);
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

void logInit(const char* name) {
    std::call_once(log_init_flag, [name]() {
        log_file.init(name);
    });
}

static void vlog(LogLevel level, const char* format, va_list args) {
    vfprintf(log_file.get(), format, args);
    fputc('\n', log_file.get());
    fflush(log_file.get());
}

void log(LogLevel level, const char* format, ...) {
    if (log_file.get() && level >= LOG_LEVEL_INFO) {
        va_list args;
        va_start(args, format);
        vlog(level, format, args);
        va_end(args);
    }
}
