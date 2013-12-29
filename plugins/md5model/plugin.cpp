#include "imodule.h"

#include "debugging/debugging.h"
#include "itextstream.h"
#include "MD5ModelLoader.h"
#include "MD5AnimationCache.h"

// DarkRadiant module entry point
extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry& registry)
{
	registry.registerModule(md5::MD5ModelLoaderPtr(new md5::MD5ModelLoader));
	registry.registerModule(md5::MD5AnimationCachePtr(new md5::MD5AnimationCache));

	// Initialise the streams using the given application context
	module::initialiseStreams(registry.getApplicationContext());

	// Remember the reference to the ModuleRegistry
	module::RegistryReference::Instance().setRegistry(registry);

	// Set up the assertion handler
	GlobalErrorHandler() = registry.getApplicationContext().getErrorHandlingFunction();
}
