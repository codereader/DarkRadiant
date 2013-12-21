#include "Doom3MapFormat.h"
#include "Doom3PrefabFormat.h"
#include "Quake4MapFormat.h"
#include "Quake3MapFormat.h"
#include "compiler/Doom3MapCompiler.h"

#include "imapformat.h"
#include "itextstream.h"
#include "debugging/debugging.h"

extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry& registry)
{
	registry.registerModule(map::Doom3MapFormatPtr(new map::Doom3MapFormat));
	registry.registerModule(map::Quake4MapFormatPtr(new map::Quake4MapFormat));
	registry.registerModule(map::Doom3PrefabFormatPtr(new map::Doom3PrefabFormat));
	registry.registerModule(map::Doom3MapCompilerPtr(new map::Doom3MapCompiler));
	registry.registerModule(map::Quake3MapFormatPtr(new map::Quake3MapFormat));
	
	// Initialise the streams using the given application context
	module::initialiseStreams(registry.getApplicationContext());

	// Remember the reference to the ModuleRegistry
	module::RegistryReference::Instance().setRegistry(registry);

	// Set up the assertion handler
	GlobalErrorHandler() = registry.getApplicationContext().getErrorHandlingFunction();
}
