#include "PreferenceSystem.h"

#include "ipreferencesystem.h"
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

void resetPreferences() {
	// Get the filename
	const std::string userSettingsFile = 
		GlobalRegistry().get(RKEY_SETTINGS_PATH) + "user.xml";
	
	// If the file exists, remove it and tell the registry to 
	// skip saving the user.xml file on shutdown
	if (file_exists(userSettingsFile.c_str())) {
		time_t timeStamp;
		time(&timeStamp);
		std::tm* timeStruc = localtime(&timeStamp); 
		
		std::string suffix = "." + intToStr(timeStruc->tm_mon+1) + "-";
		suffix += intToStr(timeStruc->tm_mday) + "_";
		suffix += intToStr(timeStruc->tm_hour) + "-";
		suffix += intToStr(timeStruc->tm_min) + "-";
		suffix += intToStr(timeStruc->tm_sec) + ".xml";
		
		std::string moveTarget = userSettingsFile;
		boost::algorithm::replace_all(moveTarget, ".xml", suffix);
		
		file_move(userSettingsFile.c_str(), moveTarget.c_str());
		GlobalRegistry().set(RKEY_SKIP_REGISTRY_SAVE, "1");
	}
}

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
		globalOutputStream() << "PreferenceSystem::initialiseModule called\n";
	}
};

// Define the static PreferenceSystem module
module::StaticModule<PreferenceSystem> preferenceSystemModule;

IPreferenceSystem& GetPreferenceSystem() {
	return *preferenceSystemModule.getModule();
}
