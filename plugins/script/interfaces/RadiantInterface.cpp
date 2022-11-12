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

ScriptEntityNode RadiantInterface::findEntityByName(const std::string& name)
{
    scene::INodePtr found;

	GlobalSceneGraph().root()->foreachNode([&] (const scene::INodePtr& node)
	{
	    if (Node_isEntity(node) && Node_getEntity(node)->getKeyValue("name") == name)
	    {
            found = node;
	    }

        return true;
	});

	// Note: manage_new_object return value policy will take care of that raw pointer
	return ScriptEntityNode(found);
}

void RadiantInterface::registerInterface(py::module& scope, py::dict& globals)
{
	py::class_<RadiantInterface> radiant(scope, "RadiantInterface");
	radiant.def("findEntityByClassname", &RadiantInterface::findEntityByClassname);
	radiant.def("findEntityByName", &RadiantInterface::findEntityByName);

	// Point the radiant variable to this
	globals["Radiant"] = this;
}

} // namespace script
