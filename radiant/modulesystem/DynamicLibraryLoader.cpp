#include "DynamicLibraryLoader.h"

#include "itextstream.h"
#include "ModuleRegistry.h"

namespace module 
{

namespace
{
	// This is the name of the entry point symbol in the module
	const char* const SYMBOL_REGISTER_MODULE = "RegisterModule";

	// Modules have to export a symbol of this type, which gets called during DLL loading
	typedef void (*RegisterModulesFunc)(IModuleRegistry& registry);
}

DynamicLibraryLoader::DynamicLibraryLoader(DynamicLibraryPtr library,
										   DynamicLibraryList& targetList)
{
	assert(library); // Don't take NULL pointers

	if (!library->failed())
	{
		// Library was successfully loaded, lookup the symbol
		DynamicLibrary::FunctionPointer funcPtr(
			library->findSymbol(SYMBOL_REGISTER_MODULE)
		);

		if (funcPtr != nullptr) 
		{
			// Brute-force conversion of the pointer to the desired type
			RegisterModulesFunc regFunc = reinterpret_cast<RegisterModulesFunc>(funcPtr);

			try
			{
				// Call the symbol and pass a reference to the ModuleRegistry
				// This method might throw a ModuleCompatibilityException in its
				// module::performDefaultInitialisation() routine.
				regFunc(ModuleRegistry::Instance());

				// Add the library to the static list (for later reference)
				targetList.push_back(library);
			}
			catch (module::ModuleCompatibilityException&)
			{
				// Report this error and don't add the module to the targetList
				rError() << "Compatibility mismatch loading library " << library->getName() << std::endl;
			}
		}
		else
		{
			// Symbol lookup error
			rError() << "WARNING: Could not find symbol " << SYMBOL_REGISTER_MODULE
			          << " in module " << library->getName() << ":" << std::endl;
		}
	}
	else
	{
		rError() << "WARNING: Failed to load module " << library->getName() << ":" << std::endl;
#ifdef __linux__
        rConsoleError() << dlerror() << std::endl;
#endif
	}
}

} // namespace module
