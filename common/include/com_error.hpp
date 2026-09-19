#ifndef COM_ERROR_HPP
#define COM_ERROR_HPP

#include <cstdio>
#include <stdexcept>
#include <string>
#include <utility>
#include <windows.h>

// Exception thrown when a COM call returns a failure HRESULT.
// Carries the HRESULT and the name of the failing operation.
class COMError : public std::runtime_error {
public:
    COMError(HRESULT hr, std::string operation)
        : std::runtime_error(buildMessage(hr, operation)),
          m_hr(hr),
          m_operation(std::move(operation)) {}

    HRESULT code() const noexcept {
        return m_hr;
    }

    const std::string& operation() const noexcept {
        return m_operation;
    }

private:
    static std::string buildMessage(HRESULT hr, const std::string& operation) {
        char buffer[64];
        std::snprintf(buffer, sizeof(buffer), " failed with HRESULT 0x%lx", static_cast<unsigned long>(hr));
        return operation + buffer;
    }

    HRESULT m_hr;
    std::string m_operation;
};

#endif /* end of include guard: COM_ERROR_HPP */
