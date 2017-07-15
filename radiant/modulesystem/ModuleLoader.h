#pragma once

#include <string>
#include <vector>
#include "imodule.h"
#include "DynamicLibrary.h"
#include "os/fs.h"

namespace module
{

/** Module loader functor class. This class is used to traverse a directory and
 * load each module into the GlobalModuleServer.
 *
 * Invoke the static method loadModules() to load the DLLs from DarkRadiant's
 * default module folders (e.g. modules/ and plugins/).
 */
class ModuleLoader
{
private:
	// This list contains all the allocated dynamic libraries
	DynamicLibraryList _dynamicLibraryList;

public:
	// Static loader algorithm, searches plugins/ and modules/ for .dll/.so files
	void loadModules(const std::string& root);

	// Frees the list of DLLs
	void unloadModules();

private:
	void loadModulesFromPath(const std::string& path);

	// File functor, gets called with each file's name in the searched folder
	void processModuleFile(const fs::path& fileName);
};

} // namespace module
