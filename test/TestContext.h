#pragma once

#include "module/ApplicationContextBase.h"
#include "os/fs.h"
#include "os/dir.h"
#include "debugging/debugging.h"

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
	std::string _tempDataPath;

public:
	TestContext()
	{
		// Set up the temporary settings folder
		auto settingsFolder = os::getTemporaryPath() / "dr_temp_settings";

		_settingsFolder = os::standardPathWithSlash(settingsFolder.string());

		os::removeDirectory(_settingsFolder);
		os::makeDirectory(_settingsFolder);

        auto tempDataFolder = os::getTemporaryPath() / "dr_temp_data";

        _tempDataPath = os::standardPathWithSlash(tempDataFolder.string());

        os::removeDirectory(_tempDataPath);
        os::makeDirectory(_tempDataPath);

		setErrorHandlingFunction([&](const std::string& title, const std::string& message)
		{
			std::cerr << "Fatal error " << title << "\n" << message << std::endl;
			DEBUGGER_BREAKPOINT();
		});
	}

	virtual ~TestContext()
	{
		if (!_settingsFolder.empty())
		{
			os::removeDirectory(_settingsFolder);
		}

        if (!_tempDataPath.empty())
        {
            os::removeDirectory(_tempDataPath);
        }
	}

    // Returns the path to the test/resources/tdm/ folder shipped with the DR sources
    virtual std::string getTestProjectPath() const
    {
        return getTestResourcePath() + "tdm/";
    }

	virtual std::string getTestResourcePath() const
	{
#if defined(POSIX)
	#if defined(TEST_BASE_PATH)
		fs::path testResourcePath(TEST_BASE_PATH);
		testResourcePath /= "resources/";
	#else
		// make check will compile the test binary to $top_builddir/test/.libs/
		fs::path testResourcePath = getApplicationPath();
		testResourcePath /= "../../test/resources/";
	#endif
#else
		fs::path testResourcePath = getApplicationPath();
		testResourcePath /= "../test/resources/";
#endif
		return os::standardPathWithSlash(testResourcePath.string());
	}

	std::string getSettingsPath() const override
	{
		return _settingsFolder;
	}

    // Path to a directory where any stuff can be written to
    // This folder will be purged once on context destruction
    std::string getTemporaryDataPath() const
    {
        return _tempDataPath;
    }

	std::string getRuntimeDataPath() const override
	{
// Allow special build settings to override the runtime data path
// this is needed to run the test binary through ctest from the build root
#ifdef TEST_BASE_PATH
		fs::path runtimeDataPath(TEST_BASE_PATH);
		runtimeDataPath /= "../install/";
		return runtimeDataPath.string();
#else
		return ApplicationContextBase::getRuntimeDataPath();
#endif
	}

	std::vector<std::string> getLibraryPaths() const override
	{
        std::vector<std::string> paths;

#ifdef TEST_BASE_PATH
        // Look for a radiantcore module in the actual radiantcore/ source
        // directory. This will not be valid for out-of-source builds.
		fs::path libraryPath(TEST_BASE_PATH);
		libraryPath /= "../radiantcore/";

        paths.push_back(libraryPath.string());
#endif

        // Also look for modules in the module install destination directory
        // (for out-of-source builds)
		auto libBasePath = os::standardPathWithSlash(getLibraryBasePath());

		// Don't load modules from the plugins/ folder, as these are relying on
		// a working UI. For the test environment we are only interested in
		// non-UI modules, for now at least.
        paths.push_back(libBasePath + MODULES_DIR);
        return paths;
	}
};

}
