#include "ScriptingSystemInterface.h"

namespace script
{

void ScriptingSystemInterface::registerBuiltinScriptCommand(py::object& cls)
{
    auto inspect = py::module::import("inspect");

    auto execute = cls.attr("execute");

    auto result = inspect.attr("signature")(execute);
    py::print(result);
}

void ScriptingSystemInterface::registerInterface(py::module& scope, py::dict& globals)
{
    // Add the ScriptingSystemInterface module declaration to the given python namespace
    py::class_<ScriptingSystemInterface> interface(scope, "ScriptingSystemInterface");

    interface.def("registerBuiltinScriptCommand", &ScriptingSystemInterface::registerBuiltinScriptCommand);

    // Now point the Python variable "GlobalScriptingSystem" to this instance
    globals["GlobalScriptingSystem"] = this;

    // Define a ScriptingSystem property in the darkradiant module
    scope.attr("ScriptingSystem") = this;
}

}
