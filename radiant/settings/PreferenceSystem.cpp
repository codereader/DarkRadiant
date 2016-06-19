#include "PreferenceSystem.h"

#include "ipreferencesystem.h"
#include "itextstream.h"
#include "imodule.h"
#include "iregistry.h"

#include "modulesystem/StaticModule.h"
#include "ui/prefdialog/PrefDialog.h"

IPreferencesPagePtr PreferenceSystem::getPage(const std::string& path)
{
	return ui::PrefDialog::Instance().createOrFindPage(path);
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

	if (_dependencies.empty())
	{
		_dependencies.insert(MODULE_XMLREGISTRY);
		_dependencies.insert(MODULE_RADIANT);
	}

	return _dependencies;
}

void PreferenceSystem::initialiseModule(const ApplicationContext& ctx)
{
	rMessage() << "PreferenceSystem::initialiseModule called" << std::endl;
}

// Define the static PreferenceSystem module
module::StaticModule<PreferenceSystem> preferenceSystemModule;

IPreferenceSystem& GetPreferenceSystem()
{
	return *preferenceSystemModule.getModule();
}
