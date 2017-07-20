#include "CommandSystemInterface.h"

#include "icommandsystem.h"
#include <pybind11/pybind11.h>

namespace script 
{

void CommandSystemInterface::execute(const std::string& buffer)
{
	// Just wrap the call
	GlobalCommandSystem().execute(buffer);
}

void CommandSystemInterface::addStatement(const std::string& statementName, const std::string& str)
{
	GlobalCommandSystem().addStatement(statementName, str);
}

void CommandSystemInterface::removeCommand(const std::string& name) 
{
	GlobalCommandSystem().removeCommand(name);
}

void CommandSystemInterface::registerInterface(py::module& scope, py::dict& globals)
{
	pybind11::class_<CommandSystemInterface> commandSys(scope, "CommandSystem");

	commandSys.def("execute", &CommandSystemInterface::execute);
	commandSys.def("addStatement", &CommandSystemInterface::addStatement);
	commandSys.def("removeCommand", &CommandSystemInterface::removeCommand);

	// Now point the Python variable "GlobalCommandSystem" to this instance
	globals["GlobalCommandSystem"] = this;
}

} // namespace script
