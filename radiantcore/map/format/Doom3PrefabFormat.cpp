#include "Doom3PrefabFormat.h"

#include "i18n.h"

#include "module/StaticModule.h"

namespace map
{

// RegisterableModule implementation
const std::string& Doom3PrefabFormat::getName() const
{
	static std::string _name("Doom3PrefabLoader");
	return _name;
}

void Doom3PrefabFormat::initialiseModule(const IApplicationContext& ctx)
{
	rMessage() << getName() << ": initialiseModule called." << std::endl;

	// Register ourselves as map format
	GlobalMapFormatManager().registerMapFormat("pfb", shared_from_this());
}

void Doom3PrefabFormat::shutdownModule()
{
	// Unregister now that we're shutting down
	GlobalMapFormatManager().unregisterMapFormat(shared_from_this());
}

const std::string& Doom3PrefabFormat::getMapFormatName() const
{
	static std::string _name = "Doom 3 Prefab";
	return _name;
}

bool Doom3PrefabFormat::allowInfoFileCreation() const
{
	return false;
}

module::StaticModuleRegistration<Doom3PrefabFormat> d3PrefabModule;

} // namespace
