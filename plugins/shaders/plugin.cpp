#include "Doom3ShaderSystem.h"

extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry& registry)
{
	module::performDefaultInitialisation(registry);

	registry.registerModule(std::make_shared<shaders::Doom3ShaderSystem>());
}
