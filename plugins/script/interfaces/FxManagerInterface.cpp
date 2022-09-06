#include "FxManagerInterface.h"

namespace script
{

ScriptFxDeclaration FxManagerInterface::findFx(const std::string& name)
{
    return ScriptFxDeclaration(GlobalFxManager().findFx(name));
}

void FxManagerInterface::registerInterface(py::module& scope, py::dict& globals) 
{
	// Add the EntityNode interface
	py::class_<ScriptFxDeclaration, ScriptDeclaration> fxDeclaration(scope, "Fx");
    fxDeclaration.def(py::init<const fx::IFxDeclaration::Ptr&>());

	// Add the FxManager module declaration to the given python namespace
	py::class_<FxManagerInterface> fxManager(scope, "FxManager");

	// Add both overloads to createEntity
    fxManager.def("findFx", &FxManagerInterface::findFx);

	// Now point the Python variable "GlobalFxManager" to this instance
	globals["GlobalFxManager"] = this;
}

}
