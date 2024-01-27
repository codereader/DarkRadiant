#include "ScriptingSystemInterface.h"

namespace script
{

void ScriptingSystemInterface::registerBuiltinScriptCommand()
{
    
}

void ScriptingSystemInterface::registerInterface(py::module& scope, py::dict& globals)
{
    // Add the ScriptingSystemInterface module declaration to the given python namespace
    py::class_<ScriptingSystemInterface> interface(scope, "ScriptingSystemInterface");

    interface.def("registerBuiltinScriptCommand", &ScriptingSystemInterface::registerBuiltinScriptCommand);

    // Now point the Python variable "GlobalScriptingSystem" to this instance
    globals["GlobalScriptingSystem"] = this;
}

}
