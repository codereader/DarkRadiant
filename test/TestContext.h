#pragma once

#include "module/ApplicationContextBase.h"
#include "os/fs.h"
#include "os/dir.h"

namespace radiant
{

/**
 * Application context implementation used in DarkRadiant's unit tests
 */
class TestContext :
	public radiant::ApplicationContextBase
{
private:
	std::string _settingsFolder;

public:
	TestContext()
	{
		// Set up the temporary settings folder
		auto settingsFolder = os::getTemporaryPath() / "dr_temp_settings";

		_settingsFolder = settingsFolder.string();

		os::removeDirectory(_settingsFolder);
		os::makeDirectory(_settingsFolder);
	}

	std::string getSettingsPath() const override
	{
		return _settingsFolder;
	}

	std::vector<std::string> getLibraryPaths() const override
	{
		auto libBasePath = os::standardPathWithSlash(getLibraryBasePath());

		// Don't load modules from the plugins/ folder, as these
		// are relying on a working UI. For the test environment
		// we are only interested in non-UI modules, for now at least.
		return
		{
			libBasePath + MODULES_DIR,
		};
	}
};

}
