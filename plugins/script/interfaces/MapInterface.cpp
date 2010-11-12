#include "MapInterface.h"

#include "imap.h"

namespace script {

ScriptSceneNode MapInterface::getWorldSpawn()
{
	return ScriptSceneNode(GlobalMapModule().getWorldspawn());
}

std::string MapInterface::getMapName()
{
	return GlobalMapModule().getMapName();
}

// IScriptInterface implementation
void MapInterface::registerInterface(boost::python::object& nspace) {
	// Add the module declaration to the given python namespace
	nspace["GlobalMap"] = boost::python::class_<MapInterface>("GlobalMap")
		.def("getWorldSpawn", &MapInterface::getWorldSpawn)
		.def("getMapName", &MapInterface::getMapName)
	;

	// Now point the Python variable "GlobalMap" to this instance
	nspace["GlobalMap"] = boost::python::ptr(this);
}

} // namespace script
