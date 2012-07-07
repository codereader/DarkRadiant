#include "PreferenceSystem.h"

#include "ipreferencesystem.h"
#include "itextstream.h"
#include "imodule.h"
#include "iregistry.h"

#include <time.h>
#include <iostream>

#include "os/file.h"
#include "string/string.h"

#include "modulesystem/StaticModule.h"
#include "modulesystem/ApplicationContextImpl.h"
#include "ui/prefdialog/PrefDialog.h"

#include <boost/algorithm/string/replace.hpp>

class PreferenceSystem :
	public IPreferenceSystem
{
public:
	// Looks up a page for the given path and returns it to the client
	PreferencesPagePtr getPage(const std::string& path) {
		return ui::PrefDialog::Instance().createOrFindPage(path);
	}

	// RegisterableModule implementation
	virtual const std::string& getName() const {
		static std::string _name(MODULE_PREFERENCESYSTEM);
		return _name;
	}

	virtual const StringSet& getDependencies() const {
		static StringSet _dependencies;

		if (_dependencies.empty()) {
			_dependencies.insert(MODULE_XMLREGISTRY);
			_dependencies.insert(MODULE_RADIANT);
		}

		return _dependencies;
	}

	virtual void initialiseModule(const ApplicationContext& ctx) {
		rMessage() << "PreferenceSystem::initialiseModule called\n";
	}
};

// Define the static PreferenceSystem module
module::StaticModule<PreferenceSystem> preferenceSystemModule;

IPreferenceSystem& GetPreferenceSystem() {
	return *preferenceSystemModule.getModule();
}
