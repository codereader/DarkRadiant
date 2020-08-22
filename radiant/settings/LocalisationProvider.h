#pragma once

#include <map>
#include <memory>
#include "i18n.h"

class wxLocale;

namespace settings
{

// Helper class acting as ILocalisationProvider.
// To resolve the localised strings, it dispatches the call to the wx framework
class LocalisationProvider :
	public language::ILocalisationProvider
{
public:
	static const char* const RKEY_LANGUAGE;

	struct Language
	{
		std::string twoDigitCode; // "en"
		std::string displayName;  // "English"

		Language()
		{}

		Language(const std::string& twoDigitCode_, const std::string& displayName_) :
			twoDigitCode(twoDigitCode_),
			displayName(displayName_)
		{}
	};

private:
	// The current language string (e.g. "en_US")
	std::string _curLanguage;
	int _curLanguageIndex;

	std::string _languageSettingFile;

	// The path where all the languages are stored
	std::string _i18nPath;

	std::unique_ptr<wxLocale> _wxLocale;

	// Supported languages
	typedef std::map<int, Language> LanguageMap;
	LanguageMap _supportedLanguages;

	// Available languages (for the preference page)
	typedef std::vector<int> LanguageList;
	LanguageList _availableLanguages;

	LocalisationProvider(IApplicationContext& context);
	LocalisationProvider(const LocalisationProvider& other) = delete;

public:
	std::string getLocalisedString(const char* stringToLocalise) override;

	void foreachAvailableLanguage(const std::function<void(const Language&)>& functor);

	// The currently selected language, as index into the list of available languages
	int getCurrentLanguageIndex() const;

	// Persists the current language selection to the file on disk
	void saveLanguageSetting();

	// Singleton instance setting up the shared collections
	static std::shared_ptr<LocalisationProvider>& Instance();

	// Sets up available and supported language collections
	static void Initialise(IApplicationContext& context);

	// Cleans up shared static resources allocated at startup
	static void Cleanup();

private:
	// Loads the language setting from the .language in the user settings folder
	std::string loadLanguageSetting();

	// Saves the language setting to the .language in the user settings folder
	void saveLanguageSetting(const std::string& language);

	// Fills the array of supported languages
	void loadSupportedLanguages();

	// Searches for available language .mo files
	void findAvailableLanguages();

	// Returns the language index for the given two-digit code
	int getLanguageIndex(const std::string& languageCode);
};

}
