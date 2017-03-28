#include "imodule.h"
#include "EClassManager.h"

extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry& registry)
{
	module::performDefaultInitialisation(registry);

	registry.registerModule(eclass::EClassManagerPtr(new eclass::EClassManager));
}
