#pragma once

#include <string>
#include "imodule.h"
#include "iradiant.h"

namespace language
{

class ILocalisationProvider
{
public:
	virtual ~ILocalisationProvider() {}

	typedef std::shared_ptr<ILocalisationProvider> Ptr;

	// Returns the localised version of the given string
	virtual std::string getLocalisedString(const char* stringToLocalise) = 0;
};

class ILanguageManager
{
public:
	virtual ~ILanguageManager() {}

	/**
	 * Registers the given provider, which will be used to resolve localised strings.
	 */
	virtual void registerProvider(const ILocalisationProvider::Ptr& instance) = 0;

	/**
	 * Returns the localised version of the given input string
	 * or the unmodified string if no suitable localisation provider
	 * was found.
	 */
	virtual std::string getLocalisedString(const char* stringToLocalise) = 0;
};

}

// This is the accessor for the global language manager module
inline language::ILanguageManager& GlobalLanguageManager()
{
	return GlobalRadiantCore().getLanguageManager();
}

#define GETTEXT_PACKAGE "darkradiant"

// Redefine the _() macro to return a std::string for convenience
#ifndef WXINTL_NO_GETTEXT_MACRO
	#define WXINTL_NO_GETTEXT_MACRO
#endif

// Custom translation macros _, N_ and C_

inline std::string _(const char* s)
{
	if (!module::IsGlobalModuleRegistryAvailable())
	{
		return s; // it's still too early for this call, return the unmodified string
	}

	if (!module::GlobalModuleRegistry().moduleExists(MODULE_RADIANT_CORE))
	{
		return s; // still too early
	}

	return GlobalLanguageManager().getLocalisedString(s);
}

// Macro used to decorate a string as localizable, such that it is recognised
// by the xgettext parser, but without triggering an actual function call
// at the place it is used. Can be used to e.g. decorate constants in the code.
#define N_(str)	str

#ifndef C_
#define C_(context,text) _(text)
#endif
