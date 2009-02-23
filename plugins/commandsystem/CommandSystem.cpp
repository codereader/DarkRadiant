#include "CommandSystem.h"

#include "itextstream.h"
#include "CommandTokeniser.h"

#include <boost/bind.hpp>
#include <boost/algorithm/string/trim.hpp>

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

	// Add the built-in commands
	addCommand("bind", boost::bind(&CommandSystem::bindCmd, this, _1), Signature(ARGTYPE_STRING, ARGTYPE_STRING));
	addCommand("unbind", boost::bind(&CommandSystem::unbindCmd, this, _1), Signature(ARGTYPE_STRING));
}

void CommandSystem::shutdownModule() {
	globalOutputStream() << "CommandSystem: shutting down.\n";

	// Free all commands
	_commands.clear();
	_binds.clear();
}

void CommandSystem::bindCmd(const ArgumentList& args) {
	// Sanity check
	if (args.size() != 2) return;

	// First argument is the command name
	// Second argument is the command to be called plus its arguments

	// Use a tokeniser to split the second argument into its pieces
	// and trim all unnecessary whitespace
	std::string input = args[1].getString();

	// Add the statement - bind complete
	addStatement(args[0].getString(), input);
}

void CommandSystem::unbindCmd(const ArgumentList& args) {
	// Sanity check
	if (args.size() != 1) return;

	// First argument is the statement to unbind
	BindMap::iterator found = _binds.find(args[0].getString());

	if (found != _binds.end()) {
		_binds.erase(found);
	}
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

void CommandSystem::addStatement(const std::string& statementName, const std::string& str) {
	// First check if a command with this name already exists
	if (_binds.find(statementName) != _binds.end()) {
		globalErrorStream() << "Cannot register statement " << statementName 
			<< ", a command with this name is already registered." << std::endl;
		return;
	}

	// Create a new statement
	std::string statement(str);

	// Remove all whitespace at the front and the tail
	boost::algorithm::trim(statement);
	
	std::pair<BindMap::iterator, bool> result = _binds.insert(
		BindMap::value_type(statementName, statement)
	);

	if (!result.second) {
		globalErrorStream() << "Cannot register statement " << statementName 
			<< ", this statement is already registered." << std::endl;
	}
}

void CommandSystem::execute(const std::string& input) {
	// Instantiate a CommandTokeniser to analyse the given input string
	CommandTokeniser tokeniser(input);

	if (!tokeniser.hasMoreTokens()) return; // nothing to do!

	std::vector<Statement> statements;

	Statement curStatement;

	while (tokeniser.hasMoreTokens()) {
		// Inspect the next token
		std::string token = tokeniser.nextToken();

		if (token.empty()) {
			continue; // skip empty tokens
		}
		else if (token == ";") {
			// Finish the current statement
			if (!curStatement.command.empty()) {
				// Add the non-empty statement to our list
				statements.push_back(curStatement);
			}

			// Clear the statement
			curStatement = Statement();
			continue;
		}
		// Token is not a semicolon
		else if (curStatement.command.empty()) {
			// The statement is still without command name, take this one
			curStatement.command = token;
			continue;
		}
		else {
			// Non-empty token, command name is already known, so
			// this must be an argument
			curStatement.args.push_back(token);
		}
	}

	// Check if we have an unfinished statement
	if (!curStatement.command.empty()) {
		// Add the non-empty statement to our list
		statements.push_back(curStatement);
	}

	// Now execute the statements
	for (std::vector<Statement>::iterator i = statements.begin(); 
		 i != statements.end(); ++i)
	{
		// If the arguments are empty, we check for a named bind
		if (i->args.empty() && _binds.find(i->command) != _binds.end()) {
			// This is matching a statement, execute it
			executeStatement(i->command);
		}
		else {
			// Attempt ordinary command execution
			executeCommand(i->command, i->args);
		}
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

void CommandSystem::executeStatement(const std::string& name) {
	// Find the named statement
	BindMap::const_iterator i = _binds.find(name);

	if (i == _binds.end()) {
		globalErrorStream() << "Cannot execute statement " << name 
			<< ": Statement not found." << std::endl;
		return;
	}

	if (i->second == name) {
		// Don't execute self and begin an infinite loop
		globalErrorStream() << "Possible infinite loop detected, "
			<< "cannot execute bind: " << name << std::endl;
		return;
	}

	// Execute the contained string
	execute(i->second);
}

} // namespace cmd

extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry& registry) {
	registry.registerModule(cmd::CommandSystemPtr(new cmd::CommandSystem));
	
	// Initialise the streams using the given application context
	module::initialiseStreams(registry.getApplicationContext());
	
	// Remember the reference to the ModuleRegistry
	module::RegistryReference::Instance().setRegistry(registry);
}
