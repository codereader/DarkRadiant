#include "ModuleLoader.h"

#include "itextstream.h"
#include <iostream>
#include "imodule.h"

#include "os/dir.h"
#include "os/path.h"

#include "ModuleRegistry.h"

#include "string/case_conv.h"

namespace module
{

namespace
{
	const std::string PLUGINS_DIR = "plugins/"; ///< name of plugins directory
	const std::string MODULES_DIR = "modules/"; ///< name of modules directory

#if defined(WIN32)
	const std::string MODULE_FILE_EXTENSION = ".dll";
#elif defined(POSIX)
	const std::string MODULE_FILE_EXTENSION = ".so";
#endif

	// This is the name of the entry point symbol in the module
	const char* const SYMBOL_REGISTER_MODULE = "RegisterModule";

	// Modules have to export a symbol of this type, which gets called during DLL loading
	typedef void(*RegisterModulesFunc)(IModuleRegistry& registry);
}

// Functor operator, gets invoked on directory traversal
void ModuleLoader::processModuleFile(const fs::path& file)
{
	// Check for the correct extension of the visited file
	if (string::to_lower_copy(file.extension().string()) != MODULE_FILE_EXTENSION) return;

	std::string fullName = file.string();
	rConsole() << "ModuleLoader: Loading module '" << fullName << "'" << std::endl;

	// Create the encapsulator class
	DynamicLibraryPtr library = std::make_shared<DynamicLibrary>(fullName);

	// greebo: Try to find our entry point and invoke it and add the library to the list
	// on success. If the load fails, the shared pointer won't be added and
	// self-destructs at the end of this scope.
	if (library->failed())
	{
		rError() << "WARNING: Failed to load module " << library->getName() << ":" << std::endl;

#ifdef __linux__
		rConsoleError() << dlerror() << std::endl;
#endif
		return;
	}

	// Library was successfully loaded, lookup the symbol
	DynamicLibrary::FunctionPointer funcPtr(
		library->findSymbol(SYMBOL_REGISTER_MODULE)
	);

	if (funcPtr == nullptr)
	{
		// Symbol lookup error
		rError() << "WARNING: Could not find symbol " << SYMBOL_REGISTER_MODULE
			<< " in module " << library->getName() << ":" << std::endl;
		return;
	}

	// Brute-force conversion of the pointer to the desired type
	RegisterModulesFunc regFunc = reinterpret_cast<RegisterModulesFunc>(funcPtr);

	try
	{
		// Call the symbol and pass a reference to the ModuleRegistry
		// This method might throw a ModuleCompatibilityException in its
		// module::performDefaultInitialisation() routine.
		regFunc(ModuleRegistry::Instance());

		// Add the library to the static list (for later reference)
		_dynamicLibraryList.push_back(library);
	}
	catch (module::ModuleCompatibilityException&)
	{
		// Report this error and don't add the module to the _dynamicLibraryList
		rError() << "Compatibility mismatch loading library " << library->getName() << std::endl;
	}
}

void ModuleLoader::loadModules(const std::string& root)
{
    // Get standardised paths
    std::string stdRoot = os::standardPathWithSlash(root);

#if defined(DR_MODULES_NEXT_TO_APP)
    // Xcode output goes to the application folder right now
    std::string modulesPath = stdRoot;
    std::string pluginsPath = stdRoot;
#else
    std::string modulesPath = stdRoot + MODULES_DIR;
    std::string pluginsPath = stdRoot + PLUGINS_DIR;
#endif

    rConsole() << "ModuleLoader: loading modules from " << root << std::endl;

    // Load modules first, then plugins
	loadModulesFromPath(modulesPath);

	// Plugins are optional
    if (pluginsPath != modulesPath)
    {
		loadModulesFromPath(pluginsPath);
    }
}

void ModuleLoader::loadModulesFromPath(const std::string& path)
{
	// In case the folder is non-existent, catch the exception
	try
	{
		os::foreachItemInDirectory(path, [&](const fs::path& file)
		{
			processModuleFile(file);
		});
	}
	catch (os::DirectoryNotFoundException&)
	{
		rConsole() << "ModuleLoader::loadModules(): modules directory '"
			<< path << "' not found." << std::endl;
	}
}

void ModuleLoader::unloadModules()
{
	while (!_dynamicLibraryList.empty())
	{
		DynamicLibraryPtr lib = _dynamicLibraryList.back();

		_dynamicLibraryList.pop_back();

		lib.reset();
	}
}

} // namespace module
