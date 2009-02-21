#ifndef _ICOMMANDSYSTEM_H_
#define _ICOMMANDSYSTEM_H_

#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include "math/Vector3.h"

#include "imodule.h"

namespace cmd {

// One command argument, provides several getter methods
class CommandArgument
{
public:
	virtual std::string getString() const = 0;
	virtual int getInt() const = 0;
	virtual double getDouble() const = 0;
	virtual Vector3 getVector() const = 0;
	virtual void* getPointer() const = 0;
};

typedef std::vector<CommandArgument> CommandArguments;

/**
 * greebo: A command target must take a CommandArguments argument, like this:
 *
 * void execute(const CommandArguments& args);
 *
 * This can be both a free function and a member function.
 */
typedef boost::function<void (const CommandArguments&)> CommandFunction;

enum ArgumentType
{
	ARGTYPE_STRING,
	ARGTYPE_INT,
	ARGTYPE_DOUBLE,
	ARGTYPE_VECTOR3,
	ARGTYPE_POINTER,
	NUM_ARGTYPES,
};

// A command signature consists just of arguments, no return types
typedef std::vector<ArgumentType> CommandSignature;

class ICommandSystem :
	public RegisterableModule
{
public:
	/**
	 * greebo: Declares a new command with the given signature.
	 */
	virtual void addCommand(const std::string& name, 
						    CommandFunction func, 
							const CommandSignature& signature = CommandSignature()) = 0;
};
typedef boost::shared_ptr<ICommandSystem> ICommandSystemPtr;

} // namespace cmd

const std::string MODULE_COMMANDSYSTEM("CommandSystem");

// This is the accessor for the commandsystem
inline cmd::ICommandSystem& GlobalCommandSystem() {
	// Cache the reference locally
	static cmd::ICommandSystem& _cmdSystem(
		*boost::static_pointer_cast<cmd::ICommandSystem>(
			module::GlobalModuleRegistry().getModule(MODULE_COMMANDSYSTEM)
		)
	);
	return _cmdSystem;
}

#endif /* _ICOMMANDSYSTEM_H_ */
