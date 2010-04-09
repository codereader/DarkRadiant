#include "LanguageManager.h"

#include <glib/gutils.h>
#include <libintl.h>

#include "os/path.h"
#include "os/file.h"
#include "i18n.h"
#include "ipreferencesystem.h"
#include "itextstream.h"
#include "modulesystem/StaticModule.h"
#include <fstream>

namespace language
{

namespace
{
	const char* const LANGUAGE_SETTING_FILE = "darkradiant.language";
	const char* const DEFAULT_LANGUAGE = "en";
	const std::string RKEY_LANGUAGE("user/ui/language");
}

const std::string& LanguageManager::getName() const
{
	static std::string _name("LanguageManager");
	return _name;
}

const StringSet& LanguageManager::getDependencies() const
{
	static StringSet _dependencies;

	if (_dependencies.empty())
	{
		_dependencies.insert(MODULE_XMLREGISTRY);
		_dependencies.insert(MODULE_PREFERENCESYSTEM);
	}

	return _dependencies;
}

void LanguageManager::initialiseModule(const ApplicationContext& ctx)
{
	globalOutputStream() << getName() << "::initialiseModule called" << std::endl;

	// Load the language into the registry
	GlobalRegistry().set(RKEY_LANGUAGE, _curLanguage);

	// Register RegistryKeyObserver
	GlobalRegistry().addKeyObserver(this, RKEY_LANGUAGE);

	// Add Preferences
	// TODO
}

void LanguageManager::shutdownModule()
{
	// Save the language setting on shutdown
	saveLanguageSetting(GlobalRegistry().get(RKEY_LANGUAGE));

	GlobalRegistry().removeKeyObserver(this);
}

void LanguageManager::keyChanged(const std::string& key, const std::string& value)
{
	_curLanguage = value;
}

void LanguageManager::initLanguageFromContext(const ApplicationContext& ctx)
{
	// Initialise these members
	_languageSettingFile = ctx.getSettingsPath() + LANGUAGE_SETTING_FILE;
	_curLanguage = loadLanguageSetting();
	
	// Set the LANG environment. As GLIB/GTK+ (in Win32) is using its own C runtime, we need
	// to call their GLIB setenv function for the environment variable to take effect.
	g_setenv("LANG", _curLanguage.c_str(), TRUE);

	std::string i18nPath = os::standardPathWithSlash(ctx.getApplicationPath() + "i18n");

	// Tell glib to load stuff from the given i18n path
	bindtextdomain(GETTEXT_PACKAGE, i18nPath.c_str());

    // set encoding to utf-8 to prevent errors for Windows
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
}

std::string LanguageManager::loadLanguageSetting()
{
	std::string language = DEFAULT_LANGUAGE;

	// Check for an existing language setting file in the user settings folder
	std::ifstream str(_languageSettingFile.c_str());

	if (str.good())
	{
		str >> language;
	}

	return language;
}

void LanguageManager::saveLanguageSetting(const std::string& language)
{
	std::ofstream str(_languageSettingFile.c_str());

	str << language;

	str.flush();
	str.close();
}

// This is registering the Language Manager module in the registry before main()
module::StaticModule<LanguageManager> languageManagerModule;

LanguageManager& getLanguageManager()
{
	// Use the above StaticModule class to acquire a reference
	return *languageManagerModule.getModule();
}

} // namespace
