#include "RadiantInterface.h"

#include <pybind11/pybind11.h>

#include "iscenegraph.h"
#include "entitylib.h"

namespace script 
{

ScriptEntityNode RadiantInterface::findEntityByClassname(const std::string& name)
{
	EntityNodeFindByClassnameWalker walker(name);
	GlobalSceneGraph().root()->traverse(walker);

	// Note: manage_new_object return value policy will take care of that raw pointer
	return ScriptEntityNode(walker.getEntityNode());
}

void RadiantInterface::registerInterface(py::module& scope, py::dict& globals)
{
	py::class_<RadiantInterface> radiant(scope, "RadiantInterface");
	radiant.def("findEntityByClassname", &RadiantInterface::findEntityByClassname);

	// Point the radiant variable to this
	globals["Radiant"] = this;
}

} // namespace script
