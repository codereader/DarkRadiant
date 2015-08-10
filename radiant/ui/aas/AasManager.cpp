#include "AasManager.h"

#include "modulesystem/StaticModule.h"

namespace map
{

// RegisterableModule implementation
const std::string& AasManager::getName() const
{
	static std::string _name("AasManager");
	return _name;
}

const StringSet& AasManager::getDependencies() const
{
	static StringSet _dependencies;

	if (_dependencies.empty())
	{
		_dependencies.insert(MODULE_EVENTMANAGER);
		_dependencies.insert(MODULE_COMMANDSYSTEM);
        _dependencies.insert(MODULE_AASFILEMANAGER);
	}

	return _dependencies;
}

void AasManager::initialiseModule(const ApplicationContext& ctx)
{
	rMessage() << "AasManager::initialiseModule called." << std::endl;


}

// Define the static module
module::StaticModule<AasManager> aasManagerModule;

}

map::AasManager& GlobalAasManager()
{
    return *map::aasManagerModule.getModule();
}
