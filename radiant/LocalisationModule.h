#pragma once

#include "i18n.h"

namespace ui
{

class LocalisationModule :
	public RegisterableModule
{
private:
	// The current language string (e.g. "en_US")
	std::string _curLanguage;
	std::string _languageSettingFile;

	// The path where all the languages are stored
	std::string _i18nPath;

	std::unique_ptr<wxLocale> _wxLocale;

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

	// Supported languages
	typedef std::map<int, Language> LanguageMap;
	LanguageMap _supportedLanguages;

	// Available languages (for the preference page)
	typedef std::vector<int> LanguageList;
	LanguageList _availableLanguages;

public:
	LocalisationModule();
	~LocalisationModule();

	const std::string& getName() const override;
	const StringSet& getDependencies() const;
	void initialiseModule(const ApplicationContext& ctx) override;
	void shutdownModule() override;

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
