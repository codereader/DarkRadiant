#pragma once

#include "icommandsystem.h"
#include <map>
#include "CaseInsensitiveCompare.h"
#include "Executable.h"

namespace cmd
{

class CommandSystem :
	public ICommandSystem
{
	// The named executables (case-insensitive lookup)
	typedef std::map<std::string, ExecutablePtr, CaseInsensitiveCompare> CommandMap;
	CommandMap _commands;

public:
	void foreachCommand(const std::function<void(const std::string&)>& functor);

	void addCommand(const std::string& name, Function func, const Signature& signature = Signature());
	void removeCommand(const std::string& name);

	void addStatement(const std::string& statementName, const std::string& string, bool saveStatementToRegistry = true);
	void foreachStatement(const std::function<void(const std::string&)>& functor, bool customStatementsOnly);

	// Retrieve the signature for the given command
	Signature getSignature(const std::string& name);

	// Execute the given command sequence
	void execute(const std::string& input);

	void executeCommand(const std::string& name);
	void executeCommand(const std::string& name, const Argument& arg1);
	void executeCommand(const std::string& name, const Argument& arg1, const Argument& arg2);
	void executeCommand(const std::string& name, const Argument& arg1, const Argument& arg2, const Argument& arg3);

	// For more than 3 arguments, use this method to pass a vector of arguments
	void executeCommand(const std::string& name, const ArgumentList& args);

	// Get autocompletion suggestions for the given prefix
	AutoCompletionInfo getAutoCompletionInfo(const std::string& prefix);

	// The "bind" command
	void bindCmd(const ArgumentList& args);
	void unbindCmd(const ArgumentList& args);
	void listCmds(const ArgumentList& args);
	void printCmd(const ArgumentList& args);

	// RegisterableModule implementation
	const std::string& getName() const;
	const StringSet& getDependencies() const;
	void initialiseModule(const ApplicationContext& ctx);
	void shutdownModule();

private:

	// Save/load bind strings from Registry
	void loadBinds();
	void saveBinds();
};
typedef boost::shared_ptr<CommandSystem> CommandSystemPtr;

} // namespace cmd
