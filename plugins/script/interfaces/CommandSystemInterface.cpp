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

void CommandSystemInterface::addStatement(const std::string& statementName,
										  const std::string& str)
{
	GlobalCommandSystem().addStatement(statementName, str);
}

void CommandSystemInterface::removeCommand(const std::string& name) {
	GlobalCommandSystem().removeCommand(name);
}

void CommandSystemInterface::registerInterface(pybind11::module& scope)
{
	pybind11::class_<CommandSystemInterface> commandSys(scope, "CommandSystem");

	commandSys.def("execute", &CommandSystemInterface::execute);
	commandSys.def("addStatement", &CommandSystemInterface::addStatement);
	commandSys.def("removeCommand", &CommandSystemInterface::removeCommand);

	// Now point the Python variable "GlobalCommandSystem" to this instance
	scope.attr("GlobalCommandSystem") = this;
}

void CommandSystemInterface::registerInterface(boost::python::object& nspace) {
	// Define the CommandSystem interface
	nspace["GlobalCommandSystem"] =
		boost::python::class_<CommandSystemInterface>("GlobalCommandSystem")
		.def("execute", &CommandSystemInterface::execute)
		.def("addStatement", &CommandSystemInterface::addStatement)
		.def("removeCommand", &CommandSystemInterface::removeCommand)
	;

	// Now point the Python variable "GlobalCommandSystem" to this instance
	nspace["GlobalCommandSystem"] = boost::python::ptr(this);
}

} // namespace script
