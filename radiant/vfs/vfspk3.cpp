#include "vfspk3.h"

#include "ifilesystem.h"
#include "Doom3FileSystem.h"

extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry& registry)
{
	module::performDefaultInitialisation(registry);

	registry.registerModule(std::make_shared<vfs::Doom3FileSystem>());
}
