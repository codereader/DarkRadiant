#ifndef _COMMANDSYSTEM_INTERFACE_H_
#define _COMMANDSYSTEM_INTERFACE_H_

#include <boost/python.hpp>
#include "iscript.h"

namespace script {

class CommandSystemInterface :
	public IScriptInterface
{
public:
	void execute(const std::string& buffer);

	// IScriptInterface implementation
	void registerInterface(boost::python::object& nspace);
};
typedef boost::shared_ptr<CommandSystemInterface> CommandSystemInterfacePtr;

} // namespace script

#endif /* _COMMANDSYSTEM_INTERFACE_H_ */
