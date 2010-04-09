#include "LanguageManager.h"

#include "iregistry.h"
#include "ipreferencesystem.h"
#include "itextstream.h"
#include "modulesystem/StaticModule.h"

namespace language
{

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
	
	// Register RegistryKeyObserver
	// TODO

	// Add Preferences
	// TODO
}

module::StaticModule<LanguageManager> languageManagerModule;

} // namespace
