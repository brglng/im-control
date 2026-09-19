#ifndef COM_PTR_HPP
#define COM_PTR_HPP

#include <unknwn.h>

// Minimal owning smart pointer for a COM interface.
// Releases the interface in the destructor; non-copyable and movable.
template <typename T>
class ComPtr {
public:
    ComPtr() noexcept : m_ptr(nullptr) {}
    explicit ComPtr(T* ptr) noexcept : m_ptr(ptr) {}

    ComPtr(const ComPtr&) = delete;
    ComPtr& operator=(const ComPtr&) = delete;

    ComPtr(ComPtr&& other) noexcept : m_ptr(other.m_ptr) {
        other.m_ptr = nullptr;
    }

    ComPtr& operator=(ComPtr&& other) noexcept {
        if (this != &other) {
            reset(other.m_ptr);
            other.m_ptr = nullptr;
        }
        return *this;
    }

    ~ComPtr() {
        reset();
    }

    void reset(T* ptr = nullptr) noexcept {
        if (m_ptr) {
            m_ptr->Release();
        }
        m_ptr = ptr;
    }

    T* get() const noexcept {
        return m_ptr;
    }

    T* release() noexcept {
        T* ptr = m_ptr;
        m_ptr = nullptr;
        return ptr;
    }

    T* operator->() const noexcept {
        return m_ptr;
    }

    explicit operator bool() const noexcept {
        return m_ptr != nullptr;
    }

private:
    T* m_ptr;
};

#endif /* end of include guard: COM_PTR_HPP */
