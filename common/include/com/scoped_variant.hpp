#ifndef COM_SCOPED_VARIANT_HPP
#define COM_SCOPED_VARIANT_HPP

#include <windows.h>
#include <oleauto.h>

// RAII owner for a VARIANT: initialized in the constructor and cleared in the destructor.
class ScopedVariant {
public:
    ScopedVariant() noexcept {
        VariantInit(&m_variant);
    }

    ScopedVariant(const ScopedVariant&) = delete;
    ScopedVariant& operator=(const ScopedVariant&) = delete;

    ScopedVariant(ScopedVariant&& other) noexcept : m_variant(other.m_variant) {
        VariantInit(&other.m_variant);
    }

    ScopedVariant& operator=(ScopedVariant&& other) noexcept {
        if (this != &other) {
            reset();
            m_variant = other.m_variant;
            VariantInit(&other.m_variant);
        }
        return *this;
    }

    ~ScopedVariant() {
        VariantClear(&m_variant);
    }

    void reset() noexcept {
        VariantClear(&m_variant);
        VariantInit(&m_variant);
    }

    VARIANT* get() noexcept {
        return &m_variant;
    }

    bool isInt() const noexcept {
        return m_variant.vt == VT_I4 || m_variant.vt == VT_UI4;
    }

    LONG asInt() const noexcept {
        return m_variant.lVal;
    }

    static ScopedVariant makeInt(LONG value) {
        ScopedVariant variant;
        variant.m_variant.vt = VT_I4;
        variant.m_variant.lVal = value;
        return variant;
    }

private:
    VARIANT m_variant;
};

#endif /* end of include guard: COM_SCOPED_VARIANT_HPP */
