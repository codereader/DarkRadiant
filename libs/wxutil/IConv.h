#pragma once

#include <string>
#if 0
#include <wx/string.h>
#endif
#include "string/encoding.h"

#include <cassert>

namespace wxutil
{

/**
 * greebo: This helper class is supposed to help converting
 * locale strings containing foreign characters like the 
 * French accented chars to UTF-8 encoding.
 *
 * I'm not an expert on this, so let's hope I got it right.
 * Be welcome to suggest any code improvements.
 */
class IConv
{
public:
	/**
	 * greebo: Converts the given string to UTF-8 encoding.
	 * Returns an empty string if the conversion fails.
	 */
	static std::string localeToUTF8(const std::string& input)
	{
#if 1
		return string::mb_to_utf8(input);
#else
		wxString inp(input);
		return inp.ToUTF8().data();
#endif
	}

	/**
	 * greebo: Converts the given string from UTF-8 to the current locale.
	 * Returns an empty string if the conversion fails.
	 */
	static std::string localeFromUTF8(const std::string& input)
	{
#if 1
		return string::utf8_to_mb(input);
#else
		return wxString::FromUTF8(input.c_str()).ToStdString();
#endif
	}
};

} // namespace
