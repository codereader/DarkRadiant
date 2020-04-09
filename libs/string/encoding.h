#pragma once

#include <string>
#include <codecvt>
#include <locale>

namespace string
{

inline std::string to_utf8(const std::wstring& wstring)
{
    return std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().to_bytes(wstring);
}

}
