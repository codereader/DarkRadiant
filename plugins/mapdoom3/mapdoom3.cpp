#include "Doom3MapFormat.h"
#include "Doom3PrefabFormat.h"
#include "Quake4MapFormat.h"
#include "Quake3MapFormat.h"
#include "aas/Doom3AasFileLoader.h"

#include "imapformat.h"
#include "itextstream.h"
#include "debugging/debugging.h"

extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry& registry)
{
	if (!module::checkModuleCompatibility(registry)) return;

	registry.registerModule(std::make_shared<map::Doom3MapFormat>());
	registry.registerModule(std::make_shared<map::Quake4MapFormat>());
	registry.registerModule(std::make_shared<map::Doom3PrefabFormat>());
	registry.registerModule(std::make_shared<map::Quake3MapFormat>());
    registry.registerModule(std::make_shared<map::Doom3AasFileLoader>());

	// Initialise the streams using the given application context
	module::initialiseStreams(registry.getApplicationContext());

	// Remember the reference to the ModuleRegistry
	module::RegistryReference::Instance().setRegistry(registry);

	// Set up the assertion handler
	GlobalErrorHandler() = registry.getApplicationContext().getErrorHandlingFunction();
}
