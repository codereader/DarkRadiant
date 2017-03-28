#include "plugin.h"

#include "Doom3ShaderSystem.h"
#include "imodule.h"

extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry& registry)
{
	module::performDefaultInitialisation(registry);

	registry.registerModule(shaders::Doom3ShaderSystemPtr(new shaders::Doom3ShaderSystem));
}
