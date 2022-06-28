#ifndef _EXECUTABLE_H_
#define _EXECUTABLE_H_

#include "icommandsystem.h"
#include <memory>

namespace cmd {

/// Common interface for Command and Statement objects
class Executable
{
public:
	/// Destructor
	virtual ~Executable() {}

	/// Execute this object with the given set of arguments.
	virtual void execute(const ArgumentList& args) = 0;

	/// Test if this Executable can currently run
	virtual bool canExecute() const = 0;

	/// Returns the function signature (argumen types) of this executable.
	virtual Signature getSignature() = 0;
};
typedef std::shared_ptr<Executable> ExecutablePtr;

} // namespace cmd

#endif /* _EXECUTABLE_H_ */
