#pragma once

#include <string>
#include <vector>
#include "imodule.h"
#include "module/DynamicLibrary.h"
#include "os/fs.h"

namespace module
{

/**
 * Module loader functor class. This class is used to traverse a directory and
 * load each module, calling the RegisterModule function they need to expose.
 *
 * Invoke the loadModules() method to load the DLLs from DarkRadiant's
 * default module folders (e.g. modules/ and plugins/).
 */
class ModuleLoader
{
private:
	// This list contains all the allocated dynamic libraries
	DynamicLibraryList _dynamicLibraryList;

	IModuleRegistry& _registry;

public:
	ModuleLoader(IModuleRegistry& registry);

	// Load algorithm, searches the given folder for .dll/.so files
	void loadModulesFromPath(const std::string& path);

	// Frees the list of DLLs
	void unloadModules();

private:
	// File functor, gets called with each file's name in the searched folder
	void processModuleFile(const fs::path& fileName);
};

} // namespace module
