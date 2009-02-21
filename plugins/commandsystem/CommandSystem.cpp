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

	// Free all commands
	_commands.clear();
}

void CommandSystem::addCommand(const std::string& name, 
					CommandFunction func, 
					const CommandSignature& signature)
{
	// Create a new command
	CommandPtr cmd(new Command(func, signature));

	std::pair<CommandMap::iterator, bool> result = _commands.insert(
		CommandMap::value_type(name, cmd)
	);

	if (!result.second) {
		globalErrorStream() << "Cannot register command " << name 
			<< ", this command is already registered." << std::endl;
	}
}

void CommandSystem::executeCommand(const std::string& name, const Argument& arg1) {
	ArgumentList args(1);
	args[0] = arg1;

	executeCommand(name, args);
}

void CommandSystem::executeCommand(const std::string& name, const Argument& arg1, 
	const Argument& arg2)
{
	ArgumentList args(2);
	args[0] = arg1;
	args[1] = arg2;

	executeCommand(name, args);
}

void CommandSystem::executeCommand(const std::string& name, 
	const Argument& arg1, const Argument& arg2, 
	const Argument& arg3)
{
	ArgumentList args(2);
	args[0] = arg1;
	args[1] = arg2;
	args[2] = arg3;

	executeCommand(name, args);
}

void CommandSystem::executeCommand(const std::string& name, const ArgumentList& args) {
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
