#ifndef _COMMANDSYSTEM_H_
#define _COMMANDSYSTEM_H_

#include "icommandsystem.h"
#include <map>

namespace cmd {

class CommandSystem :
	public ICommandSystem
{
	struct Command 
	{
		// The actual function to call
		Function function;

		// The number and types of arguments to use
		Signature signature;

		Command(const Function& _func, const Signature& _sign) :
			function(_func),
			signature(_sign)
		{}
	};
	typedef boost::shared_ptr<Command> CommandPtr;

	// A statement consists of a command and a set of arguments
	struct Statement
	{
		// The command to invoke
		std::string command;

		// The arguments to pass
		ArgumentList args;
	};

	// The named commands
	typedef std::map<std::string, CommandPtr> CommandMap;
	CommandMap _commands;

public:
	void addCommand(const std::string& name, Function func, const Signature& signature);

	// Execute the given command sequence
	void execute(const std::string& input);

	void executeCommand(const std::string& name);
	void executeCommand(const std::string& name, const Argument& arg1);
	void executeCommand(const std::string& name, const Argument& arg1, const Argument& arg2);
	void executeCommand(const std::string& name, const Argument& arg1, const Argument& arg2, const Argument& arg3);

	// For more than 3 arguments, use this method to pass a vector of arguments
	void executeCommand(const std::string& name, const ArgumentList& args);

	// RegisterableModule implementation
	const std::string& getName() const;
	const StringSet& getDependencies() const;
	void initialiseModule(const ApplicationContext& ctx);
	void shutdownModule();
};
typedef boost::shared_ptr<CommandSystem> CommandSystemPtr;

} // namespace cmd

#endif /* _COMMANDSYSTEM_H_ */
