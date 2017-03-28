#pragma once

#include <string>
#include <vector>
#include "imodule.h"
#include "DynamicLibrary.h"
#include <boost/filesystem.hpp>

namespace module {

/** Module loader functor class. This class is used to traverse a directory and
 * load each module into the GlobalModuleServer.
 *
 * Invoke the static method loadModules() to load the DLLs from DarkRadiant's
 * default module folders (e.g. modules/ and plugins/).
 */
class Loader
{
private:
	// This list contains all the allocated dynamic libraries
	static DynamicLibraryList _dynamicLibraryList;

public:
	// Static loader algorithm, searches plugins/ and modules/ for .dll/.so files
	static void LoadModules(const std::string& root);

	// Frees the list of DLLs
	static void UnloadModules();

private:
	// File functor, gets called with each file's name in the searched folder
	static void processModuleFile(const boost::filesystem::path& fileName);
};

} // namespace module
