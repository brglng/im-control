#ifndef COM_BSTR_HPP
#define COM_BSTR_HPP

#include <oleauto.h>

// RAII owner for a BSTR returned by a COM API.
// Frees the string with SysFreeString in the destructor.
class Bstr {
public:
    Bstr() noexcept : m_bstr(nullptr) {}
    explicit Bstr(BSTR bstr) noexcept : m_bstr(bstr) {}

    Bstr(const Bstr&) = delete;
    Bstr& operator=(const Bstr&) = delete;

    Bstr(Bstr&& other) noexcept : m_bstr(other.m_bstr) {
        other.m_bstr = nullptr;
    }

    Bstr& operator=(Bstr&& other) noexcept {
        if (this != &other) {
            reset(other.m_bstr);
            other.m_bstr = nullptr;
        }
        return *this;
    }

    ~Bstr() {
        reset();
    }

    void reset(BSTR bstr = nullptr) noexcept {
        if (m_bstr) {
            SysFreeString(m_bstr);
        }
        m_bstr = bstr;
    }

    BSTR get() const noexcept {
        return m_bstr;
    }

    const wchar_t* c_str() const noexcept {
        return m_bstr;
    }

    explicit operator bool() const noexcept {
        return m_bstr != nullptr;
    }

private:
    BSTR m_bstr;
};

#endif /* end of include guard: COM_BSTR_HPP */
