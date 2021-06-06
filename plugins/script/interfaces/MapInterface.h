#pragma once

#include "iscript.h"
#include "iscriptinterface.h"

#include "SceneGraphInterface.h"

namespace script
{

/**
 * greebo: This class provides the script interface for the GlobalMap module.
 */
class MapInterface :
	public IScriptInterface
{
public:
    ScriptSceneNode getWorldSpawn();
    std::string getMapName();
    bool isModified();
    ScriptSceneNode getRoot();
    IMap::EditMode getEditMode();
    void setEditMode(IMap::EditMode mode);

    void showPointFile(const std::string& filePath);
    bool isPointTraceVisible();
    std::vector<std::string> getPointFileList();

	// IScriptInterface implementation
	void registerInterface(py::module& scope, py::dict& globals) override;
};

} // namespace script
