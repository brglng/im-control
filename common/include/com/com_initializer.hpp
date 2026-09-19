#ifndef COM_INITIALIZER_HPP
#define COM_INITIALIZER_HPP

#include <windows.h>
#include "com_error.hpp"

// RAII owner for a thread's COM apartment: calls CoInitializeEx in the
// constructor and CoUninitialize in the destructor after every successful
// initialization (S_OK or S_FALSE).
class ComInitializer {
public:
    explicit ComInitializer(DWORD initFlags = COINIT_APARTMENTTHREADED) : m_uninitialize(false) {
        HRESULT hr = CoInitializeEx(nullptr, initFlags);
        if (FAILED(hr)) {
            throw COMError(hr, "CoInitializeEx");
        }
        m_uninitialize = SUCCEEDED(hr);
    }

    ComInitializer(const ComInitializer&) = delete;
    ComInitializer& operator=(const ComInitializer&) = delete;

    ComInitializer(ComInitializer&& other) noexcept : m_uninitialize(other.m_uninitialize) {
        other.m_uninitialize = false;
    }

    ComInitializer& operator=(ComInitializer&& other) noexcept {
        if (this != &other) {
            if (m_uninitialize) {
                CoUninitialize();
            }
            m_uninitialize = other.m_uninitialize;
            other.m_uninitialize = false;
        }
        return *this;
    }

    ~ComInitializer() {
        if (m_uninitialize) {
            CoUninitialize();
        }
    }

private:
    bool m_uninitialize;
};

#endif /* end of include guard: COM_INITIALIZER_HPP */
