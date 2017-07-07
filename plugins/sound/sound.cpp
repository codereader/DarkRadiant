#include "SoundManager.h"

#include "ifilesystem.h"
#include "imodule.h"

extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry& registry) 
{
	module::performDefaultInitialisation(registry);

	registry.registerModule(std::make_shared<sound::SoundManager>());
}
