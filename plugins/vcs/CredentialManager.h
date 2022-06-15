#pragma once

#include <string>

#ifdef _MSC_VER
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wincred.h>
#include <tchar.h>
#endif
#include "string/encoding.h"

namespace vcs
{

class CredentialManager
{
public:
    static std::pair<std::string, std::string> RetrievePassword(const std::string& accountName)
    {
        return RetrievePassword(string::utf8_to_unicode(accountName));
    }

    static std::pair<std::string, std::string> RetrievePassword(const std::wstring& accountName)
    {
#ifdef _MSC_VER
        PCREDENTIALW pcred;
        auto ok = ::CredReadW(accountName.c_str(), CRED_TYPE_GENERIC, 0, &pcred);

        if (!ok)
        {
            return std::make_pair("", "");
        }

        auto user = string::unicode_to_utf8(pcred->UserName);

        // Extract N characters from the credential blob
        std::wstring blob;
        blob.assign(reinterpret_cast<wchar_t*>(pcred->CredentialBlob), pcred->CredentialBlobSize / sizeof(wchar_t));

        auto pw = string::unicode_to_utf8(blob);

        // Clear out the temporary string
        memset(blob.data(), '\0', blob.size() * sizeof(wchar_t));

        ::CredFree(pcred);

        // Move-return the result
        return std::make_pair(std::move(user), std::move(pw));
#else
        return { "", "" };
#endif
    }
};

}
