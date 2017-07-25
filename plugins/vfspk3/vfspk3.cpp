#include "vfspk3.h"

#include "iarchive.h"
#include "ifilesystem.h"

#include "Doom3FileSystem.h"

extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry& registry)
{
	module::performDefaultInitialisation(registry);

	registry.registerModule(Doom3FileSystemPtr(new Doom3FileSystem));
}
