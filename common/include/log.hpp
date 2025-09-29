#ifndef LOG_HPP
#define LOG_HPP

#include <cstdarg>

void log_init(const char* name);
void vlog(const char* format, va_list args);
void log(const char* format, ...);

#endif /* LOG_HPP */
