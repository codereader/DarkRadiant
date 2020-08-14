#include "LocalisationModule.h"

#include <fstream>
#include <stdexcept>

#include "itextstream.h"
#include "ipreferencesystem.h"
#include "registry/registry.h"

#include "string/case_conv.h"
#include "os/path.h"
#include "module/StaticModule.h"

#include <wx/translation.h>

namespace ui
{

namespace
{
	const char* const LANGUAGE_SETTING_FILE = "darkradiant.language";
	const char* const DEFAULT_LANGUAGE = "en";
	const std::string RKEY_LANGUAGE("user/ui/language");
}

class UnknownLanguageException :
	public std::runtime_error
{
public:
	UnknownLanguageException(const std::string& what) :
		std::runtime_error(what)
	{}
};

// Local helper class acting as ILocalisationProvider
// To resolve the localised strings, it dispatches the call to the wx framework
class LocalisationProviderWx :
	public ILocalisationProvider
{
public:
	std::string getLocalisedString(const char* stringToLocalise) override
	{
		return wxGetTranslation(stringToLocalise).ToStdString();
	}
};

LocalisationModule::LocalisationModule()
{} // constructor needed for wxLocale pimpl

LocalisationModule::~LocalisationModule()
{} // destructor needed for wxLocale pimpl

const std::string& LocalisationModule::getName() const
{
	static std::string _name("LocalisationProviders");
	return _name;
}

const StringSet& LocalisationModule::getDependencies() const
{
	static StringSet _dependencies;

	if (_dependencies.empty())
	{
		_dependencies.insert(MODULE_LANGUAGEMANAGER);
		_dependencies.insert(MODULE_PREFERENCESYSTEM);
		_dependencies.insert(MODULE_XMLREGISTRY);
	}

	return _dependencies;
}

void LocalisationModule::initialiseModule(const ApplicationContext& ctx)
{
	rMessage() << getName() << "::initialiseModule called" << std::endl;

	// Register the provider with the core
	GlobalLanguageManager().registerProvider(std::make_shared<LocalisationProviderWx>());

	// Point wxWidgets to the folder where the catalog files are stored
	_i18nPath = os::standardPathWithSlash(ctx.getApplicationPath() + "i18n");
	wxFileTranslationsLoader::AddCatalogLookupPathPrefix(_i18nPath);

	// Load the persisted language setting
	_languageSettingFile = ctx.getSettingsPath() + LANGUAGE_SETTING_FILE;
	_curLanguage = loadLanguageSetting();

	rMessage() << "Current language setting: " << _curLanguage << std::endl;

	// Fill array of supported languages
	loadSupportedLanguages();

	// Fill array of available languages
	findAvailableLanguages();

	// Keep locale set to "C" for faster stricmp in Windows builds
	_wxLocale.reset(new wxLocale(_curLanguage, _curLanguage, "C"));
	_wxLocale->AddCatalog(GETTEXT_PACKAGE);

	int curLangIndex = 0; // english

	try
	{
		int index = getLanguageIndex(_curLanguage);

		// Get the offset into the array of available languages
		auto found = std::find(_availableLanguages.begin(), _availableLanguages.end(), index);

		if (found != _availableLanguages.end())
		{
			curLangIndex = static_cast<int>(std::distance(_availableLanguages.begin(), found));
		}
	}
	catch (UnknownLanguageException&)
	{
		rWarning() << "Warning, unknown language found in " <<
			LANGUAGE_SETTING_FILE << ", reverting to English" << std::endl;
	}

	// Construct the list of available languages
	ComboBoxValueList langs;

	for (int code : _availableLanguages)
	{
		const Language& lang = _supportedLanguages[code];
		langs.emplace_back(lang.twoDigitCode + " - " + lang.displayName);
	}

	// Load the currently selected index into the registry
	registry::setValue(RKEY_LANGUAGE, curLangIndex);
	GlobalRegistry().setAttribute(RKEY_LANGUAGE, "volatile", "1"); // don't save this to user.xml

	// Add Preferences
	IPreferencePage& page = GlobalPreferenceSystem().getPage(_("Settings/Language"));
	page.appendCombo(_("Language"), RKEY_LANGUAGE, langs);

	page.appendLabel(_("<b>Note:</b> You'll need to restart DarkRadiant\nafter changing the language setting."));
}

void LocalisationModule::shutdownModule()
{
	// Get the language setting from the registry (this is an integer)
	// and look up the language code (two digit)
	int langNum = registry::getValue<int>(RKEY_LANGUAGE);

	assert(langNum >= 0 && langNum < static_cast<int>(_availableLanguages.size()));

	// Look up the language index in the list of available languages
	int langIndex = _availableLanguages[langNum];

	assert(_supportedLanguages.find(langIndex) != _supportedLanguages.end());

	// Save the language code to the settings file
	saveLanguageSetting(_supportedLanguages[langIndex].twoDigitCode);
}

std::string LocalisationModule::loadLanguageSetting()
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

void LocalisationModule::saveLanguageSetting(const std::string& language)
{
	std::ofstream str(_languageSettingFile.c_str());

	str << language;

	str.flush();
	str.close();
}

void LocalisationModule::loadSupportedLanguages()
{
	_supportedLanguages.clear();

	int index = 0;

	_supportedLanguages[index++] = Language("en", _("English"));
	_supportedLanguages[index++] = Language("de", _("German"));
}

void LocalisationModule::findAvailableLanguages()
{
	// English (index 0) is always available
	_availableLanguages.push_back(0);

	wxFileTranslationsLoader loader;
	wxArrayString translations = loader.GetAvailableTranslations(GETTEXT_PACKAGE);

	for (const auto& lang : translations)
	{
		// Get the index (is this a known language?)
		try
		{
			int index = getLanguageIndex(lang.ToStdString());

			// Add this to the list (could use more extensive checking, but this is enough for now)
			_availableLanguages.push_back(index);
		}
		catch (UnknownLanguageException&)
		{
			rWarning() << "Skipping unknown language: " << lang << std::endl;
		}
	}

	rMessage() << "Found " << _availableLanguages.size() << " language folders." << std::endl;
}

int LocalisationModule::getLanguageIndex(const std::string& languageCode)
{
	std::string code = string::to_lower_copy(languageCode);

	for (const auto& pair : _supportedLanguages)
	{
		if (pair.second.twoDigitCode == code)
		{
			return pair.first;
		}
	}

	throw UnknownLanguageException("Unknown language: " + languageCode);
}

module::StaticModule<LocalisationModule> localisationModule;

}
