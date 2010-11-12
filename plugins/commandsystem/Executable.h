#ifndef _EXECUTABLE_H_
#define _EXECUTABLE_H_

#include "icommandsystem.h"
#include <boost/shared_ptr.hpp>

namespace cmd {

class Executable
{
public:
    /**
	 * Destructor
	 */
	virtual ~Executable() {}

	/**
	 * greebo: Execute this object with the given set of arguments.
	 */
	virtual void execute(const ArgumentList& args) = 0;

	/**
	 * Returns the function signature (argumen types) of this executable.
	 */
	virtual Signature getSignature() = 0;
};
typedef boost::shared_ptr<Executable> ExecutablePtr;

} // namespace cmd

#endif /* _EXECUTABLE_H_ */
