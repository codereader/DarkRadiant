#include "XMLRegistry.h"
#include "itextstream.h"

/**
 * greebo: This is the module entry point which the main binary will look for.
 * The symbol RegisterModule is called with the singleton ModuleRegistry as argument.
 */
extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry& registry) {
	registry.registerModule(XMLRegistryPtr(new XMLRegistry));
	
	// Initialise the streams
	const ApplicationContext& ctx = registry.getApplicationContext();
	GlobalOutputStream::instance().setOutputStream(ctx.getOutputStream());
	GlobalErrorStream::instance().setOutputStream(ctx.getErrorStream());
	
	// Remember the reference to the ModuleRegistry
	module::RegistryReference::Instance().setRegistry(registry);
}
