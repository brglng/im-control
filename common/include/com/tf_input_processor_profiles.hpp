#ifndef COM_TF_INPUT_PROCESSOR_PROFILES_HPP
#define COM_TF_INPUT_PROCESSOR_PROFILES_HPP

#include <utility>
#include <windows.h>
#include <msctf.h>
#include "com/bstr.hpp"
#include "com/com_ptr.hpp"
#include "com_error.hpp"

// Thin wrapper around ITfInputProcessorProfiles.
class TfInputProcessorProfiles {
public:
    // Creates the TSF input processor profiles object.
    // Throws COMError when CoCreateInstance fails.
    TfInputProcessorProfiles() {
        ITfInputProcessorProfiles* profiles = nullptr;
        HRESULT hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles,
                                      nullptr,
                                      CLSCTX_ALL,
                                      IID_ITfInputProcessorProfiles,
                                      reinterpret_cast<void**>(&profiles));
        ComPtr<ITfInputProcessorProfiles> acquiredProfiles(profiles);
        if (FAILED(hr)) {
            throw COMError(hr, "CoCreateInstance(CLSID_TF_InputProcessorProfiles, IID_ITfInputProcessorProfiles)");
        }
        if (!acquiredProfiles) {
            throw COMError(E_POINTER, "CoCreateInstance(CLSID_TF_InputProcessorProfiles, IID_ITfInputProcessorProfiles)");
        }
        m_profiles = std::move(acquiredProfiles);
    }

    TfInputProcessorProfiles(const TfInputProcessorProfiles&) = delete;
    TfInputProcessorProfiles& operator=(const TfInputProcessorProfiles&) = delete;

    TfInputProcessorProfiles(TfInputProcessorProfiles&&) noexcept = default;
    TfInputProcessorProfiles& operator=(TfInputProcessorProfiles&&) noexcept = default;

    // Returns the description of the given language profile.
    // Throws COMError when the call fails.
    Bstr getLanguageProfileDescription(REFCLSID clsid, LANGID langid, REFGUID guidProfile) const {
        BSTR rawDescription = nullptr;
        HRESULT hr = m_profiles.get()->GetLanguageProfileDescription(clsid, langid, guidProfile, &rawDescription);
        Bstr description(rawDescription);
        if (FAILED(hr)) {
            throw COMError(hr, "ITfInputProcessorProfiles::GetLanguageProfileDescription");
        }
        return description;
    }

private:
    ComPtr<ITfInputProcessorProfiles> m_profiles;
};

#endif /* end of include guard: COM_TF_INPUT_PROCESSOR_PROFILES_HPP */
