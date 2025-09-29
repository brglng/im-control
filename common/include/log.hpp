#ifndef LOG_HPP
#define LOG_HPP

#include <windows.h>

enum LogLevel {
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR
};

inline constexpr const char* level_to_string(LogLevel level) {
    switch (level) {
        case LOG_LEVEL_DEBUG: return "DEBUG";
        case LOG_LEVEL_INFO:  return "INFO";
        case LOG_LEVEL_WARN:  return "WARN";
        case LOG_LEVEL_ERROR: return "ERROR";
        default:              return "UNKNOWN";
    }
}

void log_init(const char* name);
void log(LogLevel level, const char* format, ...);

#define LOG(level, format, ...) do { \
    DWORD processId = GetCurrentProcessId(); \
    DWORD threadId = GetCurrentThreadId(); \
    log(level, "[%lu][%lu][%s] " format, processId, threadId, level_to_string(level), ##__VA_ARGS__); \
} while (0)

#define LOG_DEBUG(format, ...) LOG(LOG_LEVEL_DEBUG, format, ##__VA_ARGS__)
#define LOG_INFO(format, ...)  LOG(LOG_LEVEL_INFO,  format, ##__VA_ARGS__)
#define LOG_WARN(format, ...)  LOG(LOG_LEVEL_WARN,  format, ##__VA_ARGS__)
#define LOG_ERROR(format, ...) LOG(LOG_LEVEL_ERROR, format, ##__VA_ARGS__)

#endif /* LOG_HPP */
