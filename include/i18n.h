#pragma once

#define GETTEXT_PACKAGE "darkradiant"

// Redefine the _() macro to return a std::string for convenience
#ifndef WXINTL_NO_GETTEXT_MACRO
	#define WXINTL_NO_GETTEXT_MACRO
#endif 

#include <wx/intl.h>
#include <string>

// Custom translation macros

inline std::string _(const char* s)
{
	return wxGetTranslation((s)).ToStdString();
}

//#define _(s)	(wxGetTranslation((s)).ToStdString())
#define N_(str)	str

#ifndef C_
#define C_(context,text) _(text)
#endif
