#include "CommandSystem.h"

#include "itextstream.h"

namespace cmd {

// RegisterableModule implementation
const std::string& CommandSystem::getName() const {
	static std::string _name(MODULE_COMMANDSYSTEM);
	return _name;
}

const StringSet& CommandSystem::getDependencies() const {
	static StringSet _dependencies;
	return _dependencies;
}

void CommandSystem::initialiseModule(const ApplicationContext& ctx) {
	globalOutputStream() << "CommandSystem::initialiseModule called.\n";
}

void CommandSystem::shutdownModule() {
	globalOutputStream() << "CommandSystem: shutting down.\n";
}

void CommandSystem::addCommand(const std::string& name, 
					CommandFunction func, 
					const CommandSignature& signature)
{
	// TODO
}

} // namespace cmd

extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry& registry) {
	registry.registerModule(cmd::CommandSystemPtr(new cmd::CommandSystem));
	
	// Initialise the streams using the given application context
	module::initialiseStreams(registry.getApplicationContext());
	
	// Remember the reference to the ModuleRegistry
	module::RegistryReference::Instance().setRegistry(registry);
}
