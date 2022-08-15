#include "PreferenceSystem.h"

#include "ipreferencesystem.h"
#include "itextstream.h"

#include "module/StaticModule.h"

namespace settings
{

IPreferencePage& PreferenceSystem::getPage(const std::string& path)
{
	ensureRootPage();

	return _rootPage->createOrFindPage(path);
}

void PreferenceSystem::foreachPage(const std::function<void(IPreferencePage&)>& functor)
{
	ensureRootPage();

	_rootPage->foreachChildPage(functor);
}

void PreferenceSystem::ensureRootPage()
{
	if (!_rootPage)
	{
		_rootPage = std::make_shared<PreferencePage>("");
	}
}

// RegisterableModule implementation
const std::string& PreferenceSystem::getName() const
{
	static std::string _name(MODULE_PREFERENCESYSTEM);
	return _name;
}

const StringSet& PreferenceSystem::getDependencies() const
{
	static StringSet _dependencies;
	return _dependencies;
}

void PreferenceSystem::initialiseModule(const IApplicationContext& ctx)
{
}

// Define the static PreferenceSystem module
module::StaticModuleRegistration<PreferenceSystem> preferenceSystemModule;

}
