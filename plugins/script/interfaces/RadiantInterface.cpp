#include "RadiantInterface.h"

#include "iscenegraph.h"
#include "entitylib.h"

namespace script {

ScriptEntityNode RadiantInterface::findEntityByClassname(const std::string& name) {
	EntityNodeFindByClassnameWalker walker(name);
	GlobalSceneGraph().root()->traverse(walker);

	// Note: manage_new_object return value policy will take care of that raw pointer
	return ScriptEntityNode(walker.getEntityNode());
}

void RadiantInterface::registerInterface(boost::python::object& nspace) {
	nspace["Radiant"] = boost::python::class_<RadiantInterface>("Radiant")
		.def("findEntityByClassname", &RadiantInterface::findEntityByClassname)
	;

	// Point the radiant variable to this
	nspace["Radiant"] = boost::python::ptr(this);
}

} // namespace script
