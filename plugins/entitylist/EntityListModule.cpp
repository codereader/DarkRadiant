#include "EntityList.h"

#include "itextstream.h"
#include "imodule.h"
#include "icommandsystem.h"
#include "ieventmanager.h"
#include "iradiant.h"
#include "debugging/debugging.h"

class EntityListModule :
	public RegisterableModule
{
public:
	// RegisterableModule implementation
	virtual const std::string& getName() const;
	virtual const StringSet& getDependencies() const;
	virtual void initialiseModule(const ApplicationContext& ctx);
	virtual void shutdownModule();
};
typedef std::shared_ptr<EntityListModule> EntityListModulePtr;

// RegisterableModule implementation
const std::string& EntityListModule::getName() const
{
	static std::string _name("EntityList");
	return _name;
}

const StringSet& EntityListModule::getDependencies() const
{
	static StringSet _dependencies;

	if (_dependencies.empty())
	{
		_dependencies.insert(MODULE_EVENTMANAGER);
		_dependencies.insert(MODULE_COMMANDSYSTEM);
	}

	return _dependencies;
}

void EntityListModule::initialiseModule(const ApplicationContext& ctx)
{
	rMessage() << "EntityListModule::initialiseModule called" << std::endl;

	GlobalCommandSystem().addCommand("EntityList", ui::EntityList::toggle);
	GlobalEventManager().addCommand("EntityList", "EntityList");
}

void EntityListModule::shutdownModule()
{}

extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry& registry)
{
	module::performDefaultInitialisation(registry);

	registry.registerModule(EntityListModulePtr(new EntityListModule));
}
