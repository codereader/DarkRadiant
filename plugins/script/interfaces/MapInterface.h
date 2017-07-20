#pragma once

#include "iscript.h"

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

	// IScriptInterface implementation
	void registerInterface(py::module& scope, py::dict& globals) override;
};

} // namespace script
