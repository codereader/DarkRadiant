#include "PreferenceSystem.h"

#include "ipreferencesystem.h"
#include "itextstream.h"
#include "imodule.h"
#include "iregistry.h"

#include "modulesystem/StaticModule.h"
#include "ui/prefdialog/PrefDialog.h"

class PreferenceSystem :
	public IPreferenceSystem
{
public:
	// Looks up a page for the given path and returns it to the client
	IPreferencesPagePtr getPage(const std::string& path) override
	{
		return ui::PrefDialog::Instance().createOrFindPage(path);
	}

	// RegisterableModule implementation
	virtual const std::string& getName() const override
	{
		static std::string _name(MODULE_PREFERENCESYSTEM);
		return _name;
	}

	virtual const StringSet& getDependencies() const override
	{
		static StringSet _dependencies;

		if (_dependencies.empty())
		{
			_dependencies.insert(MODULE_XMLREGISTRY);
			_dependencies.insert(MODULE_RADIANT);
		}

		return _dependencies;
	}

	virtual void initialiseModule(const ApplicationContext& ctx) override
	{
		rMessage() << "PreferenceSystem::initialiseModule called" << std::endl;
	}
};

// Define the static PreferenceSystem module
module::StaticModule<PreferenceSystem> preferenceSystemModule;

IPreferenceSystem& GetPreferenceSystem()
{
	return *preferenceSystemModule.getModule();
}
