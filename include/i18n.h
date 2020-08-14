#pragma once

#include <string>
#include "imodule.h"

class ILanguageManager :
	public RegisterableModule
{
public:
	virtual ~ILanguageManager() {}

	/**
	 * Returns the localized version of the given input string
	 * or the unmodified string if no suitable localization provider
	 * was found.
	 */
	virtual std::string getLocalizedString(const char* stringToLocalise) = 0;
};

const char* const MODULE_LANGUAGEMANAGER("LanguageManager");

// This is the accessor for the global language manager module
inline ILanguageManager& GlobalLanguageManager()
{
	// Cache the reference locally
	static ILanguageManager& _instance(
		*std::static_pointer_cast<ILanguageManager>(
			module::GlobalModuleRegistry().getModule(MODULE_LANGUAGEMANAGER)
		)
	);
	return _instance;
}

#define GETTEXT_PACKAGE "darkradiant"

// Redefine the _() macro to return a std::string for convenience
#ifndef WXINTL_NO_GETTEXT_MACRO
	#define WXINTL_NO_GETTEXT_MACRO
#endif

//#include <wx/intl.h>

// Custom translation macros _, N_ and C_

inline std::string _(const char* s)
{
	if (!module::IsGlobalModuleRegistryAvailable())
	{
		return s; // it's still too early for this call, return the unmodified string
	}

	if (!module::GlobalModuleRegistry().moduleExists(MODULE_LANGUAGEMANAGER))
	{
		return s; // still too early
	}

	return GlobalLanguageManager().getLocalizedString(s);
}

// Macro used to decorate a string as localizable, such that it is recognised
// by the xgettext parser, but without triggering an actual function call
// at the place it is used. Can be used to e.g. decorate constants in the code.
#define N_(str)	str

#ifndef C_
#define C_(context,text) _(text)
#endif
