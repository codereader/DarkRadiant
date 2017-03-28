#include "itextstream.h"
#include "imodule.h"
#include "FontManager.h"

extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry& registry)
{
	module::performDefaultInitialisation(registry);

	registry.registerModule(fonts::FontManagerPtr(new fonts::FontManager));
}
