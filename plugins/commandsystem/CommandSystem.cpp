#include "CommandSystem.h"

#include "itextstream.h"
#include "iregistry.h"
#include "ieventmanager.h"
#include "debugging/debugging.h"

#include "CommandTokeniser.h"
#include "Command.h"
#include "Statement.h"

#include <boost/bind.hpp>
#include <boost/algorithm/string/trim.hpp>

namespace cmd
{

namespace
{
	const std::string RKEY_COMMANDSYSTEM_BINDS = "user/ui/commandsystem/binds";
}

// RegisterableModule implementation
const std::string& CommandSystem::getName() const {
	static std::string _name(MODULE_COMMANDSYSTEM);
	return _name;
}

const StringSet& CommandSystem::getDependencies() const {
	static StringSet _dependencies;

	if (_dependencies.empty())
	{
		_dependencies.insert(MODULE_XMLREGISTRY);
	}

	return _dependencies;
}

void CommandSystem::initialiseModule(const ApplicationContext& ctx)
{
	rMessage() << "CommandSystem::initialiseModule called." << std::endl;

	// Add the built-in commands
	addCommand("bind", boost::bind(&CommandSystem::bindCmd, this, _1), Signature(ARGTYPE_STRING, ARGTYPE_STRING));
	addCommand("unbind", boost::bind(&CommandSystem::unbindCmd, this, _1), ARGTYPE_STRING);
	addCommand("listCmds", boost::bind(&CommandSystem::listCmds, this, _1));
	addCommand("print", boost::bind(&CommandSystem::printCmd, this, _1), ARGTYPE_STRING);

	loadBinds();
}

void CommandSystem::shutdownModule()
{
	rMessage() << "CommandSystem: shutting down." << std::endl;

	// Save binds to registry
	saveBinds();

	// Free all commands
	_commands.clear();
}

void CommandSystem::printCmd(const ArgumentList& args) {
	for (ArgumentList::const_iterator i = args.begin(); i != args.end(); ++i) {
		rMessage() << i->getString() << (i != args.begin() ? " " : "");
	}
	rMessage() << std::endl;
}

void CommandSystem::loadBinds() {
	// Find all accelerators
	xml::NodeList nodeList = GlobalRegistry().findXPath(RKEY_COMMANDSYSTEM_BINDS + "//bind");

	if (nodeList.empty()) {
		return;
	}

	// Load all bind commands from the nodes
	for (std::size_t i = 0; i < nodeList.size(); ++i) {
		xml::Node& node = nodeList[i];

		std::string name = node.getAttributeValue("name");
		std::string statement = node.getAttributeValue("value");

		// remove all whitespace from the front and the tail
		boost::algorithm::trim(statement);

		// Create a new statement
		StatementPtr st(new Statement(
			statement,
			(node.getAttributeValue("readonly") == "1")
		));

		std::pair<CommandMap::iterator, bool> result = _commands.insert(
			CommandMap::value_type(name, st)
		);

		if (!result.second) {
			rWarning() << "Duplicate statement detected: "
				<< name << std::endl;
		}
	}
}

void CommandSystem::saveBinds() {
	// Delete all previous binds
	GlobalRegistry().deleteXPath(RKEY_COMMANDSYSTEM_BINDS + "//bind");

	for (CommandMap::const_iterator i = _commands.begin(); i != _commands.end(); ++i)
	{
		// Check if this is actually a statement
		StatementPtr st = boost::dynamic_pointer_cast<Statement>(i->second);

		if (st == NULL || st->isReadonly()) continue; // not a statement or readonly

		xml::Node node = GlobalRegistry().createKeyWithName(RKEY_COMMANDSYSTEM_BINDS, "bind", i->first);

		node.setAttributeValue("value", st->getValue());
	}
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

	// To enable this statement to be triggered via UI, register it as new event
	GlobalEventManager().addCommand(args[0].getString(), args[0].getString());
}

void CommandSystem::unbindCmd(const ArgumentList& args) {
	// Sanity check
	if (args.size() != 1) return;

	// First argument is the statement to unbind
	CommandMap::iterator found = _commands.find(args[0].getString());

	if (found == _commands.end()) {
		rError() << "Cannot unbind: " << args[0].getString()
			<< ": no such command." << std::endl;
		return;
	}

	// Check if this is actually a statement
	StatementPtr st = boost::dynamic_pointer_cast<Statement>(found->second);

	if (st != NULL && !st->isReadonly())
	{
		// This is a user-statement
		_commands.erase(found);

		// Unregister this as event too
		GlobalEventManager().removeEvent(args[0].getString());
	}
	else {
		rError() << "Cannot unbind built-in command: "
			<< args[0].getString() << std::endl;
		return;
	}
}

void CommandSystem::listCmds(const ArgumentList& args) {
	// Dump all commands
	for (CommandMap::const_iterator i = _commands.begin(); i != _commands.end(); ++i) {
		rMessage() << i->first;

		StatementPtr st = boost::dynamic_pointer_cast<Statement>(i->second);
		if (st != NULL) {
			rMessage() << " => " << st->getValue();
		}

		rMessage() << std::endl;
	}
}

void CommandSystem::foreachCommand(const std::function<void(const std::string&)>& functor)
{
	std::for_each(_commands.begin(), _commands.end(), [&] (const CommandMap::value_type& i)
	{
		functor(i.first);
	});
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
		rError() << "Cannot register command " << name
			<< ", this command is already registered." << std::endl;
	}
}

void CommandSystem::removeCommand(const std::string& name) {
	CommandMap::iterator i = _commands.find(name);

	if (i != _commands.end()) {
		_commands.erase(i);
	}
}

void CommandSystem::addStatement(const std::string& statementName,
								 const std::string& str,
								 bool saveStatementToRegistry)
{
	// Remove all whitespace at the front and the tail
	StatementPtr st(new Statement(
		boost::algorithm::trim_copy(str),
		!saveStatementToRegistry // read-only if we should not save this statement
	));

	std::pair<CommandMap::iterator, bool> result = _commands.insert(
		CommandMap::value_type(statementName, st)
	);

	if (!result.second) {
		rError() << "Cannot register statement " << statementName
			<< ", this statement is already registered." << std::endl;
	}
}

void CommandSystem::foreachStatement(const std::function<void(const std::string&)>& functor,
									 bool customStatementsOnly)
{
	std::for_each(_commands.begin(), _commands.end(), [&] (const CommandMap::value_type& i)
	{
		StatementPtr statement = boost::dynamic_pointer_cast<Statement>(i.second);

		if (statement && (!customStatementsOnly || !statement->isReadonly()))
		{
			functor(i.first);
		}
	});
}

Signature CommandSystem::getSignature(const std::string& name) {
	// Lookup the named command
	CommandMap::iterator i = _commands.find(name);

	if (i == _commands.end()) {
		return Signature(); // not found => empty signature
	}

	return i->second->getSignature();
}

namespace local
{
	// A statement consists of a command and a set of arguments
	struct Statement
	{
		// The command to invoke
		std::string command;

		// The arguments to pass
		ArgumentList args;
	};
}

void CommandSystem::execute(const std::string& input) {
	// Instantiate a CommandTokeniser to analyse the given input string
	CommandTokeniser tokeniser(input);

	if (!tokeniser.hasMoreTokens()) return; // nothing to do!

	std::vector<local::Statement> statements;
	local::Statement curStatement;

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
			curStatement = local::Statement();
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
	for (std::vector<local::Statement>::iterator i = statements.begin();
		 i != statements.end(); ++i)
	{
		// Attempt ordinary command execution
		executeCommand(i->command, i->args);
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
		rError() << "Cannot execute command " << name << ": Command not found." << std::endl;
		return;
	}

	i->second->execute(args);
}

AutoCompletionInfo CommandSystem::getAutoCompletionInfo(const std::string& prefix) {
	AutoCompletionInfo returnValue;

	returnValue.prefix = prefix;

	for (CommandMap::const_iterator i = _commands.begin(); i != _commands.end(); ++i) {
		// Check if the command matches the given prefix
		if (boost::algorithm::istarts_with(i->first, prefix)) {
			returnValue.candidates.push_back(i->first);
		}
	}

	return returnValue;
}

} // namespace cmd

extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry& registry) {
	registry.registerModule(cmd::CommandSystemPtr(new cmd::CommandSystem));

	// Initialise the streams using the given application context
	module::initialiseStreams(registry.getApplicationContext());

	// Remember the reference to the ModuleRegistry
	module::RegistryReference::Instance().setRegistry(registry);

	// Set up the assertion handler
	GlobalErrorHandler() = registry.getApplicationContext().getErrorHandlingFunction();
}
