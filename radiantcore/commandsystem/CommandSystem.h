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
	void foreachCommand(const std::function<void(const std::string&)>& functor) override;

	void addCommand(const std::string& name, Function func, const Signature& signature = Signature()) override;
	bool commandExists(const std::string& name) override;
	void removeCommand(const std::string& name) override;

	void addStatement(const std::string& statementName, const std::string& string, bool saveStatementToRegistry = true) override;
	void foreachStatement(const std::function<void(const std::string&)>& functor, bool customStatementsOnly) override;

	// Retrieve the signature for the given command
	Signature getSignature(const std::string& name) override;

	// Execute the given command sequence
	void execute(const std::string& input) override;

	void executeCommand(const std::string& name) override;
	void executeCommand(const std::string& name, const Argument& arg1) override;
	void executeCommand(const std::string& name, const Argument& arg1, const Argument& arg2) override;
	void executeCommand(const std::string& name, const Argument& arg1, const Argument& arg2, const Argument& arg3) override;

	// For more than 3 arguments, use this method to pass a vector of arguments
	void executeCommand(const std::string& name, const ArgumentList& args) override;

	// Get autocompletion suggestions for the given prefix
	AutoCompletionInfo getAutoCompletionInfo(const std::string& prefix) override;

	// The "bind" command
	void bindCmd(const ArgumentList& args);
	void unbindCmd(const ArgumentList& args);
	void listCmds(const ArgumentList& args);
	void printCmd(const ArgumentList& args);

	// RegisterableModule implementation
	const std::string& getName() const;
	const StringSet& getDependencies() const;
	void initialiseModule(const IApplicationContext& ctx);
	void shutdownModule();

private:

	// Save/load bind strings from Registry
	void loadBinds();
	void saveBinds();
};
typedef std::shared_ptr<CommandSystem> CommandSystemPtr;

} // namespace cmd
