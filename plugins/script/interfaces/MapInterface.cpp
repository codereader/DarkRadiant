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

IMap::EditMode MapInterface::getEditMode()
{
    return GlobalMapModule().getEditMode();
}

void MapInterface::setEditMode(IMap::EditMode mode)
{
    GlobalMapModule().setEditMode(mode);
}

ScriptSceneNode MapInterface::getRoot()
{
    return ScriptSceneNode(GlobalMapModule().getRoot());
}

bool MapInterface::isModified()
{
    return GlobalMapModule().isModified();
}

void MapInterface::showPointFile(const std::string& filePath)
{
    if (!filePath.empty())
    {
        GlobalMapModule().showPointFile(filePath);
    }
}

bool MapInterface::isPointTraceVisible()
{
    return GlobalMapModule().isPointTraceVisible();
}

std::vector<std::string> MapInterface::getPointFileList()
{
    std::vector<std::string> files;

    GlobalMapModule().forEachPointfile([&](const fs::path& path)
    {
        files.push_back(path.string());
    });

    return files;
}

// IScriptInterface implementation
void MapInterface::registerInterface(py::module& scope, py::dict& globals)
{
    // Add the module declaration to the given python namespace
	py::class_<MapInterface> map(scope, "Map");

    // Expose the edit mode enum
    py::enum_<IMap::EditMode>(scope, "MapEditMode")
        .value("Normal", IMap::EditMode::Normal)
        .value("Merge", IMap::EditMode::Merge)
        .export_values();

	map.def("getWorldSpawn", &MapInterface::getWorldSpawn);
	map.def("getMapName", &MapInterface::getMapName);
	map.def("getRoot", &MapInterface::getRoot);
	map.def("isModified", &MapInterface::isModified);

    map.def("getEditMode", &MapInterface::getEditMode);
    map.def("setEditMode", &MapInterface::setEditMode);

    map.def("showPointFile", &MapInterface::showPointFile);
    map.def("isPointTraceVisible", &MapInterface::isPointTraceVisible);
    map.def("getPointFileList", &MapInterface::getPointFileList);

	// Now point the Python variable "GlobalMap" to this instance
	globals["GlobalMap"] = this;
}

} // namespace script
