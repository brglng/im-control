#ifndef COM_TF_INPUT_PROCESSOR_PROFILE_MGR_HPP
#define COM_TF_INPUT_PROCESSOR_PROFILE_MGR_HPP

#include <utility>
#include <windows.h>
#include <msctf.h>
#include "com/com_ptr.hpp"
#include "com/enum_tf_input_processor_profiles.hpp"
#include "com_error.hpp"

// Thin wrapper around ITfInputProcessorProfileMgr.
class TfInputProcessorProfileMgr {
public:
    // Creates the TSF input processor profile manager.
    // Throws COMError when CoCreateInstance fails.
    TfInputProcessorProfileMgr() {
        ITfInputProcessorProfileMgr* profileMgr = nullptr;
        HRESULT hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles,
                                      nullptr,
                                      CLSCTX_ALL,
                                      IID_ITfInputProcessorProfileMgr,
                                      reinterpret_cast<void**>(&profileMgr));
        ComPtr<ITfInputProcessorProfileMgr> acquiredProfileMgr(profileMgr);
        if (FAILED(hr)) {
            throw COMError(hr, "CoCreateInstance(CLSID_TF_InputProcessorProfiles, IID_ITfInputProcessorProfileMgr)");
        }
        if (!acquiredProfileMgr) {
            throw COMError(E_POINTER, "CoCreateInstance(CLSID_TF_InputProcessorProfiles, IID_ITfInputProcessorProfileMgr)");
        }
        m_profileMgr = std::move(acquiredProfileMgr);
    }

    TfInputProcessorProfileMgr(const TfInputProcessorProfileMgr&) = delete;
    TfInputProcessorProfileMgr& operator=(const TfInputProcessorProfileMgr&) = delete;

    TfInputProcessorProfileMgr(TfInputProcessorProfileMgr&&) noexcept = default;
    TfInputProcessorProfileMgr& operator=(TfInputProcessorProfileMgr&&) noexcept = default;

    // Returns the active profile of the given category.
    // Throws COMError when the call fails.
    TF_INPUTPROCESSORPROFILE getActiveProfile(REFGUID catid) const {
        TF_INPUTPROCESSORPROFILE profile = {};
        HRESULT hr = m_profileMgr.get()->GetActiveProfile(catid, &profile);
        if (FAILED(hr)) {
            throw COMError(hr, "ITfInputProcessorProfileMgr::GetActiveProfile");
        }
        return profile;
    }

    // Enumerates all installed profiles.
    // Throws COMError when the call fails.
    EnumTfInputProcessorProfiles enumProfiles(DWORD flags) const {
        IEnumTfInputProcessorProfiles* enumerator = nullptr;
        HRESULT hr = m_profileMgr.get()->EnumProfiles(flags, &enumerator);
        ComPtr<IEnumTfInputProcessorProfiles> acquiredEnumerator(enumerator);
        if (FAILED(hr)) {
            throw COMError(hr, "ITfInputProcessorProfileMgr::EnumProfiles");
        }
        if (!acquiredEnumerator) {
            throw COMError(E_POINTER, "ITfInputProcessorProfileMgr::EnumProfiles");
        }
        return EnumTfInputProcessorProfiles(acquiredEnumerator.release());
    }

    // Activates the given language profile.
    // Throws COMError when the call fails.
    void activateProfile(TfProfileType profileType,
                         LANGID langid,
                         REFCLSID clsid,
                         REFGUID guidProfile,
                         HKL hkl,
                         DWORD flags) const {
        HRESULT hr = m_profileMgr.get()->ActivateProfile(profileType, langid, clsid, guidProfile, hkl, flags);
        if (FAILED(hr)) {
            throw COMError(hr, "ITfInputProcessorProfileMgr::ActivateProfile");
        }
    }

private:
    ComPtr<ITfInputProcessorProfileMgr> m_profileMgr;
};

#endif /* end of include guard: COM_TF_INPUT_PROCESSOR_PROFILE_MGR_HPP */
