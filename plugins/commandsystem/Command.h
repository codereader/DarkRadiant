#ifndef _COMMAND_H_
#define _COMMAND_H_

#include "itextstream.h"
#include "Executable.h"

namespace cmd {

class Command :
	public Executable
{
	// The actual function to call
	Function _function;

	// The number and types of arguments to use
	Signature _signature;

public:
	Command(const Function& function, const Signature& signature) :
		_function(function),
		_signature(signature)
	{}

	virtual void execute(const ArgumentList& args) {
		// Check matching number of arguments
		if (args.size() != _signature.size()) {
			globalErrorStream() << "Cannot execute command: Wrong number of arguments. " 
				<< "(" << args.size() << " passed instead of " << _signature.size() << ")" << std::endl;
			return;
		}

		// Checks passed, call the command
		_function(args);
	}
};
typedef boost::shared_ptr<Command> CommandPtr;

} // namespace cmd

#endif /* _COMMAND_H_ */
