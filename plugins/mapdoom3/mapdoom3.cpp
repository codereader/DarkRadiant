#include "Doom3MapFormat.h"
#include "Doom3PrefabFormat.h"
#include "Quake4MapFormat.h"
#include "Quake3MapFormat.h"
#include "aas/Doom3AasFileLoader.h"

#include "imapformat.h"

extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry& registry)
{
	module::performDefaultInitialisation(registry);

	registry.registerModule(std::make_shared<map::Doom3MapFormat>());
	registry.registerModule(std::make_shared<map::Quake4MapFormat>());
	registry.registerModule(std::make_shared<map::Doom3PrefabFormat>());
	registry.registerModule(std::make_shared<map::Quake3MapFormat>());
    registry.registerModule(std::make_shared<map::Doom3AasFileLoader>());
}
