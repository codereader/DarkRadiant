#ifndef MODULELOADER_H_
#define MODULELOADER_H_

#include <string>
#include <vector>
#include "imodule.h"
#include "DynamicLibrary.h"

namespace module {

/** Module loader functor class. This class is used to traverse a directory and
 * load each module into the GlobalModuleServer.
 * 
 * Invoke the static method loadModules() to load the DLLs from DarkRadiant's
 * default module folders (e.g. modules/ and plugins/).
 */
class Loader
{
	// The path of the directory the loader is searching
	const std::string _path;
	
	// The filename extension which indicates a module (platform-specific)
	const std::string _ext;
	
	// This list contains all the allocated dynamic libraries
	static DynamicLibraryList _dynamicLibraryList;
	
public:
	// Constructor, pass the path it should search for modules in
	Loader(const std::string& path);
	
	// File functor, gets called with each file's name in the searched folder
	void operator() (const std::string& fileName) const;
	
	// Static loader algorithm, searches plugins/ and modules/ for .dll/.so files
	static void loadModules(const std::string& root);
	
	// Frees the list of DLLs
	static void unloadModules();
};

} // namespace module

#endif /*MODULELOADER_H_*/
