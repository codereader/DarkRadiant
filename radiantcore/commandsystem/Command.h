#ifndef _COMMAND_H_
#define _COMMAND_H_

#include "icommandsystem.h"
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

	// Optional function to test if command can be run
	CheckFunction _checkFunction;

public:
	Command(const Function& function, const Signature& signature, CheckFunction checkFunc = {}) :
		_function(function),
		_signature(signature),
		_checkFunction(checkFunc)
	{}

	Signature getSignature() {
		return _signature;
	}

    /// Test if this command is able to be executed
    virtual bool canExecute() const
    {
        if (_checkFunction)
            return _checkFunction();
        else
            return true;
    }

    virtual void execute(const ArgumentList& args) {
		// Check arguments
		if (_signature.size() < args.size()) {
			// Too many arguments, that's for sure
			rError() << "Cannot execute command: Too many arguments. "
				<< "(max. " << _signature.size() << " arguments required)" << std::endl;
			return;
		}

		// Check matching arguments
		ArgumentList::const_iterator arg = args.begin();
		for (Signature::const_iterator cur = _signature.begin(); cur != _signature.end(); ++cur) {

			std::size_t curFlags = *cur;
			bool curIsOptional = ((curFlags & ARGTYPE_OPTIONAL) != 0);

			// If arguments have run out, all remaining parts of the signature must be optional
			if (arg == args.end()) {
				// Non-optional arguments will cause an error
				if (!curIsOptional) {
					rError() << "Cannot execute command: Missing arguments. " << std::endl;
					return;
				}
			}
			else {
				// We have incoming arguments to match our signature
				if ((curFlags & arg->getType()) == 0) {
					// Type mismatch
					rError() << "Cannot execute command: Type mismatch at argument: "
						<< arg->getString() << std::endl;
					return;
				}
			}

			// Increase argument iterator if possible
			if (arg != args.end()) {
				++arg;
			}
		}

		// Checks passed, call the command
		_function(args);
	}
};
typedef std::shared_ptr<Command> CommandPtr;

} // namespace cmd

#endif /* _COMMAND_H_ */
