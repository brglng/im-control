#ifndef WINAPI_ERROR_HPP
#define WINAPI_ERROR_HPP

#include <cstdio>
#include <stdexcept>
#include <string>
#include <utility>
#include <windows.h>

// Exception thrown when a Win32 API call fails.
// Carries the GetLastError() code and the name of the failing operation.
class WinAPIError : public std::runtime_error {
public:
    WinAPIError(DWORD code, std::string operation)
        : std::runtime_error(buildMessage(code, operation)),
          m_code(code),
          m_operation(std::move(operation)) {}

    DWORD code() const noexcept {
        return m_code;
    }

    const std::string& operation() const noexcept {
        return m_operation;
    }

private:
    static std::string buildMessage(DWORD code, const std::string& operation) {
        char buffer[64];
        std::snprintf(buffer, sizeof(buffer), " failed with Win32 error 0x%lx", code);
        return operation + buffer;
    }

    DWORD m_code;
    std::string m_operation;
};

#endif /* end of include guard: WINAPI_ERROR_HPP */
