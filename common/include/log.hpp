#ifndef LOG_HPP
#define LOG_HPP

#include <ctime>
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

#define LOG(file, line, func, level, format, ...) do { \
    struct tm localTime; \
    time_t now = time(nullptr); \
    localtime_s(&localTime, &now); \
    char timeBuffer[20]; \
    strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", &localTime); \
    DWORD processId = GetCurrentProcessId(); \
    DWORD threadId = GetCurrentThreadId(); \
    log(level, "[%s][%lu][%lu][%s:%d:%s][%s] " format, timeBuffer, processId, threadId, file, line, func, level_to_string(level), ##__VA_ARGS__); \
} while (0)

#define LOG_DEBUG(format, ...) LOG(__FILE__, __LINE__, __FUNCTION__, LOG_LEVEL_DEBUG, format, ##__VA_ARGS__)
#define LOG_INFO(format, ...)  LOG(__FILE__, __LINE__, __FUNCTION__, LOG_LEVEL_INFO,  format, ##__VA_ARGS__)
#define LOG_WARN(format, ...)  LOG(__FILE__, __LINE__, __FUNCTION__, LOG_LEVEL_WARN,  format, ##__VA_ARGS__)
#define LOG_ERROR(format, ...) LOG(__FILE__, __LINE__, __FUNCTION__, LOG_LEVEL_ERROR, format, ##__VA_ARGS__)

#endif /* LOG_HPP */
