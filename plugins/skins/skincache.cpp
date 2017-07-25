#include "Doom3SkinCache.h"
#include "imodule.h"

extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry& registry) 
{
	module::performDefaultInitialisation(registry);

	registry.registerModule(skins::Doom3SkinCachePtr(new skins::Doom3SkinCache));
}
