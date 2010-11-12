#ifndef _LANGUAGE_MANAGER_H_
#define _LANGUAGE_MANAGER_H_

#include "iregistry.h"
#include "imodule.h"
#include <vector>
#include <map>

namespace language
{

class LanguageManager :
	public RegisterableModule
{
private:
	// The current language string (e.g. "en_US")
	std::string _curLanguage;
	std::string _languageSettingFile;

	// The path where all the languages are stored
	std::string _i18nPath;

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
	// RegisterableModule implementation
	const std::string& getName() const;
	const StringSet& getDependencies() const;
	void initialiseModule(const ApplicationContext& ctx);
	void shutdownModule();

	// Method used to instantiate and register the module, and load settings
	// This is called by main() even before any other modules are loaded
	static void init(const ApplicationContext& ctx);

private:
	void initFromContext(const ApplicationContext& ctx);

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
typedef boost::shared_ptr<LanguageManager> LanguageManagerPtr;

} // namespace

#endif /* _LANGUAGE_MANAGER_H_ */
