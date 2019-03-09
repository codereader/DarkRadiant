#pragma once

#include "imodule.h"
#include <vector>

namespace radiant
{

class ApplicationContextImpl :
	public ApplicationContext
{
	// The app + home paths
	std::string _appPath;
	std::string _homePath;
	std::string _settingsPath;
	std::string _bitmapsPath;

	// The path where the .map files are stored
	std::string _mapsPath;

	// Command line arguments
	std::vector<std::string> _cmdLineArgs;

	// A function pointer to a global error handler, used for ASSERT_MESSAGE
	ErrorHandlingFunction _errorHandler;

public:
	/**
	 * Initialises the context with the arguments given to main().
	 */
	void initialise(int argc, char* argv[]);

	// Initialises the function handling pointers for debug builds
	void initErrorHandler();

    /* ApplicationContext implementation */
    std::string getApplicationPath() const override;
    std::string getLibraryPath() const override;
    std::string getRuntimeDataPath() const override;
    std::string getHTMLPath() const override;
    std::string getSettingsPath() const override;
    std::string getBitmapsPath() const override;
    const ArgumentList& getCmdLineArgs() const override;

	// Return the global stream references
    virtual std::ostream& getOutputStream() const override;
    virtual std::ostream& getWarningStream() const override;
    virtual std::ostream& getErrorStream() const override;
    virtual std::mutex& getStreamLock() const override;

	// Exports/deletes the paths to/from the registry
    virtual void savePathsToRegistry() const override;

    virtual const ErrorHandlingFunction& getErrorHandlingFunction() const override;

private:
	// Sets up the bitmap path and settings path
	void initPaths();

	// Initialises the arguments
	void initArgs(int argc, char* argv[]);
};

} // namespace radiant
