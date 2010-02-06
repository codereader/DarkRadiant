#include "MapFormatManager.h"

#include "itextstream.h"
#include "modulesystem/StaticModule.h"

namespace map
{

MapFormatPtr MapFormatManager::getMapFormat(const std::string& name)
{
	return MapFormatPtr();
}

// RegisterableModule implementation
const std::string& MapFormatManager::getName() const
{
	static std::string _name(MODULE_MAPFORMATMANAGER);
	return _name;
}

const StringSet& MapFormatManager::getDependencies() const
{
	static StringSet _dependencies; // no deps
	return _dependencies;
}

void MapFormatManager::initialiseModule(const ApplicationContext& ctx)
{
	globalOutputStream() << getName() << "::initialiseModule called." << std::endl;
}

// Creates the static module instance
module::StaticModule<MapFormatManager> staticMapFormatManagerModule;

}
