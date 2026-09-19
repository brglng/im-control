#ifndef COM_TF_COMPARTMENT_MGR_HPP
#define COM_TF_COMPARTMENT_MGR_HPP

#include <windows.h>
#include <msctf.h>
#include "com/com_ptr.hpp"
#include "com/tf_compartment.hpp"
#include "com_error.hpp"

// Thin wrapper around ITfCompartmentMgr.
class TfCompartmentMgr {
public:
    // Adopts an already-acquired ITfCompartmentMgr reference.
    explicit TfCompartmentMgr(ITfCompartmentMgr* compartmentMgr) noexcept
        : m_compartmentMgr(compartmentMgr) {}

    TfCompartmentMgr(const TfCompartmentMgr&) = delete;
    TfCompartmentMgr& operator=(const TfCompartmentMgr&) = delete;

    TfCompartmentMgr(TfCompartmentMgr&&) noexcept = default;
    TfCompartmentMgr& operator=(TfCompartmentMgr&&) noexcept = default;

    // Returns the compartment identified by the given GUID.
    // Throws COMError when the call fails.
    TfCompartment getCompartment(REFGUID guid) const {
        ITfCompartment* compartment = nullptr;
        HRESULT hr = m_compartmentMgr.get()->GetCompartment(guid, &compartment);
        ComPtr<ITfCompartment> acquiredCompartment(compartment);
        if (FAILED(hr)) {
            throw COMError(hr, "ITfCompartmentMgr::GetCompartment");
        }
        if (!acquiredCompartment) {
            throw COMError(E_POINTER, "ITfCompartmentMgr::GetCompartment");
        }
        return TfCompartment(acquiredCompartment.release());
    }

private:
    ComPtr<ITfCompartmentMgr> m_compartmentMgr;
};

#endif /* end of include guard: COM_TF_COMPARTMENT_MGR_HPP */
