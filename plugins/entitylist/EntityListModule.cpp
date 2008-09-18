#include "EntityList.h"

#include "imodule.h"
#include "ieventmanager.h"
#include "iradiant.h"
#include "stream/textstream.h"
#include "generic/callback.h"

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
typedef boost::shared_ptr<EntityListModule> EntityListModulePtr;

// RegisterableModule implementation
const std::string& EntityListModule::getName() const {
	static std::string _name("EntityList");
	return _name;
}

const StringSet& EntityListModule::getDependencies() const {
	static StringSet _dependencies;
	
	if (_dependencies.empty()) {
		_dependencies.insert(MODULE_EVENTMANAGER);
	}
	
	return _dependencies;
}

void EntityListModule::initialiseModule(const ApplicationContext& ctx) {
	globalOutputStream() << "EntityListModule::initialiseModule called\n";
	
	GlobalEventManager().addCommand(
		"EntityList", 
		FreeCaller<ui::EntityList::toggle>()
	);
}

void EntityListModule::shutdownModule() {
	ui::EntityList::destroyInstance();
}

extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry& registry) {
	registry.registerModule(EntityListModulePtr(new EntityListModule));
	
	// Initialise the streams using the given application context
	module::initialiseStreams(registry.getApplicationContext());
	
	// Remember the reference to the ModuleRegistry
	module::RegistryReference::Instance().setRegistry(registry);
}
