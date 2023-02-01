#include "ModuleLoader.h"

#include "itextstream.h"
#include <iostream>
#include "imodule.h"

#include "os/dir.h"
#include "os/path.h"

#include "ModuleRegistry.h"
#include "module/CoreModule.h"

#include "string/case_conv.h"

namespace module
{

namespace
{
	// This is the name of the entry point symbol in the module
	const char* const SYMBOL_REGISTER_MODULE = "RegisterModule";

	// Modules have to export a symbol of this type, which gets called during DLL loading
	typedef void(*RegisterModulesFunc)(IModuleRegistry& registry);
}

ModuleLoader::ModuleLoader(IModuleRegistry& registry) :
	_registry(registry)
{}

// Functor operator, gets invoked on directory traversal
void ModuleLoader::processModuleFile(const fs::path& file)
{
	// Check for the correct extension of the visited file
	if (string::to_lower_copy(file.extension().string()) != MODULE_FILE_EXTENSION) return;

	std::string fullName = file.string();
	rMessage() << "ModuleLoader: Loading module '" << fullName << "'" << std::endl;

	// Skip the core module binary
	if (file.filename() == CoreModule::Filename())
	{
		return;
	}

	// Create the encapsulator class
	auto library = std::make_shared<DynamicLibrary>(fullName);

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
	auto funcPtr = library->findSymbol(SYMBOL_REGISTER_MODULE);

	if (funcPtr == nullptr)
	{
		// Symbol lookup error
		rError() << "WARNING: Could not find symbol " << SYMBOL_REGISTER_MODULE
			<< " in module " << library->getName() << ":" << std::endl;
		return;
	}

	// Brute-force conversion of the pointer to the desired type
	auto regFunc = reinterpret_cast<RegisterModulesFunc>(funcPtr);

	try
	{
		// Call the symbol and pass a reference to the ModuleRegistry
		// This method might throw a ModuleCompatibilityException in its
		// module::performDefaultInitialisation() routine.
		regFunc(_registry);

		// Add the library to the static list (for later reference)
		_dynamicLibraryList.push_back(library);
	}
	catch (module::ModuleCompatibilityException&)
	{
		// Report this error and don't add the module to the _dynamicLibraryList
		rError() << "Compatibility mismatch loading library " << library->getName() << std::endl;
	}
	catch (std::runtime_error& ex)
	{
		// Report this error and don't add the module to the _dynamicLibraryList
		rError() << "Failure registering module " << library->getName() << ": " << ex.what() << std::endl;
	}
}

#if 0
void ModuleLoader::loadModules(const std::string& libraryPath)
{
    // Get standardised paths
    std::string stdRoot = os::standardPathWithSlash(libraryPath);

#if defined(DR_MODULES_NEXT_TO_APP)
    // Xcode output goes to the application folder right now
    std::string modulesPath = stdRoot;
    std::string pluginsPath = stdRoot;
#else
    std::string modulesPath = stdRoot + MODULES_DIR;
    std::string pluginsPath = stdRoot + PLUGINS_DIR;
#endif

    rMessage() << "ModuleLoader: loading modules from " << libraryPath << std::endl;

    // Load modules first, then plugins
	loadModulesFromPath(modulesPath);

	// Plugins are optional
    if (pluginsPath != modulesPath)
    {
		loadModulesFromPath(pluginsPath);
    }
}
#endif

void ModuleLoader::loadModulesFromPath(const std::string& path)
{
	rMessage() << "ModuleLoader: loading modules from " << path << std::endl;

	// In case the folder is non-existent, catch the exception
	try
	{
		os::forEachItemInDirectory(os::standardPathWithSlash(path), [&](const fs::path& file)
		{
			processModuleFile(file);
		});
	}
	catch (os::DirectoryNotFoundException&)
	{
		rError() << "ModuleLoader::loadModules(): modules directory '"
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
