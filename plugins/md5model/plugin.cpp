#include "imodule.h"

#include "itextstream.h"
#include "MD5ModelLoader.h"

// DarkRadiant module entry point
extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry& registry) {
	registry.registerModule(md5::MD5ModelLoaderPtr(new md5::MD5ModelLoader));
	
	// Initialise the streams using the given application context
	module::initialiseStreams(registry.getApplicationContext());
	
	// Remember the reference to the ModuleRegistry
	module::RegistryReference::Instance().setRegistry(registry);
}
