#ifndef _LANGUAGE_MANAGER_H_
#define _LANGUAGE_MANAGER_H_

#include "iregistry.h"
#include "imodule.h"

namespace language
{

class LanguageManager;
typedef boost::shared_ptr<LanguageManager> LanguageManagerPtr;

class LanguageManager :
	public RegisterableModule,
	public RegistryKeyObserver
{
private:
	// The current language string (e.g. "en_US")
	std::string _curLanguage;
	std::string _languageSettingFile;

public:
	// RegisterableModule implementation
	const std::string& getName() const;
	const StringSet& getDependencies() const;
	void initialiseModule(const ApplicationContext& ctx);
	void shutdownModule();

	// RegistryKeyObserver implementation
	void keyChanged(const std::string& key, const std::string& value);

	// Method used to initialise language settings
	// this is called by main() even before any other modules are loaded
	void initLanguageFromContext(const ApplicationContext& ctx);

private:
	// Loads the language setting from the .language in the user settings folder
	std::string loadLanguageSetting();

	// Saves the language setting to the .language in the user settings folder
	void saveLanguageSetting(const std::string& language);
};

// Accessor for the above module for use before module load time
LanguageManager& getLanguageManager();

} // namespace

#endif /* _LANGUAGE_MANAGER_H_ */
