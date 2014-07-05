#pragma once

#include <string>
#include <wx/string.h>

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
		wxString inp(input);
		return inp.ToUTF8().data();
	}

	/**
	 * greebo: Converts the given string from UTF-8 to the current locale.
	 * Returns an empty string if the conversion fails.
	 */
	static std::string localeFromUTF8(const std::string& input)
	{
		return wxString::FromUTF8(input.c_str()).ToStdString();
	}
};

} // namespace
