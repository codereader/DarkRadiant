#include "ModuleLoader.h"

#include <iostream>
#include "imodule.h"

#include "stream/textstream.h"
#include "os/dir.h"

#include "DynamicLibraryLoader.h"

#include <boost/algorithm/string/predicate.hpp>

namespace module {

	namespace {
		const std::string PLUGINS_DIR = "plugins/"; ///< name of plugins directory 
		const std::string MODULES_DIR = "modules/"; ///< name of modules directory
	}

// Constructor sets platform-specific extension to match
Loader::Loader(const std::string& path) : 
	_path(path),
#if defined(WIN32)
	  _ext(".dll")
#elif defined(POSIX)
	  _ext(".so")
#endif
{}

// Functor operator, gets invoked on directory traversal
void Loader::operator() (const std::string& fileName) const {
	// Check for the correct extension of the visited file
	if (boost::algorithm::iends_with(fileName, _ext)) {
		std::string fullName = _path + fileName;
		globalOutputStream() << "ModuleLoader: Loading module '" << fullName.c_str() << "'\n";
		
		// Create the encapsulator class
		DynamicLibraryPtr library(new DynamicLibrary(fullName));
		
		// greebo: Invoke the library loader, which will add the library to the list
		// on success. If the load fails, the shared pointer doesn't get added and 
		// self-destructs at the end of this scope.
		DynamicLibraryLoader(library, _dynamicLibraryList);
	}
}

/** Load all of the modules in the DarkRadiant install directory. Modules
 * are loaded from modules/ and plugins/.
 * 
 * @root: The root directory to search.
 */
void Loader::loadModules(const std::string& root) {
	Directory_forEach(root + MODULES_DIR, Loader(root + MODULES_DIR));
	Directory_forEach(root + PLUGINS_DIR, Loader(root + PLUGINS_DIR));
}

void Loader::unloadModules() {
	_dynamicLibraryList.clear();
}

// Initialise the static DLL list 
DynamicLibraryList Loader::_dynamicLibraryList;

} // namespace module
