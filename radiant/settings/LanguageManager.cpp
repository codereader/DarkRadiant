#include "LanguageManager.h"

#include <glib/gutils.h>
#include <libintl.h>

#include "os/path.h"
#include "os/file.h"
#include "i18n.h"
#include "ipreferencesystem.h"
#include "itextstream.h"
#include <fstream>
#include <stdexcept>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/case_conv.hpp>

namespace fs = boost::filesystem;

namespace language
{

class UnknownLanguageException : 
	public std::runtime_error
{
public:
	UnknownLanguageException(const std::string& what) :
		std::runtime_error(what)
	{}
};

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

	// Fill array of supported languages
	loadSupportedLanguages();

	// Fill array of available languages
	findAvailableLanguages();

	int curLangIndex = 0; // english

	try
	{
		int index = getLanguageIndex(_curLanguage);

		// Get the offset into the array of available languages
		LanguageList::iterator found = 
			std::find(_availableLanguages.begin(), _availableLanguages.end(), index);

		if (found != _availableLanguages.end())
		{
			curLangIndex = static_cast<int>(std::distance(_availableLanguages.begin(), found));
		}
	}
	catch (UnknownLanguageException&)
	{
		globalWarningStream() << "Warning, unknown language found in " <<
			LANGUAGE_SETTING_FILE << ", reverting to English" << std::endl;
	}

	// Construct the list of available languages
	ComboBoxValueList langs;

	for (LanguageList::const_iterator i = _availableLanguages.begin();
		 i != _availableLanguages.end(); ++i)
	{
		const Language& lang = _supportedLanguages[*i];
		langs.push_back(lang.twoDigitCode + " - " + lang.displayName);
	}

	// Load the currently selected index into the registry
	GlobalRegistry().setInt(RKEY_LANGUAGE, curLangIndex);
	GlobalRegistry().setAttribute(RKEY_LANGUAGE, "volatile", "1"); // don't save this to user.xml

	// Add Preferences
	PreferencesPagePtr page = GlobalPreferenceSystem().getPage("Settings/Language");
	page->appendCombo(_("Language"), RKEY_LANGUAGE, langs);
}

void LanguageManager::shutdownModule()
{
	// Get the language setting from the registry (this is an integer)
	// and look up the language code (two digit)
	int langNum = GlobalRegistry().getInt(RKEY_LANGUAGE);

	assert(langNum >= 0 && langNum < static_cast<int>(_availableLanguages.size()));

	// Look up the language index in the list of available languages
	int langIndex = _availableLanguages[langNum];

	assert(_supportedLanguages.find(langIndex) != _supportedLanguages.end());

	// Save the language code to the settings file
	saveLanguageSetting(_supportedLanguages[langIndex].twoDigitCode);
}

void LanguageManager::findAvailableLanguages()
{
	// English (index 0) is always available
	_availableLanguages.push_back(0);

	// Search folder
	fs::path start(_i18nPath);

	for (fs::directory_iterator it(start); it != fs::directory_iterator(); ++it)
	{
		// Get the candidate
		const fs::path& candidate = *it;

		if (fs::is_directory(candidate))
		{
			// Get the index (is this a known language?)
			try
			{
				int index = getLanguageIndex(candidate.filename());

				// Add this to the list (could use more extensive checking, but this is enough for now)
				_availableLanguages.push_back(index);
			}
			catch (UnknownLanguageException&)
			{
				globalWarningStream() << "Skipping unknown language: " 
					<< candidate.filename() << std::endl;
				continue;
			}
		}
	}

	globalOutputStream() << "Found " << _availableLanguages.size() << " language folders." << std::endl;
}

void LanguageManager::init(const ApplicationContext& ctx)
{
	// Instantiate a new language manager
	LanguageManagerPtr instancePtr(new LanguageManager);

	// Hand that over to the module registry
	module::getRegistry().registerModule(instancePtr);

	// Initialise the module manually
	instancePtr->initFromContext(ctx);
}

void LanguageManager::initFromContext(const ApplicationContext& ctx)
{
	// Initialise these members
	_languageSettingFile = ctx.getSettingsPath() + LANGUAGE_SETTING_FILE;
	_curLanguage = loadLanguageSetting();

	_i18nPath = os::standardPathWithSlash(ctx.getApplicationPath() + "i18n");
	
	// Set the LANG environment. As GLIB/GTK+ (in Win32) is using its own C runtime, we need
	// to call their GLIB setenv function for the environment variable to take effect.
	g_setenv("LANG", _curLanguage.c_str(), TRUE);

	// Tell glib to load stuff from the given i18n path
	bindtextdomain(GETTEXT_PACKAGE, _i18nPath.c_str());

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

void LanguageManager::loadSupportedLanguages()
{
	_supportedLanguages.clear();

	int index = 0;

	_supportedLanguages[index++] = Language("en", _("English"));
	_supportedLanguages[index++] = Language("fr", _("French"));
	_supportedLanguages[index++] = Language("de", _("German"));
}

int LanguageManager::getLanguageIndex(const std::string& languageCode)
{
	std::string code = boost::algorithm::to_lower_copy(languageCode);

	for (LanguageMap::const_iterator i = _supportedLanguages.begin();
		 i != _supportedLanguages.end(); ++i)
	{
		if (i->second.twoDigitCode == code)
		{
			return i->first;
		}
	}

	throw UnknownLanguageException("Unknown language: " + languageCode);
}

} // namespace
