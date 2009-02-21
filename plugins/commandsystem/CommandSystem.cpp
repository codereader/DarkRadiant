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

void CommandSystem::addCommand(const std::string& name, Function func, 
	const Signature& signature)
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

void CommandSystem::executeCommand(const std::string& name) {
	executeCommand(name, ArgumentList());
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
	// Find the named command
	CommandMap::const_iterator i = _commands.find(name);

	if (i == _commands.end()) {
		globalErrorStream() << "Cannot execute command " << name << ": Command not found." << std::endl;
		return;
	}

	const Command& cmd = *i->second;

	// Check matching number of arguments
	if (args.size() != cmd.signature.size()) {
		globalErrorStream() << "Cannot execute command " << name << ": Wrong number of arguments. " 
			<< "(" << args.size() << " passed instead of " << cmd.signature.size() << ")" << std::endl;
		return;
	}

	// Checks passed, call the command
	cmd.function(args);
}

} // namespace cmd

extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry& registry) {
	registry.registerModule(cmd::CommandSystemPtr(new cmd::CommandSystem));
	
	// Initialise the streams using the given application context
	module::initialiseStreams(registry.getApplicationContext());
	
	// Remember the reference to the ModuleRegistry
	module::RegistryReference::Instance().setRegistry(registry);
}
