#pragma once

#include <string>

#ifdef _MSC_VER
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wincred.h>
#include <tchar.h>
#include "string/encoding.h"
#endif

namespace vcs
{

class CredentialManager
{
public:
    static std::pair<std::string, std::string> RetrievePassword(const std::wstring& accountName)
    {
#ifdef _MSC_VER
        PCREDENTIALW pcred;
        BOOL ok = ::CredReadW(accountName.c_str(), CRED_TYPE_GENERIC, 0, &pcred);

        if (!ok)
        {
            return std::make_pair("", "");
        }

        auto user = string::unicode_to_utf8(pcred->UserName);
        auto pw = string::unicode_to_utf8((const wchar_t*)pcred->CredentialBlob);

        ::CredFree(pcred);

        return std::make_pair(user, pw);
#else
        return "";
#endif
    }
};

}
