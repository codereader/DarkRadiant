#include "itextstream.h"

#include "ScriptingSystem.h"
#include "debugging/debugging.h"

/**
 * greebo: This is the module entry point which the main binary will look for.
 * The symbol RegisterModule is called with the singleton ModuleRegistry as argument.
 */
extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry& registry)
{
	module::performDefaultInitialisation(registry);

	registry.registerModule(script::ScriptingSystemPtr(new script::ScriptingSystem));
}
