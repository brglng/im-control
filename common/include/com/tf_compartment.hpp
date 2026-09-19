#ifndef COM_TF_COMPARTMENT_HPP
#define COM_TF_COMPARTMENT_HPP

#include <windows.h>
#include <msctf.h>
#include "com/com_ptr.hpp"
#include "com/scoped_variant.hpp"
#include "com_error.hpp"

// Thin wrapper around ITfCompartment.
class TfCompartment {
public:
    explicit TfCompartment(ITfCompartment* compartment) noexcept
        : m_compartment(compartment) {}

    TfCompartment(const TfCompartment&) = delete;
    TfCompartment& operator=(const TfCompartment&) = delete;

    TfCompartment(TfCompartment&&) noexcept = default;
    TfCompartment& operator=(TfCompartment&&) noexcept = default;

    // Reads the compartment value. Throws COMError when the call fails.
    ScopedVariant getValue() const {
        ScopedVariant value;
        HRESULT hr = m_compartment.get()->GetValue(value.get());
        if (FAILED(hr)) {
            throw COMError(hr, "ITfCompartment::GetValue");
        }
        return value;
    }

    // Writes the compartment value. Throws COMError when the call fails.
    void setValue(TfClientId clientId, const VARIANT& value) const {
        HRESULT hr = m_compartment.get()->SetValue(clientId, &value);
        if (FAILED(hr)) {
            throw COMError(hr, "ITfCompartment::SetValue");
        }
    }

    // Convenience writer for the VT_I4 compartments used by TSF.
    void setIntValue(TfClientId clientId, LONG value) const {
        ScopedVariant variant = ScopedVariant::makeInt(value);
        setValue(clientId, *variant.get());
    }

private:
    ComPtr<ITfCompartment> m_compartment;
};

#endif /* end of include guard: COM_TF_COMPARTMENT_HPP */
