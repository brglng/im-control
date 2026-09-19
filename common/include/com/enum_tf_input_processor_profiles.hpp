#ifndef COM_ENUM_TF_INPUT_PROCESSOR_PROFILES_HPP
#define COM_ENUM_TF_INPUT_PROCESSOR_PROFILES_HPP

#include <windows.h>
#include <msctf.h>
#include "com/com_ptr.hpp"
#include "com_error.hpp"

// Thin wrapper around IEnumTfInputProcessorProfiles.
class EnumTfInputProcessorProfiles {
public:
    explicit EnumTfInputProcessorProfiles(IEnumTfInputProcessorProfiles* enumerator) noexcept
        : m_enumerator(enumerator) {}

    EnumTfInputProcessorProfiles(const EnumTfInputProcessorProfiles&) = delete;
    EnumTfInputProcessorProfiles& operator=(const EnumTfInputProcessorProfiles&) = delete;

    EnumTfInputProcessorProfiles(EnumTfInputProcessorProfiles&&) noexcept = default;
    EnumTfInputProcessorProfiles& operator=(EnumTfInputProcessorProfiles&&) noexcept = default;

    // Fetches the next profile. Returns false when the enumeration is
    // exhausted; throws COMError when the enumeration fails.
    bool next(TF_INPUTPROCESSORPROFILE& profile) const {
        ULONG fetched = 0;
        HRESULT hr = m_enumerator.get()->Next(1, &profile, &fetched);
        if (FAILED(hr)) {
            throw COMError(hr, "IEnumTfInputProcessorProfiles::Next");
        }
        return hr == S_OK;
    }

private:
    ComPtr<IEnumTfInputProcessorProfiles> m_enumerator;
};

#endif /* end of include guard: COM_ENUM_TF_INPUT_PROCESSOR_PROFILES_HPP */
