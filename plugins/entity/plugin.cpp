#include "plugin.h"

#include "EntityCreator.h"
#include "imodule.h"

extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry& registry)
{
	module::performDefaultInitialisation(registry);

	registry.registerModule(entity::Doom3EntityCreatorPtr(
			new entity::Doom3EntityCreator
	));
}
