#pragma once

#include <string>
#include <codecvt>
#include <locale>

namespace string
{

inline std::string unicode_to_utf8(const std::wstring& wstring)
{
    return std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().to_bytes(wstring);
}

inline std::wstring utf8_to_unicode(const std::string& utf8)
{
    return std::wstring_convert< std::codecvt_utf8<wchar_t> >().from_bytes(utf8);
}

inline std::wstring mb_to_unicode(const std::string& str)
{
    std::wstring ret;
    std::mbstate_t state = {};
    const char* src = str.data();

    size_t len = std::mbsrtowcs(nullptr, &src, 0, &state);

    if (len != static_cast<size_t>(-1))
    {
        std::vector<wchar_t> buff(len + 1);

        len = std::mbsrtowcs(buff.data(), &src, len, &state);

        if (len != static_cast<size_t>(-1))
        {
            ret.assign(buff.data(), len);
        }
    }

    return ret;
}

inline std::string unicode_to_mb(const std::wstring& wstr)
{
    std::string ret;
    std::mbstate_t state = {};

    const wchar_t* src = wstr.data();

    size_t len = std::wcsrtombs(nullptr, &src, 0, &state);

    if (len != static_cast<size_t>(-1))
    {
        std::vector<char> buff(len + 1);

        len = std::wcsrtombs(buff.data(), &src, len, &state);

        if (len != static_cast<size_t>(-1))
        {
            ret.assign(buff.data(), len);
        }
    }

    return ret;
}

inline std::string utf8_to_mb(const std::string& input)
{
    return unicode_to_mb(utf8_to_unicode(input));
}

inline std::string mb_to_utf8(const std::string& input)
{
    return unicode_to_utf8(mb_to_unicode(input));
}

}
