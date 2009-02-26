#include "CommandSystemInterface.h"

#include "icommandsystem.h"

namespace script {

void CommandSystemInterface::execute(const std::string& buffer) {
	// Just wrap the call
	GlobalCommandSystem().execute(buffer);
}

void CommandSystemInterface::registerInterface(boost::python::object& nspace) {
	// Define the CommandSystem interface
	nspace["GlobalCommandSystem"] = 
		boost::python::class_<CommandSystemInterface>("GlobalCommandSystem")
		.def("execute", &CommandSystemInterface::execute)
	;

	// Now point the Python variable "GlobalCommandSystem" to this instance
	nspace["GlobalCommandSystem"] = boost::python::ptr(this);
}

} // namespace script
