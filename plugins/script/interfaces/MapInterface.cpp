#include "MapInterface.h"

#include <pybind11/pybind11.h>

#include "imap.h"

namespace script 
{

ScriptSceneNode MapInterface::getWorldSpawn()
{
	return ScriptSceneNode(GlobalMapModule().getWorldspawn());
}

std::string MapInterface::getMapName()
{
	return GlobalMapModule().getMapName();
}

// IScriptInterface implementation
void MapInterface::registerInterface(py::module& scope, py::dict& globals)
{
	// Add the module declaration to the given python namespace
	py::class_<MapInterface> map(scope, "Map");

	map.def("getWorldSpawn", &MapInterface::getWorldSpawn);
	map.def("getMapName", &MapInterface::getMapName);

	// Now point the Python variable "GlobalMap" to this instance
	globals["GlobalMap"] = this;
}

} // namespace script
